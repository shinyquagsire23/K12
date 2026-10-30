// CSWSArea is the server-side area object. Its walkmesh and pathfinding queries live in
// nwsareapathfind.cpp.

#include "k2stdint.h"
#include <new>

#include "nwsarea.h"
#include "nwsobject.h"
#include "nwseffect.h"
#include "nwsscriptevent.h"
#include "nwscriptconst.h"
#include "restype.h"

// The number of mines the player's party may have set in one area.
enum {
    MAX_PLAYER_PARTY_MINES = 15,
};

// m_nWeatherOverride when no weather is forced, and SetCurrentWeather's request to derive the
// change from the current weather.
enum {
    WEATHER_NONE = 0xff,
};

// The struct ids of the GIT's list elements and sub-structs.
enum {
    GIT_STRUCT_ID_STORE           = 11,
    GIT_STRUCT_ID_AREA_OF_EFFECT  = 13,
    GIT_STRUCT_ID_CAMERA          = 14,
    GIT_STRUCT_ID_AREA_PROPERTIES = 100,
    GIT_STRUCT_ID_AREA_MAP        = 101,
};
#include "pathfindinfo.h"
#include "serveraimaster.h"
#include "nwscript.h"
#include "nwsscriptevent.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "serverexoappinternal.h"
#include "clientexoapp.h"

#ifdef K2_FIX_ENGINE_BUGS
extern "C" void k2_fix_object_not_indexed(unsigned long long objectId, int nObjectType, float fX,
                                          float fY);
#endif
#include "guiingame.h"
#include "gameobjectarray.h"
#include "CExoArrayListImpl.h"
#include "nwsobject.h"
#include "nwssoundobject.h"
#include "nwsstore.h"
#include "nwsencounter.h"
#include "nwsitem.h"
#include "nwscreature.h"
#include "nwsdoor.h"
#include "nwsplaceable.h"
#include "SWCollisionMesh.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "swsroom.h"
#include "swroomsurfacemesh.h"
#include "nwstrigger.h"
#include "partytables.h"
#include "nwcarea.h"
#include "exostring.h"
#include "nwsrules.h"
#include "nwsmodule.h"

#include "nwarea.h"
#include "aurtexcontrol.h"
#include "nwsambientsound.h"
#include "worldtimer.h"
#include "CResHelper.h"
#include "resare.h"
#include "resgff.h"
#include "swsareamap.h"
#include "nwswaypoint.h"
#include "nwsspellimpact.h"
#include "exoresman.h"
#include "nwsplayer.h"
#include "nwssendplayermessage.h"
#include "lyt.h"

// One entry of an area's expansion list, a localized name plus the expansion id.
struct NWAreaExpansion_st
{
    CExoLocString m_name;
    int           m_nID;
};

int g_nBSPWidth = 40;
int g_nBSPHeight = 40;

const float CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE = 250.0f;
const float CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE = 1000.0f;
const float CSWSArea::MAX_OBJECT_CHATWHISPER_DISTANCE = 3.0f;
const float CSWSArea::MAX_OBJECT_CONVERSATION_DISTANCE = 1000.0f;
const float CSWSArea::MAX_OBJECT_START_CONVERSATION_DISTANCE = 10.0f;
const float CSWSArea::MAX_OBJECT_CHATTALKSILENT_DISTANCE = 35.0f;
const float CSWSArea::MAX_OBJECT_CHATSHOUTSILENT_DISTANCE = 1000.0f;

#include "port/libcstring.h"
#include "port/crtapi.h"
#include "exostringinline.h"

// Debug walk-square overlay used by the testpathpoints console command.
void AurRemoveAllDrawableSquares();

// Axis-aligned box vs segment test.
unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& boxMin,
                          const Vector& boxMax);

// Line-of-sight occlusion factor between two points, 0 (clear) to 1: marches the segment in
// 0.1-unit steps and charges nothing for open space, 0.4 per placeable and 0.55 per door.
float GetObstructionFactor(Vector vFrom, Vector vTo)
{
    Vector vZero = Vector(0.0f, 0.0f, 0.0f);
    uint64_ll_t nStart = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned int nStartSec = static_cast<unsigned int>(nStart / 1000);
    float fFactor = 0.0f;

    if (equal(vTo, vZero) == 0 && g_pAppManager->m_pServerExoApp != 0)
    {
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        if (pModule != 0)
        {
            CSWSArea* pArea = pModule->GetArea();
            if (pArea != 0)
            {
                Vector vHit = Vector(0.0f, 0.0f, 0.0f);
                unsigned long long nHitId = OBJECT_INVALID;

                if (pArea->ClearLineOfSight(vFrom, vTo, &vHit, &nHitId, OBJECT_INVALID, OBJECT_INVALID,
                                            0, 0) == 0)
                {
                    do
                    {
                        if (nHitId == OBJECT_INVALID)
                        {
                            fFactor += 0.4f;
                        }
                        else
                        {
                            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nHitId);
                            if (pObject != 0)
                            {
                                if (pObject->AsSWSPlaceable() != 0)
                                    fFactor += 0.4f;
                                if (pObject->AsSWSDoor() != 0)
                                    fFactor += 0.55f;
                            }
                        }

                        if (fFactor > 1.0f)
                        {
                            fFactor = 1.0f;
                            break;
                        }

                        vFrom = vHit;
                        vFrom = vFrom + normalize(vTo - vFrom) * 0.1f;

                        unsigned long long nPrevHitId = nHitId;
                        nHitId = OBJECT_INVALID;
                        uint64_ll_t nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
                        int bClear = pArea->ClearLineOfSight(vFrom, vTo, &vHit, &nHitId,
                                                             nPrevHitId, OBJECT_INVALID, 0, 0);
                        if (static_cast<unsigned int>(nNow / 1000) - nStartSec > 99 || bClear != 0)
                            break;
                    } while (true);
                }
            }
        }
    }

    return fFactor;
}

void CSWSArea::AttachmentFromClientArea(CSWCArea* pClientArea)
{
    m_pClientArea = pClientArea;
}

void CSWSArea::DetachFromClientArea()
{
    m_pClientArea = 0;
}

void CSWSArea::SetMaxStealthXP(unsigned int xp)
{
    if (m_currentStealthXP > xp)
        m_currentStealthXP = xp;
    m_maxStealthXP = xp;
}

void CSWSArea::SetCurrentStealthXP(unsigned int xp)
{
    unsigned int max = m_maxStealthXP;
    m_currentStealthXP = (max < xp) ? max : xp;
}

unsigned char CSWSArea::GetNextTransitionPendingID()
{
    unsigned int next = static_cast<unsigned int>(m_transitionPendingID) + 1;
    unsigned char id;
    if (next & 256)
        id = 1;
    else
        id = static_cast<unsigned char>(next);
    m_transitionPendingID = id;
    return id;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSArea::SetDirtyARGB(int index, unsigned int argb)
{
    m_dirtyARGB[index] = argb;
}

void CSWSArea::SetDirtySize(int index, unsigned int size)
{
    m_dirtySize[index] = size;
}

void CSWSArea::SetDirtyFunction(int index, unsigned int fn)
{
    m_dirtyFunction[index] = fn;
}
#endif

// Pumps the area's events: timed events, body-bag spawns, point effects, and script events
// forwarded to the virtual machine.
void CSWSArea::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
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
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SPAWN_BODY_BAG))
    {
        if (pData == 0) return;
        CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
        if (pObjectArray == 0) return;
        // The payload of EVENT_SPAWN_BODY_BAG is the new body bag's id, then the dying object's
        // position.
        struct BodyBagEventData {
            unsigned long long m_bodyBagId;
            Vector             m_position;
        };
        BodyBagEventData* pEventData = static_cast<BodyBagEventData*>(pData);
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(pEventData->m_bodyBagId, &pObject)
                != CGameObjectArray::SUCCESS) {
            return;
        }
        CSWSPlaceable* pBodyBag = pObject->AsSWSPlaceable();
        if (pBodyBag == 0) return;
        pBodyBag->AddToArea(this, pEventData->m_position, 0);
        // Only a corpse shoves the living out of the way -- an ordinary loot bag is walkable.
        if (pBodyBag->m_corpse.m_bIsCorpse != 0)
        {
            Vector vMeshMin;
            Vector vMeshMax;
            vMeshMin.x = 0.0f;
            vMeshMin.y = 0.0f;
            vMeshMin.z = 0.0f;
            vMeshMax.x = 0.0f;
            vMeshMax.y = 0.0f;
            vMeshMax.z = 0.0f;
            pBodyBag->m_walkMesh.GetMeshBoundingBox(vMeshMin, vMeshMax);
            BudgeCreatures(pEventData->m_position, vMeshMin, vMeshMax, OBJECT_INVALID, 0);
        }
        return;
    }
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_APPLY_EFFECT))
    {
        if (pData == 0) return;
        CGameEffect* pEffect = static_cast<CGameEffect*>(pData);
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_VISUALEFFECT) return;
        float fX = pEffect->GetFloat(0);
        float fY = pEffect->GetFloat(1);
        float fZ = pEffect->GetFloat(2);
        Vector vPosition;
        vPosition.x = fX;
        vPosition.y = fY;
        vPosition.z = fZ;
        Vector vOrientation;  // zero
        ApplyEffect(pEffect, vPosition, vOrientation);
        return;
    }

    CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT))
    {
        switch (pEvent->m_eventType)
        {
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
            m_lastUpdateDay = nDay;
            m_lastUpdateTod = nTod;
            g_pVirtualMachine->RunScript(&m_scriptOnHeartbeat, m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
            m_nUserDefinedEventNumber = pEvent->GetInteger(0);
            g_pVirtualMachine->RunScript(&m_scriptOnUserDefined, m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER:
            m_oidEnteringObject = nCallerId;
            {
                // The OnEnter script runs with the load-from-save flag forced to the event's own
                // parameter 0, then restored.
                int bWasLoadFromSaveGame = g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame();
                g_pAppManager->m_pServerExoApp->SetLoadFromSaveGame(pEvent->GetInteger(0));
                g_pVirtualMachine->RunScript(&m_scriptOnEnter, m_objectId, 1);
                g_pAppManager->m_pServerExoApp->SetLoadFromSaveGame(bWasLoadFromSaveGame);
            }
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT:
            m_onExitScriptCallerId = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptOnExit, m_objectId, 1);
            break;
        }
    }
    else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_AREA_TRANSITION))
    {
        if (m_bTransitionPending != 0)
        {
            // A transition is already armed and this is its own re-fired event: only the one
            // carrying the in-flight id wins, so a stale queued transition is dropped.
            if (pEvent->GetInteger(0) == m_activeTransitionID)
            {
                g_pAppManager->m_pServerExoApp->SetMoveToModulePending(1);
                g_pAppManager->m_pServerExoApp->SetMoveToModuleString(pEvent->GetString(0));
                g_pAppManager->m_pServerExoApp->SetMoveToModuleStartWaypoint(
                    pEvent->GetString(1));
                for (unsigned char i = 2; i < 8; i++)
                {
                    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(pEvent->GetString(i));
                }
                m_bTransitionPending = 0;
                m_activeTransitionID = 0;
                g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);
            }
        }
        else
        {
            // Nothing armed yet: take the next pending id, copy the whole event forward, fade
            // out and re-fire 500ms later to land in the arm above.
            CScriptEvent* pTransition = new CScriptEvent();
            // The same roll as GetNextTransitionPendingID, 1..255, never 0.
            unsigned int nNext = static_cast<unsigned int>(m_transitionPendingID) + 1;
            unsigned char nPendingID;
            if (nNext & 256)
                nPendingID = 1;
            else
                nPendingID = static_cast<unsigned char>(nNext);
            m_transitionPendingID = nPendingID;
            pTransition->SetInteger(0, nPendingID);
            for (unsigned char i = 0; i < 8; i++)
            {
                pTransition->SetString(i, pEvent->GetString(i));
            }
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 500, m_objectId, m_objectId, nEventType, pTransition);
            CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
            Vector vFade;  // black
            pInGameGui->StartGlobalFade(0, 0.0f, 0.5f, vFade);
            m_bTransitionPending = 1;
            m_activeTransitionID = nPendingID;
            g_pAppManager->m_pClientExoApp->SetInputActive(0, 1);
        }
    }
    else
    {
        return;
    }
    delete pEvent;
}

// Sets the area's armed-transition state and its in-flight transition id, then locks or
// unlocks game input through the client app.
void CSWSArea::SetTransitionPending(int bState, unsigned char nTransitionID)
{
    m_bTransitionPending = bState;
    m_activeTransitionID = nTransitionID;
    if (bState)
        g_pAppManager->m_pClientExoApp->SetInputActive(0, 1);
    else
        g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);
}

// Registers a player entering; only the 0->1 edge promotes the area's AI priority.
void CSWSArea::IncrementPlayersInArea()
{
    if (m_nPlayersInArea == 0)
        IncreaseAILevelPriority();
    m_nPlayersInArea++;
}

// Deregisters a player; the 1->0 edge demotes the AI priority.
void CSWSArea::DecrementPlayersInArea()
{
    if (--m_nPlayersInArea == 0)
        DecreaseAILevelPriority();
}

// Starts an iteration over the X-sorted object index: resets the cursor and, if the index is
// empty, reports no elements.
int CSWSArea::GetFirstObjectInArea(unsigned long long& objId)
{
    m_nObjectsByXIndex = 0;
    if (m_aObjectsByX.m_count == 0)
        return 0;
    objId = m_aObjectsByX.m_pArray[0];
    return 1;
}

// Hands out the next id in the X-sorted object index, or 0 once exhausted.
int CSWSArea::GetNextObjectInArea(unsigned long long& objId)
{
    int index = m_nObjectsByXIndex + 1;
    m_nObjectsByXIndex = index;
    if (index >= m_aObjectsByX.m_count)
        return 0;
    objId = m_aObjectsByX.m_pArray[index];
    return 1;
}

// Normalizes the dirty-array formula value: 1 becomes 0 and 0 becomes 1, anything else is
// stored as-is.
#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSArea::SetDirtyFormula(int index, unsigned int formula)
{
    if (formula == 1)
        formula = 0;
    else if (formula == 0)
        formula = 1;
    m_dirtyFormula[index] = formula;
}
#endif

// Stores the player-restrict mode; on a real change the whole party is unstealthed first.
void CSWSArea::SetPlayerRestrictMode(int nMode)
{
    if (nMode != 0 && m_nPlayerRestrictMode != nMode)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->UnstealthParty(1);
    m_nPlayerRestrictMode = nMode;
}

// Packs an RGB vector (0..1 per channel) into the packed fog color: x in the low byte, then y
// and z.
void CSWSArea::SetFogColor(const Vector& color)
{
    unsigned char r = static_cast<unsigned char>(color.x * 255.0f);
    unsigned char g = static_cast<unsigned char>(color.y * 255.0f);
    unsigned char b = static_cast<unsigned char>(color.z * 255.0f);
    m_nFogColor = r | (static_cast<unsigned int>(g) << 8) | (static_cast<unsigned int>(b) << 16);
}

// The client-side area for this server area, attaching on first use.
CSWCArea* CSWSArea::GetClientArea()
{
    if (m_pClientArea == 0)
    {
        m_pClientArea = g_pAppManager->m_pClientExoApp->GetAreaByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId));
        if (m_pClientArea != 0)
            m_pClientArea->AttachmentFromServerArea(this);
    }
    return m_pClientArea;
}

// Deletes every object listed in the X-sorted object index and zeroes the count.
void CSWSArea::ClearObjects()
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    for (int i = m_aObjectsByX.m_count - 1; i >= 0; i--)
    {
        // GetGameObject always writes the out-param, so pObject needs no initializer.
        CGameObject* pObject;
        pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i], &pObject);
        if (pObject != 0)
            delete pObject;
    }
    m_aObjectsByX.m_count = 0;
}

// Banks the area's accumulated stealth XP onto the creature, sharing it with the party,
// flashing the in-game status, then resetting the bank. Guards: the creature must exist,
// stealth XP must be enabled, and the bank must be nonzero.
void CSWSArea::AwardStealthXP(unsigned long long objectId)
{
    if (g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId) == 0)
        return;
    if (m_bStealthXPEnabled == 0)
        return;
    if (m_currentStealthXP == 0)
        return;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(m_currentStealthXP, 1);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_STEALTH_XP,
                                                                     m_currentStealthXP);
    m_currentStealthXP = 0;
    m_bStealthXPEnabled = 0;
}

// Counts the mines/traps currently planted in the area by the player party: only triggers
// that are traps and player-set count.
int CSWSArea::GetNumMinesSetByPlayerParty()
{
    int nMines = 0;
    for (int i = 0; i < m_mineObjects.m_count; i++)
    {
        CSWSTrigger* pTrigger = g_pAppManager->m_pServerExoApp->GetTriggerByGameObjectID(
            m_mineObjects.m_pArray[i]);
        if (pTrigger != 0 && pTrigger->m_bTrap != 0 && pTrigger->m_bSetByPlayerParty != 0)
            nMines++;
    }
    return nMines;
}

