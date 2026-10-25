
#include "nwsspellimpact.h"


#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwscript.h"

#include "appmanager.h"
#include "serverexoapp.h"
#include "nwsmodule.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwsrules.h"
#include "nwspell.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "resgff.h"
#include "gameobjectarray.h"
#include "nwsarea.h"
#include "nwarea.h"
#include "Vector.h"
#include "exobase.h"
#include "worldtimer.h"
#if !KOTOR1  // K1 inlines AddUnique into JumpToPoint
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE 1
#endif
#include "CExoArrayListImpl.h"

// Constructs the AoE at spawn time, as a base CSWSObject with the AoE object type, then sets every
// field the GFF and 2DA loaders would otherwise leave to the allocator: shape 0, corners null,
// heartbeat clock 0, duration 0, type byte 2, links to and from the invalid object id, the world
// clock snapshotted, spell id -1 and radius 0.
CSWSAreaOfEffectObject::CSWSAreaOfEffectObject(unsigned long long objectId)
    : CSWSObject(GAME_OBJECT_TYPE_AREA_OF_EFFECT, objectId, 0)
{
    unsigned int nDay;
    unsigned int nTod;
    m_oidLinkedToObject = CGameObjectArray::INVALIDOBJID;
    m_enteredObjectId = CGameObjectArray::INVALIDOBJID;
    m_exitObjectId = CGameObjectArray::INVALIDOBJID;
    m_oidCreator = CGameObjectArray::INVALIDOBJID;
    SetPosition(Vector(), 1, 1, 0);
    m_nShape = 0;
    m_paCorners = 0;
    m_nLastHrtbtDay = 0;
    m_nLastHrtbtTime = 0;
    m_effectSpellId = SPELL_ID_NONE;
    m_fRadius = 0.0f;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
    // These two stores must follow both clock stores above, not sit between them.
    m_durationMS = 0;
    m_nDurationType = 2;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);
    m_nSpellSaveDC = 14;
    m_nSpellLevel = 0;
}

// Unhooks the linked object's AoE list, leaves the area, frees the corner block, and unregisters
// from the AI master and the module's tag lookup. The string members and the base destructor follow
// implicitly. The list removal runs before RemoveFromArea, and neither fetched object is
// null-checked, only the lookup status.
CSWSAreaOfEffectObject::~CSWSAreaOfEffectObject()
{
    if (m_oidLinkedToObject != CGameObjectArray::INVALIDOBJID) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidLinkedToObject, &pObject)
                == CGameObjectArray::SUCCESS)
            pObject->AsSWSCreature()->m_areaOfEffectIds.Remove(m_objectId);
    }
    RemoveFromArea();
    delete[] m_paCorners;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0)
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
}

void CSWSAreaOfEffectObject::AddToArea(CSWSArea* pArea, float x, float y, float z, int a)
{
    if (pArea == 0)
        return;
    SetArea(pArea);
    SetPosition(Vector(x, y, z), 0, 1, 0);
    if (m_nShape == 1 && m_paCorners != 0) {
        Vector vDir = normalize(m_orientation);
        Vector vRight(-vDir.y, vDir.x, vDir.z);
        m_paCorners[0] = vDir * m_fLength + vRight * m_fWidth;
        m_paCorners[1] = vDir * m_fLength - vRight * m_fWidth;
        m_paCorners[2] = -vDir * m_fLength - vRight * m_fWidth;
        m_paCorners[3] = -vDir * m_fLength + vRight * m_fWidth;
        for (int i = 0; i < 4; i++)
            m_paCorners[i] = m_paCorners[i] * 0.5f + GetPosition();
    }
    static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, a);
    static_cast<CSWSArea*>(GetArea())->m_objects.Add(m_objectId);
    SetAnimation(1);
    UpdateSubAreas(0);
}

