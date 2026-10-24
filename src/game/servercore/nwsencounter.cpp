#include "k2stdint.h"
#include "nwsencounter.h"
#include "nwsmodule.h"
#include "CExoArrayListImpl.h"
#include "nwsscriptevent.h"
#include "restype.h"


#include "port/libcmath.h"
#include "port/crtapi.h"

// The shared integer-CR points table, with one definition, zero-initialized, so there is no guard
// variable.
CExoArrayList<float> CSWSEncounter::m_sPowerLookup;
CSWSEncounter* CSWSEncounter::AsSWSEncounter() {
    return this;
}

CExoLocString& CSWSEncounter::GetFirstName() {
    return m_localizedName;
}

int CSWSEncounter::GetInActivateAreaList(unsigned long long objectId) {
    for (int64_t i = 0; i < m_nActivateAreaCount; i++) {
        if (m_pActivateAreaList[i] == objectId) return 1;
    }
    return 0;
}

void CSWSEncounter::RemoveFromActivateAreaList(unsigned long long objectId) {
    unsigned long long* pa = m_pActivateAreaList;
    int64_t i = 0;
    int removed = 0;
    for (; i < m_nActivateAreaCount; i++) {
        unsigned long long v = pa[i];
        if (v == objectId) {
            removed++;
        } else if (removed != 0) {
            pa[i - removed] = v;
        }
    }
    m_nActivateAreaCount -= removed;
}