// True while the party is still under the mine cap of 15.
int CSWSArea::GetPlayerCanSetMines()
{
    return static_cast<unsigned char>(GetNumMinesSetByPlayerParty()) < MAX_PLAYER_PARTY_MINES;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
// Seeds a dirty texture from the current area's own per-index dirty arrays and hands it to
// the base Init.
void DirtyTextureController::Init()
{
    if (g_pAppManager->m_pServerExoApp != 0 && g_pAppManager->m_pServerExoApp->GetModule() != 0) {
        int nSize = g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_dirtySize[m_nDirtyIndex];
        int nColor = g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_dirtyARGB[m_nDirtyIndex];
        int nFormula = g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_dirtyFormula[m_nDirtyIndex];
        int nFunction = g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_dirtyFunction[m_nDirtyIndex];
        if (nSize != 0)
            Init(nColor, nSize, nFormula, nFunction);
        else
            Init(0, 16, 0, 0);
        return;
    }
    Init(0, 16, 0, 0);
}
#endif

// Interpolates the Z of a point on the segment a->b along whichever axis has the larger
// component delta, falling back to a.z when the y delta is zero.
float CSWSArea::ComputeZFromLineSegment(const Vector& a, const Vector& b, const Vector& c)
{
    if (a.x - b.x > a.y - b.y)
        return a.z + (b.z - a.z) * ((c.x - a.x) / (b.x - a.x));
    if (a.y != b.y)
        return a.z + (b.z - a.z) * ((c.y - a.y) / (b.y - a.y));
    return a.z;
}

// The room's walkmesh surface material at a position, or 0 when no room covers it.
unsigned int CSWSArea::GetSurfaceMaterial(Vector pos)
{
    CSWSRoom* pRoom = GetRoom(pos, 0, 0);
    if (pRoom != 0)
        return pRoom->m_pSurfaceMesh->GetSurfaceMaterial(pos);
    return 0;
}

// True when the surface material at a position is flagged walkable in the surface-material table.
int CSWSArea::PositionWalkable(Vector pos)
{
    CSWSRoom* pRoom = GetRoom(pos, 0, 0);
    if (pRoom == 0)
        return 0;
    unsigned int nMat = pRoom->m_pSurfaceMesh->GetSurfaceMaterialWalkCheckOnly(pos);
    C2DA* pSurface = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
    int nWalk;
    {
        CExoString sWalk("Walk");
        pSurface->GetINTEntry(nMat, sWalk, &nWalk);
    }
    return nWalk != 0;
}

void CSWSArea::IncreaseAILevelPriority()
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();

    for (int n = 0; n < m_aObjectsByX.m_count; n++)
    {
        CGameObject* pGameObject;
        if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[n], &pGameObject) != CGameObjectArray::SUCCESS)
            continue;
        CSWSObject* pObject = static_cast<CSWSObject*>(pGameObject);
        if (pObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA || pObject->m_aiLevel != 0)
            continue;

        if (pGameObject->AsSWSCreature() != 0 || pGameObject->AsSWSAreaOfEffectObject() != 0 ||
            pGameObject->AsSWSTrigger() != 0)
        {
            pAIMaster->SetAILevel(pObject, 1);
        }
        else if (pGameObject->AsSWSDoor() != 0)
        {
            CSWSDoor* pDoor = pGameObject->AsSWSDoor();
            if (pDoor->m_scriptSlots[5].GetLength() >= 1 && pDoor->m_bStatic == 0)
                pAIMaster->SetAILevel(pObject, 1);
        }
        else if (pGameObject->AsSWSPlaceable() != 0 && pGameObject->AsSWSPlaceable()->m_bStatic == 0)
        {
            pAIMaster->SetAILevel(pObject, 1);
        }
    }
}

void CSWSArea::DecreaseAILevelPriority()
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();

    for (int n = 0; n < m_aObjectsByX.m_count; n++)
    {
        CSWSObject* pObj;
        if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[n], reinterpret_cast<CGameObject**>(&pObj)) != CGameObjectArray::SUCCESS)
            continue;
        if (pObj->m_objectType > CGameObject::OBJECT_TYPE_AREA && pObj->AsSWSCreature() != 0
            && pObj->m_aiLevel == 1)
            pAIMaster->SetAILevel(pObj, 0);
    }
}

void CSWSArea::UnloadArea()
{
    if (m_pStaticTriangleIndices)
        delete[] m_pStaticTriangleIndices;
    m_pStaticTriangleIndices = 0;

    if (m_pStaticVertices)
        delete[] m_pStaticVertices;
    m_pStaticVertices = 0;

    if (m_pStaticObjectFirstTriangles)
        delete[] m_pStaticObjectFirstTriangles;
    m_pStaticObjectFirstTriangles = 0;

    if (m_pStaticObjectTriangleCounts)
        delete[] m_pStaticObjectTriangleCounts;
    m_pStaticObjectTriangleCounts = 0;

    if (m_pStaticObjectFirstVertices)
        delete[] m_pStaticObjectFirstVertices;
    m_pStaticObjectFirstVertices = 0;

    if (m_pStaticObjectVertexCounts)
        delete[] m_pStaticObjectVertexCounts;
    m_pStaticObjectVertexCounts = 0;

    while (m_expansionList.m_pInternal->m_pHead)
        delete m_expansionList.RemoveHead();

    if (m_pHeaderIds)
        delete m_pHeaderIds;
    m_pHeaderIds = 0;

    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule)
        pModule->RemoveObjectFromLookupTable(m_sDebugName, m_objectId);

    if (m_paRooms)
        delete[] m_paRooms;
    m_paRooms = 0;
}

// Binary search for the first object whose X is at or past fX; the index is written to
// pOutIndex.
int CSWSArea::GetFirstObjectIndiceByX(int* pOutIndex, float fX)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    int nLast = m_aObjectsByX.m_count - 1;
    int lo = 0;
    int hi = nLast;

    CExoArrayList<unsigned long long>& aObjects = m_aObjectsByX;
    while (lo < hi || lo == nLast)
    {
        int mid = (lo + hi) >> 1;
        CGameObject* pObj;
        if (pObjectArray->GetGameObject(aObjects.m_pArray[mid], &pObj) != CGameObjectArray::SUCCESS)
            return 0;
        if (fX <= static_cast<CSWSObject*>(pObj)->m_position.x)
        {
            if (mid == hi)
            {
                *pOutIndex = hi;
                return 1;
            }
            hi = mid;
        }
        else
        {
            lo = mid + 1;
        }
    }

    if (lo != hi)
        return 0;
    *pOutIndex = hi;
    return 1;
}

// Registers an object in the X-sorted index, inserting at its sorted position.
int CSWSArea::AddObjectToArea(unsigned long long objectId, int n)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;
    if (pObjectArray->GetGameObject(objectId, &pObject) != CGameObjectArray::SUCCESS)
        return 0;

    // If already registered, remove it first and re-insert at the sorted position.
    if (m_aObjectsByX.Contains(objectId))
        RemoveObjectFromArea(objectId);

    float fX = static_cast<CSWSObject*>(pObject)->m_position.x;
    int nIndice;
    if (GetFirstObjectIndiceByX(&nIndice, fX))
        m_aObjectsByX.Insert(objectId, nIndice);
    else
        m_aObjectsByX.Add(objectId);

    // A creature with a client representation counts toward the area's player count, and
    // signals OnEnter when it arrives with n == 1.
    CGameObject* pObject2;
    if (pObjectArray->GetGameObject(objectId, &pObject2) == CGameObjectArray::SUCCESS &&
        pObject2 != 0 && pObject2->AsSWSCreature() != 0)
    {
        CSWSPlayer* pClient = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
        if (pClient != 0)
        {
            if (m_nPlayersInArea == 0)
                IncreaseAILevelPriority();
            m_nPlayersInArea++;
        }

        if (n == 1)
        {
            CScriptEvent* pEvent = new CScriptEvent;
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            int nLoadGame = g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame();
            pEvent->SetInteger(0, nLoadGame);
            CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
            pAIMaster->AddEventDeltaTime(0, 0, objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }
    return 1;
}

// Unregisters an object from the X-sorted index.
int CSWSArea::RemoveObjectFromArea(unsigned long long objectId)
{
    if (m_aObjectsByX.Contains(objectId))
        m_aObjectsByX.Remove(objectId);

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;
    if (pObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 &&
        (pObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA ||
         static_cast<CSWSObject*>(pObject)->GetDead() == 0) &&
        pObject->AsSWSCreature() != 0)
    {
        CSWSPlayer* pClient = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
        if (pClient != 0)
        {
            m_nPlayersInArea--;
            if (m_nPlayersInArea == 0)
                DecreaseAILevelPriority();
        }

        CScriptEvent* pEvent = new CScriptEvent;
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
        CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        pAIMaster->AddEventDeltaTime(0, 0, objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }
    return 1;
}

CSWSArea::CSWSArea(const CResRef& cResRef, int nFlags, unsigned long long nAreaId)
    : CResHelper<CResARE, 2012>(), CGameObject(CGameObject::OBJECT_TYPE_AREA, nAreaId)
{
    // The members this class adds on top of the CSWArea, CResHelper and CGameObject bases.
    m_pHeaderIds = new CSWSAreaHeaderIds;
    m_nAreaFlags = 0;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nPlayersInArea = 0;
    m_nReserved1 = 0;
    m_lastUpdateDay = 0;
    m_lastUpdateTod = 0;
    m_nListenModifier = 0;
    m_chanceFog = 0;
    m_modSpotCheck = 0;
    m_pActivePathfindInfo = 0;
    m_paGridDFSTransTable = 0;
    m_nPathPointCount = 0;
    m_paPathPoints = 0;
    m_nPathPointLinkCount = 0;
    m_paPathPointLinks = 0;
    m_paPathPointDFSTransTable = 0;
    m_paRoomNames = 0;
    m_paRoomEnvAudio = 0;
    m_paRoomAmbientScale = 0;
    m_nRoomAudioCount = 0;
    m_nPartSoundCount = 0;
#if !KOTOR1
    m_paForceRating = 0;
    m_paRoomDisableWeather = 0;
    m_paRoomTableIndex = 0;
#endif
    m_nRoomCount = 0;
    m_paRooms = 0;
    m_pClientArea = 0;
    m_bTransitionTargetMarked = 0;

    // Bind the area resref into the CResHelper base.
    SetResRef(cResRef, nFlags);

    m_pSoundPathInfo = new CPathfindInformation;
    InitializeStaticObjects();

    CGameObject* pGameObject = this;
    if (nAreaId == OBJECT_INVALID)
        g_pAppManager->m_pServerExoApp->GetObjectArray()->AddInternalObject(m_objectId, pGameObject, 0);
    else
        g_pAppManager->m_pServerExoApp->GetObjectArray()->AddObjectAtPos(nAreaId, pGameObject);

    m_pAmbientSound = new CSWSAmbientSound(m_objectId);

    unsigned int nHours, nMinutes;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nHours, &nMinutes);
    m_bWeatherOn = 0;
    m_nWeatherTimer = 0;
    m_nWeatherDay = nHours;
    m_nWeatherTod = nMinutes;
    m_nCurrentWeather = CSWCArea::WEATHER_CLEAR;
    m_nWeatherOverride = WEATHER_NONE;
    m_nSentWeather = 0;
    m_nTileBitmapBytes = 0;
    m_bAreaHeaderLoaded = 0;
    m_nLoadScreenID = 0;
    m_transitionPendingID = 0;
    m_activeTransitionID = 0;
    m_nUnescapable = 0;
#if !KOTOR1
    m_disableTransit = 0;
#endif
    m_nPlayerRestrictMode = 0;
    m_maxStealthXP = 0;
    m_currentStealthXP = 0;
    m_stealthXPDecrement = 0;
    m_bStealthXPEnabled = 0;
    m_bTransitionPending = 0;
}

CSWSArea::~CSWSArea()
{
    UnloadArea();

    g_pAppManager->m_pServerExoApp->GetObjectArray()->Delete(m_objectId);

    if (m_pSoundPathInfo)
    {
        delete m_pSoundPathInfo;
        m_pSoundPathInfo = 0;
    }
    if (m_pAmbientSound)
    {
        m_pAmbientSound->CSWSAmbientSound::~CSWSAmbientSound();
        operator delete(m_pAmbientSound);
        m_pAmbientSound = 0;
    }
    if (m_paPathPoints)
    {
        delete[] m_paPathPoints;
        m_paPathPoints = 0;
        m_nPathPointCount = 0;
    }
    if (m_paPathPointLinks)
    {
        delete[] m_paPathPointLinks;
        m_paPathPointLinks = 0;
        m_nPathPointLinkCount = 0;
    }
    if (m_paRoomNames)
    {
        delete[] m_paRoomNames;
        m_paRoomNames = 0;
    }
    if (m_paRoomEnvAudio)
    {
        delete[] m_paRoomEnvAudio;
        m_paRoomEnvAudio = 0;
    }
    if (m_paRoomAmbientScale)
    {
        delete[] m_paRoomAmbientScale;
        m_paRoomAmbientScale = 0;
    }
#if !KOTOR1
    if (m_paForceRating)
    {
        delete[] m_paForceRating;
        m_paForceRating = 0;
    }
    if (m_paRoomDisableWeather)
    {
        delete[] m_paRoomDisableWeather;
        m_paRoomDisableWeather = 0;
    }
    if (m_paRoomTableIndex)
    {
        delete[] m_paRoomTableIndex;
        m_paRoomTableIndex = 0;
    }
#endif
    if (m_pClientArea)
    {
        m_pClientArea->DetachFromServerArea();
        m_pClientArea = 0;
    }
    g_pAppManager->m_pServerExoApp->GetModule()->ClearAreaCache();
    g_pAppManager->m_pServerExoApp->ClearAreaCache();
}

// Reads the area layout (.lyt) and builds the room array from it; also seeds the area's tile
// extents.
int CSWSArea::LoadRoomInfo(CResStruct* pStruct)
{
    K2_UNUSED(pStruct);
    m_nWidth = g_nBSPWidth;
    m_nHeight = g_nBSPHeight;
    CLYT* pLyt = new CLYT;
    int nRet = pLyt->LoadLayout(m_resRef);
    if (nRet)
    {
        m_nRoomCount = pLyt->GetRoomCount();
        m_paRooms = LoadRooms(pLyt);
    }
    pLyt->UnloadLayout();
    delete pLyt;
    return nRet;
}

// Applies an effect to a point rather than to an object: area-of-effect spawns, summons,
// visual effects and linked effects. The effect is owned here on every path except a
// successful summon, which hands it to the creature.
void CSWSArea::ApplyEffect(CGameEffect* pEffect, const Vector& vPosition,
                           const Vector& vOrientation)
{
    if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_AREA_OF_EFFECT)
    {
        CSWSAreaOfEffectObject* pAreaEffect = new CSWSAreaOfEffectObject(OBJECT_INVALID);
        pAreaEffect->SetCreator(pEffect->m_creatorId);

        if ((pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) == SUBTYPE_MAGICAL && pEffect->m_creatorId != OBJECT_INVALID)
        {
            CGameObject* pObject =
                g_pAppManager->m_pServerExoApp->GetGameObject(pEffect->m_creatorId);
            if (pObject && pObject->AsSWSObject())
                pObject->AsSWSObject()->m_spellTargets.AddUnique(pAreaEffect->m_objectId);
        }

        pAreaEffect->LoadAreaEffect(pEffect->GetInteger(0));
        pAreaEffect->SetDuration(static_cast<unsigned char>(pEffect->m_durationTypeBits & 7),
                                 pEffect->m_fDuration);
        pAreaEffect->SetOrientation(vOrientation);
        pAreaEffect->SetEffectSpellId(pEffect->m_spellId);

        if (pEffect->GetString(0) != "")
            pAreaEffect->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_ENTER] = pEffect->GetString(0);
        if (pEffect->GetString(1) != "")
            pAreaEffect->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_HEARTBEAT] = pEffect->GetString(1);
        if (pEffect->GetString(2) != "")
            pAreaEffect->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_EXIT] = pEffect->GetString(2);

        pAreaEffect->AddToArea(this, vPosition.x, vPosition.y, vPosition.z, 1);
    }
    else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SUMMON_CREATURE)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pCreature)
        {
            pEffect->SetObjectID(0, m_objectId);
            pEffect->SetFloat(0, vPosition.x);
            pEffect->SetFloat(1, vPosition.y);
            pEffect->SetFloat(2, vPosition.z);
            pCreature->ApplyEffect(pEffect, 0, 0);
            return;
        }
    }
    else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_VISUALEFFECT)
    {
        PlayVisualEffect(static_cast<unsigned short>(pEffect->GetInteger(0)), vPosition);
    }
    else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LINK)
    {
        pEffect->UpdateLinked();
        if (pEffect->m_pLinkedEffect1)
            ApplyEffect(pEffect->m_pLinkedEffect1, vPosition, vOrientation);
        if (pEffect->m_pLinkedEffect2)
            ApplyEffect(pEffect->m_pLinkedEffect2, vPosition, vOrientation);
        pEffect->SetLinked(0, 0);
    }

    delete pEffect;
}

