#include "k2stdint.h"
#include "nwstrigger.h"
#include "CExoArrayListImpl.h"
#include "worldtimer.h"
#include "exobase.h"
#include "nwscript.h"
#include "nwsscriptevent.h"
#include "serveraimaster.h"
#include "nwsarea.h"
#include "nwscreature.h"
#include "nwsitem.h"
#include "nwsrules.h"
#include "twodimarray.h"
#include "nwsmodule.h"
#include "Quaternion.h"
#include "nwsplayer.h"
#include "swparty.h"
#include "guiingame.h"
#include "gameobjectarray.h"
#include "nwswaypoint.h"
#include "nwccreature.h"
#include "nwseffectlist.h"
#include "nwscreaturestats.h"
#include "nwsscriptevent.h"
#include "serveranimation.h"
#include "restype.h"
#include "factionmanager.h"

// feat.2da rows the engine tests that neither nwscript.nss nor CSWSCreatureStats names.
enum {
    FEAT_POINT_GUARD = 239,
};

int RenderQATriggers;

float CSWSTrigger::m_fTrapRadiusRange = 1.0f;

float Yaw(const Vector& orientation);
extern int quaternionidentityoptimization;
#if KOTOR1
Quaternion operator-(const Quaternion& q);
#endif

#include "port/libcstring.h"
#include "exostringinline.h"

CSWSTrigger* CSWSTrigger::AsSWSTrigger() {
    return this;
}

CExoLocString& CSWSTrigger::GetFirstName() {
    return m_localizedName;
}

CExoString* CSWSTrigger::GetScriptName(int index) {
    return static_cast<unsigned>(index) < 7 ? &m_scriptSlots[index] : 0;
}

void* CSWSTrigger::GetClientTrigger() {
    if (GetClientObject() == 0) return 0;
    CGameObject* pClientObj = GetClientObject();
    return pClientObj->AsSWCTrigger();
}

// "LinkedTo" names a waypoint in the module; the target is the area that waypoint stands in.
unsigned long long CSWSTrigger::GetTargetArea() {
    if (m_bAreaTransition != 0) {
        unsigned long long oidWaypoint =
            g_pAppManager->m_pServerExoApp->GetModule()->GetWaypoint(m_linkedTo);
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidWaypoint, &pObject)
            == CGameObjectArray::SUCCESS) {
            return static_cast<CSWSArea*>(pObject->AsSWSWaypoint()->GetArea())->m_objectId;
        }
    }
    return OBJECT_INVALID;
}

CSWSTrigger::CSWSTrigger(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_TRIGGER, objectId, 0) {
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);
    m_nAnimation = SERVER_ANIMATION_READY;
    if (g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(
            17, CExoString("PrimaryRange"), &m_fTrapRadiusRange) == 0) {
        m_fTrapRadiusRange = 1.0f;
    }
    m_linkedTo = "";
    m_nGeomVertCount = 0;
    m_pGeomVerts = 0;
    m_nGeomVertCount2 = 0;
    m_pGeomVerts2 = 0;
    m_nGeomIndexCount = 0;
    m_pGeomIndices = 0;
    m_nUserDefinedEventNumber = 0;
    m_oidEnteringObject = OBJECT_INVALID;
    m_oidExitingObject = OBJECT_INVALID;
    m_oidDisarmedBy = OBJECT_INVALID;
    m_bAreaTransition = 0;
    m_bTrap = 0;
    m_bTrapDetectable = 1;
    m_bTrapDisarmable = 1;
    m_bMineFlagged = 0;
    m_keyName = "";
    m_bAutoRemoveKey = 0;
    m_bTrapOneShot = 1;
    m_trapType = TRAP_TYPE_NONE;
    m_nFaction = 0;
    m_baseDetectDC = 0;
    m_baseDisarmDC = 0;
#if !KOTOR1  // K1 has no OwnerDemolitionsSkill
    m_nOwnerDemolitionsSkill = 0;
#endif
    for (int i = 0; i < 7; i++) {
        m_scriptSlots[i] = "default";
    }
    m_lastUpdateTod = 0;
    m_lastUpdateDay = 0;
    m_cursor = 0;
    m_fBoundsMinY = 0.0f;
    m_fBoundsMaxY = 0.0f;
    m_fBoundsMinX = 0.0f;
    m_fBoundsMaxX = 0.0f;
    m_creatorId = OBJECT_INVALID;
    m_fHighlightHeight = 0.1f;
    m_loadScreenId = 0;
    m_nScanIndex = 0;
    m_bSetByPlayerParty = 0;
    int nAnim = SERVER_ANIMATION_MINE_HIDDEN;
    SetAnimation(nAnim);
}

// Contains returns a count, so a duplicate registration is left alone.
void CSWSTrigger::RemoveFromArea() {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
        static_cast<CSWSArea*>(GetArea())->RemoveSubArea(this);
        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId) == 1) {
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
        }
    }
    SetArea(0);
    Vector vOrigin;
    SetPosition(vOrigin, 1, 1, 0);
}

void CSWSTrigger::AddToArea(CSWSArea* pArea, float x, float y, float z, int a5) {
    CSWSArea* pA;
    RemoveFromArea();
    if (pArea != 0) {
        CExoString sUnused;
        SetArea(pArea);
        {
            Vector vPos;
            vPos.x = x;
            vPos.y = y;
            vPos.z = z;
            SetPosition(vPos, 0, 1, 0);
        }
        static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, a5);
        pA = static_cast<CSWSArea*>(GetArea());
        pA->AddSubArea(this);
        float fMinX, fMinY, fMaxX, fMaxY;
        ComputeBoundingBox(&fMinX, &fMinY, &fMaxX, &fMaxY);
    }
}

