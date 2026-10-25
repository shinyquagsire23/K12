#include "k2stdint.h"
#include "nwsmodule.h"
#include "nwsscriptevent.h"
#include "nwmessagetypes.h"
#include "restype.h"

// The struct id the module IFO's player list tags each saved player with.
enum {
    GFF_STRUCT_ID_PLAYER = 0xbead,
};

#include "appmanager.h"
#include "serverexoapp.h"
#include "nwsarea.h"
#include "CExoLinkedList.h"
#include "nwscreature.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "pathfindinfo.h"
#include "worldtimer.h"
#include "nwscript.h"
#include "gameobjectarray.h"
#include "nwstrigger.h"
#include "nwsdoor.h"
#include "nwsobject.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "exoresman.h"
#include "resifo.h"
#include "swsareamap.h"
#include "resgff.h"
#include "serverinfo.h"
#include "serverexoappinternal.h"
#include "serveraimaster.h"
#include "partytables.h"
#include "factionmanager.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "exoresmaninternal.h"
#include "clientexoapp.h"
#include "nwsscriptevent.h"

extern CAppManager* g_pAppManager;
extern CExoResMan*  g_pExoResMan;
extern CTlkTable*   g_pTlkTable;
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

#include <stdio.h>
extern int   g_nSetMovieFrameRate;
extern float displayFPS;
extern int   g_bEnableAurPostString;
extern int   RenderAi;
extern "C++" char* SetFramesPerSecond(char* pszRate);

// Defined here so the destructor below can delete elements of these types.
struct NWModuleExpansion_st
{
    CExoLocString m_name;
    int           m_nID;
};

struct NWModuleCutScene_st
{
    CResRef       m_resName;
    unsigned int  m_nID;
};

struct NWPlayerListItem_st
{
    CExoString    m_sCommunityName;
    CExoLocString m_firstName;
    CExoLocString m_lastName;
    int           m_nIsPrimaryPlayer;
    char          _pad34[0x38 - 0x34];
};

#if !KOTOR1  // K2-only (no K1 counterpart)
void AddModuleOverrideResourceDirectories()
{
    char sBuf[200];
    for (int i = 0; ; i++)
    {
        const char* pOverride = GetOverridePath(i, false);
        if (pOverride == 0)
            break;
        sprintf(sBuf, "%s\\modules", pOverride);
        g_pExoResMan->AddResourceDirectory(CExoString(sBuf));
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void RemoveModuleOverrideResourceDirectories()
{
    char sBuf[200];
    for (int i = 0; ; i++)
    {
        const char* pOverride = GetOverridePath(i, false);
        if (pOverride == 0)
            break;
        sprintf(sBuf, "%s\\modules", pOverride);
        g_pExoResMan->RemoveResourceDirectory(CExoString(sBuf));
    }
}
#endif // !KOTOR1

CSWSModule::CSWSModule(const CExoString& sName, int bRequest, int nUnused)
    : CResHelper<CResIFO, 2014>(), CGameObject(OBJECT_TYPE_MODULE, OBJECT_INVALID),
      m_fActivateTargetX(0.0f), m_fActivateTargetY(0.0f), m_fActivateTargetZ(0.0f)
{
    K2_UNUSED(nUnused);
    CExoString sTag;
    m_pModuleID = new NWModuleID_st;
    m_pEntryInfo = new CSWSModuleEntryInfo;
    m_pAreaMap = new CSWSAreaMap;
    m_lastUpdateDay = 0;
    m_lastUpdateTod = 0;
    sTag = CExoString("CURRENTGAME:") + sName;
    m_sModuleResourceName = sTag;
    m_nUserDefinedEventNumber = 0;
    m_bModuleUpdateTurn = 1;
    m_nRestEventType = 0;
    m_oidRestingPlayer = OBJECT_INVALID;
    m_oidLevelUpPlayer = OBJECT_INVALID;
    m_oidActivatedItem = OBJECT_INVALID;
    m_oidItemActivator = OBJECT_INVALID;
    m_oidItemActivateTarget = OBJECT_INVALID;
    m_oidItemActivateTargetObject = OBJECT_INVALID;
    m_oidReserved1 = OBJECT_INVALID;
    m_oidEquippedItem = OBJECT_INVALID;
    m_oidEnteringClient = CGameObjectArray::INVALIDOBJID;
    m_oidLeavingClient = CGameObjectArray::INVALIDOBJID;
    m_oidAcquiredItem = CGameObjectArray::INVALIDOBJID;
    m_oidItemAcquirer = CGameObjectArray::INVALIDOBJID;
    m_oidUnAcquiredItem = CGameObjectArray::INVALIDOBJID;
    m_oidItemUnAcquirer = CGameObjectArray::INVALIDOBJID;
    m_oidDeadPlayer = CGameObjectArray::INVALIDOBJID;
    m_oidDyingPlayer = CGameObjectArray::INVALIDOBJID;
    m_oidSpawningPlayer = CGameObjectArray::INVALIDOBJID;
    m_nReserved1 = 0;
    m_bIsNWMFile = 0;
    m_sNWMResName = "";
    m_bIsSaveGame = 0;
    AddModuleResources(m_sModuleResourceName);
    m_nCheatValue = 2;
    m_nMinutesPerHour = 5;
    m_nDawnHour = 0;
    m_nDuskHour = 0;
    m_nStartMonth = 1;
    m_nXPScale = 10;
    m_nStartDay = 1;
    m_nStartHour = 0;
    m_nLastCalendarYear = CWorldTimer::DEFAULT_YEAR;
    m_nLastCalendarMonth = 1;
    m_nLastCalendarDay = 1;
    m_nLastCalendarHour = 0;
    m_nTimeOfDayElapsed = 0;
    m_nTimeOfDay = 0;
    m_bLoadFinished = 1;
    m_pSaveFile = 0;
    m_pSaveStruct = 0;
    m_pSaveGFF = 0;
    m_pSavedObjectIds = 0;
    CResRef cEmpty("");
    SetResRef(cEmpty, 0);
    CResRef cIFO("MODULE");
    SetResRef(cIFO, bRequest);
    g_pAppManager->m_pServerExoApp->GetObjectArray()->AddInternalObject(m_objectId, this, 0);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&m_nLastTimeDay, &m_nLastTimeTod);
    m_sTag = "";
    m_pCachedArea = 0;
    m_nTableCount = -1;
    m_cachedAreaId = OBJECT_INVALID;
}

// Empties the three owned-pointer lists before the member-destruction tail tears the containers down.
// The IFO handle is released here, rather than by ~CResHelper, for a module whose load never finished.
CSWSModule::~CSWSModule()
{
    if (m_bLoadFinished == 0)
        m_pRes->Release();

    while (m_ifoPlayerList.m_pInternal->m_pHead != 0)
        delete static_cast<NWPlayerListItem_st*>(m_ifoPlayerList.m_pInternal->RemoveHead());

    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    if (pArea != 0) {
        pArea->ClearObjects();
        delete pArea;
    }

    while (m_cutSceneList.m_pInternal->m_pHead != 0)
        delete static_cast<NWModuleCutScene_st*>(m_cutSceneList.m_pInternal->RemoveHead());

    while (m_expansionList.m_pInternal->m_pHead != 0)
        delete static_cast<NWModuleExpansion_st*>(m_expansionList.m_pInternal->RemoveHead());

    RemoveModuleResources(m_sModuleResourceName);
    m_sModuleResourceName = "";

    delete m_pEntryInfo;
    delete m_pModuleID;
    delete m_pAreaMap;
    m_pAreaMap = 0;

    g_pAppManager->m_pServerExoApp->GetObjectArray()->Delete(m_objectId);
    g_pAppManager->m_pServerExoApp->ClearLastModuleFound();

    m_pCachedArea = 0;
    m_cachedAreaId = OBJECT_INVALID;
}

void CSWSModule::ClearAreaCache()
{
    m_pCachedArea = 0;
    m_cachedAreaId = OBJECT_INVALID;
}

// Clears the same visited flag CSWSAreaPathFind's transition-trigger walk uses to mark an
// already-collected target.
void CSWSModule::ClearAreaVisitedFlags()
{
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    if (pArea != 0)
        pArea->m_bTransitionTargetMarked = 0;
}

// Sends each calendar/time field that moved since the last update, plus the time-of-day period
// re-classified from the dawn and dusk hours. When nothing moved, no message is sent at all.
void CSWSModule::UpdateTime(unsigned int nCalendarDay, unsigned int nTimeOfDay, unsigned int nTimeOfDayDelta)
{
    unsigned int nDayOfMonth;
    unsigned int nMonth;
    unsigned int nYear;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->ConvertFromCalendarDay(
        nCalendarDay, &nYear, &nMonth, &nDayOfMonth);
    unsigned int nHour;
    unsigned int nMinute;
    unsigned int nSecond;
    unsigned int nMs;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->ConvertFromTimeOfDay(
        nTimeOfDay, &nHour, &nMinute, &nSecond, &nMs);

    unsigned char nNewTimeOfDay;
    if (m_nDawnHour == m_nDuskHour)
        nNewTimeOfDay = 1;
    else if (nHour == m_nDawnHour)
        nNewTimeOfDay = 3;
    else if (nHour == m_nDuskHour)
        nNewTimeOfDay = 4;
    else if (m_nDawnHour < m_nDuskHour)
        nNewTimeOfDay = (nHour >= m_nDawnHour && nHour <= m_nDuskHour) ? 1 : 2;
    else
        if (nHour >= m_nDuskHour && nHour <= m_nDawnHour)
            nNewTimeOfDay = 2;
        else
            nNewTimeOfDay = 1;

    unsigned char nOldTimeOfDay = m_nTimeOfDay;
    m_nTimeOfDayElapsed =
        (static_cast<unsigned char>(nOldTimeOfDay - 3) <= 1) ? m_nTimeOfDayElapsed + nTimeOfDayDelta : 0;

    unsigned char nMask;
    if (nOldTimeOfDay != nNewTimeOfDay) {
        nMask = MSG_MODULE_TIME_TIME_OF_DAY;
        m_nTimeOfDay = nNewTimeOfDay;
    } else {
        nMask = 0;
    }
    if (m_nLastCalendarHour != nHour) {
        nMask |= MSG_MODULE_TIME_HOUR;
        m_nLastCalendarHour = nHour;
    }
    if (m_nLastCalendarDay != nDayOfMonth) {
        nMask |= MSG_MODULE_TIME_DAY;
        m_nLastCalendarDay = nDayOfMonth;
    }
    if (m_nLastCalendarMonth != nMonth) {
        nMask |= MSG_MODULE_TIME_MONTH;
        m_nLastCalendarMonth = nMonth;
    }
    if (m_nLastCalendarYear != nYear) {
        nMask |= MSG_MODULE_TIME_YEAR;
        m_nLastCalendarYear = nYear;
    }
    if (nMask == 0)
        return;

    CExoLinkedList<CSWSClient>* ppPlayerList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    CExoLinkedListNode* pos = ppPlayerList->m_pInternal->m_pHead;
    while (pos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayerList->m_pInternal->GetAtPos(pos));
        if (pPlayer != 0) {
            g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerModuleUpdate_Time(
                pPlayer, nMask, nYear, nMonth, nDayOfMonth, nHour, m_nTimeOfDay,
                m_nTimeOfDayElapsed);
        }
        ppPlayerList->m_pInternal->GetNext(pos);
    }
}

// Sweeps InterAreaDFS at increasing depth from the module's own area until the destination turns
// up, then turns the head of the route into a local goal.
int CSWSModule::ComputeInterAreaPath(CPathfindInformation* pPathInfo)
{
    pPathInfo->m_nInterAreaCount = 10;
    pPathInfo->m_paInterAreaIds = new unsigned long long[10];
    pPathInfo->m_paInterAreaTransitionIds = new unsigned long long[9];
    pPathInfo->m_bInterAreaPathFound = 0;
    int i = 1;
    bool bOk = true;
    while (bOk) {
        unsigned long long* pIds = pPathInfo->m_paInterAreaIds;
        pIds[0] = pPathInfo->m_oidStartArea;
        CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
        if (pArea != 0)
            pArea->m_bTransitionTargetMarked = 0;
        if (InterAreaDFS(0, i, pPathInfo) == 1)
            pPathInfo->m_bInterAreaPathFound = 1;
        i++;
        bOk = pPathInfo->m_bInterAreaPathFound == 0;
        if (i == 10)
            break;
    }
    if (bOk)
        return 0;
    SetIntraAreaGoal(pPathInfo);
    return 1;
}

// Turns the first transition of an inter-area route into a local goal in the destination area, so
// the next plot treats the doorway as a same-area target.
void CSWSModule::SetIntraAreaGoal(CPathfindInformation* pPathInfo)
{
    if (pPathInfo->m_nInterAreaCount < 2)
        return;

    pPathInfo->m_oidSavedGoalArea = pPathInfo->m_oidGoalArea;
    pPathInfo->m_savedGoal = pPathInfo->m_goal;

    CGameObject* pGameObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
            pPathInfo->m_paInterAreaTransitionIds[0], &pGameObject) != CGameObjectArray::SUCCESS)
        return;

    CSWSObject* pTransition;
    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
        CSWSTrigger* pTrigger = pGameObject->AsSWSTrigger();
        Vector vFrom(pPathInfo->m_vStartPoint.x, pPathInfo->m_vStartPoint.y, pPathInfo->m_vStartPoint.z);
        Vector vClosest;
        if (pTrigger->GetClosestPointWithinTrigger(vFrom, &vClosest) != 1)
            return;
        pPathInfo->m_goal.m_vPoint.x = vClosest.x;
        pPathInfo->m_goal.m_vPoint.y = vClosest.y;
        pPathInfo->m_goal.m_vPoint.z = vClosest.z;
        pPathInfo->m_goal.m_fRange = 0.0f;
        pTransition = pTrigger;
    } else if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
        CSWSDoor* pDoor = pGameObject->AsSWSDoor();
        Vector vFrom(pPathInfo->m_vStartPoint.x, pPathInfo->m_vStartPoint.y, pPathInfo->m_vStartPoint.z);
        pPathInfo->m_goal.m_vPoint = pDoor->GetNearestActionPoint(vFrom, 1);
        pPathInfo->m_goal.m_fRange = pPathInfo->m_fPersonalSpace + 0.5f;
        pTransition = pDoor;
    } else {
        return;
    }

    pPathInfo->ComputeStepTolerance();
    pPathInfo->m_oidGoalArea = pPathInfo->m_oidStartArea;
    pPathInfo->m_oidGoalObject = pTransition->m_objectId;
}