// Converts a parsed .lyt into the room array: builds each room, welds shared edge vertices so
// neighbouring rooms line up, records adjacency, then computes per-room normals.
CSWSRoom* CSWSArea::LoadRooms(CLYT* pLyt)
{
    CSWSRoom* paRooms = 0;
    int nRooms = pLyt->GetRoomCount();
    if (nRooms)
    {
        paRooms = new CSWSRoom[nRooms];

        for (int i = 0; i < nRooms; i++)
        {
            paRooms[i].SetRoomInfo(pLyt->GetRoomName(i), pLyt->GetRoomPosition(i));
            paRooms[i].LoadWalkMesh(0);
            paRooms[i].TransformToWorld();
        }

        for (int i = 0; i < nRooms; i++)
            for (int j = i + 1; j < nRooms; j++)
                paRooms[i].WeldRoom(paRooms[j], 0.01f);

        for (int i = 0; i < nRooms; i++)
            for (int j = i + 1; j < nRooms; j++)
                paRooms[i].SetRoomAdjacency(paRooms[j], i, j);

        for (int i = 0; i < nRooms; i++)
            paRooms[i].ComputeMeshNormals();
    }
    return paRooms;
}

// Reads the walk-route path points and their neighbour links from the area's "PTH " resource.
int CSWSArea::LoadPathPoints(CResStruct* pStruct)
{
    K2_UNUSED(pStruct);
    int nSuccess = 1;

    if (!g_pExoResMan->Exists(m_resRef, RESTYPE_PTH, static_cast<unsigned int*>(0)))
        return 1;

    CResGFF* pGff = new CResGFF(RESTYPE_PTH, const_cast<char*>("PTH "), m_resRef);

    if (!pGff->m_nValid)
    {
        delete pGff;
        return 0;
    }

    CResList list;
    int bSuccess;
    CResStruct st;
    CResStruct stElem;
    pGff->GetTopLevelStruct(&st);

    if (pGff->GetList(&list, &st, const_cast<char*>("Path_Points")))
    {
        m_nPathPointCount = pGff->GetListCount(&list);
        if (!m_nPathPointCount)
        {
            delete pGff;
            return 0;
        }
        m_paPathPoints = reinterpret_cast<CSWSAreaPathPoint*>(new char[m_nPathPointCount * sizeof(CSWSAreaPathPoint)]);
        nSuccess = 1;
        for (unsigned int i = 0; i < m_nPathPointCount; i++)
        {
            if (!pGff->GetListElement(&stElem, &list, i))
                continue;
            if (pGff->GetElementType(&stElem) != 2)
                continue;

            m_paPathPoints[i].m_node.m_x = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("X"), bSuccess, 0.0f);
            nSuccess &= bSuccess;
            m_paPathPoints[i].m_node.m_y = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Y"), bSuccess, 0.0f);
            nSuccess &= bSuccess;
            m_paPathPoints[i].m_node.m_nNeighborCount = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("Conections"), bSuccess, 0);
            nSuccess &= bSuccess;
            m_paPathPoints[i].m_nNeighborStart = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("First_Conection"), bSuccess, 0);
            nSuccess &= bSuccess;
        }
    }

    if (pGff->GetList(&list, &st, const_cast<char*>("Path_Conections")))
    {
        m_nPathPointLinkCount = pGff->GetListCount(&list);
        m_paPathPointLinks = new unsigned int[m_nPathPointLinkCount];
        if (m_nPathPointLinkCount)
        {
            for (unsigned int i = 0; i < m_nPathPointLinkCount; i++)
            {
                if (!pGff->GetListElement(&stElem, &list, i))
                    continue;
                if (pGff->GetElementType(&stElem) != 3)
                    continue;

                m_paPathPointLinks[i] = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("Destination"), bSuccess, 0);
                nSuccess &= bSuccess;
            }
        }
    }

    delete pGff;
    return nSuccess;
}

// Reads the area's .git companion resource and fans out to the per-type object loaders. An
// interior area has its weather forced to none.
int CSWSArea::LoadGIT(int nFlags)
{
    if (!g_pExoResMan->Exists(m_resRef, RESTYPE_GIT, static_cast<unsigned int*>(0)))
        return 0;
    CResGFF* pGff = new CResGFF(RESTYPE_GIT, const_cast<char*>("GIT "), m_resRef);
    if (!pGff->m_nValid)
    {
        delete pGff;
        m_pRes->Release();
        return 0;
    }
    CResStruct st;
    pGff->GetTopLevelStruct(&st);
    int bSuccess;
    if (nFlags)
    {
        m_scriptVars.LoadVarTable(pGff, &st);
        m_varTable.LoadVarTable(pGff, &st);
        m_nCurrentWeather = pGff->ReadFieldBYTE(&st, const_cast<char*>("CurrentWeather"), bSuccess, 0);
        m_bWeatherOn = pGff->ReadFieldBYTE(&st, const_cast<char*>("WeatherStarted"), bSuccess, 0);
        if (m_nAreaFlags & 1)
        {
            m_nCurrentWeather = WEATHER_NONE;
            m_bWeatherOn = 0;
        }
    }
    int nUseTemplates = pGff->ReadFieldBYTE(&st, const_cast<char*>("UseTemplates"), bSuccess, 0);
    LoadCreatures(pGff, &st, nFlags, nUseTemplates);
    LoadItems(pGff, &st, nFlags, nUseTemplates);
    LoadDoors(pGff, &st, nFlags, nUseTemplates);
    LoadTriggers(pGff, &st, nFlags, nUseTemplates);
    LoadEncounters(pGff, &st, nFlags, nUseTemplates);
    LoadWaypoints(pGff, &st, nFlags, nUseTemplates);
    LoadSounds(pGff, &st, nFlags, nUseTemplates);
    LoadPlaceables(pGff, &st, nFlags, nUseTemplates);
    LoadStores(pGff, &st, nFlags, nUseTemplates);
    LoadAreaEffects(pGff, &st, nFlags, nUseTemplates);
    LoadProperties(pGff, &st);
    LoadMaps(pGff, &st);
    LoadPlaceableCameras(pGff, &st);
    delete pGff;
    return 1;
}

// Loads the .git WaypointList: each marker gets the walkmesh height under it before being
// registered.
int CSWSArea::LoadWaypoints(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    K2_UNUSED(nUseTemplates);
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("WaypointList")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 5)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSWaypoint* pWaypoint = new CSWSWaypoint(nObjectId);
        if (!pWaypoint->LoadWaypoint(pGff, &stElem))
        {
            delete pWaypoint;
            continue;
        }
        if (nFlags)
            pWaypoint->LoadObjectState(pGff, &stElem);
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
        pWaypoint->AddToArea(this, fX, fY, fZ, !nFlags);
        Vector v = pWaypoint->m_position;
        v.z = ComputeHeight(v, 1, 0, 0);
        pWaypoint->SetPosition(v, 1, 1, 0);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git AreaEffectList, orienting and registering each area-of-effect object.
int CSWSArea::LoadAreaEffects(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    K2_UNUSED(nUseTemplates);
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("AreaEffectList")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != GIT_STRUCT_ID_AREA_OF_EFFECT)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSAreaOfEffectObject* pEffect = new CSWSAreaOfEffectObject(nObjectId);
        if (!pEffect->LoadEffect(pGff, &stElem))
        {
            delete pEffect;
            continue;
        }
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("OrientationX"), bSuccess, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("OrientationY"), bSuccess, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("OrientationZ"), bSuccess, 0.0f);
        Vector v;
        v.x = fX;
        v.y = fY;
        v.z = fZ;
        if (fX * fX + fY * fY + fZ * fZ > 0.0001f)
            v = normalize(v);
        else
        {
            v.x = 0.0f;
            v.y = 1.0f;
            v.z = 0.0f;
        }
        pEffect->SetOrientation(v);
        if (nFlags)
            pEffect->LoadObjectState(pGff, &stElem);
        float fPX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("PositionX"), bSuccess, 0.0f);
        float fPY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("PositionY"), bSuccess, 0.0f);
        float fPZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("PositionZ"), bSuccess, 0.0f);
        pEffect->AddToArea(this, fPX, fPY, fPZ, !nFlags);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git SoundList from a template or from stored data.
int CSWSArea::LoadSounds(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("SoundList")))
        return 1;
    int nCount = pGff->GetListCount(&list);
    if (!nCount)
        return 1;
    int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 6)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSSoundObject* pSound = new CSWSSoundObject(nObjectId);
        if (nUseTemplates)
        {
            CResRef refSound = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccess, CResRef(""));
            if (!bSuccess || !pSound->LoadFromTemplate(refSound))
            {
                delete pSound;
                continue;
            }
            pSound->m_generatedType = static_cast<unsigned char>(pGff->ReadFieldDWORD(&stElem, const_cast<char*>("GeneratedType"), bSuccess, 0));
            float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
            float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
            float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
            Vector v;
            v.x = fX;
            v.y = fY;
            v.z = fZ;
            pSound->SetPosition(v, 1, 1, 0);
        }
        else if (!pSound->Load(pGff, &stElem))
        {
            delete pSound;
            continue;
        }
        pSound->AddToArea(this, !nFlags);
    } while (++i != nCount);
    return 1;
}

// Loads the .git StoreList from a template or from stored data, orienting each store.
int CSWSArea::LoadStores(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("StoreList")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != GIT_STRUCT_ID_STORE)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSStore* pStore = new CSWSStore(nObjectId);
        if (nUseTemplates)
        {
            CResRef refStore = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("ResRef"), bSuccess, CResRef(""));
            if (!bSuccess || !pStore->LoadFromTemplate(refStore, 1))
            {
                delete pStore;
                continue;
            }
        }
        else if (!pStore->LoadStore(pGff, &stElem, 0))
        {
            delete pStore;
            continue;
        }
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XOrientation"), bSuccess, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YOrientation"), bSuccess, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZOrientation"), bSuccess, 0.0f);
        Vector v;
        v.x = fX;
        v.y = fY;
        v.z = fZ;
        if (fX * fX + fY * fY + fZ * fZ > 1e-5f)
            v = normalize(v);
        else
        {
            v.x = 0.0f;
            v.y = 1.0f;
            v.z = 0.0f;
        }
        pStore->SetOrientation(v);
        if (nFlags)
            pStore->LoadObjectState(pGff, &stElem);
        float fPX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
        float fPY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
        float fPZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
        pStore->AddToArea(this, fPX, fPY, fPZ, !nFlags);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git "Encounter List", including each encounter's geometry and spawn points.
int CSWSArea::LoadEncounters(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("Encounter List")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 7)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSEncounter* pEnc = new CSWSEncounter(nObjectId);
        if (nUseTemplates)
        {
            CResRef refEnc = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccess, CResRef(""));
            if (bSuccess && pEnc->LoadFromTemplate(refEnc, 0, 0))
            {
                float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
                float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
                float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
                CResList resList;   // reused for both "Geometry" and "SpawnPointList"
                pEnc->SetPosition(Vector(fX, fY, fZ), 1, 1, 0);
                if (pGff->GetList(&resList, &stElem, const_cast<char*>("Geometry")))
                {
                    if (!pEnc->LoadEncounterGeometry(pGff, resList))
                    {
                        delete pEnc;
                        pEnc = 0;
                    }
                }
                if (pGff->GetList(&resList, &stElem, const_cast<char*>("SpawnPointList")))
                    pEnc->LoadEncounterSpawnPoints(pGff, resList);
                if (pEnc)
                {
                    if (nFlags)
                        pEnc->LoadObjectState(pGff, &stElem);
                    pEnc->AddToArea(this, pEnc->m_position.x, pEnc->m_position.y, pEnc->m_position.z, !nFlags);
                }
                continue;
            }
            delete pEnc;
            continue;
        }
        else
        {
            if (!pEnc->LoadEncounter(pGff, &stElem))
            {
                delete pEnc;
                continue;
            }
            if (nFlags)
                pEnc->LoadObjectState(pGff, &stElem);
            pEnc->AddToArea(this, pEnc->m_position.x, pEnc->m_position.y, pEnc->m_position.z, !nFlags);
        }
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git item list: position first, then orientation, then registration.
int CSWSArea::LoadItems(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("List")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 0)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSItem* pItem = new CSWSItem(nObjectId);
        int bSuccessReads;   // separate flag for the template/position/orientation reads
        if (nUseTemplates)
        {
            CResRef refItem = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccessReads, CResRef(""));
            if (bSuccessReads && pItem->LoadFromTemplate(refItem))
                goto DoLoad;
        }
        else if (pItem->LoadItem(pGff, &stElem))
            goto DoLoad;
        delete pItem;
        continue;
    DoLoad:
        Vector v(0.0f, 0.0f, 0.0f);
        if (nFlags)
            static_cast<CSWSObject*>(pItem)->LoadObjectState(pGff, &stElem);
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccessReads, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccessReads, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccessReads, 0.0f);
        pItem->AddToArea(this, fX, fY, fZ, !nFlags);
        v.x = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XOrientation"), bSuccessReads, 0.0f);
        v.y = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YOrientation"), bSuccessReads, 0.0f);
        v.z = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZOrientation"), bSuccessReads, 0.0f);
        static_cast<CSWSObject*>(pItem)->SetOrientation(v);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git "Creature List". A creature is placed through ComputeSafeLocation when the
// walkmesh suggests a safer spot.
int CSWSArea::LoadCreatures(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("Creature List")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 4)
            continue;
        int bSuccess;
        CSWSCreature* pCreature;
        if (nUseTemplates)
        {
            pCreature = new CSWSCreature(OBJECT_INVALID, 0);
            CResRef refCreature = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccess, CResRef(""));
            if (bSuccess && pCreature->LoadFromTemplate(refCreature, 1))
                goto DoLoad;
        }
        else
        {
            unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
            pCreature = new CSWSCreature(nObjectId, 0);
            if (pCreature->LoadCreature(pGff, &stElem, nFlags))
                goto DoLoad;
        }
        delete pCreature;
        continue;
    DoLoad:
        Vector pos(0.0f, 0.0f, 0.0f);
        Vector out(0.0f, 0.0f, 0.0f);
        pos.x = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
        pos.y = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
        pos.z = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
        if (ComputeSafeLocation(pos, 20.0f, pCreature->m_pPathfindInfo, 1, &out))
            pos = out;
        pCreature->AddToArea(this, pos.x, pos.y, pos.z, nFlags, 0);
        float oX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XOrientation"), bSuccess, 0.0f);
        float oY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YOrientation"), bSuccess, 0.0f);
        float oZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZOrientation"), bSuccess, 0.0f);
        pCreature->SetOrientation(Vector(oX, oY, oZ));
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git "Door List": each door is external or placeable per LoadDoorExternal, and a
// trapped door is registered as a mine.
int CSWSArea::LoadDoors(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("Door List")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 8)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSDoor* pDoor = new CSWSDoor(nObjectId);
        if (!pDoor->LoadDoorExternal(pGff, &stElem, nUseTemplates))
        {
            delete pDoor;
            continue;
        }
        float bearing = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Bearing"), bSuccess, 0.0f);
        Quaternion q(Vector(0.0f, 0.0f, 1.0f), bearing);   // axis-angle about +Z by the bearing
        pDoor->m_bearing = bearing;
        pDoor->SetOrientation(q);
        if (nFlags)
            pDoor->LoadObjectState(pGff, &stElem);
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("X"), bSuccess, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Y"), bSuccess, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Z"), bSuccess, 0.0f);
        pDoor->AddToArea(this, fX, fY, fZ, !nFlags);
        if (pDoor->m_bTrapFlag)
            m_mineObjects.Add(pDoor->m_objectId);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git "Placeable List" with its tweak-color fields; a trapped placeable is
// registered as a mine.
int CSWSArea::LoadPlaceables(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("Placeable List")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 9)
            continue;
        int bSuccess;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
        CSWSPlaceable* pPlaceable = new CSWSPlaceable(nObjectId);
        if (nUseTemplates)
        {
            CResRef refPlaceable = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccess, CResRef(""));
            if (bSuccess && pPlaceable->LoadFromTemplate(refPlaceable, 1))
            {
#if !KOTOR1  // K1: no tweak colour
                pPlaceable->m_bUseTweakColor = pGff->ReadFieldBYTE(&stElem, const_cast<char*>("UseTweakColor"), bSuccess, 0);
                pPlaceable->m_tweakColor = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("TweakColor"), bSuccess, 0);