// Adds the id if absent, growing the backing array by 16 slots when it is full.
void CSWSEncounter::AddToActivateAreaList(unsigned long long objectId) {
    int64_t count = m_nActivateAreaCount;
    unsigned long long* pArr = m_pActivateAreaList;
    for (int64_t i = 0; i < count; i++) {
        if (pArr[i] == objectId) return;
    }
    if (count >= m_nActivateAreaCapacity) {
        int newCap = m_nActivateAreaCapacity + 16;
        m_nActivateAreaCapacity = newCap;
        unsigned long long* pNew = new unsigned long long[newCap];
        // Copies count+1 elements, reading one slot past the full old array. The extra element is
        // overwritten below, but the over-read is real.
        int newCount = static_cast<int>(count) + 1;
#ifdef K2_FIX_ENGINE_BUGS
        K2_ASSUME(count < newCap);
        for (int i = 0; i < count; i++) {
#else
        for (int i = 0; i < newCount; i++) {
#endif
            pNew[i] = pArr[i];
        }
        m_nActivateAreaCount = newCount;
        pNew[count] = objectId;
        m_pActivateAreaList = pNew;
        delete[] pArr;
    } else {
        m_nActivateAreaCount = static_cast<int>(count + 1);
        pArr[count] = objectId;
    }
}

void CSWSEncounter::SetActive(int bActive) {
    m_bActive = bActive;
#if KOTOR1
    // K1 sizes the spawn pool from the hostiles around the entering creature first.
    SpawnIfAppropriate(OBJECT_INVALID, -1.0f);
#else
    if (bActive != 0 && m_bStarted != 1) {
        m_bShouldSpawn = 0;
        UpdateActivateAreaList(OBJECT_INVALID);
        if (m_bShouldSpawn == 1) {
            BuildSpawnList();
            m_bStarted = 1;
        }
    }
#endif
}

void CSWSEncounter::ClearSpawnList() {
    // Drained back to front, which avoids the list's own tail shift.
    for (int i = m_spawnList.m_count - 1; i >= 0; i--) {
        CEncounterListEntry* pEntry = m_spawnList.m_pArray[i];
        m_spawnList.DelIndex(i);
        delete pEntry;
    }
}

void CSWSEncounter::BuildSpawnList() {
    CEncounterListEntry* pTemp = new CEncounterListEntry[m_nMaxCreatures];

    int nSpawnCount = 0;
    BuildInitialSpawnList(pTemp, nSpawnCount);
#if KOTOR1
    // K1 splits picks into cheaper creatures when short of the recruit count, then pads the list out.
    if (nSpawnCount < m_nRecCreatures - m_nActiveCreatureCount && m_fSpawnPool > m_fMinSpawnPoints) {
        for (int i = nSpawnCount - 1; i >= 0; i--) {
            if (nSpawnCount >= m_nRecCreatures - m_nActiveCreatureCount)
                break;
            AddCreaturesToSpawnList(pTemp, nSpawnCount, i, pTemp[i]);
        }
    }
    if (nSpawnCount < m_nRecCreatures - m_nActiveCreatureCount)
        PadOutCreatureCount(pTemp, nSpawnCount);
    if (m_fSpawnPool < m_fMinSpawnPoints)
        m_fSpawnPool = 0.0f;
#endif

    // The early return leaks pTemp.
    if (nSpawnCount == 0)
        return;

    K2_ASSUME(nSpawnCount <= m_nMaxCreatures);
    for (int i = 0; i < nSpawnCount; i++) {
        CEncounterListEntry* pEntry = new CEncounterListEntry();
        pEntry->resRef = pTemp[i].resRef;
        pEntry->m_fCR = pTemp[i].m_fCR;

        m_spawnList.Add(pEntry);
    }

#if !KOTOR1
    if (m_nSpawnPointPositionCount > 0) {
        m_nSpawnsPerPoint = m_spawnList.m_count / m_nSpawnPointPositionCount;
    }
#endif

    delete[] pTemp;
}

// Picks the creature roster for one activation into the list, appending at nCount. The head-count is
// a draw over [RecCreatures, MaxCreatures], filled in three passes: one creature per SingleSpawn
// slot, then each other slot's GuaranteedCount, then the remainder drawn at random from a compacted
// pool of the non-SingleSpawn slots. Each pass stops at the head-count, so a table whose guaranteed
// counts overshoot truncates.
//
// Pass 1 arms the "already spawned" latch on the local copy rather than on the table entry.

void CSWSEncounter::BuildInitialSpawnList(CEncounterListEntry* pList, int& nCount)
{
#if KOTOR1
    // K1 spends the point pool down the CR-sorted table instead of drawing a head-count.
    float fPool = m_fSpawnPool - m_fSpawnPoolActive;
    CEncounterListEntry entry;
    int i = 0;
    if (i >= m_nSpawnPointCount)
        return;
    float fSlack = fPool / 10.0f;
    while (fPool > 0.0f) {
        int j = i;
        for (;;) {
            if (nCount >= m_nMaxCreatures - m_nActiveCreatureCount)
                return;
            entry = m_paSpawnPoints[j];
            if (entry.m_bSingleSpawn == 1 && entry.m_bAlreadyUsed == 0) {
                pList[nCount].resRef = entry.resRef;
                float fPoints = entry.m_fPoints;
                float fCR = entry.m_fCR;
                CEncounterListEntry* pOut = pList + nCount;
                nCount++;
                pOut->m_fPoints = fPoints;
                pOut->m_fCR = fCR;
                entry.m_bAlreadyUsed = 1;
            } else if (entry.m_fCR == 0.0f || entry.m_fPoints > fSlack + fPool) {
                // Free or unaffordable, so keep looking.
            } else {
                break;
            }
            j++;
            if (j >= m_nSpawnPointCount)
                return;
        }

        // Reads one slot past the table when j is the last slot.
#ifdef K2_FIX_ENGINE_BUGS
        if (j + 1 >= m_nSpawnPointCount || entry.m_fCR != m_paSpawnPoints[j + 1].m_fCR) {
#else
        if (entry.m_fCR != m_paSpawnPoints[j + 1].m_fCR) {
#endif
            while (entry.m_fPoints <= fSlack + fPool) {
                if (nCount >= m_nMaxCreatures - m_nActiveCreatureCount)
                    break;
                if (entry.m_bSingleSpawn != 0 && entry.m_bAlreadyUsed != 0)
                    break;
                pList[nCount].resRef = entry.resRef;
                CEncounterListEntry* pOut = pList + nCount;
                nCount++;
                pOut->m_fPoints = entry.m_fPoints;
                pOut->m_fCR = entry.m_fCR;
                fPool -= entry.m_fPoints;
                entry.m_bAlreadyUsed = 1;
            }
            i = j + 1;
        } else {
            entry.m_bAlreadyChecked = 0;
            int nGroup = 1;
            int nSingles = 1;
            int nLast = j;
            while (nLast + 1 < m_nSpawnPointCount
                   && m_paSpawnPoints[nLast + 1].m_fCR == m_paSpawnPoints[j].m_fCR) {
                nLast++;
                if (m_paSpawnPoints[nLast].m_bSingleSpawn != 0)
                    nSingles++;
                m_paSpawnPoints[nLast].m_bAlreadyChecked = 0;
                nGroup++;
            }
            int nPicked = 0;
#ifdef K2_FIX_ENGINE_BUGS
            int nPick = j;   // the group's first slot
#else
            int nPick = -1;  // sic: the first test reads the slot before the table
#endif
            while (m_paSpawnPoints[nPick].m_fPoints <= fSlack + fPool
                   && nCount < m_nMaxCreatures - m_nActiveCreatureCount) {
                nPick = j + rand_wincompatible() % nGroup;
                CEncounterSpawnPoint* pPoint = &m_paSpawnPoints[nPick];
                if (pPoint->m_bSingleSpawn == 0 || pPoint->m_bAlreadyUsed == 0) {
                    pList[nCount].resRef = pPoint->resRef;
                    CEncounterSpawnPoint* pSrc = &m_paSpawnPoints[nPick];
                    float fCR = pSrc->m_fCR;
                    float fPoints = pSrc->m_fPoints;
                    CEncounterListEntry* pOut = pList + nCount;
                    nCount++;
                    pOut->m_fCR = fCR;
                    pOut->m_fPoints = fPoints;
                    pSrc->m_bAlreadyUsed = 1;
                    fPool -= pSrc->m_fPoints;
                    pPoint = pSrc;
                }
                if (pPoint->m_bAlreadyChecked == 0) {
                    pPoint->m_bAlreadyChecked = 1;
                    nPicked++;
                }
                if (nSingles == nGroup && nPicked == nGroup)
                    break;
            }
            i = nLast + 1;
        }
        if (i >= m_nSpawnPointCount)
            return;
    }
#else
#ifdef K2_FIX_ENGINE_BUGS
    int nRoll = rand_wincompatible();
    int nRange = m_nMaxCreatures + 1 - m_nRecCreatures;
    int nToSpawn = nRange > 0 ? nRoll % nRange + m_nRecCreatures : m_nMaxCreatures;
#else
    int nToSpawn = rand_wincompatible() % (m_nMaxCreatures + 1 - m_nRecCreatures) + m_nRecCreatures;
#endif
    if (nToSpawn <= 0)
        return;

    CEncounterListEntry entry;
    bool bAllSingleSpawn = true;

    // Pass 1: one creature per SingleSpawn slot.
    for (int i = 0; i < m_nSpawnPointCount; i++) {
        entry = m_paSpawnPoints[i];
        bAllSingleSpawn &= (entry.m_bSingleSpawn != 0);
        if (entry.m_bSingleSpawn == 1 && entry.m_bAlreadyUsed == 0) {
            pList[nCount].resRef = entry.resRef;
            float fPoints = entry.m_fPoints;
            float fCR = entry.m_fCR;
            CEncounterListEntry* pOut = pList + nCount;
            nCount++;
            pOut->m_fPoints = fPoints;
            pOut->m_fCR = fCR;
            entry.m_bAlreadyUsed = 1;
            nToSpawn--;
            if (nToSpawn == 0)
                return;
        }
    }
    if (bAllSingleSpawn)
        return;

    // Pass 2: each non-SingleSpawn slot contributes its guaranteed count.
    for (int i = 0; i < m_nSpawnPointCount; i++) {
        entry = m_paSpawnPoints[i];
        if (entry.m_bSingleSpawn == 0) {
            for (int j = 0; j < entry.m_nGuaranteedCount; j++) {
                pList[nCount].resRef = entry.resRef;
                float fPoints = entry.m_fPoints;
                float fCR = entry.m_fCR;
                CEncounterListEntry* pOut = pList + nCount;
                nCount++;
                pOut->m_fPoints = fPoints;
                pOut->m_fCR = fCR;
                nToSpawn--;
                if (nToSpawn == 0)
                    return;
            }
        }
    }

    // Pass 3: compact the non-SingleSpawn slots into a pool and draw the remainder from it.
    CEncounterListEntry* pPool = new CEncounterListEntry[m_nSpawnPointCount];
    int nPool = 0;
    for (int i = 0; i < m_nSpawnPointCount; i++) {
        if (m_paSpawnPoints[i].m_bSingleSpawn == 0) {
            pPool[nPool] = m_paSpawnPoints[i];
            nPool++;
        }
    }
    while (nToSpawn > 0) {
        entry = pPool[rand_wincompatible() % nPool];
        pList[nCount].resRef = entry.resRef;
        float fPoints = entry.m_fPoints;
        float fCR = entry.m_fCR;
        CEncounterListEntry* pOut = pList + nCount;
        nCount++;
        pOut->m_fPoints = fPoints;
        pOut->m_fCR = fCR;
        nToSpawn--;
    }
    delete[] pPool;
#endif
}

// Builds the encounter object: chains the base, registers with the server AI master, stamps the world
// clock into the base's AI-tick slots, zeroes its runtime state, pre-allocates the activation-area id
// array, fills the integer-CR points table, loads the fractional-CR thresholds, arms the spawn
// machinery and parks the object at the origin with the default UTE settings.
CSWSEncounter::CSWSEncounter(unsigned long long objectId)
    : CSWSObject(GAME_OBJECT_TYPE_ENCOUNTER, objectId, 0)
{
    unsigned int nWorldDay;
    unsigned int nWorldTime;

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nWorldDay, &nWorldTime);
    m_nLastAIUpdateDay = nWorldDay;
    m_nLastAIUpdateTod = nWorldTime;

    m_nSpawnPointCount = 0;
    m_paSpawnPoints = 0;
    m_lastUpdateDay = 0;
    m_lastUpdateTod = 0;
    m_lastSpawnDay = 0;
    m_lastSpawnTime = 0;
    m_nActivateAreaCapacity = 16;
    m_pActivateAreaList = new unsigned long long[16];
    m_pGeomIndices = 0;
    m_nGeomIndexCount = 0;
    m_nGeomVertCount = 0;
    m_nActivateAreaCount = 0;
    m_bShouldSpawn = 0;
    m_pGeomVerts = 0;
    m_nSpawnPointPositionCount = 0;
    m_nEncounterLevel = 0;
    m_nCurrentRespawns = 0;
    m_nActiveCreatureCount = 0;
    m_fReserved1 = 0.0f;
    m_fMinSpawnPoints = 0.0f;
    m_fSpawnPool = 0.0f;
    m_fSpawnPoolActive = 0.0f;
    m_enteredObjectId = OBJECT_INVALID;
    m_exitObjectId = OBJECT_INVALID;
    m_paSpawnPointPositions = 0;
    m_bStarted = 0;

    // The table is file-scope, so every encounter constructed appends another 51 entries.
    for (int i = 0; i < 51; i++) {
        m_sPowerLookup.Add(static_cast<float>(pow(1.5, i)));
    }

    LoadFractionalCRData();

#if KOTOR1
    SetActive(1);
#else
    m_bActive = 1;
    SpawnIfAppropriate(OBJECT_INVALID, 0.0f);
#endif

    m_bReset = 0;
    m_nRespawnTime = 60;
    m_nSpawnOption = 0;
    m_nFaction = 1;
    m_nDifficulty = 0;

    Vector vOrigin;
    SetPosition(vOrigin, 1, 1, 0);

    m_nRecCreatures = 2;
    m_nMaxCreatures = 8;
    m_nMaxRespawns = 0;
    m_bPlayerOnly = 0;
#if !KOTOR1
    m_nSpawnedAtPoint = 0;
    m_nSpawnsPerPoint = 0;
    m_nCurrentSpawnPoint = 0;
#endif
}

// Encounters own their spawn points, geometry, activation list and threshold list.
CSWSEncounter::~CSWSEncounter() {
    RemoveFromArea();
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    }
    if (m_paSpawnPoints != 0) {
        delete[] m_paSpawnPoints;
        m_paSpawnPoints = 0;
    }
    if (m_pActivateAreaList != 0) {
        delete[] m_pActivateAreaList;
        m_pActivateAreaList = 0;
    }
    if (m_pGeomVerts != 0) {
        delete[] m_pGeomVerts;
        m_pGeomVerts = 0;
    }
    if (m_pGeomIndices != 0) {
        delete[] m_pGeomIndices;
        m_pGeomIndices = 0;
    }
    if (m_paSpawnPointPositions != 0) {
        delete[] m_paSpawnPointPositions;
        m_paSpawnPointPositions = 0;
    }
    ClearSpawnList();
    // Read once, because the delete below is opaque, so going back to the member could see a
    // different pointer.
    CExoArrayList<float>* pThresholds = m_pFractionalCRThresholds;
    if (pThresholds != 0) {
        delete pThresholds;
    }
}