CSWSTrigger::~CSWSTrigger() {
    delete[] m_pGeomVerts;
    delete[] m_pGeomIndices;
    if (GetArea() != 0) {
        RemoveFromArea();
    }
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    }
}

// Runs OnHeartbeat at most once every 6 seconds of world time, then, for a trap, refreshes the
// mine's visibility animation before advancing the action queue.
void CSWSTrigger::AIUpdate() {
    unsigned long long timerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned int elapsedDay, elapsedTod;
    unsigned int day, tod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&day, &tod);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_lastUpdateDay, m_lastUpdateTod, &elapsedDay, &elapsedTod);
    if ((elapsedTod >> 4) >= HEARTBEAT_INTERVAL_16MS) {
        g_pVirtualMachine->RunScript(&m_scriptSlots[0], m_objectId, 1);  // ScriptHeartbeat
        m_lastUpdateDay = day;
        m_lastUpdateTod = tod;
    }
    if (m_bTrap != 0) {
        CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (m_bMineFlagged != 0) {
            if (m_nAnimation != SERVER_ANIMATION_MINE_VISIBLE) SetAnimation(SERVER_ANIMATION_MINE_VISIBLE);
        } else if (pPlayer != 0 &&
                   (m_nFaction == static_cast<unsigned int>(pPlayer->m_pStats->m_factionId) ||
                    GetReputation(pPlayer->m_objectId, 0) > CFactionManager::REPUTATION_FRIENDLY_ABOVE ||
                    m_objectsInTrigger.Contains(pPlayer->m_objectId) != 0)) {
            if (m_nAnimation != SERVER_ANIMATION_MINE_VISIBLE) SetAnimation(SERVER_ANIMATION_MINE_VISIBLE);
        } else {
            if (m_nAnimation != SERVER_ANIMATION_MINE_HIDDEN) SetAnimation(SERVER_ANIMATION_MINE_HIDDEN);
        }
    }
    RunActions(day, tod, timerSnapshot);
}

// Min and max equal on both axes means the cached bounds have not been computed yet.
void CSWSTrigger::ComputeBoundingBox(float* pMinX, float* pMinY, float* pMaxX, float* pMaxY) {
    if (m_fBoundsMinX == m_fBoundsMaxX && m_fBoundsMinY == m_fBoundsMaxY) {
        m_fBoundsMinX = m_pGeomVerts[0].x;
        m_fBoundsMinY = m_pGeomVerts[0].y;
        m_fBoundsMaxX = m_pGeomVerts[0].x;
        m_fBoundsMaxY = m_pGeomVerts[0].y;
        for (int i = 1; i < m_nGeomVertCount; i++) {
            if (m_pGeomVerts[i].x < m_fBoundsMinX) m_fBoundsMinX = m_pGeomVerts[i].x;
            if (m_pGeomVerts[i].y < m_fBoundsMinY) m_fBoundsMinY = m_pGeomVerts[i].y;
            if (m_pGeomVerts[i].x > m_fBoundsMaxX) m_fBoundsMaxX = m_pGeomVerts[i].x;
            if (m_pGeomVerts[i].y > m_fBoundsMaxY) m_fBoundsMaxY = m_pGeomVerts[i].y;
        }
    }
    *pMinX = m_fBoundsMinX;
    *pMinY = m_fBoundsMinY;
    *pMaxX = m_fBoundsMaxX;
    *pMaxY = m_fBoundsMaxY;
}

// Samples a 6x6 lattice over the polygon's bounding box and keeps the point inside the polygon
// that is closest to `v`.
int CSWSTrigger::GetClosestPointWithinTrigger(const Vector& v, Vector* pOut) {
    float fMinX, fMinY, fMaxX, fMaxY;
    ComputeBoundingBox(&fMinX, &fMinY, &fMaxX, &fMaxY);
    Vector vTest;
    if (fMinX <= fMaxX) {
        float fStepX = (fMaxX - fMinX) * 0.2f;
        float fStepY = (fMaxY - fMinY) * 0.2f;
        float fBestDistSq = 1.0e11f;
        float fBestY = 0.0f;
        float fBestX = 0.0f;
        for (float x = fMinX; x <= fMaxX; x += fStepX) {
            for (float y = fMinY; y <= fMaxY; y += fStepY) {
                vTest.x = x;
                vTest.y = y;
                vTest.z = 0.0f;
                float dx = v.x - vTest.x;
                float dy = v.y - vTest.y;
                float dz = v.z - vTest.z;
                float fDistSq = dx * dx + dy * dy + dz * dz;
                if (fDistSq < fBestDistSq && InTrigger(vTest) == 1) {
                    fBestX = x;
                    fBestY = y;
                    fBestDistSq = fDistSq;
                }
            }
        }
        if (fBestDistSq < 1.0e10f) {
            pOut->x = fBestX;
            pOut->y = fBestY;
            pOut->z = 0.0f;
            return 1;
        }
    }
    return 0;
}

// The centre of the 2D bounding box, with Z sampled off the area walkmesh.
Vector CSWSTrigger::GetFacingPosition() {
    Vector vFacing;
    float fMinX, fMinY, fMaxX, fMaxY;
    ComputeBoundingBox(&fMinX, &fMinY, &fMaxX, &fMaxY);
    vFacing.x = (fMaxX - fMinX) * 0.5f + fMinX;
    vFacing.y = fMinY + (fMaxY - fMinY) * 0.5f;
    vFacing.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vFacing, 1, 0, 0);
    return vFacing;
}