#endif
                goto DoLoad;
            }
        }
        else if (pPlaceable->LoadPlaceable(pGff, &stElem, 0))
            goto DoLoad;
        delete pPlaceable;
        continue;
    DoLoad:
        float bearing = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Bearing"), bSuccess, 0.0f);
        pPlaceable->m_bearing = bearing;
        Quaternion q(Vector(0.0f, 0.0f, 1.0f), bearing);
        pPlaceable->SetOrientation(q);
        if (nFlags)
            pPlaceable->LoadObjectState(pGff, &stElem);
        float fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("X"), bSuccess, 0.0f);
        float fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Y"), bSuccess, 0.0f);
        float fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Z"), bSuccess, 0.0f);
        Vector pos(fX, fY, fZ);
        pPlaceable->AddToArea(this, pos, nFlags);
        if (pPlaceable->m_bTrapFlag)
            m_mineObjects.Add(pPlaceable->m_objectId);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Loads the .git "TriggerList", including the transition state and geometry of templated
// triggers; a trapped trigger is registered as a mine.
int CSWSArea::LoadTriggers(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates)
{
    CResList list;
    if (!pGff->GetList(&list, pStruct, const_cast<char*>("TriggerList")))
        return 1;
    if (!pGff->GetListCount(&list))
        return 1;
    unsigned int i = 0;
    do
    {
        CResStruct stElem;
        if (!pGff->GetListElement(&stElem, &list, i))
            continue;
        if (pGff->GetElementType(&stElem) != 1)
            continue;
        int bSuccessId;
        unsigned long long nObjectId = pGff->ReadFieldDWORD(&stElem, const_cast<char*>("ObjectId"), bSuccessId, OBJECT_INVALID);
        CSWSTrigger* pTrigger = new CSWSTrigger(nObjectId);
        int bSuccess;
        float fX, fY, fZ;   // filled by either load path, read by the shared tail
        if (nUseTemplates)
        {
            CResRef refTrigger = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("TemplateResRef"), bSuccess, CResRef(""));
            if (bSuccess && pTrigger->LoadFromTemplate(refTrigger))
            {
                CExoString sLinkedToModule;
                CResList listGeom;
                CResRef refMod = pGff->ReadFieldCResRef(&stElem, const_cast<char*>("LinkedToModule"), bSuccess, CResRef(""));
                refMod.CopyToString(sLinkedToModule);
                {
                    CExoLocString result = pGff->ReadFieldCExoLocString(
                        &stElem, const_cast<char*>("TransitionDestination"), bSuccess, pTrigger->m_transitionDestination);
                    pTrigger->m_transitionDestination = result;
                }
                pTrigger->m_linkedTo = pGff->ReadFieldCExoString(
                    &stElem, const_cast<char*>("LinkedTo"), bSuccess, CExoString(""));
                pTrigger->m_linkedToModule = sLinkedToModule;
                pTrigger->m_linkedToFlags = static_cast<unsigned char>(pGff->ReadFieldBYTE(
                    &stElem, const_cast<char*>("LinkedToFlags"), bSuccess, 0));
                fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
                fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
                fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
                pTrigger->SetPosition(Vector(fX, fY, fZ), 1, 1, 0);
                if (pGff->GetList(&listGeom, &stElem, const_cast<char*>("Geometry")))
                    pTrigger->LoadTriggerGeometry(pGff, listGeom);
                goto DoLoad;
            }
        }
        else if (pTrigger->LoadTrigger(pGff, &stElem))
        {
            fX = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("XPosition"), bSuccess, 0.0f);
            fY = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("YPosition"), bSuccess, 0.0f);
            fZ = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
            goto DoLoad;
        }
        delete pTrigger;
        continue;
    DoLoad:
        if (nFlags)
            pTrigger->LoadObjectState(pGff, &stElem);
        pTrigger->AddToArea(this, fX, fY, fZ, !nFlags);
        if (pTrigger->m_bTrap)
            m_mineObjects.Add(pTrigger->m_objectId);
    } while (++i < pGff->GetListCount(&list));
    return 1;
}

// Hands the .git AreaMap blob and its resolutions to the module's area map.
int CSWSArea::LoadMaps(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct st;
    if (!pGff->GetStructFromStruct(&st, pStruct, const_cast<char*>("AreaMap")))
        return 0;
    int bSuccess;
    int nSuccess = 0;
    int nResX = pGff->ReadFieldINT(&st, const_cast<char*>("AreaMapResX"), bSuccess, 0);
    int nResY = pGff->ReadFieldINT(&st, const_cast<char*>("AreaMapResY"), bSuccess, 0);
    unsigned int nDataSize = pGff->ReadFieldDWORD(&st, const_cast<char*>("AreaMapDataSize"), bSuccess, 0);
    unsigned int* paData = new unsigned int[nDataSize];
    pGff->ReadFieldVOID(&st, paData, nDataSize, const_cast<char*>("AreaMapData"), bSuccess, 0);
    if (bSuccess)
    {
        nSuccess = 1;
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        if (pModule->m_pAreaMap)
            pModule->m_pAreaMap->LoadSavedAreaMapData(paData, nDataSize >> 2, nResX, nResY);
    }
    delete[] paData;
    return nSuccess;
}

// Loads the .git "CameraList" into the in-game GUI's placeable camera slots.
int CSWSArea::LoadPlaceableCameras(CResGFF* pGff, CResStruct* pStruct)
{
    CResList list;
    CResStruct stElem;
    int bSuccess = 1;
    Vector vDefaultPosition;
    Quaternion qDefaultOrientation;
    pGff->GetList(&list, pStruct, const_cast<char*>("CameraList"));
    int nCameras = pGff->GetListCount(&list);
#if KOTOR1  // K1 allows at most 50 placeable cameras, K2 70
    if (nCameras > 50)
#else
    if (nCameras > 70)
#endif
        return 0;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->InitializePlaceableCameras(nCameras);
    if (nCameras <= 0)
        return 1;
    for (int i = 0; i < nCameras; i++)
    {
        pGff->GetListElement(&stElem, &list, i);
        int nCameraID = pGff->ReadFieldINT(&stElem, const_cast<char*>("CameraID"), bSuccess, -1);
        Vector vPosition = pGff->ReadFieldVector(&stElem, const_cast<char*>("Position"), bSuccess, vDefaultPosition);
        Quaternion qOrientation = pGff->ReadFieldQuaternion(&stElem, const_cast<char*>("Orientation"), bSuccess, qDefaultOrientation);
        float fPitch = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Pitch"), bSuccess, 0.0f);
        float fHeight = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("Height"), bSuccess, 0.0f);
        float fFieldOfView = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("FieldOfView"), bSuccess, 55.0f);
        float fMicRange = pGff->ReadFieldFLOAT(&stElem, const_cast<char*>("MicRange"), bSuccess, 0.0f);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetPlaceableCamera(
            i, nCameraID, vPosition, qOrientation, fPitch, fHeight, fFieldOfView, fMicRange);
    }
    return 1;
}

// Reads the .git AreaProperties sub-struct and applies the flags it carries.
int CSWSArea::LoadProperties(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct st;
    int bSuccess;
    if (!pGff->GetStructFromStruct(&st, pStruct, const_cast<char*>("AreaProperties")))
        return 0;
    m_nUnescapable = pGff->ReadFieldBYTE(&st, const_cast<char*>("Unescapable"), bSuccess, static_cast<unsigned char>(m_nUnescapable));
    SetPlayerRestrictMode(m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("RestrictMode"), bSuccess, static_cast<unsigned char>(m_nPlayerRestrictMode)));
    unsigned int nMax = pGff->ReadFieldDWORD(&st, const_cast<char*>("StealthXPMax"), bSuccess, m_maxStealthXP);
    if (m_currentStealthXP > nMax)
        m_currentStealthXP = nMax;
    m_maxStealthXP = nMax;
    unsigned int nCurrent = pGff->ReadFieldDWORD(&st, const_cast<char*>("StealthXPCurrent"), bSuccess, nMax);
    m_currentStealthXP = (m_maxStealthXP < nCurrent) ? m_maxStealthXP : nCurrent;
    m_stealthXPDecrement = pGff->ReadFieldDWORD(&st, const_cast<char*>("StealthXPLoss"), bSuccess, m_stealthXPDecrement);
    m_bStealthXPEnabled = pGff->ReadFieldBYTE(&st, const_cast<char*>("StealthXPEnabled"), bSuccess, static_cast<unsigned char>(m_bStealthXPEnabled));
    m_bTransitionPending = pGff->ReadFieldBYTE(&st, const_cast<char*>("TransPending"), bSuccess, static_cast<unsigned char>(m_bTransitionPending));
    m_transitionPendingID = pGff->ReadFieldBYTE(&st, const_cast<char*>("TransPendNextID"), bSuccess, m_transitionPendingID);
    m_activeTransitionID = pGff->ReadFieldBYTE(&st, const_cast<char*>("TransPendCurrID"), bSuccess, m_activeTransitionID);
    m_nFogColor = pGff->ReadFieldDWORD(&st, const_cast<char*>("SunFogColor"), bSuccess, m_nFogColor);
    return m_pAmbientSound->Load(pGff, &st);
}

// Loads the area: the .are header, the layout, the per-room audio-table index, the .git
// objects and the path points.
int CSWSArea::LoadArea(int nFlags)
{
    if (!m_pRes->Demand())
        return 0;
    CResStruct st;
    m_pRes->GetTopLevelStruct(&st);
    LoadAreaHeader(&st);
    LoadRoomInfo(&st);
#if !KOTOR1  // K1 has no room-to-area-table index map
    if (m_nRoomCount > 0)
        for (int i = 0; i < m_nRoomCount; i++)
        {
            if (m_nRoomAudioCount <= 0)
                continue;
            CResRef cResRef = m_paRooms[i].m_resRoom;
            int nCount = 0;
            for (int j = 0; nCount < m_nRoomAudioCount; j++)
            {
                if (m_paRoomNames[j].CompareNoCase(cResRef.GetResRefStr()))
                {
                    m_paRoomTableIndex[i] = static_cast<unsigned char>(nCount);
                    break;
                }
                nCount++;
            }
        }
#endif
    LoadGIT(nFlags);
    LoadPathPoints(&st);
    m_pRes->Release();
    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sDebugName,
        m_objectId);
#if KOTOR1
    m_nTileBitmapBytes = static_cast<unsigned char>(__builtin_ceilf(static_cast<float>(m_nHeight * m_nWidth) * 0.125f));
#else
    m_nTileBitmapBytes = static_cast<unsigned char>(__builtin_ceilf(static_cast<float>(m_nWidth * m_nHeight) * 0.125f));
#endif
    return 1;
}

// Reads the .are header: the reserved id block, the expansion list, the script slots, the area
// fields, the room audio tables and the module map.
// EFFECTIVE: the orig materializes the 708/354 map scales once per product (4 mov+fmov), ours once.
int CSWSArea::LoadAreaHeader(CResStruct* pStruct)
{
    CResList list;
    CResStruct stElem;
    int bSuccess;
    float fMoonNear;
    float fMoonFar;
    float fSunNear;
    float fSunFar;
    unsigned int nStealthMax;
    unsigned int nStealthCur;
#if !KOTOR1
    int nFormOne;
    int nFormTwo;
    int nFormThree;
#endif

    // The reserved id block holds ID, creator and version.
    m_pHeaderIds->nID = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ID"), bSuccess, 0);
    m_pHeaderIds->nCreatorID = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("Creator_ID"), bSuccess, 0);
    m_pHeaderIds->nVersion = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("Version"), bSuccess, 0);

    m_sComments = m_pRes->ReadFieldCExoString(pStruct, const_cast<char*>("Comments"), bSuccess, CExoString(""));

    if (m_pRes->GetList(&list, pStruct, const_cast<char*>("Expansion_List")) == 1)
    {
        int nCount = m_pRes->GetListCount(&list);
        for (int i = 0; i < nCount; i++)
        {
            m_pRes->GetListElement(&stElem, &list, i);
            NWAreaExpansion_st* pExp = new NWAreaExpansion_st;
            pExp->m_name = m_pRes->ReadFieldCExoLocString(&stElem, const_cast<char*>("Expansion_Name"), bSuccess, CExoLocString());
            pExp->m_nID = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("Expansion_ID"), bSuccess, 0);
            m_expansionList.AddTail(pExp);
        }
    }

    // The four script slots are stored as CResRef, so each one is copied into a shared string
    // before being assigned to its member.
    CExoString scriptStr;
    m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("OnHeartbeat"), bSuccess, CResRef("")).CopyToString(scriptStr);
    m_scriptOnHeartbeat = scriptStr;
    m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("OnUserDefined"), bSuccess, CResRef("")).CopyToString(scriptStr);
    m_scriptOnUserDefined = scriptStr;
    m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("OnEnter"), bSuccess, CResRef("")).CopyToString(scriptStr);
    m_scriptOnEnter = scriptStr;
    m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("OnExit"), bSuccess, CResRef("")).CopyToString(scriptStr);
    m_scriptOnExit = scriptStr;

    m_areaName = m_pRes->ReadFieldCExoLocString(pStruct, const_cast<char*>("Name"), bSuccess, CExoLocString());

    SetTag(m_pRes->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, CExoString("")));
    m_nAreaFlags = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("Flags"), bSuccess, 0);

    m_cameraStyle = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("CameraStyle"), bSuccess, 0);
    m_defaultEnvMap = m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("DefaultEnvMap"), bSuccess, CResRef(""));
    m_bAreaHeaderLoaded = 1;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_followState == 1)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->SetPartyFollowState(0);

    m_nUnescapable = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("Unescapable"), bSuccess, static_cast<unsigned char>(m_nUnescapable));
#if !KOTOR1
    m_disableTransit = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("DisableTransit"), bSuccess, 0);
#endif
    SetPlayerRestrictMode(m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("RestrictMode"), bSuccess, static_cast<unsigned char>(m_nPlayerRestrictMode)));

    m_chanceRain = static_cast<unsigned char>(m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ChanceRain"), bSuccess, 0));
    m_chanceSnow = static_cast<unsigned char>(m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ChanceSnow"), bSuccess, 0));
    m_chanceFog = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ChanceFog"), bSuccess, 0);
    m_chanceLightning = static_cast<unsigned char>(m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ChanceLightning"), bSuccess, 0));
    m_windPower = static_cast<unsigned char>(m_pRes->ReadFieldINT(pStruct, const_cast<char*>("WindPower"), bSuccess, 0));
    if (m_nAreaFlags & 1)
    {
        m_chanceFog = 0;
        m_chanceRain = 0;
        m_chanceSnow = 0;
        m_chanceLightning = 0;
        m_windPower = 0;
    }

    m_moonAmbientColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("MoonAmbientColor"), bSuccess, 0);
    m_moonDiffuseColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("MoonDiffuseColor"), bSuccess, 0);
    m_moonFogColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("MoonFogColor"), bSuccess, 0);
    fMoonNear = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("MoonFogNear"), bSuccess, 10000.0f);
    m_moonFogNear = fMoonNear < 0.0f ? 0.0f : fMoonNear;
    fMoonFar = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("MoonFogFar"), bSuccess, 10000.0f);
    m_moonFogFar = fMoonFar < 0.0f ? 0.0f : fMoonFar;
    m_moonFogOn = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("MoonFogOn"), bSuccess, 0);
    m_moonShadows = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("MoonShadows"), bSuccess, 0);
    m_sunAmbientColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("SunAmbientColor"), bSuccess, 0);
    m_sunDiffuseColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("SunDiffuseColor"), bSuccess, 0);
    m_nFogColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("SunFogColor"), bSuccess, 0);
    fSunNear = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("SunFogNear"), bSuccess, 10000.0f);
    m_sunFogNear = fSunNear < 0.0f ? 0.0f : fSunNear;
    fSunFar = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("SunFogFar"), bSuccess, 10000.0f);
    m_sunFogFar = fSunFar < 0.0f ? 0.0f : fSunFar;
    m_sunFogOn = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("SunFogOn"), bSuccess, 0);
    m_sunShadows = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("SunShadows"), bSuccess, 0);
    m_dayNightCycle = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("DayNightCycle"), bSuccess, 0);
    m_isNight = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("IsNight"), bSuccess, 0);
    m_dynAmbientColor = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("DynAmbientColor"), bSuccess, 0);
    m_noRest = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("NoRest"), bSuccess, static_cast<unsigned char>(m_noRest));
    m_shadowOpacity = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("ShadowOpacity"), bSuccess, m_shadowOpacity);
    m_nLightingScheme = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("LightingScheme"), bSuccess, 0);
    m_modSpotCheck = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ModSpotCheck"), bSuccess, 0);
    m_nListenModifier = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("ModListenCheck"), bSuccess, 0);

    m_miniGame = m_pRes->GetStructFromStruct(&stElem, pStruct, const_cast<char*>("MiniGame"));
    m_nLoadScreenID = m_pRes->ReadFieldWORD(pStruct, const_cast<char*>("LoadScreenID"), bSuccess, 0);

    m_grassDiffuse = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("Grass_Diffuse"), bSuccess, 0);
    m_grassAmbient = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("Grass_Ambient"), bSuccess, 0);