// Reads the active flag as a gate rather than setting it, and forwards the object id to the
// activate-area scan. The float argument is unused.
void CSWSEncounter::SpawnIfAppropriate(unsigned long long objectId, float f) {
#if !KOTOR1
    K2_UNUSED(f);
#endif
    if (m_bActive != 0 && m_bStarted != 1) {
        m_bShouldSpawn = 0;
        UpdateActivateAreaList(objectId);
#if KOTOR1  // K1 sizes the pool from f and spawns nothing from an empty pool
        if (m_bShouldSpawn == 1) {
            CalculateSpawnPool(f);
            if (m_fSpawnPool > 0.0f) {
                BuildSpawnList();
                m_bStarted = 1;
            }
        }
#else
        if (m_bShouldSpawn == 1) {
            BuildSpawnList();
            m_bStarted = 1;
        }
#endif
    }
}

int CSWSEncounter::LineSegmentIntersectActivateArea(const Vector& p1, const Vector& p2) {
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

// Detaches from the area, clears the area pointer and parks the encounter at the origin.
void CSWSEncounter::RemoveFromArea() {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        pArea->m_objects.Remove(m_objectId);
    }
    SetArea(0);
    Vector v = {};
    SetPosition(v, 1, 1, 0);
}

// Attaches to the area: detaches from any current one, sets the new area and position, registers with
// the area, then appends the object id to its object list.
void CSWSEncounter::AddToArea(CSWSArea* pArea, float x, float y, float z, int n) {
    RemoveFromArea();
    if (pArea != 0) {
        SetArea(pArea);
        Vector v;
        v.x = x;
        v.y = y;
        v.z = z;
        SetPosition(v, 0, 1, 0);
        static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, n);
        CSWSArea* pA = static_cast<CSWSArea*>(GetArea());
        pA->m_objects.Add(m_objectId);
    }
}

// Decrements the live-creature tally. Once it reaches zero and this was not a count-1 "silent"
// removal, the encounter is exhausted: it schedules a signal event on the server AI master,
// optionally stamps the exhaustion time, clears each spawn point's spawned-creature id and resets the
// active and spawn-built flags.
void CSWSEncounter::RemoveFromActiveCreatureCount(float f, int a2) {
    int count = m_nActiveCreatureCount;
    if (count == 0) return;
#if KOTOR1
    // K1 returns the creature's points to the active pool.
    float fPoints = CalculatePointsFromCR(f);
    count = count - 1;
    m_nActiveCreatureCount = count;
    m_fSpawnPoolActive -= fPoints;
    if (m_fSpawnPoolActive < 0.0f)
        m_fSpawnPoolActive = 0.0f;
    if (a2 == 1) {
        m_fSpawnPool += CalculatePointsFromCR(f);
        return;
    }
#else
    K2_UNUSED(f);
    count = count - 1;
    m_nActiveCreatureCount = count;
    if (a2 == 1) return;
#endif
    if (count != 0) return;

    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_ENCOUNTER_EXHAUSTED;
    pEvent->SetObjectID(0, m_objectId);
    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    pAI->AddEventDeltaTime(0, 0, m_objectId, m_objectId,
                           CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);

    if (m_bReset == 1) {
        CWorldTimer* pTimer = g_pAppManager->m_pServerExoApp->GetWorldTimer();
        pTimer->GetWorldTime(&m_lastSpawnDay, &m_lastSpawnTime);
    }

    for (int i = 0; i < m_nSpawnPointCount; i++) {
        m_paSpawnPoints[i].m_bAlreadyUsed = 0;
        m_paSpawnPoints[i].m_bAlreadyChecked = 0;
    }

    m_bActive = 0;
    m_bStarted = 0;
}

// Tests whether the point is inside the activation-area polygon with a crossing-number ray cast
// along +y. For each edge whose x span the ray could cross, the edge's y at point.x is evaluated
// and counted when it is above point.y and point.x falls inside the edge's dominant-axis span; a
// vertex sitting exactly on the ray is special-cased. Returns the parity.
int CSWSEncounter::GetInActivateArea(const Vector& point) {
    int64_t n = m_nGeomIndexCount;
    int count = 0;
    if (n >= 1) {
        int* pIndices = m_pGeomIndices;
        Vector* pVerts = m_pGeomVerts;
        float px = point.x;
        float py = point.y;
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

// Refreshes the set of creatures inside the activation area and decides whether the encounter should
// spawn: first prunes entries whose creature has left the polygon, then scans the area's X-sorted
// object index across the bounding box, recording each live hostile creature and, unless the
// player-only flag gates it to a party member, flagging the should-spawn bit. The object id passed in
// is the creature that just moved and is always treated as in-box.
void CSWSEncounter::UpdateActivateAreaList(unsigned long long objectId) {
    CSWArea* pAreaBase = GetArea();
    if (pAreaBase == 0) return;
    CSWSArea* pArea = static_cast<CSWSArea*>(pAreaBase);

    // Drop any tracked creature that has since left the activation polygon, or vanished.
    for (int64_t i = 0; i < m_nActivateAreaCount; i++) {
        CSWSCreature* pCr =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_pActivateAreaList[i]);
        if (pCr != 0 && GetInActivateArea(pCr->m_position) == 0) {
            RemoveFromActivateAreaList(m_pActivateAreaList[i]);
        }
    }

    int idx;
    if (pArea->GetFirstObjectIndiceByX(&idx, m_fBBoxMinX) == 0) return;
    CExoArrayList<unsigned long long>& aObjects = pArea->m_aObjectsByX;
    while (idx < aObjects.m_count) {
        unsigned long long objId = aObjects[idx];
        CSWSCreature* pCr = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objId);
        if (pCr != 0) {
            Vector pos = pCr->m_position;
            if (pos.x > m_fBBoxMaxX) return;  // past the box in X; the index is X-sorted, so stop.
#if KOTOR1  // K1 also refuses to trigger on a dying player character
            if (pCr->GetAIStateReputation(m_objectId) == 2 && pCr->GetDead() == 0
                && pCr->GetIsPCDying() == 0) {
#else
            if (pCr->GetAIStateReputation(m_objectId) == 2 && pCr->GetDead() == 0) {
#endif
                if (objId == objectId ||
                    (pos.y >= m_fBBoxMinY && pos.y <= m_fBBoxMaxY && GetInActivateArea(pos))) {
                    if (GetInActivateAreaList(objId) == 0) AddToActivateAreaList(objId);
                    if (m_bPlayerOnly == 0 || pCr->m_pStats->m_bIsPC != 0) {
                        m_bShouldSpawn = 1;
                    } else {
                        CSWPartyTable* pt = g_pAppManager->m_pServerExoApp->GetPartyTable();
                        if (pt != 0 && pt->GetIsMember(objId) == 1) m_bShouldSpawn = 1;
                    }
                }
            }
        }
        idx++;
    }
}

// Pops one creature off the front of the spawn list and places it in the area, in three tiers:
//
// 1. round-robin over the spawn points looking for a safe location within 20 units
// 2. a non-visible location within 40 units of the encounter
// 3. give up and delete the creature
//
// Each tier ends in the same build-a-string-and-throw-it-away block, a log call whose message
// argument survived the release build. On success the creature joins the area, takes the AI level
// from the area's own gate, is turned along the spawn point's facing, and its points are billed
// against the pool.
void CSWSEncounter::SpawnCreatureFromSpawnList() {
    if (m_spawnList.m_count == 0)
        return;

    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return;

    Vector vSpawnPosition;
    Vector vOrientation;
    vOrientation.x = 1.0f;
    vOrientation.y = 0.0f;
    vOrientation.z = 0.0f;
    int bUseSpawnPoint = 0;
    if (m_nSpawnPointPositionCount > 0) {
        CSWSCreature* pTrigger =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_enteredObjectId);
#if KOTOR1
        // K1 takes the farthest spawn point and drops the computed facing, so the creature faces +x.
        if (pTrigger != 0) {
            int nFarthest = 0;
            float fFarthest = 0.0f;
            for (int i = 0; i < m_nSpawnPointPositionCount; i++) {
                Vector d = m_paSpawnPointPositions[i].m_vPos - pTrigger->m_position;
                float fDist2 = d.x * d.x + d.y * d.y + d.z * d.z;
                if (fDist2 > fFarthest * fFarthest) {
                    nFarthest = i;
                    fFarthest = sqrtf(fDist2);
                }
            }
            CEncounterSpawnPointPos* pPoint = &m_paSpawnPointPositions[nFarthest];
            vSpawnPosition = pPoint->m_vPos;
            float fAngle = 1.5707964f - pPoint->m_fOrientation;
            Vector vFacing;
            vFacing.x = cosf(fAngle);
            vFacing.y = sinf(fAngle);
            vFacing.z = 0.0f;
            normalize(vFacing);
        }
#else
        if (m_nSpawnsPerPoint == 0)
            m_nSpawnsPerPoint = m_spawnList.m_count / m_nSpawnPointPositionCount;
        if (m_nSpawnedAtPoint == m_nSpawnsPerPoint) {
            m_nCurrentSpawnPoint++;
            m_nSpawnedAtPoint = 0;
            if (m_nCurrentSpawnPoint >= m_nSpawnPointPositionCount)
                m_nCurrentSpawnPoint = 0;
        }
        if (pTrigger != 0) {
            CEncounterSpawnPointPos* pPoint = &m_paSpawnPointPositions[m_nCurrentSpawnPoint];
            vSpawnPosition = pPoint->m_vPos;
            float fAngle = 1.5707964f - pPoint->m_fOrientation;
            Vector vFacing;
            vFacing.x = cosf(fAngle);
            vFacing.y = sinf(fAngle);
            vFacing.z = 0.0f;
            vOrientation = normalize(vFacing);
        }
        m_nSpawnedAtPoint++;
#endif
        bUseSpawnPoint = 1;
    }

    Vector vSpawnLocation;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);
    if (pCreature->LoadFromTemplate(m_spawnList.m_pArray[0]->resRef, 0) == 0) {
        delete pCreature;
        return;
    }
    pCreature->m_oidEncounter = m_objectId;
    pCreature->m_bPlayerCharacter = 0;

    float fX = 0.0f;
    float fY = 0.0f;
    float fZ = 0.0f;
    int bPositionFound = 0;

    if (bUseSpawnPoint) {
        if (pArea->ComputeSafeLocation(vSpawnPosition, 20.0f, pCreature->m_pPathfindInfo, 0,
                                       &vSpawnLocation) == 1) {
            fX = vSpawnLocation.x;
            fY = vSpawnLocation.y;
            fZ = pArea->ComputeHeight(vSpawnLocation, 1, 0, 0);
            CExoString sResRef;
            m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
            sResRef.CStr();
            bPositionFound = 1;
        } else {
            CExoString sResRef;
            m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
            sResRef.CStr();
        }
    }

    if (bPositionFound == 0) {
        if (pArea->ComputeNonVisibleLocation(m_position, pCreature->m_pPathfindInfo, 0,
                                             &vSpawnLocation, 40.0f) == 1) {
            fX = vSpawnLocation.x;
            fY = vSpawnLocation.y;
            fZ = pArea->ComputeHeight(vSpawnLocation, 1, 0, 0);
            CExoString sResRef;
            m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
            sResRef.CStr();
        } else if (vSpawnLocation.x == -1.0f && vSpawnLocation.y == -1.0f &&
                   vSpawnLocation.z == -1.0f) {
            {
                CExoString sResRef;
                m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
                sResRef.CStr();
            }
            delete pCreature;
            {
                CExoString sResRef;
                m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
                sResRef.CStr();
            }
            return;
        } else {
            fY = vSpawnLocation.y;
            fX = vSpawnLocation.x;
            fZ = pArea->ComputeHeight(vSpawnLocation, 1, 0, 0);
            CExoString sResRef;
            m_spawnList.m_pArray[0]->resRef.CopyToString(sResRef);
            sResRef.CStr();
        }
    }

    pCreature->AddToArea(pArea, fX, fY, fZ, 0, 0);
    int nAILevel = pArea->m_nPlayersInArea > 0 ? 1 : 0;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pCreature, nAILevel);
    if (bUseSpawnPoint)
        pCreature->SetOrientation(vOrientation);

    m_nActiveCreatureCount++;
    float fPoints = m_spawnList.m_pArray[0]->m_fPoints;
    m_fSpawnPoolActive += fPoints;
    float fRemaining = m_fSpawnPool - fPoints;
    if (fRemaining < 0.0f)
        fRemaining = 0.0f;
    m_fSpawnPool = fRemaining;
    m_spawnList.DelIndex(0);
}