// Depth-first half of the inter-area route search. The route arrays are walked head-first: index nFrom
// holds the area this level came from, so a hit on the destination just trims the count and reports
// success; otherwise every successor transition is tried in turn.
int CSWSModule::InterAreaDFS(int nFrom, int nDepth, CPathfindInformation* pPathInfo)
{
    if (pPathInfo->m_paInterAreaIds[nFrom] == pPathInfo->m_oidGoalArea) {
        pPathInfo->m_nInterAreaCount = nFrom + 1;
        return 1;
    }
    if (nFrom >= nDepth)
        return 0;

    unsigned long long* paGenerated = 0;
    int nSuccessors = GenerateInterAreaDFSSuccessors(nFrom, pPathInfo, &paGenerated);
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    unsigned long long* paSuccessors = paGenerated;

    int nResult = 0;
    if (nSuccessors >= 1) {
        for (unsigned int i = 0; i < static_cast<unsigned int>(nSuccessors); i++) {
            pPathInfo->m_paInterAreaTransitionIds[nFrom] = paSuccessors[i];

            CGameObject* pGameObject;
            if (pObjectArray->GetGameObject(paSuccessors[i], &pGameObject)
                    != CGameObjectArray::SUCCESS)
                continue;

            CSWSArea* pArea;
            if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
                CSWSTrigger* pTrigger = pGameObject->AsSWSTrigger();
                if (pTrigger->m_bAreaTransition == 0)
                    continue;
                if (pObjectArray->GetGameObject(pTrigger->GetTargetArea(), &pGameObject)
                        != CGameObjectArray::SUCCESS)
                    continue;
                pArea = pGameObject->AsSWSArea();
                if (pArea == 0)
                    continue;
            } else if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
                CSWSDoor* pDoor = pGameObject->AsSWSDoor();
                if (pDoor == 0)
                    continue;
                if (pDoor->GetIsLinked() == 0)
                    continue;
                CSWSObject* pLinkedDoor = pDoor->GetLinkedObject();
                if (pLinkedDoor == 0)
                    continue;
                pArea = static_cast<CSWSArea*>(pLinkedDoor->GetArea());
                if (pArea == 0)
                    continue;
            } else {
                continue;
            }

            pPathInfo->m_paInterAreaIds[nFrom + 1] = pArea->m_objectId;
            if (InterAreaDFS(nFrom + 1, nDepth, pPathInfo) == 1) {
                nResult = 1;
                break;
            }
        }
    }

    if (paSuccessors != 0)
        operator delete[](paSuccessors);
    return nResult;
}

// Collects the areas reachable from the route slot nFrom in one hop. The module's cached area is
// asked for its area-transition triggers and linked doors, first for a count and then into a fresh
// array that is freed with delete[] afterwards.
int CSWSModule::GenerateInterAreaDFSSuccessors(int nFrom, CPathfindInformation* pPathInfo,
                                              unsigned long long** ppaSuccessors)
{
    K2_UNUSED(nFrom);
    CSWSArea* pArea;
    if (m_cachedAreaId != m_oidArea)
    {
        pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
        m_pCachedArea = pArea;
    }
    else
    {
        pArea = m_pCachedArea;
    }

    if (pArea == 0)
    {
        m_pCachedArea = 0;
        m_cachedAreaId = OBJECT_INVALID;
        return 0;
    }

    m_cachedAreaId = m_oidArea;

    int nCount = pArea->CountAreaTransitionTriggers(
        0, pPathInfo->m_nTraversedTransitions, pPathInfo->m_paTraversedTransitions);
    if (nCount == 0)
        return 0;

    unsigned long long* paSuccessors = new unsigned long long[nCount];
    *ppaSuccessors = paSuccessors;
    return pArea->CountAreaTransitionTriggers(
        paSuccessors, pPathInfo->m_nTraversedTransitions, pPathInfo->m_paTraversedTransitions);
}

// Builds or reuses an inter-area route, plots the current area, and on failure extends the route by
// one leg and retries; the route layer under PlotPath.
unsigned int CSWSModule::PlotInterAreaPath(CPathfindInformation* pPathInfo, unsigned int nTimeOut)
{
    unsigned int nResult = nTimeOut;
    unsigned long long nGoalAreaId = pPathInfo->m_oidGoalArea;

    if (nGoalAreaId == pPathInfo->m_oidStartArea)
    {
        if (pPathInfo->m_paInterAreaIds == 0)
        {
            pPathInfo->m_bInterAreaPathFound = 1;
            pPathInfo->m_nInterAreaCount = 1;
            unsigned long long* paIds = new unsigned long long[1];
            paIds[0] = nGoalAreaId;
            pPathInfo->m_paInterAreaIds = paIds;
            pPathInfo->m_paInterAreaTransitionIds = 0;
        }
    }
    else if (pPathInfo->m_paInterAreaIds != 0)
    {
        pPathInfo->FlipTempEndPoints();
    }
    else if (ComputeInterAreaPath(pPathInfo) == 0)
    {
        return CSWSObject::ACTION_FAILED;
    }

    nResult = PlotPathInArea(pPathInfo, nResult);

    if (nResult == static_cast<unsigned int>(CSWSObject::ACTION_FAILED))
    {
        if (pPathInfo->m_nInterAreaCount < 2)
            return nResult;

        int nCount = pPathInfo->m_nTraversedTransitions;
        unsigned long long* paNew = new unsigned long long[nCount + 1];
        // The old count is published before the old pointer is freed.
        if (nCount > 0)
        {
            int i = 0;
            do
            {
                paNew[i] = pPathInfo->m_paTraversedTransitions[i];
            }
            while (++i < nCount);
        }
        paNew[nCount] = pPathInfo->m_paInterAreaTransitionIds[0];
        pPathInfo->m_nTraversedTransitions = nCount + 1;
        if (pPathInfo->m_paTraversedTransitions != 0)
            operator delete[](pPathInfo->m_paTraversedTransitions);
        pPathInfo->m_paTraversedTransitions = paNew;

        pPathInfo->ResetInterAreaPathSearchData();
        pPathInfo->FlipTempEndPoints();
        if (ComputeInterAreaPath(pPathInfo) == 0)
            return nResult;
        nResult = CSWSObject::ACTION_IN_PROGRESS;
    }
    else if (pPathInfo->m_nInterAreaCount < 2)
    {
        return nResult;
    }

    pPathInfo->FlipTempEndPoints();
    return nResult;
}

// Plots a module path (the entry point): tears down the previous search's scratch state on a
// restart cycle, runs the inter-area layer, then the RenderAi debug hook.
int CSWSModule::PlotPath(CPathfindInformation* pPathInfo, unsigned int nPathId)
{
    unsigned int nResult = nPathId;

    if (pPathInfo == 0)
        return CSWSObject::ACTION_FAILED;

    if (pPathInfo->m_bNewPathRequested != 1)
    {
        if (pPathInfo->m_bNewPathRequested == 0)
        {
            if (pPathInfo->m_bPlotFinished == 1)
                return CSWSObject::ACTION_COMPLETE;
        }
    }
    else
    {
        if (pPathInfo->m_bKeepInterAreaRoute == 1)
        {
            pPathInfo->ResetInterTileSearchData();
            pPathInfo->ResetGridSearchData();
            pPathInfo->m_bKeepInterAreaRoute = 0;
        }
        else
        {
            int nSaved = pPathInfo->m_bPointsReversed;
            pPathInfo->ResetInterAreaSearchData();
            pPathInfo->m_bPointsReversed = nSaved;
        }
        pPathInfo->m_nPlotInAreaPasses = 0;
        pPathInfo->m_bPlotFinished = 0;
        pPathInfo->m_bNewPathRequested = 0;
    }

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->m_bRanInterAreaPath = 1;

    nResult = PlotInterAreaPath(pPathInfo, nResult);

    if (RenderAi)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPathInfo->m_oidCreature);
        if (nResult == static_cast<unsigned int>(CSWSObject::ACTION_COMPLETE))
        {
            pCreature->ComputeTotalWalkDistance();
            pCreature->ComputeTotalWalkDistance();
        }
    }

    return nResult;
}

// Plots the intra-area half of the path: probes the direct line from->to (cached in
// m_nDirectLineCode), pulls the destination back to the sight-range stand-off when the walk must go
// straight at the point, and on failure falls back to a bisected search plus a second,
// arg-different probe.
unsigned int CSWSModule::PlotPathInArea(CPathfindInformation* pPathInfo, unsigned int nTimeOut)
{
    int nResult = CSWSObject::ACTION_FAILED;
    float fHitFraction = pPathInfo->m_fPersonalSpace;
    float fRadius = pPathInfo->m_fHeightAlias;
    float fFromX = pPathInfo->m_vStartPoint.x;
    float fFromY = pPathInfo->m_vStartPoint.y;
    float fToX = pPathInfo->m_goal.m_vPoint.x;
    float fToY = pPathInfo->m_goal.m_vPoint.y;

    CSWSArea* pArea = GetArea();
    if (pArea == 0)
    {
        m_pCachedArea = 0;
        m_cachedAreaId = OBJECT_INVALID;
        return nResult;
    }

    m_cachedAreaId = m_oidArea;

    if (pArea->m_objectId != pPathInfo->m_oidStartArea)
        return nResult;

    ++pPathInfo->m_nPlotInAreaPasses;
    pArea->m_pActivePathfindInfo = pPathInfo;

    if (pPathInfo->m_goal.m_fRange > 0.001f && pPathInfo->m_bCheckSightToTarget == 0)
    {
        float fDx = fToX - fFromX;
        float fDy = fToY - fFromY;
        float fDist2 = fDx * fDx + fDy * fDy + 0.0f;

        if (fDist2 >= 0.0001f * 0.0001f)
        {
            float fStandOff;
            if (fDist2 > pPathInfo->m_goal.m_fRange * pPathInfo->m_goal.m_fRange)
                fStandOff = pPathInfo->m_goal.m_fRange - 0.001f;
            else
                fStandOff = __builtin_sqrtf(fDist2);

            Vector vDir = normalize(Vector(fFromX - fToX, fFromY - fToY, 0.0f));

            fToX += fStandOff * vDir.x;
            fToY += fStandOff * vDir.y;
        }
    }

    if (pPathInfo->m_nPlotInAreaPasses == 1)
    {
        nResult = pArea->TestDirectLine(Vector(fFromX, fFromY, 0.0f),
                                        Vector(fToX, fToY, 0.0f), fHitFraction, fRadius, 1, 0);
        pPathInfo->m_nDirectLineCode = nResult;
    }
    else
    {
        nResult = pPathInfo->m_nDirectLineCode;
    }

    if (pPathInfo->m_bStraightLineOnly == 1)
    {
        if (nResult != 1 && pPathInfo->m_bPointsReversed == 0)
        {
            float fDx = fToX - fFromX;
            float fDy = fToY - fFromY;
            float fT = 0.5f;
            float fStep = 0.25f;
            float fFoundX = 0.0f;
            float fFoundY = 0.0f;

            nResult = 0;
            do
            {
                Vector vTest(fFromX + fDx * fT, fFromY + fDy * fT, 0.0f);

                if (pArea->TestDirectLine(Vector(fFromX, fFromY, 0.0f), vTest, fHitFraction,
                                          fRadius, 1, 0) == 1)
                {
                    fT += fStep;
                    nResult = 1;
                    fFoundX = vTest.x;
                    fFoundY = vTest.y;
                }
                else
                {
                    fT -= fStep;
                }
                fStep *= 0.5f;
            }
            while (fStep > 0.005f);

            if (nResult == 1)
            {
                fToX = fFoundX;
                fToY = fFoundY;
            }
            else
            {
                nResult = -4;
            }
        }

        pPathInfo->m_bGoalWithinReach = 0;
        pPathInfo->m_bStraightLineOnly = 0;
    }

    const int nLineCode = nResult;
    if (nResult == 1)
    {
        pPathInfo->m_nWayPointFloats = 4;
        float* paPath = new float[4];
        paPath[0] = fFromX;
        paPath[1] = fFromY;
        paPath[2] = fToX;
        paPath[3] = fToY;
        pPathInfo->m_paWayPoints = paPath;
        nResult = CSWSObject::ACTION_COMPLETE;
    }
    else
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPathInfo->m_oidCreature);

        if (pCreature == 0 || pCreature->m_aiLevel != 0)
        {
            if (pPathInfo->m_bGoalWithinReach == 0)
            {
                if (static_cast<unsigned int>(nResult - 1) >= static_cast<unsigned int>(-4))
                {
                    if (pArea->m_nPathPointCount == 0)
                    {
                        if (RenderAi)
                            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                                pPathInfo->m_oidCreature);
                        nResult = pArea->PlotGridPath(pPathInfo, nTimeOut);
                    }
                    else
                    {
                        if (RenderAi)
                            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                                pPathInfo->m_oidCreature);
                        nResult = pArea->PlotPathPointPath(pPathInfo, nTimeOut);
                    }
                }
                else
                {
                    nResult = CSWSObject::ACTION_FAILED;
                }
            }
            else
            {
                nResult = pArea->PlotGridPath(pPathInfo, nTimeOut);

                if (RenderAi)
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                        pPathInfo->m_oidCreature);
            }

            if (RenderAi)
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPathInfo->m_oidCreature);

            if (nResult == CSWSObject::ACTION_IN_PROGRESS && pPathInfo->m_nPlotInAreaPasses > 100)
                nResult = CSWSObject::ACTION_FAILED;
        }
        else
        {
            nResult = CSWSObject::ACTION_FAILED;
        }
    }

    if (nResult == CSWSObject::ACTION_FAILED)
    {
        if (pPathInfo->m_bClientSidePath == 0)
        {
            float fFromX2 = pPathInfo->m_vStartPoint.x;
            float fFromY2 = pPathInfo->m_vStartPoint.y;
            float fT = 0.5f;
            float fStep = 0.25f;
            float fFoundX2 = 0.0f;
            float fFoundY2 = 0.0f;
            int bFound = 0;

            pPathInfo->m_bPointsReversed = 0;

            if (nLineCode != -1)
            {
                Vector vFrom(fFromX2, fFromY2, 0.0f);
                Vector vDelta = Vector(pPathInfo->m_goal.m_vPoint.x, pPathInfo->m_goal.m_vPoint.y, 0.0f) - vFrom;
                do
                {
                    Vector vTest = vFrom + vDelta * fT;

                    if (pArea->TestDirectLine(vFrom, vTest, fHitFraction, fRadius, 0, 0) == 1)
                    {
                        fT += fStep;
                        bFound = 1;
                        fFoundX2 = vTest.x;
                        fFoundY2 = vTest.y;
                    }
                    else
                    {
                        fT -= fStep;
                    }
                    fStep *= 0.5f;
                }
                while (fStep > 0.005f);
            }

            if (bFound)
            {
                pPathInfo->m_nWayPointFloats = 4;
                float* paPath = new float[4];
                paPath[0] = fFromX2;
                paPath[1] = fFromY2;
                paPath[2] = fFoundX2;
                paPath[3] = fFoundY2;
                pPathInfo->m_paWayPoints = paPath;
                nResult = CSWSObject::ACTION_COMPLETE;
            }
        }

#if !KOTOR1  // K1: a failed plot leaves the path-point state where it was
        pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_STARTPOINTS;
#endif
    }

    if (nResult == CSWSObject::ACTION_COMPLETE)
    {
        if (pPathInfo->m_bPointsReversed == 1)
        {
            int nCount = pPathInfo->m_nWayPointFloats;
            float* paOld = pPathInfo->m_paWayPoints;
            float* paNew = new float[nCount];

            for (int i = 0, j = nCount - 2; i < nCount; i += 2, j -= 2)
            {
                K2_ASSUME(i + 1 < nCount && j >= 0);
                paNew[i] = paOld[j];
                paNew[i + 1] = paOld[j + 1];
            }

            pPathInfo->m_paWayPoints = paNew;
            if (paOld != 0)
                operator delete[](paOld);
            pPathInfo->m_bPointsReversed = 0;
            pPathInfo->FlipStartEndPoints();
        }
    }

    if (nResult == CSWSObject::ACTION_FAILED || nResult == CSWSObject::ACTION_COMPLETE)
        pPathInfo->m_bPlotFinished = 1;

    return nResult;
}