#if !KOTOR1  // K1 has no grass emissive colour
    m_grassEmissive = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("Grass_Emissive"), bSuccess, 0);
#endif
    m_grassDensity = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_Density"), bSuccess, 0.0f);
    m_grassQuadSize = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_QuadSize"), bSuccess, 0.0f);
    m_grassTexName = m_pRes->ReadFieldCResRef(pStruct, const_cast<char*>("Grass_TexName"), bSuccess, CResRef(""));
    if (!m_grassTexName.IsValid())
        m_grassTexName = const_cast<char*>("grass");
    m_grassProbLL = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_Prob_LL"), bSuccess, 0.0f);
    m_grassProbLR = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_Prob_LR"), bSuccess, 0.0f);
    m_grassProbUL = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_Prob_UL"), bSuccess, 0.0f);
    m_grassProbUR = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("Grass_Prob_UR"), bSuccess, 0.0f);
    m_alphaTest = m_pRes->ReadFieldFLOAT(pStruct, const_cast<char*>("AlphaTest"), bSuccess, 0.2f);

    // -- Rooms --
    m_pRes->GetList(&list, pStruct, const_cast<char*>("Rooms"));
    m_nRoomAudioCount = m_pRes->GetListCount(&list);
    InitializeRoomAudioParams(m_nRoomAudioCount);
    if (m_nRoomAudioCount >= 1)
    {
        for (int i = 0; i < m_nRoomAudioCount; i++)
        {
            m_pRes->GetListElement(&stElem, &list, i);
            m_paRoomNames[i] = m_pRes->ReadFieldCExoString(&stElem, const_cast<char*>("RoomName"), bSuccess, CExoString(""));
            m_paRoomEnvAudio[i] = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("EnvAudio"), bSuccess, 0);
            m_paRoomAmbientScale[i] = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("AmbientScale"), bSuccess, 0.0f);
#if !KOTOR1
            m_paForceRating[i] = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("ForceRating"), bSuccess, 0);
            m_paRoomDisableWeather[i] = m_pRes->ReadFieldBYTE(&stElem, const_cast<char*>("DisableWeather"), bSuccess, 0);
#endif
            CResList partsList;
            if (m_pRes->GetList(&partsList, &stElem, const_cast<char*>("PartSounds")))
            {
                int nParts = m_pRes->GetListCount(&partsList);
                m_nPartSoundCount += nParts;
                for (int j = 0; j < nParts; j++)
                {
                    CResStruct stPart;
                    int nLoop;
                    m_pRes->GetListElement(&stPart, &partsList, j);
                    m_lstPartSoundRoomNames.Add(m_paRoomNames[i]);
                    nLoop = m_pRes->ReadFieldBYTE(&stPart, const_cast<char*>("Looping"), bSuccess, 0);
                    m_lstPartSoundLooping.Add(nLoop);
                    m_lstPartSoundModelParts.Add(m_pRes->ReadFieldCExoString(&stPart, const_cast<char*>("ModelPart"), bSuccess, CExoString("")));
                    m_lstPartSoundOmenEvents.Add(m_pRes->ReadFieldCExoString(&stPart, const_cast<char*>("OmenEvent"), bSuccess, CExoString("")));
                    m_lstPartSoundResRefs.Add(m_pRes->ReadFieldCResRef(&stPart, const_cast<char*>("Sound"), bSuccess, CResRef("")));
                }
            }
        }
    }

    // -- Module map --
    CResRef moduleRef = g_pAppManager->m_pServerExoApp->GetModule()->m_resRef;
    CExoString mapStr1;
    CExoString mapStr2;
    moduleRef.CopyToString(mapStr1);
    mapStr2.Format(const_cast<char*>("lbl_map%s"), mapStr1.CStr());
    if (g_pExoResMan->Exists(CResRef(mapStr2), RESTYPE_TGA, static_cast<unsigned int*>(0)) ||
        g_pExoResMan->Exists(CResRef(mapStr2), RESTYPE_TPC, static_cast<unsigned int*>(0)))
    {
        if (m_pRes->GetStructFromStruct(&stElem, pStruct, const_cast<char*>("Map")))
        {
            int nMapResX = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapResX"), bSuccess, 0);
            if (nMapResX != 0)
            {
                int nNorthAxis = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("NorthAxis"), bSuccess, 0);
                int nMapZoom = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapZoom"), bSuccess, 1);
                int nMapPt1X, nMapPt1Y, nMapPt2X, nMapPt2Y;
                if (m_pRes->GetFieldType(&stElem, const_cast<char*>("MapPt1X"), static_cast<unsigned int>(-1)) == 8)
                {
                    float f1x;
                    float f1y;
                    float f2x;
                    float f2y;
                    f1x = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("MapPt1X"), bSuccess, 0.0f);
                    f1y = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("MapPt1Y"), bSuccess, 0.0f);
                    f2x = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("MapPt2X"), bSuccess, 0.0f);
                    f2y = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("MapPt2Y"), bSuccess, 0.0f);
#if KOTOR1  // K1's map image is 440 x 256, K2's 708 x 354
                    nMapPt1X = static_cast<int>(__builtin_floorf(f1x * 440.0f + 0.5f));
                    nMapPt1Y = static_cast<int>(__builtin_floorf(f1y * 256.0f + 0.5f));
                    nMapPt2X = static_cast<int>(__builtin_floorf(f2x * 440.0f + 0.5f));
                    nMapPt2Y = static_cast<int>(__builtin_floorf(f2y * 256.0f + 0.5f));
#else
                    nMapPt1X = static_cast<int>(__builtin_floorf(f1x * 708.0f + 0.5f));
                    nMapPt1Y = static_cast<int>(__builtin_floorf(f1y * 354.0f + 0.5f));
                    nMapPt2X = static_cast<int>(__builtin_floorf(f2x * 708.0f + 0.5f));
                    nMapPt2Y = static_cast<int>(__builtin_floorf(f2y * 354.0f + 0.5f));
#endif
                }
                else
                {
                    nMapPt1X = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapPt1X"), bSuccess, 0);
                    nMapPt1Y = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapPt1Y"), bSuccess, 0);
                    nMapPt2X = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapPt2X"), bSuccess, 0);
                    nMapPt2Y = m_pRes->ReadFieldINT(&stElem, const_cast<char*>("MapPt2Y"), bSuccess, 0);
                }
                float fW1X = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("WorldPt1X"), bSuccess, 0.0f);
                float fW1Y = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("WorldPt1Y"), bSuccess, 0.0f);
                float fW2X = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("WorldPt2X"), bSuccess, 0.0f);
                float fW2Y = m_pRes->ReadFieldFLOAT(&stElem, const_cast<char*>("WorldPt2Y"), bSuccess, 0.0f);
                g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap->Initialize(1, nMapResX, nNorthAxis,
                    fW1X, fW1Y, fW2X, fW2Y, nMapPt1X, nMapPt1Y, nMapPt2X, nMapPt2Y, nMapZoom);
            }
            else
            {
                g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap->Initialize(0, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
            }
        }
        else
        {
            g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap->Initialize(0, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
        }
    }
    else
    {
        g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap->Initialize(0, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
    }

    // -- Stealth XP --
    nStealthMax = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("StealthXPMax"), bSuccess, m_maxStealthXP);
    if (m_currentStealthXP > nStealthMax)
        m_currentStealthXP = nStealthMax;
    m_maxStealthXP = nStealthMax;
    nStealthCur = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("StealthXPCurrent"), bSuccess, nStealthMax);
    m_currentStealthXP = m_maxStealthXP < nStealthCur ? m_maxStealthXP : nStealthCur;
    m_stealthXPDecrement = m_pRes->ReadFieldDWORD(pStruct, const_cast<char*>("StealthXPLoss"), bSuccess, m_stealthXPDecrement);
    m_bStealthXPEnabled = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("StealthXPEnabled"), bSuccess, static_cast<unsigned char>(m_bStealthXPEnabled));

#if !KOTOR1
    // -- per-transition dirty arrays --
    m_dirtyARGB[0] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyARGBOne"), bSuccess, 0);
    m_dirtySize[0] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtySizeOne"), bSuccess, 16);
    nFormOne = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFormulaOne"), bSuccess, 1);
    switch (nFormOne)
    {
    case 1:
        m_dirtyFormula[0] = 0;
        break;
    case 0:
        m_dirtyFormula[0] = 1;
        break;
    default:
        m_dirtyFormula[0] = nFormOne;
        break;
    }
    m_dirtyFunction[0] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFuncOne"), bSuccess, 0);
    m_dirtyARGB[1] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyARGBTwo"), bSuccess, 0);
    m_dirtySize[1] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtySizeTwo"), bSuccess, 16);
    nFormTwo = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFormulaTwo"), bSuccess, 1);
    switch (nFormTwo)
    {
    case 1:
        m_dirtyFormula[1] = 0;
        break;
    case 0:
        m_dirtyFormula[1] = 1;
        break;
    default:
        m_dirtyFormula[1] = nFormTwo;
        break;
    }
    m_dirtyFunction[1] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFuncTwo"), bSuccess, 0);
    m_dirtyARGB[2] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyARGBThree"), bSuccess, 0);
    m_dirtySize[2] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtySizeThree"), bSuccess, 16);
    nFormThree = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFormulaThree"), bSuccess, 1);
    switch (nFormThree)
    {
    case 1:
        m_dirtyFormula[2] = 0;
        break;
    case 0:
        m_dirtyFormula[2] = 1;
        break;
    default:
        m_dirtyFormula[2] = nFormThree;
        break;
    }
    m_dirtyFunction[2] = m_pRes->ReadFieldINT(pStruct, const_cast<char*>("DirtyFuncThree"), bSuccess, 0);
#endif

    // -- transition-pending state --
    m_bTransitionPending = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("TransPending"), bSuccess, static_cast<unsigned char>(m_bTransitionPending));
    m_transitionPendingID = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("TransPendNextID"), bSuccess, m_transitionPendingID);
    m_activeTransitionID = m_pRes->ReadFieldBYTE(pStruct, const_cast<char*>("TransPendCurrID"), bSuccess, m_activeTransitionID);

    return 1;
}

// The .git writer is SaveGIT and the per-section leaf savers below. Each leaf saver iterates the
// list it is handed; only the guard-bearing savers read members of the object they write.

void CSWSArea::SaveCreatures(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    g_pAppManager->m_pServerExoApp->GetPartyTable();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("Creature List"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSCreature* pCreature = pObj->AsSWSCreature();
#if KOTOR1  // K1: no puppets
                if (pCreature->m_bInParty == 0)
#else
                if (pCreature->m_bInParty == 0 && pCreature->m_isPuppet == 0)
#endif
                {
                    pGff->AddListElement(&st, &list, 4);
                    pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pCreature->m_objectId), const_cast<char*>("ObjectId"));
                    pCreature->SaveCreature(pGff, &st);
                }
            }
        }
    }
}

// CSWSItem keeps its identity field on the CSWSObject base, so SaveObjectState is reached
// through the inherited member call rather than a cast.
void CSWSArea::SaveItems(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("List"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSItem* pItem = pObj->AsSWSItem();
                pGff->AddListElement(&st, &list, 0);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pItem->m_objectId), const_cast<char*>("ObjectId"));
                pItem->SaveItem(pGff, &st);
                pItem->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveDoors(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("Door List"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSDoor* pDoor = pObj->AsSWSDoor();
                pGff->AddListElement(&st, &list, 8);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pDoor->m_objectId), const_cast<char*>("ObjectId"));
                pDoor->SaveDoor(pGff, &st);
                pDoor->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveTriggers(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("TriggerList"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSTrigger* pTrigger = pObj->AsSWSTrigger();
                pGff->AddListElement(&st, &list, 1);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pTrigger->m_objectId), const_cast<char*>("ObjectId"));
                pTrigger->SaveTrigger(pGff, &st);
                pTrigger->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveEncounters(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("Encounter List"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSEncounter* pEncounter = pObj->AsSWSEncounter();
                pGff->AddListElement(&st, &list, 7);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pEncounter->m_objectId), const_cast<char*>("ObjectId"));
                pEncounter->SaveEncounter(pGff, &st);
                pEncounter->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveWaypoints(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("WaypointList"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSWaypoint* pWaypoint = pObj->AsSWSWaypoint();
                pGff->AddListElement(&st, &list, 5);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pWaypoint->m_objectId), const_cast<char*>("ObjectId"));
                pWaypoint->SaveWaypoint(pGff, &st);
                pWaypoint->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveSounds(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("SoundList"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSSoundObject* pSound = pObj->AsSWSSoundObject();
                pGff->AddListElement(&st, &list, 6);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pSound->m_objectId), const_cast<char*>("ObjectId"));
                pSound->Save(pGff, &st);
                pSound->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SavePlaceables(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("Placeable List"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSPlaceable* pPlaceable = pObj->AsSWSPlaceable();
                if (pPlaceable->m_corpse.m_bIsCorpse == 0)
                {
                    pGff->AddListElement(&st, &list, 9);
                    pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pPlaceable->m_objectId), const_cast<char*>("ObjectId"));
                    pPlaceable->SavePlaceable(pGff, &st);
                    pPlaceable->SaveObjectState(pGff, &st);
                }
            }
        }
    }
}

void CSWSArea::SaveStores(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("StoreList"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSStore* pStore = pObj->AsSWSStore();
                pGff->AddListElement(&st, &list, GIT_STRUCT_ID_STORE);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pStore->m_objectId), const_cast<char*>("ObjectId"));
                pStore->SaveStore(pGff, &st);
                pStore->SaveObjectState(pGff, &st);
            }
        }
    }
}

void CSWSArea::SaveAreaEffects(CResGFF* pGff, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("AreaEffectList"));
    if (aList.m_count >= 1)
    {
        for (int n = 0; n < aList.m_count; n++)
        {
            CResStruct st;
            CGameObject* pObj;
            if (pObjectArray->GetGameObject(aList.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
            {
                CSWSAreaOfEffectObject* pAreaEffect = pObj->AsSWSAreaOfEffectObject();
                pGff->AddListElement(&st, &list, GIT_STRUCT_ID_AREA_OF_EFFECT);
                pGff->WriteFieldDWORD(&st, static_cast<unsigned int>(pAreaEffect->m_objectId), const_cast<char*>("ObjectId"));
                pAreaEffect->SaveEffect(pGff, &st);
                pAreaEffect->SaveObjectState(pGff, &st);
            }
        }
    }
}

// Writes the AreaProperties sub-struct: ambient sound, then the restrict/stealth/transition
// flags, then the fog color.
void CSWSArea::SaveProperties(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct st;
    pGff->AddStructToStruct(&st, pStruct, const_cast<char*>("AreaProperties"), GIT_STRUCT_ID_AREA_PROPERTIES);
    m_pAmbientSound->Save(pGff, &st);
    pGff->WriteFieldBYTE(&st, static_cast<unsigned char>(m_nUnescapable), const_cast<char*>("Unescapable"));
#if !KOTOR1
    pGff->WriteFieldBYTE(&st, static_cast<unsigned char>(m_disableTransit), const_cast<char*>("DisableTransit"));
#endif
    pGff->WriteFieldBYTE(&st, static_cast<unsigned char>(m_nPlayerRestrictMode), const_cast<char*>("RestrictMode"));
    pGff->WriteFieldDWORD(&st, m_maxStealthXP, const_cast<char*>("StealthXPMax"));
    pGff->WriteFieldDWORD(&st, m_currentStealthXP, const_cast<char*>("StealthXPCurrent"));
    pGff->WriteFieldDWORD(&st, m_stealthXPDecrement, const_cast<char*>("StealthXPLoss"));
    pGff->WriteFieldBYTE(&st, static_cast<unsigned char>(m_bStealthXPEnabled), const_cast<char*>("StealthXPEnabled"));
    pGff->WriteFieldBYTE(&st, static_cast<unsigned char>(m_bTransitionPending), const_cast<char*>("TransPending"));
    pGff->WriteFieldBYTE(&st, m_transitionPendingID, const_cast<char*>("TransPendNextID"));
    pGff->WriteFieldBYTE(&st, m_activeTransitionID, const_cast<char*>("TransPendCurrID"));
    pGff->WriteFieldDWORD(&st, m_nFogColor, const_cast<char*>("SunFogColor"));
}

// Serializes the module's saved-data map into the AreaMap sub-struct.
void CSWSArea::SaveMaps(CResGFF* pGff, CResStruct* pStruct)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0 && pModule->m_pAreaMap != 0 && pModule->m_pAreaMap->m_nSize != 0)
    {
        unsigned int nDataSizeDwords = 0;
        int nResX;   // written by GetMapData before any use
        int nResY = 0;
        void* pData = pModule->m_pAreaMap->GetMapData(nDataSizeDwords, nResX, nResY);
        CResStruct st;
        pGff->AddStructToStruct(&st, pStruct, const_cast<char*>("AreaMap"), GIT_STRUCT_ID_AREA_MAP);
        pGff->WriteFieldINT(&st, nResX, const_cast<char*>("AreaMapResX"));
        pGff->WriteFieldINT(&st, nResY, const_cast<char*>("AreaMapResY"));
        unsigned int nBytes = nDataSizeDwords << 2;
        pGff->WriteFieldDWORD(&st, nBytes, const_cast<char*>("AreaMapDataSize"));
        pGff->WriteFieldVOID(&st, pData, nBytes, const_cast<char*>("AreaMapData"));
    }
}