// Runs the per-frame AI tick, which spawns any pending spawn-list creatures, snapshots the world
// clock, then checks whether an exhausted encounter should re-arm and, while active, sends the
// periodic signal event and runs queued actions.
void CSWSEncounter::AIUpdate() {
    SpawnCreatureFromSpawnList();
    unsigned long long timerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();

    unsigned int day, tod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&day, &tod);

    unsigned int tickElapsedDay, tickElapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &tickElapsedDay, &tickElapsedTod);

    unsigned int hbElapsedDay, hbElapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_lastUpdateDay, m_lastUpdateTod, &hbElapsedDay, &hbElapsedTod);

    m_nLastAIUpdateDay = day;
    m_nLastAIUpdateTod = tod;
    m_nAIUpdateDeltaMs = tickElapsedTod;

    if (m_nSpawnOption == 1 && m_bReset == 1 && m_lastSpawnDay != 0 && m_lastSpawnTime != 0) {
        unsigned int exElapsedDay, exElapsedTod;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
            day, tod, m_lastSpawnDay, m_lastSpawnTime, &exElapsedDay, &exElapsedTod);
        if (exElapsedTod >= static_cast<unsigned int>(m_nRespawnTime * 1000) || exElapsedDay != 0) {
            if (m_nCurrentRespawns < m_nMaxRespawns || m_nMaxRespawns == -1) {
                m_nCurrentRespawns = m_nCurrentRespawns + 1;
                m_lastSpawnDay = 0;
                m_lastSpawnTime = 0;
#if KOTOR1
                SetActive(1);
#else
                m_bActive = 1;
                if (m_bStarted != 1) {
                    m_bShouldSpawn = 0;
                    UpdateActivateAreaList(OBJECT_INVALID);
                    if (m_bShouldSpawn == 1) {
                        BuildSpawnList();
                        m_bStarted = 1;
                    }
                }
#endif
                m_nSpawnPoolBits = 0;
                m_nSpawnPoolActiveBits = 0;
            }
        }
    }

    if (m_bActive != 0) {
        if ((hbElapsedTod >> 4) >= HEARTBEAT_INTERVAL_16MS) {
            if (m_lastUpdateTod != 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT;
                pEvent->SetObjectID(0, m_objectId);
                CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
                pAI->AddEventDeltaTime(0, 0, m_objectId, m_objectId,
                                       CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
            m_lastUpdateTod = tod;
            m_lastUpdateDay = day;
        }
        RunActions(day, tod, timerSnapshot);
    }
}

// The encounter's script-event dispatcher. Only the signal event is delivered here; the event's own
// sub-type selects which encounter script runs. OnEntered also refreshes the activate-area list and
// rebuilds the spawn list first, and OnExit clears the activate-area list. The event is destroyed on
// every path, including an unrecognised sub-type.
void CSWSEncounter::EventHandler(unsigned int eventType, unsigned long long a2, void* pEvent,
                                  unsigned int a4, unsigned int a5) {
    K2_UNUSED(a2); K2_UNUSED(a4); K2_UNUSED(a5);
    if (eventType != static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) return;
    CScriptEvent* pScriptEvent = static_cast<CScriptEvent*>(pEvent);
    switch (pScriptEvent->m_eventType) {
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
        g_pVirtualMachine->RunScript(&m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
        m_nUserDefinedEventNumber = pScriptEvent->GetInteger(0);
        g_pVirtualMachine->RunScript(&m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER: {
        m_enteredObjectId = pScriptEvent->GetObjectID(0);
        unsigned long long enteredId = pScriptEvent->GetObjectID(0);
#if KOTOR1
        SpawnIfAppropriate(enteredId, -1.0f);
#else
        if (m_bActive != 0 && m_bStarted != 1) {
            m_bShouldSpawn = 0;
            UpdateActivateAreaList(enteredId);
            if (m_bShouldSpawn == 1) {
                BuildSpawnList();
                m_bStarted = 1;
            }
        }
#endif
        g_pVirtualMachine->RunScript(&m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED], m_objectId, 1);
        break;
    }
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT:
        m_exitObjectId = pScriptEvent->GetObjectID(0);
        UpdateActivateAreaList(OBJECT_INVALID);
        g_pVirtualMachine->RunScript(&m_scripts[ENCOUNTER_SCRIPT_ON_EXIT], m_objectId, 1);
        break;
    case CSWSSCRIPTEVENT_EVENTTYPE_ON_ENCOUNTER_EXHAUSTED:  // (the sub-type RemoveFromActiveCreatureCount schedules on kill)
        g_pVirtualMachine->RunScript(&m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED], m_objectId, 1);
        break;
    default:
        break;
    }
    pScriptEvent->~CScriptEvent();
    operator delete(pScriptEvent);
}

// Reads the whole encounter out of a UTE GFF struct, the exact mirror of SaveEncounter in the same
// field order. bDefaultGeometry only matters when the template carries no Geometry list: the
// encounter then synthesises a 15x15 square centred on its own position.
//
// The AreaList loop reads element 3 every iteration and is bounded by the list length while the array
// is sized from the declared maximum, so a longer list overruns the allocation. The string loop below
// the creature sort computes nothing at all.
int CSWSEncounter::ReadEncounterFromGff(CResGFF* pGFF, CResStruct* pStruct, int bDefaultGeometry,
                                         Vector* pPosOverride)
{
    int bSuccess;
    m_bActive = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Active"), bSuccess, static_cast<unsigned char>(m_bActive));
    m_bReset = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Reset"), bSuccess, static_cast<unsigned char>(m_bReset));
    m_nRespawnTime = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ResetTime"), bSuccess, m_nRespawnTime);
    m_nMaxRespawns = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Respawns"), bSuccess, m_nMaxRespawns);
    m_nSpawnOption = pGFF->ReadFieldINT(pStruct, const_cast<char*>("SpawnOption"), bSuccess, m_nSpawnOption);
    m_nMaxCreatures = pGFF->ReadFieldINT(pStruct, const_cast<char*>("MaxCreatures"), bSuccess, m_nMaxCreatures);
    m_nRecCreatures = pGFF->ReadFieldINT(pStruct, const_cast<char*>("RecCreatures"), bSuccess, m_nRecCreatures);
    m_bPlayerOnly = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PlayerOnly"), bSuccess, static_cast<unsigned char>(m_bPlayerOnly));
    m_nFaction = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Faction"), bSuccess, m_nFaction);
    m_nDifficultyIndex = pGFF->ReadFieldINT(pStruct, const_cast<char*>("DifficultyIndex"), bSuccess, -1);
    // The 2DA is the authority when it is loaded; the GFF's own "Difficulty" is only the fallback.
    if (g_pRules->m_p2DArrays->m_pEncounterDifficulty != 0) {
        g_pRules->m_p2DArrays->m_pEncounterDifficulty->GetINTEntry(m_nDifficultyIndex,
                                                                   CExoString("VALUE"),
                                                                   &m_nDifficulty);
    } else {
        m_nDifficulty = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Difficulty"), bSuccess, m_nDifficulty);
    }

    Vector vPos = {};
    vPos.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XPosition"), bSuccess, m_position.x);
    vPos.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YPosition"), bSuccess, m_position.y);
    vPos.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZPosition"), bSuccess, m_position.z);
    SetPosition(vPos, 1, 1, 0);
    if (pPosOverride != 0)
        SetPosition(*pPosOverride, 1, 1, 0);

    m_localizedName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocalizedName"), bSuccess,
                                                    m_localizedName);
    m_sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, m_sTag).LowerCase();
    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);

    CResList geometryList;
    if (pGFF->GetList(&geometryList, pStruct, const_cast<char*>("Geometry"))) {
        if (LoadEncounterGeometry(pGFF, geometryList) == 0)
            return 0;
    } else if (bDefaultGeometry != 0) {
        m_nGeomVertCount = 4;
        m_nGeomIndexCount = 4;
        if (m_pGeomVerts != 0)
            delete[] m_pGeomVerts;
        m_pGeomVerts = new Vector[m_nGeomVertCount]();
        if (m_pGeomIndices != 0)
            delete[] m_pGeomIndices;
        m_pGeomIndices = new int[m_nGeomIndexCount];
        m_fBBoxMinX = m_position.x - 7.5f;
        m_fBBoxMaxX = m_position.x + 7.5f;
        m_fBBoxMinY = m_position.y - 7.5f;
        m_fBBoxMaxY = m_position.y + 7.5f;
        float fZ = m_position.z;
        m_pGeomVerts[0].x = m_fBBoxMinX;
        m_pGeomVerts[0].y = m_fBBoxMinY;
        m_pGeomVerts[0].z = fZ;
        m_pGeomIndices[0] = 0;
        m_pGeomVerts[1].x = m_fBBoxMaxX;
        m_pGeomVerts[1].y = m_fBBoxMinY;
        m_pGeomVerts[1].z = fZ;
        m_pGeomIndices[1] = 1;
        m_pGeomVerts[2].x = m_fBBoxMaxX;
        m_pGeomVerts[2].y = m_fBBoxMaxY;
        m_pGeomVerts[2].z = fZ;
        m_pGeomIndices[2] = 2;
        m_pGeomVerts[3].x = m_fBBoxMinX;
        m_pGeomVerts[3].y = m_fBBoxMaxY;
        m_pGeomVerts[3].z = fZ;
        m_pGeomIndices[3] = 3;
    }

    CResList creatureList;
    CResStruct creatureElem;
    if (pGFF->GetList(&creatureList, pStruct, const_cast<char*>("CreatureList"))) {
        m_nSpawnPointCount = pGFF->GetListCount(&creatureList);
        if (m_paSpawnPoints != 0)
            delete[] m_paSpawnPoints;
        m_paSpawnPoints = new CEncounterSpawnPoint[m_nSpawnPointCount];
        for (int i = 0; i < m_nSpawnPointCount; i++) {
            if (pGFF->GetListElement(&creatureElem, &creatureList, i)) {
                m_paSpawnPoints[i].resRef =
                    pGFF->ReadFieldCResRef(&creatureElem, const_cast<char*>("ResRef"), bSuccess, CResRef(""));
                m_paSpawnPoints[i].m_fCR =
                    pGFF->ReadFieldFLOAT(&creatureElem, const_cast<char*>("CR"), bSuccess, 0.0f);
#if KOTOR1  // K1 derives each slot's points from its CR and has no GuaranteedCount
                m_paSpawnPoints[i].m_fPoints = CalculatePointsFromCR(m_paSpawnPoints[i].m_fCR);
                m_paSpawnPoints[i].m_bSingleSpawn =
                    pGFF->ReadFieldBYTE(&creatureElem, const_cast<char*>("SingleSpawn"), bSuccess, 0);
                m_paSpawnPoints[i].m_bAlreadyUsed = 0;
#else
                m_paSpawnPoints[i].m_fPoints = 0.0f;
                m_paSpawnPoints[i].m_bSingleSpawn =
                    pGFF->ReadFieldBYTE(&creatureElem, const_cast<char*>("SingleSpawn"), bSuccess, 0);
                m_paSpawnPoints[i].m_bAlreadyUsed = 0;
                m_paSpawnPoints[i].m_nGuaranteedCount =
                    pGFF->ReadFieldINT(&creatureElem, const_cast<char*>("GuaranteedCount"), bSuccess, 0);
#endif
            }
        }
    }

    CEncounterSpawnPoint swapTemp;
    for (int i = 0; i < m_nSpawnPointCount; i++) {
        for (int j = 0; j < m_nSpawnPointCount - 1; j++) {
            if (m_paSpawnPoints[j].m_fCR < m_paSpawnPoints[j + 1].m_fCR) {
                swapTemp = m_paSpawnPoints[j];
                m_paSpawnPoints[j] = m_paSpawnPoints[j + 1];
                m_paSpawnPoints[j + 1] = swapTemp;
            }
        }
    }

    CExoString sResRefText;
    for (int64_t i = 0; i < m_nSpawnPointCount; i++) {
        m_paSpawnPoints[i].resRef.CopyToString(sResRefText);
        sResRefText.CStr();
    }
    if (m_nSpawnPointCount > 0)
        m_fMinSpawnPoints = m_paSpawnPoints[m_nSpawnPointCount - 1].m_fPoints;

    CResList spawnPointList;
    if (pGFF->GetList(&spawnPointList, pStruct, const_cast<char*>("SpawnPointList")))
        LoadEncounterSpawnPoints(pGFF, spawnPointList);

    m_nActiveCreatureCount = pGFF->ReadFieldINT(pStruct, const_cast<char*>("NumberSpawned"), bSuccess,
                                                 m_nActiveCreatureCount);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_nActiveCreatureCount < 0)
        m_nActiveCreatureCount = 0;