// Releases the time-stop pause when no creature on the active exclusion list still carries an
// EFFECT_TIMESTOP.
void CSWSModule::TimeStopSanityCheck()
{
    CExoLinkedList<unsigned long long>* pList =
        g_pAppManager->m_pServerExoApp->GetActiveExclusionList();
    CExoLinkedListNode* pos = pList->m_pInternal->m_pHead;
    int bFound = 0;
    while (pos != 0) {
        unsigned long long objectId = reinterpret_cast<unsigned long long>(pList->m_pInternal->GetAtPos(pos));
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
        bFound = 0;
        if (pCreature != 0) {
            for (int64_t i = 0; i < pCreature->m_effects.m_count; i++) {
                if (pCreature->m_effects.m_pArray[i]->m_subType
                        == CSWSEffectListHandler::EFFECT_TIMESTOP)
                    bFound = 1;
            }
        }
        pList->m_pInternal->GetNext(pos);
        if (pos == 0 || bFound != 0)
            break;
    }
    if (bFound == 0)
        g_pAppManager->m_pServerExoApp->TogglePauseState(1);
}

// Alternates the module's own AIUpdate with the area's, one frame each.
void CSWSModule::DoUpdate()
{
    if (m_bModuleUpdateTurn != 0) {
        m_bModuleUpdateTurn = 0;
        AIUpdate();
        return;
    }
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    if (pArea != 0)
        pArea->AIUpdate();
    m_bModuleUpdateTurn = 1;
}


void CSWSModule::SavePrimaryPlayerInfo(CSWSCreature* pCreature, const CExoString& path)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pCreature->m_objectId);
    CResGFF* pGff = new CResGFF();
    CResStruct* pStruct = new CResStruct();
    if (pGff->CreateGFFFile(pStruct, CExoString("BIC "), CExoString("V2.0")))
    {
        CExoString sFile;
        pGff->WriteFieldCExoString(pStruct, pPlayer->GetPlayerName(), const_cast<char*>("Mod_CommntyName"));
        pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(pPlayer->m_bIsPrimaryPlayer), const_cast<char*>("Mod_IsPrimaryPlr"));
        pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(pCreature->m_objectId), const_cast<char*>("ObjectId"));
        pCreature->SaveCreature(pGff, pStruct);
#if KOTOR1  // K1 (Android port): "/" separators
        sFile = path + "/" + "Player";
#else
        sFile = path + "\\" + "Player";
#endif
        pGff->WriteGFFFile(sFile, RESTYPE_BIC);
    }
    delete pGff;
}


// The module's AI-master event sink. EVENT_TIMED_EVENT resumes a suspended script situation;
// EVENT_SIGNAL_EVENT dispatches the event sub-type onto one of the fifteen module scripts.
void CSWSModule::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                              unsigned int nDay, unsigned int nTod)
{
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT))
    {
        if (pData != 0)
        {
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
        }
        return;
    }
    if (nEventType != static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) return;

    CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
    switch (pEvent->m_eventType)
    {
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
        m_lastUpdateDay = nDay;
        m_lastUpdateTod = nTod;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_HEARTBEAT], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH:
        m_oidDeadPlayer = nCallerId;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_PLAYER_DEATH], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
        m_nUserDefinedEventNumber = pEvent->GetInteger(0);
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_USER_DEFINED], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_ENTER:
        m_oidEnteringClient = pEvent->GetObjectID(0);
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_CLIENT_ENTER], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_EXIT:
        m_oidLeavingClient = pEvent->GetObjectID(0);
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_CLIENT_LEAVE], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_START:
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_MODULE_START], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_LOAD:
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_MODULE_LOAD], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_ACTIVATE_ITEM: {
        m_oidActivatedItem = pEvent->GetObjectID(0);
        m_oidItemActivator = pEvent->GetObjectID(1);
        m_oidItemActivateTarget = pEvent->GetObjectID(2);
        m_oidItemActivateTargetObject = pEvent->GetObjectID(3);
        float fX = pEvent->GetFloat(0);
        float fY = pEvent->GetFloat(1);
        float fZ = pEvent->GetFloat(2);
        m_fActivateTargetX = fX;
        m_fActivateTargetY = fY;
        m_fActivateTargetZ = fZ;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_ACTIVATE_ITEM], m_objectId, 1);
        break;
    }
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM:  // The acquirer is read before the item
        m_oidItemAcquirer = pEvent->GetObjectID(1);
        m_oidAcquiredItem = pEvent->GetObjectID(0);
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_ACQUIRE_ITEM], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM:  // The loser is the event's caller, not a parameter
        m_oidUnAcquiredItem = pEvent->GetObjectID(0);
        m_oidItemUnAcquirer = nCallerId;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_UNACQUIRE_ITEM], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING:
        m_oidDyingPlayer = nCallerId;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_PLAYER_DYING], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_RESPAWN_BUTTON_PRESSED:
        m_oidSpawningPlayer = nCallerId;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_SPAWN_BUTTON_DOWN], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST:
        m_oidRestingPlayer = nCallerId;
        m_nRestEventType = static_cast<unsigned char>(pEvent->GetInteger(0));
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_PLAYER_REST], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_DESTROYPLAYERCREATURE: {  // Destroys the caller; the one sub-type with no module script
        CGameObject* pObject = 0;
        g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nCallerId, &pObject);
        // Only creatures are reaped; every other object type is left in the array.
        if (pObject != 0 && pObject->AsSWSCreature() != 0)
        {
            g_pAppManager->m_pServerExoApp->GetObjectArray()->Delete(nCallerId);
        }
        break;
    }
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_LEVEL_UP:
        m_oidLevelUpPlayer = nCallerId;
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_PLAYER_LEVEL_UP], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_EQUIP_ITEM:
        m_oidEquippedItem = pEvent->GetObjectID(0);
        g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_EQUIP_ITEM], m_objectId, 1);
        break;
    default:
        break;
    }
    delete pEvent;
}

// The module's own tick, run on alternate frames. A time-of-day delta of 375 ticks or more is a
// heartbeat; otherwise the world time is fanned out to the players.
int CSWSModule::AIUpdate()
{
    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nDayDelta;
    unsigned int nTodDelta;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_lastUpdateDay, m_lastUpdateTod, &nDayDelta, &nTodDelta);
    if (nTodDelta / 16 >= 375) {
        if (m_lastUpdateTod != 0)
            g_pVirtualMachine->RunScript(&m_aScript[SCRIPT_HEARTBEAT], m_objectId, 1);
        m_lastUpdateDay = nDay;
        m_lastUpdateTod = nTod;
        return 1;
    }
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastTimeDay, m_nLastTimeTod, &nDayDelta, &nTodDelta);
    UpdateTime(nDay, nTod, nTodDelta);
    m_nLastTimeDay = nDay;
    m_nLastTimeTod = nTod;
    if (g_pAppManager->m_pServerExoApp->GetActivePauseState() == 1)
        TimeStopSanityCheck();
    return 0;
}

void CSWSModule::PostProcess() {}

void CSWSModule::UnloadModule() {}

CSWSModule* CSWSModule::AsSWSModule()
{
    return this;
}

// The module's current area, memoized against the area's object id; a failed lookup is cached too.
CSWSArea* CSWSModule::GetArea()
{
    if (m_cachedAreaId != m_oidArea)
        m_pCachedArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    if (m_pCachedArea)
        m_cachedAreaId = m_oidArea;
    else
    {
        m_pCachedArea = 0;
        m_cachedAreaId = OBJECT_INVALID;
    }
    return m_pCachedArea;
}

unsigned long long CSWSModule::GetWaypoint(const CExoString& sTag)
{
    return FindObjectByTagTypeOrdinal(sTag, CGameObject::OBJECT_TYPE_WAYPOINT, 0);
}

// The module-level "/TestPathPoint" console hook.
void CSWSModule::TestPathPoint(char* pParams)
{
    float fHitFraction = static_cast<float>(atof(pParams));
    CSWSArea* pArea = GetArea();
    if (pArea)
        pArea->TestPathPoint(fHitFraction, 1);
}

// Reads the last-shipped world-clock stamp back out through six out-params.
void CSWSModule::GetTime(unsigned int* pDate, unsigned int* pHour, unsigned int* pMinute,
                         unsigned int* pSecond, unsigned char* pMs, unsigned int* pUnk)
{
    *pDate = m_nLastCalendarYear;
    *pHour = m_nLastCalendarMonth;
    *pMinute = m_nLastCalendarDay;
    *pSecond = m_nLastCalendarHour;
    *pMs = m_nTimeOfDay;
    *pUnk = m_nTimeOfDayElapsed;
}

// Tests membership in the limbo list with a reverse scan of the id array, newest entries first.
int CSWSModule::IsObjectInLimbo(unsigned long long objectId)
{
    for (long long n = m_limboList.m_count; n > 0; n--)
        if (m_limboList.m_pArray[n - 1] == objectId)
            return 1;
    return 0;
}

// Binary search for a tag in the sorted table. Returns the index of the first row that does not
// sort before sTag, or the count when sTag sorts after every row: both the exact hit for a unique
// tag and the start of its same-tag run for a duplicated one.
int CSWSModule::FindTagPositionInTable(char* sTag)
{
    int nLast = m_tagTable.m_count - 1;
    int nFirst = 0;
    int nTop = nLast;

    while (nFirst == nTop || nFirst < nLast)
    {
        int nMiddle = (nFirst + nLast) >> 1;
        if (strcmp(m_tagTable.m_pArray[nMiddle].m_sTag, sTag) >= 0)
        {
            if (nMiddle == nLast)
                return nLast;
            nLast = nMiddle;
        }
        else
            nFirst = nMiddle + 1;
    }
    return nFirst;
}