// Writes the GUI camera list with its four lens parameters.
void CSWSArea::SavePlaceableCameras(CResGFF* pGff, CResStruct* pStruct)
{
    CGuiInGame* pInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("CameraList"));
    if (pInGame->m_placeableCameraCount >= 1)
    {
        for (int n = 0; n < pInGame->m_placeableCameraCount; n++)
        {
            CGuiPlaceableCamera* pCamera = static_cast<CGuiPlaceableCamera*>(pInGame->GetPlaceableCamera(n));
            if (pCamera != 0)
            {
                CResStruct st;
                pGff->AddListElement(&st, &list, GIT_STRUCT_ID_CAMERA);
                pGff->WriteFieldINT(&st, pCamera->m_charId, const_cast<char*>("CameraID"));
                pGff->WriteFieldVector(&st, pCamera->m_position, const_cast<char*>("Position"));
                pGff->WriteFieldQuaternion(&st, pCamera->m_rotation, const_cast<char*>("Orientation"));
                pGff->WriteFieldFLOAT(&st, pCamera->m_a, const_cast<char*>("Pitch"));
                pGff->WriteFieldFLOAT(&st, pCamera->m_b, const_cast<char*>("Height"));
                pGff->WriteFieldFLOAT(&st, pCamera->m_c, const_cast<char*>("FieldOfView"));
                pGff->WriteFieldFLOAT(&st, pCamera->m_d, const_cast<char*>("MicRange"));
            }
        }
    }
}

// Writes the .git: partitions the area's objects into per-type lists and runs the per-section
// savers.
void CSWSArea::SaveGIT(CERFFile* pERF, const CExoString& sFileName, CExoArrayList<unsigned long long>& aSectionLists)
{
    K2_UNUSED(sFileName);
    CExoString sScratch;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CExoArrayList<unsigned long long> aCreatures, aItems, aDoors, aTriggers, aEncounters,
                                      aWaypoints, aSounds, aPlaceables, aStores, aAreaEffects;
    CResStruct* pGffRoot;
    CResGFF* pGff = new CResGFF;
    pGffRoot = new CResStruct();
    {
        CExoString sType("GIT ");
        CExoString sVersion("V2.0");
        pGff->CreateGFFFile(pGffRoot, sType, sVersion);
    }

    for (int n = 0; n < m_aObjectsByX.m_count; n++)
    {
        CGameObject* pObj;
        if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[n], &pObj) == CGameObjectArray::SUCCESS)
        {
            if (pObj->AsSWSCreature() != 0)
            {
                if (pObj->AsSWSCreature()->m_pStats->m_bIsPC == 0)
                    aCreatures.Add(m_aObjectsByX.m_pArray[n]);
                else
                    aSectionLists.Add(m_aObjectsByX.m_pArray[n]);
            }
            else if (pObj->AsSWSItem() != 0)
                aItems.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSDoor() != 0)
                aDoors.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSTrigger() != 0)
                aTriggers.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSEncounter() != 0)
                aEncounters.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSWaypoint() != 0)
                aWaypoints.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSSoundObject() != 0)
                aSounds.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSPlaceable() != 0)
                aPlaceables.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSStore() != 0)
                aStores.Add(m_aObjectsByX.m_pArray[n]);
            else if (pObj->AsSWSAreaOfEffectObject() != 0)
                aAreaEffects.Add(m_aObjectsByX.m_pArray[n]);
        }
    }

    m_scriptVars.SaveVarTable(pGff, pGffRoot);
    m_varTable.SaveVarTable(pGff, pGffRoot);
    pGff->WriteFieldBYTE(pGffRoot, m_nCurrentWeather, const_cast<char*>("CurrentWeather"));
    pGff->WriteFieldBYTE(pGffRoot, static_cast<unsigned char>(m_bWeatherOn), const_cast<char*>("WeatherStarted"));
    pGff->WriteFieldBYTE(pGffRoot, static_cast<unsigned char>(m_bTransitionPending), const_cast<char*>("TransPending"));
    pGff->WriteFieldBYTE(pGffRoot, m_transitionPendingID, const_cast<char*>("TransPendNextID"));
    pGff->WriteFieldBYTE(pGffRoot, m_activeTransitionID, const_cast<char*>("TransPendCurrID"));

    SaveCreatures(pGff, pGffRoot, aCreatures);
    SaveItems(pGff, pGffRoot, aItems);
    SaveDoors(pGff, pGffRoot, aDoors);
    SaveTriggers(pGff, pGffRoot, aTriggers);
    SaveEncounters(pGff, pGffRoot, aEncounters);
    SaveWaypoints(pGff, pGffRoot, aWaypoints);
    SaveSounds(pGff, pGffRoot, aSounds);
    SavePlaceables(pGff, pGffRoot, aPlaceables);
    SaveStores(pGff, pGffRoot, aStores);
    SaveAreaEffects(pGff, pGffRoot, aAreaEffects);
    SaveProperties(pGff, pGffRoot);
    SaveMaps(pGff, pGffRoot);
    SavePlaceableCameras(pGff, pGffRoot);

    CExoString sName;
    this->m_resRef.CopyToString(sName);
#if KOTOR1  // K1 passes null as the last argument
    pERF->WriteResource(static_cast<char*>(sName.CStr()), RESTYPE_GIT, pGff, 1, 0);
#else
#if KOTOR1
    pERF->WriteResource(static_cast<char*>(sName.CStr()), RESTYPE_GIT, pGff, 1, 0);
#else
    pERF->WriteResource(static_cast<char*>(sName.CStr()), RESTYPE_GIT, pGff, 1, reinterpret_cast<void*>(-1));
#endif
#endif

    delete pGff;
    delete pGffRoot;
}
int CSWSArea::GetIsIDInExcludeList(unsigned long long nObjectId,
                                   CExoArrayList<CGameObject*>* pExcludeList)
{
    if (pExcludeList)
    {
        int nCount = pExcludeList->m_count;

        for (int i = 0; i < nCount; i++)
        {
            if (pExcludeList->m_pArray[i]->m_objectId == nObjectId)
                return 1;
        }
    }

    return 0;
}

int CSWSArea::NoNonWalkPolys(const Vector& a, const Vector& b, float f1, float f2, float f3,
                             unsigned long long nIgnoreId, CWalkHitInfo* pHit, int nFlag)
{
    unsigned long long nHitId;

    return NoNonWalkPolysDetailed(a, b, f1, f2, f3, nIgnoreId, nHitId, 0, 0, 0, pHit, nFlag);
}

// The clearpathpoints console command removes the walk-square overlay drawn by testpathpoints.
const char* ClearPathPoints(char* pParams)
{
    K2_UNUSED(pParams);
    AurRemoveAllDrawableSquares();

    return "SUSSESS";
}

// The testpathpoints console command runs the path-point self-test and returns the reply string
// (the shipped spelling is misspelt).
const char* TestPathPoints(char* pParams)
{
    if (strlen(pParams) == 0)
        return "USAGE: <Personal Space>";

    g_pAppManager->m_pServerExoApp->GetModule()->TestPathPoint(pParams);

    return "SUSSESS";
}

// Reallocates the per-room audio and table arrays for nRoomCount rooms. m_paRoomTableIndex is
// allocated here but released by the destructor.
void CSWSArea::InitializeRoomAudioParams(int nRoomCount)
{
    if (m_paRoomNames)
    {
        delete[] m_paRoomNames;
        m_paRoomNames = 0;
    }

    if (m_paRoomEnvAudio)
    {
        delete[] m_paRoomEnvAudio;
        m_paRoomEnvAudio = 0;
    }

    if (m_paRoomAmbientScale)
    {
        delete[] m_paRoomAmbientScale;
        m_paRoomAmbientScale = 0;
    }

#if !KOTOR1  // K1 has only the name, EnvAudio and AmbientScale tables
    if (m_paForceRating)
    {
        delete[] m_paForceRating;
        m_paForceRating = 0;
    }

    if (m_paRoomDisableWeather)
    {
        delete[] m_paRoomDisableWeather;
        m_paRoomDisableWeather = 0;
    }
#endif

    if (nRoomCount)
    {
        m_paRoomNames = new CExoString[nRoomCount];
        m_paRoomEnvAudio = new int[nRoomCount];
        m_paRoomAmbientScale = new float[nRoomCount];
#if !KOTOR1
        m_paForceRating = new int[nRoomCount];
        m_paRoomDisableWeather = new unsigned char[nRoomCount];
        m_paRoomTableIndex = new unsigned char[nRoomCount];
#endif
    }
}

// Flags creatures whose head appearance collides with a player character's, so the renderer
// swaps in their backup head.
void CSWSArea::EnforceNoPCHeadCloning(int nHeadId)
{
    for (int i = 0; i < m_aObjectsByX.m_count; i++)
    {
        unsigned long long oidObject = m_aObjectsByX.m_pArray[i];
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);

        if (pCreature)
        {
            C2DA* pAppearance = g_pRules->m_p2DArrays->m_pAppearance;
            int nHead;

            if (pCreature->m_bDisguised)
            {
                unsigned short nAppearance = pCreature->m_nDisguiseAppearance;
                CExoString sColumn("NORMALHEAD");
                pAppearance->GetINTEntry(nAppearance, sColumn, &nHead);
            }
            else
            {
                unsigned short nAppearance = pCreature->m_appearance.m_headId;
                CExoString sColumn("NORMALHEAD");
                pAppearance->GetINTEntry(nAppearance, sColumn, &nHead);
            }

            if (nHead == nHeadId)
                pCreature->m_appearance.m_bUseBackupHead = 1;
        }
    }
}

// The id of the first door whose box the segment crosses and whose facing plane the endpoints
// straddle, or OBJECT_INVALID.
unsigned long long CSWSArea::GetDoorCrossed(const Vector& from, const Vector& to)
{
    Vector vFrom = from;
    Vector vTo = to;

    vFrom.z += 0.5f;
    vTo.z += 0.5f;

    for (int i = 0; i < m_nStaticBBoxCount; i++)
    {
        const CSWSAreaDoorBox& box = reinterpret_cast<const CSWSAreaDoorBox*>(m_pStaticBBoxes)[i];
        if (BoxIntersect(vFrom, vTo, box.m_vMin, box.m_vMax))
        {
            unsigned long long oidDoor = m_pStaticBBoxIds[i];
            CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidDoor);

            if (pDoor)
            {
                Vector vDoorPos = pDoor->m_position;
                float fFrom = dot(vFrom - vDoorPos, pDoor->CSWSObject::m_orientation);
                float fTo = dot(vTo - vDoorPos, pDoor->CSWSObject::m_orientation);

                if ((fFrom > 0.0f) != (fTo > 0.0f))
                    return oidDoor;
            }
        }
    }

    return OBJECT_INVALID;
}

// The area's weather tick, then the OnHeartbeat gate. New weather is only rolled for an
// exterior area whose weather override is unset.
int CSWSArea::AIUpdate()
{
    unsigned int day, tod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&day, &tod);

    unsigned int elapsedDay, elapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_nWeatherDay, m_nWeatherTod, &elapsedDay, &elapsedTod);
    m_nWeatherDay = day;
    m_nWeatherTod = tod;

    if (m_nWeatherOverride == WEATHER_NONE && (m_nAreaFlags & 1) == 0)
    {
        if (m_nWeatherTimer != 0)
        {
            if (m_nWeatherTimer <= elapsedTod)
                m_nWeatherTimer = 0;
            else
                m_nWeatherTimer -= elapsedTod;
        }
        else
        {
#if KOTOR1  // K1 uses libc rand() and rolls only rain and snow
            int nRoll = rand() % 100;
#else
            int nRoll = rand_wincompatible() % 100;
#endif

            if (m_chanceRain != 0)
            {
                if (nRoll < m_chanceRain)
                {
                    if (m_bWeatherOn == 0)
                    {
                        m_nCurrentWeather = CSWCArea::WEATHER_RAIN;
                        m_bWeatherOn = 1;
                        SetCurrentWeather(CSWCArea::WEATHER_RAIN, 1);
                    }
                }
                else if (m_bWeatherOn != 0)
                {
                    m_nCurrentWeather = CSWCArea::WEATHER_RAIN;
                    m_bWeatherOn = 0;
                    SetCurrentWeather(CSWCArea::WEATHER_RAIN, 0);
                }
            }
            else if (m_chanceSnow != 0)
            {
                if (nRoll < m_chanceSnow)
                {
                    if (m_bWeatherOn == 0)
                    {
                        m_nCurrentWeather = CSWCArea::WEATHER_SNOW;
                        m_bWeatherOn = 1;
                        SetCurrentWeather(CSWCArea::WEATHER_SNOW, 1);
                    }
                }
                else if (m_bWeatherOn != 0)
                {
                    m_nCurrentWeather = CSWCArea::WEATHER_SNOW;
                    m_bWeatherOn = 0;
                    SetCurrentWeather(CSWCArea::WEATHER_SNOW, 0);
                }
            }
#if !KOTOR1
            else if (m_chanceLightning != 0)
            {
                if (nRoll < m_chanceLightning)
                {
                    if (m_bWeatherOn == 0)
                    {
                        m_nCurrentWeather = CSWCArea::WEATHER_RAIN;
                        m_bWeatherOn = 1;
                        SetCurrentWeather(CSWCArea::WEATHER_RAIN, 1);
                    }
                }
                else if (m_bWeatherOn != 0)
                {
                    m_nCurrentWeather = CSWCArea::WEATHER_RAIN;
                    m_bWeatherOn = 0;
                    SetCurrentWeather(CSWCArea::WEATHER_RAIN, 0);
                }
            }
#endif

            m_nWeatherTimer = 300000;
        }
    }

    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_lastUpdateDay, m_lastUpdateTod, &elapsedDay, &elapsedTod);

    if ((elapsedTod >> 4) >= CSWSObject::HEARTBEAT_INTERVAL_16MS)
    {
        if (m_lastUpdateTod != 0)
            g_pVirtualMachine->RunScript(&m_scriptOnHeartbeat, m_objectId, 1);

        m_lastUpdateDay = day;
        m_lastUpdateTod = tod;

        return 1;
    }

    return 0;
}

// Keeps the X-sorted object index ordered after one object moves.
int CSWSArea::UpdatePositionInObjectsArray(CGameObject* pObject)
{
    int nIndex = m_aObjectsByX.IndexOf(pObject->m_objectId);
    int nCount = m_aObjectsByX.m_count;

#ifdef K2_FIX_ENGINE_BUGS
    // An object missing from the index would otherwise make the swap below read the word before
    // the array, so report it and bail out instead.
    if (nIndex == -1) {
        k2_fix_object_not_indexed(pObject->m_objectId, pObject->m_objectType,
                                  static_cast<CSWSObject*>(pObject)->m_position.x,
                                  static_cast<CSWSObject*>(pObject)->m_position.y);
        return 1;
    }
#endif

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pOther = pObject;
    float fX = static_cast<CSWSObject*>(pObject)->m_position.x;
    int i = nIndex;

    // Bubble right while the next object sits further left; otherwise fall through to the
    // bubble-left check below.
    if (nIndex + 1 != nCount)
    {
        if (!(pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i + 1], &pOther)
                      == CGameObjectArray::SUCCESS
              && static_cast<CSWSObject*>(pOther)->m_position.x >= fX))
        {
            do
            {
                if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i + 1], &pOther)
                        != CGameObjectArray::SUCCESS)
                    break;

                if (!(static_cast<CSWSObject*>(pOther)->m_position.x < fX))
                    break;

                unsigned long long oidSwap = m_aObjectsByX.m_pArray[i];
                m_aObjectsByX.m_pArray[i] = m_aObjectsByX.m_pArray[i + 1];
                m_aObjectsByX.m_pArray[i + 1] = oidSwap;
                i++;
            }
            while (i + 1 != nCount);
            return 1;
        }
    }

    if (i != 0)
    {
        if (!(pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i - 1], &pOther)
                      == CGameObjectArray::SUCCESS
              && static_cast<CSWSObject*>(pOther)->m_position.x <= fX))
        {
            do
            {
                if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i - 1], &pOther)
                        != CGameObjectArray::SUCCESS)
                    break;

                if (!(static_cast<CSWSObject*>(pOther)->m_position.x > fX))
                    break;

                unsigned long long oidSwap = m_aObjectsByX.m_pArray[i - 1];
                m_aObjectsByX.m_pArray[i - 1] = m_aObjectsByX.m_pArray[i];
                m_aObjectsByX.m_pArray[i] = oidSwap;
                i--;
            }
            while (i != 0);
        }
    }

    return 1;
}