#endif
    m_lastUpdateDay = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("HeartbeatDay"), bSuccess, m_lastUpdateDay);
    m_lastUpdateTod = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("HeartbeatTime"), bSuccess, m_lastUpdateTod);
    m_lastSpawnDay = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("LastSpawnDay"), bSuccess, m_lastSpawnDay);
    m_lastSpawnTime = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("LastSpawnTime"), bSuccess, m_lastSpawnTime);
    m_enteredObjectId = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("LastEntered"), bSuccess,
                                              static_cast<unsigned int>(m_enteredObjectId));
    m_exitObjectId = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("LastLeft"), bSuccess, static_cast<unsigned int>(m_exitObjectId));
    m_bStarted = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Started"), bSuccess, static_cast<unsigned char>(m_bStarted));
    m_bExhausted = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Exhausted"), bSuccess, static_cast<unsigned char>(m_bExhausted));
    m_nCurrentRespawns = pGFF->ReadFieldINT(pStruct, const_cast<char*>("CurrentSpawns"), bSuccess,
                                             m_nCurrentRespawns);
    m_nUserDefinedEventNumber = pGFF->ReadFieldINT(pStruct, const_cast<char*>("CustomScriptId"), bSuccess,
                                                    m_nUserDefinedEventNumber);
    m_nActivateAreaCapacity = pGFF->ReadFieldINT(pStruct, const_cast<char*>("AreaListMaxSize"), bSuccess,
                                                  m_nActivateAreaCapacity);
    m_fSpawnPoolActive = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("SpawnPoolActive"), bSuccess,
                                               m_fSpawnPoolActive);
    m_fAreaPoints = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("AreaPoints"), bSuccess, m_fAreaPoints);

    CResList areaList;
    CResStruct areaElem;
    if (pGFF->GetList(&areaList, pStruct, const_cast<char*>("AreaList"))) {
        m_nActivateAreaCount = pGFF->GetListCount(&areaList);
        if (m_nActivateAreaCount > 0) {
            if (m_pActivateAreaList != 0)
                delete[] m_pActivateAreaList;
#ifdef K2_FIX_ENGINE_BUGS
            if (m_nActivateAreaCapacity < m_nActivateAreaCount)
                m_nActivateAreaCapacity = m_nActivateAreaCount;
#endif
            m_pActivateAreaList = new unsigned long long[m_nActivateAreaCapacity];
            for (int i = 0; i < m_nActivateAreaCount; i++) {
                // Index 3, not i.
#ifdef K2_FIX_ENGINE_BUGS
                K2_ASSUME(i < m_nActivateAreaCapacity);
                if (pGFF->GetListElement(&areaElem, &areaList, i)) {
#else
                if (pGFF->GetListElement(&areaElem, &areaList, 3)) {
#endif
                    m_pActivateAreaList[i] =
                        pGFF->ReadFieldDWORD(&areaElem, const_cast<char*>("AreaObject"), bSuccess, 0);
                }
            }
        }
    }

    CResList spawnList;
    CResStruct spawnElem;
    if (pGFF->GetList(&spawnList, pStruct, const_cast<char*>("SpawnList"))) {
        if (m_spawnList.m_count > 0)
            ClearSpawnList();
        int nSpawns = pGFF->GetListCount(&spawnList);
        for (int i = 0; i < nSpawns; i++) {
            if (pGFF->GetListElement(&spawnElem, &spawnList, i)) {
                CEncounterListEntry* pEntry = new CEncounterListEntry();
                pEntry->resRef = pGFF->ReadFieldCResRef(&spawnElem, const_cast<char*>("SpawnResRef"), bSuccess,
                                                        CResRef(""));
                pEntry->m_fCR = pGFF->ReadFieldFLOAT(&spawnElem, const_cast<char*>("SpawnCR"), bSuccess, 0.0f);
                m_spawnList.Add(pEntry);
            }
        }
    }
    return 1;
}

