#include "k2stdint.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "swguimaininterfacepc2.h"
#include "nwchandlemessage.h"

#include "swparty.h"
#include "exostring.h"

#include "Quaternion.h"
#include "nwsaiaction.h"

// Shortest-arc quaternion rotating one vector onto another.
Quaternion RotationArc(Vector from, Vector to);

#include "nwccreaturestats.h"

#include "guiingame.h"

#include "clientexoapp.h"

#include "partytables.h"

#include "serverexoapp.h"
#include "pathfindinfo.h"
#include "nwsarea.h"

#include "appmanager.h"

#include "nwsrules.h"

int FollowMode = -1;

extern int quaternionidentityoptimization;

extern Vector zaxis;

#include "nwscript.h"

#include "nwseffectlist.h"
#include "nwseffect.h"
#include "aurorainterface.h"

#include "SWCollisionMesh.h"

int g_bEnablePartySmoothing = 1;

#include "nwsplayer.h"

extern CAppManager* g_pAppManager;

#include "port/libcstring.h"
#include "strref.h"

// ranges.2da rows the party reads.
enum {
    RANGES_PERCEPTION_DEFAULT = 11,   // PercepRngDefault
    RANGES_PERCEPTION_PLAYER  = 12,   // PercepRngPlayer
    RANGES_PARTY_CLOSE_FOLLOW = 14,   // PartyCloseFollow
    RANGES_PARTY_HOLD_FOLLOW  = 15,   // PartyHoldFollow
    RANGES_PARTY_ESCAPE       = 16,   // PartyEscape
};

CSWPartyMemberData::CSWPartyMemberData()
    : id(OBJECT_INVALID), m_nFollowState(FOLLOW_STATE_NEW), m_followPathDistance(1.0f),
      m_nReserved1(), m_nReserved2(), m_nReserved3(), m_vPathPoint(), m_nPathIndex(),
      m_followPoint(), m_bFollowPointValid(), m_formationOffset(),
      m_nStateSteps(), m_followDistance(),
      m_followOrientation(Vector(0.0f, 1.0f, 0.0f)), m_fPathSpanRatio(1.0f),
      m_safePosition(), m_bReseatSafePosition(), m_bWaitForLeaderDistance(), m_vLeaderPathOrigin(),
      m_oidLastTarget(OBJECT_INVALID), m_nCombatMessageStrRef(STRREF_COMBAT_MODE_ENGAGED)
{
}

CSWParty::CSWParty()
{
    m_formationTable[0].vCompanion1Offset = Vector(1.5f, -0.7f, 0.0f);
    m_formationTable[0].vCompanion2Offset = Vector(-1.5f, 0.8f, 0.0f);
    m_afFollowSpacing[0] = 1.5f;
    m_afFollowSpacing[1] = 1.5f;
    m_formationTable[1].vCompanion1Offset = Vector();
    m_afFollowSpacing[2] = 1.5f;
    m_formationTable[2].vCompanion1Offset = Vector(-2.0f, 1.5f, 0.0f);
    m_formationTable[2].vCompanion2Offset = Vector(2.0f, 4.5f, 0.0f);
    m_bFormationInverted = 0;
    m_members[1].m_formationOffset = m_formationTable[0].vCompanion1Offset;
    m_members[2].m_formationOffset = m_formationTable[0].vCompanion2Offset;
    m_nFormation = 0;
    m_followRangeBias = 1.0f;
    m_members[0].id = OBJECT_INVALID;
    m_members[1].id = OBJECT_INVALID;
    m_members[2].id = OBJECT_INVALID;
    m_nCharacterCount = 0;
    m_nReserved1 = 0;
    m_bRosterDirty = 0;
    m_fReserved1 = 0.0f;
    m_fReserved2 = 0.5f;
    m_bEnabled = 1;
    m_vReserved1 = Vector();
    m_abFormationValid[0] = 0;
    m_abFormationValid[1] = 0;
    m_abFormationValid[2] = 0;
    m_vLastAnchorLeaderPos = Vector();
    m_fRetreatScriptTimer = 0.0f;
    m_followData.m_pParty = this;
    m_members[0].m_followPathDistance = 0.0f;
    m_members[1].m_followPathDistance = 1.5f;
    m_members[2].m_followPathDistance = 3.0f;
}

CSWParty::~CSWParty()
{
}

int CSWParty::GetFormation()
{
    return m_nFormation;
}

int CSWParty::IsEnabled()
{
    return m_bEnabled;
}

void CSWParty::InvertFormation()
{
    m_members[1].m_formationOffset.x = -m_members[1].m_formationOffset.x;
    m_members[2].m_formationOffset.x = -m_members[2].m_formationOffset.x;
    m_bFormationInverted ^= 1;
}

// Defines custom formation formationId (a negative id; SetFormation indexes the custom table by the
// id's complement): the two companions' offsets.
void CSWParty::DefineFormation(int formationId, const Vector& a, const Vector& b)
{
    FormationDef& def = m_aCustomFormations[~formationId];
    def.vCompanion1Offset = a;
    def.vCompanion2Offset = b;
}

// Sets the formation; -9999 advances to the next usable one.
void CSWParty::SetFormation(int formation)
{
    if (formation > 2) {
        return;
    }
    if (m_nFormation == formation) {
        return;
    }

    int nIndex;
    if (formation < 0) {
        if (formation == -9999) {
            formation = (m_nFormation > 1) ? -3 : m_nFormation + 1;
            while (formation < 0 && m_abFormationValid[~formation] == 0) {
                formation++;
            }
            SetFormation(formation);
            return;
        }
        nIndex = ~formation;
        if (static_cast<unsigned int>(nIndex) > 2) {
            return;
        }
        if (m_abFormationValid[nIndex] == 0) {
            return;
        }
        m_members[1].m_formationOffset = m_aCustomFormations[nIndex].vCompanion1Offset;
        m_members[2].m_formationOffset = m_aCustomFormations[nIndex].vCompanion2Offset;
    } else {
        nIndex = formation;
        m_members[1].m_formationOffset = m_formationTable[formation].vCompanion1Offset;
        m_members[2].m_formationOffset = m_formationTable[formation].vCompanion2Offset;
    }

    float fBias = m_afFollowSpacing[nIndex];
    m_nFormation = formation;
    m_followRangeBias = fBias;
    if (m_bFormationInverted) {
        Vector tmp = m_members[1].m_formationOffset;
        m_members[1].m_formationOffset = m_members[2].m_formationOffset;
        m_members[2].m_formationOffset = tmp;
    }
}

CSWCCreature* CSWParty::GetPlayerCharacter()
{
    if (m_nCharacterCount < 1) {
        return 0;
    }
    return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[0].id);
}

CSWCCreature* CSWParty::GetCharacter(int index)
{
    if (index < 0) {
        return 0;
    }
    if (index >= m_nCharacterCount) {
        return 0;
    }
    return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[index].id);
}

int CSWParty::IsPartyMember(unsigned long long id)
{
    for (int i = 0; i < m_nCharacterCount; i++) {
        if (m_members[i].id == id) {
            return 1;
        }
    }
    return 0;
}

int CSWParty::GetIndex(unsigned long long id)
{
    if (id == OBJECT_INVALID) {
        return -1;
    }
    for (int i = 0; i < m_nCharacterCount; i++) {
        if (m_members[i].id == id) {
            return i;
        }
    }
    return -1;
}