// Broadcasts a weather change to every player in the area. A nWeather of 255 asks the area to
// derive the change from its current weather.
void CSWSArea::SetCurrentWeather(unsigned char nWeather, int bTurnOn)
{
    if (nWeather == WEATHER_NONE)
    {
        unsigned char nSent = m_nSentWeather;
        unsigned char nCurrent = m_nCurrentWeather;

        if (nSent == nCurrent)
            return;

        nWeather = nSent;

        if (m_nAreaFlags & 1)
        {
            m_nCurrentWeather = WEATHER_NONE;

            if (nSent == CSWCArea::WEATHER_RAIN || nSent == CSWCArea::WEATHER_SNOW)
                bTurnOn = 0;
            else
                goto unknown;
        }
        else if (nSent == CSWCArea::WEATHER_RAIN)
        {
            bTurnOn = (nCurrent == CSWCArea::WEATHER_SNOW);
            nWeather = (nCurrent == CSWCArea::WEATHER_SNOW) ? CSWCArea::WEATHER_SNOW : CSWCArea::WEATHER_RAIN;
        }
        else if (nSent == CSWCArea::WEATHER_SNOW)
        {
            bTurnOn = (nCurrent == CSWCArea::WEATHER_RAIN);
            nWeather = (nCurrent == CSWCArea::WEATHER_RAIN) ? CSWCArea::WEATHER_RAIN : CSWCArea::WEATHER_SNOW;
        }
        else
        {
        unknown:
            nWeather = WEATHER_NONE;
        }
    }

    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();

    if (pList != 0)
    {
        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        CSWSPlayer* pPlayer = (pList->m_pInternal->m_pHead != 0)
                                  ? static_cast<CSWSPlayer*>(pList->m_pInternal->m_pHead->m_pData)
                                  : 0;

        while (pNode != 0)
        {
            if (pPlayer != 0)
            {
                CSWSObject* pObject = pPlayer->GetGameObject();

                if (pObject != 0 && pObject->GetArea() != 0
                    && static_cast<CSWSArea*>(pObject->GetArea())->m_objectId
                           == m_objectId)
                {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()
                        ->SendServerToPlayerArea_Weather(pPlayer, nWeather, bTurnOn,
                                                         m_chanceRain);
                }
            }

            pPlayer = reinterpret_cast<CSWSPlayer*>(pList->GetNext(pNode));
        }
    }

    m_nSentWeather = (bTurnOn == 0) ? 0 : nWeather;
}

// Sends a one-shot visual effect to every player object within the effect's radius, plus a
// little slack.
void CSWSArea::PlayVisualEffect(unsigned short nEffectId, const Vector& vPosition)
{
    float fRange;
    {
        CExoString sColumn("RADIUS");
        g_pRules->m_p2DArrays->m_pVisualEffect->GetFLOATEntry(nEffectId, sColumn, &fRange);
    }

    fRange = fRange + 250.0f;

    float fXMin = vPosition.x - fRange;
    float fXMax = fRange + vPosition.x;

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObjectArray* pSearchArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;

    int nLast = m_aObjectsByX.m_count - 1;
    int nLow = 0;
    int nHigh = nLast;

    while (nLow < nHigh || nLow == nLast)
    {
        int nMid = (nLow + nHigh) >> 1;

        if (pSearchArray->GetGameObject(m_aObjectsByX.m_pArray[nMid], &pObject)
                != CGameObjectArray::SUCCESS)
            return;

        if (static_cast<CSWSObject*>(pObject)->m_position.x >= fXMin)
        {
            if (nMid == nHigh)
                goto found;

            nHigh = nMid;
        }
        else
        {
            nLow = nMid + 1;
        }
    }

    if (nLow != nHigh)
        return;

found:
    for (int i = nHigh; i < m_aObjectsByX.m_count; i++)
    {
        if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i], &pObject)
                != CGameObjectArray::SUCCESS)
            continue;

        if (pObject == 0)
            continue;

        if (pObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
            continue;

        Vector vObjectPos = static_cast<CSWSObject*>(pObject)->m_position;

        if (vObjectPos.x > fXMax)
            return;

        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pObject->m_objectId);

        if (pPlayer == 0)
            continue;

        float dx = vObjectPos.x - vPosition.x;
        float dy = vObjectPos.y - vPosition.y;
        float dz = vObjectPos.z - vPosition.z;

        if (!(dx * dx + dy * dy + dz * dz <= fRange * fRange))
            continue;

        g_pAppManager->m_pServerExoApp->GetSWSMessage()
            ->SendServerToPlayerArea_VisualEffect(pPlayer, nEffectId, vPosition);
    }
}

// Self-tests the path-point graph for the testpathpoints command. Returns 1 when the graph is bad
// and 0 when every link checks out.
int CSWSArea::TestPathPoint(float fHitFraction, int nFlag)
{
    K2_UNUSED(nFlag);
    int bBad = 0;

    for (unsigned int i = 0; i < m_nPathPointCount; i++)
    {
        ComputeHeight(Vector(m_paPathPoints[i].m_node.m_x, m_paPathPoints[i].m_node.m_y, 0.0f), 1, 0, 0);

        if (m_paPathPoints[i].m_nNeighborStart + m_paPathPoints[i].m_node.m_nNeighborCount == 0)
            bBad = 1;

        for (unsigned int j = m_paPathPoints[i].m_nNeighborStart;
             j < m_paPathPoints[i].m_nNeighborStart + m_paPathPoints[i].m_node.m_nNeighborCount;
             j++)
        {
            Vector vFrom = { m_paPathPoints[i].m_node.m_x, m_paPathPoints[i].m_node.m_y, 0.0f };
            Vector vTo   = { m_paPathPoints[m_paPathPointLinks[j]].m_node.m_x,
                             m_paPathPoints[m_paPathPointLinks[j]].m_node.m_y, 0.0f };

            vFrom.z = ComputeHeight(vFrom, 1, 0, 0);
            vTo.z   = ComputeHeight(vTo, 1, 0, 0);

            if (i > m_paPathPointLinks[j])
            {
                vFrom.z += 0.03f;
                vTo.z   += 0.03f;
            }

            int nResult = TestDirectLine(vFrom, vTo, fHitFraction, 0.0f, 1, 0);

            normalize(cross(Vector(0.0f, 0.0f, 1.0f), vTo - vFrom));
            normalize(vTo - vFrom);

            if (nResult != -3 && nResult != 1)
                bBad = 1;
        }
    }

    return bBad;
}

// Lets line-of-sight pass through a door whose 2DA row says it does not block sight, by
// re-probing the segment with that door excluded.
int CSWSArea::HandleTransparentDoors(const Vector& from, const Vector& to, float zLow, float zHigh,
                                     float radius, unsigned long long oidIgnore,
                                     unsigned long long& oidHit, int nFlag,
                                     CExoArrayList<CGameObject*>* pHits, int nUnused)
{
    K2_UNUSED(nUnused);
    if (oidHit == OBJECT_INVALID)
        return 0;

    CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidHit);

    if (pDoor == 0)
        return 0;

    int nBlockSight;
    unsigned char nAppearance = pDoor->m_appearance;

    if (nAppearance)
    {
        C2DA* pDoorTypes = g_pRules->m_p2DArrays->m_pDoorTypes;
        CExoString sColumn("BlockSight");
        pDoorTypes->GetINTEntry(nAppearance, sColumn, &nBlockSight);
    }
    else
    {
        C2DA* pGenericDoors = g_pRules->m_p2DArrays->m_pGenericDoors;
        unsigned char nGenericType = pDoor->m_genericType;
        CExoString sColumn("BlockSight");
        pGenericDoors->GetINTEntry(nGenericType, sColumn, &nBlockSight);
    }

    if (pHits == 0)
        return 0;

    if (nBlockSight)
        return 0;

    pHits->Add(pDoor->AsSWSObject());

    return NoNonWalkPolysDetailed(from, to, zLow, zHigh, radius, oidIgnore, oidHit, nFlag,
                                  pHits, 0, 0, 0);
}

// Probes the segment against the doors, stopping at the first door box it crosses, and returns 1
// when nothing blocks. On a hit the door is recorded and, when flag2 asks for it, the segment is
// re-probed through the transparent-door path.
int CSWSArea::NoNonWalkPolysInDoors(const Vector& a, const Vector& b, float z, float height,
                                    float radius, unsigned long long oidIgnore,
                                    unsigned long long& oidHit,
                                    CExoArrayList<CGameObject*>* pHits, int flag1, int flag2)
{
    if (m_nStaticBBoxCount < 1)
        return 1;

    float fXMin = (a.x < b.x) ? a.x : b.x;
    float fXMax = (a.x < b.x) ? b.x : a.x;
    float fYMin = (a.y < b.y) ? a.y : b.y;
    float fYMax = (a.y < b.y) ? b.y : a.y;

    fXMin -= radius;
    fXMax += radius;
    fYMin -= radius;
    fYMax += radius;

    for (int i = 0; i < m_nStaticBBoxCount; i++)
    {
        CSWSAreaDoorBox box = reinterpret_cast<const CSWSAreaDoorBox*>(m_pStaticBBoxes)[i];

        if (fXMin < box.m_vMin.x && fXMax < box.m_vMin.x)
            continue;
        if (fXMax > box.m_vMax.x && fXMin > box.m_vMax.x)
            continue;
        if (fYMin < box.m_vMin.y && fYMax < box.m_vMin.y)
            continue;
        if (fYMax > box.m_vMax.y && fYMin > box.m_vMax.y)
            continue;
        if (box.m_vMin.z > z && box.m_vMin.z > height)
            continue;
        if (box.m_vMax.z < height && box.m_vMax.z < z)
            continue;

        unsigned long long oidDoor = m_pStaticBBoxIds[i];
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidDoor);

        if (pObject == 0)
            continue;
        if (pObject->AsSWSDoor() == 0)
            continue;
        if (GetIsIDInExcludeList(oidDoor, pHits))
            continue;
        if (NoNoneWalkPolysInStaticObject(m_pStaticBBoxIds[i], a, b, z, height, radius, oidIgnore,
                                          flag1, 0))
            continue;

        oidHit = m_pStaticBBoxIds[i];

        if (flag2 != 1)
            return 0;

        CExoArrayList<CGameObject*>* pList;
        int bOwned;

        if (pHits != 0)
        {
            pList = pHits;
            bOwned = 0;
        }
        else
        {
            pList = new CExoArrayList<CGameObject*>;
            bOwned = 1;
        }

        int nResult = HandleTransparentDoors(a, b, z, height, radius, oidIgnore, oidHit,
                                                    flag1, pList, 1);

        if (bOwned)
            delete pList;

        return nResult;
    }

    return 1;
}

// Sweeps all the doors like NoNonWalkPolysInDoors, but every door box is visited, the blocking door
// is recorded, and each transparent-door retry is ANDed into the result.
int CSWSArea::NoNonWalkPolysDetailed(const Vector& from, const Vector& to, float zLow, float zHigh,
                                     float radius, unsigned long long oidIgnore,
                                     unsigned long long& oidHit, int a,
                                     CExoArrayList<CGameObject*>* pList, int b,
                                     CWalkHitInfo* pHit, int c)
{
#if KOTOR1  // K1 has an equal(to, from) test whose result is never used
    equal(to, from);
#endif
    if (m_nStaticBBoxCount < 1)
        return 1;

    float fXMin = (from.x < to.x) ? from.x : to.x;
    float fXMax = (from.x < to.x) ? to.x : from.x;
    float fYMin = (from.y < to.y) ? from.y : to.y;
    float fYMax = (from.y < to.y) ? to.y : from.y;

    fXMin -= radius;
    fXMax += radius;
    fYMin -= radius;
    fYMax += radius;

    int nResult = 1;

    for (int i = 0; i < m_nStaticBBoxCount; i++)
    {
        if (c)
        {
            CGameObject* pObject =
                g_pAppManager->m_pServerExoApp->GetGameObject(m_pStaticBBoxIds[i]);

            if (pObject != 0)
            {
                CSWSDoor* pDoor = pObject->AsSWSDoor();

                if (pDoor == 0 || pDoor->m_curOpenState)
                    continue;
            }
        }

        CSWSAreaDoorBox box = reinterpret_cast<const CSWSAreaDoorBox*>(m_pStaticBBoxes)[i];

        if (fXMin < box.m_vMin.x && fXMax < box.m_vMin.x)
            continue;
        if (fXMax > box.m_vMax.x && fXMin > box.m_vMax.x)
            continue;
        if (fYMin < box.m_vMin.y && fYMax < box.m_vMin.y)
            continue;
        if (fYMax > box.m_vMax.y && fYMin > box.m_vMax.y)
            continue;
        if (box.m_vMin.z > zLow && box.m_vMin.z > zHigh)
            continue;
        if (box.m_vMax.z < zHigh && box.m_vMax.z < zLow)
            continue;

        if (GetIsIDInExcludeList(m_pStaticBBoxIds[i], pList))
            continue;

        if (NoNoneWalkPolysInStaticObject(m_pStaticBBoxIds[i], from, to, zLow, zHigh, radius,
                                          oidIgnore, a, pHit))
            continue;

        unsigned long long oidDoor = m_pStaticBBoxIds[i];
        oidHit = oidDoor;

        if (pHit)
            pHit->m_hitObjectId = oidDoor;

        if (b != 1)
        {
            nResult = 0;
            continue;
        }

        nResult &= HandleTransparentDoors(from, to, zLow, zHigh, radius, oidIgnore, oidHit,
                                                 a, pList, 1);
    }

    return nResult;
}

// Queries line of sight across the area. It sweeps the doors and static objects, then the rooms,
// keeping the nearest blocking hit. Returns 1 when the line is clear. `f` allows transparent doors
// to be swept; `g` forces the room pass.
int CSWSArea::ClearLineOfSight(const Vector& vFrom, const Vector& vTo, Vector* pOut,
                               unsigned long long* pIds, unsigned long long idA,
                               unsigned long long idB, int f, int g)
{
    float fZLow = (vFrom.z < vTo.z) ? vFrom.z : vTo.z;
    float fZHigh = (vFrom.z < vTo.z) ? vTo.z : vFrom.z;

    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitSubType = 0;

    Vector vRoomHit = {};
    unsigned long long oidRoomHit = OBJECT_INVALID;

    CExoArrayList<CGameObject*> aObjects;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(idA);
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        aObjects.Add(pObject);

    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(idB);
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        aObjects.Add(pObject);

    hitInfo.m_hitObjectId = OBJECT_INVALID;

    int bResult = 1;
    int nResult = NoNonWalkPolysDetailed(vFrom, vTo, fZLow, fZHigh, 0.001f, OBJECT_INVALID,
                                         *pIds, 1, &aObjects, f, &hitInfo, 0);

    if (nResult == 0)
    {
        pOut->x = hitInfo.m_hitPosition.x;
        pOut->y = hitInfo.m_hitPosition.y;
        pOut->z = hitInfo.m_hitPosition.z;
        bResult = 0;
    }

    if (nResult != 0 || g != 0)
    {
        float fBest = 3.402823466e+38f;   // FLT_MAX

        for (int i = 0; i < m_nRoomCount; i++)
        {
            if (m_paRooms[i].ClearLineOfSight(vFrom, vTo, &vRoomHit, &oidRoomHit) != 0)
                continue;

            float fDist = (vRoomHit.x - vFrom.x) * (vRoomHit.x - vFrom.x) +
                          (vRoomHit.y - vFrom.y) * (vRoomHit.y - vFrom.y) +
                          (vRoomHit.z - vFrom.z) * (vRoomHit.z - vFrom.z);

            if (fDist < fBest)
            {
                *pOut = vRoomHit;
                bResult = 0;
                fBest = fDist;
            }
        }
    }

    return bResult;
}