// Re-scans the containing area for the subareas covering this AoE's position: walks the area's
// X-sorted object index from the minimum X of {position, pVector} minus the radius and, for every
// live creature the AoE now covers or no longer covers, fires the OnEnter or OnExit script pair.
//
// The range bracket re-calls GetPosition() per comparison instead of caching a local, and the same
// creature can get OnEnter followed immediately by OnExit, since the two paths both run.
void CSWSAreaOfEffectObject::UpdateSubAreas(Vector* pVector)
{
    if (GetArea() == 0)
        return;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;
    int nIndex = 0;
    float fMinX = GetPosition().x;
    float fMaxX = fMinX;
    if (pVector != 0) {
#if KOTOR1
        float fX = GetPosition().x;
        fMinX = fX < pVector->x ? fX : pVector->x;
        fX = GetPosition().x;
        fMaxX = fX > pVector->x ? fX : pVector->x;
#else
        fMinX = pVector->x;
        if (GetPosition().x < fMinX)
            fMinX = GetPosition().x;
        fMaxX = pVector->x;
        if (GetPosition().x > fMaxX)
            fMaxX = GetPosition().x;
#endif
    }
    static_cast<CSWSArea*>(GetArea())->GetFirstObjectIndiceByX(&nIndex, fMinX - m_fRadius);
    for (int i = nIndex; i < pArea->m_aObjectsByX.m_count; i++) {
        if (pObjectArray->GetGameObject(pArea->m_aObjectsByX.m_pArray[i], &pObject)
                != CGameObjectArray::SUCCESS)
            continue;
        if (pObject->AsSWSObject() == 0)
            continue;
        int bContains = 0;
        int bInRange = 0;
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            bContains = pCreature->m_currentSubareaIds.Contains(m_objectId);
            if (m_nShape == 0) {
                if (pVector == 0) {
                    Vector vSelf = GetPosition();
                    float fDx = pCreature->m_position.x - vSelf.x;
                    float fDy = pCreature->m_position.y - vSelf.y;
                    float fDz = pCreature->m_position.z - vSelf.z;
                    bInRange = fDx * fDx + fDy * fDy + fDz * fDz <= m_fRadius * m_fRadius;
                } else {
                    Vector vProj = segmentproject(GetPosition(), *pVector,
                                                  pCreature->m_position + GetPosition());
                    float fDx = pCreature->m_position.x - vProj.x;
                    float fDy = pCreature->m_position.y - vProj.y;
                    float fDz = pCreature->m_position.z - vProj.z;
                    if (fDx * fDx + fDy * fDy + fDz * fDz <= m_fRadius * m_fRadius) {
                        if (bContains != 0)
                            bInRange = 1;
                        else {
                            float gDx = pCreature->m_position.x - pVector->x;
                            float gDy = pCreature->m_position.y - pVector->y;
                            float gDz = pCreature->m_position.z - pVector->z;
                            if (gDx * gDx + gDy * gDy + gDz * gDz > m_fRadius * m_fRadius) {
                                pCreature->m_currentSubareaIds.AddUnique(m_objectId);
                                CScriptEvent* pEvent = new CScriptEvent();
                                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
                                pEvent->SetObjectID(0, pCreature->m_objectId);
                                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                                    0, 0, pCreature->m_objectId, m_objectId,
                                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                                bContains = 1;
                            } else
                                bInRange = 1;
                        }
                    }
                }
            } else {
                bInRange = InAreaOfEffect(pCreature->m_position);
            }
            if (bContains != 0 && bInRange == 0) {
                pCreature->m_currentSubareaIds.Remove(m_objectId);
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
                pEvent->SetObjectID(0, pCreature->m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pCreature->m_objectId, m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }

            if (bContains == 0 && bInRange != 0) {
                pCreature->m_currentSubareaIds.AddUnique(m_objectId);
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
                pEvent->SetObjectID(0, pCreature->m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pCreature->m_objectId, m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                bContains = 1;
            }
        }
        if (bInRange == 0) {
            if (pObject->AsSWSObject()->m_position.x - fMaxX > m_fRadius)
                break;
        }
    }
}


// Drops the AoE out of every creature standing inside it: walks the containing area's X-sorted
// object list and, for each live creature the AoE covers, unhooks the id from the creature's current
// subarea list and signals it OnExit.
//
// The contains call precedes the removal, so the list is scanned twice.
void CSWSAreaOfEffectObject::RemoveFromSubAreas(int a)
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(pArea->m_aObjectsByX.m_pArray[i], &pObject)
                != CGameObjectArray::SUCCESS)
            continue;
        if (pObject->m_objectType != GAME_OBJECT_TYPE_CREATURE)
            continue;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (InAreaOfEffect(pCreature->m_position) == 0)
            continue;
        // Only creatures that were in this AoE get the exit event.
        if (pCreature->m_currentSubareaIds.Contains(m_objectId) == 0)
            continue;
        pCreature->m_currentSubareaIds.Remove(m_objectId);
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
        pEvent->SetObjectID(0, pCreature->m_objectId);
        if (a == 0) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pCreature->m_objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        } else {
            EventHandler(CServerAIMaster::EVENT_SIGNAL_EVENT, pCreature->m_objectId, pEvent, 0, 0);
        }
    }
}