// Finds the tag's run in the table, walks it counting only the rows that still resolve to a live
// object of nType, and returns the nOrdinal-th one's id.
unsigned long long CSWSModule::FindObjectByTagTypeOrdinal(const CExoString& tag, int nType,
                                                          unsigned int nOrdinal)
{
    CExoString sLower = tag.LowerCase();
    unsigned long long oid = OBJECT_INVALID;
    int nIndex = FindTagPositionInTable(sLower.CStr());
    CGameObjectArray* pObjectArray = 0;
    CGameObject* pObject;
    if (nIndex >= m_tagTable.m_count)
        return oid;

    for (; nIndex < m_tagTable.m_count; nIndex++)
    {
        if (!(sLower == m_tagTable.m_pArray[nIndex].m_sTag))
            break;

        if (pObjectArray == 0)
            pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();

        if (CGameObjectArray::SUCCESS ==
            pObjectArray->GetGameObject(m_tagTable.m_pArray[nIndex].m_oid, &pObject))
        {
            if (pObject->m_objectType == nType)
            {
                if (nOrdinal == 0)
                {
                    oid = m_tagTable.m_pArray[nIndex].m_oid;
                    break;
                }
                nOrdinal--;
            }
        }
    }
    return oid;
}

// Looks up an object by tag and ordinal with no type filter, taking the insertion point plus
// nOrdinal, guarded by a second tag comparison.
unsigned long long CSWSModule::FindObjectByTagOrdinal(const CExoString& tag, unsigned int nOrdinal)
{
    CExoString sLower(tag);
    int nIndex = FindTagPositionInTable(sLower.CStr()) + static_cast<int>(nOrdinal);
    if (nIndex >= m_tagTable.m_count)
        return OBJECT_INVALID;
    if (!(sLower == m_tagTable.m_pArray[nIndex].m_sTag))
        return OBJECT_INVALID;
    return m_tagTable.m_pArray[nIndex].m_oid;
}

// Inserts {tag, id} at the end of the tag's own run, so a second object with the same tag gets the
// higher ordinal.
bool CSWSModule::AddObjectToLookupTable(const CExoString& tag, unsigned long long objectId)
{
    CExoString sLower(tag);
    int nIndex = FindTagPositionInTable(sLower.CStr());

    CSWSTagNode node;
    node.m_oid = objectId;
    __strcpy_chk(node.m_sTag, sLower.CStr(), sizeof(node.m_sTag));

    while (nIndex < m_tagTable.m_count && tag == m_tagTable.m_pArray[nIndex].m_sTag)
        nIndex++;

    m_tagTable.Insert(node, nIndex);
    return true;
}

// Erases the row that carries both this tag and this id, from the tag's insertion point onward.
bool CSWSModule::RemoveObjectFromLookupTable(const CExoString& tag, unsigned long long objectId)
{
    CExoString sLower(tag);
    int nIndex = FindTagPositionInTable(sLower.CStr());
    while (nIndex < m_tagTable.m_count && m_tagTable.m_pArray[nIndex].m_oid != objectId)
        nIndex++;
    if (nIndex >= m_tagTable.m_count)
        return false;
    if (!(sLower == m_tagTable.m_pArray[nIndex].m_sTag))
        return false;

    m_tagTable.DelIndex(nIndex);
    return true;
}

// Reads the whole IFO back into the module, restores the world clock and rebuilds the module's
// three IFO lists.
unsigned int CSWSModule::LoadModuleStart(const CExoString& sModuleName, int bFromSaveGame)
{
    CResList   sList;
    CResStruct sStruct;
    CResStruct sElement;
    int        bSuccess;
    unsigned short nMinute;
    unsigned short nSecond;
    unsigned short nMilli;
    CResStruct sFactionStruct;
    CExoString sKey;
    CExoString sToken;

    g_pExoResMan->FreeChunk();

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();

    sKey = CExoString("CURRENTGAME:") + sModuleName;

    if (m_sModuleResourceName != sKey)
    {
#if KOTOR1  // K1 reads the length through GetLength
        if (m_sModuleResourceName.GetLength() >= 1)
#else
        if (m_sModuleResourceName.m_sString != 0 && static_cast<int>(strlen(m_sModuleResourceName.m_sString)) >= 1)
#endif
            RemoveModuleResources(m_sModuleResourceName);
        m_sModuleResourceName = sKey;
        AddModuleResources(sKey);
    }

    if (m_pRes->Demand() == 0)
        return 1;

    m_pRes->GetTopLevelStruct(&sStruct);

    m_bIsSaveGame = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_IsSaveGame"), bSuccess, static_cast<unsigned char>(m_bIsSaveGame));
    m_bIsNWMFile = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_IsNWMFile"), bSuccess, static_cast<unsigned char>(m_bIsNWMFile));
    if (m_bIsNWMFile == 1)
        m_sNWMResName = m_pRes->ReadFieldCExoString(&sStruct, const_cast<char*>("Mod_NWMResName"), bSuccess, m_sNWMResName);

    m_nTableCount = g_pExoResMan->GetTableCount(m_pRes, m_bIsNWMFile == 0);

    m_pRes->ReadFieldVOID(&sStruct, m_pModuleID, sizeof(m_pModuleID->m_id), const_cast<char*>("Mod_ID"), bSuccess, 0);
    m_pModuleID->m_nCreatorId = m_pRes->ReadFieldINT(&sStruct, const_cast<char*>("Mod_Creator_ID"), bSuccess, 0);
    m_pModuleID->m_nVersion   = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_Version"), bSuccess, 0);

    g_pAppManager->m_pServerExoApp->GetServerInfo()->SetModuleName(sModuleName);

    m_sName = m_pRes->ReadFieldCExoLocString(&sStruct, const_cast<char*>("Mod_Name"), bSuccess, CExoLocString());
    m_sDescription = m_pRes->ReadFieldCExoLocString(&sStruct, const_cast<char*>("Mod_Description"), bSuccess,
                                                   CExoLocString());
    m_startMovie = m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_StartMovie"), bSuccess, CResRef(""));
    SetTag(m_pRes->ReadFieldCExoString(&sStruct, const_cast<char*>("Mod_Tag"), bSuccess, CExoString("")));

    m_pEntryInfo->m_entryArea =
        m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_Entry_Area"), bSuccess, CResRef(""));
    m_pEntryInfo->m_entryX = m_pRes->ReadFieldFLOAT(&sStruct, const_cast<char*>("Mod_Entry_X"), bSuccess, 0.0f);
    m_pEntryInfo->m_entryY = m_pRes->ReadFieldFLOAT(&sStruct, const_cast<char*>("Mod_Entry_Y"), bSuccess, 0.0f);
    m_pEntryInfo->m_entryZ = m_pRes->ReadFieldFLOAT(&sStruct, const_cast<char*>("Mod_Entry_Z"), bSuccess, 0.0f);
    m_pEntryInfo->m_entryDirX = m_pRes->ReadFieldFLOAT(&sStruct, const_cast<char*>("Mod_Entry_Dir_X"), bSuccess, 0.0f);
    m_pEntryInfo->m_entryDirY = m_pRes->ReadFieldFLOAT(&sStruct, const_cast<char*>("Mod_Entry_Dir_Y"), bSuccess, 0.0f);
    if (bSuccess == 0)
    {
        m_pEntryInfo->m_entryDirX = 1.0f;
        m_pEntryInfo->m_entryDirY = 0.0f;
    }

    m_nMinutesPerHour = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_MinPerHour"), bSuccess, 0);
    m_nDawnHour       = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_DawnHour"), bSuccess, 0);
    m_nDuskHour       = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_DuskHour"), bSuccess, 0);

    unsigned int nPauseTime;
    unsigned int nPauseDay;
    if (bFromSaveGame)
    {
        m_nLastCalendarYear = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_StartYear"), bSuccess,
                                                     CWorldTimer::DEFAULT_YEAR);
        m_nStartMonth = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_StartMonth"), bSuccess,
                                              static_cast<unsigned char>(CWorldTimer::DEFAULT_MONTH));
        m_nStartDay = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_StartDay"), bSuccess,
                                            static_cast<unsigned char>(CWorldTimer::DEFAULT_DAY));
        m_nStartHour = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_StartHour"), bSuccess,
                                             static_cast<unsigned char>(CWorldTimer::DEFAULT_HOUR));
        m_nTimeOfDayElapsed = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_Transition"), bSuccess, 0);
        nMinute = m_pRes->ReadFieldWORD(&sStruct, const_cast<char*>("Mod_StartMinute"), bSuccess, 0);
        nSecond = m_pRes->ReadFieldWORD(&sStruct, const_cast<char*>("Mod_StartSecond"), bSuccess, 0);
        nMilli  = m_pRes->ReadFieldWORD(&sStruct, const_cast<char*>("Mod_StartMiliSec"), bSuccess, 0);
        nPauseTime = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_PauseTime"), bSuccess, 0);
        nPauseDay  = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_PauseDay"), bSuccess, 0);
    }
    else
    {
        g_pAppManager->m_pServerExoApp->GetMoveToWorldTime(m_nLastCalendarYear, m_nStartMonth,
                                                           m_nStartDay, m_nStartHour, nMinute,
                                                           nSecond, nMilli);
        nPauseTime = g_pAppManager->m_pServerExoApp->GetMoveToPausedTimeOfDay();
        nPauseDay  = g_pAppManager->m_pServerExoApp->GetMoveToPausedCalendarDay();
    }

    m_nLastCalendarMonth = m_nStartMonth;
    m_nLastCalendarDay   = m_nStartDay;
    m_nLastCalendarHour  = m_nStartHour;

    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetMinutesPerHour(m_nMinutesPerHour);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetWorldTime(
        m_nLastCalendarYear, m_nStartMonth, m_nStartDay, m_nStartHour, nMinute, nSecond, nMilli, 1);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->m_nPauseDay = nPauseDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->m_nPauseTime = nPauseTime;
    g_pAppManager->m_pServerExoApp->GetTimestopTimer()->ResetTimer(
        g_pAppManager->m_pServerExoApp->GetWorldTimer());
    g_pAppManager->m_pServerExoApp->GetPauseTimer()->ResetTimer(
        g_pAppManager->m_pServerExoApp->GetWorldTimer());
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&m_nLastTimeDay, &m_nLastTimeTod);

    if (m_nDawnHour == m_nDuskHour)
        m_nTimeOfDay = 1;
    else if (m_nLastCalendarHour == m_nDawnHour)
        m_nTimeOfDay = 3;
    else if (m_nLastCalendarHour == m_nDuskHour)
        m_nTimeOfDay = 4;
    else if (m_nDawnHour < m_nDuskHour)
    {
        if (m_nLastCalendarHour >= m_nDawnHour && m_nLastCalendarHour <= m_nDuskHour)
            m_nTimeOfDay = 1;
        else
            m_nTimeOfDay = 2;
    }
    else
    {
        if (m_nLastCalendarHour >= m_nDuskHour && m_nLastCalendarHour <= m_nDawnHour)
            m_nTimeOfDay = 2;
        else
            m_nTimeOfDay = 1;
    }

    m_nXPScale = m_pRes->ReadFieldBYTE(&sStruct, const_cast<char*>("Mod_XPScale"), bSuccess, 10);

    m_pRes->GetList(&sList, &sStruct, const_cast<char*>("Mod_Expan_List"));
    unsigned int nCount = m_pRes->GetListCount(&sList);
    for (unsigned int i = 0; i < nCount; i++)
    {
        m_pRes->GetListElement(&sElement, &sList, i);
        NWModuleExpansion_st* pExpansion = new NWModuleExpansion_st;
        pExpansion->m_name = m_pRes->ReadFieldCExoLocString(&sElement, const_cast<char*>("Expansion_Name"), bSuccess,
                                                            CExoLocString());
        pExpansion->m_nID = m_pRes->ReadFieldINT(&sElement, const_cast<char*>("Expansion_ID"), bSuccess, 0);
        m_expansionList.AddTail(pExpansion);
    }

    m_pRes->GetList(&sList, &sStruct, const_cast<char*>("Mod_CutSceneList"));
    nCount = m_pRes->GetListCount(&sList);
    for (unsigned int i = 0; i < nCount; i++)
    {
        m_pRes->GetListElement(&sElement, &sList, i);
        NWModuleCutScene_st* pCutScene = new NWModuleCutScene_st;
        pCutScene->m_resName = m_pRes->ReadFieldCResRef(&sElement, const_cast<char*>("CutScene_Name"), bSuccess,
                                                        CResRef(""));
        pCutScene->m_nID = m_pRes->ReadFieldDWORD(&sElement, const_cast<char*>("CutScene_ID"), bSuccess, 0);
        m_cutSceneList.AddTail(pCutScene);
    }

    CExoString sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnHeartbeat"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_HEARTBEAT] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnUsrDefined"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_USER_DEFINED] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnClientEntr"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_CLIENT_ENTER] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnClientLeav"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_CLIENT_LEAVE] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnActvtItem"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_ACTIVATE_ITEM] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnAcquirItem"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_ACQUIRE_ITEM] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnUnAqreItem"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_UNACQUIRE_ITEM] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnModLoad"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_MODULE_LOAD] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnModStart"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_MODULE_START] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnPlrDeath"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_PLAYER_DEATH] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnPlrDying"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_PLAYER_DYING] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnSpawnBtnDn"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_SPAWN_BUTTON_DOWN] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnPlrRest"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_PLAYER_REST] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnPlrLvlUp"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_PLAYER_LEVEL_UP] = sScript;
    m_pRes->ReadFieldCResRef(&sStruct, const_cast<char*>("Mod_OnEquipItem"), bSuccess, CResRef(""))
        .CopyToString(sScript);
    m_aScript[SCRIPT_EQUIP_ITEM] = sScript;

    m_pRes->GetList(&sList, &sStruct, const_cast<char*>("Mod_Area_list"));
    unsigned int nAreaListCount = m_pRes->GetListCount(&sList);
    m_pRes->GetListElement(&sElement, &sList, 0);
    m_resRef = m_pRes->ReadFieldCResRef(&sElement, const_cast<char*>("Area_Name"), bSuccess, CResRef(""));

    if (m_bIsSaveGame)
        m_oidArea = m_pRes->ReadFieldDWORD(&sElement, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);

    if (m_bIsSaveGame)
    {
        CGameEffect::m_nNextID = m_pRes->ReadFieldDWORD64(&sStruct, const_cast<char*>("Mod_Effect_NxtId"), bSuccess, 0);
        pObjectArray->m_nNextCharacterObjectId[0] =
            m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_NextCharId0"), bSuccess, 0);
        pObjectArray->m_nNextCharacterObjectId[1] =
            m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_NextCharId1"), bSuccess, 0);
        pObjectArray->m_nNextObjectId[0] = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_NextObjId0"), bSuccess, 0);
        pObjectArray->m_nNextObjectId[1] = m_pRes->ReadFieldDWORD(&sStruct, const_cast<char*>("Mod_NextObjId1"), bSuccess, 0);

        m_pRes->GetList(&sList, &sStruct, const_cast<char*>("Mod_PlayerList"));
        for (unsigned int i = 0; i < m_pRes->GetListCount(&sList); i++)
        {
            NWPlayerListItem_st* pItem = new NWPlayerListItem_st;
            m_pRes->GetListElement(&sElement, &sList, i);
            pItem->m_sCommunityName = m_pRes->ReadFieldCExoString(&sElement, const_cast<char*>("Mod_CommntyName"),
                                                                  bSuccess, CExoString(""));
            pItem->m_firstName = m_pRes->ReadFieldCExoLocString(&sElement, const_cast<char*>("Mod_FirstName"),
                                                               bSuccess, CExoLocString());
            pItem->m_lastName = m_pRes->ReadFieldCExoLocString(&sElement, const_cast<char*>("Mod_LastName"),
                                                              bSuccess, CExoLocString());
            pItem->m_nIsPrimaryPlayer =
                m_pRes->ReadFieldBYTE(&sElement, const_cast<char*>("Mod_IsPrimaryPlr"), bSuccess, 0);
            m_ifoPlayerList.AddTail(pItem);
        }

        m_pRes->GetList(&sList, &sStruct, const_cast<char*>("Mod_Tokens"));
        for (unsigned int i = 0; i < m_pRes->GetListCount(&sList); i++)
        {
            m_pRes->GetListElement(&sElement, &sList, i);
            int nToken = m_pRes->ReadFieldDWORD(&sElement, const_cast<char*>("Mod_TokensNumber"), bSuccess, 0);
            sToken = m_pRes->ReadFieldCExoString(&sElement, const_cast<char*>("Mod_TokensValue"), bSuccess,
                                                 CExoString(""));
            g_pTlkTable->SetCustomToken(nToken, sToken);
        }

        m_scriptVarTable.LoadVarTable(m_pRes, &sStruct);
        m_varTable.LoadVarTable(m_pRes, &sStruct);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->LoadEventQueue(m_pRes, &sStruct);
    }

    if (g_pAppManager->m_pServerExoApp->m_pInternal != 0)
    {
        CFactionManager* pFactionManager =
            g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
        g_pAppManager->m_pServerExoApp->GetPartyTable()->AddGameInProgress();

        int bNeedLoad = 1;
        if (g_pExoResMan->Exists(CResRef("REPUTE"), RESTYPE_FAC, KEYTABLE_SOURCE_DIRECTORY))
        {
            CResGFF* pFacGFF = new CResGFF(RESTYPE_FAC, const_cast<char*>("FAC "), CResRef("REPUTE"));
            if (pFacGFF->m_nValid)
            {
                pFacGFF->GetTopLevelStruct(&sFactionStruct);
                if (pFacGFF->GetList(&sList, &sFactionStruct, const_cast<char*>("FactionList"))
                    && pFactionManager->LoadFactionsFromSaveGame(pFacGFF, &sList))
                {
                    CResList sRepList;
                    if (pFacGFF->GetList(&sRepList, &sFactionStruct, const_cast<char*>("RepList")))
                        pFactionManager->LoadReputationsFromSaveGame(pFacGFF, &sRepList);
                    bNeedLoad = 0;
                }
            }
            delete pFacGFF;
        }
        g_pAppManager->m_pServerExoApp->GetPartyTable()->RemoveGameInProgress();

        if (bNeedLoad)
        {
            if (pFactionManager->LoadFactions(0))
                pFactionManager->LoadReputations();
            else
                pFactionManager->CreateDefaultFactions();
        }
    }

    LoadLimboCreatures(m_pRes, &sStruct, 1);
    CLoadGameErrorInfo* pLoadInfo = g_pAppManager->m_pLoadGameErrorInfo;
    pLoadInfo->m_loadBar.m_total = 0;
    pLoadInfo->m_loadBar.m_progress = nAreaListCount;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_UpdateStallEvent(
        0, nAreaListCount);
    m_bLoadFinished = 0;
    return 0;
}