// A trap is a sphere around its own position; anything else is a crossing-number ray-cast in +y
// over the polygon.
int CSWSTrigger::InTrigger(const Vector& v) {
    if (m_bTrap) {
        float dx = v.x - m_position.x;
        float dy = v.y - m_position.y;
        float dz = v.z - m_position.z;
        return dx * dx + dy * dy + dz * dz <= m_fTrapRadiusRange * m_fTrapRadiusRange;
    }
    int64_t n = m_nGeomIndexCount;
    int count = 0;
    if (n >= 1) {
        int* pIndices = m_pGeomIndices;
        Vector* pVerts = m_pGeomVerts;
        float px = v.x;
        float py = v.y;
        unsigned last = m_nGeomIndexCount - 1;
        int64_t i = 0;
        do {
            int64_t idxCur = pIndices[i];
            int64_t idxNext = pIndices[(i == last) ? 0 : i + 1];
            float nextX = pVerts[idxNext].x;
            float curX = pVerts[idxCur].x;
            if (nextX != curX) {
                float nextY = pVerts[idxNext].y;
                float curY = pVerts[idxCur].y;
                float dx = nextX - curX;
                float curMinusPx = curX - px;
                float dy = nextY - curY;
                float slope = dy / dx;
                float yInt = curY - slope * curMinusPx;
                if (yInt > py) {
                    bool inside;
                    if (__builtin_fabsf(dy) > __builtin_fabsf(dx)) {
                        if (nextY > curY) inside = (yInt >= curY) && (yInt < nextY);
                        else inside = (curY > nextY) && (yInt > nextY) && (yInt <= curY);
                    } else {
                        if (nextX > curX) inside = (px >= curX) && (px < nextX);
                        else inside = (curX > nextX) && (px > nextX) && (px <= curX);
                    }
                    count += inside;
                }
            }
            if (nextX == px) {
                if (pVerts[idxNext].y > py) count++;
                else if (pVerts[idxCur].y >= py) count++;
            }
            i++;
        } while (i < n);
    }
    return (count % 2) == 1;
}

// For a trap, the nearest point on the mine's blast radius along the direction of `v`; otherwise
// the closest point on any polygon edge.
int CSWSTrigger::CalculateNearestPoint(const Vector& v, Vector* pOut) {
    float fNearX, fNearY, fNearZ;
    if (m_bTrap) {
        Vector vDir;
        float fRange;
        if (g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(
                17, CExoString("PrimaryRange"), &fRange) == 0) {
            fRange = 1.0f;
        }
        fNearX = v.x;
        fNearY = v.y;
        fNearZ = v.z;
        vDir.x = v.x - m_position.x;
        vDir.y = v.y - m_position.y;
        vDir.z = v.z - m_position.z;
        if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 0.0001f) {
            vDir = normalize(vDir);
            fNearX = m_position.x + vDir.x * fRange;
            fNearY = vDir.y * fRange + m_position.y;
            fNearZ = vDir.z * fRange + m_position.z;
        }
    } else {
        int64_t n = m_nGeomIndexCount;
        fNearX = 0.0f;
        fNearY = 0.0f;
        fNearZ = 0.0f;
        if (n >= 1) {
            int* pIndices = m_pGeomIndices;
            Vector* pVerts = m_pGeomVerts;
            float px = v.x;
            float py = v.y;
            float pz = v.z;
            float fBestDistSq = 1.0e8f;
            unsigned last = m_nGeomIndexCount - 1;
            for (int64_t i = 0; i < n; i++) {
                int64_t idxCur = pIndices[i];
                int64_t idxNext = pIndices[(i == last) ? 0 : i + 1];
                float curX = pVerts[idxCur].x;
                float curY = pVerts[idxCur].y;
                float curZ = pVerts[idxCur].z;
                float ex = pVerts[idxNext].x - curX;
                float ey = pVerts[idxNext].y - curY;
                float ez = pVerts[idxNext].z - curZ;
                float fDot = (px - curX) * ex + (py - curY) * ey + (pz - curZ) * ez;
                float fLenSq = ex * ex + ey * ey + ez * ez;
                float t = fDot / fLenSq;
                float ptX, ptY, ptZ;
                if (t > 1.0f) {
                    ptX = curX + ex;
                    ptY = curY + ey;
                    ptZ = curZ + ez;
                } else if (t < 0.0f) {
                    ptX = curX;
                    ptY = curY;
                    ptZ = curZ;
                } else {
                    ptX = curX + ex * t;
                    ptY = curY + ey * t;
                    ptZ = curZ + ez * t;
                }
                float dx = px - ptX;
                float dy = py - ptY;
                float dz = pz - ptZ;
                float fDistSq = dx * dx + dy * dy + dz * dz;
                if (fDistSq < fBestDistSq) {
                    fNearX = ptX;
                    fNearY = ptY;
                    fNearZ = ptZ;
                    fBestDistSq = fDistSq;
                }
            }
        }
    }
    pOut->x = fNearX;
    pOut->y = fNearY;
    pOut->z = fNearZ;
    return 1;
}

#if !KOTOR1  // absent in K1
int CSWSTrigger::GetDisarmDC() {
    int base = m_baseDisarmDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    int dc = adj + base;
    return dc > 1 ? dc : 1;
}
#endif // !KOTOR1

#if !KOTOR1  // absent in K1
int CSWSTrigger::GetDetectDC() {
    int base = m_baseDetectDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    int dc = adj + base;
    return dc > 1 ? dc : 1;
}
#endif // !KOTOR1

