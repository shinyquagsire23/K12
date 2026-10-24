
#include "k2stdint.h"
#include "nwsparty.h"

#include "appmanager.h"
#include "serverexoappinternal.h"
#include "factionmanager.h"
#include "exostring.h"
#include "resgff.h"
#include "nwsrules.h"
#include "nwscreature.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "nwsarea.h"
#include "CExoArrayListImpl.h"

#include "port/libcmath.h"

CFactionManager* CSWSFaction::GetFactionManager()
{
    CServerExoAppInternal* pInternal = g_pAppManager->m_pServerExoApp->m_pInternal;
    if (pInternal == 0)
        return 0;
    return pInternal->m_pFactionManager;
}

void CSWSFaction::SendFactionUpdateAdd(unsigned long long gameObjectId)
{
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_members.m_pArray[i]);
        if (pPlayer != 0)
            pMessage->SendServerToPlayerParty_List(pPlayer->m_nPlayerId, 1, &gameObjectId, 3,
                                                   m_members.m_pArray[i]);
    }
}

void CSWSFaction::SendFactionUpdateRemove(unsigned long long gameObjectId)
{
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_members.m_pArray[i]);
        if (pPlayer != 0)
            pMessage->SendServerToPlayerParty_List(pPlayer->m_nPlayerId, 1, &gameObjectId, 4,
                                                   OBJECT_INVALID);
    }
}

void CSWSFaction::SendFactionUpdateList()
{
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_members.m_pArray[i]);
        if (pPlayer != 0)
            pMessage->SendServerToPlayerParty_List(pPlayer->m_nPlayerId, m_members.m_count,
                                                   m_members.m_pArray, 1, m_members.m_pArray[i]);
    }
}

void CSWSFaction::SendChatMessage(unsigned long long speakerId, const CExoString& sText)
{
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_members.m_pArray[i]);
        if (pPlayer == 0)
            continue;
        pMessage->SendServerToPlayerChat_Party(pPlayer->m_nPlayerId, speakerId, sText);
    }
}

void CSWSFaction::Initialise()
{
    m_sName = CExoString("");
    m_nMemberIterator = 0;
    m_nGlobal = 1;
}

int CSWSFaction::GetIsInvited(unsigned long long gameObjectId)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(GetLeader());
    if (pCreature == 0)
        return 0;
    return pCreature->GetIsInInvitationsOffered(gameObjectId);
}

void CSWSFaction::AttemptJoin(unsigned long long gameObjectId)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(GetLeader());
    if (pCreature == 0)
        return;
    if (!pCreature->GetIsInInvitationsOffered(gameObjectId))
        return;
    AddMember(gameObjectId, 0);
}

int CSWSFaction::AttemptRemove(unsigned long long partyId, unsigned long long creatureId)
{
    if (m_members.m_count < 2)
        return 0;
    if (partyId != creatureId && GetLeader() != creatureId)
        return 0;
    RemoveMember(partyId);
    return 1;
}

// The leading message fetch is discarded. The next call overwrites its result, and the call stays
// because the method is not pure.
int CSWSFaction::InviteMember(unsigned long long partyId, unsigned long long creatureId)
{
    g_pAppManager->m_pServerExoApp->GetSWSMessage();

    CSWSCreature* pParty = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(partyId);
    if (pParty != 0) {
        pParty->SetInvitedToParty(1);
        pParty->m_oidInvitedToPartyBy = creatureId;
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(creatureId);
    if (pCreature != 0) {
        pCreature->AddToInvitationsOffered(partyId);
        return 1;
    }
    return 0;
}

int CSWSFaction::GetAverageLevel()
{
    if (m_members.m_count < 1)
        return 0;
    int nTotal = 0;
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        nTotal += pCreature->m_pStats->GetLevel(0);
    }
    if (m_members.m_count < 1)
        return 0;
    return nTotal / m_members.m_count;
}

// The whole-faction mean of each member's reputation toward the target, clamped to [0,100]. The
// target is resolved once, up front; a null target skips the walk entirely.
int CSWSFaction::GetAverageReputation(unsigned long long oidTarget)
{
    int nTotal = 0;
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget != 0 && m_members.m_count >= 1) {
        for (int64_t i = 0; i < m_members.m_count; i++)
            nTotal += pTarget->GetReputation(m_members.m_pArray[i], 0);
    }
    int nAverage = nTotal / m_members.m_count;
    if (nAverage < 0)
        nAverage = 0;
    else if (nAverage > 100)
        nAverage = 100;
    return nAverage;
}