int CSWSEncounter::LoadFromTemplate(const CResRef& resRef, int m, Vector* pVec) {
    CResStruct topStruct;
    CResGFF* pGff = new CResGFF(RESTYPE_UTE, const_cast<char*>("UTE "), resRef);
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        if (m != 0)
            ReadEncounterFromGff(pGff, &topStruct, m, pVec);
        else
            ReadEncounterFromGff(pGff, &topStruct, 0, 0);
        ReadEncounterScriptsFromGff(pGff, &topStruct);
        delete pGff;
        return 1;
    } else {
        delete pGff;
        CExoString errStr;
        CExoString resStr;
        resRef.CopyToString(resStr);
        errStr.Format(const_cast<char*>("Encounter template %s doesn't exist.\n"), resStr.CStr());
        return 0;
    }
}

float CSWSEncounter::CalculatePointsFromCR(float cr) {
    if (cr > 0.0f && cr < 1.0f) {
        float* pThresh = m_pFractionalCRThresholds->m_pArray;
        if (pThresh[0] <= cr) return 1.0f;
        if (pThresh[1] <= cr) return 0.6666667f;
        if (pThresh[2] <= cr) return 0.44444445f;
        if (pThresh[3] <= cr) return 0.2962963f;
        return 0.19753087f;
    }
    if (cr >= 0.0f && cr <= 50.0f) {
        return m_sPowerLookup.m_pArray[static_cast<int>(cr)];
    }
    return powf(1.5f, cr);
}

void CSWSEncounter::SetDifficulty(int difficulty) {
    float outVal = 0;
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pEncounterDifficulty;
    int result;
    if (p2DA != 0) {
        if (difficulty < 0) return;
        if (p2DA->m_nRows <= difficulty) return;
        {
            CExoString colName("VALUE");
            p2DA->GetFLOATEntry(difficulty, colName, &outVal);
        }
        result = static_cast<int>(outVal);
    } else {
        result = 0;
    }
    m_nDifficulty = result;
}

void CSWSEncounter::LoadFractionalCRData() {
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pFractionalCR;
    if (p2DA == 0) return;

    CExoArrayList<float>* pList = new CExoArrayList<float>;
    m_pFractionalCRThresholds = pList;

    for (int i = 0; i < 5; i++) {
        float value;
        {
            CExoString colName("Min");
            p2DA->GetFLOATEntry(i, colName, &value);
        }
        float v = value;
        pList = m_pFractionalCRThresholds;
        int count = pList->m_count;
        if (count == pList->m_capacity) {
            int newCap = count == 0 ? 16 : count * 2;
            float* pOld = pList->m_pArray;
            pList->m_capacity = newCap;
            float* pNew = new float[newCap];
            K2_ASSUME(count < newCap);
            pList->m_pArray = pNew;
            for (int64_t j = 0; j < count; j++) {
                pNew[j] = pOld[j];
            }
            delete[] pOld;
            count = pList->m_count;
        }
        float* pData = pList->m_pArray;
        pList->m_count = count + 1;
        pData[count] = v;
    }
}

void CSWSEncounter::TallyEnemyRadiusPoints() {
#if KOTOR1
    // K1 counts the points of live hostiles within 30 units, non-PCs at half level.
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return;
    m_fAreaPoints = 0.0f;
    CSWSCreature* pEntered =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_enteredObjectId);
    if (pEntered == 0)
        return;
    float fX = pEntered->m_position.x;
    float fY = pEntered->m_position.y;
    int idx;
    if (pArea->GetFirstObjectIndiceByX(&idx, fX - 30.0f) == 0)
        return;
    for (; idx < pArea->m_aObjectsByX.m_count; idx++) {
        unsigned long long id = pArea->m_aObjectsByX[idx];
        CSWSCreature* pCr = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
        if (pCr == 0)
            continue;
        if (pCr->m_position.x > fX + 30.0f)
            return;
        float y = pCr->m_position.y;
        if (pCr->GetAIStateReputation(m_objectId) == 2 && pCr->GetDead() != 1
            && pCr->GetIsPCDying() != 1 && y >= fY - 30.0f && y <= fY + 30.0f) {
            int nLevel = pCr->m_pStats->GetLevel(0);
            if (pCr->m_bPlayerCharacter == 0)
                nLevel = nLevel / 2;
            m_fAreaPoints += CalculatePointsFromCR(static_cast<float>(nLevel));
        }
    }
#endif
}

void CSWSEncounter::CalculateSpawnPool(float f) {
#if !KOTOR1
    K2_UNUSED(f);
#endif
#if KOTOR1
    // K1: an argument of -1 tallies the hostiles nearby; the pool grows by the encounter level's
    // points.
    if (f == -1.0f)
        TallyEnemyRadiusPoints();
    else
        m_fAreaPoints = CalculatePointsFromCR(f);
    if (m_fAreaPoints == 0.0f)
        return;
    int nLevel = static_cast<int>(log(m_fAreaPoints / 4.0) / log(1.5) + 0.5);
    m_nEncounterLevel = m_nDifficulty + nLevel;
    m_fSpawnPool += CalculatePointsFromCR(static_cast<float>(m_nEncounterLevel - 1));
#endif
}