void CSWSTrigger::SetCreator(unsigned long long objectId) {
    m_creatorId = objectId;
    int result;
    if (objectId != OBJECT_INVALID) {
        if (g_pAppManager->m_pServerExoApp->GetPlayerCreatureId() == objectId) {
            goto isCreator;
        }
        CSWPartyTable* pTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pTable->GetIsMember(objectId) != 0) {
            goto isCreator;
        }
    }
    result = 0;
    goto store;
isCreator:
    result = 1;
store:
    m_bSetByPlayerParty = result;
}

// Rebuilds the polygon as a square of half-extent fHalfExtent centred on m_position; the position
// parameter is unused because the trigger has already been moved there. Each corner's Z comes from
// the area walkmesh, falling back to the trigger's own Z.
void CSWSTrigger::CreateNewGeometry(float fHalfExtent, const Vector& vCentre, CSWSArea* pArea) {
    K2_UNUSED(vCentre);
    m_nGeomIndexCount = 4;
    m_nGeomVertCount = 4;
    if (m_pGeomVerts != 0) {
        delete[] m_pGeomVerts;
        m_pGeomVerts = 0;
    }
    if (m_pGeomIndices != 0) {
        delete[] m_pGeomIndices;
        m_pGeomIndices = 0;
    }
    if (pArea == 0) return;

    m_pGeomVerts = new Vector[4];
    m_pGeomIndices = new int[4];

    m_pGeomVerts[0].x = m_position.x - fHalfExtent;
    m_pGeomVerts[0].y = m_position.y + fHalfExtent;
    m_pGeomVerts[0].z = pArea->ComputeHeight(m_pGeomVerts[0], 1, 0, 0);
    if (m_pGeomVerts[0].z == 0.0f) m_pGeomVerts[0].z = m_position.z;
    m_pGeomIndices[0] = 0;

    m_pGeomVerts[1].x = m_position.x - fHalfExtent;
    m_pGeomVerts[1].y = m_position.y - fHalfExtent;
    m_pGeomVerts[1].z = pArea->ComputeHeight(m_pGeomVerts[1], 1, 0, 0);
    if (m_pGeomVerts[1].z == 0.0f) m_pGeomVerts[1].z = m_position.z;
    m_pGeomIndices[1] = 1;

    m_pGeomVerts[2].x = m_position.x + fHalfExtent;
    m_pGeomVerts[2].y = m_position.y - fHalfExtent;
    m_pGeomVerts[2].z = pArea->ComputeHeight(m_pGeomVerts[2], 1, 0, 0);
    if (m_pGeomVerts[2].z == 0.0f) m_pGeomVerts[2].z = m_position.z;
    m_pGeomIndices[2] = 2;

    m_pGeomVerts[3].x = m_position.x + fHalfExtent;
    m_pGeomVerts[3].y = m_position.y + fHalfExtent;
    m_pGeomVerts[3].z = pArea->ComputeHeight(m_pGeomVerts[3], 1, 0, 0);
    if (m_pGeomVerts[3].z == 0.0f) m_pGeomVerts[3].z = m_position.z;
    m_pGeomIndices[3] = 3;
}

int CSWSTrigger::LoadFromTemplate(const CResRef& resRef) {
    CResStruct topStruct;
    CResGFF* pGff = new CResGFF(RESTYPE_UTT, const_cast<char*>("UTT "), resRef);
    int result;
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        LoadTrigger(pGff, &topStruct);
        delete pGff;
        result = 1;
    } else {
        delete pGff;
        result = 0;
    }
    return result;
}

int CSWSTrigger::LineSegmentIntersectTrigger(const Vector& p1, const Vector& p2) {
    CSWArea* pArea = GetArea();
    if (pArea == 0) return 0;
    for (int i = 0; i < m_nGeomIndexCount; i++) {
        int next = (i == m_nGeomIndexCount - 1) ? 0 : i + 1;
        int idxA = m_pGeomIndices[i];
        int idxB = m_pGeomIndices[next];
        Vector& a = m_pGeomVerts[idxA];
        Vector& b = m_pGeomVerts[idxB];
        float outT1, outT2;
        if (pArea->IntersectLineSegments(p1.x, p1.y, p2.x, p2.y, a.x, a.y, b.x, b.y, &outT1, &outT2) == 1) {
            return 1;
        }
    }
    return 0;
}

// Only fires when the target is not spared by faction or reputation, its own mine immunity, or the
// party leader's.
int CSWSTrigger::GetCanFireMineOnObject(unsigned long long oid, int nParam) {
    CSWSCreature* pCreator =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_creatorId);
    CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oid);
    if (pTarget == 0) return 1;

    int bCanFire = 1;
    if (nParam == 0) {
        if (pCreator != 0) {
            if (pCreator->GetFaction() != 0) {
                if (pTarget->GetReputation(pCreator->m_objectId, 0) > 10 ||
                    m_nFaction == static_cast<unsigned int>(pTarget->m_pStats->m_factionId)) {
                    bCanFire = 0;
                }
            }
        } else {
            if (pTarget->GetReputation(m_objectId, 0) > 10 ||
                static_cast<unsigned int>(pTarget->m_pStats->m_factionId) == m_nFaction) {
                bCanFire = 0;
            }
        }
    }

    if (pTarget->m_pStats->GetEffectImmunity(static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MINE), 0) != 0) {
        bCanFire = 0;
    }
#if !KOTOR1  // K1 has no mine-immunity feats
    if (pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_MINE_IMMUNITY) != 0) {
        bCanFire = 0;
    }

    if (pTarget->m_bInParty != 0) {
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader != 0) {
            CSWSCreature* pLeaderServer = pLeader->GetServerCreature();
            if (pLeaderServer != 0) {
                if (pLeaderServer->m_pStats->HasFeat(FEAT_POINT_GUARD) != 0) bCanFire = 0;
            }
        }
    }