void CSWParty::ClearParty()
{
    for (int i = 0; i < m_nCharacterCount; i++) {
        m_members[i].id = OBJECT_INVALID;
    }
    m_nCharacterCount = 0;
}

CSWCCreature* CSWParty::RemoveCharacter(int index)
{
    if (!g_pAppManager) {
        return 0;
    }
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[index].id);
    if (index <= 1) {
        for (int i = index; i < 2; i++) {
            m_members[i].id = m_members[i + 1].id;
        }
    }
    m_members[2].id = OBJECT_INVALID;
    m_nCharacterCount--;
#if !KOTOR1
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->ResetPartyMemberItemUse();
#endif
    return pCreature;
}

int CSWParty::RemoveCharacter(unsigned long long id)
{
    if (id == OBJECT_INVALID) {
        return 0;
    }
    if (m_nCharacterCount < 1) {
        return 0;
    }
    int index = -1;
    for (int i = 0; i < m_nCharacterCount; i++) {
        if (m_members[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return 0;
    }
    if (!g_pAppManager) {
        return 0;
    }
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[index].id);
    if (index <= 1) {
        for (int i = index; i < 2; i++) {
            m_members[i].id = m_members[i + 1].id;
        }
    }
    m_members[2].id = OBJECT_INVALID;
    m_nCharacterCount--;
#if !KOTOR1
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->ResetPartyMemberItemUse();
#endif
    if (pCreature) {
        return 1;
    }
    return 0;
}

void CSWParty::FollowStateChanged()
{
    RecaulateFollowPoint();
    for (int i = 0; i < 3; i++) {
        switch (m_members[i].m_nFollowState) {
        case CSWPartyMemberData::FOLLOW_STATE_OFFSET:
            m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_OFFSET_WAIT_DEST;
            break;
        case CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH:
            m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_TO_PATH_WAIT_DEST;
            break;
        case CSWPartyMemberData::FOLLOW_STATE_FOLLOWING_PATH:
            m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_PATH_WAIT_DEST;
            break;
        case CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET:
            m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_TO_OFFSET_WAIT_DEST;
            break;
        default: break;
        }
    }
}

float CSWParty::GetFollowRange()
{
    float range = 5.0f;
    CSWPartyTable* pTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int state = pTable->m_followState;
    int effective = (FollowMode == -1) ? state : FollowMode;
    if (effective == 0) {
        g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PARTY_CLOSE_FOLLOW, CExoString("PrimaryRange"), &range);
    } else if (effective == 1) {
        g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PARTY_HOLD_FOLLOW, CExoString("PrimaryRange"), &range);
    }
    return range;
}

float CSWParty::GetEscapeRange()
{
    float range;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pRanges;
    {
        CExoString col("PrimaryRange");
        pTable->GetFLOATEntry(RANGES_PARTY_ESCAPE, col, &range);
    }
    return range;
}

int CSWParty::IsPartyMemberAware()
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    for (int i = 0; i < 3; i++) {
        unsigned long long id = pServerApp->ClientToServerObjectId(m_members[i].id);
        CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(id);
        if (pCreature && pCreature->GetActivity(2)) {
            return 1;
        }
    }
    return 0;
}

int CSWParty::IsPartyMemberStealthed()
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    for (int i = 0; i < 3; i++) {
        unsigned long long id = pServerApp->ClientToServerObjectId(m_members[i].id);
        CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(id);
        if (pCreature && pCreature->GetActivity(1)) {
            return 1;
        }
    }
    return 0;
}

int CSWParty::IsPartyHealthy()
{
    unsigned long long id = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (!pCreature || pCreature->GetCurrentHitPoints(0) < 1) {
        return 0;
    }
    for (int i = 0; i < m_nCharacterCount; i++) {
        unsigned long long mid = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_members[i].id);
        CSWSCreature* pMember = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(mid);
        if (pMember && pMember->GetCurrentHitPoints(0) < 1) {
            return 0;
        }
    }
    return 1;
}

int CSWParty::IsPartyInCombat()
{
    unsigned long long id = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    unsigned long long sid = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(id);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(sid);
    if (!pCreature) {
        return 0;
    }
    if (pCreature->m_bCombatState && pCreature->m_nCombatType == 1) {
        return 1;
    }
    for (int i = 0; i < m_nCharacterCount; i++) {
        unsigned long long mid = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_members[i].id);
        CSWSCreature* pMember = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(mid);
        if (pMember && pMember->m_bCombatState && pMember->m_nCombatType == 1) {
            return 1;
        }
    }
    return 0;
}

int CSWParty::IsPartyTogether()
{
    unsigned long long id = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    unsigned long long sid = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(id);
    CSWSCreature* pLeader = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(sid);
    if (!pLeader) {
        return 0;
    }
    pLeader->GetArea();
    if (m_nCharacterCount < 1) {
        return 1;
    }
    Vector leaderPos = pLeader->m_position;
    for (int i = 0; i < m_nCharacterCount; i++) {
        unsigned long long mid = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_members[i].id);
        CSWSCreature* pMember = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(mid);
        if (!pMember) {
            continue;
        }
        float dx = leaderPos.x - pMember->m_position.x;
        float dy = leaderPos.y - pMember->m_position.y;
        float dz = leaderPos.z - pMember->m_position.z;
        if (dx * dx + dy * dy + dz * dz > 900.0f) {
            return 0;
        }
    }
    return 1;
}

void CSWParty::RecaulateFollowPoint()
{
    Vector pos = {};
    int valid;
    if (m_followData.GetPointBackFromStart(GetFollowRange() - m_followRangeBias, pos, valid, 1)) {
        m_members[1].m_followPoint = pos;
        m_members[1].m_bFollowPointValid = valid;
    }
    if (m_followData.GetPointBackFromStart(GetFollowRange() + m_followRangeBias, pos, valid, 2)) {
        m_members[2].m_followPoint = pos;
        m_members[2].m_bFollowPointValid = valid;
    }
}

int CSWParty::PartyHasRace(unsigned short race)
{
    for (int i = 0; i < m_nCharacterCount; i++) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[i].id);
        if (pCreature->m_pStats->m_nRace == race) {
            return 1;
        }
    }
    return 0;
}

// Rotates a member's formation offset into world space about the facing direction.
Vector CSWParty::GetWorldFormationPosition(const Vector& pos, const Vector& facing, int direction)
{
    if (direction == 0) {
        return pos;
    }
    if (static_cast<unsigned>(direction - 1) > 1) {
        Vector zero = {};
        return zero;
    }

    Quaternion q = RotationArc(Vector{0.0f, 1.0f, 0.0f}, facing);
    Vector* pOffset = &m_members[direction].m_formationOffset;
    Vector rot;

    if (quaternionidentityoptimization == 0 || (q.w != 1.0f && q.w != -1.0f)) {
        float w = q.w, x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        rot.x = pOffset->x * (1.0f - ((yy + zz) + (yy + zz))) +
                pOffset->y * ((xy - wz) + (xy - wz)) + pOffset->z * ((xz + wy) + (xz + wy));
        rot.y = pOffset->x * ((xy + wz) + (xy + wz)) +
                pOffset->y * (1.0f - ((xx + zz) + (xx + zz))) +
                pOffset->z * ((yz - wx) + (yz - wx));
        rot.z = pOffset->x * ((xz - wy) + (xz - wy)) + pOffset->y * ((yz + wx) + (yz + wx)) +
                pOffset->z * (1.0f - ((xx + yy) + (xx + yy)));
    } else {
        rot = *pOffset;
    }

    Vector result;
    result.x = rot.x + pos.x;
    result.y = rot.y + pos.y;
    result.z = rot.z + pos.z;
    return result;
}