void CSWSEncounter::AddCreaturesToSpawnList(CEncounterListEntry* pEntry, int& n, int i,
                                            CEncounterListEntry entry2) {
#if !KOTOR1
    K2_UNUSED(pEntry); K2_UNUSED(n); K2_UNUSED(i); K2_UNUSED(entry2);
#endif
#if KOTOR1
    // K1 replaces the entry with strictly cheaper creatures worth up to its points.
    float fPoints = CalculatePointsFromCR(entry2.m_fCR);
    if (fPoints == m_fMinSpawnPoints)
        return;
    if (n == 1) {
        n = 0;
    } else {
        int nRemoved = 0;
        for (int k = 0; k < n; k++) {
            if (k == i) {
                nRemoved++;
            } else if (nRemoved != 0) {
                pEntry[k - nRemoved] = pEntry[k];
            }
        }
        n -= nRemoved;
    }
    float fLimit = fPoints - 0.001f;
    for (int k = 0; k < m_nSpawnPointCount; k++) {
        if (n > m_nRecCreatures - m_nActiveCreatureCount)
            return;
        while (m_paSpawnPoints[k].m_fPoints <= fPoints && m_paSpawnPoints[k].m_fPoints <= fLimit) {
            if (n >= m_nMaxCreatures)
                break;
            if (m_paSpawnPoints[k].m_bSingleSpawn != 0 && m_paSpawnPoints[k].m_bAlreadyUsed != 0)
                break;
            pEntry[n].resRef = m_paSpawnPoints[k].resRef;
            CEncounterSpawnPoint* pPoint = &m_paSpawnPoints[k];
            float fCR = pPoint->m_fCR;
            float fPointPoints = pPoint->m_fPoints;
            pEntry[n].m_fCR = fCR;
            pEntry[n].m_fPoints = fPointPoints;
            pPoint->m_bAlreadyUsed = 1;
            n++;
            fPoints -= pPoint->m_fPoints;
        }
    }
#endif
}

// A debug dump of a spawn list, compiled out in the shipping build except for the string work its
// removed print call still had to evaluate.
void PrintCreatureList(CEncounterListEntry* pList, int nCount)
{
    CExoString sResRef;

    for (int i = 0; i < nCount; i++)
    {
        pList[i].resRef.CopyToString(sResRef);
        sResRef.CStr();
    }
}

void CSWSEncounter::PadOutCreatureCount(CEncounterListEntry* pEntry, int& n) {
#if !KOTOR1
    K2_UNUSED(pEntry); K2_UNUSED(n);
#endif
#if KOTOR1
    // K1 tops the list up to the recruit count, at random within each point band, cheapest first.
    int nMissing = m_nRecCreatures - m_nActiveCreatureCount - n;
    float fLastPoints = 0.0f;
    while (nMissing > 0) {
        int k;
        float fPoints = 0.0f;
        for (k = m_nSpawnPointCount - 1; k >= 0; k--) {
            CExoString sResRef;
            m_paSpawnPoints[k].resRef.CopyToString(sResRef);
            fPoints = m_paSpawnPoints[k].m_fPoints;
            if (fLastPoints < fPoints
                && (m_paSpawnPoints[k].m_bSingleSpawn == 0 || m_paSpawnPoints[k].m_bAlreadyUsed == 0))
                break;
        }
        if (k < 0)
            return;

        int nFirst = 0;
        int nLast = 0;
        int nSingles = 0;
        int bFirst = 1;
        for (int j = 0; j < m_nSpawnPointCount; j++) {
            if (m_paSpawnPoints[j].m_fPoints == fPoints) {
                if (bFirst == 1) {
                    nFirst = j;
                    bFirst = 0;
                }
                nLast = j;
                if (m_paSpawnPoints[j].m_bSingleSpawn == 1)
                    nSingles++;
                m_paSpawnPoints[j].m_bAlreadyChecked = 0;
            }
        }
        int nRange = nLast - nFirst + 1;
        int nPicked = 0;
        while (nMissing > 0) {
            int nPick = nFirst + rand_wincompatible() % nRange;
            CEncounterSpawnPoint* pPoint = &m_paSpawnPoints[nPick];
            if (pPoint->m_bSingleSpawn == 0 || pPoint->m_bAlreadyUsed == 0) {
#ifdef K2_FIX_ENGINE_BUGS
                if (n >= m_nMaxCreatures)
                    return;
#endif
                pEntry[n].resRef = pPoint->resRef;
                CEncounterSpawnPoint* pSrc = &m_paSpawnPoints[nPick];
                float fCR = pSrc->m_fCR;
                float fPointPoints = pSrc->m_fPoints;
                CEncounterListEntry* pOut = pEntry + n;
                n++;
                pOut->m_fCR = fCR;
                pOut->m_fPoints = fPointPoints;
                pSrc->m_bAlreadyUsed = 1;
                nMissing--;
                pPoint = pSrc;
            }
            if (pPoint->m_bAlreadyChecked == 0) {
                pPoint->m_bAlreadyChecked = 1;
                nPicked++;
            }
            if (nRange == nSingles && nPicked == nRange)
                break;
        }
        fLastPoints = fPoints;
    }
#endif
}

// Layout-identical, trivially-default-constructible mirror of the real spawn-point position type.
// Allocating the real type would run its default constructor, so the array allocation goes through
// this mirror while the pointer is stored as the real type, keeping every access on it. Both are
// trivially destructible and the same size, so `delete[]` reaches the same deallocation with the same
// pointer.
struct CEncounterSpawnPointPosPod {
    float m_aFloats[4];
};

// Loads the SpawnPointList, one spawn-point position per element.
void CSWSEncounter::LoadEncounterSpawnPoints(CResGFF* pGFF, CResList& resList) {
    m_nSpawnPointPositionCount = pGFF->GetListCount(&resList);
    if (m_nSpawnPointPositionCount < 1)
        return;
    delete[] m_paSpawnPointPositions;
    CEncounterSpawnPointPosPod* pNew = new CEncounterSpawnPointPosPod[m_nSpawnPointPositionCount];
    if (m_nSpawnPointPositionCount != 0)
        __builtin_memset(pNew, 0, m_nSpawnPointPositionCount * sizeof(CEncounterSpawnPointPosPod));
    m_paSpawnPointPositions = reinterpret_cast<CEncounterSpawnPointPos*>(pNew);
    CResStruct elem;
    int found;
    for (int i = 0; i < m_nSpawnPointPositionCount; i++) {
        if (pGFF->GetListElement(&elem, &resList, i)) {
            m_paSpawnPointPositions[i].m_vPos.x = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("X"), found, 0.0f);
            m_paSpawnPointPositions[i].m_vPos.y = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("Y"), found, 0.0f);
            m_paSpawnPointPositions[i].m_vPos.z = pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("Z"), found, 0.0f);
            m_paSpawnPointPositions[i].m_fOrientation =
                pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("Orientation"), found, 0.0f);
        }
    }
}

// Loads the activation-area polygon from the Geometry list: one vertex per element, each stored as the
// encounter's position plus the read offset, a parallel edge-index array, and the polygon's XY
// bounding box. An empty list returns 0.
int CSWSEncounter::LoadEncounterGeometry(CResGFF* pGFF, CResList& resList) {
    CResStruct elem;
    int found;
    m_nGeomVertCount = pGFF->GetListCount(&resList);
    m_nGeomIndexCount = m_nGeomVertCount;
    delete[] m_pGeomVerts;
    if (m_nGeomVertCount == 0) {
        CExoString msg;
        msg.Format(const_cast<char*>("Problem loading encounter with tag '%s'.  It has geometry, but no "
                          "vertices.  Skipping."),
                   m_sTag.CStr());
        return 0;
    }
    m_pGeomVerts = new Vector[m_nGeomVertCount]();
    delete[] m_pGeomIndices;
    m_pGeomIndices = new int[m_nGeomIndexCount];
    int first = 1;
    for (int i = 0; i < m_nGeomVertCount; i++) {
        if (pGFF->GetListElement(&elem, &resList, i)) {
            m_pGeomVerts[i].x = m_position.x + pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("X"), found, 0.0f);
            m_pGeomVerts[i].y = m_position.y + pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("Y"), found, 0.0f);
            m_pGeomVerts[i].z = m_position.z + pGFF->ReadFieldFLOAT(&elem, const_cast<char*>("Z"), found, 0.0f);
            m_pGeomIndices[i] = i;
            float vx = m_pGeomVerts[i].x;
            if (first) {
                m_fBBoxMinX = vx;
                m_fBBoxMaxX = vx;
                m_fBBoxMinY = m_pGeomVerts[i].y;
                m_fBBoxMaxY = m_pGeomVerts[i].y;
            } else {
                if (vx < m_fBBoxMinX) m_fBBoxMinX = vx;
                if (vx > m_fBBoxMaxX) m_fBBoxMaxX = vx;
                float vy = m_pGeomVerts[i].y;
                if (vy < m_fBBoxMinY) m_fBBoxMinY = vy;
                if (vy > m_fBBoxMaxY) m_fBBoxMaxY = vy;
            }
            first = 0;
        }
    }
    return 1;
}