// Runs one leg of the load-progress chain: builds the area object, loads it, and either reports
// progress (0, keep going) or tears everything down (4).
unsigned int CSWSModule::LoadModuleInProgress(int total, int progress)
{
    K2_UNUSED(progress);
    CSWSArea* pArea;
    if (m_bIsSaveGame)
    {
        unsigned long long nAreaId = m_oidArea;
        pArea = new CSWSArea(m_resRef, 0, nAreaId);
    }
    else
    {
        pArea = new CSWSArea(m_resRef, 0, OBJECT_INVALID);
    }

    if (pArea->LoadArea(m_bIsSaveGame))
    {
        if (m_bIsSaveGame == 0)
            m_oidArea = pArea->m_objectId;

        int nNewProgress = total + 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_loadBar.m_total = nNewProgress;
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_UpdateStallEvent(
            nNewProgress, static_cast<unsigned int>(-1));
        return 0;
    }

    m_pRes->Release();
    RemoveModuleResources(m_sModuleResourceName);
    delete pArea;
    return 4;
}

// Finishes a module load: closes the IFO, publishes the load bar, queues the module's own
// OnModuleLoad script event, releases the IFO and marks the module as finished loading.
unsigned int CSWSModule::LoadModuleFinish()
{
    CResStruct sStruct;
    m_pRes->GetTopLevelStruct(&sStruct);

    CLoadGameErrorInfo* pLoadInfo = g_pAppManager->m_pLoadGameErrorInfo;
    int nProgress = pLoadInfo->m_loadBar.m_progress;
    pLoadInfo->m_loadBar.m_total = nProgress;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_UpdateStallEvent(
        nProgress, static_cast<unsigned int>(-1));

    CScriptEvent* pEvent = new CScriptEvent;
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_LOAD;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);

#if !KOTOR1  // K1 has no reputation-adjustment load
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->LoadReputationAdjustments();
#endif

    m_pRes->Release();
    m_bLoadFinished = 1;

    if (g_nSetMovieFrameRate)
    {
        SetFramesPerSecond(const_cast<char*>("30"));
        displayFPS = static_cast<float>(g_nSetMovieFrameRate);
        g_bEnableAurPostString = 0;
    }

    return 0;
}

// The whole payload of CSWSMessage::SendServerToPlayerModule_Info: the module's tag, its localized
// display name, minutes-per-hour and the two CResRefs.
void CSWSModule::PackModuleIntoMessage(unsigned int nPlayerId)
{
    K2_UNUSED(nPlayerId);
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    pMsg->WriteCExoString(CExoString(strchr(m_sModuleResourceName.CStr(), ':') + 1), 32);
    pMsg->WriteCExoLocStringServer(m_sName, 0);
    pMsg->WriteBYTE(m_nMinutesPerHour, 8);
    pMsg->WriteCResRef(m_resHak, 16);
    pMsg->WriteCResRef(m_startMovie, 16);
}

// Rebuilds the module's limbo list from the IFO's own "Creature List".
int CSWSModule::LoadLimboCreatures(CResGFF* pGFF, CResStruct* pStruct, int bFromSaveGame)
{
    CResList sList;
    if (!pGFF->GetList(&sList, pStruct, const_cast<char*>("Creature List")))
        return 1;

    for (unsigned int i = 0; i < pGFF->GetListCount(&sList); i++)
    {
        CResStruct sElement;
        if (!pGFF->GetListElement(&sElement, &sList, i))
            continue;
        if (pGFF->GetElementType(&sElement) != 4)
            continue;

        int bSuccess;
        unsigned long long oid =
            pGFF->ReadFieldDWORD(&sElement, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);

        CSWSCreature* pCreature = new CSWSCreature(oid, 0);
        if (!pCreature->LoadCreature(pGFF, &sElement, bFromSaveGame))
        {
            delete pCreature;
            continue;
        }

        if (bFromSaveGame)
            pCreature->m_bSpawnPerceptionDone = 1;

        pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("XPosition"), bSuccess, 0.0f);
        pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("YPosition"), bSuccess, 0.0f);
        pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
        float fXOrientation = pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("XOrientation"), bSuccess, 0.0f);
        float fYOrientation = pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("YOrientation"), bSuccess, 0.0f);
        float fZOrientation = pGFF->ReadFieldFLOAT(&sElement, const_cast<char*>("ZOrientation"), bSuccess, 0.0f);
        Vector vOrientation(fXOrientation, fYOrientation, fZOrientation);
        pCreature->SetOrientation(vOrientation);

        AddObjectToLimbo(pCreature->m_objectId);
    }

    return 1;
}

// Appends an object id to the limbo list, ignoring one that is already parked there.
void CSWSModule::AddObjectToLimbo(unsigned long long objectId)
{
    for (int i = 0; i < m_limboList.m_count; i++)
        if (m_limboList.m_pArray[i] == objectId)
            return;

    // inlined CExoArrayList<unsigned long long>::Add(objectId)
    if (m_limboList.m_count == m_limboList.m_capacity)
    {
        int nNewCapacity = (m_limboList.m_count == 0) ? 16 : m_limboList.m_count * 2;
        unsigned long long* paOld = m_limboList.m_pArray;
        m_limboList.m_capacity = nNewCapacity;
        m_limboList.m_pArray = new unsigned long long[nNewCapacity];
        for (long long i = 0; i < static_cast<long long>(m_limboList.m_count); i++)
            m_limboList.m_pArray[i] = paOld[i];
        delete[] paOld;
    }
    m_limboList.m_pArray[m_limboList.m_count++] = objectId;
}

// The index of the module's primary player in the IFO's player list, or -1.
int CSWSModule::GetPrimaryPlayerIndex()
{
    CExoLinkedListNode* pNode = m_ifoPlayerList.m_pInternal->m_pHead;
    if (pNode == 0)
        return -1;

    int nIndex = 0;
    for (;;)
    {
        NWPlayerListItem_st* pItem =
            static_cast<NWPlayerListItem_st*>(m_ifoPlayerList.m_pInternal->GetAtPos(pNode));
        if (pItem->m_nIsPrimaryPlayer == 1)
            return nIndex;
        m_ifoPlayerList.m_pInternal->GetNext(pNode);
        nIndex++;
        if (pNode == 0)
            return -1;
    }
}

// The index in the IFO's player list of the row that describes pPlayer, or -1. A row matches when
// its community name and both localized names agree with the player and its creature.
int CSWSModule::GetPlayerIndexInPlayerList(CSWSPlayer* pPlayer)
{
    CExoLinkedListNode* pNode = m_ifoPlayerList.m_pInternal->m_pHead;
    if (pNode == 0)
        return -1;

    int nIndex = 0;
    for (;;)
    {
        NWPlayerListItem_st* pItem =
            static_cast<NWPlayerListItem_st*>(m_ifoPlayerList.m_pInternal->GetAtPos(pNode));
        if (pItem->m_sCommunityName == pPlayer->GetPlayerName())
        {
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
            if (pItem->m_firstName == pCreature->m_pStats->m_firstName &&
                pItem->m_lastName == pCreature->m_pStats->m_lastName)
                return nIndex;
        }
        m_ifoPlayerList.m_pInternal->GetNext(pNode);
        if (pNode == 0)
            return -1;
        nIndex++;
    }
}

// Appends the module's own IFO player list to a character list already built from the servervault.
void CSWSModule::PackPlayerCharacterListIntoMessage(
    CSWSPlayer* pPlayer, CExoArrayList<NWPlayerCharacterList_st*>& list)
{
    K2_UNUSED(pPlayer);
    CResRef    resRef;
    CExoString sIndex;

    CExoLinkedListNode* pNode = m_ifoPlayerList.GetHeadPos();
    if (pNode == 0)
        return;

    int nIndex = 0;
    while (pNode != 0)
    {
        NWPlayerListItem_st* pItem;
        NWPlayerCharacterList_st* pEntry;
        pItem = m_ifoPlayerList.GetAtPos(pNode);
        pEntry = new NWPlayerCharacterList_st;
        pEntry->m_firstName = pItem->m_firstName;
        pEntry->m_lastName  = pItem->m_lastName;
        pEntry->m_nLocation = MSG_LOGIN_SERVER_CHARACTER_JOIN;

        sIndex.Format(const_cast<char*>("%i"), static_cast<unsigned short>(nIndex));
        resRef = sIndex;
        pEntry->m_resRef = resRef;

        if (list.DerefContains(pEntry))
        {
            delete pEntry;
        }
        else
        {
            list.Add(pEntry);
        }

        m_ifoPlayerList.GetNext(pNode);
        nIndex++;
    }
}