// Runs the AoE's heartbeat tick: refreshes the world-clock snapshot, runs the heartbeat script on a
// 6 s cadence, follows the linked object, then either expires or runs the action queue. The world
// timer is re-fetched before all three calls, so there is no cached timer local. The linked
// object's position is read directly, with no virtual call, and only for a round AoE.
void CSWSAreaOfEffectObject::AIUpdate()
{
    unsigned long long nTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nDayDelta1;
    unsigned int nTodDelta1;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nDayDelta1, &nTodDelta1);
    unsigned int nDayDelta2;
    unsigned int nTodDelta2;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastHrtbtDay, m_nLastHrtbtTime, &nDayDelta2, &nTodDelta2);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
    m_nAIUpdateDeltaMs = nTodDelta1;
    if (nTodDelta2 / 16 >= 375) {
        if (m_nLastHrtbtTime != 0)
            g_pVirtualMachine->RunScript(&m_asScripts[SCRIPT_SLOT_HEARTBEAT], m_objectId, 1);
        m_nLastHrtbtTime = nTod;
        m_nLastHrtbtDay = nDay;
    }
    if (m_oidLinkedToObject != CGameObjectArray::INVALIDOBJID && m_nShape == 0) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidLinkedToObject, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0) {
            Vector vPos = static_cast<CSWSObject*>(pObject)->m_position;
            if (GetArea() != 0)
                UpdateSubAreas(&vPos);
            SetPosition(vPos, 1, 1, 0);
        } else {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
    }
    if (m_nDurationType == 1) {
        if (m_durationMS <= nTodDelta1) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            return;
        }
        m_durationMS -= nTodDelta1;
    }
    RunActions(nDay, nTod, nTimer);
}

// Unhooks every creature inside the AoE, then leaves the area; the teardown twin of AddToArea.
// RemoveFromSubAreas(1) comes first, before the area's own bookkeeping.
void CSWSAreaOfEffectObject::RemoveFromArea()
{
    if (GetArea() != 0) {
        RemoveFromSubAreas(1);
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
        static_cast<CSWSArea*>(GetArea())->m_objects.Remove(m_objectId);
    }
    SetArea(0);
    Vector vOrigin;
    SetPosition(vOrigin, 1, 1, 0);
    SetAnimation(0);
}