// Pushes creatures out of a newly blocking volume to a safe location. A closing door or
// placeable sends them to their nearest action point; anything else sends them directly away
// from vPosition.
void CSWSArea::BudgeCreatures(const Vector& vPosition, const Vector& vMin, const Vector& vMax,
                              unsigned long long objectId, int isClosing)
{
    int nIndex = 0;
    GetFirstObjectIndiceByX(&nIndex, vPosition.x - vMin.x - 3.0f);

    Vector vTarget;
    Vector vPos;

    if (m_aObjectsByX.m_count > 0)
    {
        while (nIndex < m_aObjectsByX.m_count)
        {
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_aObjectsByX.m_pArray[nIndex]);
            nIndex++;

            if (pCreature == 0)
                continue;

            vPos = pCreature->m_position;

            if (vPos.x >= vMax.x + 3.0f)
                return;
            if (vPos.y <= vMin.y - 3.0f)
                continue;
            if (vPos.y >= vMax.y + 3.0f)
                continue;
            if (TestSafeLocationPoint(vPos, pCreature->m_pPathfindInfo))
                continue;

            if (isClosing)
            {
                CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(objectId);

                if (pDoor)
                {
                    vTarget = pDoor->GetNearestActionPoint(vPos, 0);
                }
                else
                {
                    CSWSPlaceable* pPlaceable =
                        g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(objectId);

                    if (pPlaceable)
                        vTarget = pPlaceable->GetNearestActionPoint(vPos);
                }
            }
            else
            {
                vTarget = vPos + normalize(vPos - vPosition);
            }

            if (ComputeSafeLocation(vTarget, 10.0f, pCreature->m_pPathfindInfo, 0, &vTarget) == 0)
                continue;

            pCreature->SetPosition(vTarget, 1, 1, 0);
        }
    }
}

// Builds the area's client-replication blob into the already-open message. Returns 1 on
// success and 0 when there is no message to write into.
int CSWSArea::PackAreaIntoMessage(int nX, int nY, int nZ, CSWSPlayer* pPlayer)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoString sUnused;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    int nCount = 0;
    CExoArrayList<unsigned long long> aWaypoints;
    CExoString sName;
    CExoString sValue;
    CExoArrayList<int> aPins;
    CExoArrayList<unsigned long long> aSounds;

    if (pMsg == 0)
        return 0;

    pMsg->WriteOBJECTIDServer(m_objectId);
    pMsg->WriteCResRef(m_resRef, 16);
    pMsg->WriteCExoLocStringServer(m_areaName, 0);
    pMsg->WriteINT(nX, 32);
    pMsg->WriteINT(nY, 32);
    pMsg->WriteINT(nZ, 32);
    // The .are "Flags" field is a byte member read as a 32-bit value here.
    pMsg->WriteDWORD(m_nAreaFlags, 32);
    pMsg->WriteINT(m_cameraStyle, 32);
    pMsg->WriteCResRef(m_defaultEnvMap, 16);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_miniGame), 8);
    pMsg->WriteDWORD(m_moonAmbientColor, 32);
    pMsg->WriteDWORD(m_moonDiffuseColor, 32);
    pMsg->WriteBYTE(m_moonFogOn, 8);
    pMsg->WriteDWORD(m_moonFogColor, 32);
    pMsg->WriteFLOAT(m_moonFogNear, 1.0f, 32);
    pMsg->WriteFLOAT(m_moonFogFar, 1.0f, 32);
    pMsg->WriteBOOL(m_moonShadows);
    pMsg->WriteDWORD(m_sunAmbientColor, 32);
    pMsg->WriteDWORD(m_sunDiffuseColor, 32);
    pMsg->WriteBYTE(m_sunFogOn, 8);
    pMsg->WriteDWORD(m_nFogColor, 32);
    pMsg->WriteFLOAT(m_sunFogNear, 1.0f, 32);
    pMsg->WriteFLOAT(m_sunFogFar, 1.0f, 32);
    pMsg->WriteBOOL(m_sunShadows);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_dayNightCycle), 8);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_isNight), 8);
    pMsg->WriteDWORD(m_dynAmbientColor, 32);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_noRest), 8);
    pMsg->WriteBYTE(m_shadowOpacity, 8);

    m_pAmbientSound->PackIntoMessage(pMsg);

    unsigned int nDate, nHour, nMinute, nSecond;
    unsigned int nTimeOfDayElapsed;
    unsigned char nMs;
    g_pAppManager->m_pServerExoApp->GetModule()->GetTime(&nDate, &nHour, &nMinute, &nSecond,
                                                        &nMs, &nTimeOfDayElapsed);
    pMsg->WriteBYTE(nMs, 8);
    pMsg->WriteDWORD(nTimeOfDayElapsed, 32);
    pMsg->WriteBYTE(static_cast<unsigned char>(nSecond), 8);
    pMsg->WriteBYTE(static_cast<unsigned char>(nMinute), 8);
    pMsg->WriteBYTE(static_cast<unsigned char>(nHour), 8);
    pMsg->WriteDWORD(nDate, 32);
    pMsg->WriteBYTE(g_pAppManager->m_pServerExoApp->GetModule()->m_nDawnHour, 8);
    pMsg->WriteBYTE(g_pAppManager->m_pServerExoApp->GetModule()->m_nDuskHour, 8);

    pMsg->WriteCResRef(m_grassTexName, 16);
    pMsg->WriteFLOAT(m_grassDensity, 1.0f, 32);
    pMsg->WriteFLOAT(m_grassQuadSize, 1.0f, 32);
    pMsg->WriteDWORD(m_grassDiffuse, 32);
    pMsg->WriteDWORD(m_grassAmbient, 32);
#if !KOTOR1
    pMsg->WriteDWORD(m_grassEmissive, 32);
#endif
    pMsg->WriteFLOAT(m_grassProbLL, 1.0f, 32);
    pMsg->WriteFLOAT(m_grassProbLR, 1.0f, 32);
    pMsg->WriteFLOAT(m_grassProbUL, 1.0f, 32);
    pMsg->WriteFLOAT(m_grassProbUR, 1.0f, 32);
    pMsg->WriteBYTE(m_chanceLightning, 8);
    pMsg->WriteBYTE(m_windPower, 8);
    if (m_nWeatherOverride == WEATHER_NONE)
    {
        pMsg->WriteBYTE(m_nCurrentWeather, 8);
        pMsg->WriteBOOL(m_bWeatherOn);
    }
    else
    {
        pMsg->WriteBYTE(m_nWeatherOverride, 8);
        pMsg->WriteBOOL(1);
    }
#if !KOTOR1  // K1 does not send the rain chance
    pMsg->WriteBYTE(m_chanceRain, 8);
#endif
    pMsg->WriteFLOAT(m_alphaTest, 1.0f, 32);

    pMsg->WriteINT(m_nRoomAudioCount, 32);
    if (m_nRoomAudioCount >= 1)
    {
        for (int i = 0; i < m_nRoomAudioCount; i++)
        {
            pMsg->WriteCExoString(m_paRoomNames[i], 32);
            pMsg->WriteINT(m_paRoomEnvAudio[i], 32);
            pMsg->WriteFLOAT(m_paRoomAmbientScale[i], 1.0f, 32);
        }
    }

    pMsg->WriteINT(m_nPartSoundCount, 32);
    if (m_nPartSoundCount >= 1)
    {
        for (int i = 0; i < m_nPartSoundCount; i++)
        {
            pMsg->WriteCExoString(m_lstPartSoundRoomNames.m_pArray[i], 32);
            pMsg->WriteBOOL(m_lstPartSoundLooping.m_pArray[i]);
            pMsg->WriteCExoString(m_lstPartSoundModelParts.m_pArray[i], 32);
            pMsg->WriteCExoString(m_lstPartSoundOmenEvents.m_pArray[i], 32);
            pMsg->WriteCResRef(m_lstPartSoundResRefs.m_pArray[i], 16);
        }
    }

    // Read after both room lists, because the client expects the north axis and the extents here.
    pMsg->WriteDWORD(g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap->m_nNorthAxis, 32);
    pMsg->WriteINT(m_nWidth, 32);
    pMsg->WriteINT(m_nHeight, 32);

    // Filters the visible objects in one pass over the spatial index, collecting the objects that
    // need their own blob.
    if (m_aObjectsByX.m_count >= 1)
    {
        for (int i = 0; i < m_aObjectsByX.m_count; i++)
        {
            CGameObject* pObject;
            if (pObjectArray->GetGameObject(m_aObjectsByX.m_pArray[i], &pObject) != CGameObjectArray::SUCCESS)
                continue;
            CSWSWaypoint* pWaypoint = pObject->AsSWSWaypoint();
            if (pWaypoint == 0)
            {
                if (pObject->AsSWSSoundObject() != 0)
                    aSounds.Add(m_aObjectsByX.m_pArray[i]);
            }
            else if (pWaypoint->m_bHasMapNote != 0)
            {
                aWaypoints.Add(m_aObjectsByX.m_pArray[i]);
                nCount++;
            }
        }
    }

    pMsg->WriteINT(nCount, 32);
    if (nCount > 0)
    {
        for (int i = 0; i < nCount; i++)
        {
            CGameObject* pObject;
            if (pObjectArray->GetGameObject(aWaypoints.m_pArray[i], &pObject) != CGameObjectArray::SUCCESS)
                continue;
            CSWSWaypoint* pWaypoint = pObject->AsSWSWaypoint();
            if (pWaypoint == 0)
                continue;
            pMsg->WriteOBJECTIDServer(pWaypoint->m_objectId);
            pMsg->WriteFLOAT(pWaypoint->m_position.x, 1.0f, 32);
            pMsg->WriteFLOAT(pWaypoint->m_position.y, 1.0f, 32);
            pMsg->WriteFLOAT(pWaypoint->m_position.z, 1.0f, 32);
            pMsg->WriteBOOL(pWaypoint->m_bMapNoteEnabled);
            pMsg->WriteCExoLocStringServer(pWaypoint->m_mapNote, 0);
        }
    }

    // Map pins live as script variables on the player's own creature; only the pins that name
    // this area are sent.
    CSWSCreature* pCreature = 0;
    CSWSObject* pPlayerObject = pPlayer->GetGameObject();
    if (pPlayerObject != 0)
        pCreature = pPlayerObject->AsSWSCreature();
    if (pCreature != 0)
    {
        CExoString sTotal("NW_TOTAL_MAP_PINS");
        int nPins = pCreature->m_scriptVars.GetInt(sTotal);
        if (nPins != 0)
        {
            // The loop is unsigned, so a negative NW_TOTAL_MAP_PINS wraps.
            for (unsigned int i = 0; i <= static_cast<unsigned int>(nPins); i++)
            {
                sName.Format(const_cast<char*>("NW_MAP_PIN_NTRY_%i"), i);
                sValue = pCreature->m_scriptVars.GetString(sName);
                if (sValue != "")
                {
                    sName.Format(const_cast<char*>("NW_MAP_PIN_AREA_%i"), i);
                    if (pCreature->m_scriptVars.GetObjectA(sName) == m_objectId)
                        aPins.Add(i);
                }
            }
        }
    }

    pMsg->WriteINT(aPins.m_count, 32);
    for (int i = 0; i < aPins.m_count; i++)
    {
        K2_ASSUME(pCreature != 0);
        pMsg->WriteINT(aPins.m_pArray[i], 32);
        sName.Format(const_cast<char*>("NW_MAP_PIN_NTRY_%i"), aPins.m_pArray[i]);
        sValue = pCreature->m_scriptVars.GetString(sName);
        pMsg->WriteCExoString(sValue, 32);
        sName.Format(const_cast<char*>("NW_MAP_PIN_XPOS_%i"), aPins.m_pArray[i]);
        pMsg->WriteFLOAT(pCreature->m_scriptVars.GetFloat(sName), 1.0f, 32);
        sName.Format(const_cast<char*>("NW_MAP_PIN_YPOS_%i"), aPins.m_pArray[i]);
        pMsg->WriteFLOAT(pCreature->m_scriptVars.GetFloat(sName), 1.0f, 32);
        sName.Format(const_cast<char*>("NW_MAP_PIN_ZPOS_%i"), aPins.m_pArray[i]);
        pMsg->WriteFLOAT(pCreature->m_scriptVars.GetFloat(sName), 1.0f, 32);
    }

    unsigned short nSoundCount = static_cast<unsigned short>(aSounds.m_count);
    pMsg->WriteWORD(nSoundCount, 16);
    for (unsigned short i = 0; i < nSoundCount; i++)
    {
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(aSounds.m_pArray[i], &pObject) != CGameObjectArray::SUCCESS)
            continue;
        CSWSSoundObject* pSound = pObject->AsSWSSoundObject();
        if (pSound != 0)
            pSound->PackIntoMessage(pMsg);
    }

    // -- placeables: two counts and two matching lists (lit, and "Static" == 1).
    int nLitCount = 0;
    int nStaticCount = 0;
    if (m_aObjectsByX.m_count >= 1)
    {
        for (int i = 0; i < m_aObjectsByX.m_count; i++)
        {
            CSWSPlaceable* pPlaceable =
                g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(m_aObjectsByX.m_pArray[i]);
            if (pPlaceable == 0)
                continue;
            int bLightIsOn = pPlaceable->GetLightIsOn();
            int bStatic = pPlaceable->m_bStatic;
            nLitCount += (bLightIsOn != 0);
            nStaticCount += (bStatic == 1);
        }
    }

    pMsg->WriteWORD(static_cast<unsigned short>(nLitCount), 16);
    if (m_aObjectsByX.m_count >= 1)
    {
        for (int i = 0; i < m_aObjectsByX.m_count; i++)
        {
            CSWSPlaceable* pPlaceable =
                g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(m_aObjectsByX.m_pArray[i]);
            if (pPlaceable == 0)
                continue;
            if (pPlaceable->GetLightIsOn() == 0)
                continue;
#if KOTOR1  // K1 copies the position out before the first write
            Vector vPosition = pPlaceable->m_position;
            pMsg->WriteOBJECTIDServer(pPlaceable->m_objectId);
            pMsg->WriteBYTE(pPlaceable->m_appearanceType, 8);
            pMsg->WriteFLOAT(vPosition.x, 1.0f, 32);
            pMsg->WriteFLOAT(vPosition.y, 1.0f, 32);
            pMsg->WriteFLOAT(vPosition.z, 1.0f, 32);
#else
            pMsg->WriteOBJECTIDServer(pPlaceable->m_objectId);
            pMsg->WriteINT(pPlaceable->m_appearanceType, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.x, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.y, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.z, 1.0f, 32);
#endif
        }
    }

    pMsg->WriteWORD(static_cast<unsigned short>(nStaticCount), 16);
    if (m_aObjectsByX.m_count >= 1)
    {
        for (int i = 0; i < m_aObjectsByX.m_count; i++)
        {
            CSWSPlaceable* pPlaceable =
                g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(m_aObjectsByX.m_pArray[i]);
            if (pPlaceable == 0)
                continue;
            if (pPlaceable->m_bStatic != 1)
                continue;
#if KOTOR1  // K1 copies position and orientation out before the first write
            Vector vPosition = pPlaceable->m_position;
            Vector vOrientation = pPlaceable->CSWSObject::m_orientation;
            pMsg->WriteOBJECTIDServer(pPlaceable->m_objectId);
            pMsg->WriteBYTE(pPlaceable->m_appearanceType, 8);
            pMsg->WriteFLOAT(vPosition.x, 1.0f, 32);
            pMsg->WriteFLOAT(vPosition.y, 1.0f, 32);
            pMsg->WriteFLOAT(vPosition.z, 1.0f, 32);
            pMsg->WriteFLOAT(vOrientation.x, 1.0f, 32);
            pMsg->WriteFLOAT(vOrientation.y, 1.0f, 32);
            pMsg->WriteFLOAT(vOrientation.z, 1.0f, 32);
#else
            pMsg->WriteOBJECTIDServer(pPlaceable->m_objectId);
            pMsg->WriteINT(pPlaceable->m_appearanceType, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.x, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.y, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->m_position.z, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->CSWSObject::m_orientation.x, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->CSWSObject::m_orientation.y, 1.0f, 32);
            pMsg->WriteFLOAT(pPlaceable->CSWSObject::m_orientation.z, 1.0f, 32);
#endif
        }
    }

    return 1;
}

// Empty wrapper; the real save entry point is SaveGIT.
void CSWSArea::SaveArea(CERFFile* pERF, const CExoString& sFileName) { K2_UNUSED(pERF); K2_UNUSED(sFileName); }

CSWSArea* CSWSArea::AsSWSArea() { return this; }

CSWSArea* CSWSArea::NWAreaAsSWSArea() { return this; }