// Mounts everything a module's name needs from the resource manager. Only the part of the name
// after the first colon is used for resource lookups.
void CSWSModule::AddModuleResources(const CExoString& sModule)
{
    CExoString sStem;
    unsigned char nStep0 = g_pAppManager->m_pClientExoApp->GetLoadStep(0);
    unsigned char nStep1 = g_pAppManager->m_pClientExoApp->GetLoadStep(1);
    g_pExoResMan->FindModuleDownloadedPkg(sModule);
#if KOTOR1  // K1 mounts the players archive here and advances the bar in sevenths
    g_pExoResMan->AddEncapsulatedResourceFile(CExoString("HD0:players"), 2);
    int nIncrement = nStep1 / 7;
#else

    // A fifteenth of the first load step's count (255 -> 17).
    int nIncrement = nStep1 / 15;
#endif
    int nProgress = nStep0 + nIncrement;
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(nProgress, 1);
    UpdateScreen(1.0f / 30.0f, 0, 0);

    int nColon = sModule.Find(':', 0);
    if (nColon == -1)
    {
        sStem = sStem + sModule;
    }
    else
    {
#if KOTOR1  // K1 reads the length through GetLength
        int nLength = sModule.GetLength();
#else
        int nLength = (sModule.m_sString != 0) ? static_cast<int>(strlen(sModule.m_sString)) : 0;
#endif
        sStem = sStem + sModule.Right(nLength - nColon - 1);
    }

    g_pExoResMan->AddResourceDirectory(CExoString("CURRENTGAME:"));
    if (g_pExoResMan->Exists(CResRef(sStem), RESTYPE_SAV, static_cast<unsigned int*>(0)))
        m_bIsSaveGame = 1;
    g_pExoResMan->RemoveResourceDirectory(CExoString("CURRENTGAME:"));

    CExoString sLoc;
    sLoc.Format(const_cast<char*>("LIPS:%s_loc"), sStem.CStr());
    g_pExoResMan->AddEncapsulatedResourceFile(sLoc, 2);
    g_pExoResMan->AddEncapsulatedResourceFile(CExoString("LIPS:localization"), 2);

#if KOTOR1  // K1 has no override lips directories
    g_pExoResMan->AddResourceImageFilesAsync(sStem, 1, m_bIsSaveGame);
    int nTotal = nStep0 + nStep1;
#else
    int nTotal = nStep0 + nStep1;

    CExoArrayList<CExoString> aAddedPaths;
    for (int i = 0; ; i++)
    {
        CExoString sLocalization;

        const char* pPath = GetOverridePath(i, false);
        if (pPath == 0)
            break;

        sLoc.Format(const_cast<char*>("%s\\lips\\%s_loc"), pPath, sStem.CStr());
        if (!aAddedPaths.Contains(sLoc))
        {
            g_pExoResMan->AddEncapsulatedResourceFile(sLoc, 2);
            aAddedPaths.Add(sLoc);
        }

        sLocalization.Format(const_cast<char*>("%s\\lips\\localization"), pPath);
        if (!aAddedPaths.Contains(sLocalization))
        {
            g_pExoResMan->AddEncapsulatedResourceFile(sLocalization, 2);
            aAddedPaths.Add(sLocalization);
        }
    }

    g_pExoResMan->AddResourceImageFilesAsync(sStem, 1, m_bIsSaveGame);
#endif

    int nTicks = 0;
    while (g_pExoResMan->m_nAsyncLoadState == 1)
    {
        Sleep(10);
        if (nProgress < nTotal)
        {
            if (nTicks < 100)
            {
                nTicks++;
            }
            else
            {
                nProgress = (nProgress + nIncrement < nTotal) ? nProgress + nIncrement : nTotal;
                g_pAppManager->m_pClientExoApp->SetLoadBarProgress(nProgress, 1);
                UpdateScreen(1.0f / 30.0f, 0, 0);
                nTicks = 0;
            }
        }
    }

    g_pAppManager->m_pClientExoApp->SetModuleResourceLoaded(g_pExoResMan->m_nModuleResourceFlags);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(nTotal, 1);
    UpdateScreen(1.0f / 30.0f, 0, 0);
}

// Tears down a module's resource mounts and clears the client's loaded latch.
void CSWSModule::RemoveModuleResources(const CExoString& sModule)
{
    g_pExoResMan->RemoveEncapsulatedResourceFile(sModule);

    CExoString sStem;
    int nColon = sModule.Find(':', 0);
    if (nColon == -1)
    {
        sStem = sModule;
    }
    else
    {
#if KOTOR1  // K1 reads the length through GetLength
        int nLength = sModule.GetLength();
#else
        int nLength = (sModule.m_sString != 0) ? static_cast<int>(strlen(sModule.m_sString)) : 0;
#endif
        sStem = sModule.Right(nLength - nColon - 1);
    }

    int nDownloaded = g_pExoResMan->FindModuleDownloadedPkg(sStem);

    g_pExoResMan->AddResourceDirectory(CExoString("CURRENTGAME:"));
    g_pExoResMan->RemoveResourceImageFile(CExoString("CURRENTGAME:") + sStem);
    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("CURRENTGAME:") + sStem);
    g_pExoResMan->RemoveResourceDirectory(CExoString("CURRENTGAME:"));

#if KOTOR1  // K1 has no override directories and no _dlg archive
    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
    if (g_pExoResMan->Exists(CResRef(sStem), RESTYPE_MOD, static_cast<unsigned int*>(0)))
    {
        g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("MODULES:") + sStem);
    }
    else if (g_pExoResMan->Exists(CResRef(sStem + CExoString("_s")), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        if (nDownloaded != 0)
        {
            CExoString sLive;
            sLive.Format(const_cast<char*>("LIVE%d:MODULES\\"), nDownloaded);
            g_pExoResMan->RemoveResourceImageFile(sLive + sStem);
        }
        else
        {
            g_pExoResMan->RemoveResourceImageFile(CExoString("MODULES:") + sStem + CExoString("_s"));
        }
    }
    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));

    CExoString sLoc;
    sLoc.Format(const_cast<char*>("LIPS:%s_loc"), sStem.CStr());
    g_pExoResMan->RemoveEncapsulatedResourceFile(sLoc);
    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("LIPS:localization"));

    if (g_pExoResMan->Exists(CResRef(sStem + CExoString("_a")), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:") + sStem + CExoString("_a"));

    if (g_pExoResMan->Exists(CResRef(sStem + CExoString("_adx")), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:") + sStem + CExoString("_adx"));

    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("HD0:players"));
    g_pAppManager->m_pClientExoApp->ResetModuleResourcesLoaded();
}
#else
    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
    AddModuleOverrideResourceDirectories();

    if (g_pExoResMan->Exists(CResRef(sStem), RESTYPE_MOD, static_cast<unsigned int*>(0)))
    {
        g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("MODULES:") + sStem);
        for (int i = 0; ; i++)
        {
            char sBuf[200];

            const char* pPath = GetOverridePath(i, false);
            if (pPath == 0)
                break;
            sprintf(sBuf, const_cast<char*>("%s\\modules\\%s"), pPath, sStem.CStr());
            g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString(sBuf));
        }
    }
    else
    {
        if (g_pExoResMan->Exists(CResRef(sStem + "_dlg"), RESTYPE_ERF, static_cast<unsigned int*>(0)))
        {
            if (nDownloaded != 0)
            {
                CExoString sLive;
                sLive.Format(const_cast<char*>("LIVE%d:MODULES\\"), nDownloaded);
                g_pExoResMan->RemoveEncapsulatedResourceFile(sLive + sStem + CExoString("_dlg"));
            }
            else
            {
                g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("MODULES:") + sStem +
                                                             CExoString("_dlg"));
                for (int i = 0; ; i++)
                {
                    char sBuf[200];

                    const char* pPath = GetOverridePath(i, false);
                    if (pPath == 0)
                        break;
                    sprintf(sBuf, const_cast<char*>("%s\\modules\\%s"), pPath, sStem.CStr());
                    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString(sBuf));
                }
            }
        }

        if (g_pExoResMan->Exists(CResRef(sStem + "_s"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        {
            if (nDownloaded != 0)
            {
                CExoString sLive;
                sLive.Format(const_cast<char*>("LIVE%d:MODULES\\"), nDownloaded);
                g_pExoResMan->RemoveResourceImageFile(sLive + sStem + CExoString("_s"));
            }
            else
            {
                g_pExoResMan->RemoveResourceImageFile(CExoString("MODULES:") + sStem + CExoString("_s"));
                for (int i = 0; ; i++)
                {
                    char sBuf[200];

                    const char* pPath = GetOverridePath(i, false);
                    if (pPath == 0)
                        break;
                    sprintf(sBuf, const_cast<char*>("%s\\modules\\%s_s"), pPath, sStem.CStr());
                    g_pExoResMan->RemoveResourceImageFile(CExoString(sBuf));
                }
            }
        }
    }

    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
    RemoveModuleOverrideResourceDirectories();

    CExoString sLoc;
    sLoc.Format(const_cast<char*>("LIPS:%s_loc"), sStem.CStr());
    g_pExoResMan->RemoveEncapsulatedResourceFile(sLoc);
    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("LIPS:localization"));

    for (int i = 0; ; i++)
    {
        CExoString sLocalization;

        const char* pPath = GetOverridePath(i, false);
        if (pPath == 0)
            break;

        sLoc.Format(const_cast<char*>("%s\\lips\\%s_loc"), pPath, sStem.CStr());
        g_pExoResMan->RemoveEncapsulatedResourceFile(sLoc);

        sLocalization.Format(const_cast<char*>("%s\\lips\\localization"), pPath);
        g_pExoResMan->RemoveEncapsulatedResourceFile(sLocalization);
    }

    if (g_pExoResMan->Exists(CResRef(sStem + CExoString("_a")), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:") + sStem + CExoString("_a"));

    if (g_pExoResMan->Exists(CResRef(sStem + CExoString("_adx")), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:") + sStem + CExoString("_adx"));

    g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("HD0:players"));
    g_pAppManager->m_pClientExoApp->ResetModuleResourcesLoaded();
}
#endif

// Cuts the leading current-game resource tag and its colon off the module's stored name, one colon
// at a time.
CExoString CSWSModule::GetModuleResourceName()
{
    CExoString sName;
    int nColon;
    sName = m_sModuleResourceName;

    nColon = sName.Find(':', 0);
    while (nColon != -1)
    {
#if KOTOR1  // K1 reads the length through GetLength
        int nLength = sName.GetLength();
#else
        int nLength = (sName.m_sString != 0) ? static_cast<int>(strlen(sName.m_sString)) : 0;
#endif
        sName = sName.Right(nLength - nColon - 1);

        nColon = sName.Find(':', 0);
    }
    return sName;
}

// The module's half of the world journal mirrors each local add or drop out to the players' own
// journals. Entries live in m_worldJournal and the entry class is the client journal's own; bit 0
// of m_nEntryFlags separates the two add paths, and at least one search starts one element past the
// end of the array, an off-by-one that is reproduced rather than corrected.
int CSWSModule::GetWorldJournalIndexUnique()
{
    int i = 0;
    int nIndex = 1;
    if (m_worldJournal.m_count > 0) {
        do {
            if ((m_worldJournal.m_pArray[i].m_nEntryFlags & 1) == 0
                && m_worldJournal.m_pArray[i].m_nEntryId == static_cast<unsigned int>(nIndex)) {
                nIndex++;
                i = -1;
            }
            i++;
        } while (i < m_worldJournal.m_count);
    }
    return nIndex;
}

// Appends the entry, then ships it to every player as a pair of strings.
void CSWSModule::AddWorldJournalEntry(int nIndex, const CExoString& sName, const CExoString& sText,
                                     unsigned int nDay, unsigned int nTime)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    CExoLinkedList<CSWSClient>* ppPlayers;

    CWorldJournalEntry entry;
    entry.m_nEntryId = nIndex;
    entry.m_sName = sName;
    entry.m_sText = sText;
    entry.m_nCalendarDay = nDay;
    entry.m_nTimeOfDay = nTime;
    entry.m_nEntryFlags &= ~1;

    m_worldJournal.Add(entry);

    if (pMsg == 0)
        return;

    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0) {
            if (pEach->GetGameObject() != 0)
                pMsg->SendServerToPlayerJournalAddWorld(pEach, nIndex, sName, sText, nDay, nTime);
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

// Adds an entry that carries talk-table strrefs instead of strings.
void CSWSModule::AddWorldJournalEntryStrref(uint64_t nNameStrRef, uint64_t nTextStrRef,
                                           unsigned int nDay, unsigned int nTime)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    CExoLinkedList<CSWSClient>* ppPlayers;

    CWorldJournalEntry entry;
    entry.m_nEntryId = static_cast<unsigned int>(nNameStrRef);
    entry.m_nTextStrRef = nTextStrRef;
    entry.m_nCalendarDay = nDay;
    entry.m_nTimeOfDay = nTime;
    entry.m_nEntryFlags |= 1;

    m_worldJournal.Add(entry);

    if (pMsg == 0)
        return;

    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0) {
            if (pEach->GetGameObject() != 0)
                pMsg->SendServerToPlayerJournalAddWorldStrref(pEach, static_cast<unsigned int>(nNameStrRef), static_cast<unsigned int>(nTextStrRef), nDay,
                                                              nTime);
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

// Drops the last non-strref entry whose m_nEntryId equals nIndex.
void CSWSModule::DeleteWorldJournalEntry(int nIndex)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    int nPosition = m_worldJournal.m_count;
    while (nPosition >= 0) {
        CWorldJournalEntry* pEntry = &m_worldJournal.m_pArray[nPosition];
        if ((pEntry->m_nEntryFlags & 1) == 0 && pEntry->m_nEntryId == static_cast<unsigned int>(nIndex)) {
            m_worldJournal.DelIndex(nPosition);
            break;
        }
        nPosition--;
    }

    if (pMsg == 0)
        return;

    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0) {
            if (pEach->GetGameObject() != 0)
                pMsg->SendServerToPlayerJournalDeleteWorld(pEach, nIndex);
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

// The strref-keyed twin of DeleteWorldJournalEntry.
void CSWSModule::DeleteWorldJournalEntryStrref(uint64_t nStrRef)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    int nPosition = m_worldJournal.m_count;
    while (nPosition >= 0) {
        CWorldJournalEntry* pEntry = &m_worldJournal.m_pArray[nPosition];
        if ((pEntry->m_nEntryFlags & 1) != 0 && pEntry->m_nEntryId == nStrRef) {
            m_worldJournal.DelIndex(nPosition);
            break;
        }
        nPosition--;
    }

    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0) {
            if (pEach->GetGameObject() != 0 && pMsg != 0)
                pMsg->SendServerToPlayerJournalDeleteWorldStrref(pEach, static_cast<unsigned int>(nStrRef));
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

// Wipes the journal for the script command: drops every entry and tells each player to do the same.
void CSWSModule::DeleteWorldJournalAllEntries()
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    m_worldJournal.SetSize(0);

    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0) {
            if (pEach->GetGameObject() != 0 && pMsg != 0)
                pMsg->SendServerToPlayerJournalDeleteWorldAll(pEach);
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

// Removes an object id from the limbo list.
void CSWSModule::RemoveObjectFromLimbo(unsigned long long objectId)
{
    int i = m_limboList.m_count;
    while (i > 0) {
        i--;
        if (m_limboList.m_pArray[i] == objectId) {
            m_limboList.DelIndex(i);
            break;
        }
    }
}

// Reaps every limbo id that still resolves to a live object placed in an area. The scan runs
// backward from the seeded count, so a reap mid-scan does not rescan the compacted entries.
void CSWSModule::CleanUpLimboList()
{
    int nCount = m_limboList.m_count;
    for (int i = nCount - 1; i >= 0; i--) {
        unsigned long long id = m_limboList.m_pArray[i];
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(id, &pObject) !=
            CGameObjectArray::SUCCESS)
            continue;
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject == 0)
            continue;
        if (pSWSObject->GetArea() == 0)
            continue;
        m_limboList.DelIndex(i);
    }
}

// In the save-game family, SaveModuleStart opens the ERF writer and the IFO GFF and hands both to
// the payload writers; SaveModuleFAC, SaveModuleIFOStart, SaveModuleIFOFinish, SavePlayers and
// SaveLimboCreatures write the sections; SaveModuleInProgress and SaveStatic are the per-area and
// per-file helpers; SaveModuleFinish seals the archive.
#include "exobase.h"

extern CExoBase* g_pExoBase;
#include "port/win32api.h"

// Deletes any existing save at the target path, then opens the ERF writer and the IFO GFF.
void CSWSModule::SaveModuleStart(CExoString& resourceName, CExoString& sFileName)
{
    m_bIsSaveGame = 1;
    CExoString sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sFileName, RESTYPE_RSV);
    DeleteFileA(sResolved.CStr());
    m_pSaveFile = new CERFFile();
    m_pSavedObjectIds = new CExoArrayList<unsigned long long>();
    m_pSaveFile->Create(sFileName);
    m_pSaveFile->SetVersion(const_cast<char*>("MOD V1.0"));
    m_pSaveFile->WriteHeader();
    m_pSaveFile->WriteStringTable();
    m_nTableCount = 3;
    m_pSaveFile->SetNumEntries(3);
    m_pSaveGFF = new CResGFF();
    m_pSaveStruct = new CResStruct();
    if (m_pSaveGFF->CreateGFFFile(m_pSaveStruct, CExoString("IFO "), CExoString("V2.0")))
    {
        SaveModuleIFOStart(m_pSaveGFF, m_pSaveStruct);
        SaveModuleFAC();
    }
    else
    {
        delete m_pSaveFile;
        m_pSaveFile = 0;
        delete m_pSaveGFF;
        m_pSaveGFF = 0;
        delete m_pSaveStruct;
        m_pSaveStruct = 0;
    }
    K2_UNUSED(resourceName);
}