void CSWPartyFollowDataArray::SetPathData(int index, const Vector& pos, float value)
{
    m_entries[index].m_pos = pos;
    m_entries[index].m_fLeaderYaw = value;
}

CSWPartyFollowDataArray::CSWPartyFollowDataArray()
{
    memset(m_entries, 0, sizeof(m_entries));
    Vector vZero;
    Reset(vZero, 0.0f);
}

void CSWPartyFollowDataArray::Reset(const Vector& pos, float value)
{
    for (int i = 0; i < 100; i++) {
        m_entries[i].m_pos = pos;
        m_entries[i].m_fLeaderYaw = value;
    }
    m_nWriteIndex = 0;
    m_nSampleCount = 0;
}

CSWPartyFollowData* CSWPartyFollowDataArray::GetPathData(int index)
{
    if (index < 0 || index >= 100) {
        CExoString sError;
        sError.Format(const_cast<char*>("PathFollowData requesting bad data position %d"), index);
    }
    return &m_entries[index];
}

float CSWPartyFollowDataArray::PointOnLineDistanceFromLeader(int nMember, int bSquared)
{
    CSWParty* pParty = m_pParty;
    CSWCCreature* pLeader =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(pParty->m_members[0].id);

    float fDx = pParty->m_members[nMember].m_vPathPoint.x - pLeader->m_position.x;
    float fDy = pParty->m_members[nMember].m_vPathPoint.y - pLeader->m_position.y;
    float fDz = pParty->m_members[nMember].m_vPathPoint.z - pLeader->m_position.z;

    float fDistance = fDx * fDx + fDy * fDy + fDz * fDz;
    if (bSquared == 0) {
        fDistance = __builtin_sqrtf(fDistance);
    }
    return fDistance;
}