#endif
    return bCanFire;
}

// A matching key in the enterer's repository or equipment disarms the mine; otherwise the trap
// fires and TrapOneShot decides whether it destroys itself.
void CSWSTrigger::OnEnterMine(int nParam)
{
    CSWSCreature* pEnterer =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidEnteringObject);
    if (pEnterer == 0) return;
    if (GetCanFireMineOnObject(pEnterer->m_objectId, nParam) == 0) return;

    unsigned long long oidKey = pEnterer->GetItemRepository(1)->FindItemWithTag(m_keyName);
    if (oidKey == OBJECT_INVALID) {
#if KOTOR1  // K1 has 18 equipment slots
        for (int nSlot = 0; nSlot < 18; nSlot++) {
#else
        for (int nSlot = 0; nSlot < 20; nSlot++) {
#endif
            CSWSItem* pSlotKey = static_cast<CSWSItem*>(pEnterer->m_pInventory->GetItemInSlot(1 << nSlot));
            if (pSlotKey != 0 && pSlotKey->m_sTag == m_keyName) {
                oidKey = pSlotKey->m_objectId;
            }
        }
    }

    CSWSItem* pKey = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidKey);
    if (pKey != 0) {
        // Disarmed: an AutoRemoveKey is consumed from the repository and the inventory.
        if (m_bAutoRemoveKey != 0) {
            pEnterer->GetItemRepository(1)->RemoveItem(pKey);
            pEnterer->m_pInventory->RemoveItem(pKey);
            delete pKey;
        }
        m_oidDisarmedBy = m_oidEnteringObject;
        g_pVirtualMachine->RunScript(&m_scriptSlots[5], m_objectId, 1);
    } else {
        pEnterer->SendFeedbackMessage(FEEDBACK_TRIGGERED_MINE, 0);
        g_pVirtualMachine->RunScript(&m_scriptSlots[4], m_objectId, 1);
        g_pVirtualMachine->RunScript(&m_scriptSlots[1], m_objectId, 1);
        if (m_bTrapOneShot == 0) return;
    }
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
}

// Trap scripts and the two trap DCs come from traps.2da when the trigger's own fields are unset or
// still "default". "Type" (2 == trap, 1 == area transition) is read back into the two separate
// flags. The polygon is re-oriented from the old facing to the new one before the new orientation
// is installed.
int CSWSTrigger::LoadTrigger(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;

    // A real portrait row id wins; any value above the stored row range means "unset" and the
    // resref fallback is read instead.
    unsigned short nPortraitId =
        pGFF->ReadFieldWORD(pStruct, const_cast<char*>("PortraitId"), bSuccess, PORTRAIT_ID_NONE);
    if (nPortraitId >= PORTRAIT_ID_CUSTOM) {
        SetPortrait(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), bSuccess,
                                            CResRef("")));
    } else {
        SetPortraitId(nPortraitId);
    }

    CExoString sScript;
    SetCreator(pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("CreatorId"), bSuccess, OBJECT_INVALID));
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptHeartbeat"), bSuccess,
                            CResRef(m_scriptSlots[0])).CopyToString(sScript);
    m_scriptSlots[0] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptOnEnter"), bSuccess,
                            CResRef(m_scriptSlots[1])).CopyToString(sScript);
    m_scriptSlots[1] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptOnExit"), bSuccess,
                            CResRef(m_scriptSlots[2])).CopyToString(sScript);
    m_scriptSlots[2] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptUserDefine"), bSuccess,
                            CResRef(m_scriptSlots[3])).CopyToString(sScript);
    m_scriptSlots[3] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnTrapTriggered"), bSuccess,
                            CResRef(m_scriptSlots[4])).CopyToString(sScript);
    m_scriptSlots[4] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDisarm"), bSuccess,
                            CResRef(m_scriptSlots[5])).CopyToString(sScript);
    m_scriptSlots[5] = sScript;
    // A UTT with no "OnClick" field inherits whatever the OnEnter slot holds: the default here is
    // m_scriptSlots[1], not this slot.
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnClick"), bSuccess,
                            CResRef(m_scriptSlots[1])).CopyToString(sScript);
    m_scriptSlots[6] = sScript;

    m_trapType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapType"), bSuccess, m_trapType);
    if (m_scriptSlots[4].IsEmpty() || m_scriptSlots[4] == "default") {
        CExoString sTrapScript;
        g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(m_trapType,
                                                            CExoString("TrapScript"),
                                                            &sTrapScript);
        m_scriptSlots[4] = sTrapScript;
    }
    m_bTrapOneShot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapOneShot"), bSuccess, static_cast<unsigned char>(m_bTrapOneShot));

    CExoString sLinkedTo;
    sLinkedTo = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("LinkedTo"), bSuccess,
                                           CExoString(const_cast<char*>("")));
    if (bSuccess == 1)
        m_linkedTo = sLinkedTo;
    m_linkedToFlags = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("LinkedToFlags"), bSuccess, 0);

    CResRef resLinkedToModule;
    resLinkedToModule = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("LinkedToModule"), bSuccess,
                                                CResRef(""));
    if (bSuccess == 1)
        m_linkedToModule = resLinkedToModule.GetResRefStr();

    unsigned char bAutoRemoveKey = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AutoRemoveKey"),
                                                        bSuccess, 0);
    if (bSuccess == 1)
        m_bAutoRemoveKey = bAutoRemoveKey;

    CExoString sTag;
    sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, CExoString(const_cast<char*>("")));
    if (bSuccess == 1)
        SetTag(sTag);

    m_localizedName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocalizedName"), bSuccess,
                                                    CExoLocString());
    unsigned int nFaction = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Faction"), bSuccess, 0);
    if (bSuccess)
        m_nFaction = nFaction;
    m_keyName = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("KeyName"), bSuccess,
                                           CExoString(const_cast<char*>("")));
    m_bTrapDisarmable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDisarmable"), bSuccess, 0);
    m_bTrapDetectable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDetectable"), bSuccess, 0);