// Maps each member's m_nAlignment into the buckets GetAlignmentString uses (0..40 -> 3, 41..59 ->
// 1, 60..100 -> 2), averages them, then snaps the mean to the nearest of the two non-neutral
// buckets. A one-member faction short-circuits to the raw bucket so the neutral bucket survives
// the rounding; an empty roster answers 1.
int CSWSFaction::GetAverageGoodEvilAlignment()
{
    if (m_members.m_count < 1)
        return 1;
    int64_t i;
    int nTotal = 0;
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
#if KOTOR1  // K1: through the stats helper (it returns a byte)
        nTotal += static_cast<unsigned char>(pCreature->m_pStats->GetSimpleAlignmentGoodEvil());
#else
        int nAlignment = pCreature->m_pStats->m_nAlignment;
        nTotal += (nAlignment < 41) ? 3 : (1 + (nAlignment > 59));
#endif
    }
    if (m_members.m_count < 1)
        return 1;
    if (m_members.m_count == 1)
        return nTotal;
    int nAverage = nTotal / m_members.m_count;
    if (__builtin_fabsf(static_cast<float>(nAverage) - 2.0f) < __builtin_fabsf(static_cast<float>(nAverage) - 3.0f))
        return 2;
    return 3;
}

int CSWSFaction::GetAverageXP()
{
    if (m_members.m_count < 1)
        return 0;
    int nTotal = 0;
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        nTotal += pCreature->m_pStats->m_nExperience;
    }
    if (m_members.m_count < 1)
        return 0;
    return nTotal / m_members.m_count;
}

// Iterates the members, answering the next member at or after nStartIndex whose player-character
// latch matches bPCOnly, or the invalid-object sentinel. Resuming records the cursor one past the
// member just answered, so the next call picks up from there.
unsigned long long CSWSFaction::GetFactionMember(int nStartIndex, int bPCOnly)
{
    if (m_members.m_count < 1)
        return OBJECT_INVALID;
    if (m_members.m_count <= nStartIndex)
        return OBJECT_INVALID;
    for (int64_t i = nStartIndex; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        if (pCreature->m_pStats->m_bIsPC != bPCOnly)
            continue;
        m_nMemberIterator = static_cast<int>(i + 1);
        return m_members.m_pArray[i];
    }
    return OBJECT_INVALID;
}

// Nonzero unless the roster holds two or more player characters; the early-out is on the second
// one found, so a member with the latch clear never ends the walk.
int CSWSFaction::GetSingletonParty()
{
    int nPCCount = 0;
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        nPCCount += (pCreature->m_bPlayerCharacter != 0);
        if (nPCCount < 2)
            continue;
        return 0;
    }
    return 1;
}

int CSWSFaction::GetMemberList(unsigned long long** ppMembers)
{
    *ppMembers = m_members.m_pArray;
    return m_members.m_count;
}

int CSWSFaction::GetCreatureInParty(unsigned long long gameObjectId)
{
    for (int64_t i = 0; i < m_members.m_count; i++) {
        if (m_members.m_pArray[i] == gameObjectId)
            return 1;
    }
    return 0;
}

// The whole roster is walked and the creature lookups discarded; the answer is always 0.
int CSWSFaction::GetGold()
{
    for (int64_t i = 0; i < m_members.m_count; i++)
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
    return 0;
}

// Moves a member into this faction. An already-present member is a no-op. The second argument
// suppresses only the new-roster broadcast to the existing members; the whole-list ship to the new
// member's client always happens.
void CSWSFaction::AddMember(unsigned long long gameObjectId, int a)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(gameObjectId);
    if (pCreature == 0)
        return;
    CSWSFaction* pOldFaction = pCreature->GetFaction();
    if (GetCreatureInParty(gameObjectId) != 0)
        return;
    if (pOldFaction != 0)
        pOldFaction->RemoveMember(gameObjectId);
    if (a == 0)
        SendFactionUpdateAdd(gameObjectId);
    if (m_members.Contains(gameObjectId) == 0)
        m_members.Add(gameObjectId);
    pCreature->m_pStats->m_factionId = m_nFactionId;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(gameObjectId);
    if (pPlayer != 0)
        pMessage->SendServerToPlayerParty_List(pPlayer->m_nPlayerId, m_members.m_count,
                                               m_members.m_pArray, 1, gameObjectId);
}