// Tests whether the from-to segment crosses this AoE's region, the twin of InAreaOfEffect. Round
// effects get their own line and circle math; the rectangle walks the four corners SetShape built
// and asks CSWArea::IntersectLineSegments about each edge.
//
// The rectangle body is the fallthrough, so the test is spelled `if (m_nShape != 0)`. The
// `fMinY >= t && fMaxY <= t` range test is inverted and effectively dead for a normal range;
// flipping it changes both branches.
int CSWSAreaOfEffectObject::LineSegmentIntersectAreaOfEffect(Vector from, Vector to)
{
    if (m_nShape != 0) {
        CSWArea* pArea = GetArea();
        if (pArea == 0)
            return 0;
        for (int i = 0; i < 4; i++) {
            int j = (i == 3) ? 0 : i + 1;
            float fT1;
            float fT2;
            if (pArea->IntersectLineSegments(from.x, from.y, to.x, to.y, m_paCorners[i].x,
                                             m_paCorners[i].y, m_paCorners[j].x,
                                             m_paCorners[j].y, &fT1, &fT2) == 1)
                return 1;
        }
        return 0;
    }

    if (from.x == to.x) {
        float fDx = GetPosition().x - to.x;
        if (fDx < -m_fRadius || fDx > m_fRadius)
            return 0;
        float fMaxY = (from.y > to.y) ? from.y : to.y;
        float fMinY = (from.y < to.y) ? from.y : to.y;
        float fDy = __builtin_sqrtf(m_fRadius * m_fRadius - fDx * fDx);
        if (fMinY >= GetPosition().y - fDy && fMaxY <= GetPosition().y - fDy)
            return 1;
        if (!(fMinY >= GetPosition().y + fDy) || !(fMaxY <= GetPosition().y + fDy))
            return 0;
        return 1;
    }

    float fSlope = (to.y - from.y) / (to.x - from.x);
    float fIntercept = to.y - fSlope * to.x;
    float fA = fSlope * fSlope + 1.0f;
    float fB = 2.0f * (fSlope * (fIntercept - GetPosition().y) - GetPosition().x);
    float fC = GetPosition().x * GetPosition().x
               - (fIntercept - GetPosition().y) * (fIntercept - GetPosition().y)
               - m_fRadius * m_fRadius;
    float fDisc = fB * fB - 4.0f * fA * fC;
    if (fDisc < 0.0f)
        return 0;
    float fSqrtDisc = __builtin_sqrtf(fDisc);
    float fX1 = (-fB - fSqrtDisc) / (2.0f * fA);
    float fMinX = (from.x < to.x) ? from.x : to.x;
    float fMaxX = (from.x > to.x) ? from.x : to.x;
    float fX2 = (fSqrtDisc - fB) / (2.0f * fA);
    if (fMinX >= fX1 && fMaxX <= fX1)
        return 1;
    if (fMinX >= fX2 && fMaxX <= fX2)
        return 1;
    return 0;
}

unsigned int CSWSAreaOfEffectObject::GetEffectSpellId()
{
    return m_effectSpellId;
}

void CSWSAreaOfEffectObject::SetEffectSpellId(unsigned int spellId)
{
    m_effectSpellId = spellId;
}

void CSWSAreaOfEffectObject::SetDuration(unsigned char b, float seconds)
{
    m_nDurationType = b;
    if (seconds > 0.0f)
        m_durationMS = static_cast<unsigned int>(seconds * 1000.0f);
}

// Dispatches the AoE's script events through a three-way event-id chain with no jump table.
// Sub-types 0xc and 0xd are the same entered/exit pair CSWSEncounter::EventHandler handles, which
// is what names the offset block this class adds.
void CSWSAreaOfEffectObject::EventHandler(unsigned int nEventId, unsigned long long callerId,
                                          void* pData, unsigned int a, unsigned int b)
{
    K2_UNUSED(callerId); K2_UNUSED(a); K2_UNUSED(b);
    if (nEventId == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT)) {
        if (pData == 0) return;
        g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
    } else if (nEventId == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) {
        CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
        // Written 0xc then 0xd: the compare chain comes out in the reverse of source order here.
        if (pEvent->m_eventType == CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER) {
            m_enteredObjectId = pEvent->GetObjectID(0);
            g_pVirtualMachine->RunScript(&m_asScripts[SCRIPT_SLOT_ENTER], m_objectId, 1);
        } else if (pEvent->m_eventType == CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT) {
            m_exitObjectId = pEvent->GetObjectID(0);
            g_pVirtualMachine->RunScript(&m_asScripts[SCRIPT_SLOT_EXIT], m_objectId, 1);
        }
        delete pEvent;
    } else if (nEventId == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) {
        if (m_bDestroyable != 0) {
            RemoveFromArea();
            delete this;
        }
    }
}

CSWSAreaOfEffectObject* CSWSAreaOfEffectObject::AsSWSAreaOfEffectObject()
{
    return this;
}

// The shape setter, shared by both GFF load paths. The rectangle arm keeps only the half-extents:
// the radius is derived, and the corner block is re-allocated each time without freeing the old
// one.
void CSWSAreaOfEffectObject::SetShape(unsigned char b, float fWidth, float fLength)
{
    m_nShape = b;
    if (b == 0) {
        m_fRadius = fWidth;
    } else if (b == 1) {
        m_fWidth = fWidth;
        m_fLength = fLength;
        float fHalfWidth = fWidth * 0.5f;
        float fHalfLength = fLength * 0.5f;
        m_fRadius = __builtin_sqrtf(fHalfWidth * fHalfWidth + fHalfLength * fHalfLength);
        m_paCorners = new Vector[4]();
        m_oidLinkedToObject = OBJECT_INVALID;
    }
}