#if !KOTOR1  // K1 has no OwnerDemolitionsSkill
    m_nOwnerDemolitionsSkill = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OwnerDemolitionsSkill"),
                                                   bSuccess, 0);
#endif

    // The DCs come from the trap's traps.2da row; the GFF does not carry them.
    int nDCMod;
    if (g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(m_trapType, CExoString("DisarmDCMod"),
                                                     &nDCMod))
        m_baseDisarmDC = nDCMod;
    if (g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(m_trapType, CExoString("DetectDCMod"),
                                                     &nDCMod))
        m_baseDetectDC = nDCMod;

    m_cursor = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Cursor"), bSuccess, 0);
    SetPortrait(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), bSuccess, CResRef("")));
    m_transitionDestination = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("TransitionDestination"),
                                                            bSuccess, m_transitionDestination);

    // "Type" splits back into the trap and area-transition flags, and picks the cursor.
    int nType = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Type"), bSuccess, 0);
    if (bSuccess == 1) {
        if (nType == 1) {
            m_bAreaTransition = 1;
            m_cursor = 1;
        } else if (nType == 2) {
            m_bTrap = 1;
            m_cursor = 0;
        }
    }

    float fHighlightHeight = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("HighlightHeight"), bSuccess,
                                                   0.1f);
    if (fHighlightHeight > 0.0f)
        m_fHighlightHeight = fHighlightHeight;

    Vector vPos = {};
    int bPosLoaded = 0;
    vPos.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XPosition"), bPosLoaded, 0.0f);
    vPos.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YPosition"), bPosLoaded, 0.0f);
    vPos.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZPosition"), bPosLoaded, 0.0f);

    Vector vOri = {};
    int bOriLoaded = 0;
    vOri.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XOrientation"), bOriLoaded, 0.0f);
    vOri.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YOrientation"), bOriLoaded, 0.0f);
    vOri.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZOrientation"), bOriLoaded, 0.0f);
    if (vOri.x * vOri.x + vOri.y * vOri.y + vOri.z * vOri.z != 1.0f)
        vOri.Normalize();

    if (bOriLoaded == 1) {
        Quaternion qOld = YawPitchRoll(Yaw(m_orientation), 0.0f, 0.0f);
        Quaternion qNew = YawPitchRoll(Yaw(vOri), 0.0f, 0.0f);
        for (int64_t i = 0; i < m_nGeomVertCount; i++) {
            Vector v;
            v.x = m_pGeomVerts[i].x - m_position.x;
            v.y = m_pGeomVerts[i].y - m_position.y;
            v.z = m_pGeomVerts[i].z - m_position.z;
            // Un-rotate by the old facing; the conjugate negates w.
#if KOTOR1  // K1 negates the quaternion out of line
            Quaternion qInv = -qOld;
            if (quaternionidentityoptimization == 0 || (qInv.w != 1.0f && qInv.w != -1.0f)) {
                float w = qInv.w, x = qInv.x, y = qInv.y, z = qInv.z;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, yz = y * z;
                float wx = x * w, wy = y * w, wz = z * w;
                Vector r;
                r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
                r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
                r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
                v = r;
            }
#else
            if (quaternionidentityoptimization == 0 || (qOld.w != -1.0f && qOld.w != 1.0f)) {
                float x = qOld.x, y = qOld.y, z = qOld.z;
                float nw = -qOld.w;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, yz = y * z;
                float wx = x * nw, wy = y * nw, wz = z * nw;
                Vector r;
                r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
                r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
                r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
                v = r;
            }
#endif
            // Re-rotate by the new facing.
            if (quaternionidentityoptimization == 0 || !(qNew.w == 1.0f || qNew.w == -1.0f)) {
                float w = qNew.w, x = qNew.x, y = qNew.y, z = qNew.z;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, yz = y * z;
                float wx = x * w, wy = y * w, wz = z * w;
                Vector r;
                r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
                r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
                r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
                v = r;
            }
            Vector vOut;
            vOut.x = v.x + vPos.x;
            vOut.y = v.y + vPos.y;
            vOut.z = v.z + vPos.z;
            m_pGeomVerts[i] = vOut;
        }
    }
    if (bPosLoaded != 0)
        SetPosition(vPos, 1, 1, 0);
    if (bOriLoaded != 0)
        SetOrientation(vOri);

    CResList geometryList;
    if (pGFF->GetList(&geometryList, pStruct, const_cast<char*>("Geometry")))
        LoadTriggerGeometry(pGFF, geometryList);

    m_loadScreenId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("LoadScreenID"), bSuccess, 0);
    m_bSetByPlayerParty = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("SetByPlayerParty"), bSuccess,
                                               static_cast<unsigned char>(m_bSetByPlayerParty));
    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    return 1;
}