// The factions/reputations section of a save.
int CSWSModule::SaveModuleFAC()
{
    CExoString sFileName;
    CFactionManager* pFactions = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
    CResGFF* pGFF = new CResGFF();
    CResStruct* pStruct = new CResStruct();
    CResList list;
    int bSuccess = pGFF->CreateGFFFile(pStruct, CExoString("FAC "), CExoString("V2.0"));
    if (bSuccess == 1)
    {
        pGFF->AddList(&list, pStruct, const_cast<char*>("FactionList"));
        pFactions->SaveFactions(pGFF, &list);
        pGFF->AddList(&list, pStruct, const_cast<char*>("RepList"));
        pFactions->SaveReputations(pGFF, &list);
        sFileName = "GAMEINPROGRESS:REPUTE";
        pGFF->WriteGFFFile(CExoString(sFileName.CStr()), RESTYPE_FAC);
    }
    delete pGFF;
    delete pStruct;
    return bSuccess;
}

// Flushes the module's current area into the archive and reports the save-bar progress. Returns 0
// when there is no open archive.
int CSWSModule::SaveModuleInProgress()
{
    CExoString sFileName;
    CExoString sTmpArea;
    CExoString sTmpGit;
    CSWSArea* pArea;
    unsigned int nProgress;
    if (m_pSaveFile == 0)
        return 0;
    sTmpArea = "tmparea";
    sTmpGit = "tmpgit";
    pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    sFileName = sTmpGit;
    pArea->SaveGIT(m_pSaveFile, sFileName, *m_pSavedObjectIds);
    // A local here avoids re-reading g_pAppManager->m_pLoadGameErrorInfo for the second access.
    CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
    pInfo->m_loadBar.m_total = 1;
    nProgress = pInfo->m_loadBar.m_progress;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_UpdateStallEvent(
        1, nProgress);
    return 1;
}

// Seals the save: the IFO section, then the archive's own index. Returns 0 when there is no
// archive. The two heap objects are released here.
int CSWSModule::SaveModuleFinish(CExoString& resourceName, CExoString& tag)
{
    K2_UNUSED(tag);
    if (m_pSaveFile == 0)
        return 0;
    if (m_bIsNWMFile == 0)
        SaveStatic(m_pSaveFile, CExoString("ARE "), RESTYPE_ARE, 1);
    SaveModuleIFOFinish(m_pSaveGFF, m_pSaveStruct, m_pSaveFile, resourceName, *m_pSavedObjectIds);
    m_pSaveFile->Finish();
    delete m_pSaveFile;
    m_pSaveFile = 0;
    delete m_pSavedObjectIds;
    m_pSavedObjectIds = 0;
    return 1;
}

// Writes every resource of one type named in the module's IFO into the archive. The list of names
// comes from the IFO's own "Module" CResRef table.
int CSWSModule::SaveStatic(CERFFile* pERF, const CExoString& sFileType, unsigned short nResType,
                           int nBufferSize)
{
    CRes* pModule = g_pExoResMan->GetResObject(CResRef("Module"), RESTYPE_IFO);
    CExoStringList* pList = static_cast<CExoStringList*>(g_pExoResMan->GetResOfType(nResType, pModule));
    int bSuccess = 1;
    if (pList == 0)
    {
        g_pExoResMan->ReleaseResObject(pModule);
        return 1;
    }
    for (int i = 0; i < pList->GetCount(); i++)
    {
        CExoString* pEntry = pList->GetString(i);
        CRes* pRes = g_pExoResMan->GetResObject(CResRef(pEntry->CStr()), nResType);
        int bCreated = 0;
        if (pRes == 0)
        {
            if (nBufferSize != 0)
            {
                CResGFF* pNewGFF = new CResGFF();
                const char* pszFileType = sFileType.CStr();
                strncpy(pNewGFF->m_sFileType, pszFileType, 4);
                pRes = pNewGFF;
            }
            else
            {
                pRes = new CRes();
            }
            g_pExoResMan->SetResObject(CResRef(pEntry->CStr()), nResType, pRes);
            bCreated = 1;
        }
        if (pRes->Demand() != 0)
        {
#if KOTOR1
            pERF->WriteResource(pEntry->CStr(), nResType, pRes, nBufferSize, 0);
#else
            pERF->WriteResource(pEntry->CStr(), nResType, pRes, nBufferSize, reinterpret_cast<void*>(-1));
#endif
            pRes->Release();
        }
        else
        {
            bSuccess = 0;
        }
        g_pExoResMan->ReleaseResObject(pRes);
        if (bCreated)
            delete pRes;
    }
    g_pExoResMan->ReleaseResObject(pModule);
    delete pList;
    return bSuccess;
}

// Closes the IFO section: the player list, then the IFO itself into the archive, then the two heap
// objects the section owned.
int CSWSModule::SaveModuleIFOFinish(CResGFF* pGFF, CResStruct* pStruct, CERFFile* pERF,
                                    CExoString& sFileName,
                                    CExoArrayList<unsigned long long>& aSavedObjectIds)
{
    CExoString sUnused1;
    CExoString sUnused2;
    CExoString sUnused3;
    SavePlayers(pGFF, pStruct, sFileName, aSavedObjectIds);
#if KOTOR1
    pERF->WriteResource(const_cast<char*>("Module"), RESTYPE_IFO, pGFF, 1, 0);
#else
    pERF->WriteResource(const_cast<char*>("Module"), RESTYPE_IFO, pGFF, 1, reinterpret_cast<void*>(-1));
#endif
    delete pGFF;
    delete pStruct;
    return 1;
}

// Writes the limbo list as a GFF list of the parked ids that still resolve to a live creature.
void CSWSModule::SaveLimboCreatures(CResGFF* pGFF, CResStruct* pStruct)
{
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResStruct sElement;
    CGameObject* pObject;
    CResList list;
    pGFF->AddList(&list, pStruct, const_cast<char*>("Creature List"));
    for (int64_t i = 0; i < static_cast<int64_t>(m_limboList.m_count); i++)
    {
        if (pArray->GetGameObject(m_limboList.m_pArray[i], &pObject) != CGameObjectArray::SUCCESS)
            continue;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pGFF->AddListElement(&sElement, &list, 4);
        pGFF->WriteFieldDWORD(&sElement, static_cast<unsigned int>(pCreature->m_objectId), const_cast<char*>("ObjectId"));
        pCreature->SaveCreature(pGFF, &sElement);
    }
}

#include "swparty.h"
#include "nwccreature.h"
#include "nwscreaturestats.h"
#include "exostringlist.h"
#include "CExoArrayListImpl.h"

// Writes the player-list section of the save, one GFF element per saved object id that still
// resolves to a live creature.
int CSWSModule::SavePlayers(CResGFF* pGFF, CResStruct* pStruct, const CExoString& sUnused,
                            CExoArrayList<unsigned long long>& aSavedObjectIds)
{
    K2_UNUSED(sUnused);
    CExoArrayList<int> aPlayerIndices;
#if KOTOR1
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
#else
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CGameObjectArray* pObjectArray = pServer->GetObjectArray();
#endif
    CResList list;
    pGFF->AddList(&list, pStruct, const_cast<char*>("Mod_PlayerList"));
    CResStruct sElement;
    CGameObject* pObject;
    CResStruct sTop;
    for (int64_t i = 0; i < static_cast<int64_t>(aSavedObjectIds.m_count); i++)
    {
        if (pObjectArray->GetGameObject(aSavedObjectIds.m_pArray[i], &pObject)
            != CGameObjectArray::SUCCESS)
            continue;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSPlayer* pPlayer = pServer->GetClientObjectByObjectId(pCreature->m_objectId);
        if (pPlayer == 0)
        {
            // No client-side player for this creature: fall back to the party's own first member
            // (client object id -> server object id), and give up if even that does not resolve.
            CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            CSWCCreature* pPartyMember = pParty->GetCharacter(0);
            pPlayer = pServer->GetClientObjectByObjectId(
                pServer->ClientToServerObjectId(pPartyMember->m_objectId));
            if (pPlayer == 0)
            {
                pCreature->DumpToLog();
                continue;
            }
        }
        pPlayer->SaveServerCharacter();
        int nIndex = GetPlayerIndexInPlayerList(pPlayer);
        if (nIndex != -1)
            aPlayerIndices.Add(nIndex);
        pGFF->AddListElement(&sElement, &list, GFF_STRUCT_ID_PLAYER);
        pGFF->WriteFieldCExoString(&sElement, pPlayer->GetPlayerName(), const_cast<char*>("Mod_CommntyName"));
        pGFF->WriteFieldBYTE(&sElement, static_cast<unsigned char>(pPlayer->m_bIsPrimaryPlayer), const_cast<char*>("Mod_IsPrimaryPlr"));
        pGFF->WriteFieldCExoLocString(&sElement, pCreature->m_pStats->m_firstName,
                                      const_cast<char*>("Mod_FirstName"));
        pGFF->WriteFieldCExoLocString(&sElement, pCreature->m_pStats->m_lastName,
                                      const_cast<char*>("Mod_LastName"));
        pGFF->WriteFieldDWORD(&sElement, static_cast<unsigned int>(pCreature->m_objectId), const_cast<char*>("ObjectId"));
        pCreature->SaveCreature(pGFF, &sElement);
    }
    CRes* pModuleRes = g_pExoResMan->GetResObject(CResRef("Module"), RESTYPE_IFO);
#if KOTOR1
    // K1 also walks the module IFO's own player list, saving every player that was not written
    // above and whose object id is not among the saved ids. Returns 1 once that list was walked.
    CExoString sCommunityName;
    if (pModuleRes != 0)
    {
        pModuleRes->Demand();
        static_cast<CResGFF*>(pModuleRes)->GetTopLevelStruct(&sTop);
        CResList ifoList;
        if (static_cast<CResGFF*>(pModuleRes)->GetList(&ifoList, &sTop, const_cast<char*>("Mod_PlayerList")))
        {
            for (unsigned int i = 0; i < m_ifoPlayerList.m_pInternal->m_nCount; i++)
            {
                int j;
                for (j = 0; j < aPlayerIndices.m_count; j++)
                {
                    if (aPlayerIndices.m_pArray[j] == static_cast<int>(i))
                        break;
                }
                if (j < aPlayerIndices.m_count)
                    continue;
                CResStruct sIfoPlayer;
                if (!static_cast<CResGFF*>(pModuleRes)->GetListElement(&sIfoPlayer, &ifoList, i))
                {
                    g_pExoResMan->ReleaseResObject(pModuleRes);
                    return 0;
                }
                int valid;
                unsigned int nObjectId = static_cast<CResGFF*>(pModuleRes)->ReadFieldDWORD(
                    &sIfoPlayer, const_cast<char*>("ObjectId"), valid, OBJECT_INVALID);
                sCommunityName = static_cast<CResGFF*>(pModuleRes)->ReadFieldCExoString(
                    &sIfoPlayer, const_cast<char*>("Mod_CommntyName"), valid, CExoString(-1));
                int nFound = 0;
                for (int k = 0; k < aSavedObjectIds.m_count; k++)
                {
                    if (aSavedObjectIds.m_pArray[k] == nObjectId)
                        nFound++;
                }
                if (nObjectId != OBJECT_INVALID && nFound == 0)
                {
                    CSWSPlayer* pIfoPlayer = new CSWSPlayer(nObjectId);
                    pIfoPlayer->LoadCharacterFromIFO(i, 1, 0);
                    CSWSCreature* pCreature = static_cast<CSWSCreature*>(pIfoPlayer->GetGameObject());
                    pGFF->AddListElement(&sElement, &list, GFF_STRUCT_ID_PLAYER);
                    pGFF->WriteFieldCExoString(&sElement, sCommunityName, const_cast<char*>("Mod_CommntyName"));
                    pGFF->WriteFieldBYTE(&sElement, static_cast<unsigned char>(pIfoPlayer->m_bIsPrimaryPlayer), const_cast<char*>("Mod_IsPrimaryPlr"));
                    pGFF->WriteFieldCExoLocString(&sElement, pCreature->m_pStats->m_firstName,
                                                  const_cast<char*>("Mod_FirstName"));
                    pGFF->WriteFieldCExoLocString(&sElement, pCreature->m_pStats->m_lastName,
                                                  const_cast<char*>("Mod_LastName"));
                    pGFF->WriteFieldDWORD(&sElement, static_cast<unsigned int>(pCreature->m_objectId),
                                          const_cast<char*>("ObjectId"));
                    pCreature->SaveCreature(pGFF, &sElement);
                    pCreature->RemoveFromArea(0);
                    delete pIfoPlayer;
                }
            }
            pModuleRes->Release();
            g_pExoResMan->ReleaseResObject(pModuleRes);
            return 1;
        }
        pModuleRes->Release();
        g_pExoResMan->ReleaseResObject(pModuleRes);
    }
    return 0;
#else
    CExoString sUnusedTail;
    if (pModuleRes != 0)
    {
        pModuleRes->Demand();
        static_cast<CResGFF*>(pModuleRes)->GetTopLevelStruct(&sTop);
        pModuleRes->Release();
        g_pExoResMan->ReleaseResObject(pModuleRes);
    }
    return 0;
#endif
}