// Sets the AoE's creator: the AoE inherits the caster's metamagic bits and derives its own spell
// save DC and effective caster level from whoever spawned it. GetEffectSpellId() is called on the
// creature rather than on this AoE, three times and never cached. Above the last class index the
// level comes from the spell row instead of the class.
void CSWSAreaOfEffectObject::SetCreator(unsigned long long creatorId)
{
    m_oidCreator = creatorId;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(creatorId);
    if (pCreature == 0)
        return;
    if (pCreature->GetEffectSpellId() == CSWSObject::SPELL_ID_NONE)
        return;
    m_nCastMetaMagic = pCreature->m_nCastMetaMagic;
    m_nSpellSaveDC = pCreature->CalculateSpellSaveDC(pCreature->GetEffectSpellId());
    if (!(pCreature->m_nCastClassIndex >= CSWSObject::CAST_CLASS_SPELL_LIKE_ABILITY)) {
        m_nSpellLevel = pCreature->m_pStats->GetClassLevel(pCreature->m_nCastClassIndex, 1);
    } else {
        CSWSpell* pSpell = g_pRules->GetSpell(pCreature->GetEffectSpellId());
        if (pSpell == 0)
            return;
        m_nSpellLevel = pSpell->m_nSpellLevelOther * 2 - 1;
    }
}

// Moves the AoE along with its linked object: only when that object still exists and is in the area
// the AoE is being moved to does the AoE leave its old area's subarea set, teleport, and add itself
// to the new one's.
void CSWSAreaOfEffectObject::JumpToPoint(CSWSArea* pArea, const Vector& vPos)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidLinkedToObject);
    if (pCreature == 0 || static_cast<CSWSArea*>(GetArea()) != pArea)
        return;
    pCreature->m_currentSubareaIds.Remove(m_objectId);
    RemoveFromSubAreas(0);
    SetPosition(vPos, 1, 1, 0);
    pCreature->m_currentSubareaIds.AddUnique(m_objectId);
    UpdateSubAreas(0);
}

// The vfx_persistent.2da row reader behind CSWSArea::ApplyEffect's OnApplyAreaOfEffect hook. The
// circle arm writes the shape and radius by hand while the rectangle arm calls SetShape. All five
// string locals share one scope, so their destructors run together at the epilogue, and the
// rectangle arm's two floats are uninitialised.
void CSWSAreaOfEffectObject::LoadAreaEffect(int nRow)
{
    m_nAreaEffectId = nRow;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPersistentVisualEffect;

    CExoString sShape;
    pTable->GetCExoStringEntry(m_nAreaEffectId, CExoString("SHAPE"), &sShape);
    if (sShape == "C") {
        float fRadius;
        pTable->GetFLOATEntry(m_nAreaEffectId, CExoString("RADIUS"), &fRadius);
        m_nShape = 0;
        m_fRadius = fRadius;
    } else if (sShape == "R") {
        float fWidth;
        float fLength;
        pTable->GetFLOATEntry(m_nAreaEffectId, CExoString("WIDTH"), &fWidth);
        pTable->GetFLOATEntry(m_nAreaEffectId, CExoString("LENGTH"), &fLength);
        SetShape(1, fWidth, fLength);
    }

    CExoString sOnEnter;
    pTable->GetCExoStringEntry(m_nAreaEffectId, CExoString("ONENTER"), &sOnEnter);
    m_asScripts[SCRIPT_SLOT_ENTER] = sOnEnter;

    CExoString sOnHeartbeat;
    pTable->GetCExoStringEntry(m_nAreaEffectId, CExoString("HEARTBEAT"), &sOnHeartbeat);
    m_asScripts[SCRIPT_SLOT_HEARTBEAT] = sOnHeartbeat;

    CExoString sOnExit;
    pTable->GetCExoStringEntry(m_nAreaEffectId, CExoString("ONEXIT"), &sOnExit);
    m_asScripts[SCRIPT_SLOT_EXIT] = sOnExit;

    CExoString sLabel;
    pTable->GetCExoStringEntry(m_nAreaEffectId, CExoString("LABEL"), &sLabel);
    SetTag(sLabel);

    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
}