// Points are stored relative to the trigger position, so each is translated back into world space
// on the way in; the index array is the identity permutation.
void CSWSTrigger::LoadTriggerGeometry(CResGFF* pGFF, CResList& list)
{
    m_nGeomIndexCount = m_nGeomVertCount = pGFF->GetListCount(&list);
    if (m_pGeomVerts != 0) {
        delete[] m_pGeomVerts;
        m_pGeomVerts = 0;
    }
    if (m_pGeomIndices != 0) {
        delete[] m_pGeomIndices;
        m_pGeomIndices = 0;
    }
    m_pGeomVerts = new Vector[m_nGeomVertCount]();
    m_pGeomIndices = new int[m_nGeomVertCount];
    for (int64_t i = 0; i < m_nGeomVertCount; i++) {
        int bSuccess;
        CResStruct elem;
        if (pGFF->GetListElement(&elem, &list, static_cast<unsigned int>(i)) == 0) continue;
        if (pGFF->GetElementType(&elem) != 3) continue;
        float fX = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("PointX"), bSuccess, 0.0f);
        float fY = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("PointY"), bSuccess, 0.0f);
        float fZ = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("PointZ"), bSuccess, 0.0f);
        Vector v;
        v.x = fX + m_position.x;
        v.y = fY + m_position.y;
        v.z = fZ + m_position.z;
        m_pGeomVerts[i] = v;
        m_pGeomIndices[i] = static_cast<int>(i);
    }
}

// Pumps the server events OnUserDefined, OnEnter (which also drives area transitions and mine
// triggers), OnExit, OnDisarm and OnClick.
void CSWSTrigger::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                               unsigned int a, unsigned int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT))
    {
        if (pData != 0)
        {
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
        }
        return;
    }
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT))
    {
        if (m_bDestroyable != 0)
        {
            RemoveFromArea();
            delete this;
        }
        return;
    }
    if (nEventType != static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) return;

    CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
    switch (pEvent->m_eventType)
    {
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
        m_nUserDefinedEventNumber = pEvent->GetInteger(0);
#if KOTOR1  // K1 runs OnUserDefined with nCallerId as OBJECT_SELF
        g_pVirtualMachine->RunScript(&m_scriptSlots[3], nCallerId, 1);
#else
        g_pVirtualMachine->RunScript(&m_scriptSlots[3], m_objectId, 1);
#endif
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER: {  // Also the area-transition trigger
        int nParam = pEvent->GetInteger(0);
        m_oidEnteringObject = pEvent->GetObjectID(0);
        CSWSCreature* pEnterer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pEvent->GetObjectID(0));
        if (m_bAreaTransition != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pEnterer != 0 &&
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0) {
#if KOTOR1  // K1 calls CExoString::GetLength out of line
                if (pArea != 0 && m_linkedToModule.GetLength() > 0) {
#else
                if (pArea != 0 && m_linkedToModule.m_sString != 0 &&
                    static_cast<int>(strlen(m_linkedToModule.m_sString)) > 0) {
#endif
                    if (pArea->m_bTransitionPending != 0) {
                        // A transition is already armed (this is the re-fired event): the id must
                        // match and the traveller must still be alive.
                        if (pEvent->GetInteger(1) == pArea->m_activeTransitionID) {
                            if (pEnterer->GetDead() != 0 || pEnterer->GetDeadTemp() != 0) {
                                g_pAppManager->m_pClientExoApp->GetInGameGui()->StopGlobalFade();
                            } else {
                                CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
                                pServerExoApp->SetMoveToModulePending(1);
                                pServerExoApp->SetMoveToModuleString(m_linkedToModule);
                                pServerExoApp->SetMoveToModuleStartWaypoint(m_linkedTo);
                            }
                            pArea->SetTransitionPending(0, 0);
                        }
                    } else {
                        // Nothing armed yet: gate on who walked in, fade out, and re-fire this
                        // same event 500ms later to land in the arm above.
                        if (pEnterer->m_bPlayerCharacter != 0 ||
                            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(
                                pEnterer->m_objectId) != 0) {
#if KOTOR1  // K1 only requires the party to be together
                            if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyTogether() ==
                                0) {
#else
                            if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyHealthy() ==
                                0) {
#endif
                                // The party is in no state to travel: run the fail script instead.
                                CExoString sFailScript("k_trg_transfail");
                                g_pVirtualMachine->RunScript(&sFailScript, pEnterer->m_objectId, 1);
                            } else {
                                CScriptEvent* pTransition = new CScriptEvent();
                                unsigned char nPendingID = pArea->GetNextTransitionPendingID();
                                pTransition->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
                                pTransition->SetObjectID(0, pEvent->GetObjectID(0));
                                pTransition->SetInteger(0, pEvent->GetInteger(0));
                                pTransition->SetInteger(1, nPendingID);
                                g_pAppManager->m_pServerExoApp->GetServerAIMaster()
                                    ->AddEventDeltaTime(0, 500, m_objectId, m_objectId, nEventType,
                                                        pTransition);
                                CGuiInGame* pInGameGui =
                                    g_pAppManager->m_pClientExoApp->GetInGameGui();
                                Vector vFade;
                                pInGameGui->StartGlobalFade(0, 0.0f, 0.5f, vFade);
                                pArea->SetTransitionPending(1, nPendingID);
                                g_pAppManager->m_pServerExoApp->SetPauseState(2, 0);
                            }
                        }
                    }
                }
            } else if (pArea != 0) {
                pArea->SetTransitionPending(0, 0);
            }
        } else if (m_bTrap != 0) {
            OnEnterMine(nParam);
        } else {
            g_pVirtualMachine->RunScript(&m_scriptSlots[1], m_objectId, 1);
        }
        SetFadeOutTime(1);
        break;
    }
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT:
        m_oidExitingObject = pEvent->GetObjectID(0);
        g_pVirtualMachine->RunScript(&m_scriptSlots[2], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM:  // The disarmer comes from the event, not a parameter
        m_oidDisarmedBy = nCallerId;
        g_pVirtualMachine->RunScript(&m_scriptSlots[5], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED: {
        m_oidEnteringObject = pEvent->GetObjectID(0);
        // An unset or still-"default" script on a transition trigger falls back to the stock
        // transition script.
        if (m_scriptSlots[6].IsEmpty() || m_scriptSlots[6] == "default") {
            if (m_bAreaTransition == 1) {
                m_scriptSlots[6] = "NW_G0_Transition";
            }
        }
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
            pEvent->GetObjectID(0));
        if (pPlayer != 0 && m_loadScreenId != 0) {
            pPlayer->SetAreaTransitionBMP(m_loadScreenId, CExoString(""));
        }
        g_pVirtualMachine->RunScript(&m_scriptSlots[6], m_objectId, 1);
        break;
    }
    }
    delete pEvent;
}