// Drops a member from the roster. A player character that leaves is given a brand-new
// single-member faction of its own, and once the old roster empties out the faction itself is
// deleted, but only for a non-NPC faction whose departing member was that player character.
void CSWSFaction::RemoveMember(unsigned long long gameObjectId)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(gameObjectId);
    if (pCreature == 0)
        return;
    if (m_members.Contains(gameObjectId) == 0)
        return;
    m_members.Remove(gameObjectId);
    if (pCreature->m_bPlayerCharacter == 1) {
        CSWSFaction* pNewFaction = new CSWSFaction(gameObjectId);
        pNewFaction->SendFactionUpdateList();
    }
    if (m_members.m_count != 0) {
        SendFactionUpdateRemove(gameObjectId);
    } else {
        CFactionManager* pFactionManager = GetFactionManager();
        if (pFactionManager != 0 && pFactionManager->GetIsNPCFaction(m_nFactionId) == 0 &&
            pCreature->m_pStats->m_bIsPC == 1)
            pFactionManager->DeleteFaction(m_nFactionId);
    }
}

// Builds the empty faction, with no members, no name, iterator at 0 and global on. It still appends
// itself to the faction manager's roster and counts itself as an NPC faction whenever the id it
// lands on is >= 1.
CSWSFaction::CSWSFaction()
{
    CExoArrayList<CSWSFaction*>* pFactions;
    CFactionManager* pFactionManager = GetFactionManager();
    if (pFactionManager != 0) {
        pFactions = pFactionManager->m_paFactions;
        pFactions->Add(this);
        m_nFactionId = pFactionManager->m_paFactions->m_count - 1;
        if (m_nFactionId >= 1)
            pFactionManager->m_nNPCFactionCount = pFactionManager->m_nNPCFactionCount + 1;
    }
    Initialise();
}

// Builds a brand-new faction whose only member is oidCreator, the per-player personal faction. It
// leaves whatever faction the creature was in, appends itself to the faction manager's roster, and
// takes the resulting index as its faction id, pushing that id onto the creature's stats.
CSWSFaction::CSWSFaction(unsigned long long oidCreator)
{
    CExoArrayList<CSWSFaction*>* pFactions;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreator);
    if (pCreature->GetFaction() != 0)
        pCreature->GetFaction()->RemoveMember(oidCreator);
    m_members.Add(oidCreator);
    CFactionManager* pFactionManager = GetFactionManager();
    if (pFactionManager != 0) {
        pFactions = pFactionManager->m_paFactions;
        pFactions->Add(this);
        m_nFactionId = pFactionManager->m_paFactions->m_count - 1;
        pCreature->m_pStats->m_factionId = m_nFactionId;
    }
    Initialise();
}

CSWSFaction::~CSWSFaction() {
}

// One body shape serves the six member-picking queries (the NWScript GetFaction*Member commands):
// walk the roster, keep the best-scoring member found so far, and answer its object id, or the
// invalid-object sentinel when nobody qualifies. Only the score and the direction of the
// comparison differ between them, so each query keeps its own comparison.
static const unsigned long long kOidInvalid = OBJECT_INVALID;