int CSWPartyFollowDataArray::FindFurthestPartyMemeber_Current()
{
    int nPoint1 = (m_pParty->m_members[1].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[1].m_nPathIndex;
    int nPoint2 = (m_pParty->m_members[2].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[2].m_nPathIndex;

    if (m_nWriteIndex > nPoint1 && nPoint1 != -1) {
        nPoint1 += 100;
    }
    if (m_nWriteIndex > nPoint2 && nPoint2 != -1) {
        nPoint2 += 100;
    }

    if (nPoint1 >= nPoint2) {
        return 2;
    }
    return 1;
}

int CSWPartyFollowDataArray::FindClosestPartyMemeber_Current()
{
    int nPoint1 = (m_pParty->m_members[1].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[1].m_nPathIndex;
    int nPoint2 = (m_pParty->m_members[2].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[2].m_nPathIndex;

    if (m_nWriteIndex > nPoint1 && nPoint1 != -1) {
        nPoint1 += 100;
    }
    if (m_nWriteIndex > nPoint2 && nPoint2 != -1) {
        nPoint2 += 100;
    }

    if (nPoint1 <= nPoint2) {
        return 2;
    }
    return 1;
}

int CSWPartyFollowDataArray::FindClosestPartyMemeber_Target()
{
    int nPoint1 =
        (m_pParty->m_members[1].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[1].m_bFollowPointValid;
    int nPoint2 =
        (m_pParty->m_members[2].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[2].m_bFollowPointValid;

    if (m_nWriteIndex > nPoint1 && nPoint1 != -1) {
        nPoint1 += 100;
    }
    if (m_nWriteIndex > nPoint2 && nPoint2 != -1) {
        nPoint2 += 100;
    }

    if (nPoint1 <= nPoint2) {
        return 2;
    }
    return 1;
}

int CSWPartyFollowDataArray::FindFurthestPartyMemeber_Target()
{
    int nPoint1 =
        (m_pParty->m_members[1].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[1].m_bFollowPointValid;
    int nPoint2 =
        (m_pParty->m_members[2].id == OBJECT_INVALID) ? -1 : m_pParty->m_members[2].m_bFollowPointValid;

    if (m_nWriteIndex > nPoint1 && nPoint1 != -1) {
        nPoint1 += 100;
    }
    if (m_nWriteIndex > nPoint2 && nPoint2 != -1) {
        nPoint2 += 100;
    }

    if (nPoint1 >= nPoint2) {
        return 2;
    }
    return 1;
}

// Resets the follow path and re-seats every member's creature on a safe spot.
void CSWParty::ResetAllFollowData(const Vector& pos, float yaw, int bBuildOrientation)
{
    Reset(pos, yaw, bBuildOrientation);

    for (int i = 0; i < 3; i++) {
        m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;
        m_members[i].m_vPathPoint = pos;
        m_members[i].m_nPathIndex = 0;
        m_members[i].m_followPoint = pos;
        m_members[i].m_bFollowPointValid = 0;
    }

    for (int i = 0; i < 3; i++) {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pCreature =
            pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(m_members[i].id));
        if (pCreature != 0) {
            pCreature->m_pPathfindInfo->m_bIgnoreCrePath = 0;
            pCreature->m_pPathfindInfo->m_oidIgnoredBlocker = OBJECT_INVALID;

            if (!g_pAppManager->m_pServerExoApp->TestSafeLocation(pCreature->m_objectId,
                                                                  pCreature->m_position)) {
                Vector vSafe;
                g_pAppManager->m_pServerExoApp->ComputeSafeLocation(
                    pCreature->m_position, 5.0f, pCreature->m_objectId, 1, &vSafe);
                pCreature->SetPosition(vSafe, 1, 1, 0);
            }
        }
    }
}

void CSWParty::Reset(const Vector& pos, float yaw, int bBuildOrientation)
{
    m_followData.Reset(pos, yaw);

    if (bBuildOrientation == 0) {
        return;
    }

    Quaternion q(zaxis, yaw * 0.017453292f);
    Vector vUp = {0.0f, 1.0f, 0.0f};
    Vector rot;

    if (quaternionidentityoptimization == 0 || (q.w != 1.0f && q.w != -1.0f)) {
        float w = q.w, x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        rot.x = vUp.x * (1.0f - ((yy + zz) + (yy + zz))) + vUp.y * ((xy - wz) + (xy - wz)) +
                vUp.z * ((xz + wy) + (xz + wy));
        rot.y = vUp.x * ((xy + wz) + (xy + wz)) + vUp.y * (1.0f - ((xx + zz) + (xx + zz))) +
                vUp.z * ((yz - wx) + (yz - wx));
        rot.z = vUp.x * ((xz - wy) + (xz - wy)) + vUp.y * ((yz + wx) + (yz + wx)) +
                vUp.z * (1.0f - ((xx + yy) + (xx + yy)));
    } else {
        rot = vUp;
    }

    Vector vBack;
    vBack.x = rot.x * -1.5f;
    vBack.y = rot.y * -1.5f;
    vBack.z = rot.z * -1.5f;

    Vector vSafe;

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pLeader =
        pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(m_members[0].id));
    if (pLeader != 0) {
        for (int i = 1; i < 3; i++) {
            CSWSArea* pArea = pServer->GetAreaByGameObjectID(pLeader->m_cachedAreaId);
            Vector vTry = {vBack.x + pos.x, vBack.y + pos.y, vBack.z + pos.z};
            if (pArea->ComputeSafeLocation(vTry, 2.0f, pLeader->m_pPathfindInfo, 1, &vSafe)) {
                m_members[i].m_followPoint = vSafe;
                m_members[i].m_bFollowPointValid = 0;
                m_followData.ProcessPoint(vSafe, yaw, 0.0f);
            }
            vBack.x += vBack.x;
            vBack.y += vBack.y;
            vBack.z += vBack.z;
        }
    }

    m_followData.ProcessPoint(pos, yaw, 0.0f);
}

float DegToRad(float degrees);

// Detects the party walking back down its own path and re-solves the follow points.
int CSWParty::TestAndResetPath()
{
    int nIndex = m_followData.m_nWriteIndex + ((m_followData.m_nWriteIndex < 1) ? 99 : -1);

    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(m_followData.GetPathData(nIndex)->m_fLeaderYaw));
    Vector vForward = {0.0f, 1.0f, 0.0f};
    Vector vDir;

    if (quaternionidentityoptimization == 0 || (q.w != 1.0f && q.w != -1.0f)) {
        float w = q.w, x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        vDir.x = vForward.x * (1.0f - ((yy + zz) + (yy + zz))) +
                 vForward.y * ((xy - wz) + (xy - wz)) + vForward.z * ((xz + wy) + (xz + wy));
        vDir.y = vForward.x * ((xy + wz) + (xy + wz)) +
                 vForward.y * (1.0f - ((xx + zz) + (xx + zz))) +
                 vForward.z * ((yz - wx) + (yz - wx));
        vDir.z = vForward.x * ((xz - wy) + (xz - wy)) + vForward.y * ((yz + wx) + (yz + wx)) +
                 vForward.z * (1.0f - ((xx + yy) + (xx + yy)));
    } else {
        vDir = vForward;
    }

    Vector* pOrientation = &m_members[m_followData.FindClosestPartyMemeber_Current()].m_followOrientation;
    float fDot = vDir.x * pOrientation->x + vDir.y * pOrientation->y + vDir.z * pOrientation->z;
    if (fDot < 0.0f) {
        RecaulateFollowPoint();

        for (int i = 1; i < 3; i++) {
            if (i >= m_nCharacterCount)
                break;
            if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_members[i].id) != 0) {
                switch (m_members[i].m_nFollowState) {
                case CSWPartyMemberData::FOLLOW_STATE_NEW:
                case CSWPartyMemberData::FOLLOW_STATE_FIND_PATH:
                case CSWPartyMemberData::FOLLOW_STATE_WAITING_FOR_PATH:
                case CSWPartyMemberData::FOLLOW_STATE_OFFSET_WAIT_DEST:
                case CSWPartyMemberData::FOLLOW_STATE_PATH_WAIT_DEST:
                    break;
                default:
                    m_members[i].m_nPathIndex = m_members[i].m_bFollowPointValid;
                    m_members[i].m_vPathPoint = m_members[i].m_followPoint;
                    m_members[i].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET;
                    break;
                }
            }
        }

        InvertFormation();
        return 1;
    }

    return 0;
}

int CSWParty::AddCharacter(unsigned long long id)
{
    if (id == OBJECT_INVALID) {
        return -1;
    }

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer == 0) {
        return -1;
    }

    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CSWSCreature* pCreature =
        pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(id));
    if (pCreature == 0) {
        return -1;
    }

    if (m_members[0].id == id) {
        return 0;
    }

    int nIndex = -1;
    if (m_members[0].id == OBJECT_INVALID) {
        m_members[0].id = id;
        m_members[0].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;

        float fPrimaryRange;
        float fSecondaryRange;
        g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_PLAYER, CExoString("PrimaryRange"),
                                                        &fPrimaryRange);
        g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_PLAYER, CExoString("SecondaryRange"),
                                                        &fSecondaryRange);

        pClient->GetSWCMessage()->SendPlayerToServerGuiInventory_Status(1);
        pClient->GetSWCMessage()->SendPlayerToServerJournalMessage_QuestScreenOpen();

        nIndex = 0;
        pCreature->m_fPerceptionPrimaryRange = fPrimaryRange;
        pCreature->m_fPerceptionSecondaryRange = fSecondaryRange;
        m_nCharacterCount++;
    } else {
        for (int i = 1; i < 3; i++) {
            if (m_members[i].id == id) {
                return i;
            }
            if (m_members[i].id == OBJECT_INVALID) {
                m_members[i].id = id;
                nIndex = i;
                m_members[nIndex].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;

                float fPrimaryRange;
                float fSecondaryRange;
                g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_DEFAULT, CExoString("PrimaryRange"),
                                                                &fPrimaryRange);
                g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_DEFAULT, CExoString("SecondaryRange"),
                                                                &fSecondaryRange);

                pCreature->m_fPerceptionPrimaryRange = fPrimaryRange;
                pCreature->m_fPerceptionSecondaryRange = fSecondaryRange;
                pCreature->AddAction(AIACTION_PARTY_FOLLOW_LEADER, ACTION_GROUP_NEW);
                m_nCharacterCount++;
                break;
            }
        }
    }

    if (pClient->GetPlayerCreatureId() == id) {
        pServer->GetPartyTable()->TransferInventory(pCreature);
    }

    int bAreaNotReady = pClient->GetAreaNotReadyToDisplay();
    int bIsLeader = pServer->GetPartyTable()->GetIsLeader(pServer->ClientToServerObjectId(id));

    if (bAreaNotReady) {
        if (bIsLeader) {
            pClient->SetFutureLeader(id);
            m_bRosterDirty = 1;
            return nIndex;
        }
    } else if (bIsLeader) {
        SetLeader(nIndex);
        m_bRosterDirty = 1;
        return nIndex;
    }

    if (!pServer->GetPartyTable()->GetIsLeader(
            pServer->ClientToServerObjectId(m_members[0].id)) &&
        pCreature->m_pStats->m_bIsPC != 0) {
        SetLeader(nIndex);
    }

    m_bRosterDirty = 1;
    return nIndex;
}