// The per-element "AreaEffectList" GFF reader. Shape is read into a local and the SetShape calls
// come after the spell-level read.
int CSWSAreaOfEffectObject::LoadEffect(CResGFF* pGff, CResStruct* pStruct)
{
    int bSuccess;
    m_sTag = pGff->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, CExoString(""))
                 .LowerCase();
    m_nAreaEffectId = pGff->ReadFieldINT(pStruct, const_cast<char*>("AreaEffectId"), bSuccess, 0);
    m_effectSpellId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("SpellId"), bSuccess, 0);
    unsigned char nShape = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("Shape"), bSuccess, 0);
    m_nCastMetaMagic = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("MetaMagicType"), bSuccess, 0);
    m_nSpellSaveDC = pGff->ReadFieldINT(pStruct, const_cast<char*>("SpellSaveDC"), bSuccess, 0);
    m_nSpellLevel = pGff->ReadFieldINT(pStruct, const_cast<char*>("SpellLevel"), bSuccess, 0);
    if (nShape == 0) {
        float fRadius = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Radius"), bSuccess, 0.0f);
        SetShape(0, fRadius, 0.0f);
    } else if (nShape == 1) {
        float fLength = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Length"), bSuccess, 0.0f);
        float fWidth = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Width"), bSuccess, 0.0f);
        SetShape(1, fWidth, fLength);
    }
    m_oidCreator = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("CreatorId"), bSuccess, 0);
    m_oidLinkedToObject = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("LinkedToObject"), bSuccess, 0);
    m_enteredObjectId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("LastEntered"), bSuccess, 0);
    m_exitObjectId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("LastLeft"), bSuccess, 0);
    m_durationMS = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("Duration"), bSuccess, 0);
    m_nDurationType = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("DurationType"), bSuccess, 0);
    m_nLastHrtbtDay = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("LastHrtbtDay"), bSuccess, 0);
    m_nLastHrtbtTime = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("LastHrtbtTime"), bSuccess, 0);
    return 1;
}

// The writer's twin of LoadEffect, same fields in the same order. The object-id fields go out
// through WriteFieldDWORD, so the 64-bit ids are truncated; position and orientation come from the
// live members rather than from the GFF read-backs.
int CSWSAreaOfEffectObject::SaveEffect(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGff->WriteFieldINT(pStruct, m_nAreaEffectId, const_cast<char*>("AreaEffectId"));
    pGff->WriteFieldDWORD(pStruct, GetEffectSpellId(), const_cast<char*>("SpellId"));
    pGff->WriteFieldBYTE(pStruct, m_nShape, const_cast<char*>("Shape"));
    pGff->WriteFieldBYTE(pStruct, m_nCastMetaMagic, const_cast<char*>("MetaMagicType"));
    pGff->WriteFieldINT(pStruct, m_nSpellSaveDC, const_cast<char*>("SpellSaveDC"));
    pGff->WriteFieldINT(pStruct, m_nSpellLevel, const_cast<char*>("SpellLevel"));
    if (m_nShape == 0) {
        pGff->WriteFieldFLOAT(pStruct, m_fRadius, const_cast<char*>("Radius"));
    } else if (m_nShape == 1) {
        pGff->WriteFieldFLOAT(pStruct, m_fLength, const_cast<char*>("Length"));
        pGff->WriteFieldFLOAT(pStruct, m_fWidth, const_cast<char*>("Width"));
    }
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidCreator), const_cast<char*>("CreatorId"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidLinkedToObject), const_cast<char*>("LinkedToObject"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_enteredObjectId), const_cast<char*>("LastEntered"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_exitObjectId), const_cast<char*>("LastLeft"));
    pGff->WriteFieldDWORD(pStruct, m_durationMS, const_cast<char*>("Duration"));
    pGff->WriteFieldBYTE(pStruct, m_nDurationType, const_cast<char*>("DurationType"));
    pGff->WriteFieldDWORD(pStruct, m_nLastHrtbtDay, const_cast<char*>("LastHrtbtDay"));
    pGff->WriteFieldDWORD(pStruct, m_nLastHrtbtTime, const_cast<char*>("LastHrtbtTime"));
    pGff->WriteFieldCExoString(pStruct, m_asScripts[SCRIPT_SLOT_HEARTBEAT], const_cast<char*>("OnHeartbeat"));
    pGff->WriteFieldCExoString(pStruct, m_asScripts[SCRIPT_SLOT_USERDEFINED], const_cast<char*>("OnUserDefined"));
    pGff->WriteFieldCExoString(pStruct, m_asScripts[SCRIPT_SLOT_ENTER], const_cast<char*>("OnObjEnter"));
    pGff->WriteFieldCExoString(pStruct, m_asScripts[SCRIPT_SLOT_EXIT], const_cast<char*>("OnObjExit"));
    Vector vPos = GetPosition();
    pGff->WriteFieldFLOAT(pStruct, vPos.x, const_cast<char*>("PositionX"));
    pGff->WriteFieldFLOAT(pStruct, vPos.y, const_cast<char*>("PositionY"));
    pGff->WriteFieldFLOAT(pStruct, vPos.z, const_cast<char*>("PositionZ"));
    Vector vOrientation = m_orientation;
    pGff->WriteFieldFLOAT(pStruct, vOrientation.x, const_cast<char*>("OrientationX"));
    pGff->WriteFieldFLOAT(pStruct, vOrientation.y, const_cast<char*>("OrientationY"));
    pGff->WriteFieldFLOAT(pStruct, vOrientation.z, const_cast<char*>("OrientationZ"));
    return 1;
}