unsigned long long CSWSFaction::GetWeakestMember(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    int nBest = 999;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        int nLevel = pCreature->m_pStats->GetLevel(0);
        if (nBest <= nLevel)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nLevel;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nLevel;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

unsigned long long CSWSFaction::GetStrongestMember(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    int nBest = 0;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        int nLevel = pCreature->m_pStats->GetLevel(0);
        if (nBest >= nLevel)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nLevel;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nLevel;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

unsigned long long CSWSFaction::GetBestAC(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    short nBest = 0;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        short nAC = pCreature->m_pStats->GetArmorClassVersus(0, 0);
        if (nAC <= nBest)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nAC;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nAC;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

unsigned long long CSWSFaction::GetWorstAC(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    short nBest = 999;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        short nAC = pCreature->m_pStats->GetArmorClassVersus(0, 0);
        if (nAC >= nBest)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nAC;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nAC;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

unsigned long long CSWSFaction::GetLeastDamagedMember(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    int nBest = 999;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature->GetDead() != 0)
            continue;
#if KOTOR1  // K1 asks the creature whether it is dying
        if (pCreature->GetDeadTemp() != 0)
#else
        if (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) < 1)
#endif
            continue;
        int nDamaged = pCreature->GetMaxHitPoints(1) - pCreature->GetCurrentHitPoints(0);
        if (nDamaged >= nBest)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nDamaged;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nDamaged;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

unsigned long long CSWSFaction::GetMostDamagedMember(unsigned long long oidCreature, int bMustBeVisible)
{
    if (m_members.m_count < 1)
        return kOidInvalid;
    int64_t i;
    unsigned long long oidResult = kOidInvalid;
    int nBest = -1;
    int bCheck = (oidCreature != kOidInvalid) && (bMustBeVisible != 0);
    for (i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        if (pCreature->GetDead() != 0)
            continue;
#if KOTOR1  // K1 asks the creature whether it is dying
        if (pCreature->GetDeadTemp() != 0)
#else
        if (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) < 1)
#endif
            continue;
        int nDamaged = pCreature->GetMaxHitPoints(1) - pCreature->GetCurrentHitPoints(0);
        if (nDamaged <= nBest)
            continue;
        if (bCheck) {
            unsigned long long oidHit;
            Vector vOut = { 0.0f, 0.0f, 0.0f };
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            if (pOther == 0)
                continue;
            if (pCreature->GetArea() == 0)
                continue;
            if (static_cast<CSWSArea*>(pCreature->GetArea())->ClearLineOfSight(pCreature->m_position,
                    pOther->m_position, &vOut, &oidHit, kOidInvalid, kOidInvalid, 0, 0) != 1)
                continue;
            nBest = nDamaged;
            oidResult = m_members.m_pArray[i];
            continue;
        }
        nBest = nDamaged;
        oidResult = m_members.m_pArray[i];
    }
    return oidResult;
}

// The class held by the most roster members, ties going to the first one seen. Three nested walks:
// every member's every class slot, and for each of those a full re-walk of the roster counting how
// many members carry it.
int CSWSFaction::GetMostFrequentClass()
{
    if (m_members.m_count < 1)
        return 0;
    int nBestClass = 0;
    int nBestCount = 0;
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        for (int64_t j = 0; j < pCreature->m_pStats->m_numClasses; j++) {
            int nClass = pCreature->m_pStats->GetClass(static_cast<unsigned char>(j));
            int nCount = 0;
            for (int64_t k = 0; k < m_members.m_count; k++) {
                CSWSCreature* pOther =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[k]);
                if (pOther == 0)
                    continue;
                for (int64_t l = 0; l < pOther->m_pStats->m_numClasses; l++) {
                    if (pOther->m_pStats->GetClass(static_cast<unsigned char>(l)) == nClass)
                        nCount++;
                }
            }
            if (nCount > nBestCount) {
                nBestCount = nCount;
                nBestClass = nClass;
            }
        }
    }
    return nBestClass;
}

// Hands the leadership over. Only the current leader's client may do it, and the new leader is
// moved to the front of the roster: the member is removed and re-added, then the whole array is
// rotated right so it lands at index 0.
int CSWSFaction::TransferLeadership(unsigned long long partyId, unsigned long long creatureId)
{
    if (GetLeader() != creatureId)
        return 0;
    m_members.Remove(partyId);
    m_members.Add(partyId);
    for (int64_t i = m_members.m_count - 1; i > 0; i--)
        m_members.m_pArray[i] = m_members.m_pArray[i - 1];
    m_members.m_pArray[0] = partyId;
    return 1;
}

// Answers the leader in one of two ways, picked by the faction kind. An NPC faction's leader is
// the member with the highest challenge rating, while a player faction answers the member carrying
// the player-character latch. Both fall back to the invalid-object sentinel.
unsigned long long CSWSFaction::GetLeader()
{
    CFactionManager* pFactionManager = GetFactionManager();
    if (pFactionManager == 0)
        return OBJECT_INVALID;
    if (pFactionManager->GetIsNPCFaction(m_nFactionId) != 0) {
        if (m_members.m_count < 1)
            return OBJECT_INVALID;
        float fBest = 0.0f;
        unsigned long long oidResult = OBJECT_INVALID;
        for (int64_t i = 0; i < m_members.m_count; i++) {
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
            if (pCreature == 0)
                continue;
            float fChallengeRating = pCreature->m_pStats->m_fChallengeRating;
            if (fChallengeRating > fBest) {
                fBest = fChallengeRating;
                oidResult = m_members.m_pArray[i];
            }
        }
        return oidResult;
    }
    if (m_members.m_count < 1)
        return OBJECT_INVALID;
    unsigned long long oidResult = OBJECT_INVALID;
    for (int64_t i = 0; i < m_members.m_count; i++) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_members.m_pArray[i]);
        if (pCreature == 0)
            continue;
        if (pCreature->m_bPlayerCharacter == 0)
            continue;
        return pCreature->m_objectId;
    }
    return oidResult;
}