// Promotes a member to leader; -2 means the first companion and -3 the first party member. The
// roster is rotated rather than having the two slots swapped.
void CSWParty::SetLeader(int nIndex)
{
    unsigned long long oidOldLeader = m_members[0].id;
    if (nIndex == 0) {
        return;
    }
    if (m_nCharacterCount < 2) {
        return;
    }

    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

    CSWPartyMemberData* pMember;
    if (nIndex == -2) {
        pMember = &m_members[1];
    } else if (nIndex == -3) {
        pMember = m_members;
        int i;
        for (i = 0; i < 3; i++) {
            CSWSCreature* pCreature =
                pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(pMember->id));
            if (pCreature != 0 && pCreature->m_pStats->m_bIsPC != 0) {
                break;
            }
            pMember++;
        }
        if (i == 3) {
            return;
        }
    } else {
        if (nIndex < 1) {
            return;
        }
        if (nIndex >= m_nCharacterCount) {
            return;
        }
        pMember = &m_members[nIndex];
    }

    unsigned long long oidNewLeader = pMember->id;
    if (oidNewLeader == OBJECT_INVALID) {
        return;
    }

    unsigned long long oidTarget = OBJECT_INVALID;
    if (GetCharacter(0) != 0 && GetCharacter(0)->GetServerCreature() != 0) {
        oidTarget = GetCharacter(0)->GetServerCreature()->m_oidAttackTarget;
    }
    m_members[0].m_oidLastTarget = oidTarget;
    g_pAppManager->m_pClientExoApp->SetLastTarget(m_members[0].m_oidLastTarget, 0);

    while (m_members[0].id != oidNewLeader) {
        unsigned long long oidRolled = m_members[0].id;
        unsigned long long oidRolledTarget = m_members[0].m_oidLastTarget;
        int i;
        for (i = 0; i < 2; i++) {
            m_members[i].id = m_members[i + 1].id;
            m_members[i].m_oidLastTarget = m_members[i + 1].m_oidLastTarget;
            if (m_members[i].id == OBJECT_INVALID) {
                break;
            }
        }
        m_members[i].id = oidRolled;
        m_members[i].m_oidLastTarget = oidRolledTarget;
    }

    CSWCCreature* pNewLeaderClient = pClient->GetCreatureByGameObjectID(oidNewLeader);
    if (pNewLeaderClient != 0) {
        pClient->UpdateRoom(pNewLeaderClient->m_position);
    }

    if (!m_bEnabled) {
        return;
    }

    float fLeaderPrimaryRange;
    float fLeaderSecondaryRange;
    float fPartyPrimaryRange;
    float fPartySecondaryRange;
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_PLAYER, CExoString("PrimaryRange"),
                                                    &fLeaderPrimaryRange);
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_PLAYER, CExoString("SecondaryRange"),
                                                    &fLeaderSecondaryRange);
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_DEFAULT, CExoString("PrimaryRange"),
                                                    &fPartyPrimaryRange);
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PERCEPTION_DEFAULT, CExoString("SecondaryRange"),
                                                    &fPartySecondaryRange);

    CSWSPlayer* pPlayer = 0;
    CExoLinkedList<CSWSPlayerListEntry>* ppPlayers = pServer->GetPlayerList();
    CExoLinkedListNode* pNode = ppPlayers->m_pInternal->m_pHead;
    if (pNode != 0) {
        pPlayer = static_cast<CSWSPlayer*>(pNode->m_pData);
    }

    CSWSCreature* pNewLeaderServer =
        pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(m_members[0].id));
    CSWPartyTable* pPartyTable = pServer->GetPartyTable();
    pPartyTable->SetLeader(pServer->ClientToServerObjectId(oidNewLeader));
    pPlayer->SetGameObject(pNewLeaderServer);
    pNewLeaderServer->m_fPerceptionPrimaryRange = fLeaderPrimaryRange;
    pNewLeaderServer->m_fPerceptionSecondaryRange = fLeaderSecondaryRange;

    CSWCCreature* pOldLeaderClient =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oidOldLeader);
    if (oidOldLeader != oidNewLeader && pOldLeaderClient != 0) {
        CSWSCreature* pOldLeaderServer = pOldLeaderClient->GetServerCreature();
        if (pOldLeaderServer != 0) {
            CExoArrayList<CSWVisibilityNode*>* pOldList = &pOldLeaderServer->m_visibilityList;
            for (int i = pNewLeaderServer->m_visibilityList.m_count - 1; i >= 0; i--) {
                CSWVisibilityNode* pVisNode = pNewLeaderServer->m_visibilityList.m_pArray[i];
                pNewLeaderServer->m_visibilityList.DelIndex(i);
                delete pVisNode;
            }
            pNewLeaderServer->SetVisibilityList(pOldList);
            pOldLeaderServer->m_fPerceptionPrimaryRange = fPartyPrimaryRange;
            pOldLeaderServer->m_fPerceptionSecondaryRange = fPartySecondaryRange;
            pOldLeaderServer->SetVisibilityList(0);
            pOldLeaderServer->UpdateVisibleList(0);
        }
    }

    if (oidOldLeader != m_members[0].id) {
        CExoString sScript("k_hen_leadchng");
        unsigned long long oidScriptObject =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_members[0].id);
        g_pVirtualMachine->RunScript(&sScript, oidScriptObject, oidScriptObject != OBJECT_INVALID);
        m_members[0].m_bWaitForLeaderDistance = 1;
        m_members[1].m_bWaitForLeaderDistance = 1;
        m_members[2].m_bWaitForLeaderDistance = 1;
    }

    g_pAppManager->m_pClientExoApp->SetLastTarget(m_members[0].m_oidLastTarget, 0);
    if (pNewLeaderClient != 0) {
        g_pAppManager->m_pClientExoApp->SetCombatMode(pNewLeaderClient->m_nStateFlags & 1);
        if ((pNewLeaderClient->m_nStateFlags & 1) != 0 &&
            m_members[0].m_nCombatMessageStrRef != STRREF_COMBAT_MODE_ENGAGED) {
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetCombatMessage(
                m_members[0].m_nCombatMessageStrRef);
        }
    }
}

// Walks the recorded path backwards by a distance, refusing to pass the given member's own sample.
int CSWPartyFollowDataArray::GetPointBackFromStart(float fDistance, Vector& vOut, int& nOutIndex,
                                                   int nMember)
{
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;

    int nIndex = m_nWriteIndex + ((m_nWriteIndex < 1) ? 99 : -1);

    if (m_nSampleCount <= 2) {
        nOutIndex = 0;
        vOut.x = 0.0f;
        vOut.y = 0.0f;
        vOut.z = 0.0f;
        return 0;
    }

    CSWPartyFollowData* pEntry = GetPathData(nIndex);
    float fX = pEntry->m_pos.x;
    float fY = pEntry->m_pos.y;
    float fZ = pEntry->m_pos.z;
    int nRemaining = m_nSampleCount;

    nOutIndex = nIndex;

    int nStopIndex = -1;
    if (nMember != 0) {
        nStopIndex = m_pParty->m_members[nMember].m_nPathIndex;
    }

    while (fDistance > 0.0f && nRemaining >= 0) {
        nOutIndex = nIndex;

        if (nMember != 0 && nIndex == nStopIndex) {
            vOut.x = fX;
            vOut.y = fY;
            vOut.z = fZ;
            return 1;
        }

        nIndex = nIndex + ((nIndex < 1) ? 99 : -1);

        CSWPartyFollowData* pNext = GetPathData(nIndex);
        Vector vStep = {pNext->m_pos.x - fX, pNext->m_pos.y - fY, pNext->m_pos.z - fZ};
        float fStepSq = vStep.x * vStep.x + vStep.y * vStep.y + vStep.z * vStep.z;

        if (fStepSq >= fDistance * fDistance) {
            Vector vDir = normalize(vStep);
            vOut.x = fX + fDistance * vDir.x;
            vOut.y = fY + fDistance * vDir.y;
            vOut.z = fZ + fDistance * vDir.z;
            return 1;
        }

        fDistance -= __builtin_sqrtf(fStepSq);
        nRemaining--;
        CSWPartyFollowData* pStep = GetPathData(nIndex);
        fX = pStep->m_pos.x;
        fY = pStep->m_pos.y;
        fZ = pStep->m_pos.z;
    }

    if (nRemaining < 0) {
        vOut.x = 0.0f;
        vOut.y = 0.0f;
        vOut.z = 0.0f;
        return 0;
    }
    return 1;
}