// Reads the five encounter script resrefs.
void CSWSEncounter::ReadEncounterScriptsFromGff(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;
#ifdef K2_FIX_ENGINE_BUGS
    m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnEntered"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED])).GetResRefStr());
    m_scripts[ENCOUNTER_SCRIPT_ON_EXIT] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnExit"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXIT])).GetResRefStr());
    m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnHeartbeat"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT])).GetResRefStr());
    m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnExhausted"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED])).GetResRefStr());
    m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUserDefined"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED])).GetResRefStr());
#else
    m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnEntered"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED])).GetResRef());
    m_scripts[ENCOUNTER_SCRIPT_ON_EXIT] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnExit"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXIT])).GetResRef());
    m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnHeartbeat"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT])).GetResRef());
    m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnExhausted"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED])).GetResRef());
    m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED] = CExoString(
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUserDefined"), bSuccess, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED])).GetResRef());
#endif
}

// Loads the template at the top level, reading the encounter body and its scripts. A missing
// template returns 0.
int CSWSEncounter::LoadEncounter(CResGFF* pGFF, CResStruct* pStruct)
{
    if (!ReadEncounterFromGff(pGFF, pStruct, 0, 0))
        return 0;
    ReadEncounterScriptsFromGff(pGFF, pStruct);

    CEncounterSpawnPoint temp;

    for (int i = 0; i < m_nSpawnPointCount; i++) {
        for (int j = 0; j < m_nSpawnPointCount - 1; j++) {
            if (m_paSpawnPoints[j].m_fCR < m_paSpawnPoints[j + 1].m_fCR) {
                temp = m_paSpawnPoints[j];
                m_paSpawnPoints[j] = m_paSpawnPoints[j + 1];
                m_paSpawnPoints[j + 1] = temp;
            }
        }
    }

    CExoString sTmp;
    for (int64_t i = 0; i < m_nSpawnPointCount; i++) {
        m_paSpawnPoints[i].resRef.CopyToString(sTmp);
        sTmp.CStr();
    }
    int last = m_nSpawnPointCount - 1;
    if (last >= 0)
        m_fMinSpawnPoints = m_paSpawnPoints[last].m_fPoints;
    return 1;
}

// Top-level writer, the mirror of the read side: each scalar field with its GFF type, the localized
// name, the tag, the five script resrefs, then the five sub-lists in the order the reader expects.
// Always returns 1.
int CSWSEncounter::SaveEncounter(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bActive), const_cast<char*>("Active"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bReset), const_cast<char*>("Reset"));
    pGFF->WriteFieldINT(pStruct, m_nRespawnTime, const_cast<char*>("ResetTime"));
    pGFF->WriteFieldINT(pStruct, m_nMaxRespawns, const_cast<char*>("Respawns"));
    pGFF->WriteFieldINT(pStruct, m_nSpawnOption, const_cast<char*>("SpawnOption"));
    pGFF->WriteFieldINT(pStruct, m_nMaxCreatures, const_cast<char*>("MaxCreatures"));
    pGFF->WriteFieldINT(pStruct, m_nRecCreatures, const_cast<char*>("RecCreatures"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bPlayerOnly), const_cast<char*>("PlayerOnly"));
    pGFF->WriteFieldDWORD(pStruct, m_nFaction, const_cast<char*>("Faction"));
    pGFF->WriteFieldINT(pStruct, m_nDifficultyIndex, const_cast<char*>("DifficultyIndex"));
    pGFF->WriteFieldINT(pStruct, m_nDifficulty, const_cast<char*>("Difficulty"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("ZPosition"));
    pGFF->WriteFieldCExoLocString(pStruct, m_localizedName, const_cast<char*>("LocalizedName"));
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_ENTERED]), const_cast<char*>("OnEntered"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXIT]), const_cast<char*>("OnExit"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_HEARTBEAT]), const_cast<char*>("OnHeartbeat"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_EXHAUSTED]), const_cast<char*>("OnExhausted"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scripts[ENCOUNTER_SCRIPT_ON_USER_DEFINED]), const_cast<char*>("OnUserDefined"));

    if (m_nGeomVertCount >= 1) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("Geometry"));
        for (int64_t i = 0; i < m_nGeomVertCount; i++) {
            float dx = m_pGeomVerts[i].x - m_position.x;
            float dy = m_pGeomVerts[i].y - m_position.y;
            float dz = m_pGeomVerts[i].z - m_position.z;
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, static_cast<unsigned int>(i));
            pGFF->WriteFieldFLOAT(&elem, dx, const_cast<char*>("X"));
            pGFF->WriteFieldFLOAT(&elem, dy, const_cast<char*>("Y"));
            pGFF->WriteFieldFLOAT(&elem, dz, const_cast<char*>("Z"));
        }
    }

    if (m_nSpawnPointCount >= 1) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("CreatureList"));
        for (int64_t i = 0; i < m_nSpawnPointCount; i++) {
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, static_cast<unsigned int>(i));
            pGFF->WriteFieldCResRef(&elem, m_paSpawnPoints[i].resRef, const_cast<char*>("ResRef"));
            pGFF->WriteFieldFLOAT(&elem, m_paSpawnPoints[i].m_fCR, const_cast<char*>("CR"));
            pGFF->WriteFieldBYTE(&elem, static_cast<unsigned char>(m_paSpawnPoints[i].m_bSingleSpawn), const_cast<char*>("SingleSpawn"));
#if !KOTOR1
            pGFF->WriteFieldINT(&elem, m_paSpawnPoints[i].m_nGuaranteedCount, const_cast<char*>("GuaranteedCount"));
#endif
        }
    }

    if (m_nSpawnPointPositionCount >= 1) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("SpawnPointList"));
        for (int64_t i = 0; i < m_nSpawnPointPositionCount; i++) {
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, static_cast<unsigned int>(i));
            pGFF->WriteFieldFLOAT(&elem, m_paSpawnPointPositions[i].m_vPos.x, const_cast<char*>("X"));
            pGFF->WriteFieldFLOAT(&elem, m_paSpawnPointPositions[i].m_vPos.y, const_cast<char*>("Y"));
            pGFF->WriteFieldFLOAT(&elem, m_paSpawnPointPositions[i].m_vPos.z, const_cast<char*>("Z"));
            pGFF->WriteFieldFLOAT(&elem, m_paSpawnPointPositions[i].m_fOrientation, const_cast<char*>("Orientation"));
        }
    }

    pGFF->WriteFieldINT(pStruct, m_nActiveCreatureCount, const_cast<char*>("NumberSpawned"));
    pGFF->WriteFieldDWORD(pStruct, m_lastUpdateDay, const_cast<char*>("HeartbeatDay"));
    pGFF->WriteFieldDWORD(pStruct, m_lastUpdateTod, const_cast<char*>("HeartbeatTime"));
    pGFF->WriteFieldDWORD(pStruct, m_lastSpawnDay, const_cast<char*>("LastSpawnDay"));
    pGFF->WriteFieldDWORD(pStruct, m_lastSpawnTime, const_cast<char*>("LastSpawnTime"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bStarted), const_cast<char*>("Started"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bExhausted), const_cast<char*>("Exhausted"));
    pGFF->WriteFieldINT(pStruct, m_nCurrentRespawns, const_cast<char*>("CurrentSpawns"));
    pGFF->WriteFieldFLOAT(pStruct, m_fSpawnPoolActive, const_cast<char*>("SpawnPoolActive"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_enteredObjectId), const_cast<char*>("LastEntered"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_exitObjectId), const_cast<char*>("LastLeft"));
    pGFF->WriteFieldINT(pStruct, m_nUserDefinedEventNumber, const_cast<char*>("CustomScriptId"));
    pGFF->WriteFieldINT(pStruct, m_nActivateAreaCapacity, const_cast<char*>("AreaListMaxSize"));
    pGFF->WriteFieldFLOAT(pStruct, m_fAreaPoints, const_cast<char*>("AreaPoints"));

    if (m_nActivateAreaCount >= 1) {
        pGFF->WriteFieldINT(pStruct, m_nActivateAreaCount, const_cast<char*>("AreaListSize"));
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("AreaList"));
        for (int64_t i = 0; i < m_nActivateAreaCount; i++) {
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, 3);
            pGFF->WriteFieldDWORD(&elem, static_cast<unsigned int>(m_pActivateAreaList[i]), const_cast<char*>("AreaObject"));
        }
    }

    if (m_spawnList.m_count >= 1) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("SpawnList"));
        for (int64_t i = 0; i < m_spawnList.m_count; i++) {
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, static_cast<unsigned int>(i));
            pGFF->WriteFieldCResRef(&elem, m_spawnList.m_pArray[i]->resRef, const_cast<char*>("SpawnResRef"));
            pGFF->WriteFieldFLOAT(&elem, m_spawnList.m_pArray[i]->m_fCR, const_cast<char*>("SpawnCR"));
        }
    }

    return 1;
}