// Returns the AoE's position, which is its anchor's: an AoE linked to a live object and shaped
// round reports that object's position, so everything from NWScript to the subarea tests sees the
// effect where it is attached. The fallback calls this same function, because the override shadows
// CSWSObject's own position accessor and the unqualified call resolves back here. The recursion
// only runs when the link is invalid or absent, or the shape is a rectangle.
Vector CSWSAreaOfEffectObject::GetPosition()
{
    if (m_oidLinkedToObject != CGameObjectArray::INVALIDOBJID && m_nShape == 0) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidLinkedToObject, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0)
            return pObject->AsSWSCreature()->m_position;
    }
    return GetPosition();
}

// Point-in-AoE. Round effects are a pure radius test; rectangle effects additionally clip against
// the four corners SetShape built, as a convex polygon walk that rejects on the first positive cross
// product.
int CSWSAreaOfEffectObject::InAreaOfEffect(Vector vPos)
{
    Vector vSelf = GetPosition();
    float fDx = vPos.x - vSelf.x;
    float fDy = vPos.y - vSelf.y;
    float fDz = vPos.z - vSelf.z;
    if (!(fDx * fDx + fDy * fDy + fDz * fDz <= m_fRadius * m_fRadius))
        return 0;
    if (m_nShape == 0)
        return 1;
    for (int i = 0; i < 4; i++) {
        int j = (i == 3) ? 0 : i + 1;
        float fEdgeX = m_paCorners[j].x - m_paCorners[i].x;
        float fEdgeY = m_paCorners[j].y - m_paCorners[i].y;
        float fPointX = vPos.x - m_paCorners[i].x;
        float fPointY = vPos.y - m_paCorners[i].y;
        if (fEdgeX * fPointY - fPointX * fEdgeY > 0.0f)
            return 0;
    }
    return 1;
}

// Re-anchors the AoE: unhooks it from the old linked object's AoE list and hooks it into the new
// one's.
void CSWSAreaOfEffectObject::SetTargetObjID(unsigned long long objectId)
{
    CGameObject* pObject;
    unsigned char bSuccess = CGameObjectArray::SUCCESS;
    if (m_oidLinkedToObject != CGameObjectArray::INVALIDOBJID) {
        bSuccess = g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
            m_oidLinkedToObject, &pObject);
        if (bSuccess == CGameObjectArray::SUCCESS && pObject != 0)
            pObject->AsSWSCreature()->m_areaOfEffectIds.Remove(m_objectId);
    }
    m_oidLinkedToObject = objectId;
    bSuccess = g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidLinkedToObject,
                                                                              &pObject);
    if (bSuccess == CGameObjectArray::SUCCESS && pObject != 0)
        pObject->AsSWSCreature()->m_areaOfEffectIds.Add(m_objectId);
}