// Walks the recorded path forward by fDistance, following its corners, and snaps to the follow
// point when that is within the budget.
float CSWPartyFollowDataArray::AdvancePointOnPath(float fDistance, int nDirection,
                                                  const Vector& vPos, int nIndex,
                                                  const Vector& vFollowPos, int nFollowIndex,
                                                  Vector& vOut, int& nOutIndex)
{
    K2_UNUSED(nDirection);
    Vector vFrom = vPos;
    vOut = vPos;

    if (fDistance > 0.0f) {
        float fDistanceSq = fDistance * fDistance;
        int bAtNewest = 0;
        do {
            CSWPartyFollowData* pEntry = GetPathData(nIndex);
            Vector vStep = pEntry->m_pos - vFrom;
            float fStepLen = magnitude(vStep);
            vStep = normalize(vStep);
            float fMove = (fStepLen < fDistance) ? fStepLen : fDistance;
            vOut = vOut + vStep * fMove;

            float fDx = vFollowPos.x - vPos.x;
            float fDy = vFollowPos.y - vPos.y;
            float fDz = vFollowPos.z - vPos.z;
            if (fDx * fDx + fDy * fDy + fDz * fDz <= fDistanceSq) {
                vOut = vFollowPos;
                nOutIndex = nFollowIndex;
                nIndex = nFollowIndex;
                // Result unused.
                magnitude(vFollowPos - vPos);
                break;
            }

            bAtNewest = 0;
            if (fDistance >= fStepLen) {
                CSWPartyFollowData* pNext = GetPathData(nIndex);
                vFrom = pNext->m_pos;
                nIndex = (nIndex > 98) ? 0 : nIndex + 1;
                if (nIndex == m_nWriteIndex - 1) {
                    bAtNewest = 1;
                }
                if (nIndex == 99 && m_nWriteIndex == 0) {
                    bAtNewest = 1;
                }
            }
            fDistance -= fMove;
        } while (fDistance > 0.0f && !bAtNewest);
    }

    nOutIndex = nIndex;
    return fDistance;
}

// Runs the party's per-frame tick. It picks the full-screen video effect the leader's force powers
// ask for, slides the follow anchor toward the point behind the leader, then runs the companion
// proximity scripts.
void CSWParty::Update(float fDelta)
{
    Vector vAnchor = m_followAnchorPosition;

    CSWCCreature* pLeader = GetPlayerCharacter();

#if !KOTOR1  // K1's party tick picks no video effect
    int nGuiSuppressed = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

    if (nGuiSuppressed == 0 && pLeader != 0 && pServer != 0) {
        CSWSCreature* pServerLeader =
            pServer->GetCreatureByGameObjectID(pServer->ClientToServerObjectId(pLeader->m_objectId));

        int bFury = 0;
        int bForceSight = 0;
        for (int i = 0; i < pServerLeader->m_effects.m_count; i++) {
            unsigned short nSubType = pServerLeader->m_effects.m_pArray[i]->m_subType;
            if (nSubType == CSWSEffectListHandler::EFFECT_FURY) {
                bFury = 1;
            } else if (nSubType == CSWSEffectListHandler::EFFECT_FORCE_SIGHT) {
                bForceSight = 1;
                break;
            }
        }

        int bSpeedBlur = AurIsSpeedBlurEnabled();
        if (bForceSight) {
            g_pAppManager->m_pClientExoApp->EnableVideoEffect(4);
        } else if (bSpeedBlur) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#if !KOTOR1  // K1: no Fury video effect
        } else if (bFury) {
            g_pAppManager->m_pClientExoApp->EnableVideoEffect(
                pServerLeader->m_pStats->m_nVideoEffectType);
#endif
        } else if (g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 4 ||
                   g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 7 ||
                   g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 8 ||
                   g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 9) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
    } else if (nGuiSuppressed != 0) {
        if (g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 4 ||
            g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 7 ||
            g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 8 ||
            g_pAppManager->m_pClientExoApp->GetVideoEffectType() == 9) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
    }
#endif

    if (pLeader != 0) {
        Quaternion qFacing(zaxis, Yaw(pLeader->m_orientation) * 0.017453292f);

        Vector vMoved = m_vLastAnchorLeaderPos - pLeader->m_position;
        float fMovedSq = vMoved.x * vMoved.x + vMoved.y * vMoved.y + vMoved.z * vMoved.z;

        Vector vTarget = pLeader->m_position - pLeader->m_orientation * (m_followRangeBias * 0.5f);

        float fAx = m_followAnchorPosition.x - vTarget.x;
        float fAy = m_followAnchorPosition.y - vTarget.y;
        float fAz = m_followAnchorPosition.z - vTarget.z;
        if (fAx * fAx + fAy * fAy + fAz * fAz >= 100.0f) {
            m_followAnchorPosition = vTarget;
            m_followAnchorOrientation = qFacing;
            vAnchor = vTarget;
        }

        if (fMovedSq >= 1.0f) {
            m_vLastAnchorLeaderPos = pLeader->m_position;
            Vector vDir = normalize(vTarget - vAnchor);
            m_followAnchorPosition = vAnchor + vDir * __builtin_sqrtf(fMovedSq);
            m_followAnchorOrientation = qFacing;
        }
    }

    int nFurthest = m_followData.FindFurthestPartyMemeber_Current();
    if (GetCharacter(nFurthest) != 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0 &&
        m_followData.PointOnLineDistanceFromLeader(nFurthest, 1) >= 49.0f &&
        TestAndResetPath() == 0) {
        RecaulateFollowPoint();
    }

    m_fRetreatScriptTimer += fDelta;

    CExoString sRetreat("k_hen_retreat");

    float fPrimaryRange;
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PARTY_ESCAPE, CExoString("PrimaryRange"),
                                                    &fPrimaryRange);
    fPrimaryRange = fPrimaryRange * fPrimaryRange;

    float fSecondaryRange;
    g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_PARTY_ESCAPE, CExoString("SecondaryRange"),
                                                    &fSecondaryRange);

#ifdef K2_FIX_ENGINE_BUGS
    if (m_nCharacterCount >= 2 && pLeader != 0) {
#else
    if (m_nCharacterCount >= 2) {
#endif
        int bRanRetreat = 0;
        for (int i = 1; i < m_nCharacterCount; i++) {
            unsigned long long oid =
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_members[i].id);
            CSWSCreature* pServerMember =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oid);
            if (pServerMember == 0) {
                continue;
            }
            if (pServerMember->m_pFollowInfo == 0) {
                continue;
            }
            int bWasWithin5m = pServerMember->m_pFollowInfo->m_bWithin5m;

            CSWCCreature* pMember = GetCharacter(i);
            float fDx = pLeader->m_position.x - pMember->m_position.x;
            float fDy = pLeader->m_position.y - pMember->m_position.y;
            float fDz = pLeader->m_position.z - pMember->m_position.z;
            float fDistanceSq = fDx * fDx + fDy * fDy + fDz * fDz;

            if (m_fRetreatScriptTimer >= fSecondaryRange && fDistanceSq >= fPrimaryRange) {
                g_pVirtualMachine->RunScript(&sRetreat, oid, oid != OBJECT_INVALID);
                bRanRetreat = 1;
            }

            if (bWasWithin5m != 0 && !(fDistanceSq <= 25.0f)) {
                CExoString sExit("k_hen_exit5m");
                g_pVirtualMachine->RunScript(&sExit, oid, oid != OBJECT_INVALID);
                pServerMember->m_pFollowInfo->m_bWithin5m = 0;
            }
            if (bWasWithin5m == 0 && fDistanceSq <= 25.0f) {
                CExoString sEnter("k_hen_enter5m");
                g_pVirtualMachine->RunScript(&sEnter, oid, oid != OBJECT_INVALID);
                pServerMember->m_pFollowInfo->m_bWithin5m = 1;
            }
        }
        if (bRanRetreat) {
            m_fRetreatScriptTimer = 0.0f;
        }
    }
}