// Writes the IFO section of the save: the module identity block, the world clock as the Mod_Start*
// calendar stamp, the pause stamp, the limbo list, the expansion and cutscene lists, the module
// scripts, the area list and the custom-token table.
int CSWSModule::SaveModuleIFOStart(CResGFF* pGFF, CResStruct* pStruct)
{
    CExoLinkedListNode* pNode;
    CResStruct sElement;
    CExoLocString sUnused;
    CResList list;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();

    pGFF->WriteFieldVOID(pStruct, m_pModuleID->m_id, sizeof(m_pModuleID->m_id), const_cast<char*>("Mod_ID"));
    pGFF->WriteFieldINT(pStruct, m_pModuleID->m_nCreatorId, const_cast<char*>("Mod_Creator_ID"));
    pGFF->WriteFieldDWORD(pStruct, m_pModuleID->m_nVersion, const_cast<char*>("Mod_Version"));
    pGFF->WriteFieldCExoLocString(pStruct, m_sName, const_cast<char*>("Mod_Name"));
    pGFF->WriteFieldCExoLocString(pStruct, m_sDescription, const_cast<char*>("Mod_Description"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bIsSaveGame), const_cast<char*>("Mod_IsSaveGame"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bIsNWMFile), const_cast<char*>("Mod_IsNWMFile"));
    if (m_bIsNWMFile != 0)
        pGFF->WriteFieldCExoString(pStruct, m_sNWMResName, const_cast<char*>("Mod_NWMResName"));
    pGFF->WriteFieldCExoString(pStruct, CExoString(m_resHak.GetResRefStr()),
                               const_cast<char*>("Mod_Hak"));
    pGFF->WriteFieldDWORD64(pStruct, CGameEffect::m_nNextID, const_cast<char*>("Mod_Effect_NxtId"));
    pGFF->WriteFieldDWORD(pStruct, pObjectArray->m_nNextCharacterObjectId[0],
                          const_cast<char*>("Mod_NextCharId0"));
    pGFF->WriteFieldDWORD(pStruct, pObjectArray->m_nNextCharacterObjectId[1],
                          const_cast<char*>("Mod_NextCharId1"));
    pGFF->WriteFieldDWORD(pStruct, pObjectArray->m_nNextObjectId[0], const_cast<char*>("Mod_NextObjId0"));
    pGFF->WriteFieldDWORD(pStruct, pObjectArray->m_nNextObjectId[1], const_cast<char*>("Mod_NextObjId1"));
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Mod_Tag"));
    pGFF->WriteFieldCResRef(pStruct, *reinterpret_cast<CResRef*>(m_pEntryInfo), const_cast<char*>("Mod_Entry_Area"));
    pGFF->WriteFieldFLOAT(pStruct, m_pEntryInfo->m_entryX, const_cast<char*>("Mod_Entry_X"));
    pGFF->WriteFieldFLOAT(pStruct, m_pEntryInfo->m_entryY, const_cast<char*>("Mod_Entry_Y"));
    pGFF->WriteFieldFLOAT(pStruct, m_pEntryInfo->m_entryZ, const_cast<char*>("Mod_Entry_Z"));
    pGFF->WriteFieldFLOAT(pStruct, m_pEntryInfo->m_entryDirX, const_cast<char*>("Mod_Entry_Dir_X"));
    pGFF->WriteFieldFLOAT(pStruct, m_pEntryInfo->m_entryDirY, const_cast<char*>("Mod_Entry_Dir_Y"));
    pGFF->WriteFieldBYTE(pStruct, m_nMinutesPerHour, const_cast<char*>("Mod_MinPerHour"));
    pGFF->WriteFieldBYTE(pStruct, m_nDawnHour, const_cast<char*>("Mod_DawnHour"));
    pGFF->WriteFieldBYTE(pStruct, m_nDuskHour, const_cast<char*>("Mod_DuskHour"));

    unsigned int nDay, nTimeOfDay, nHour, nMinute, nSecond, nMillisecond;
    unsigned int nPauseDay, nPauseTime;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->ConvertFromTimeOfDay(
        nTimeOfDay, &nHour, &nMinute, &nSecond, &nMillisecond);
    nPauseDay = g_pAppManager->m_pServerExoApp->GetWorldTimer()->m_nPauseDay;
    nPauseTime = g_pAppManager->m_pServerExoApp->GetWorldTimer()->m_nPauseTime;

    pGFF->WriteFieldDWORD(pStruct, m_nLastCalendarYear, const_cast<char*>("Mod_StartYear"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nLastCalendarMonth), const_cast<char*>("Mod_StartMonth"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nLastCalendarDay), const_cast<char*>("Mod_StartDay"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nLastCalendarHour), const_cast<char*>("Mod_StartHour"));
    pGFF->WriteFieldDWORD(pStruct, m_nTimeOfDayElapsed, const_cast<char*>("Mod_Transition"));
    pGFF->WriteFieldWORD(pStruct, static_cast<unsigned short>(nMinute), const_cast<char*>("Mod_StartMinute"));
    pGFF->WriteFieldWORD(pStruct, static_cast<unsigned short>(nSecond), const_cast<char*>("Mod_StartSecond"));
    pGFF->WriteFieldWORD(pStruct, static_cast<unsigned short>(nMillisecond), const_cast<char*>("Mod_StartMiliSec"));
    pGFF->WriteFieldDWORD(pStruct, nPauseDay, const_cast<char*>("Mod_PauseDay"));
    pGFF->WriteFieldDWORD(pStruct, nPauseTime, const_cast<char*>("Mod_PauseTime"));
    pGFF->WriteFieldBYTE(pStruct, m_nXPScale, const_cast<char*>("Mod_XPScale"));

    SaveLimboCreatures(pGFF, pStruct);

    // -- the IFO's own expansion list --
    pGFF->AddList(&list, pStruct, const_cast<char*>("Mod_Expan_List"));
    pNode = m_expansionList.m_pInternal->m_pHead;
    while (pNode != 0)
    {
        NWModuleExpansion_st* pExpansion =
            static_cast<NWModuleExpansion_st*>(m_expansionList.m_pInternal->GetAtPos(pNode));
        pGFF->AddListElement(&sElement, &list, 0);
        pGFF->WriteFieldCExoLocString(&sElement, pExpansion->m_name, const_cast<char*>("Expansion_Name"));
        pGFF->WriteFieldINT(&sElement, pExpansion->m_nID, const_cast<char*>("Expansion_ID"));
        m_expansionList.m_pInternal->GetNext(pNode);
    }

    // -- the IFO's own cutscene list --
    pGFF->AddList(&list, pStruct, const_cast<char*>("Mod_CutSceneList"));
    pNode = m_cutSceneList.m_pInternal->m_pHead;
    while (pNode != 0)
    {
        NWModuleCutScene_st* pCutScene =
            static_cast<NWModuleCutScene_st*>(m_cutSceneList.m_pInternal->GetAtPos(pNode));
        pGFF->AddListElement(&sElement, &list, 1);
        pGFF->WriteFieldCResRef(&sElement, pCutScene->m_resName, const_cast<char*>("CutScene_Name"));
        pGFF->WriteFieldDWORD(&sElement, pCutScene->m_nID, const_cast<char*>("CutScene_ID"));
        m_cutSceneList.m_pInternal->GetNext(pNode);
    }

    // The fifteen module scripts, in non-monotonic order.
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_HEARTBEAT]),
                            const_cast<char*>("Mod_OnHeartbeat"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_USER_DEFINED]),
                            const_cast<char*>("Mod_OnUsrDefined"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_CLIENT_ENTER]),
                            const_cast<char*>("Mod_OnClientEntr"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_CLIENT_LEAVE]),
                            const_cast<char*>("Mod_OnClientLeav"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_ACTIVATE_ITEM]),
                            const_cast<char*>("Mod_OnActvtItem"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_ACQUIRE_ITEM]),
                            const_cast<char*>("Mod_OnAcquirItem"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_UNACQUIRE_ITEM]),
                            const_cast<char*>("Mod_OnUnAqreItem"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_MODULE_LOAD]),
                            const_cast<char*>("Mod_OnModLoad"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_MODULE_START]),
                            const_cast<char*>("Mod_OnModStart"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_PLAYER_DEATH]),
                            const_cast<char*>("Mod_OnPlrDeath"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_PLAYER_DYING]),
                            const_cast<char*>("Mod_OnPlrDying"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_SPAWN_BUTTON_DOWN]),
                            const_cast<char*>("Mod_OnSpawnBtnDn"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_PLAYER_REST]),
                            const_cast<char*>("Mod_OnPlrRest"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_aScript[SCRIPT_PLAYER_LEVEL_UP]),
                            const_cast<char*>("Mod_OnPlrLvlUp"));

    // -- one area-list entry for the module's own current area --
    pGFF->AddList(&list, pStruct, const_cast<char*>("Mod_Area_list"));
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(m_oidArea);
    CResRef areaRef = pArea->m_resRef;
    pGFF->AddListElement(&sElement, &list, 6);
    pGFF->WriteFieldCResRef(&sElement, areaRef, const_cast<char*>("Area_Name"));
    pGFF->WriteFieldDWORD(&sElement, static_cast<unsigned int>(m_oidArea), const_cast<char*>("ObjectId"));

    // -- the module's own view of the load bar, then the custom-token table --
    g_pAppManager->m_pLoadGameErrorInfo->m_loadBar.m_progress = 1;
    pGFF->AddList(&list, pStruct, const_cast<char*>("Mod_Tokens"));
    if (g_pTlkTable->m_nCustomTokenCount != 0)
    {
        for (uint64_t i = 0; i < static_cast<uint64_t>(g_pTlkTable->m_nCustomTokenCount); i++)
        {
            if (static_cast<unsigned int>(g_pTlkTable->m_pCustomTokens[i].m_nId)
                <= static_cast<unsigned int>(CTlkTable::CUSTOMTOKEN_RESERVED_END))
                continue;
            pGFF->AddListElement(&sElement, &list, 7);
            pGFF->WriteFieldDWORD(&sElement, g_pTlkTable->m_pCustomTokens[i].m_nId,
                                  const_cast<char*>("Mod_TokensNumber"));
            pGFF->WriteFieldCExoString(&sElement, g_pTlkTable->m_pCustomTokens[i].m_sValue,
                                       const_cast<char*>("Mod_TokensValue"));
        }
    }

    m_scriptVarTable.SaveVarTable(pGFF, pStruct);
    m_varTable.SaveVarTable(pGFF, pStruct);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SaveEventQueue(pGFF, pStruct);
    return 1;
}