// Writes the trigger back as a UTT: the seven script resrefs, every scalar and string field, the
// derived "Type", the portrait, position and orientation, then the geometry list with vertices
// stored relative to the trigger position.
int CSWSTrigger::SaveTrigger(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[0]), const_cast<char*>("ScriptHeartbeat"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[1]), const_cast<char*>("ScriptOnEnter"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[2]), const_cast<char*>("ScriptOnExit"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[3]), const_cast<char*>("ScriptUserDefine"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[4]), const_cast<char*>("OnTrapTriggered"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[5]), const_cast<char*>("OnDisarm"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[6]), const_cast<char*>("OnClick"));
    pGFF->WriteFieldBYTE(pStruct, m_trapType, const_cast<char*>("TrapType"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapOneShot), const_cast<char*>("TrapOneShot"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_creatorId), const_cast<char*>("CreatorId"));
    pGFF->WriteFieldCExoString(pStruct, m_linkedTo, const_cast<char*>("LinkedTo"));
    pGFF->WriteFieldBYTE(pStruct, m_linkedToFlags, const_cast<char*>("LinkedToFlags"));
    CResRef resLinkedToModule(static_cast<char*>(m_linkedToModule.CStr()));
    pGFF->WriteFieldCResRef(pStruct, resLinkedToModule, const_cast<char*>("LinkedToModule"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bAutoRemoveKey), const_cast<char*>("AutoRemoveKey"));
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCExoLocString(pStruct, m_localizedName, const_cast<char*>("LocalizedName"));
    pGFF->WriteFieldDWORD(pStruct, m_nFaction, const_cast<char*>("Faction"));
    pGFF->WriteFieldBYTE(pStruct, m_cursor, const_cast<char*>("Cursor"));
    pGFF->WriteFieldCExoString(pStruct, m_keyName, const_cast<char*>("KeyName"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDisarmable), const_cast<char*>("TrapDisarmable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDetectable), const_cast<char*>("TrapDetectable"));
#if !KOTOR1  // K1 has no OwnerDemolitionsSkill
    pGFF->WriteFieldINT(pStruct, m_nOwnerDemolitionsSkill, const_cast<char*>("OwnerDemolitionsSkill"));
#endif

    unsigned short portraitId = GetPortraitId();
    if (portraitId != PORTRAIT_ID_NONE) {
        pGFF->WriteFieldWORD(pStruct, GetPortraitId(), const_cast<char*>("PortraitId"));
    } else {
        pGFF->WriteFieldCResRef(pStruct, GetPortrait(), const_cast<char*>("Portrait"));
    }

    // "Type": 1 is an area transition, 2 a trap, 0 generic.
    int type;
    if (m_bAreaTransition != 0)
        type = 1;
    else if (m_bTrap != 0)
        type = 2;
    else
        type = 0;
    pGFF->WriteFieldINT(pStruct, type, const_cast<char*>("Type"));

    pGFF->WriteFieldFLOAT(pStruct, m_fHighlightHeight, const_cast<char*>("HighlightHeight"));
    float posX = m_position.x, posY = m_position.y, posZ = m_position.z;
    pGFF->WriteFieldFLOAT(pStruct, posX, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, posY, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, posZ, const_cast<char*>("ZPosition"));
    float oriX = m_orientation.x, oriY = m_orientation.y, oriZ = m_orientation.z;
    pGFF->WriteFieldFLOAT(pStruct, oriX, const_cast<char*>("XOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, oriY, const_cast<char*>("YOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, oriZ, const_cast<char*>("ZOrientation"));

    float gx = m_position.x;
    float gy = m_position.y;
    float gz = m_position.z;
    pGFF->AddList(&list, pStruct, const_cast<char*>("Geometry"));
    for (int64_t i = 0; i < m_nGeomVertCount; i++) {
        CResStruct elem;
        pGFF->AddListElement(&elem, &list, 3);
        float dx = m_pGeomVerts[i].x - gx;
        float dy = m_pGeomVerts[i].y - gy;
        float dz = m_pGeomVerts[i].z - gz;
        pGFF->WriteFieldFLOAT(&elem, dx, const_cast<char*>("PointX"));
        pGFF->WriteFieldFLOAT(&elem, dy, const_cast<char*>("PointY"));
        pGFF->WriteFieldFLOAT(&elem, dz, const_cast<char*>("PointZ"));
    }

    pGFF->WriteFieldWORD(pStruct, m_loadScreenId, const_cast<char*>("LoadScreenID"));
    pGFF->WriteFieldCExoLocString(pStruct, m_transitionDestination, const_cast<char*>("TransitionDestination"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bSetByPlayerParty), const_cast<char*>("SetByPlayerParty"));
    return 1;
}