// Commits a leader position into the follow-path ring and caches each companion's world formation
// position. A segment blocked by anything but a party member commits the last known-good point
// instead.
void CSWPartyFollowDataArray::ProcessPoint(const Vector& vPos, float fYaw, float fUnused)
{
    K2_UNUSED(fUnused);
    static Vector m_vLastGoodProcessedPoint;

    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;

    int nIndex = m_nWriteIndex + ((m_nWriteIndex < 1) ? 99 : -1);
    CSWPartyFollowData* pPrev = GetPathData(nIndex);

    float fDx = vPos.x - pPrev->m_pos.x;
    float fDy = vPos.y - pPrev->m_pos.y;
    float fDz = vPos.z - pPrev->m_pos.z;

    unsigned long long oidLeader = m_pParty->m_members[0].id;
    unsigned long long oidServerLeader =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader);
    CSWSCreature* pLeader =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidServerLeader);

    CResRef resRef;

    if (m_nSampleCount == 0) {
        m_vLastGoodProcessedPoint = vPos;
    }
    if (pLeader == 0) {
        return;
    }

    float fStepSq = fDx * fDx + fDy * fDy + fDz * fDz;

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    float fPersonalSpace = pLeader->m_pPathfindInfo->m_fPersonalSpace;
    int nBlocked = pServer->TestDirectLine(oidLeader, resRef, GetPathData(nIndex)->m_pos, vPos,
                                            fPersonalSpace, pLeader->m_pPathfindInfo->m_fHeightAlias, 1,
                                            &hitInfo);

    if (nBlocked != 1 &&
        !(nBlocked == -3 && hitInfo.m_hitObjectId != OBJECT_INVALID &&
          m_pParty->IsPartyMember(hitInfo.m_hitObjectId))) {
        m_entries[m_nWriteIndex].m_pos = m_vLastGoodProcessedPoint;
        int nNextBlocked;
        if (m_nWriteIndex > 98) {
            nNextBlocked = 0;
        } else {
            m_nWriteIndex = m_nWriteIndex + 1;
            nNextBlocked = m_nWriteIndex;
        }
        m_nWriteIndex = nNextBlocked;
    }

    m_vLastGoodProcessedPoint = vPos;

    if (!(fStepSq >= 0.25f) && nBlocked == 1) {
        return;
    }

    m_entries[m_nWriteIndex].m_pos = vPos;
    m_entries[m_nWriteIndex].m_fLeaderYaw = fYaw;
    int nNext;
    if (m_nWriteIndex > 98) {
        nNext = 0;
    } else {
        m_nWriteIndex = m_nWriteIndex + 1;
        nNext = m_nWriteIndex;
    }
    m_nWriteIndex = nNext;

    if (m_nSampleCount <= 99) {
        m_nSampleCount = m_nSampleCount + 1;
        if (m_nSampleCount < 3) {
            return;
        }
    }

    int nNewest = m_nWriteIndex + ((m_nWriteIndex < 1) ? 99 : -1);
    float fNewestYaw = m_entries[nNewest].m_fLeaderYaw;
    int nBase = nNewest + ((nNewest < 1) ? 99 : -1);
    CSWPartyFollowData* pBase = &m_entries[nBase];
    float fBaseX = pBase->m_pos.x;
    float fBaseY = pBase->m_pos.y;
    float fBaseZ = pBase->m_pos.z;
    float fBaseYaw = pBase->m_fLeaderYaw;

    for (int i = 0; i < 2; i++) {
        Vector vOffset = m_pParty->m_members[i + 1].m_formationOffset;
        {
            Quaternion qUnused(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fBaseYaw));
        }
        Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fNewestYaw));

        Vector vRotated;
        if (quaternionidentityoptimization != 0 && (q.w == 1.0f || q.w == -1.0f)) {
            vRotated = vOffset;
        } else {
            float xx = q.x * q.x;
            float yy = q.y * q.y;
            float zz = q.z * q.z;
            float xy = q.x * q.y;
            float xz = q.x * q.z;
            float wx = q.w * q.x;
            float yz = q.y * q.z;
            float wy = q.w * q.y;
            float wz = q.w * q.z;

            vRotated.x = vOffset.x * (1.0f - (yy + zz) * 2.0f) + vOffset.y * ((xy - wz) * 2.0f) +
                          vOffset.z * ((xz + wy) * 2.0f);
            vRotated.y = vOffset.x * ((xy + wz) * 2.0f) + vOffset.y * (1.0f - (xx + zz) * 2.0f) +
                          vOffset.z * ((yz - wx) * 2.0f);
            vRotated.z = vOffset.x * ((xz - wy) * 2.0f) + vOffset.y * ((yz + wx) * 2.0f) +
                          vOffset.z * (1.0f - (xx + yy) * 2.0f);
        }

        if (i != 0) {
            pBase->m_aFormationPos[1].x = fBaseX + vRotated.x;
            pBase->m_aFormationPos[1].y = fBaseY + vRotated.y;
            pBase->m_aFormationPos[1].z = fBaseZ + vRotated.z;
        } else {
            pBase->m_aFormationPos[0].x = fBaseX + vRotated.x;
            pBase->m_aFormationPos[0].y = fBaseY + vRotated.y;
            pBase->m_aFormationPos[0].z = fBaseZ + vRotated.z;
        }
    }

    if (g_bEnablePartySmoothing && m_pParty->m_nCharacterCount >= 2) {
        if (PointOnLineDistanceFromLeader(FindFurthestPartyMemeber_Current(), 1) >= 49.0f) {
            SmoothFollowPath();
        } else if (m_pParty->m_nCharacterCount == 3) {
            if (m_pParty->m_members[1].m_followDistance >= 100.0f &&
                m_pParty->m_members[2].m_followDistance >= 100.0f) {
                SmoothFollowPath();
            }
        } else if (m_pParty->m_nCharacterCount == 2) {
            if (m_pParty->m_members[1].m_followDistance >= 100.0f) {
                SmoothFollowPath();
            }
        }
    }

    if (m_pParty->m_members[1].id != OBJECT_INVALID) {
        unsigned long long oidMember =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_pParty->m_members[1].id);
        CSWSCreature* pMember =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidMember);
        if (pMember != 0 && pMember->HasAction(AIACTION_PARTY_FOLLOW_LEADER) == 0) {
            m_pParty->m_members[1].m_nPathIndex = m_pParty->m_members[1].m_bFollowPointValid;
            m_pParty->m_members[1].m_vPathPoint = m_pParty->m_members[1].m_followPoint;
        }
    }

    if (m_pParty->m_members[2].id != OBJECT_INVALID) {
        unsigned long long oidMember =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_pParty->m_members[2].id);
        CSWSCreature* pMember =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidMember);
        if (pMember != 0 && pMember->HasAction(AIACTION_PARTY_FOLLOW_LEADER) == 0) {
            m_pParty->m_members[2].m_nPathIndex = m_pParty->m_members[2].m_bFollowPointValid;
            m_pParty->m_members[2].m_vPathPoint = m_pParty->m_members[2].m_followPoint;
        }
    }
}

// Re-fits the recorded ring into a straighter path at even spacing: re-seats the companions' sample
// indices, drops the samples a straight walk can skip, and writes the result back.
void CSWPartyFollowDataArray::SmoothFollowPath()
{
    static int OFF;

    // Plain data twin of CSWPartyFollowPos, so the whole array can be zeroed.
    CSWPartyFollowPos aPath[201];
    __builtin_memset(aPath, 0, sizeof(aPath));

    int nLast = m_nWriteIndex + ((m_nWriteIndex < 1) ? 99 : -1);

    unsigned long long oidLeader = m_pParty->m_members[0].id;
    unsigned long long oidServerLeader =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader);
    CSWSCreature* pLeader =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidServerLeader);
    if (pLeader == 0) {
        return;
    }

#if !KOTOR1  // K1 has no per-companion line-of-walk test (and so no follow-point refresh here)
    CSWPartyFollowData* pLastEntry = GetPathData(nLast);

    CResRef resRef;
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;

    // sic: both lookups resolve the party leader, not companions 1 and 2, though the results are
    // stored as the companions' follow samples.
    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader);
    CSWSCreature* pCompanion1 =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidServerLeader);
    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader);
    CSWSCreature* pCompanion2 =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidServerLeader);

    if (pCompanion1 != 0) {
        float fSpace = pLeader->m_pPathfindInfo->m_fPersonalSpace;
        int nResult = g_pAppManager->m_pServerExoApp->TestDirectLine(
            oidLeader, resRef, pCompanion1->m_position, pLastEntry->m_pos, fSpace,
            pLeader->m_pPathfindInfo->m_fHeightAlias, 1, &hitInfo);
        unsigned long long oidHit =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfo.m_hitObjectId);
        if (nResult == -3) {
            if (m_pParty->IsPartyMember(oidHit)) {
                m_pParty->m_members[1].m_bFollowPointValid = nLast;
            }
        } else if (nResult == 1) {
            m_pParty->m_members[1].m_bFollowPointValid = nLast;
        }
    }

    if (pCompanion2 != 0) {
        float fSpace = pLeader->m_pPathfindInfo->m_fPersonalSpace;
        int nResult = g_pAppManager->m_pServerExoApp->TestDirectLine(
            oidLeader, resRef, pCompanion2->m_position, pLastEntry->m_pos, fSpace,
            pLeader->m_pPathfindInfo->m_fHeightAlias, 1, &hitInfo);
        unsigned long long oidHit =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfo.m_hitObjectId);
        if (nResult == -3) {
            if (m_pParty->IsPartyMember(oidHit)) {
                m_pParty->m_members[2].m_bFollowPointValid = nLast;
            }
        } else if (nResult == 1) {
            m_pParty->m_members[2].m_bFollowPointValid = nLast;
        }
    }
#endif

    int nStart = m_pParty->m_members[FindClosestPartyMemeber_Current()].m_nPathIndex;
    if (static_cast<unsigned int>(nStart) > 99) {
        nStart = 0;
    }

    CSWPartyFollowData* pStartEntry = GetPathData(nStart);
    aPath[0] = pStartEntry->m_pos;

    int nPrev = (nStart > 98) ? 0 : nStart + 1;
    CResRef resRefWalk;
    int nCount = 1;
    int nCur = nPrev;
    int nResult = 1;

    while (nCur != nLast && nLast != nStart && nCount <= 199 && nResult != 0) {
        CWalkHitInfo hitInfoWalk = {};
        hitInfoWalk.m_hitObjectId = OBJECT_INVALID;

        Vector vA = aPath[nCount - 1] - GetPathData(nCur)->m_pos;
        Vector vB = aPath[nCount - 1] - GetPathData(nCur)->m_pos;
        float fDot = vA.x * vB.x + vA.y * vB.y + vA.z * vB.z;

        nResult = 1;
        if (fDot > 1e-05) {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            float fSpace = pLeader->m_pPathfindInfo->m_fPersonalSpace;
            nResult = pServer->TestDirectLine(oidLeader, resRefWalk, aPath[nCount - 1],
                                               GetPathData(nCur)->m_pos, fSpace,
                                               pLeader->m_pPathfindInfo->m_fHeightAlias, 1,
                                               &hitInfoWalk);
        }
        unsigned long long oidHit =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfoWalk.m_hitObjectId);

        if (nResult == 1) {
            nPrev = nCur;
            nCur = (nCur > 98) ? 0 : nCur + 1;
            continue;
        }
        if (nResult == -3 && m_pParty->IsPartyMember(oidHit)) {
            nPrev = nCur;
            nCur = (nCur > 98) ? 0 : nCur + 1;
            continue;
        }

        if (equal(aPath[nCount - 1], GetPathData(nPrev)->m_pos)) {
            aPath[nCount] = GetPathData(nCur)->m_pos;
            nCur = (nCur > 98) ? 0 : nCur + 1;
        } else {
            aPath[nCount] = GetPathData(nPrev)->m_pos;
        }
        nCount++;
    }

    if (nCount <= 199) {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        float fSpace = pLeader->m_pPathfindInfo->m_fPersonalSpace;
        int nTail = pServer->TestDirectLine(oidLeader, resRefWalk, aPath[nCount - 1],
                                             GetPathData(nCur)->m_pos, fSpace,
                                             pLeader->m_pPathfindInfo->m_fHeightAlias, 1, 0);
        if (nTail == 1) {
            aPath[nCount] = GetPathData(nCur)->m_pos;
            nCount = nCount + 1;
        } else {
            aPath[nCount] = GetPathData(nPrev)->m_pos;
            aPath[nCount + 1] = GetPathData(nCur)->m_pos;
            nCount = nCount + 2;
        }
    }

    CSWPartyFollowData* pRewind = GetPathData(nStart);
    float fPrevX = pRewind->m_pos.x;
    float fPrevY = pRewind->m_pos.y;
    float fPrevZ = pRewind->m_pos.z;
    m_nWriteIndex = nStart;

    for (int i = 0; i < nCount;) {
        float fNewX = aPath[i].x;
        float fNewY = aPath[i].y;
        float fNewZ = aPath[i].z;

        Vector vDelta = {fNewX - fPrevX, fNewY - fPrevY, fNewZ - fPrevZ};
        if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z > 1.0f) {
            Vector vDir = normalize(vDelta);
            fNewX = fPrevX + vDir.x;
            fNewY = fPrevY + vDir.y;
            fNewZ = fPrevZ + vDir.z;
        } else {
            i++;
        }

        m_entries[m_nWriteIndex].m_pos.x = fNewX;
        m_entries[m_nWriteIndex].m_pos.y = fNewY;
        m_entries[m_nWriteIndex].m_pos.z = fNewZ;

        float fYaw;
        if (vDelta.x == 0.0f && vDelta.y == 0.0f && vDelta.z == 0.0f) {
            fYaw = m_entries[m_nWriteIndex].m_fLeaderYaw;
        } else {
            Vector vStep = {fNewX - fPrevX, fNewY - fPrevY, fNewZ - fPrevZ};
            fYaw = Yaw(normalize(vStep));
        }
        m_entries[m_nWriteIndex].m_fLeaderYaw = fYaw;

        int nNext;
        if (m_nWriteIndex > 98) {
            nNext = 0;
        } else {
            m_nWriteIndex = m_nWriteIndex + 1;
            nNext = m_nWriteIndex;
        }
        m_nWriteIndex = nNext;

        fPrevX = fNewX;
        fPrevY = fNewY;
        fPrevZ = fNewZ;
    }

    OFF = OFF + 6;
    if (OFF > 40) {
        OFF = 0;
    }
    m_pParty->RecaulateFollowPoint();
}

// Posts a strref to the member's combat-feedback line, for the local player's character only.
void CSWPartyMemberData::SetCombatMessage(uint64_t nStrRef)
{
    m_nCombatMessageStrRef = nStrRef;

    if (id != g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
        return;

    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetCombatMessage(m_nCombatMessageStrRef);
}
