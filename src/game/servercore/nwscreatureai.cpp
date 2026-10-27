#include "k2stdint.h"
#include "nwscreature.h"
#include "nwscriptconst.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "itemtype.h"
#include "nwsaiaction.h"
#include "strref.h"

// baseitems.2da rows past nwscript.nss's BASE_ITEM_* list.
enum {
    BASE_ITEM_SONIC_GRENADE_TYPE2 = 97,
};

// The spells.2da row (ITEM_ABILITY_RECOVERY_STIM) whose activate-item property lifts an item's race
// restriction.
enum {
    SPELL_ITEM_ABILITY_RECOVERY_STIM = 129,
};

// The drive action's movement-input bits (m_nDriveFlags): forward, backward (both together run
// forward), the two strafes, and a 30-degree turn counter-clockwise or clockwise.
enum {
    DRIVE_FORWARD      = 1 << 0,
    DRIVE_BACKWARD     = 1 << 1,
    DRIVE_RUN          = DRIVE_FORWARD | DRIVE_BACKWARD,
    DRIVE_STRAFE_LEFT  = 1 << 2,
    DRIVE_STRAFE_RIGHT = 1 << 3,
    DRIVE_TURN_CCW     = 1 << 4,
    DRIVE_TURN_CW      = 1 << 5,
};

// Flag bit AddCastSpellActions ors into the cast action's path-type word for an attack cast.
enum : unsigned int {
    CAST_FLAG_ATTACK = 1u << 30,
};

// spells.2da row of the first wrist-launched grenade; the thrown grenades come before it.
enum {
    SPELL_GRENADE_LAUNCHED_FRAGMENTATION = 246,
};

float fSpringFactor = 0.5f;

int RenderPartyFollow;

int FollowLeaderFullLogging;
int NPCTurbo;
int CreaturePartyIndex;

extern int g_nJournalSortType;
int g_bRunSubAreaScripts = 1;
float GrenadeThrowThreshold = 10.0f;
extern int NPCTurbo;
int AiTimeSlice = CSWSObject::AI_TIME_SLICE;
int bNoForceMoves;
#include "nwsitem.h"
#include "nwsplaceable.h"
#include "nwsitemproperty.h"
#include "nwcobject.h"
#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "nwcreaturelevelstats.h"
#include "nwclass.h"
#include "nwrace.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "nwitem.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwsarea.h"
#include "nwsambientsound.h"
#include "nwsspellimpact.h"
#include "SWCollisionMesh.h"
#include "gameobjectarray.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwssendplayermessage.h"
#include "factionmanager.h"
#include "serverexoappinternal.h"
#include "pathfindinfo.h"
#include "exolinkedlistinternal.h"
#include "nwsobject.h"
#include "worldtimer.h"
#include "nwscombatround.h"
#include "nwsplayer.h"
#include "nwsplayerluo.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "nwmessage.h"
#include "resgff.h"
#include "nwspell.h"
#include "nwscript.h"
#include "nwsstore.h"
#include "nwsencounter.h"
#include "swparty.h"
#include "nwsmodule.h"
#include "nwsjournal.h"
#include "nwsparty.h"
#include "nwsdoor.h"
#include "nwstrigger.h"
#include "serverinfo.h"
#include "nwsforcedaction.h"
#include "clientoptions.h"
#include "globalvartables.h"

extern int g_bDisableDamage;

#include "port/crtapi.h"

#include "port/libcbase.h"
#include "port/libcmath.h"
extern int quaternionidentityoptimization;

extern float g_fInjuredLevel;



#if KOTOR1
int CSWSCreature::CheckItemRaceRestrictions(CSWSItem* pItem) {
    int bRestrictionActive;
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) != 0) {
        CSWItemProperty* pOverride = 0;
        pItem->GetPropertyByType(&pOverride, ITEM_PROPERTY_ACTIVATE_ITEM, SPELL_ITEM_ABILITY_RECOVERY_STIM);
        bRestrictionActive = (pOverride == 0);
    } else {
        bRestrictionActive = 1;
    }

    CSWBaseItem* pBaseItem = pItem->GetBaseItem();
    if (bRestrictionActive != 0 && pBaseItem->m_nRaceRestriction != 0) {
        if (pBaseItem->m_nRaceRestriction == 2) {
            if (m_pStats->m_nRace != CSWRace::CRACE_DROID)
                return 0;
        } else if (pBaseItem->m_nRaceRestriction == 1) {
            if (m_pStats->m_nRace != CSWRace::CRACE_HUMAN)
                return 0;
        } else {
            return 0;
        }
    }

    if ((pItem->GetBaseItem()->m_nDenySubrace & (1 << m_pStats->m_nSubraceIndex)) != 0)
        return 0;

    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_RACIAL_TYPE, 0) == 0)
        return 1;

    for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
        unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_RACIAL_TYPE) {
                pItem->GetPassiveProperty(i);
                if (m_pStats->m_nRace == pItem->GetPassiveProperty(i)->m_subType)
                    return 1;
            }
        }
    }
    return 0;
}
#else
int CSWSCreature::CheckItemRaceRestrictions(CSWSItem* pItem, int bSilent) {
    int bRestrictionActive;
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) != 0) {
        CSWItemProperty* pOverride = 0;
        pItem->GetPropertyByType(&pOverride, ITEM_PROPERTY_ACTIVATE_ITEM, SPELL_ITEM_ABILITY_RECOVERY_STIM);
        bRestrictionActive = (pOverride == 0);
    } else {
        bRestrictionActive = 1;
    }

    CSWBaseItem* pBaseItem = pItem->GetBaseItem();
    if (bRestrictionActive != 0 && pBaseItem->m_nRaceRestriction != 0) {
        if (pBaseItem->m_nRaceRestriction == 2) {
            if (m_pStats->m_nRace != CSWRace::CRACE_DROID)
                return bSilent == 0;
        } else if (pBaseItem->m_nRaceRestriction == 1) {
            if (m_pStats->m_nRace != CSWRace::CRACE_HUMAN)
                return bSilent == 0;
        } else {
            return 0;
        }
    }

    if ((pItem->GetBaseItem()->m_nDenySubrace & (1 << m_pStats->m_nSubraceIndex)) != 0)
        return 0;

    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_RACIAL_TYPE, 0) == 0 &&
        pItem->GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_SUBRACE, 0) == 0)
        return 1;

    for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
        unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_RACIAL_TYPE &&
                m_pStats->m_nRace == pItem->GetPassiveProperty(i)->m_subType)
                return 1;
        }
        nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_LIMIT_USE_BY_SUBRACE &&
                m_pStats->m_nRace != CSWRace::CRACE_DROID &&
                m_pStats->m_nSubraceIndex == pItem->GetPassiveProperty(i)->m_subType)
                return 1;
        }
    }
    return 0;
}
#endif

#if !KOTOR1
int CSWSCreature::CheckItemGenderRestrictions(CSWSItem* pItem)
{
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_GENDER, 0) != 0) {
        for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
            unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
            if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
                if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_LIMIT_USE_BY_GENDER) {
                    unsigned short nGender = pItem->GetPassiveProperty(i)->m_subType;
                    unsigned char nMyGender = GetGender();
                    if (nGender != nMyGender) {
                        if (nGender > 1)
                            return 0;
                        if (nMyGender != 2)
                            return 0;
                    }
                }
            }
        }
    }
    return 1;
}
#endif

int CSWSCreature::CheckItemClassRestrictions(CSWSItem* pItem)
{
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_CLASS,
                                       0) == 0 || m_pStats->HasFeat(FEAT_PROFICIENCY_ALL) != 0)
        return 1;
    for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
        unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_CLASS) {
#if KOTOR1
                unsigned short nClass = pItem->GetPassiveProperty(i)->m_subType;
                for (int j = 0; j < m_pStats->m_numClasses; j++) {
                    if (nClass == m_pStats->GetClass(static_cast<unsigned char>(j)))
                        return 1;
                }
#else
                CSWItemProperty* pProp = pItem->GetPassiveProperty(i);
                for (int j = 0; j < m_pStats->m_numClasses; j++) {
                    if (pProp->m_subType == m_pStats->GetClass(static_cast<unsigned char>(j)))
                        return 1;
                }
#endif
            }
        }
    }
    return 0;
}

int CSWSCreature::CheckItemAlignmentRestrictions(CSWSItem* pItem)
{
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_ALIGNMENT_GROUP,
                                       0) == 0 || m_pStats->HasFeat(FEAT_PROFICIENCY_ALL) != 0)
        return 1;
#if KOTOR1  // K1 tests each property against the creature's simple good/evil alignment.
    unsigned char nSimple = static_cast<unsigned char>(m_pStats->GetSimpleAlignmentGoodEvil());
    for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
        unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_ALIGNMENT_GROUP &&
                pItem->GetPassiveProperty(i)->m_subType != nSimple)
                return 0;
        }
    }
    return 1;
#else
    short nAlignment = m_pStats->m_nAlignment;
    for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
        unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
            if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_ALIGNMENT_GROUP) {
                switch (pItem->GetPassiveProperty(i)->m_subType) {
                case ALIGNMENT_ALL:
                    return 1;
                case ALIGNMENT_NEUTRAL:
                    if (nAlignment >= 41 && nAlignment <= 59)
                        return 1;
                    break;
                case ALIGNMENT_LIGHT_SIDE:
                    if (nAlignment > 59)
                        return 1;
                    break;
                case ALIGNMENT_DARK_SIDE:
                    if (nAlignment < 41)
                        return 1;
                    break;
                case IPRP_ALIGNGRP_DARK_100:
                    if (nAlignment == 0)
                        return 1;
                    break;
                case IPRP_ALIGNGRP_LIGHT_100:
                    if (nAlignment == 100)
                        return 1;
                    break;
                }
            }
        }
    }
    return 0;
#endif
}

int CSWSCreature::TestLocationAgainstEscapeLocation(int a, float fRange, const Vector& vLocation)
{
    int nIndex = (a == 1) << 1;
    if (a == 2)
        nIndex = 1;
    int bResult = nIndex != 0;
    CSWPartyMemberData* pMember =
        &g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex];
    float fX1 = pMember->m_vPathPoint.x - vLocation.x;
    float fY1 = pMember->m_vPathPoint.y - vLocation.y;
    float fZ1 = pMember->m_vPathPoint.z - vLocation.z;
    bResult &= !(fX1 * fX1 + fY1 * fY1 + fZ1 * fZ1 <= fRange * fRange);
    CSWPartyMemberData* pMember2 =
        &g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex];
    float fX2 = pMember2->m_followPoint.x - vLocation.x;
    float fY2 = pMember2->m_followPoint.y - vLocation.y;
    float fZ2 = pMember2->m_followPoint.z - vLocation.z;
    bResult &= !(fX2 * fX2 + fY2 * fY2 + fZ2 * fZ2 <= fRange * fRange);
    return bResult;
}

void CSWSCreature::SetDetectMode(unsigned char mode)
{
    if (m_detectMode == mode)
        return;
    m_detectMode = mode;
    ComputeModifiedMovementRate();
}

void CSWSCreature::SetStealthMode(unsigned char mode, int b)
{
    if (m_stealthMode == mode)
        return;
    m_stealthMode = mode;
    m_bStealthNoAnimation = b;
    ComputeModifiedMovementRate();
}

int CSWSCreature::GetNumInvited()
{
    if (m_pInvitationsOffered != 0)
        return m_pInvitationsOffered->m_count;
    return 0;
}

CSWSFaction* CSWSCreature::GetFaction()
{
    return g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager->GetFaction(m_pStats->m_factionId);
}

int CSWSCreature::GetIsInInvitationsOffered(unsigned long long objectId)
{
    if (m_pInvitationsOffered != 0) {
        for (int i = m_pInvitationsOffered->m_count - 1; i >= 0; i--) {
            if (m_pInvitationsOffered->m_pArray[i].m_objectId == objectId) {
                unsigned int nDay;
                unsigned int nTimeOfDay;
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
                unsigned int nDays;
                unsigned int nTime;
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
                    nDay, nTimeOfDay,
                    m_pInvitationsOffered->m_pArray[i].m_nDayOffered,
                    m_pInvitationsOffered->m_pArray[i].m_nTimeOfDayOffered,
                    &nDays, &nTime);
                if (nTime < 30001)
                    return 1;
                m_pInvitationsOffered->DelIndex(i);
            }
        }
    }
    return 0;
}

int CSWSCreature::GetIsInInvitationsIgnored(unsigned long long objectId)
{
    if (m_pInvitationsIgnored != 0) {
        for (int i = m_pInvitationsIgnored->m_count - 1; i >= 0; i--) {
            if (m_pInvitationsIgnored->m_pArray[i].m_objectId == objectId) {
                unsigned int nDay;
                unsigned int nTimeOfDay;
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
                unsigned int nDays;
                unsigned int nTime;
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
                    nDay, nTimeOfDay,
                    m_pInvitationsIgnored->m_pArray[i].m_nDayOffered,
                    m_pInvitationsIgnored->m_pArray[i].m_nTimeOfDayOffered,
                    &nDays, &nTime);
                if (nTime < 60001)
                    return 1;
                m_pInvitationsIgnored->DelIndex(i);
            }
        }
    }
    return 0;
}

void CSWSCreature::SetVisibilityList(CExoArrayList<CSWVisibilityNode*>* pList)
{
    m_visibilityList.SetSize(0);
    if (pList != 0 && pList->m_count > 0) {
        for (int i = 0; i < pList->m_count; i++)
            m_visibilityList.Add(pList->m_pArray[i]);
    }
}

extern int quaternionidentityoptimization;

float DegToRad(float degrees);

static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    Vector p = v;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = p.x * m00 + p.y * m01 + p.z * m02;
        r.y = p.x * m10 + p.y * m11 + p.z * m12;
        r.z = p.x * m20 + p.y * m21 + p.z * m22;
        p = r;
    }
    return p;
}

// Places the creature at its slot in the party leader's marching formation: the slot's polar
// offset (distance, angle) is rotated about +Z by the leader's yaw and added to the leader's
// position.
void CSWSCreature::SetFormation(unsigned long long oidLeader, int nFormation, int nSlot)
{
    CSWSObject* pLeader = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidLeader));
    if (pLeader == 0)
        return;
    CExoString sAngleColumn;
    sAngleColumn.Format(const_cast<char*>("ANGLE_%d"), nSlot);
    CExoString sDistanceColumn;
    int nAngle;
    int nDistance;
    sDistanceColumn.Format(const_cast<char*>("DISTANCE_%d"), nSlot);
    if (g_pRules->m_p2DArrays->m_pFormations->GetINTEntry(nFormation, sAngleColumn, &nAngle) != 0 &&
        g_pRules->m_p2DArrays->m_pFormations->GetINTEntry(nFormation, sDistanceColumn, &nDistance)
            != 0) {
        Vector vTarget;
        Vector vOffset;
        vOffset.x = static_cast<float>(nDistance);
        vOffset.y = 0.0f;
        vOffset.z = 0.0f;
        Vector vAxis;
        vAxis.x = 0.0f;
        vAxis.y = 0.0f;
        vAxis.z = 1.0f;
        Quaternion qFormation(vAxis,
                              (static_cast<float>(nAngle) + Yaw(pLeader->m_orientation)) * 0.017453292f);
        vTarget = RotateVector(vOffset, qFormation) + pLeader->m_position;
        float fFacingX = vTarget.x + pLeader->m_orientation.x;
        float fFacingY = vTarget.y + pLeader->m_orientation.y;
        float fFacingZ = vTarget.z + pLeader->m_orientation.z;
        AddMoveToPointAction(ACTION_GROUP_NEW, vTarget, pLeader->m_cachedAreaId, OBJECT_INVALID, 1, 0.0f,
                             0.0f,
                             0, 0, 0, 0, 0, 0, Vector(), 0, 0, 0);
        AddAction(AIACTION_CHANGE_FACING_POINT, ACTION_GROUP_PREVIOUS,
                  ACTION_PARAM_FLOAT, &fFacingX, ACTION_PARAM_FLOAT, &fFacingY,
                  ACTION_PARAM_FLOAT, &fFacingZ);
    }
}

void CSWSCreature::UnsummonMyself()
{
    CExoString sName;  // constructed and destroyed only
    g_pAppManager->m_pServerExoApp->GetSWSMessage();
    ClearAllActions(1);
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
        pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                      (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pArea->ApplyEffect(pEffect, m_position, m_orientation);
    }
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
}

// nMode: 0 = every creature, 1 = the party slots only, 2 = all but party-controlled creatures.
// The non-party path prunes m_visibilityList first, then scans the area's X-sorted index.
void CSWSCreature::UpdateVisibleList(unsigned char nMode)
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
        pApp->GetObjectArray();
        pApp->GetServerAIMaster()->m_bRanVisibilityUpdate = 1;
        // The player character keeps perceiving while dead; nobody else does.
        if (m_bPlayerCharacter == 0 && GetDead() == 1)
            return;
        // The winning accessor is called again in its own arm.
        float fRange = (GetSpotCheckDistance() >= GetListenCheckDistance())
                           ? GetSpotCheckDistance()
                           : GetListenCheckDistance();
        float fRangeSq = fRange * fRange;
        if (nMode == 1) {
            if (m_bInParty == 0) {
                CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                if (pParty != 0) {
                    for (int i = 0; i < pParty->m_nCharacterCount; i++) {
                        unsigned long long id =
                            pApp->ClientToServerObjectId(pParty->m_members[i].id);
                        CSWSCreature* pCreature = pApp->GetCreatureByGameObjectID(id);
                        if (pCreature != 0) {
                            Vector vDelta = pCreature->m_position - m_position;
                            // Already on the list, so keep updating it regardless of range.
#if KOTOR1  // K1: no ForceAlwaysUpdate override
                            if (GetVisibleListElement(id) != 0
                                    || vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                               + vDelta.z * vDelta.z <= fRangeSq) {
#else
                            if (GetVisibleListElement(id) != 0
                                    || vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                               + vDelta.z * vDelta.z <= fRangeSq
                                    || pCreature->m_bForceAlwaysUpdate != 0) {
#endif
                                DoPerceptionUpdateOnCreature(pCreature);
                            }
                        }
                    }
                }
            }
        } else {
            // Pass 1 prunes, backward, and m_count is read once since the body may remove entries.
            for (int i = m_visibilityList.m_count - 1; i >= 0; i--) {
                unsigned long long id = m_visibilityList.m_pArray[i]->m_objectId;
                if (id != m_objectId) {
                    CSWSCreature* pCreature = pApp->GetCreatureByGameObjectID(id);
                    if (pCreature == 0) {
                        RemoveFromVisibleList(id);
                    } else if (nMode == 0
                            || (nMode == 2 && pCreature->m_bInParty == 0)) {
                        DoPerceptionUpdateOnCreature(pCreature);
                    }
                }
            }
            // Pass 2 scans the X-sorted spatial index.
            float fMyX = m_position.x;
            int nIndex;
            if (pArea->GetFirstObjectIndiceByX(&nIndex, fMyX - fRange) == 0)
                return;
            float fMaxX = fMyX + fRange;
            for (; nIndex < pArea->m_aObjectsByX.m_count; nIndex++) {
                unsigned long long id = pArea->m_aObjectsByX.m_pArray[nIndex];
                if (id == m_objectId)
                    continue;
                CSWSCreature* pCreature = pApp->GetCreatureByGameObjectID(id);
                if (pCreature == 0)
                    continue;
                // Sorted by X, so the first one past the window ends the whole scan.
                if (pCreature->m_position.x > fMaxX)
                    break;
                // Anyone already on the list was handled by the prune pass.
                CSWVisibilityNode* pNode = GetVisibleListElement(id);
                if (pNode != 0)
                    continue;
                Vector vDelta = pCreature->m_position - m_position;
#if KOTOR1  // K1: no ForceAlwaysUpdate override
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRangeSq) {
#else
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRangeSq
                        || pCreature->m_bForceAlwaysUpdate != 0) {
#endif
                    if (nMode == 0 || (nMode == 2 && pCreature->m_bInParty == 0))
                        DoPerceptionUpdateOnCreature(pCreature);
                }
            }
        }
    }
    m_bSuppressPerceptionEvents = 0;
}

CSWVisibilityNode* CSWSCreature::GetVisibleListElement(unsigned long long id)
{
    int nCount = m_visibilityList.m_count;
    if (nCount > 0) {
        uint64_t n = static_cast<unsigned int>(nCount);
        CSWVisibilityNode** ppNode = m_visibilityList.m_pArray;
        do {
            CSWVisibilityNode* pNode = *ppNode;
            if (pNode->m_objectId == id)
                return pNode;
            ppNode++;
        } while (--n != 0);
    }
    return 0;
}

// Takes pItem out of whatever container holds it, then blanks its possessor. Equipped items are
// unequipped first; the repository used depends on who owns the item.
int CSWSCreature::RemoveItem(CSWSItem* pItem, int a, int b, CSWSItem** ppOut)
{
    if (pItem == 0)
        return 0;
    if (m_pInventory->GetItemInInventory(pItem) == 1) {
        RunUnequip(pItem->m_objectId, OBJECT_INVALID, a, ppOut, 0, 0);
        if (ppOut != 0)
            pItem = *ppOut;
    }
    int bOwnRepository = pItem->m_possessorId == m_objectId;
#if !KOTOR1  // K1 has no shared-party-inventory fallback.
    if (!bOwnRepository) {
        CGameObject* pPossessor =
            g_pAppManager->m_pServerExoApp->GetGameObject(pItem->m_possessorId);
        CGameObject* pSelf = g_pAppManager->m_pServerExoApp->GetGameObject(m_objectId);
        bOwnRepository = pPossessor != 0 && pPossessor->AsSWSCreature() != 0
                         && pPossessor->AsSWSCreature()->m_bInParty != 0
                         && pSelf != 0 && pSelf->AsSWSCreature() != 0
                         && pSelf->AsSWSCreature()->m_bInParty != 0;
    }
#endif
    if (bOwnRepository) {
        if (GetItemRepository(1)->GetItemInRepository(pItem, 0) == 1)
            GetItemRepository(1)->RemoveItem(pItem);
    } else {
        CGameObject* pContainer =
            g_pAppManager->m_pServerExoApp->GetGameObject(pItem->m_possessorId);
        if (pContainer == 0
                || pContainer->m_objectType != CGameObject::OBJECT_TYPE_ITEM)
            return 0;
        CSWSItem* pContainerItem = static_cast<CSWSItem*>(pContainer->AsSWSItem());
        if (pContainerItem->m_possessorId != m_objectId)
            return 0;
        pContainerItem->m_pItemRepository->RemoveItem(pItem);
    }
    pItem->SetPossessor(OBJECT_INVALID, 1, b, OBJECT_INVALID);
    return 1;
}

// Switches the creature's combat mode. Parry and the two power attacks are refused when the
// equipped weapon is ranged, rapid shot needs a ranged weapon and flurry needs an empty weapon
// slot. bForce == 1 skips the mid-round deferral and the no-op check.
void CSWSCreature::SetCombatMode(unsigned char nMode, int bForce)
{
    unsigned char nOldMode = m_combatMode;
    if (bForce == 1) {
        if (nMode == CREATURE_COMBAT_MODE_PARRY || nMode == CREATURE_COMBAT_MODE_POWER_ATTACK ||
            nMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK) {
            CSWSItem* pItem =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pItem != 0
                    && g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon != 0) {
                SendFeedbackMessage(FEEDBACK_COMBAT_MODE_WRONG_WEAPON, 0);
                return;
            }
        } else if (nMode == CREATURE_COMBAT_MODE_RAPID_SHOT) {
            CSWSItem* pItem =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pItem == 0
                    || g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon == 0) {
                SendFeedbackMessage(FEEDBACK_COMBAT_MODE_WRONG_WEAPON, 0);
                return;
            }
        } else if (nMode == CREATURE_COMBAT_MODE_FLURRY) {
            if (m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) != 0) {
                SendFeedbackMessage(FEEDBACK_COMBAT_MODE_WRONG_WEAPON, 0);
                return;
            }
        }
    } else if (nMode != CREATURE_COMBAT_MODE_COUNTERSPELL ||
               nOldMode == CREATURE_COMBAT_MODE_COUNTERSPELL) {
        if (nOldMode == nMode)
            return;
        if (m_pCombatRound->m_bRoundStarted == 1) {
            m_pendingCombatMode = nMode;
            return;
        }
        if (nMode == CREATURE_COMBAT_MODE_RAPID_SHOT) {
            CSWSItem* pItem =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pItem == 0
                    || g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon == 0) {
                SendFeedbackMessage(FEEDBACK_COMBAT_MODE_WRONG_WEAPON, 0);
                return;
            }
        } else if (nMode == CREATURE_COMBAT_MODE_FLURRY) {
            if (m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) != 0) {
                SendFeedbackMessage(FEEDBACK_COMBAT_MODE_WRONG_WEAPON, 0);
                return;
            }
        }
        nOldMode = m_combatMode;
    }
    m_combatMode = nMode;
    if (nMode == CREATURE_COMBAT_MODE_PARRY || nOldMode == CREATURE_COMBAT_MODE_PARRY)
        SetActivity(ACTIVITY_PARRY, nMode == CREATURE_COMBAT_MODE_PARRY, 0);
    else if (nMode == CREATURE_COMBAT_MODE_POWER_ATTACK ||
             nOldMode == CREATURE_COMBAT_MODE_POWER_ATTACK)
        SetActivity(ACTIVITY_POWER_ATTACK, nMode == CREATURE_COMBAT_MODE_POWER_ATTACK, 0);
    else if (nMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK ||
             nOldMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK)
        SetActivity(ACTIVITY_IMPROVED_POWER_ATTACK,
                    nMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK, 0);
    else if (nMode == CREATURE_COMBAT_MODE_COUNTERSPELL ||
             nOldMode == CREATURE_COMBAT_MODE_COUNTERSPELL)
        SetActivity(ACTIVITY_COUNTERSPELL, nMode == CREATURE_COMBAT_MODE_COUNTERSPELL, 0);
    else if (nMode == CREATURE_COMBAT_MODE_FLURRY || nOldMode == CREATURE_COMBAT_MODE_FLURRY)
        SetActivity(ACTIVITY_FLURRY, nMode == CREATURE_COMBAT_MODE_FLURRY, 0);
    else if (nMode == CREATURE_COMBAT_MODE_RAPID_SHOT ||
             nOldMode == CREATURE_COMBAT_MODE_RAPID_SHOT)
        SetActivity(ACTIVITY_RAPID_SHOT, nMode == CREATURE_COMBAT_MODE_RAPID_SHOT, 0);
}

int CSWSCreature::CheckUseMagicDeviceSkill(CSWSItem* pItem, int a)
{
    K2_UNUSED(pItem); K2_UNUSED(a);
    return 0;
}

int CSWSCreature::AIActionAnimalEmpathy(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    return ACTION_FAILED;
}

int CSWSCreature::AIActionPickPocket(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    return ACTION_FAILED;
}

int CSWSCreature::AIActionRest(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionTaunt(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    return ACTION_FAILED;
}

int CSWSCreature::AIActionAppear(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    unsigned int nDays;
    unsigned int nTime;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nActionStartDay, m_nActionStartTod, &nDays, &nTime);
    if (nTime <= 1999) {
        m_fAnimationSpeed = 1.0f;
        SetAnimation(SERVER_ANIMATION_APPEAR);
        return ACTION_IN_PROGRESS;
    }
    if (AsSWSCreature() != 0) {
        m_fAnimationSpeed = 1.0f;
        SetAnimation(SERVER_ANIMATION_READY);
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionDisappear(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    unsigned int nDays;
    unsigned int nTime;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nActionStartDay, m_nActionStartTod, &nDays, &nTime);
    if (nTime <= 1999) {
        m_fAnimationSpeed = 1.0f;
        SetAnimation(SERVER_ANIMATION_DISAPPEAR);
        return ACTION_IN_PROGRESS;
    }
    if (AsSWSCreature() != 0) {
        m_fAnimationSpeed = 1.0f;
        SetAnimation(SERVER_ANIMATION_READY);
    }
    return ACTION_COMPLETE;
}

// Heals over three ticks, walking into use range, playing the treat-injury animation, then
// resolving the skill check.
int CSWSCreature::AIActionHeal(CSWSObjectActionNode* pNode)
{
    unsigned long long oidTarget;
    unsigned long long oidItem;
    int    nSkillParam;
    int    bMayApproach;
    int    nDC;
    Vector vDest;
    float  fRange;
    int    nOneA;
    int    nOneB;

    // GetDead()'s result is discarded; no death test here.
#if KOTOR1  // K1 uses GetIsPCDying/GetDeadTemp; GetDead() only gates GetIsPCDying().
    if ((GetDead() == 1 && GetIsPCDying() == 1) || GetDeadTemp() == 1)
        return ACTION_FAILED;
#else
    GetDead();
    if (GetDeadTemp() == 1)
        return ACTION_FAILED;
#endif

    ActivityManager(2);

    oidTarget    = pNode->m_args.m_aqTargetId;
    oidItem      = pNode->m_args.m_healItemId;
    nSkillParam       = pNode->m_args.m_healSpellId;
    bMayApproach = pNode->m_args.m_bHealMayApproach;

    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget == 0) {
        m_bHealAnimPlayed = 0;
        return ACTION_FAILED;
    }
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0) {
        m_bHealAnimPlayed = 0;
        return ACTION_FAILED;
    }

    vDest.x = 0.0f;
    vDest.y = 0.0f;
    vDest.z = 0.0f;

    if (GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0 && bMayApproach != 0) {
        // Out of reach and still allowed to close the distance, re-queue the heal behind a walk to
        // the patient's use point, with the approach permission spent so this happens only once.
        nOneA = 1;
        nOneB = 1;
        bMayApproach = 0;
        AsSWSCreature()->GetUseRange(oidTarget, vDest, fRange, 0, 0);
        AddActionToFront(AIACTION_HEAL, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidTarget,
                         ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_INT, &nSkillParam,
                         ACTION_PARAM_INT, &bMayApproach, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nOneA,
                         ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                         ACTION_PARAM_INT, &nOneB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, oidTarget, nOneA,
                                    fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    char nSkillRank = m_pStats->GetSkillRank(SKILL_TREAT_INJURY, pTarget->AsSWSObject(), 0);

    if (m_bHealAnimPlayed == 0) {
        // In range on the first tick, kneel and play the treat-injury animation, re-queueing the
        // heal behind it so the resolution happens when the animation finishes.
        m_bHealAnimPlayed = 1;
        int   nAnimation = SERVER_ANIMATION_CAST_1;
        int   nOne       = 1;
        float fSpeed     = 1.0f;
        float fDuration  = 2.0f;
        AddActionToFront(AIACTION_HEAL, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidTarget,
                         ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_INT, &nSkillParam,
                         ACTION_PARAM_INT, &bMayApproach, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        return ACTION_COMPLETE;
    }

    int nSkill = nSkillRank;
    unsigned short nDieRoll = g_pRules->RollDice(1, 20);
    int nCombatState = m_bCombatState;
    int nRoll       = (nCombatState == 1) ? nDieRoll : 20;
    int bTakeTwenty = (nCombatState != 1);

    CGameEffect* pEffect;
    for (int i = 0; i < pTarget->m_effects.m_count; i++) {
        CGameEffect* pEff = pTarget->m_effects.m_pArray[i];
        if (pEff->m_subType > CSWSEffectListHandler::EFFECT_POISON)
            break;
        if (pEff->m_subType == CSWSEffectListHandler::EFFECT_POISON ||
            pEff->m_subType == CSWSEffectListHandler::EFFECT_DISEASE) {
            pEffect = pEff;
            goto foundAilment;
        }
    }
    nDC = 0;
    goto noAilment;

foundAilment:
    nDC = 0;
    if (pEffect != 0) {
        // The save DC comes from the ailment's own 2DA row (poison.2da "Save_DC" or disease.2da
        // "Subs_Save"), indexed by the effect's level.
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_POISON)
            g_pRules->m_p2DArrays->m_pPoison->GetINTEntry(pEffect->GetInteger(0),
                                                         CExoString("Save_DC"), &nDC);
        else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DISEASE)
            g_pRules->m_p2DArrays->m_pDisease->GetINTEntry(pEffect->GetInteger(0),
                                                          CExoString("Subs_Save"), &nDC);
        nDC = nDC > 1 ? nDC : 1;
        if (nRoll + nSkill >= nDC)
            pTarget->RemoveEffectById(pEffect->m_effectId);
        goto haveOutcome;
    }

noAilment:
    // Nothing to cure, so this is a plain heal, and a plain heal on someone already at full
    // health is refused outright.
    SendFeedbackMessage(FEEDBACK_HEAL_NO_AILMENT, 0);
    if (pTarget->GetCurrentHitPoints(0) >= pTarget->GetMaxHitPoints(1)) {
        SendFeedbackMessage(FEEDBACK_HEAL_TARGET_UNHURT, 0);
        return ACTION_FAILED;
    }

haveOutcome:

        // Tell the clients how the skill check went; payload integer 5 carries the outcome code.
    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetInteger(0, STRREF_TREAT_INJURY);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, bTakeTwenty);
    int nTotal = nRoll + nSkill;
    pData->SetInteger(5, (nTotal < nDC) ? ((nCombatState == 1) ? 0 : 3) : 1);
    pData->SetInteger(6, STRREF_TREAT_INJURY);
    BroadcastSkillData(pData);

    // Spend the medpac by splitting one off a stack, or removing and destroying the last one.
    if (GetItemRepository(1)->GetItemInRepository(pItem, 1) == 1) {
        if (pItem->m_stackCount >= 2) {
            pItem->m_stackCount = pItem->m_stackCount - 1;
        } else if (RemoveItem(pItem, 1, 1, 0) != 0) {
            CSWSPlayer* pPlayer =
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
            if (pPlayer != 0)
                g_pAppManager->m_pServerExoApp->GetSWSMessage()
                    ->SendServerPlayerItemUpdate_DestroyItem(pPlayer, pItem->m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, pItem->m_objectId,
                CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
    }

    CGameEffect* pHeal = new CGameEffect(1);
    pHeal->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HEAL);
    pHeal->m_durationTypeBits =
        (pHeal->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
    pHeal->SetCreator(m_objectId);
    pHeal->SetInteger(0, nTotal);
    pTarget->ApplyEffect(pHeal, 0, 0);

    CGameEffect* pVfx = new CGameEffect(1);
    pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pVfx->m_durationTypeBits =
        (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
    pVfx->SetCreator(m_objectId);
    pVfx->SetInteger(0, VFX_IMP_HEALING_SMALL);
    pTarget->ApplyEffect(pVfx, 0, 0);

    m_bHealAnimPlayed = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionEquipItem(CSWSObjectActionNode* pNode)
{
    ActivityManager(2);
    if (pNode->m_nNumParameters < 3)
        return ACTION_FAILED;
    unsigned long long itemId = pNode->m_args.m_itemId;
    unsigned int nSlot = pNode->m_args.m_equipSlot;
    int bEquipFlag = pNode->m_args.m_equipFlag;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 0 && GetIsPCDying() == 0 && GetDeadTemp() == 0 &&
#else
    if (GetDead() == 0 &&
        GetDeadTemp() == 0 &&
#endif
        RunEquip(itemId, nSlot, bEquipFlag) == 1)
        return ACTION_COMPLETE;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, itemId, nSlot);
    return ACTION_FAILED;
}

int CSWSCreature::AIActionUnequipItem(CSWSObjectActionNode* pNode)
{
    ActivityManager(2);
#if KOTOR1  // K1: dying tests GetIsPCDying()/GetDeadTemp(), and RunUnequip's third argument is 1.
    unsigned long long itemId = pNode->m_args.m_itemId;
    unsigned long long itemId2 = pNode->m_args.m_unequipContainerId;
    int nEquipFlag = pNode->m_args.m_equipFlag;
    if (GetDead() == 0 && GetIsPCDying() == 0 && GetDeadTemp() == 0 &&
        RunUnequip(itemId, itemId2, 1, 0, nEquipFlag, 0) == 1)
        return ACTION_COMPLETE;
#else
    unsigned long long itemId = pNode->m_args.m_itemId;
    unsigned long long itemId2 = pNode->m_args.m_unequipContainerId;
    int nEquipFlag = pNode->m_args.m_equipFlag;
    int bUnequipMessageFlag = pNode->m_args.m_bCheckMoveAwayRun;
    int bStack = pNode->m_args.m_nCheckMoveToObjParam;
    if (GetDead() == 0 &&
        GetDeadTemp() == 0 &&
        RunUnequip(itemId, itemId2, bStack, 0, nEquipFlag, bUnequipMessageFlag) == 1)
        return ACTION_COMPLETE;
#endif
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, itemId);
    return ACTION_FAILED;
}

int CSWSCreature::AIActionChangeFacingPoint(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    Vector vDelta;
    vDelta.x = pNode->m_args.m_facingX - m_position.x;
    vDelta.y = pNode->m_args.m_facingY - m_position.y;
    vDelta.z = 0.0f;
    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z >= 0.00001f) {
        Vector vDir = normalize(vDelta);
        SetOrientation(vDir);
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::CheckVisibleList(unsigned long long id)
{
    int nCount = m_visibilityList.m_count;
    if (nCount > 0) {
        uint64_t n = static_cast<unsigned int>(nCount);
        CSWVisibilityNode** ppNode = m_visibilityList.m_pArray;
        do {
            if ((*ppNode)->m_objectId == id)
                return 1;
            ppNode++;
        } while (--n != 0);
    }
    return 0;
}

#if !KOTOR1
int CSWSCreature::CheckItemPCRestrictions(CSWSItem* pItem)
{
    CExoString sExpectedTag;
    int bRet = 1;
    int nMatching = 0;
    int bTagOk = 0;
    unsigned char nSlot;
    unsigned short nRow;
    int bOk;
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_PC, 0) != 0) {
        if (pItem->m_nPassiveProperties >= 1) {
            for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
                nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
                if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
                    if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_LIMIT_USE_BY_PC) {
                        nRow = pItem->GetPassiveProperty(i)->m_subType;
                        nMatching++;
                        if (nRow == 0) {
                            bOk = m_bPlayerMadeCharacter != 0;
                        } else {
                            g_pRules->m_p2DArrays->m_pIPRPPC->GetCExoStringEntry(
                                nRow, CExoString("ExpectedTag"), &sExpectedTag);
                            bOk = _stricmp(sExpectedTag.CStr(), m_sTag.CStr()) == 0;
                        }
                        bTagOk |= bOk;
                    }
                }
            }
        }
        bRet = (bTagOk | (nMatching < 1)) & 1;
    }
    return bRet;
}
#endif

#if KOTOR1
int CSWSCreature::CanUseItem(CSWSItem* pItem, int a)
#else
int CSWSCreature::CanUseItem(CSWSItem* pItem, int a, int bSilent)
#endif
{
    K2_UNUSED(a);
    int bOk = 0;
    if (pItem != 0) {
        if (CheckItemAlignmentRestrictions(pItem) != 0 &&
            CheckItemClassRestrictions(pItem) != 0 &&
#if KOTOR1
            CheckItemRaceRestrictions(pItem) != 0) {
            bOk = CheckItemFeatRestrictions(pItem) != 0;
#else
            CheckItemRaceRestrictions(pItem, bSilent) != 0) {
            CSWBaseItem* pBaseItem = static_cast<CSWItem*>(pItem)->GetBaseItem();
            if (bSilent != 0 &&
                (pBaseItem->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT || pBaseItem->m_nItemType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT) &&
                GetCurrentHitPoints(0) == GetMaxHitPoints(1)) {
                bOk = 0;
            } else if (CheckItemFeatRestrictions(pItem) != 0 &&
                       CheckItemGenderRestrictions(pItem) != 0 &&
                       CheckItemPCRestrictions(pItem) != 0 &&
                       CheckItemAttributeRestrictions(pItem) != 0) {
                bOk = CheckItemCNPCRestrictions(pItem) != 0;
            }
#endif
        }
        if (m_bPlayerCharacter == 1) {
            unsigned char nLevel = m_pStats->GetLevel(0);
            unsigned char nMinLevel = pItem->GetMinEquipLevel();
            if (nLevel < nMinLevel) {
                CServerInfo* pServerInfo = g_pAppManager->m_pServerExoApp->GetServerInfo();
                if (bOk == 0)
                    return 0;
                if (pServerInfo->m_bItemLevelRestrictions != 0)
                    return 0;
            }
        }
        if (bOk == 0)
            return 0;
        CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
        int bUsable;
        if (pBaseItem == 0 || pBaseItem->m_weaponWieldType == 8 ||
            pBaseItem->m_nWeaponType == 0) {
            bUsable = 1;
        } else {
            bUsable = 0;
            CSWBaseItem* pBaseItem2 = g_pRules->GetBaseItem(pItem->m_baseItem);
            if (static_cast<int>(pBaseItem2->m_nWeaponSize - m_nCreatureSize) < 2)
                bUsable = bOk;
        }
        bOk = bUsable;
#ifdef K2_FIX_ENGINE_BUGS
        if (pBaseItem == 0)
            return bOk;
#endif
        if ((pBaseItem->m_nEquipableSlots & ((1 << NUM_INVENTORY_SLOTS) - 1)) != 0) {
            if (CheckProficiencies(pItem, 0) == 0)  // the slot argument is unused in the callee
                bOk = 0;
        }
    }
    return bOk;
}

#if !KOTOR1
__attribute__((noinline))
int CSWSCreature::CheckItemCNPCRestrictions(CSWSItem* pItem)
{
    if (m_sTag.CompareNoCase("baodur") == 0)
        return 1;
    int nBaseItem = pItem->m_baseItem;
    unsigned int nDelta = nBaseItem - BASE_ITEM_SONIC_GRENADE_TYPE2;
    if (nDelta <= 5 && nDelta != 4)
        return 0;
    if (static_cast<unsigned int>(nBaseItem - BASE_ITEM_JEDI_ROBE) < 3)
        return 0;
    return 1;
}
#endif

void CSWSCreature::UnpossessCreature()
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer == 0)
        return;
    if (pPlayer->m_pLastUpdateObject != 0)
        pPlayer->m_pLastUpdateObject->ClearVisibilityList();
    if (pPlayer->m_pLastUpdateObject != 0)
        pPlayer->m_pLastUpdateObject->ClearActionQueue();
}

void CSWSCreature::PossessCreature(unsigned long long objectId)
{
    if (objectId == OBJECT_INVALID)
        return;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
    if (pPlayer == 0)
        return;
    if (pPlayer->m_pLastUpdateObject != 0)
        pPlayer->m_pLastUpdateObject->ClearVisibilityList();
    if (pPlayer->m_pLastUpdateObject != 0)
        pPlayer->m_pLastUpdateObject->ClearActionQueue();
}

void CSWSCreature::TerminateClientSidePath(int a)
{
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0) {
        CLastUpdateObject* pLastObject = pPlayer->GetLastUpdateObject(m_objectId);
        if (pLastObject != 0) {
            pLastObject->m_orientation = Vector();
            pLastObject->m_nAnimation = 0xffff;
            pLastObject->m_nClientPathId = static_cast<unsigned char>(a - 1);
        }
    }
}

int CSWSCreature::AIActionAreaWait(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    if (m_bPlayerCharacter != 0) {
        if (GetArea() != 0)
            return ACTION_COMPLETE;
        return ACTION_IN_PROGRESS;
    }
    return ACTION_FAILED;
}

int CSWSCreature::AIActionSurrenderToEnemies(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    if (m_bPlayerCharacter == 1)
        return ACTION_FAILED;
    SurrenderToEnemies(0);
    return ACTION_COMPLETE;
}

void CSWSCreature::SurrenderToEnemies(int a)
{
    CancelCombat(0);
    ClearAllEffects(a);
    Vector vPos = m_position;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        float fMaxDist = CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE;
        int nIndex;
        if (pArea->GetFirstObjectIndiceByX(&nIndex, vPos.x - fMaxDist) == 1) {
            while (nIndex < pArea->m_aObjectsByX.m_count) {
                CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp
                    ->GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[nIndex]);
                if (pCreature != 0 && m_objectId != pCreature->m_objectId) {
                    if (pCreature->m_position.x > vPos.x + fMaxDist)
                        break;
                    float fDx = pCreature->m_position.x - vPos.x;
                    float fDy = pCreature->m_position.y - vPos.y;
                    float fDz = pCreature->m_position.z - vPos.z;
                    if (fDx * fDx + fDy * fDy + fDz * fDz <= fMaxDist * fMaxDist
                            && GetReputation(pCreature->m_objectId, 0) <= 10) {
                        pCreature->CancelCombat(0);
                        pCreature->ClearAllEffects(a);
                    }
                }
                nIndex++;
            }
        }
    }
    if (g_pAppManager->m_pServerExoApp->GetFactionManager() != 0
            && g_pAppManager->m_pServerExoApp->GetFactionManager()->m_pFactionNeutral != 0) {
        g_pAppManager->m_pServerExoApp->GetFactionManager()->m_pFactionNeutral->AddMember(
            m_objectId, 0);
    }
}

int CSWSCreature::GetStandardFactionReputation(int nFaction)
{
    if (m_pStats->m_bIsPC != 0) {
        int nReputation = -1;
        CExoArrayList<int>* pReputations = m_pStandardFactionReputations;
        if (nFaction >= 0 && pReputations != 0 && pReputations->m_count > nFaction)
            nReputation = pReputations->m_pArray[nFaction - 1];
        return nReputation;
    }
    CServerExoAppInternal* pInternal = g_pAppManager->m_pServerExoApp->m_pInternal;
    if (pInternal == 0)
        return -1;
    return pInternal->m_pFactionManager->GetNPCFactionReputation(nFaction, m_pStats->m_factionId);
}

void CSWSCreature::SetStandardFactionReputation(int nFaction, int nRep)
{
    if (nRep > 100)
        nRep = 100;
    if (nRep < 0)
        nRep = 0;
    if (m_pStats->m_bIsPC != 0) {
        if (nFaction >= 0 && m_pStandardFactionReputations != 0
            && nFaction < m_pStandardFactionReputations->m_count) {
            m_pStandardFactionReputations->m_pArray[nFaction - 1] = nRep;
        }
        return;
    }
    CServerExoAppInternal* pInternal = g_pAppManager->m_pServerExoApp->m_pInternal;
    if (pInternal != 0)
        pInternal->m_pFactionManager->SetNPCFactionReputation(m_pStats->m_factionId, nFaction,
                                                              nRep);
}

CSWSObjectActionNode* CSWSCreature::GetAction(unsigned int nAction)
{
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    do {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
        if (pAction->m_nActionId == static_cast<int>(nAction))
            return pAction;
        m_actionList.m_pInternal->GetNext(pNode);
    } while (pNode != 0);
    return 0;
}

int CSWSCreature::AIActionWaitForEndOfRound(CSWSObjectActionNode* pActionNode)
{
    K2_UNUSED(pActionNode);
    if (GetDead() == 1)
        return ACTION_FAILED;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() == 1 || GetDeadTemp() == 1)
        return ACTION_FAILED;
#else
    if (GetDeadTemp() == 1)
        return ACTION_FAILED;
#endif
    if (m_pCombatRound->m_bRoundStarted == 1 && m_pCombatRound->GetSpellActionPending() == 0)
        return ACTION_IN_PROGRESS;
    return ACTION_COMPLETE;
}

void CSWSCreature::AdjustReputation(int nFaction, int nAdjustment)
{
    CFactionManager* pFactionManager =
        g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
    if (pFactionManager->GetIsNPCFaction(nFaction)) {
        int nReputation = pFactionManager->GetNPCFactionReputation(m_pStats->m_factionId, nFaction);
        if (m_pStats->m_factionId != nFaction)
            pFactionManager->SetNPCFactionReputation(m_pStats->m_factionId, nFaction,
                                                     nReputation + nAdjustment);
    }
}

int CSWSCreature::HasAction(unsigned int nActionId)
{
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    do {
        CSWSObjectActionNode* pAction =
            static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
        if (pAction->m_nActionId == static_cast<int>(nActionId))
            return 1;
        m_actionList.m_pInternal->GetNext(pNode);
    } while (pNode != 0);
    return 0;
}

int CSWSCreature::ReplyToInvitation(int nResponse, unsigned long long creatureId)
{
    int bReplied = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(creatureId);
    if (pCreature != 0 && pCreature->GetIsInInvitationsOffered(m_objectId) == 1 &&
        m_oidInvitedToPartyBy == creatureId) {
        if (nResponse == 2) {
            AddToInvitationsIgnored(creatureId);
        } else if (nResponse != 1 && nResponse != 0) {
            return 0;
        } else if (nResponse == 0) {
            // A null faction aborts without any cleanup.
            if (g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                    ->GetFaction(pCreature->m_pStats->m_factionId) == 0)
                return 0;
            g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                ->GetFaction(pCreature->m_pStats->m_factionId)
                ->AddMember(m_objectId, 0);
        }
        pCreature->RemoveFromInvitationsOffered(m_objectId);
        SetInvitedToParty(0);
        m_oidInvitedToPartyBy = OBJECT_INVALID;
        bReplied = 1;
    }
    return bReplied;
}

void CSWSCreature::RemoveFromInvitationsOffered(unsigned long long objectId)
{
    if (m_pInvitationsOffered != 0) {
        for (int i = m_pInvitationsOffered->m_count - 1; i >= 0; i--) {
            if (m_pInvitationsOffered->m_pArray[i].m_objectId == objectId)
                m_pInvitationsOffered->DelIndex(i);
        }
    }
}

int CSWSCreature::GetCreatureReputation(unsigned long long objectId, int nReputation)
{
    int nFaction = m_pStats->m_factionId;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0 && m_bInParty != 0 && pCreature->m_bInParty != 0)
        return 100;
    if (m_bInParty != 0)
        nFaction = 0;
    if (pCreature != 0 && pCreature->m_bInParty != 0)
        nReputation = 0;
    return g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
        ->GetNPCFactionReputation(nReputation, nFaction);
}

int CSWSCreature::AIActionOrientCamera(CSWSObjectActionNode* pNode)
{
    int   nMask = pNode->m_args.m_cameraMask;
    float fX    = pNode->m_args.m_cameraX;
    float fY    = pNode->m_args.m_cameraY;
    float fZ    = pNode->m_args.m_cameraZ;
    int   nMode = pNode->m_args.m_cameraMode;
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0) {
        CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        if (pMessage != 0) {
            pMessage->SendServerToPlayerCamera_ChangeLocation(pPlayer, nMask, fX, fY, fZ, nMode);
        }
    }
    return ACTION_COMPLETE;
}

// Runs the pick-up-item action (id 7), a walk-to, then the reach animation, then AcquireItem.
int CSWSCreature::AIActionPickUpItem(CSWSObjectActionNode* pNode)
{
    unsigned long long oidItem;
    unsigned long long oidDest;
    unsigned char bNotify;
    CSWSItem* pItem;
    Vector vDest;
    Vector vSafe;

    ActivityManager(2);

    oidItem = pNode->m_args.m_pickUpItemId;
    oidDest = pNode->m_args.m_pickUpDestId;
    bNotify = pNode->m_args.m_bPickUpNotify;

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
    pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);

#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() != 1 && GetIsPCDying() != 1 && GetDeadTemp() != 1 && pItem != 0)
    {
#else
    if (GetDead() != 1)
    {
        int bIncapacitated = GetDeadTemp();
        if (bIncapacitated != 0 || pItem == 0)
        {
            if (pMessage != 0 && pPlayer != 0)
                pMessage->SendServerToPlayerInventory_PickupCancel(pPlayer->m_nPlayerId, oidItem);
            m_bPickUpItemStarted = 0;
            return ACTION_FAILED;
        }
#endif

        if (pItem->m_possessorId != OBJECT_INVALID)
            return ACTION_FAILED;

        float fDX = m_position.x - pItem->m_position.x;
        float fDY = m_position.y - pItem->m_position.y;
        float fDZ = m_position.z - pItem->m_position.z;
        if (fDX * fDX + fDY * fDY + fDZ * fDZ > 1.21f)
        {
            AddActionToFront(AIACTION_PICK_UP_ITEM, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_OBJECT, &oidDest,
                             ACTION_PARAM_INT, &bNotify, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidItem, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            Vector vDir = normalize(m_position - pItem->m_position);
            vDest = pItem->m_position + vDir;
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            vDest = (pArea->ComputeSafeLocation(vDest, 0.5f, m_pPathfindInfo, 1, &vSafe) == 1)
                        ? vSafe
                        : pItem->m_position;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, OBJECT_INVALID, 1,
                                        0.0f, 0.0f, 0, 0, 0, 0, 0, 0, Vector(), 0, 0, 0);
            return ACTION_COMPLETE;
        }

        if (m_bPickUpItemStarted != 0)
        {
            if (AcquireItem(&pItem, OBJECT_INVALID, oidDest, bNotify) == 1)
            {
                if (pPlayer != 0 && pMessage != 0)
                    pMessage->SendServerToPlayerInventory_Pickup(pPlayer->m_nPlayerId, oidItem);
                m_bPickUpItemStarted = 0;
                return ACTION_COMPLETE;
            }
            if (pPlayer != 0 && pMessage != 0)
                pMessage->SendServerToPlayerInventory_PickupCancel(pPlayer->m_nPlayerId, oidItem);
            m_bPickUpItemStarted = 0;
            return ACTION_FAILED;
        }

        m_bPickUpItemStarted = 1;
        int   nAnimation = (vDest.z > 0.0f) ? SERVER_ANIMATION_GET_MID : SERVER_ANIMATION_GET_LOW;
        int   nOne       = 1;
        float fSpeed     = 1.0f;
        float fDuration  = 1.5f;
        AddActionToFront(AIACTION_PICK_UP_ITEM, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidItem,
                         ACTION_PARAM_OBJECT, &oidDest, ACTION_PARAM_INT, &bNotify, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidItem, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    if (pMessage != 0 && pPlayer != 0)
        pMessage->SendServerToPlayerInventory_PickupCancel(pPlayer->m_nPlayerId, oidItem);
    m_bPickUpItemStarted = 0;
    return ACTION_FAILED;
}

// Runs the drop-item action (id 8), the twin of AIActionPickUpItem, dropping onto the packed point
// or into a container the creature owns.
int CSWSCreature::AIActionDropItem(CSWSObjectActionNode* pNode)
{
    ActivityManager(2);

    unsigned long long oidItem = pNode->m_args.m_dropItemId;
    float fX = pNode->m_args.m_dropPointX;
    float fY = pNode->m_args.m_dropPointY;
    float fZ = pNode->m_args.m_dropPointZ;
    Vector vDrop;
    vDrop.x = fX;
    vDrop.y = fY;
    vDrop.z = fZ;
    int   bDropGold = pNode->m_args.m_bDropGold;

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);

    if (GetDead() != 1)
    {
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
        if (GetIsPCDying() != 1 && GetDeadTemp() != 1)
#else
        if (GetDeadTemp() != 1)
#endif
        {
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
            CItemRepository* pRepository = 0;
            int bOwned = 0;
            if (pItem != 0)
            {
                if (pItem->m_possessorId == m_objectId)
                {
                    bOwned = 1;
                    pRepository = GetItemRepository(1);
                }
                else
                {
                    // The item is inside something else, and that something must itself be ours.
                    CSWSItem* pContainer =
                        g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pItem->m_possessorId);
                    if (pContainer == 0 || pContainer->m_possessorId != m_objectId)
                        goto fail;
                    pRepository = pContainer->m_pItemRepository;
                    bOwned = 1;
                }
            }

            float fDX = m_position.x - fX;
            float fDY = m_position.y - fY;
            float fDZ = m_position.z - fZ;
            if (fDX * fDX + fDY * fDY + fDZ * fDZ > 1.21f)
            {
                AddActionToFront(AIACTION_DROP_ITEM, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_FLOAT, &fX,
                                 ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ,
                                 ACTION_PARAM_INT, &bDropGold, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0);
                AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                                 ACTION_PARAM_FLOAT, &vDrop.x, ACTION_PARAM_FLOAT, &vDrop.y,
                                 ACTION_PARAM_FLOAT, &vDrop.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0);
                Vector vDir = normalize(m_position - vDrop);
                vDrop = vDrop + vDir;
                Vector vZero;
                AddMoveToPointActionToFront(pNode->m_nGroupId, vDrop, m_cachedAreaId, OBJECT_INVALID, 1,
                                            0.0f, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
                return ACTION_COMPLETE;
            }

            if (m_bActionApproachDone != 0)
            {
                m_bActionApproachDone = 0;
                if (pRepository == 0)
                    goto cancel;
                if (bOwned != 0 && bDropGold == 0)
                {
                    // No null test here, unlike every other use of pPlayer in this function.
                    if (pItem->m_pItemRepository != 0 && pPlayer->m_pContainerGUI != 0
                        && pPlayer->m_pContainerGUI->m_objectId == pItem->m_objectId)
                        pItem->CloseInventory(m_objectId, 1);
                    if (RemoveItem(pItem, 1, 1, 0) == 1)
                    {
                        pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), fX, fY, fZ, 1);
                        if (pMessage != 0 && pPlayer != 0)
                            pMessage->SendServerToPlayerInventory_Drop(pPlayer->m_nPlayerId,
                                                                       pItem->m_objectId);
                        return ACTION_COMPLETE;
                    }
                }
                if (bDropGold != 1)
                    goto cancel;
                RemoveGold(pItem->m_stackCount, 1);
                pItem->SetPossessor(OBJECT_INVALID, 0, 0, OBJECT_INVALID);
                pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), fX, fY, fZ, 1);
                if (pMessage != 0 && pPlayer != 0)
                    pMessage->SendServerToPlayerInventory_Drop(pPlayer->m_nPlayerId,
                                                               pItem->m_objectId);
                return ACTION_COMPLETE;
            cancel:
                if (pMessage != 0 && pPlayer != 0)
                    pMessage->SendServerToPlayerInventory_DropCancel(pPlayer->m_nPlayerId, oidItem);
                return ACTION_FAILED;
            }

            int   nAnimation = (fZ > 1.0f) ? SERVER_ANIMATION_GET_MID : SERVER_ANIMATION_GET_LOW;
            int   nOne       = 1;
            float fSpeed     = 1.0f;
            float fDuration  = 1.5f;
            AddActionToFront(AIACTION_DROP_ITEM, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidItem,
                             ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY,
                             ACTION_PARAM_FLOAT, &fZ, ACTION_PARAM_INT, &bDropGold, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId,
                             ACTION_PARAM_INT, &nAnimation, ACTION_PARAM_FLOAT, &fSpeed,
                             ACTION_PARAM_FLOAT, &fDuration, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                             ACTION_PARAM_FLOAT, &vDrop.x, ACTION_PARAM_FLOAT, &vDrop.y,
                             ACTION_PARAM_FLOAT, &vDrop.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0);
            m_bActionApproachDone = 1;
            return ACTION_COMPLETE;
        }
    }

fail:
    if (pMessage != 0 && pPlayer != 0)
        pMessage->SendServerToPlayerInventory_DropCancel(pPlayer->m_nPlayerId, oidItem);
    m_bActionApproachDone = 0;
    return ACTION_FAILED;
}

// Runs the mine skill check, re-entered once per tick while m_bMineActionStarted gates three
// phases: out of range re-queue a walk, kneel and start the GUI timing bar, then roll the result.
int CSWSCreature::AIActionExamineMine(CSWSObjectActionNode* pNode)
{
    unsigned long long oidMine;
    float fRange;
    Vector vTarget;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;

    ActivityManager(2);

    oidMine = pNode->m_args.m_examineMineId;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidMine);
    if (pObject == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    if (GetIsInUseRange(oidMine, 0.0f, 0, 0) == 0)
    {
        // Too far away, so re-queue this action behind a walk to the mine's use point.
        GetUseRange(oidMine, vTarget, fRange, 0, 0);
        AddActionToFront(AIACTION_EXAMINE_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vTarget, m_cachedAreaId, OBJECT_INVALID, 1,
                                    fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        // In range on the first tick, kneel at the mine and start the GUI timing bar.
        m_bMineActionStarted = 1;
        int nAnimation;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            nAnimation = SERVER_ANIMATION_GET_MID;
        else
            nAnimation = SERVER_ANIMATION_GET_LOW + (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE);
        int   nOne      = 1;
        float fSpeed    = 1.0f;
        float fDuration = 4.5f;
        AddActionToFront(AIACTION_EXAMINE_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        StartGuiTimingBar(4500, 4);
        return ACTION_COMPLETE;
    }

    // The bar has run; resolve the check. Out of combat the creature takes 20; in combat it rolls.
    int nTakeTwenty = 1;
    char nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    int nRoll = 20;
    if (m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nSkill = nSkillRank;

    int nDC;
#ifdef K2_FIX_ENGINE_BUGS
    nDC = 0;
#endif
    int bOwnMine;
    int bSuccess;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        if (pObject->AsSWSTrigger()->m_creatorId == m_objectId)
        {
            bOwnMine = 1;
            bSuccess = 1;
        }
        else
        {
#if KOTOR1  // K1: the DC is the raw base disarm DC.
            nDC = pObject->AsSWSTrigger()->m_baseDisarmDC - 7;
            if (nDC <= 0)
                nDC = 1;
#else
            nDC = 1;
            if (pObject->AsSWSTrigger()->GetDisarmDC() - 7 > 0)
                nDC = pObject->AsSWSTrigger()->GetDisarmDC() - 7;
#endif
            bSuccess = (nRoll + nSkill >= nDC);
            bOwnMine = 0;
        }
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
#if KOTOR1  // K1: the DC is the raw base disarm DC.
        nDC = pObject->AsSWSDoor()->m_baseDisarmDC - 7;
        if (nDC <= 0)
            nDC = 1;
#else
        nDC = 1;
        if (pObject->AsSWSDoor()->GetDisarmDC() - 7 > 0)
            nDC = pObject->AsSWSDoor()->GetDisarmDC() - 7;
#endif
        bSuccess = (nRoll + nSkill >= nDC);
        bOwnMine = 0;
    }
    else
    {
        bOwnMine = 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
#if KOTOR1  // K1: the DC is the raw base disarm DC.
            nDC = pObject->AsSWSPlaceable()->m_baseDisarmDC - 7;
            if (nDC <= 0)
                nDC = 1;
#else
            if (pObject->AsSWSPlaceable()->GetDisarmDC() - 7 <= 0)
                nDC = 1;
            else
                nDC = pObject->AsSWSPlaceable()->GetDisarmDC() - 7;
#endif
            bSuccess = (nRoll + nSkill >= nDC);
        }
        else
        {
            bSuccess = 0;
        }
    }

    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetInteger(0, STRREF_EXAMINE_MINE);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    int nStatus = 4;
    if (bOwnMine == 0)
    {
        if (nRoll + nSkill >= nDC)
            nStatus = 1;
        else
            nStatus = (nTakeTwenty != 0) ? 3 : 0;
    }
    pData->SetInteger(5, nStatus);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0)
    {
        g_pAppManager->m_pServerExoApp->GetSWSMessage()
            ->SendServerToPlayerExamineGui_MineData(pPlayer, oidMine, this, bSuccess);
    }
    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionFlagMine(CSWSObjectActionNode* pNode)
{
    unsigned long long oidMine;
    float fRange;
    Vector vTarget;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;

    ActivityManager(2);

    oidMine = pNode->m_args.m_flagMineId;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidMine);
    if (pObject == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    if (GetIsInUseRange(oidMine, 0.0f, 0, 0) == 0)
    {
        // Too far away, so re-queue this action behind a walk to the mine's use point.
        GetUseRange(oidMine, vTarget, fRange, 0, 0);
        AddActionToFront(AIACTION_FLAG_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vTarget, m_cachedAreaId, OBJECT_INVALID, 1,
                                    fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        // In range on the first tick, kneel at the mine and start the GUI timing bar.
        m_bMineActionStarted = 1;
        int   nOne;
        int   nAnimation;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            nAnimation = SERVER_ANIMATION_GET_MID;
        else
            nAnimation = SERVER_ANIMATION_GET_LOW + (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE);
        nOne = 1;
        float fSpeed    = 1.0f;
        float fDuration = 4.5f;
        AddActionToFront(AIACTION_FLAG_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        StartGuiTimingBar(4500, 1);
        return ACTION_COMPLETE;
    }

    // The bar has run; resolve the check. Out of combat the creature takes 20; in combat it rolls.
    int nTakeTwenty = 1;
    char nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    int nRoll = 20;
    if (m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nSkill = nSkillRank;

    int nDC;
#ifdef K2_FIX_ENGINE_BUGS
    nDC = 0;
#endif
    int bOwnMine;
#if KOTOR1  // K1: the DC is the raw base disarm DC, max(base, 6) - 5.
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        unsigned long long oidCreator = pObject->AsSWSTrigger()->m_creatorId;
        CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
        if (oidCreator == m_objectId)
        {
            pTrigger->m_bMineFlagged = 1;
            bOwnMine = 1;
        }
        else
        {
            nDC = pTrigger->m_baseDisarmDC - 5;
            if (nDC <= 0)
                nDC = 1;
            bOwnMine = 0;
            if (nRoll + nSkill >= nDC)
                pObject->AsSWSTrigger()->m_bMineFlagged = 1;
        }
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
        nDC = pObject->AsSWSDoor()->m_baseDisarmDC - 5;
        if (nDC <= 0)
            nDC = 1;
        bOwnMine = 0;
        if (nRoll + nSkill >= nDC)
            pObject->AsSWSDoor()->m_bMineFlagged = 1;
    }
    else
    {
        bOwnMine = 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            nDC = pObject->AsSWSPlaceable()->m_baseDisarmDC - 5;
            if (nDC <= 0)
                nDC = 1;
            if (nRoll + nSkill >= nDC)
                pObject->AsSWSPlaceable()->m_bMineFlagged = 1;
        }
    }
#else
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        if (pObject->AsSWSTrigger()->m_creatorId == m_objectId)
        {
            // An own mine is flagged outright, with no roll.
            pObject->AsSWSTrigger()->m_bMineFlagged = 1;
            bOwnMine = 1;
        }
        else
        {
            if (pObject->AsSWSTrigger()->GetDisarmDC() - 5 <= 0)
                nDC = 1;
            else
                nDC = pObject->AsSWSTrigger()->GetDisarmDC() - 5;
            bOwnMine = 0;
            if (nRoll + nSkill >= nDC)
                pObject->AsSWSTrigger()->m_bMineFlagged = 1;
        }
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
        if (pObject->AsSWSDoor()->GetDisarmDC() - 5 <= 0)
            nDC = 1;
        else
            nDC = pObject->AsSWSDoor()->GetDisarmDC() - 5;
        bOwnMine = 0;
        if (nRoll + nSkill >= nDC)
            pObject->AsSWSDoor()->m_bMineFlagged = 1;
    }
    else
    {
        bOwnMine = 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            if (pObject->AsSWSPlaceable()->GetDisarmDC() - 5 <= 0)
                nDC = 1;
            else
                nDC = pObject->AsSWSPlaceable()->GetDisarmDC() - 5;
            if (nRoll + nSkill >= nDC)
                pObject->AsSWSPlaceable()->m_bMineFlagged = 1;
        }
    }
#endif

    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetInteger(0, 0);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    int nStatus = 4;
    if (bOwnMine == 0)
    {
        if (nRoll + nSkill >= nDC)
            nStatus = 1;
        else
            nStatus = (nTakeTwenty != 0) ? 3 : 0;
    }
    pData->SetInteger(5, nStatus);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionRecoverMine(CSWSObjectActionNode* pNode)
{
    unsigned long long oidMine;
    float fRange;
    Vector vTarget;
    CSWSItem* pItem;
#if !KOTOR1
    int nXP;
#endif

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;

    ActivityManager(2);

    oidMine = pNode->m_args.m_recoverMineId;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidMine);
    if (pObject == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    if (GetIsInUseRange(oidMine, 0.25f, 0, 0) == 0)
    {
        // Too far away, so re-queue this action behind a walk to the mine's use point.
        GetUseRange(oidMine, vTarget, fRange, 0, 0);
        fRange = fRange + 0.25f;
        AddActionToFront(AIACTION_RECOVER_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vTarget, m_cachedAreaId, OBJECT_INVALID, 1,
                                    fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        // In range on the first tick, kneel at the mine and start the GUI timing bar. A door and a
        // placeable share one animation, everything else (a trigger) takes another.
        m_bMineActionStarted = 1;
        int nAnimation = (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR
                          || pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                             ? SERVER_ANIMATION_GET_MID
                             : SERVER_ANIMATION_RECOVER_MINE;
        int   nOne      = 1;
        float fSpeed    = 1.0f;
        float fDuration = 4.5f;
        AddActionToFront(AIACTION_RECOVER_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        StartGuiTimingBar(4500, 2);
        return ACTION_COMPLETE;
    }

    // The bar has run; resolve the check. Out of combat the creature takes 20; in combat it rolls.
    int nTakeTwenty = 1;
    char nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    int nRoll = 20;
    if (m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nSkill = nSkillRank;

    int nDC;
    int bNoRoll;
        // bSuccess means "a successful roll" up to the recovered label, then the message's flag.
    int bSuccess;
        // traps.2da row index; assigned on every path that reaches the code below.
    unsigned char nTrapType;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        // Resolve the creator before the identity compare.
        CSWSCreature* pCreator = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pObject->AsSWSTrigger()->m_creatorId);
        if (pObject->AsSWSTrigger()->m_creatorId == m_objectId
            || ((g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(
                     pObject->AsSWSTrigger()->m_creatorId) != 0
                 || (pCreator != 0 && pCreator->m_pStats->m_bIsPC != 0))
                && (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(m_objectId) != 0
                    || m_pStats->m_bIsPC != 0)))
        {
            // For your own mine, or one laid by somebody on your side, pick it up with no roll, and
            // therefore no XP.
            nTrapType = pObject->AsSWSTrigger()->m_trapType;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            pObject->AsSWSTrigger()->SetFadeOutTime(2);
            bSuccess = 0;
            nDC = 0;
            bNoRoll = 1;
            goto recovered;
        }
#if KOTOR1  // K1: the raw base disarm DC.
        nDC = pObject->AsSWSTrigger()->m_baseDisarmDC + 10;
        if (nDC <= 0)
            nDC = 1;
#else
        if (pObject->AsSWSTrigger()->GetDisarmDC() + 10 <= 0)
            nDC = 1;
        else
            nDC = pObject->AsSWSTrigger()->GetDisarmDC() + 10;
#endif
        if (nRoll + nSkill < nDC)
            goto failed;
        nTrapType = pObject->AsSWSTrigger()->m_trapType;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        pObject->AsSWSTrigger()->SetFadeOutTime(2);
        bNoRoll = 0;
        bSuccess = 1;
        goto recovered;
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
#if KOTOR1  // K1: the raw base disarm DC.
        nDC = pObject->AsSWSDoor()->m_baseDisarmDC + 10;
        if (nDC <= 0)
            nDC = 1;
#else
        if (pObject->AsSWSDoor()->GetDisarmDC() + 10 <= 0)
            nDC = 1;
        else
            nDC = pObject->AsSWSDoor()->GetDisarmDC() + 10;
#endif
        if (nRoll + nSkill < nDC)
            goto failed;
        nTrapType = pObject->AsSWSDoor()->m_trapType;
        {
            // A door or placeable trap is only signalled; it is never destroyed or faded out.
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        bNoRoll = 0;
        bSuccess = 1;
        goto recovered;
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
    {
#if KOTOR1  // K1: the raw base disarm DC.
        nDC = pObject->AsSWSPlaceable()->m_baseDisarmDC + 10;
        if (nDC <= 0)
            nDC = 1;
#else
        if (pObject->AsSWSPlaceable()->GetDisarmDC() + 10 <= 0)
            nDC = 1;
        else
            nDC = pObject->AsSWSPlaceable()->GetDisarmDC() + 10;
#endif
        if (nRoll + nSkill < nDC)
            goto failed;
        nTrapType = pObject->AsSWSPlaceable()->m_trapType;
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        bNoRoll = 0;
        bSuccess = 1;
        goto recovered;
    }
    else
    {
        nDC = 0;
        goto failed;
    }

recovered:
    {
        // The recovered trap becomes an inventory item; sName's scope spans the whole tail.
        CExoString sName;
        pItem = new CSWSItem(OBJECT_INVALID);
        if (g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(
                nTrapType, CExoString("ResRef"), &sName) != 0)
        {
            pItem->LoadFromTemplate(CResRef(static_cast<char*>(sName.CStr())));
            if (GetItemRepository(1)->AddItem(&pItem, 1, 0, 1) != 0 && pItem != 0)
                pItem->SetPossessor(m_objectId, 1, 1, OBJECT_INVALID);
            // Detach the trap from the area's mine list; Contains returns the match count.
            if (GetArea() != 0)
            {
                GetArea();
                if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(oidMine) == 1)
                    static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(oidMine);
            }
        }
#if !KOTOR1  // K1: a recovery pays no XP.
        if (bSuccess != 0 && m_bInParty != 0)
        {
            // The party's own recovery pays XP: 15x the player's level for a hard trap, 10x
            // otherwise.
            CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
            CSWSCreature* pPlayer = pApp->GetPlayerCreature();
            int nHardDC = pPlayer->m_pStats->GetLevel(0) + 20;
            if (nDC < nHardDC)
                nXP = 10 * pPlayer->m_pStats->GetLevel(0);
            else
                nXP = 15 * pPlayer->m_pStats->GetLevel(0);
            g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nXP, 1);
        }
#endif
    }
    bSuccess = 1;
    goto message;

failed:
    if (nRoll + nSkill < nDC - 5)
    {
        // Missed by more than 5, the trap fires.
        if (pObject->AsSWSTrigger() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEvent->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        else if (pObject->AsSWSPlaceable() != 0 || pObject->AsSWSDoor() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }
    bSuccess = 0;
    bNoRoll = 0;

message:
    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetObjectID(1, oidMine);
    pData->SetInteger(0, STRREF_RECOVER_MINE);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    if (bNoRoll != 0)
        pData->SetInteger(5, 4);
    else if (bSuccess != 0)
        pData->SetInteger(5, 1);
    else if (nTakeTwenty != 0 && nRoll + nSkill < nDC)
        pData->SetInteger(5, 3);
    else if (nRoll + nSkill < nDC - 10)
        pData->SetInteger(5, 2);
    else if (nRoll + nSkill < nDC)
        pData->SetInteger(5, 0);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionDisarmMine(CSWSObjectActionNode* pNode)
{
    unsigned long long oidMine;
    float fRange;
    Vector vTarget;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;

    ActivityManager(2);

    oidMine = pNode->m_args.m_disarmMineId;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidMine);
    if (pObject == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    if (GetIsInUseRange(oidMine, 0.25f, 0, 0) == 0)
    {
        // Too far away, so re-queue this action behind a walk to the mine's use point.
        GetUseRange(oidMine, vTarget, fRange, 0, 0);
        fRange = fRange + 0.25f;
        AddActionToFront(AIACTION_DISARM_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vTarget, m_cachedAreaId, OBJECT_INVALID, 1,
                                    fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        m_bMineActionStarted = 1;
        int nAnimation;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            nAnimation = SERVER_ANIMATION_DISARM_DOOR_MINE;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            nAnimation = SERVER_ANIMATION_DISARM_CONTAINER_MINE;
        else
            nAnimation = SERVER_ANIMATION_DISARM_MINE;
        int   nOne      = 1;
        float fSpeed    = 1.0f;
        float fDuration = 4.5f;
        AddActionToFront(AIACTION_DISARM_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidMine, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0);
        StartGuiTimingBar(4500, 3);
        return ACTION_COMPLETE;
    }

    // The bar has run; resolve the check. Out of combat the creature takes 20; in combat it rolls.
    int nTakeTwenty = 1;
    char nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    int nRoll = 20;
    if (m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nSkill = nSkillRank;

    int nDC;
#ifdef K2_FIX_ENGINE_BUGS
    nDC = 0;
#endif
    int bNoRoll;
    int bSuccess;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        CSWSCreature* pCreator = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pObject->AsSWSTrigger()->m_creatorId);
        if (pObject->AsSWSTrigger()->m_creatorId == m_objectId
            || ((g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(
                     pObject->AsSWSTrigger()->m_creatorId) != 0
                 || (pCreator != 0 && pCreator->m_pStats->m_bIsPC != 0))
                && (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(m_objectId) != 0
                    || m_pStats->m_bIsPC != 0)))
        {
            // For your own mine, or one laid by somebody on your side, pick it up with no roll.
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            pObject->AsSWSTrigger()->SetFadeOutTime(2);
            bNoRoll = 1;
            goto removed;
        }
        #if KOTOR1  // K1: the raw base disarm DC, and a DC above 35 cannot be disarmed.
        nDC = pObject->AsSWSTrigger()->m_baseDisarmDC;
        if (nDC <= 0)
            nDC = 1;
        if (nDC > 35 || nRoll + nSkill < nDC)
            goto failed;
#else
        nDC = 1;
        if (pObject->AsSWSTrigger()->GetDisarmDC() > 0)
            nDC = pObject->AsSWSTrigger()->GetDisarmDC();
        if (nDC > 65 || nRoll + nSkill < nDC)
            goto failed;
#endif
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            pObject->AsSWSTrigger()->SetFadeOutTime(2);
        }
        bNoRoll = 0;
        goto removed;
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
        #if KOTOR1  // K1: the raw base disarm DC, and a DC above 35 cannot be disarmed.
        nDC = pObject->AsSWSDoor()->m_baseDisarmDC;
        if (nDC <= 0)
            nDC = 1;
        if (nDC > 35 || nRoll + nSkill < nDC)
            goto failed;
#else
        nDC = 1;
        if (pObject->AsSWSDoor()->GetDisarmDC() > 0)
            nDC = pObject->AsSWSDoor()->GetDisarmDC();
        if (nDC > 65 || nRoll + nSkill < nDC)
            goto failed;
#endif
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        bNoRoll = 0;
        goto removed;
    }
    else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
    {
        #if KOTOR1  // K1: the raw base disarm DC, and a DC above 35 cannot be disarmed.
        nDC = pObject->AsSWSPlaceable()->m_baseDisarmDC;
        if (nDC <= 0)
            nDC = 1;
        if (nDC > 35 || nRoll + nSkill < nDC)
            goto failed;
#else
        nDC = 1;
        if (pObject->AsSWSPlaceable()->GetDisarmDC() > 0)
            nDC = pObject->AsSWSPlaceable()->GetDisarmDC();
        if (nDC > 65 || nRoll + nSkill < nDC)
            goto failed;
#endif
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        bNoRoll = 0;
        goto removed;
    }
    else
    {
        goto failed;
    }

removed:
    if (GetArea() != 0)
    {
        GetArea();
        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(oidMine) == 1)
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(oidMine);
    }
    bSuccess = 1;
#if !KOTOR1  // K1: a disarm pays no XP.
    if (m_bInParty != 0)
    {
        CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pPlayer = pApp->GetPlayerCreature();
        int nHardDC = pPlayer->m_pStats->GetLevel(0) + 20;
        int nXP = pPlayer->m_pStats->GetLevel(0) * ((nDC < nHardDC) ? 10 : 15);
        g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nXP, 1);
    }
#endif
    goto message;

failed:
    GetClientCreature()->PlaySoundSetSound(VOICE_CHAT_UNLOCK_FAILED, 1);
    if (nRoll + nSkill < nDC - 10)
    {
        // Missed by more than 10, the trap fires.
        if (pObject->AsSWSTrigger() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEvent->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        else if (pObject->AsSWSPlaceable() != 0 || pObject->AsSWSDoor() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidMine, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }
    bSuccess = 0;
    bNoRoll = 0;

message:
    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetObjectID(1, oidMine);
    pData->SetInteger(0, STRREF_DISABLE_MINE);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    if (bNoRoll != 0)
        pData->SetInteger(5, 4);
    else if (bSuccess != 0)
        pData->SetInteger(5, 1);
    else if (nTakeTwenty != 0 && nRoll + nSkill < nDC)
        pData->SetInteger(5, 3);
    else if (nRoll + nSkill < nDC - 10)
        pData->SetInteger(5, 2);
    else if (nRoll + nSkill < nDC)
        pData->SetInteger(5, 0);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionSetMine(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: no walk latch or mine-kind event; the trap goes straight onto a door/placeable
    CExoString sTrapScript;
    unsigned long long oidTarget;
    unsigned long long oidMine;
    int   nDC;
    int   nDetectDCMod;
    int   nDisarmDCMod;
    float fX;
    float fY;
    float fZ;
    Vector vTarget;
    Vector vDest;
    float fRange K2_UNSET;
    char nSkillRank;
    char nSkillBase;
    int nSkill;
    int nTakeTwenty;
    int nRoll;
    CSWSTrigger* pTrigger;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;
    vDest.x = 0.0f;
    vDest.y = 0.0f;
    vDest.z = 0.0f;

    ActivityManager(2);
    CSWSItem* pItem;
    CGameObject* pObject;

    oidMine   = pNode->m_args.m_setMineItemId;
    oidTarget = pNode->m_args.m_aqObjectId;
    fX = pNode->m_args.m_aqObjPointX;
    fY = pNode->m_args.m_aqObjPointY;
    fZ = pNode->m_args.m_aqObjPointZ;
    vTarget.x = fX;
    vTarget.y = fY;
    vTarget.z = fZ;

    pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidMine);
    if (pItem == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0)
    {
        vTarget = pObject->AsSWSObject()->m_position;
    }
    else
    {
        Vector vZero;
        if (equal(vTarget, vZero) != 0)
        {
            m_bMineActionStarted = 0;
            return ACTION_FAILED;
        }
    }

    if (GetArea() != 0 && static_cast<CSWSArea*>(GetArea())->GetPlayerCanSetMines() == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    int bInRange;
    if (pObject != 0)
    {
        GetUseRange(oidTarget, vDest, fRange, 0, 0);
        bInRange = GetIsInUseRange(oidTarget, 0.5f, 0, 0);
    }
    else
    {
        vDest = vTarget - normalize(m_position - vTarget);
        Vector vDelta = m_position - vDest;
        bInRange = (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z) < 2.25f;
    }

    if (bInRange == 0)
    {
        AddActionToFront(AIACTION_SET_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX,
                         ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0);
        if (pObject != 0)
        {
            AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            Vector vZero;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, oidTarget, 1,
                                        fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        }
        else
        {
            AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                             ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY,
                             ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0);
            Vector vZero;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, OBJECT_INVALID, 1,
                                        1.0f, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        }
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        m_bMineActionStarted = 1;
        int   nAnimation = SERVER_ANIMATION_SET_MINE;
        if (pObject != 0 && (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR
                             || pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE))
            nAnimation = SERVER_ANIMATION_GET_MID;
        int   nOne       = 1;
        float fSpeed     = 1.0f;
        float fDuration  = 2.0f;
        AddActionToFront(AIACTION_SET_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX,
                         ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        StartGuiTimingBar(2000, 5);
        return ACTION_COMPLETE;
    }

    unsigned short nTrapType = pItem->GetActiveProperty(0)->m_subType;
    g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("SetDC"), &nDC);
    if (nDC <= 0)
        nDC = 1;

    // The demolitions rank, +2 when the second GetSkillRank read is above 4.
    nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    nSkillBase = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 1);
    nSkill = nSkillBase > 4 ? nSkillRank + 2 : nSkillRank;
    nTakeTwenty = 1;
    nRoll = 20;
    if (m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nTotal = nRoll + nSkill;

    if (nTotal >= nDC || (nTakeTwenty == 0 && nTotal < nDC - 10))
    {
        g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("DetectDCMod"),
                                                    &nDetectDCMod);
        g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("DisarmDCMod"),
                                                    &nDisarmDCMod);
        g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(nTrapType, CExoString("TrapScript"),
                                                           &sTrapScript);

        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
        {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            pDoor->m_trapType = static_cast<unsigned char>(nTrapType);
            pDoor->m_bTrapFlag = 1;
            pDoor->m_scriptSlots[9] = sTrapScript;
            pDoor->m_baseDetectDC = static_cast<unsigned char>(nDetectDCMod + nTotal);
            pDoor->m_baseDisarmDC = static_cast<unsigned char>(nDisarmDCMod + nTotal);
            pDoor->m_bTrapDisarmable = 1;
            pDoor->m_bTrapDetectable = 1;
            pDoor->m_bTrapOneShot = 1;
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pDoor->m_objectId);
        }
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            pPlaceable->m_trapType = static_cast<unsigned char>(nTrapType);
            pPlaceable->m_bTrapFlag = 1;
            pPlaceable->m_scriptsOn[CSWSPlaceable::ON_SCRIPT_TRAP_TRIGGERED] = sTrapScript;
            pPlaceable->m_baseDetectDC = static_cast<unsigned char>(nDetectDCMod + nTotal);
            pPlaceable->m_baseDisarmDC = static_cast<unsigned char>(nDisarmDCMod + nTotal);
            pPlaceable->m_bTrapDisarmable = 1;
            pPlaceable->m_bTrapDetectable = 1;
            pPlaceable->m_bTrapOneShot = 1;
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pPlaceable->m_objectId);
        }
        else
        {
            pTrigger = new CSWSTrigger(OBJECT_INVALID);
            pTrigger->SetCreator(m_objectId);
            pTrigger->m_trapType = static_cast<unsigned char>(nTrapType);
            pTrigger->m_scriptSlots[4] = sTrapScript;
            pTrigger->m_bTrap = 1;
            pTrigger->m_bTrapDetectable = 1;
            pTrigger->m_bTrapDisarmable = 1;
            pTrigger->m_baseDetectDC = nDetectDCMod + nTotal;
            pTrigger->m_baseDisarmDC = nDisarmDCMod + nTotal;
            pTrigger->SetPosition(m_position, 1, 1, 0);
            pTrigger->m_nFaction = m_pStats->m_factionId;
            int nTrapName = 0;
            g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("TrapName"),
                                                        &nTrapName);
            pTrigger->m_localizedName.m_nStrRef = nTrapName;
            pTrigger->CreateNewGeometry(2.0f, vTarget, static_cast<CSWSArea*>(GetArea()));
            pTrigger->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y, m_position.z, 1);
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pTrigger->m_objectId);
        }

        if (GetItemRepository(1)->GetItemInRepository(pItem, 1) != 0)
        {
            if (pItem->m_stackCount >= 2)
            {
                pItem->m_stackCount = pItem->m_stackCount - 1;
            }
            else if (RemoveItem(pItem, 1, 1, 0) == 1)
            {
                CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
                if (pPlayer != 0)
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()
                        ->SendServerPlayerItemUpdate_DestroyItem(pPlayer, pItem->m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, pItem->m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            }
        }
    }

    // Botched by more than ten, only a mine laid on an existing trigger, placeable or door is
    // signalled; a mine on bare ground never goes off.
    if (nTakeTwenty == 0 && pObject != 0 && nTotal < nDC - 10)
    {
        if (pObject->AsSWSTrigger() != 0)
        {
            pObject->AsSWSTrigger()->m_bTrap = 1;
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEvent->SetObjectID(0, m_objectId);
            pEvent->SetInteger(0, 1);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        else if (pObject->AsSWSPlaceable() != 0 || pObject->AsSWSDoor() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
            pEvent->SetInteger(0, 1);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }

    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    pData->SetInteger(0, STRREF_SET_MINE);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    int bSuccess = 0;
    if (nTotal >= nDC)
    {
        bSuccess = 1;
        pData->SetInteger(5, 1);
    }
    else if (nTakeTwenty != 0)
        pData->SetInteger(5, 3);
    else if (nTotal < nDC - 10)
        pData->SetInteger(5, 2);
    else
        pData->SetInteger(5, 0);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    BroadcastVoiceChat(bSuccess != 0 ? VOICE_CHAT_SET_MINE : VOICE_CHAT_UNLOCK_FAILED);
    ClearActivities((ACTIVITY_DETECT | ACTIVITY_CONVERSATION | ACTIVITY_RESTING));
    RemoveCombatInvisibilityEffects();
    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    return ACTION_COMPLETE;
#else
    CExoString sTrapScript;
    unsigned long long oidTarget;
    unsigned long long oidMine;
    int   nDC;
    int   nDetectDCMod;
    int   nDisarmDCMod;
    float fX;
    float fY;
    float fZ;
    Vector vTarget;
    Vector vDest;
    float fRange K2_UNSET;
    CGameObject* pObject;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;
    vDest.x = 0.0f;
    vDest.y = 0.0f;
    vDest.z = 0.0f;

    ActivityManager(2);

    oidMine   = pNode->m_args.m_setMineItemId;
    oidTarget = pNode->m_args.m_aqObjectId;
    fX = pNode->m_args.m_aqObjPointX;
    fY = pNode->m_args.m_aqObjPointY;
    fZ = pNode->m_args.m_aqObjPointZ;
    vTarget.x = fX;
    vTarget.y = fY;
    vTarget.z = fZ;

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidMine);
    if (pItem == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0)
    {
        // Planting ON something, the object's own position wins over whatever point was queued.
        vTarget = pObject->AsSWSObject()->m_position;
    }
    else
    {
        // Planting on bare ground, an all-zero point means "nowhere", and the action just fails.
        Vector vZero;
        if (equal(vTarget, vZero) != 0)
        {
            m_bMineActionStarted = 0;
            return ACTION_FAILED;
        }
    }

    // Some areas forbid mines outright; the first GetArea() result is used only for the null test.
    if (GetArea() != 0 && static_cast<CSWSArea*>(GetArea())->GetPlayerCanSetMines() == 0)
    {
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        return ACTION_FAILED;
    }

    int bInRange;
    if (pObject != 0)
    {
        GetUseRange(oidTarget, vDest, fRange, 0, 0);
        bInRange = GetIsInUseRange(oidTarget, 0.5f, 0, 0);
    }
    else
    {
        vDest = vTarget - normalize(m_position - vTarget);
        Vector vDelta = m_position - vDest;
        bInRange = (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z) < 2.25f;
    }

    if (bInRange == 0)
    {
        // Too far, so re-queue this action verbatim behind a walk to the plant point.
        AddActionToFront(AIACTION_SET_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX,
                         ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0);
        if (pObject != 0)
        {
            AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            Vector vZero;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, oidTarget, 1,
                                        fRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        }
        else
        {
            AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                             ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY,
                             ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0);
            Vector vZero;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, m_cachedAreaId, OBJECT_INVALID, 1,
                                        1.0f, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        }
        m_bActionApproachDone = 1;
        return ACTION_COMPLETE;
    }

    if (m_bActionApproachDone == 0)
    {
        // In range but the walk phase never ran, so re-queue and let the walk latch.
        AddActionToFront(AIACTION_SET_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX,
                         ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        m_bActionApproachDone = 1;
        return ACTION_COMPLETE;
    }

    if (m_bMineActionStarted == 0)
    {
        // In range on the first tick, kneel and start the GUI timing bar. One animation here,
        // unlike every reader twin: planting does not care what it is planting on.
        m_bMineActionStarted = 1;
        int   nAnimation = SERVER_ANIMATION_SET_MINE;
        int   nOne       = 1;
        float fSpeed     = 1.0f;
        float fDuration  = 2.0f;
        AddActionToFront(AIACTION_SET_MINE, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidMine,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX,
                         ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnimation,
                         ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fDuration,
                         ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        StartGuiTimingBar(2000, 5);
        return ACTION_COMPLETE;
    }

    int nCheckBonus = 0;
    unsigned short nTrapType = pItem->GetActiveProperty(0)->m_subType;
    switch (nTrapType)
    {
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_AVERAGE:
        case TRAP_BASE_TYPE_LASER_SLICING_AVERAGE:
        case TRAP_BASE_TYPE_SONIC_CHARGE_AVERAGE:
            nCheckBonus = 3;
            break;
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEADLY:
        case TRAP_BASE_TYPE_LASER_SLICING_DEADLY:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEADLY:
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_STRONG:
        case TRAP_BASE_TYPE_LASER_SLICING_STRONG:
        case TRAP_BASE_TYPE_SONIC_CHARGE_STRONG:
            nCheckBonus = 9;
            break;
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEVASTATING:
        case TRAP_BASE_TYPE_LASER_SLICING_DEVASTATING:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEVASTATING:
            nCheckBonus = 12;
            break;
    }

    g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("SetDC"), &nDC);
    unsigned char nDifficulty = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    nDC = nDC + (nDifficulty == 0 ? -5 : (nDifficulty == 2 ? 5 : 0));
    if (nDC <= 0)
        nDC = 1;

    int nTakeTwenty = 1;
    int nRoll = 20;
    char nSkillRank = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
    if (pObject == 0 && m_bCombatState != 0)
    {
        nRoll = rand_wincompatible() % 20 + 1;
        nTakeTwenty = 0;
    }
    int nSkill = nSkillRank;
    // The roll total is read as `mod + total`, so it needs its own local.
    int nTotal = nRoll + nSkill;

    int bSuccess;
    int bSetOff;
    int bWentOff;
    if (nTotal >= nDC)
    {
        bSetOff = 0;
        bSuccess = 1;
    }
    else
    {
        bSuccess = 0;
        // Taking twenty and still failing just wastes the attempt: the mine stays in the pack.
        if (nTakeTwenty != 0)
        {
            bWentOff = 0;
            goto message;
        }
        bSetOff = 1;
        // Missed by ten or less is a plain fumble; the mine is still consumed but does not fire.
        if (nTotal >= nDC - 10)
        {
            bWentOff = 0;
            goto message;
        }
    }

    // Either the mine is going down, or it is going off; both need the trap's own 2DA row.
    g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("DetectDCMod"),
                                                &nDetectDCMod);
    g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("DisarmDCMod"),
                                                &nDisarmDCMod);
    g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(nTrapType, CExoString("TrapScript"),
                                                       &sTrapScript);

    CSWSTrigger* pTrigger;
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        pTrigger = 0;
        // An already-trapped door is left alone, and the mine is still consumed below.
        if (pDoor->m_bTrapFlag == 0)
        {
            pDoor->m_bTrapFlag = 1;
            pDoor->m_trapType = static_cast<unsigned char>(nTrapType);
            pDoor->m_scriptSlots[9] = sTrapScript;
            pDoor->m_baseDetectDC = static_cast<unsigned char>(nDetectDCMod + nTotal);
            pDoor->m_baseDisarmDC = static_cast<unsigned char>(nDisarmDCMod + nTotal);
            pDoor->m_bTrapDisarmable = 0;
            pDoor->m_bTrapDetectable = 1;
            pDoor->m_bTrapOneShot = 1;
#if !KOTOR1
            pDoor->m_nOwnerDemolitionsSkill = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
#endif
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pDoor->m_objectId);

            // The door carries the trap fields, but the geometry that fires it is still a real
            // trigger hung off the door.
            pTrigger = new CSWSTrigger(OBJECT_INVALID);
            pTrigger->SetCreator(m_objectId);
            pTrigger->m_trapType = static_cast<unsigned char>(nTrapType);
            pTrigger->m_bTrap = 1;
            pTrigger->SetPosition(pDoor->m_position, 1, 1, 0);
            pTrigger->m_nFaction = m_pStats->m_factionId;
            pTrigger->m_bTrapDetectable = 0;
            pTrigger->m_bTrapDisarmable = 0;
            pTrigger->m_bPartyInteract = 0;
            int nTrapName = 0;
            g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("TrapName"),
                                                        &nTrapName);
            pTrigger->m_localizedName.m_nStrRef = nTrapName;
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pTrigger->m_objectId);
            pTrigger->CreateNewGeometry(2.0f, vTarget, static_cast<CSWSArea*>(GetArea()));
            pTrigger->AddToArea(static_cast<CSWSArea*>(GetArea()), pDoor->m_position.x, pDoor->m_position.y,
                                pDoor->m_position.z, 1);
#if !KOTOR1
            pDoor->m_pMineTrigger = pTrigger;
#endif
            if (bSetOff == 0)
            {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                pEvent->SetInteger(0, 1);
                pEvent->SetInteger(1, 0);
                pEvent->SetInteger(2, 1);
                pEvent->SetInteger(3, nCheckBonus);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 3000, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }
    else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
    {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        pTrigger = 0;
        if (pPlaceable->m_bTrapFlag == 0)
        {
            pPlaceable->m_bTrapFlag = 1;
            pPlaceable->m_trapType = static_cast<unsigned char>(nTrapType);
            pPlaceable->m_scriptsOn[CSWSPlaceable::ON_SCRIPT_TRAP_TRIGGERED] = sTrapScript;
            pPlaceable->m_baseDetectDC = static_cast<unsigned char>(nDetectDCMod + nTotal);
            pPlaceable->m_baseDisarmDC = static_cast<unsigned char>(nDisarmDCMod + nTotal);
            pPlaceable->m_bTrapDisarmable = 0;
            pPlaceable->m_bTrapDetectable = 1;
            pPlaceable->m_bTrapOneShot = 1;
#if !KOTOR1
            pPlaceable->m_nOwnerDemolitionsSkill = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
#endif
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pPlaceable->m_objectId);

            pTrigger = new CSWSTrigger(OBJECT_INVALID);
            pTrigger->SetCreator(m_objectId);
            pTrigger->m_trapType = static_cast<unsigned char>(nTrapType);
            pTrigger->m_bTrap = 1;
            pTrigger->SetPosition(pPlaceable->m_position, 1, 1, 0);
            pTrigger->m_nFaction = m_pStats->m_factionId;
            pTrigger->m_bTrapDetectable = 0;
            pTrigger->m_bTrapDisarmable = 0;
            pTrigger->m_bPartyInteract = 0;
            int nTrapName = 0;
            g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("TrapName"),
                                                        &nTrapName);
            pTrigger->m_localizedName.m_nStrRef = nTrapName;
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pTrigger->m_objectId);
            pTrigger->CreateNewGeometry(2.0f, vTarget, static_cast<CSWSArea*>(GetArea()));
            pTrigger->AddToArea(static_cast<CSWSArea*>(GetArea()), pPlaceable->m_position.x,
                                pPlaceable->m_position.y, pPlaceable->m_position.z, 1);
#if !KOTOR1
            pPlaceable->m_pOwnedTrigger = pTrigger;
#endif
            if (bSetOff == 0)
            {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                pEvent->SetInteger(0, 1);
                pEvent->SetInteger(1, 0);
                pEvent->SetInteger(2, 1);
                pEvent->SetInteger(3, nCheckBonus);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 3000, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }
    else
    {
        pTrigger = new CSWSTrigger(OBJECT_INVALID);
        pTrigger->SetCreator(m_objectId);
        pTrigger->m_trapType = static_cast<unsigned char>(nTrapType);
        pTrigger->m_scriptSlots[4] = sTrapScript;
        pTrigger->m_bTrap = 1;
        pTrigger->m_bTrapDetectable = 1;
        pTrigger->m_bTrapDisarmable = 1;
        pTrigger->m_baseDetectDC = nDetectDCMod + nTotal;
        pTrigger->m_baseDisarmDC = nDisarmDCMod + nTotal;
        pTrigger->SetPosition(m_position, 1, 1, 0);
        pTrigger->m_nFaction = m_pStats->m_factionId;
        pTrigger->m_nOwnerDemolitionsSkill = m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
        int nTrapName = 0;
        g_pRules->m_p2DArrays->m_pTraps->GetINTEntry(nTrapType, CExoString("TrapName"),
                                                    &nTrapName);
        pTrigger->m_localizedName.m_nStrRef = nTrapName;
        pTrigger->CreateNewGeometry(2.0f, vTarget, static_cast<CSWSArea*>(GetArea()));
        pTrigger->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y, m_position.z, 1);
        static_cast<CSWSArea*>(GetArea())->m_mineObjects.Add(pTrigger->m_objectId);
    }

    // The mine leaves the pack whether it went down or went off, so decrement the stack, or destroy
    // the last one outright and tell the owning client about it.
    if (GetItemRepository(1)->GetItemInRepository(pItem, 1) != 0)
    {
        if (pItem->m_stackCount >= 2)
        {
            pItem->m_stackCount = pItem->m_stackCount - 1;
        }
        else if (RemoveItem(pItem, 1, 1, 0) != 0)
        {
            CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
            if (pPlayer != 0)
                g_pAppManager->m_pServerExoApp->GetSWSMessage()
                    ->SendServerPlayerItemUpdate_DestroyItem(pPlayer, pItem->m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, pItem->m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
    }

    {
        // The module hears about every mine that gets planted.
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM;
        pEvent->SetObjectID(0, pItem->m_objectId);
        unsigned long long oidModule = g_pAppManager->m_pServerExoApp->GetModule()->m_objectId;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, oidModule, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }

    if (bSetOff != 0)
    {
        // Botched by more than ten, the trap fires on the spot. A free-standing mine is armed and
        // signalled on itself; a door or placeable is signalled through the object it sits on.
        if (pObject == 0 && pTrigger != 0)
        {
            pTrigger->m_bTrap = 1;
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEvent->SetObjectID(0, m_objectId);
            pEvent->SetInteger(0, 1);
            pEvent->SetInteger(1, 1);
            pEvent->SetInteger(2, 1);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, pTrigger->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT,
                pEvent);
        }
        else if (pObject->AsSWSPlaceable() != 0 || pObject->AsSWSDoor() != 0)
        {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
            pEvent->SetInteger(0, 1);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        bWentOff = 1;
    }
    else
    {
        bWentOff = 0;
    }

message:
    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, m_objectId);
    if (pObject != 0
        && (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE
            || pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR))
        pData->SetObjectID(1, pObject->m_objectId);
    pData->SetInteger(0, STRREF_SET_MINE);
    pData->SetInteger(1, nRoll);
    pData->SetInteger(2, nSkill);
    pData->SetInteger(3, nDC);
    pData->SetInteger(4, nTakeTwenty);
    if (bSuccess != 0)
        pData->SetInteger(5, 1);
    else if (nTakeTwenty != 0)
        pData->SetInteger(5, 3);
    else if (bWentOff != 0)
        pData->SetInteger(5, 2);
    else
        pData->SetInteger(5, 0);
    pData->SetInteger(6, STRREF_DEMOLITIONS);
    BroadcastSkillData(pData);

    BroadcastVoiceChat(bSuccess != 0 ? VOICE_CHAT_SET_MINE : VOICE_CHAT_UNLOCK_FAILED);
    ClearActivities((ACTIVITY_DETECT | ACTIVITY_CONVERSATION | ACTIVITY_RESTING));
    RemoveCombatInvisibilityEffects();
    StopGuiTimingBar();
    m_bMineActionStarted = 0;
    m_bActionApproachDone = 0;
    return ACTION_COMPLETE;
#endif
}


unsigned char CSWSCreature::CanEquipWeapon(CSWSItem* pItem, unsigned int* pnSlot, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (*pnSlot != CSWInventory::SLOT_LEFTWEAPON && *pnSlot != CSWInventory::SLOT_RIGHTWEAPON)
        return CAN_EQUIP_NO;
    CSWItem* pRight = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
    CSWItem* pLeft = m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON);
    CSWBaseItem* pBaseRight = 0;
    if (pRight != 0)
        pBaseRight = g_pRules->GetBaseItem(pRight->m_baseItem);
    if (pLeft != 0)
        g_pRules->GetBaseItem(pLeft->m_baseItem);  // result discarded
    if (pItem == 0)
        return CAN_EQUIP_NO;
    CSWBaseItem* pBaseNew = g_pRules->GetBaseItem(pItem->m_baseItem);
    if (pRight == 0 && pLeft == 0) {
        if (*pnSlot == CSWInventory::SLOT_LEFTWEAPON)
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
        return CAN_EQUIP_YES;
    }
    if (pRight != 0 && pLeft == 0) {
        if (pBaseNew->GetIsTwoHanded() == 1) {
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
            return CAN_EQUIP_CLEAR_WEAPONS;
        }
        if ((pBaseRight->m_bRangedWeapon != 0) == (pBaseNew->m_bRangedWeapon != 0)) {
            if (pBaseRight->GetIsOneHanded() == 1)
                return (*pnSlot == CSWInventory::SLOT_LEFTWEAPON) ? CAN_EQUIP_YES : CAN_EQUIP_SWAP;
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
            return CAN_EQUIP_SWAP;
        }
        *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
        return CAN_EQUIP_SWAP;
    }
    if (pRight == 0 || pLeft == 0)
        return CAN_EQUIP_NO;
    if (pBaseNew->GetIsTwoHanded() == 1) {
        *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
        return CAN_EQUIP_CLEAR_WEAPONS;
    }
    if ((pBaseRight->m_bRangedWeapon != 0) == (pBaseNew->m_bRangedWeapon != 0))
        return CAN_EQUIP_SWAP;
    *pnSlot = CSWInventory::SLOT_RIGHTWEAPON;
    return CAN_EQUIP_CLEAR_WEAPONS;
}

#if !KOTOR1
// The exact twin of CanEquipWeapon, with the slot numbers shifted.
unsigned char CSWSCreature::CanEquipWeapon2(CSWSItem* pItem, unsigned int* pnSlot, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (*pnSlot != CSWInventory::SLOT_LEFTWEAPON2 && *pnSlot != CSWInventory::SLOT_RIGHTWEAPON2)
        return CAN_EQUIP_NO;
    CSWItem* pRight = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2);
    CSWItem* pLeft = m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2);
    CSWBaseItem* pBaseRight = 0;
    if (pRight != 0)
        pBaseRight = g_pRules->GetBaseItem(pRight->m_baseItem);
    if (pLeft != 0)
        g_pRules->GetBaseItem(pLeft->m_baseItem);  // result discarded
    if (pItem == 0)
        return CAN_EQUIP_NO;
    CSWBaseItem* pBaseNew = g_pRules->GetBaseItem(pItem->m_baseItem);
    if (pRight == 0 && pLeft == 0) {
        if (*pnSlot == CSWInventory::SLOT_LEFTWEAPON2)
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
        return CAN_EQUIP_YES;
    }
    if (pRight != 0 && pLeft == 0) {
        if (pBaseNew->GetIsTwoHanded() == 1) {
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
            return CAN_EQUIP_CLEAR_WEAPONS2;
        }
        if ((pBaseRight->m_bRangedWeapon != 0) == (pBaseNew->m_bRangedWeapon != 0)) {
            if (pBaseRight->GetIsOneHanded() == 1)
                return (*pnSlot == CSWInventory::SLOT_LEFTWEAPON2) ? CAN_EQUIP_YES : CAN_EQUIP_SWAP;
            *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
            return CAN_EQUIP_SWAP;
        }
        *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
        return CAN_EQUIP_SWAP;
    }
    if (pRight == 0 || pLeft == 0)
        return CAN_EQUIP_NO;
    if (pBaseNew->GetIsTwoHanded() == 1) {
        *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
        return CAN_EQUIP_CLEAR_WEAPONS2;
    }
    if ((pBaseRight->m_bRangedWeapon != 0) == (pBaseNew->m_bRangedWeapon != 0))
        return CAN_EQUIP_SWAP;
    *pnSlot = CSWInventory::SLOT_RIGHTWEAPON2;
    return CAN_EQUIP_CLEAR_WEAPONS2;
}
#endif

unsigned char CSWSCreature::CanEquipMiscellaneous(CSWSItem* pItem, unsigned int* pnSlot,
                                                  int a, int b)
{
    if (CheckProficiencies(pItem, static_cast<unsigned int>(reinterpret_cast<unsigned long long>(pnSlot))) != 0) {
        if (m_pInventory->GetItemInSlot(*pnSlot) != 0) {
            if (a == 1 && b == 1)
                SendFeedbackMessage(FEEDBACK_EQUIPPED_ITEM_SWAPPED, 0);
            return CAN_EQUIP_SWAP;
        }
        return CAN_EQUIP_YES;
    }
    if (a == 1 && b == 1)
        SendFeedbackMessage(FEEDBACK_EQUIP_NOT_PROFICIENT, 0);
    return CAN_EQUIP_NO;
}

unsigned char CSWSCreature::CanEquipItem(CSWSItem* pItem, unsigned int* pnSlot, int a, int b, int c)
{
    if (a == 1 && b == 0 && m_pStats->m_bIsPC == 1) {
        unsigned char nLevel = m_pStats->GetLevel(0);
        unsigned char nMinLevel = pItem->GetMinEquipLevel();
        if (nLevel < nMinLevel &&
            g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bItemLevelRestrictions != 0) {
            if (c == 1)
                SendFeedbackMessage(FEEDBACK_EQUIP_LEVEL_TOO_LOW, 0);
            return CAN_EQUIP_NO;
        }
    }
    if ((pItem->m_nFlagsLow & CSWSItem::FLAG_NON_EQUIPPABLE) != 0)
        return CAN_EQUIP_NO;
    if (CheckProficiencies(pItem, 0) == 0)  // the slot argument is unused in the callee
        return CAN_EQUIP_NO;
    if (CheckItemAlignmentRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_ALIGNMENT_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
    if (CheckItemClassRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_CLASS_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
#if KOTOR1
    if (CheckItemRaceRestrictions(pItem) == 0) {
#else
    if (CheckItemRaceRestrictions(pItem, 1) == 0) {
#endif
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_RACE_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
    if (CheckItemFeatRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_FEAT_REQUIRED, 0);
        return CAN_EQUIP_NO;
    }
#if !KOTOR1
    if (CheckItemGenderRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_GENDER_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
    if (CheckItemPCRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_PC_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
    if (CheckItemAttributeRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_ATTRIBUTE_TOO_LOW, 0);
        return CAN_EQUIP_NO;
    }
    if (CheckItemCNPCRestrictions(pItem) == 0) {
        if (a == 1 && c == 1)
            SendFeedbackMessage(FEEDBACK_EQUIP_CHARACTER_RESTRICTED, 0);
        return CAN_EQUIP_NO;
    }
#endif

    unsigned char bResult;
    switch (*pnSlot) {
    case CSWInventory::SLOT_RIGHTWEAPON:
    case CSWInventory::SLOT_LEFTWEAPON:
        bResult = CanEquipWeapon(pItem, pnSlot, a, c);
        break;
#if !KOTOR1
    case CSWInventory::SLOT_RIGHTWEAPON2:
    case CSWInventory::SLOT_LEFTWEAPON2:
        bResult = CanEquipWeapon2(pItem, pnSlot, a, c);
        break;
#endif
    default:
        bResult = CanEquipMiscellaneous(pItem, pnSlot, a, c);
        break;
    }

#if KOTOR1
    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
    if (pBaseItem == 0)
        return CAN_EQUIP_NO;
    return ((*pnSlot & pBaseItem->m_nEquipableSlots) != 0) ? bResult : CAN_EQUIP_NO;
#else
    unsigned int nSlot = *pnSlot;
    if (nSlot == CSWInventory::SLOT_RIGHTWEAPON2)
        *pnSlot = (CSWInventory::SLOT_RIGHTWEAPON2 | CSWInventory::SLOT_RIGHTWEAPON);
    else if (nSlot == CSWInventory::SLOT_LEFTWEAPON2)
        *pnSlot = (CSWInventory::SLOT_LEFTWEAPON2 | CSWInventory::SLOT_LEFTWEAPON);
    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
    if (pBaseItem == 0) {
        *pnSlot = nSlot;
        return CAN_EQUIP_NO;
    }
    // Both operands are read into locals before the slot is put back.
    unsigned int nSlotMask = *pnSlot;
    unsigned int nEquipableSlots = pBaseItem->m_nEquipableSlots;
    *pnSlot = nSlot;
    return ((nEquipableSlots & nSlotMask) != 0) ? bResult : CAN_EQUIP_NO;
#endif
}

static inline Vector crossProduct(const Vector& a, const Vector& b)
{
    Vector r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * w, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

int CSWSCreature::AIActionDrive(CSWSObjectActionNode* pNode)
{
    if (GetDead() == 1)
        return ACTION_FAILED;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() == 1 || GetDeadTemp() == 1)
        return ACTION_FAILED;
#else
    if (GetDeadTemp() == 1)
        return ACTION_FAILED;
#endif

    Vector vPoint;
    vPoint.z = 0.0f;
    vPoint.x = pNode->m_args.m_driveX;
    vPoint.y = pNode->m_args.m_driveY;
    int nHeading = pNode->m_args.m_nDriveHeading;
    int nClientPathId = pNode->m_args.m_nDriveClientPathId;
    int nFlags = pNode->m_args.m_nDriveFlags;
    int nTicksLeft = pNode->m_args.m_nDriveTicksLeft;
    int bComputePath = pNode->m_args.m_bDriveComputePath;

    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return ACTION_FAILED;
    m_bDriving = 1;
    if (m_bMovementSpeedLimited == 1 && (nFlags & DRIVE_RUN) == DRIVE_RUN)
        nFlags = (nFlags & ~DRIVE_RUN) | DRIVE_FORWARD;

    Vector vGoal = m_position;
    float fSpeedFactor = 1.0f;
    if (bComputePath != 0) {
        m_fDriveSpeedFactor = 1.0f;
        pNode->m_args.m_bDriveComputePath = 0;

        double dHeading = nHeading / 10.0 * 0.017453292f;
        Vector vFacing;
        vFacing.x = static_cast<float>(cos(dHeading));
        vFacing.y = static_cast<float>(sin(dHeading));
        vFacing.z = 0.0f;
        Vector vMove;
        float fRate;
        if (nFlags & DRIVE_STRAFE_LEFT) {
            vMove.x -= vFacing.y;
            vMove.y += vFacing.x;
            vMove.z = 0.0f;
            fRate = 0.8f;
        } else if (nFlags & DRIVE_STRAFE_RIGHT) {
            vMove.x += vFacing.y;
            vMove.y -= vFacing.x;
            vMove.z = 0.0f;
            fRate = 0.8f;
        } else {
            fRate = 0.0f;
        }
        if ((nFlags & DRIVE_RUN) == DRIVE_RUN) {
            vMove.x += vFacing.x;
            vMove.y += vFacing.y;
            vMove.z = 0.0f;
            fRate = GetRunRate() * 400.0f / 1000000.0f;
        } else if (nFlags & DRIVE_FORWARD) {
            vMove.x += vFacing.x;
            vMove.y += vFacing.y;
            vMove.z = 0.0f;
            fRate = GetWalkRate() * 400.0f / 1000000.0f;
        } else if (nFlags & DRIVE_BACKWARD) {
            vMove.x -= vFacing.x;
            vMove.y -= vFacing.y;
            fRate = GetWalkRate() * 400.0f / 1000000.0f;
        }

        if (nFlags & DRIVE_TURN_CCW) {
            Quaternion qTurn = YawPitchRoll(30.0f, 0.0f, 0.0f);
            vFacing = RotateByQuaternion(vFacing, qTurn);
            vMove = RotateByQuaternion(vMove, qTurn);
        } else if (nFlags & DRIVE_TURN_CW) {
            Quaternion qTurn = YawPitchRoll(-30.0f, 0.0f, 0.0f);
            vFacing = RotateByQuaternion(vFacing, qTurn);
            vMove = RotateByQuaternion(vMove, qTurn);
        }

        vFacing.Normalize();
        SetOrientation(vFacing);
        vMove.Normalize();

        Vector vDest;
        Vector vGap = vPoint - m_position;
        if (vGap.x * vGap.x + vGap.y * vGap.y + vGap.z * vGap.z > 0.25f
                && pArea->ComputeSafeLocation(vPoint, 0.5f, m_pPathfindInfo, 0, &vDest) != 0) {
            SetPosition(vDest, 1, 1, 0);
        } else {
            vDest = m_position;
        }

        float fSpace = m_pPathfindInfo->m_fPersonalSpace;
        float fHeight = m_pPathfindInfo->m_fHeight;
        m_fDriveSpeedFactor = 1.0f;
        vDest = vDest + vMove * fRate;
        vDest.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vDest, 0, 0, 0);

        unsigned long long oidBlocker;
        if (pArea->NoCreaturesOnLine(m_position, vDest, m_pPathfindInfo, 1, 0, &oidBlocker, 0, 0) == 0
                && oidBlocker != OBJECT_INVALID) {
            CSWSCreature* pBlocker =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidBlocker);
            if (pBlocker != 0)
                BumpFriends(pBlocker, m_position, vDest);
        }

        Vector vFrom;
        vFrom.x = m_position.x;
        vFrom.y = m_position.y;
        vFrom.z = 0.0f;
        int bClear = 0;
        if (pArea->TestDirectLine(vFrom, vDest, fSpace, fHeight, 0, 0) == 1) {
            vGoal = vDest;
            bClear = 1;
        } else {
            // Probe 1 bisects back down the segment for the furthest point that is reachable.
            float fFraction = 0.5f;
            float fStep = 0.25f;
            Vector vTry;
            do {
                vTry = m_position + (vDest - m_position) * fFraction;
                vFrom.x = m_position.x;
                vFrom.y = m_position.y;
                vFrom.z = 0.0f;
                if (pArea->TestDirectLine(vFrom, vTry, fSpace, fHeight, 0, 0) == 1) {
                    vGoal = vTry;
                    bClear = 1;
                    fFraction = fFraction + fStep;
                } else {
                    fFraction = fFraction - fStep;
                }
                fStep = fStep * 0.5f;
            } while (fStep > 0.005f);

            if (nTicksLeft >= 2 && bClear == 1)
                pNode->m_args.m_nDriveTicksLeft = nTicksLeft - 1;
        }

        if (bClear == 0) {
            // Probe 2 is the angular bisection. vBaseL/vBaseR are the blocked bounds and
            // vDirL/vDirR the clear ones; each side halves its own angle until neither moves.
            Vector vPerp;
            vPerp.x = vMove.y;
            vPerp.y = -vMove.x;
            vPerp.z = 0.0f;
            Vector vDirR = normalize(vPerp);
            Vector vNegPerp;
            vNegPerp.x = -vDirR.x;
            vNegPerp.y = -vDirR.y;
            vNegPerp.z = -vDirR.z;
            Vector vDirL = normalize(vNegPerp);
            Vector vBaseR = vMove;
            Vector vBaseL = vMove;
            int bAliveL = 1;
            int bAliveR = 1;
            int bHitL K2_UNSET;
            int bHitR K2_UNSET;
            int i = 0;
            while (bAliveL != 0 || bAliveR != 0) {
                Vector vNextL = normalize(vBaseL + vDirL);
                Vector vNextR = normalize(vBaseR + vDirR);
                Vector vProbe;
                if (bAliveL != 0) {
                    vProbe = m_position + vNextL * fRate;
                    vFrom.x = m_position.x;
                    vFrom.y = m_position.y;
                    vFrom.z = 0.0f;
                    bHitL = pArea->TestDirectLine(vFrom, vProbe, fSpace, fHeight, 0, 0) == 1;
                }
                if (bAliveR != 0) {
                    vProbe = m_position + vNextR * fRate;
                    vFrom.x = m_position.x;
                    vFrom.y = m_position.y;
                    vFrom.z = 0.0f;
                    bHitR = pArea->TestDirectLine(vFrom, vProbe, fSpace, fHeight, 0, 0) == 1;
                }
                if (bAliveL != 0) {
                    if (bHitL != 0) {
                        vDirL = vNextL;
                    } else {
                        vBaseL = vNextL;
                        bAliveL = bHitR == 0;
                    }
                }
                if (bAliveR != 0) {
                    if (bHitR != 0) {
                        vDirR = vNextR;
                    } else {
                        vBaseR = vNextR;
                        bAliveR = bHitL == 0;
                    }
                }
                i++;
                if (i >= 15)
                    break;
            }

            Vector vBest;
            if (bAliveL == bAliveR) {
                float fDotL = vDirL.x * vMove.x + vDirL.y * vMove.y + vDirL.z * vMove.z;
                float fDotR = vDirR.x * vMove.x + vDirR.y * vMove.y + vDirR.z * vMove.z;
                vBest = fDotL > fDotR ? vDirL : vDirR;
            } else if (bAliveL != 0) {
                vBest = vDirL;
            } else if (bAliveR != 0) {
                vBest = vDirR;
            }

            float fDot = vBest.x * vMove.x + vBest.y * vMove.y + vBest.z * vMove.z;
            fSpeedFactor = fDot > 0.25f ? fDot : 0.25f;
            vGoal = m_position + vBest * (fRate * fSpeedFactor);
            vFrom.x = m_position.x;
            vFrom.y = m_position.y;
            vFrom.z = 0.0f;
            bClear = pArea->TestDirectLine(vFrom, vGoal, fSpace, fHeight, 0, 0) == 1;
        }

        if (bClear != 1) {
            m_pPathfindInfo->m_nClientPathId = nClientPathId;
            TerminateClientSidePath(nClientPathId);
            m_bDriving = 0;
            SetAnimation(SERVER_ANIMATION_PAUSE);
            return ACTION_FAILED;
        }

        m_fDriveSpeedFactor = fSpeedFactor;
        float* pPath = new float[2];
        pPath[0] = vGoal.x;
        pPath[1] = vGoal.y;
        m_pPathfindInfo->m_nWayPointFloats = 2;
        m_pPathfindInfo->m_paWayPoints = pPath;
        m_pPathfindInfo->m_bClientSidePath = 1;
        m_pPathfindInfo->m_oidCreature = m_objectId;
        m_pPathfindInfo->m_oidGoalObject = OBJECT_INVALID;
        m_pPathfindInfo->m_bPointsReversed = 0;
        m_pPathfindInfo->m_oidGoalArea = m_cachedAreaId;
        m_pPathfindInfo->m_goal.m_vPoint.x = vGoal.x;
        m_pPathfindInfo->m_goal.m_vPoint.y = vGoal.y;
        m_pPathfindInfo->m_goal.m_vPoint.z = vGoal.z;
        m_pPathfindInfo->m_vStartPoint.x = m_position.x;
        m_pPathfindInfo->m_vStartPoint.y = m_position.y;
        m_pPathfindInfo->m_vStartPoint.z = m_position.z;
        m_pPathfindInfo->m_nWayPointCursor = 0;
        m_pPathfindInfo->m_nClientPathId = nClientPathId;
    }

    vGoal.x = m_pPathfindInfo->m_goal.m_vPoint.x;
    vGoal.y = m_pPathfindInfo->m_goal.m_vPoint.y;
    vGoal.z = m_pPathfindInfo->m_goal.m_vPoint.z;

    // The drive animation table; the strafe answer is computed first and unconditionally.
    int nStrafeAnim = (nFlags & DRIVE_STRAFE_LEFT) ? SERVER_ANIMATION_STRAFE_LEFT
                    : ((nFlags & DRIVE_STRAFE_RIGHT) ? SERVER_ANIMATION_STRAFE_RIGHT : SERVER_ANIMATION_RUN);
    int nAnim;
    int bRun;
    if ((nFlags & DRIVE_RUN) == DRIVE_RUN) {
        bRun = 1;
        nAnim = (nFlags & DRIVE_STRAFE_LEFT) ? SERVER_ANIMATION_RUN_STRAFE_LEFT
              : ((nFlags & DRIVE_STRAFE_RIGHT) ? SERVER_ANIMATION_RUN_STRAFE_RIGHT : SERVER_ANIMATION_RUN);
    } else if (nFlags & DRIVE_FORWARD) {
        bRun = 0;
        nAnim = (nFlags & DRIVE_STRAFE_LEFT) ? SERVER_ANIMATION_WALK_STRAFE_LEFT
              : ((nFlags & DRIVE_STRAFE_RIGHT) ? SERVER_ANIMATION_WALK_STRAFE_RIGHT : SERVER_ANIMATION_WALK);
    } else {
        bRun = 0;
        nAnim = (nFlags & DRIVE_BACKWARD) ? SERVER_ANIMATION_WALK_BACKWARD : nStrafeAnim;
    }
    SetAnimation(static_cast<unsigned short>(nAnim));

    int nResult = DriveUpdateLocation(bRun);
    if (nResult != ACTION_IN_PROGRESS) {
        if (nTicksLeft >= 2 && nResult == ACTION_COMPLETE) {
            double dFacing = atan2(m_orientation.y, m_orientation.x) * 57.295776f;
            if (dFacing < 0.0)
                dFacing = dFacing + 360.0;
            AddDriveAction(pNode->m_nGroupId, m_position,
                           static_cast<unsigned short>(static_cast<int>(dFacing) * 10), nClientPathId, nFlags,
                           nTicksLeft - 1);
        } else {
            m_bDriving = 0;
            SetAnimation(SERVER_ANIMATION_PAUSE);
        }
    }
    return nResult;
}

int CSWSCreature::DriveUpdateLocation(int bRun)
{
    if (m_nAIUpdateDeltaMs == 0)
        return ACTION_IN_PROGRESS;
    if (m_pPathfindInfo->m_paWayPoints == 0)
        return ACTION_FAILED;

    int nResult = ACTION_IN_PROGRESS;
    Vector vNew;
    float fRate = (bRun != 0) ? GetRunRate() : GetWalkRate();
    if ((m_nAnimation & ~1) == SERVER_ANIMATION_STRAFE_LEFT)
        fRate = 1600.0001f;
    float fSpeed = m_fDriveSpeedFactor * fRate;

    Vector vTarget;
    vTarget.x = m_pPathfindInfo->m_paWayPoints[0];
    vTarget.y = m_pPathfindInfo->m_paWayPoints[1];
    vTarget.z = 0.0f;
    vTarget.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vTarget, 0, 0, 0);

    Vector vDelta = vTarget - m_position;
    Vector vDir = normalize(vDelta);

    float fStep = fSpeed * static_cast<float>(m_nAIUpdateDeltaMs) / 1000000.0f;
    if (fStep * fStep >=
        vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z) {
        vNew = vTarget;
        nResult = ACTION_COMPLETE;
    } else {
        vNew = m_position + vDir * fStep;
    }

    if (static_cast<CSWSArea*>(GetArea())->NoCreaturesOnLine(m_position, vNew, m_pPathfindInfo,
                                                 1, 0, 0, 0, 0) == 0)
        return ACTION_FAILED;
    AutoCloseGUIPanels(1);
    UpdateSubareasOnMoveTo(m_position, vNew, 0, 0, 1);

    unsigned long long oidDoor = static_cast<CSWSArea*>(GetArea())->GetDoorCrossed(m_position, vNew);
    if (oidDoor != OBJECT_INVALID) {
        CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidDoor);
        if (pDoor != 0 && pDoor->GetIsLinked() != 0 && pDoor->m_curOpenState != 0) {
            CScriptEvent* pEvent = new CScriptEvent;
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
            pEvent->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidDoor, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            return ACTION_FAILED;
        }
    }

    vNew.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vNew, 0, 0, 0);
    SetPosition(vNew, 1, 1, 0);
    return nResult;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk(int bRestartTimer)
{
    if (m_bVisibleToPlayer == 0 || m_nMoveState == 1 || GetDead() != 0)
        return ACTION_COMPLETE;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() != 0 || GetDeadTemp() != 0)
        return ACTION_COMPLETE;
#else
    if (GetDeadTemp() != 0)
        return ACTION_COMPLETE;
#endif

    unsigned int nDay = 0;
    unsigned int nTime = 0;
    unsigned int nElapsedDays = 0;
    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTime);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTime, m_nLastWalkUpdateDay, m_nLastWalkUpdateTod, &nElapsedDays, &m_nWalkDeltaMs);
    if (bRestartTimer != 0) {
        m_nWalkDeltaMs = 0;
        m_nLastWalkUpdateDay = nDay;
        m_nLastWalkUpdateTod = nTime;
    }

    if (m_nMoveState >= 4 && m_nMoveState <= 6) {
        int nPushed = WalkUpdateLocation_QuickWalk_ForcePushed();
        m_nLastWalkUpdateDay = nDay;
        m_nLastWalkUpdateTod = nTime;
        return nPushed;
    }
#if !KOTOR1  // K1: no walk state 7.
    if (m_nMoveState == 7) {
        m_nLastWalkUpdateDay = nDay;
        m_nLastWalkUpdateTod = nTime;
        return ACTION_IN_PROGRESS;
    }
#endif
    if (m_nMoveState == 3) {
        int nFollowed = WalkUpdateLocation_QuickWalk_FollowLeader();
        m_nLastWalkUpdateDay = nDay;
        m_nLastWalkUpdateTod = nTime;
        return nFollowed;
    }

    // An armed deadline that has expired gives up on steering and hands the walk to the forced-move
    // path. Nothing in the engine arms it (every write of m_bForceMoveDeadline is a 0), so this
    // block never runs.
    if (m_pPathfindInfo->m_bForceMoveDeadline != 0) {
        unsigned int nNowDay;
        unsigned int nNowTime;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nNowDay, &nNowTime);
        if (g_pAppManager->m_pServerExoApp->GetWorldTimer()->CompareWorldTimes(
                nNowDay, nNowTime, m_pPathfindInfo->m_nForceMoveDeadlineDay, m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay) >= 0) {
            m_pPathfindInfo->HandlePathfindError(
                0, const_cast<char*>("Force Timeout, executing a forced move to point."));
            CSWSForcedAction forcedAction;
            forcedAction.m_nTargetArea = m_pPathfindInfo->m_oidGoalArea;
            forcedAction.m_vPoint = m_pPathfindInfo->m_goal.m_vPoint;
            forcedAction.m_nTargetObject = m_pPathfindInfo->m_oidGoalObject;
            m_nMoveState = 1;
            ForceMoveToPoint(&forcedAction);
            // Pop the walk action the forced move has just replaced. Anything else at the head is
            // put straight back: this is a "discard if it is mine" idiom, not a real dequeue.
            CSWSObjectActionNode* pNode =
                static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
            if (pNode->m_nActionId == AIACTION_MOVE_TO_POINT) {
                m_pCurrentActionNode = 0;
                delete pNode;
                m_nActionStartDay = 0;
                m_nActionStartTod = 0;
            } else {
                m_actionList.AddHead(pNode);
            }
            SetAnimation(GetPauseReadyAnimation());
            m_pPathfindInfo->m_bForceMoveDeadline = 0;
            m_pPathfindInfo->m_nForceMoveDeadlineDay = 0;
            m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay = 0;
            return ACTION_COMPLETE;
        }
    }

    if (m_nWalkDeltaMs == 0)
        return ACTION_IN_PROGRESS;
    m_nLastWalkUpdateDay = nDay;
    m_nLastWalkUpdateTod = nTime;

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    float fRate;
#if KOTOR1  // K1: no feat-0xc5 override of the walk-only activity.
    if (GetActivity(ACTIVITY_STEALTH) == 0) {
#else
    if (GetActivity(ACTIVITY_STEALTH) == 0 || m_pStats->HasFeat(FEAT_STEALTH_RUN) != 0) {
#endif
        unsigned short bAnimate = m_nAIState & AI_STATE_MOBILE;
        if (m_bRunning != 0) {
            if (bAnimate != 0)
                SetAnimation(SERVER_ANIMATION_RUN);
            fRate = GetRunRate();
        } else {
            if (bAnimate != 0)
                SetAnimation(SERVER_ANIMATION_WALK);
            fRate = GetWalkRate();
        }
    } else {
        fRate = GetWalkRate();
    }
    if (fRate == 0.0f)
        return ACTION_IN_PROGRESS;

    if (m_bInParty != 0 && HasAction(AIACTION_PARTY_FOLLOW_LEADER) != 0) {
        int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(
            GetClientCreature()->m_objectId);
        if (nIndex >= 0) {
            Vector vLag = pParty->m_members[nIndex].m_vPathPoint
                        - pParty->m_members[nIndex].m_followPoint;
            float fLagSq = vLag.x * vLag.x + vLag.y * vLag.y + vLag.z * vLag.z;
            if (fLagSq >= 0.001f) {
#if KOTOR1  // K1: each rate is read once.
                float fRun = GetRunRate();
                float fLeader = g_pAppManager->m_pClientExoApp->GetSWParty()
                                    ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f;
                fRate = (fRun > fLeader) ? fRun : fLeader;
#else
                if (GetRunRate() > g_pAppManager->m_pClientExoApp->GetSWParty()
                                       ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f)
                    fRate = GetRunRate();
                else
                    fRate = g_pAppManager->m_pClientExoApp->GetSWParty()
                                ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f;
#endif
            }
            if (fLagSq <= 1.0f)
                fRate = fRate * 0.9f;
            else if (fLagSq <= 225.0f)
                fRate = fRate * 1.2f;
            else
                fRate = fRate * 1.5f;
        }
    }

    float fSpeed = ComputeSpeedFactor(fRate, m_nWalkDeltaMs);
    float fStep = (fSpeed + m_fLastSpeedFactor) * (fRate * static_cast<float>(m_nWalkDeltaMs)) * 0.5f / 1000000.0f;
    m_fLastSpeedFactor = fSpeed;
    fStep = NPCTurbo == 0 ? fStep : fStep * 15.0f;

    Vector vNew;
    Vector vFacing;
    CExoArrayList<unsigned long long> aCrossed;

    int nSavedCursor = m_pPathfindInfo->m_nWayPointCursor;
    int nResult = WalkUpdateLocationDistance(fStep, &vNew, &vFacing, &aCrossed);
    float fClimb = vNew.z - m_position.z;
    if (fClimb <= -0.001f || fClimb >= 0.001f) {
        Vector vSlope;
        vSlope.x = fStep;
        vSlope.y = 0.0f;
        vSlope.z = fClimb;
        float fFlat = fStep * normalize(vSlope).x;
        m_pPathfindInfo->m_nWayPointCursor = nSavedCursor;
        nResult = WalkUpdateLocationDistance(fFlat, &vNew, &vFacing, &aCrossed);
    }
    AutoCloseGUIPanels(1);
    UpdateSubareasOnMoveTo(m_position, vNew, 1, &aCrossed, 0);
    SetPosition(vNew, 1, 1, 0);
    if (m_bInParty != 0) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(oidClient);
        if (nIndex != -1)
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex].m_safePosition = vNew;
    }
    vFacing.z = 0.0f;
    vFacing.Normalize();
    SetOrientation(vFacing);
    // The party leader feeds its own footsteps to the follow-point solver the followers read.
    if (m_bInParty != 0 &&
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) != 0 &&
        m_objectId == g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                          g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0)
                              ->m_objectId)) {
        g_pAppManager->m_pClientExoApp->GetSWParty()->m_followData.ProcessPoint(
            vNew, Yaw(vFacing), 0.5f);
        CSWCCreature* pClientCreature = GetClientCreature();
        if (pClientCreature != 0)
            pClientCreature->ResetCameraToFollow();
    }

    if (m_bInParty != 0) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(oidClient);
        if (nIndex != -1) {
            CSWParty* pParty2 = g_pAppManager->m_pClientExoApp->GetSWParty();
            CSWSCreature* pLeader = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                    pParty2->m_members[0].id));
            // The leader has pulled 7 units away from where this member was last given a path:
            // re-solve the follow point and re-aim at the member's formation slot around it.
            if (pLeader != 0 && HasAction(AIACTION_PARTY_FOLLOW_LEADER) != 0 && nResult == 0) {
                Vector vPulled = pLeader->m_position
                               - pParty2->m_members[nIndex].m_vLeaderPathOrigin;
                if (vPulled.x * vPulled.x + vPulled.y * vPulled.y + vPulled.z * vPulled.z
                    >= 49.0f) {
                    g_pAppManager->m_pClientExoApp->GetSWParty()->RecaulateFollowPoint();
                    CSWPartyMemberData* pMemberData = &pParty2->m_members[nIndex];
                    // The follow point and its sample index are read out before GetPathData, which
                    // may alias the member, and written back after the heading load.
                    Vector vFollowPoint = pMemberData->m_followPoint;
                    int nSample = pMemberData->m_bFollowPointValid;
                    float fHeading = pParty2->m_followData.GetPathData(nSample)->m_fLeaderYaw;
                    pMemberData->m_vPathPoint = vFollowPoint;
                    pMemberData->m_nPathIndex = nSample;

                    Vector vOffset;
                    {
                    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fHeading));
                    float w = q.w;
                    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
                        Vector p = pMemberData->m_formationOffset;
                        float x = q.x, y = q.y, z = q.z;
                        float xx = x * x, yy = y * y, zz = z * z;
                        float xy = x * y, xz = x * z, wx = x * w;
                        float yz = y * z, wy = y * w;
                        float wz = z * w;
                        float m00 = 1.0f - ((yy + zz) + (yy + zz));
                        float m01 = (xy - wz) + (xy - wz);
                        float m02 = (xz + wy) + (xz + wy);
                        float m10 = (xy + wz) + (xy + wz);
                        float m11 = 1.0f - ((xx + zz) + (xx + zz));
                        float m12 = (yz - wx) + (yz - wx);
                        float m20 = (xz - wy) + (xz - wy);
                        float m21 = (yz + wx) + (yz + wx);
                        float m22 = 1.0f - ((xx + yy) + (xx + yy));
                        vOffset.x = p.x * m00 + p.y * m01 + p.z * m02;
                        vOffset.y = p.x * m10 + p.y * m11 + p.z * m12;
                        vOffset.z = p.x * m20 + p.y * m21 + p.z * m22;
                    } else {
                        vOffset = pMemberData->m_formationOffset;
                    }
                    }
                    Vector vDest = vOffset + vFollowPoint;
                    vDest.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vDest, 1, 0, 0);

                    Vector vCommit = {};
                    if (pMemberData->m_nFollowState == CSWPartyMemberData::FOLLOW_STATE_OFFSET_WAIT_DEST &&
                        static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vDest, m_pPathfindInfo)
                            != 0) {
                        vCommit = vDest;
                        goto commitFollowPoint;
                    }
                    if (pMemberData->m_nFollowState == CSWPartyMemberData::FOLLOW_STATE_PATH_WAIT_DEST &&
                        static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vFollowPoint,
                                                                      m_pPathfindInfo) != 0) {
                        vCommit = vFollowPoint;
                        goto commitFollowPoint;
                    }
                    return ACTION_IN_PROGRESS;

                commitFollowPoint:
                    m_nMoveState = -1;
                    m_pPathfindInfo->m_vWalkDestination = vCommit;
                    {
                        CSWSObjectActionNode* pMoveAction = GetAction(AIACTION_MOVE_TO_POINT);
                        if (pMoveAction != 0) {
                            pMoveAction->m_args.m_moveToX = vCommit.x;
                            pMoveAction->m_args.m_moveToY = vCommit.y;
                            pMoveAction->m_args.m_moveToZ = vCommit.z;
                        }
                    }
                    return ACTION_IN_PROGRESS;
                }
            }
#if !KOTOR1  // K1: no re-aim at the queued move-to point.
            // With no formation work, just re-aim at the queued move-to point, if there is one.
            if (HasAction(AIACTION_MOVE_TO_POINT) != 0 && nResult == 0) {
                CSWSObjectActionNode* pMoveAction = GetAction(AIACTION_MOVE_TO_POINT);
                Vector vTarget = {};
                if (pMoveAction != 0) {
                    vTarget.x = pMoveAction->m_args.m_moveToX;
                    vTarget.y = pMoveAction->m_args.m_moveToY;
                    vTarget.z = pMoveAction->m_args.m_moveToZ;
                }
                m_nMoveState = -1;
                m_pPathfindInfo->m_vWalkDestination = vTarget;
            }
#endif
        }
    }

    unsigned long long oidTarget = m_pPathfindInfo->m_oidGoalObject;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0) {
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
            // The creature this path is chasing has moved more than 2 units since the point was
            // stamped: re-aim at where it is now and let the next tick re-plot.
            CSWSCreature* pTarget = pObject->AsSWSCreature();
            float fdx = pTarget->m_position.x - m_pPathfindInfo->m_vWalkDestination.x;
            float fdy = pTarget->m_position.y - m_pPathfindInfo->m_vWalkDestination.y;
            float fdz = pTarget->m_position.z - m_pPathfindInfo->m_vWalkDestination.z;
            if (fdx * fdx + fdy * fdy + fdz * fdz >= 4.0f) {
                m_nMoveState = -1;
                m_pPathfindInfo->m_vWalkDestination.x = pTarget->m_position.x;
                m_pPathfindInfo->m_vWalkDestination.y = pTarget->m_position.y;
                m_pPathfindInfo->m_vWalkDestination.z = pTarget->m_position.z;
                return ACTION_IN_PROGRESS;
            }
        } else if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            if (pDoor->GetIsLinked() != 0 && GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
                if (pDoor->m_curOpenState != 0) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                    pEvent->SetObjectID(0, m_objectId);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                    SetAnimation(SERVER_ANIMATION_PAUSE);
                    return ACTION_COMPLETE;
                }
                // The door is still shut. Ask it to open, so the two ids go out the other way round
                // and no SetObjectID slot is stamped.
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidTarget, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                SetAnimation(SERVER_ANIMATION_PAUSE);
                return ACTION_IN_PROGRESS;
            }
        } else if (pObject->m_objectType == GAME_OBJECT_TYPE_TRIGGER) {
            if (pObject->AsSWSTrigger()->m_bTrap == 0 &&
                GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                SetAnimation(SERVER_ANIMATION_PAUSE);
                return ACTION_COMPLETE;
            }
        }
    }

    if (nResult == 1 && m_nMoveState != 2) {
        m_pPathfindInfo->ResetAtEndOfPath();
        m_nMoveState = 1;
    #if !KOTOR1
    m_nReserved15 = 0;
#endif
        if (m_actionList.m_pInternal->m_pHead != 0) {
            CSWSObjectActionNode* pNode =
                static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
            if (pNode->m_nActionId == AIACTION_MOVE_TO_POINT) {
                m_pCurrentActionNode = 0;
                delete pNode;
                m_nActionStartDay = 0;
                m_nActionStartTod = 0;
            } else {
                m_actionList.AddHead(pNode);
            }
        }
        SetAnimation(GetPauseReadyAnimation());
        return ACTION_COMPLETE;
    }
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::WalkUpdateLocation()
{
    if (m_nAIUpdateDeltaMs == 0)
        return ACTION_IN_PROGRESS;

    float fRate;
    float fWalkDist;
#ifdef K2_FIX_ENGINE_BUGS
    fRate = 0.0f;
    fWalkDist = 0.0f;
#endif
    if (m_nAnimation == SERVER_ANIMATION_WALK || m_nAnimation == SERVER_ANIMATION_WALK_BACKWARD) {
        fRate = GetWalkRate();
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_WALKDIST, &fWalkDist);
    } else if (m_nAnimation == SERVER_ANIMATION_RUN) {
        fRate = GetRunRate();
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_RUNDIST, &fWalkDist);
    }
    fWalkDist = fWalkDist * 1000.0f;

    float fTotal = ComputeTotalWalkDistance();
    float fRef = (fRate * 1000.0f) / 2000000.0f;
    float fSpeed = 1.0f;
    if (fTotal <= fRef) {
        fSpeed = fTotal / fRef;
        fSpeed = !(fSpeed >= 0.01f) ? 0.1f : __builtin_sqrtf(fSpeed);
        fSpeed = fSpeed + static_cast<float>(m_nAIUpdateDeltaMs) / -1000.0f;
    }
    if (m_fLastSpeedFactor <= 1.0f) {
        float fCap = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f + m_fLastSpeedFactor;
        if (fSpeed >= fCap)
            fSpeed = fCap;
    }
    float fFactor = fSpeed <= 0.1f ? 0.1f : fSpeed;
    float fStep = (m_fLastSpeedFactor + fFactor) * (fRate * static_cast<float>(m_nAIUpdateDeltaMs)) * 0.5f / 1000000.0f;
    m_fLastSpeedFactor = fFactor;
    fStep = NPCTurbo == 0 ? fStep : fStep * 15.0f;

    Vector vNew;
    Vector vDelta;
    CExoArrayList<unsigned long long> aCrossed;

    int nSavedCursor = m_pPathfindInfo->m_nWayPointCursor;
    int nResult;
    float fClimb;
    nResult = WalkUpdateLocationDistance(fStep, &vNew, &vDelta, &aCrossed);
    fClimb = vNew.z - m_position.z;
    if (fClimb <= -0.001f || fClimb >= 0.001f) {
        Vector vSlope;
        vSlope.x = fStep;
        vSlope.y = 0.0f;
        vSlope.z = fClimb;
        float fFlat = fStep * normalize(vSlope).x;
        m_pPathfindInfo->m_nWayPointCursor = nSavedCursor;
        nResult = WalkUpdateLocationDistance(fFlat, &vNew, &vDelta, &aCrossed);
    }
    AutoCloseGUIPanels(1);
    UpdateSubareasOnMoveTo(m_position, vNew, 1, &aCrossed, 0);
    SetPosition(vNew, 1, 1, 0);
    vDelta.z = 0.0f;
    vDelta.Normalize();
    SetOrientation(vDelta);
    // The party leader feeds its own footsteps to the follow-point solver the followers read.
    if (m_bInParty != 0 && g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) != 0) {
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (m_objectId == g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId))
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_followData.ProcessPoint(
                vNew, Yaw(vDelta), 0.5f);
    }

    unsigned long long oidTarget = m_pPathfindInfo->m_oidGoalObject;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0) {
        if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            if (pDoor->GetIsLinked() != 0 && GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
                if (pDoor->m_curOpenState != 0) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                    pEvent->SetObjectID(0, m_objectId);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                    SetAnimation(SERVER_ANIMATION_PAUSE);
                    return ACTION_COMPLETE;
                }
                // The door is still shut. Ask it to open, so the two ids go out the other way round
                // and no SetObjectID slot is stamped.
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidTarget, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                return ACTION_IN_PROGRESS;
            }
        } else if (pObject->m_objectType == GAME_OBJECT_TYPE_TRIGGER) {
            if (pObject->AsSWSTrigger()->m_bTrap == 0 &&
                GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                SetAnimation(SERVER_ANIMATION_PAUSE);
                return ACTION_COMPLETE;
            }
        }
    }
    if (nResult == 1) {
        m_pPathfindInfo->ResetAtEndOfPath();
        SetAnimation(SERVER_ANIMATION_PAUSE);
        return ACTION_COMPLETE;
    }
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::UpdateSubareasOnMoveTo(const Vector& from, const Vector& to, int bUsePassedList,
                                         CExoArrayList<unsigned long long>* pCrossed, int nFlag)
{
    CExoArrayList<unsigned long long> listNew;

    g_pAppManager->m_pServerExoApp->GetObjectArray();

    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return 0;

    int nPrevCount = m_currentSubareaIds.m_count;
    int bInSubAreas = pArea->InSubAreas(to, &listNew);

    if (bUsePassedList) {
        if (nPrevCount <= 0 && bInSubAreas == 0)
            return 0;
        if (HandleSubAreaStateChanges(&listNew, pCrossed, nFlag, 1) != 1)
            return 0;
        m_currentSubareaIds.m_count = 0;
        for (int64_t i = 0; i < listNew.m_count; i++)
            m_currentSubareaIds.Add(listNew.m_pArray[i]);
        return 1;
    }

    CExoArrayList<unsigned long long> listCrossed;
    pArea->IntersectingLineSegment(from, to, &listCrossed);
    if (nPrevCount <= 0 && bInSubAreas == 0)
        return 0;
    if (HandleSubAreaStateChanges(&listNew, &listCrossed, nFlag, 1) != 1)
        return 0;
    m_currentSubareaIds.m_count = 0;
    for (int64_t i = 0; i < listNew.m_count; i++)
        m_currentSubareaIds.Add(listNew.m_pArray[i]);
    return 1;
}

int CSWSCreature::UpdateSubareasOnJumpPosition(const Vector& vPos, unsigned long long oidArea,
                                               int nFlag)
{
    CExoArrayList<unsigned long long> listNew;

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();

    int nRunScripts = g_bRunSubAreaScripts ? 1 : nFlag;

    CSWSArea* pArea;
    int bHadSubareas;
    int bInSubAreas;
    if (oidArea == CGameObjectArray::INVALIDOBJID) {
        pArea = 0;
        bHadSubareas = m_currentSubareaIds.m_count > 0;
    } else {
        CGameObject* pObject;
        pObjectArray->GetGameObject(oidArea, &pObject);
        pArea = pObject->AsSWSArea();
        bHadSubareas = m_currentSubareaIds.m_count > 0;
    }
    if (pArea != 0)
        bInSubAreas = pArea->InSubAreas(vPos, &listNew);
    else
        bInSubAreas = 0;

    int64_t i;
    for (i = 0; i < m_areaOfEffectIds.m_count; i++) {
        CSWSAreaOfEffectObject* pAreaOfEffect =
            g_pAppManager->m_pServerExoApp->GetAreaOfEffectByGameObjectID(
                m_areaOfEffectIds.m_pArray[i]);
        if (pAreaOfEffect != 0) {
            pAreaOfEffect->JumpToPoint(pArea, vPos);
            listNew.AddUnique(m_areaOfEffectIds.m_pArray[i]);
        }
    }

    bHadSubareas |= (bInSubAreas != 0);
    if (bHadSubareas) {
        if (HandleSubAreaStateChanges(&listNew, 0, 0, nRunScripts) == 1) {
            m_currentSubareaIds.m_count = 0;
            for (i = 0; i < listNew.m_count; i++)
                m_currentSubareaIds.Add(listNew.m_pArray[i]);
            return 1;
        }
    }
    return 0;
}

int CSWSCreature::HandleSubAreaStateChanges(CExoArrayList<unsigned long long>* pNew,
                                            CExoArrayList<unsigned long long>* pCrossed,
                                            int bCheckAreaTransitions, int bRunScripts)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    int bChanged = 0;

    // Subareas we were standing in that the new set drops -> fire OnExit.
    for (int64_t i = 0; i < m_currentSubareaIds.m_count; i++) {
        if (pNew->Contains(m_currentSubareaIds.m_pArray[i]) != 0)
            continue;

        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                m_currentSubareaIds.m_pArray[i], &pObject) != CGameObjectArray::SUCCESS)
            continue;

        CSWSDoor* pDoor = pObject->AsSWSDoor();
        bChanged = 1;
        if (bRunScripts && pDoor == 0) {
            CScriptEvent* pEvent = new CScriptEvent;
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
            pEvent->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, m_currentSubareaIds.m_pArray[i],
                CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }

    // Subareas the new set adds -> fire OnEnter, unless a non-leader party member is walking into an
    // area transition, which vetoes the whole update.
    for (int64_t i = 0; i < pNew->m_count; i++) {
        unsigned long long objectId = pNew->m_pArray[i];
        if (m_currentSubareaIds.Contains(objectId) != 0)
            continue;

        CGameObject* pObject;
        unsigned char nStatus = pObjectArray->GetGameObject(objectId, &pObject);
        bChanged = 1;
        if (bRunScripts && nStatus == CGameObjectArray::SUCCESS) {
            CScriptEvent* pEvent = new CScriptEvent;
            if ((bCheckAreaTransitions != 0 && pObject->AsSWSTrigger() != 0 &&
                 pObject->AsSWSTrigger()->m_bAreaTransition != 0) ||
                pObject->AsSWSDoor() != 0) {
                if (m_bInParty != 0) {
                    if (g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(
                            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId)) == 0) {
                        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                    } else if (m_bInParty != 0) {
                        delete pEvent;
                        bChanged = 0;
                        continue;
                    }
                }
            } else {
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            }
            pEvent->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            bChanged = 1;
        }
    }

    if (pCrossed == 0)
        return bChanged;

    // Subareas the movement passed through. Neither loop above sees them, because their membership
    // is unchanged between the old and new sets; they still owe a script pair, in the order the
    // creature actually experienced it.
    for (int64_t i = 0; i < pCrossed->m_count; i++) {
        unsigned long long objectId = pCrossed->m_pArray[i];

        int nInCurrent = m_currentSubareaIds.Contains(objectId);
        if (nInCurrent != 0) {
            // Was inside and still is, so it was left and re-entered.
            if (pNew->Contains(objectId) == 0)
                continue;

            CGameObject* pObject;
            unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()->
                GetGameObject(objectId, &pObject);
            if (!bRunScripts || nStatus != CGameObjectArray::SUCCESS)
                continue;

            CScriptEvent* pExit = new CScriptEvent;
            pExit->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
            pExit->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pExit);

            CScriptEvent* pEnter = new CScriptEvent;
            if ((bCheckAreaTransitions != 0 && pObject->AsSWSTrigger() != 0 &&
                 pObject->AsSWSTrigger()->m_bAreaTransition != 0) ||
                pObject->AsSWSDoor() != 0)
                pEnter->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
            else
                pEnter->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEnter->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEnter);
        }
        if (nInCurrent == 0) {
            // Was outside and still is, so it was entered and left again.
            if (pNew->Contains(objectId) != 0)
                continue;

            CGameObject* pObject;
            unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()->
                GetGameObject(objectId, &pObject);
            if (!bRunScripts || nStatus != CGameObjectArray::SUCCESS)
                continue;

            CScriptEvent* pEnter = new CScriptEvent;
            if ((bCheckAreaTransitions != 0 && pObject->AsSWSTrigger() != 0 &&
                 pObject->AsSWSTrigger()->m_bAreaTransition != 0) ||
                pObject->AsSWSDoor() != 0)
                pEnter->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
            else
                pEnter->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER;
            pEnter->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEnter);

            CScriptEvent* pExit = new CScriptEvent;
            pExit->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT;
            pExit->SetObjectID(0, m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pExit);
        }
    }

    return bChanged;
}

void CSWSCreature::CleanInvitationLists()
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);

    if (m_pInvitationsOffered != 0) {
        for (int i = m_pInvitationsOffered->m_count - 1; i >= 0; i--) {
            unsigned int nDays;
            unsigned int nTime;
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
                nDay, nTimeOfDay,
                m_pInvitationsOffered->m_pArray[i].m_nDayOffered,
                m_pInvitationsOffered->m_pArray[i].m_nTimeOfDayOffered,
                &nDays, &nTime);
            if (nTime > 30000) {
                CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->
                    GetCreatureByGameObjectID(m_pInvitationsOffered->m_pArray[i].m_objectId);
                if (pCreature != 0)
                    pCreature->SetInvitedToParty(0);
                m_pInvitationsOffered->DelIndex(i);
#ifdef K2_FIX_ENGINE_BUGS
                if (pCreature == 0)
                    continue;
#endif
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, m_objectId);
                pCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_EXPIRED, pData);
            }
        }
    }

    if (m_pInvitationsIgnored != 0) {
        for (int i = m_pInvitationsIgnored->m_count - 1; i >= 0; i--) {
            unsigned int nDays;
            unsigned int nTime;
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
                nDay, nTimeOfDay,
                m_pInvitationsIgnored->m_pArray[i].m_nDayOffered,
                m_pInvitationsIgnored->m_pArray[i].m_nTimeOfDayOffered,
                &nDays, &nTime);
            if (nTime > 60000)
                m_pInvitationsIgnored->DelIndex(i);
        }
    }

    unsigned int nDays;
    unsigned int nTime;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nInvitedToPartyDay, m_nInvitedToPartyTime, &nDays, &nTime);
    if (nTime > 30000)
        SetInvitedToParty(0);
}

void CSWSCreature::ClearVisibleList()
{
    for (int i = m_visibilityList.m_count - 1; i >= 0; i--) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        if (pNode->m_objectId != m_objectId) {
            m_visibilityList.DelIndex(i);
            if (pNode != 0)
                operator delete(pNode);
        }
    }
}

void CSWSCreature::RemoveFromVisibleList(unsigned long long id)
{
    for (int i = m_visibilityList.m_count - 1; i >= 0; i--) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        if (pNode->m_objectId == id) {
            m_visibilityList.DelIndex(i);
            if (pNode != 0)
                operator delete(pNode);
        }
    }
}

float CSWSCreature::ComputeTotalWalkDistance()
{
    float fTotal = 0.0f;
    int nCount = m_pPathfindInfo->m_nWayPointFloats;
    int nCursor = m_pPathfindInfo->m_nWayPointCursor;
    if (nCursor < nCount) {
        for (int j = nCursor; j < nCount; j += 2) {
            float* paPoints = m_pPathfindInfo->m_paWayPoints;
            Vector vPoint = { paPoints[j], paPoints[j + 1], 0.0f };
            if (j == nCursor) {
                fTotal += magnitude(vPoint - m_position);
            } else {
                Vector vPrev = { paPoints[j - 2], paPoints[j - 1], 0.0f };
                fTotal += magnitude(vPoint - vPrev);
            }
        }
    }
    return fTotal;
}

float CSWSCreature::ComputeSpeedFactor(float fDistance, int nTime)
{
    float fDist = ComputeTotalWalkDistance();
    float fRef = (fDistance * 1000.0f) / 2000000.0f;
    float fNew = 1.0f;
    if (fDist <= fRef) {
        fNew = fDist / fRef;
        if (!(fNew >= 0.01f))
            fNew = 0.1f;
        else
            fNew = __builtin_sqrtf(fNew);
        fNew = fNew + static_cast<float>(nTime) / -1000.0f;
    }
    if (m_fLastSpeedFactor <= 1.0f && fNew >= static_cast<float>(nTime) / 1000.0f + m_fLastSpeedFactor)
        fNew = static_cast<float>(nTime) / 1000.0f + m_fLastSpeedFactor;
    return fNew <= 0.1f ? 0.1f : fNew;
}

unsigned char CSWSCreature::CanUnEquipWeapon(CSWSItem* pItem)
{
    CSWSItem* pRightWeapon =
            static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeftWeapon =
            static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    unsigned char nResult = 1;
    if (pRightWeapon != 0 && pRightWeapon->m_objectId == pItem->m_objectId &&
        pLeftWeapon != 0 &&
        g_pRules->GetBaseItem(pLeftWeapon->m_baseItem)->m_nWeaponType != 0) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        nResult = 2;
        if (pPlayer != 0) {
            pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[4] = OBJECT_INVALID;
            pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[5] = OBJECT_INVALID;
        }
    }
    return nResult;
}

unsigned char CSWSCreature::CanUnEquipWeapon2(CSWSItem* pItem)
{
    CSWSItem* pRightWeapon =
            static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2));
    CSWSItem* pLeftWeapon =
            static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2));
    unsigned char nResult = 1;
    if (pRightWeapon != 0 && pRightWeapon->m_objectId == pItem->m_objectId &&
        pLeftWeapon != 0 &&
        g_pRules->GetBaseItem(pLeftWeapon->m_baseItem)->m_nWeaponType != 0) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        nResult = 3;
        if (pPlayer != 0) {
            pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[15] = OBJECT_INVALID;
            pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[16] = OBJECT_INVALID;
        }
    }
    return nResult;
}

int CSWSCreature::CheckProficiencies(CSWSItem* pItem, unsigned int nSlot)
{
    K2_UNUSED(nSlot);
    if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_nEquipableSlots == 0)
        return 0;
    if (m_pStats->HasFeat(FEAT_PROFICIENCY_ALL) != 0)
        return 1;
    if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_nRequiredFeats == 0)
        return 1;
    int i = 0;
    do {
        CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
        unsigned short nFeat = pBaseItem->GetRequiredFeat(static_cast<unsigned char>(i++));
        if (m_pStats->HasFeat(nFeat) == 0)
            return 0;
    } while (i < g_pRules->GetBaseItem(pItem->m_baseItem)->m_nRequiredFeats);
    return 1;
}

int CSWSCreature::AIActionCounterSpell(CSWSObjectActionNode* pNode)
{
    unsigned long long oidTarget;
    Vector vTargetPos;
#if KOTOR1  // K1: GetIsPCDying() / GetDeadTemp(); GetDead() only gates GetIsPCDying().
    if ((GetDead() == 1 && GetIsPCDying() == 1) || GetDeadTemp() == 1)
        return ACTION_FAILED;
#else
    GetDead();  // result unused
    if (GetDeadTemp() == 1)
        return ACTION_FAILED;
#endif
    ActivityManager(4);
    oidTarget = pNode->m_params[0].m_n;
    CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget == 0 || pTarget->GetDead() != 0)
        return ACTION_FAILED;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (pTarget->GetIsPCDying() != 0 || pTarget->GetDeadTemp() != 0)
        return ACTION_FAILED;
#else
    if (pTarget->GetDeadTemp() != 0)
        return ACTION_FAILED;
#endif
    CSWSArea* pArea = static_cast<CSWSArea*>(pTarget->GetArea());
    if (pArea == 0)
        return ACTION_IN_PROGRESS;
    vTargetPos = pTarget->m_position;
    float fRange = g_pRules->m_fSpellRange[4];
    int nOne = 1;
    int nOne2 = 1;
    float dx = m_position.x - vTargetPos.x;
    float dy = m_position.y - vTargetPos.y;
    float dz = m_position.z - vTargetPos.z;
    if (pArea->m_objectId != static_cast<CSWSArea*>(GetArea())->m_objectId
            || dx * dx + dy * dy + dz * dz > fRange * fRange) {
        if ((m_nAIState & AI_STATE_MOBILE) == 0) {
            SendFeedbackMessage(FEEDBACK_ACTION_NOT_ALLOWED_IN_STATE, 0);
            return ACTION_FAILED;
        }
        AddActionToFront(AIACTION_COUNTER_SPELL, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidTarget,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nOne,
                         ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                         ACTION_PARAM_INT, &nOne2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vUp;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vTargetPos, pArea->m_objectId,
                                    oidTarget, nOne, fRange, 0.0f, 0, 0, 0, 0, 0, 0, vUp, 0, 0, 0);
        AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
        return ACTION_COMPLETE;
    }
    BroadcastCombatStateToParty(oidTarget);
    ResolveInitiative();
    SetCombatMode(CREATURE_COMBAT_MODE_COUNTERSPELL, 0);
    if (m_counterSpellTargetId != oidTarget)
        m_counterSpellTargetId = oidTarget;
    if (GetLockOrientationToObject() != oidTarget)
        SetLockOrientationToObject(oidTarget, 0);
    return ACTION_IN_PROGRESS;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSCreature::AIActionStanceTotalDefense(CSWSObjectActionNode* pNode)
{
    m_nEndRoundScriptTimer = 3000;
    int bAlive = 1;
    if (m_bInParty != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        int nIndex = pClient->GetSWParty()->GetIndex(
            pClient->ServerToClientObjectId(m_objectId));
        if (nIndex >= 0)
            pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
        bAlive = GetDeadTemp() == 0;
    }
    if (GetDead() == 0 && bAlive != 0 && m_pCombatRound != 0) {
        if (g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0
                || m_pCombatRound->m_bRoundPaused != 0)
            return ACTION_IN_PROGRESS;
        unsigned long long oidTarget = pNode->m_args.m_aqTargetId;
        if (m_nStanceStartDay == 0 && m_nStanceStartTime == 0) {
            SetAnimation(SERVER_ANIMATION_READY);
            SetAttemptedAttackTarget(oidTarget);
            SetLockOrientationToObject(oidTarget, 0);
            ActivityManager(4);
        }
        if (m_nStancePosture != STANCE_POSTURE_TOTAL_DEFENSE)
            m_nStancePosture = STANCE_POSTURE_TOTAL_DEFENSE;
        if (m_pCombatRound->m_bRoundStarted == 0) {
            int nEngaged = m_pCombatRound->ResolveEngagedState(oidTarget);
            int nMaster = m_pCombatRound->ResolveMasterState(oidTarget, nEngaged);
            if (nEngaged != 0) {
                if (nMaster != 0) {
                    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(
                        &m_nStanceStartDay, &m_nStanceStartTime);
                    m_pCombatRound->StartCombatRound(oidTarget, nEngaged, nMaster, 0, 0);
                    CSWSCreature* pTarget =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
                    if (pTarget != 0) {
                        pTarget->m_pCombatRound->StartCombatRound(m_objectId, nEngaged, 0, 0, 0);
                        pTarget->m_bInitiativeExpired = 1;
                    }
                }
            } else {
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(
                    &m_nStanceStartDay, &m_nStanceStartTime);
                m_pCombatRound->StartCombatRound(oidTarget, nEngaged, nMaster, 0, 0);
            }
        }
        if (m_pCombatRound->m_bRoundStarted == 0)
            return ACTION_IN_PROGRESS;
        unsigned int nDay;
        unsigned int nTime;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
        unsigned int nElapsedDay;
        unsigned int nElapsedTime;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
            nDay, nTime, m_nStanceStartDay, m_nStanceStartTime, &nElapsedDay, &nElapsedTime);
        if (nElapsedTime >= 1500) {
            m_nStanceStartDay = 0;
            m_nStanceStartTime = 0;
            m_nStancePosture = STANCE_POSTURE_NONE;
            return ACTION_COMPLETE;
        }
        return ACTION_IN_PROGRESS;
    }
    CancelAllCombatModes(1);
    return ACTION_FAILED;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSCreature::AIActionStanceMeditative(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
    // inlined CSWSCreature::GetDeadTemp()
    // inlined CSWSCreature::GetDeadTemp(): the call merges both tests into one orr (LENGTH +4)
    int bAlive = 1;
    if (m_bInParty != 0)
        bAlive = GetCurrentHitPoints(0) > 0;
    int nDead = GetDead();
    int nResult = ACTION_FAILED;
    if (bAlive != 0 && nDead == 0) {
        ActivityManager(2);
        if (m_nStancePosture == STANCE_POSTURE_MEDITATIVE) {
            if (m_actionList.m_pInternal->m_nCount >= 2) {
                m_nStancePosture = STANCE_POSTURE_NONE;
                RecomputeAmbientAnimationState();
                return ACTION_COMPLETE;
            }
        } else {
            m_nStancePosture = STANCE_POSTURE_MEDITATIVE;
            RecomputeAmbientAnimationState();
        }
        nResult = ACTION_IN_PROGRESS;
    }
    return nResult;
}
#endif // !KOTOR1

// The `m_bInParty != 0` re-test inside the HP clause is dead, since the first clause already
// returned when it was zero.
int CSWSCreature::AIActionPartyFollowLeader(CSWSObjectActionNode* pNode)
{
    K2_UNUSED(pNode);
#if KOTOR1  // K1: GetIsPCDying() / GetDeadTemp(); the leader is the server's player creature.
    if (m_bInParty == 0 || m_pFollowInfo == 0 || GetDead() == 1 || GetIsPCDying() == 1 ||
        GetDeadTemp() == 1 || m_pPathfindInfo == 0 || !(m_nAIState & AI_STATE_MOBILE)) {
        return ACTION_FAILED;
    }
    if (GetArea() != 0) {
        if (g_pAppManager->m_pServerExoApp->GetPlayerCreatureId() == m_objectId) {
            return ACTION_FAILED;
        }
        m_nMoveState = 3;
    }
#else
    if (m_bInParty == 0 || m_pFollowInfo == 0 || GetDead() == 1 ||
        GetDeadTemp() == 1 ||
        m_pPathfindInfo == 0 || !(m_nAIState & AI_STATE_MOBILE)) {
        return ACTION_FAILED;
    }
    if (GetArea() != 0) {
        unsigned long long leaderId = g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[0].id;
        if (g_pAppManager->m_pServerExoApp->ClientToServerObjectId(leaderId) == m_objectId) {
            return ACTION_FAILED;
        }
        m_nMoveState = 3;
    }
#endif
    return ACTION_IN_PROGRESS;
}

#if !KOTOR1
int CSWSCreature::AIActionFollowOwner(CSWSObjectActionNode* pNode)
{
    if (m_isPuppet == 0 || GetDead() != 0)
        return ACTION_FAILED;
    if (GetDeadTemp() != 0)
        return ACTION_FAILED;
    if (!(m_nAIState & AI_STATE_MOBILE))
        return ACTION_FAILED;

    if (GetArea() == 0)
        return ACTION_IN_PROGRESS;
    float fFollowDist = pNode->m_args.m_followDist;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0)
        return ACTION_IN_PROGRESS;
    CSWSCreature* pOwner =
        pPartyTable->GetPUPOwner(pPartyTable->GetPUPID(m_objectId));
    if (pOwner == 0)
        return ACTION_IN_PROGRESS;

    {
        Vector vDiff;
        vDiff.x = pOwner->m_position.x - m_position.x;
        vDiff.y = pOwner->m_position.y - m_position.y;
        vDiff.z = 0.0f;
        if (!(magnitude(vDiff) >= fFollowDist))
            return ACTION_IN_PROGRESS;
    }

    Vector vDest(pOwner->m_position.x, pOwner->m_position.y, pOwner->m_position.z);
    AddMoveToPointAction(ACTION_GROUP_NEW, vDest,
                         static_cast<CSWSArea*>(pOwner->GetArea())->m_objectId,
                         pOwner->m_objectId, 1, fFollowDist, 0.0f,
                         0, 0, 0, 0, 0, 0, Vector(), 0, 0, 0);
    AddAction(AIACTION_FOLLOW_OWNER, ACTION_GROUP_NEW, ACTION_PARAM_FLOAT, &fFollowDist, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return ACTION_COMPLETE;
}
#endif

int CSWSCreature::AIActionEncounterCreatureDestroySelf()
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1) {
#else
    if (GetDead() == 1 || GetDeadTemp() == 1) {
#endif
        return ACTION_FAILED;
    }
    CSWSEncounter* pEncounter =
        g_pAppManager->m_pServerExoApp->GetEncounterByGameObjectID(m_oidEncounter);
    if (pEncounter != 0) {
        pEncounter->RemoveFromActiveCreatureCount(m_pStats->m_fChallengeRating, 1);
        m_bEncounterSlotReleased = 1;
    }
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 5000, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    m_bCommandable = 0;
    return ACTION_COMPLETE;
}

int CSWSCreature::PlotPathFromTo(const Vector& vFrom, const Vector& vTo)
{
    if (m_pPathfindInfo->m_bNewPathRequested == 1) {
        m_pPathfindInfo->m_oidCreature = m_objectId;
        m_pPathfindInfo->m_oidGoalObject = OBJECT_INVALID;
        m_bDriving = 0;
        CPathfindInformation* p = m_pPathfindInfo;
        p->m_goal.m_vPoint = vTo;
        p->m_oidGoalArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
        p = m_pPathfindInfo;
        p->m_vStartPoint = vFrom;
        p->m_oidStartArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
        m_pPathfindInfo->m_bPlotFinished = 0;
    } else if (m_pPathfindInfo->m_bPlotFinished != 0) {
        return ACTION_IN_PROGRESS;
    }
    m_pPathfindInfo->m_bStraightLineOnly = 0;
    return g_pAppManager->m_pServerExoApp->GetModule()->PlotPath(m_pPathfindInfo, AI_TIME_SLICE);
}

int CSWSCreature::AIActionMoveToPoint(CSWSObjectActionNode* pNode)
{
    if (GetDead() == 1)
        return ACTION_FAILED;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() == 1 || GetDeadTemp() == 1)
        return ACTION_FAILED;
#else
    if (GetDeadTemp() == 1)
        return ACTION_FAILED;
#endif
    if (m_bVisibleToPlayer != 0) {
        if (m_nMoveState == 0 || m_nMoveState == 4 || m_nMoveState == 5 || m_nMoveState == 6)
            return ACTION_IN_PROGRESS;
    } else {
        m_nMoveState = -1;
    }
    if (m_pPathfindInfo == 0)
        return ACTION_FAILED;

    unsigned int nDeadlineDay = 0;
    unsigned int nDeadlineTod = 0;
    unsigned int nNowDay;
    unsigned int nNowTod;
    Vector vOffset = {};
    Vector vDest;
    vDest.x = pNode->m_args.m_moveToX;
    vDest.y = pNode->m_args.m_moveToY;
    vDest.z = pNode->m_args.m_moveToZ;
    unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
    unsigned long long oidTarget = OBJECT_INVALID;
    int nFlags = 0;
    if (pNode->m_nNumParameters >= 5) {
        oidTarget = pNode->m_args.m_moveToTargetId;
        if (pNode->m_nNumParameters >= 6)
            nFlags = pNode->m_args.m_moveToParam;
    }
    int bCheckSight = (nFlags >> 10) & 1;
    m_pPathfindInfo->m_bCheckSightToTarget = bCheckSight;
    int bRun = (m_bMovementSpeedLimited == 1) ? 0 : (nFlags & 1);
    int nClientPathId = 0;
    float fRequeueDelay = 0.0f;
    float fRange;
    if (pNode->m_nNumParameters >= 7) {
        fRange = pNode->m_args.m_moveToRadius;
        Vector vLosHit = {};
        unsigned long long oidLosHit;
        float dx = vDest.x - m_position.x;
        float dy = vDest.y - m_position.y;
        float dz = vDest.z - m_position.z;
#if KOTOR1  // K1: no area test outside the in-range line-of-sight trim.
        if (dx * dx + dy * dy + dz * dz < fRange * fRange) {
#else
        if (GetArea() == 0) {
            fRange = 0.0f;
        } else if (dx * dx + dy * dy + dz * dz < fRange * fRange) {
#endif
            if (GetArea() == 0
                    || static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(m_position, vDest, &vLosHit,
                                                                &oidLosHit, OBJECT_INVALID,
                                                                OBJECT_INVALID, 0, 0) == 0)
                fRange = 0.0f;
        }
    } else {
        fRange = 0.0f;
    }
    if (pNode->m_nNumParameters >= 8) {
        nClientPathId = pNode->m_args.m_nMoveToClientPathId;
        if (pNode->m_nNumParameters >= 9) {
            fRequeueDelay = pNode->m_args.m_moveToRequeueDelay;
            if (pNode->m_nNumParameters >= 10) {
                vOffset.x = pNode->m_args.m_moveToOffsetX;
                vOffset.y = pNode->m_args.m_moveToOffsetY;
                if (pNode->m_nNumParameters >= 12) {
                    nDeadlineDay = pNode->m_args.m_moveToDeadlineDay;
                    nDeadlineTod = pNode->m_args.m_moveToDeadlineTod;
                }
            }
        }
    }

    if (nDeadlineDay != 0 && nDeadlineTod != 0) {
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nNowDay, &nNowTod);
        if (g_pAppManager->m_pServerExoApp->GetWorldTimer()->CompareWorldTimes(
                nNowDay, nNowTod, nDeadlineDay, nDeadlineTod) >= 0) {
            m_pPathfindInfo->HandlePathfindError(
                0, const_cast<char*>("Force Timeout, executing a forced move to point."));
            CSWSForcedAction forcedAction;
            forcedAction.m_nTargetArea = oidArea;
            forcedAction.m_vPoint = vDest;
            forcedAction.m_nTargetObject = oidTarget;
            m_nMoveState = 1;
            ForceMoveToPoint(&forcedAction);
            return ACTION_COMPLETE;
        }
    }

    int bClientSidePath = (nFlags >> 1) & 1;
    int nFlagF = (nFlags >> 3) & 1;
    int nFlagD = (nFlags >> 4) & 7;
    int nFlagE = (nFlags >> 7) & 3;
    int nFlagG = (nFlags >> 9) & 1;

    if ((nFlags & 4) != 0 && fRequeueDelay > 0.0f) {
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nNowDay, &nNowTod);
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->AddWorldTimes(
            nNowDay, nNowTod, 0, static_cast<int>(fRequeueDelay * 1000.0f),
            &nDeadlineDay, &nDeadlineTod);
        AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea, oidTarget, bRun,
                                    fRange, 0.0f, bClientSidePath, nClientPathId,
                                    nFlagD, nFlagE, nFlagF, nFlagG, vOffset,
                                    nDeadlineDay, nDeadlineTod,
                                    m_pPathfindInfo->m_bCheckSightToTarget);
        m_pPathfindInfo->m_bForceMoveDeadline = 0;
        m_pPathfindInfo->m_nForceMoveDeadlineDay = 0;
        m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay = 0;
        m_nMoveState = 1;
        return ACTION_COMPLETE;
    }

    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            == CGameObjectArray::SUCCESS) {
        switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE: {
            if (g_pAppManager->m_pServerExoApp->GetModule() == 0)
                break;
            CSWSCreature* pTargetCre = pObject->AsSWSCreature();
            m_pPathfindInfo->m_vWalkDestination = pTargetCre->m_position;
            if (pTargetCre->GetArea() == 0) {
                Vector vLastKnown = pTargetCre->m_vDesiredAreaLocation;
                if (oidArea == pTargetCre->m_currentAreaId) {
                    float dx = vLastKnown.x - vDest.x;
                    float dy = vLastKnown.y - vDest.y;
                    float dz = vLastKnown.z - vDest.z;
                    if (!(dx * dx + dy * dy + dz * dz > 4.0f))
                        break;
                }
                AddMoveToPointActionToFront(pNode->m_nGroupId, vLastKnown,
                                            pTargetCre->m_currentAreaId, oidTarget, bRun,
                                            fRange, 0.0f, bClientSidePath, nClientPathId,
                                            nFlagD, nFlagE, nFlagF, nFlagG, vOffset,
                                            nDeadlineDay, nDeadlineTod, 0);
                return ACTION_COMPLETE;
            }
            unsigned long long oidTargetArea =
                static_cast<CSWSArea*>(pTargetCre->GetArea())->m_objectId;
            Vector vGoal = pTargetCre->m_position;
            if (vOffset.x != 0.0f || vOffset.y != 0.0f) {
                Vector vAnchor =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->m_followAnchorPosition;
                Vector vRotated = RotateVector(
                    vOffset,
                    g_pAppManager->m_pClientExoApp->GetSWParty()->m_followAnchorOrientation);
                vGoal.x = vAnchor.x + vRotated.x;
                vGoal.y = vAnchor.y + vRotated.y;
                vGoal.z = vAnchor.z + vRotated.z;
                oidTarget = OBJECT_INVALID;
            }
            float fGap = magnitude(m_position - vGoal);
            float gx = vDest.x - vGoal.x;
            float gy = vDest.y - vGoal.y;
            float gz = vDest.z - vGoal.z;
            float fGoalDistSq = gx * gx + gy * gy + gz * gz;
            float fCap = fGap / 2.5f;
            fCap = (fCap > 2.0f) ? fCap : 2.0f;
            if (oidTargetArea != oidArea || fGoalDistSq > fCap * fCap) {
                static_cast<CSWSArea*>(pTargetCre->GetArea())->ComputeSafeLocation(vGoal, 20.0f,
                                                                       m_pPathfindInfo, 1, &vGoal);
                float sx = pTargetCre->m_position.x - vGoal.x;
                float sy = pTargetCre->m_position.y - vGoal.y;
                float sz = pTargetCre->m_position.z - vGoal.z;
                if (fGoalDistSq > sx * sx + sy * sy + sz * sz) {
                    AddMoveToPointActionToFront(pNode->m_nGroupId, vGoal, oidTargetArea,
                                                oidTarget, bRun, fRange, 0.0f, bClientSidePath,
                                                nClientPathId, nFlagD, nFlagE, nFlagF, nFlagG,
                                                vOffset, nDeadlineDay, nDeadlineTod, 0);
                    return ACTION_COMPLETE;
                }
            }
            Vector vUsePos = {};
            float fUseRange;
            GetUseRange(oidTarget, vUsePos, fUseRange, 0, 0);
            if (vOffset.x == 0.0f && vOffset.y == 0.0f) {
                if (GetIsInUseRange(oidTarget, fRange - fUseRange, 0, 0) == 0)
                    break;
                m_pPathfindInfo->ResetAtEndOfPath();
                SetAnimation(SERVER_ANIMATION_PAUSE);
                m_nMoveState = 1;
                return ACTION_COMPLETE;
            }
            float ax = m_position.x - vGoal.x;
            float ay = m_position.y - vGoal.y;
            float az = m_position.z - vGoal.z;
            if (!(ax * ax + ay * ay + az * az < 0.001f))
                break;
            m_pPathfindInfo->ResetAtEndOfPath();
            m_nMoveState = 1;
            SetAnimation(SERVER_ANIMATION_PAUSE);
            return ACTION_COMPLETE;
        }
        case GAME_OBJECT_TYPE_TRIGGER: {
            Vector vNearest = {};
            m_pPathfindInfo->m_vWalkDestination = pObject->AsSWSTrigger()->m_position;
            if (pObject->AsSWSTrigger()->m_bTrap == 0) {
                if (GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0)
                    break;
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                return ACTION_COMPLETE;
            }
            pObject->AsSWSTrigger()->CalculateNearestPoint(m_position, &vNearest);
            vDest = vNearest + normalize(m_position - vNearest) * 0.5f;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea, OBJECT_INVALID, bRun,
                                        fRange, 0.0f, bClientSidePath, nClientPathId,
                                        nFlagD, nFlagE, nFlagF, nFlagG, vOffset,
                                        nDeadlineDay, nDeadlineTod, 0);
            return ACTION_COMPLETE;
        }
        case GAME_OBJECT_TYPE_PLACEABLE: {
            m_pPathfindInfo->m_vWalkDestination = pObject->AsSWSPlaceable()->m_position;
            if (nFlagG != 0)
                break;
            pObject->AsSWSPlaceable();
            Vector vUsePos = {};
            float fUseRange;
            GetUseRange(oidTarget, vUsePos, fUseRange, 0, 0);
            if (fRange == fUseRange) {
                if (!(magnitude(vUsePos - vDest) > 0.001f))
                    break;
            }
            AddMoveToPointActionToFront(pNode->m_nGroupId, vUsePos, oidArea, oidTarget, bRun,
                                        fUseRange, 0.0f, bClientSidePath, nClientPathId,
                                        nFlagD, nFlagE, nFlagF, 1, vOffset,
                                        nDeadlineDay, nDeadlineTod, 0);
            m_nMoveState = 1;
            return ACTION_COMPLETE;
        }
        case GAME_OBJECT_TYPE_DOOR: {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            m_pPathfindInfo->m_vWalkDestination = pDoor->m_position;
            if (pDoor->GetIsLinked() != 0 && GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
                if (pDoor->m_curOpenState != 0) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                    pEvent->SetObjectID(0, m_objectId);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                    SetAnimation(SERVER_ANIMATION_PAUSE);
                    m_pPathfindInfo->ResetGridSearchData();
                    m_pPathfindInfo->ResetWayPointData();
                    m_pPathfindInfo->ResetInterTileSearchData();
                    m_nMoveState = 1;
                    return ACTION_COMPLETE;
                }
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidTarget, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                return ACTION_IN_PROGRESS;
            }
            if (nFlagG != 0)
                break;
            pObject->AsSWSDoor();
            Vector vUsePos = {};
            float fUseRange;
            GetUseRange(oidTarget, vUsePos, fUseRange, 0, 0);
            if (fRange == fUseRange) {
                if (!(magnitude(vUsePos - vDest) > 0.001f))
                    break;
            }
            AddMoveToPointActionToFront(pNode->m_nGroupId, vUsePos, oidArea, oidTarget, bRun,
                                        fUseRange, 0.0f, bClientSidePath, nClientPathId,
                                        nFlagD, nFlagE, nFlagF, 1, vOffset,
                                        nDeadlineDay, nDeadlineTod, 0);
            m_nMoveState = 1;
            return ACTION_COMPLETE;
        }
        }
    }

    if ((m_nAIState & AI_STATE_MOBILE) == 0) {
        if (bClientSidePath != 0)
            TerminateClientSidePath(nClientPathId);
        m_pPathfindInfo->HandlePathfindError(
            0, const_cast<char*>("I have no legs! I can't feel my legs!!!!"));
        return ACTION_FAILED;
    }
    if (GetArea() == 0)
        return ACTION_IN_PROGRESS;
    if (GetArea() != 0
            && static_cast<CSWSArea*>(GetArea())->m_objectId == oidArea) {
        float dx = vDest.x - m_position.x;
        float dy = vDest.y - m_position.y;
        float dz = vDest.z - m_position.z;
        if (0.01f >= dx * dx + dy * dy + dz * dz) {
            if (static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vDest, m_pPathfindInfo) != 0)
                SetPosition(vDest, 1, 1, 0);
            else
                m_pPathfindInfo->HandlePathfindError(
                    0, const_cast<char*>("Bailed the desired position is unsafe."));
            SetAnimation(SERVER_ANIMATION_READY);
            return ACTION_COMPLETE;
        }
    }
    m_pPathfindInfo->m_bClientSidePath = bClientSidePath;
    m_pPathfindInfo->m_oidCreature = m_objectId;
    m_pPathfindInfo->m_oidGoalObject = oidTarget;
    m_bDriving = 0;
    Vector* pvPlotted;
    unsigned long long* poidPlotted;
    // The points are CPathfindPos, a layout twin of Vector, so the address needs a cast.
    if (m_pPathfindInfo->m_bPointsReversed != 0) {
        pvPlotted = reinterpret_cast<Vector*>(&m_pPathfindInfo->m_vStartPoint);
        poidPlotted = &m_pPathfindInfo->m_oidStartArea;
    } else {
        pvPlotted = reinterpret_cast<Vector*>(&m_pPathfindInfo->m_goal.m_vPoint);
        poidPlotted = &m_pPathfindInfo->m_oidGoalArea;
    }
    float px = vDest.x - pvPlotted->x;
    float py = vDest.y - pvPlotted->y;
    float pz = vDest.z - pvPlotted->z;
    if (*poidPlotted != oidArea || px * px + py * py + pz * pz >= 0.01f) {
        m_pPathfindInfo->m_goal.m_vPoint.x = vDest.x;
        m_pPathfindInfo->m_goal.m_vPoint.y = vDest.y;
        m_pPathfindInfo->m_goal.m_vPoint.z = vDest.z;
        m_pPathfindInfo->m_oidGoalArea = oidArea;
        m_pPathfindInfo->m_goal.m_fRange = fRange;
        m_pPathfindInfo->ComputeStepTolerance();
        m_pPathfindInfo->m_vStartPoint.x = m_position.x;
        m_pPathfindInfo->m_vStartPoint.y = m_position.y;
        m_pPathfindInfo->m_vStartPoint.z = m_position.z;
        m_pPathfindInfo->m_oidStartArea = m_cachedAreaId;
        m_pPathfindInfo->m_bNewPathRequested = 1;
        m_pPathfindInfo->m_bPlotFinished = 0;
    } else if (m_pPathfindInfo->m_bPlotFinished != 0) {
        SetAnimation(bRun == 0 ? SERVER_ANIMATION_WALK : SERVER_ANIMATION_RUN);
        g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)
            ->GetWorldTime(&m_nLastWalkUpdateDay, &m_nLastWalkUpdateTod);
        ActivityManager(8);
        m_nMoveState = 0;
        if (m_bVisibleToPlayer != 0)
            return ACTION_IN_PROGRESS;
        return WalkUpdateLocation();
    }
    m_pPathfindInfo->m_bStraightLineOnly = nFlagF;
    m_pPathfindInfo->m_nBumpCount = 0;
    if (m_attemptedAttackTargetId != OBJECT_INVALID) {
        float rx = vDest.x - m_position.x;
        float ry = vDest.y - m_position.y;
        float rz = vDest.z - m_position.z;
        if (rx * rx + ry * ry + rz * rz < 25.0f)
            m_pPathfindInfo->m_bGoalWithinReach = 1;
    }
    if (m_bVisibleToPlayer != 0)
        m_nMoveState = 2;
    if (m_bIgnoreCrePath != 0)
        m_pPathfindInfo->m_bIgnoreCrePath = 1;
    int nPlotResult =
        g_pAppManager->m_pServerExoApp->GetModule()->PlotPath(m_pPathfindInfo, AiTimeSlice);
    m_pPathfindInfo->m_bStraightLineOnly = 0;
    if (nPlotResult == ACTION_COMPLETE) {
        unsigned long long oidPlotTarget = m_pPathfindInfo->m_oidGoalObject;
        m_oidAttemptedMovementTarget = oidPlotTarget;
        m_pPathfindInfo->m_oidPathTarget = OBJECT_INVALID;
        m_pPathfindInfo->m_vPathTarget.x = 0.0f;
        m_pPathfindInfo->m_vPathTarget.y = 0.0f;
        m_pPathfindInfo->m_vPathTarget.z = 0.0f;
        if (oidPlotTarget != oidTarget)
            pNode->m_args.m_moveToTargetId = oidPlotTarget;
        m_pPathfindInfo->m_nClientPathId = static_cast<unsigned char>(nClientPathId);
        m_pPathfindInfo->m_nWayPointCursor = 0;
        SetAnimation(bRun == 0 ? SERVER_ANIMATION_WALK : SERVER_ANIMATION_RUN);
        m_bRunning = bRun;
        ActivityManager(8);
        int nWalkResult;
        if (m_bVisibleToPlayer != 0) {
            g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)
                ->GetWorldTime(&m_nLastWalkUpdateDay, &m_nLastWalkUpdateTod);
            m_nMoveState = 0;
            nWalkResult = WalkUpdateLocation_QuickWalk(1);
        } else {
            nWalkResult = WalkUpdateLocation();
        }
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
        return nWalkResult;
    }
    if (nPlotResult != ACTION_FAILED)
        return nPlotResult;
    m_pPathfindInfo->HandlePathfindError(0, const_cast<char*>("The Path find has Failed... Why?"));
    if (m_bVisibleToPlayer != 0)
        m_nMoveState = 2;
    m_pPathfindInfo->m_nClientPathId = static_cast<unsigned char>(nClientPathId);
    SetAnimation(SERVER_ANIMATION_PAUSE);
    if (bClientSidePath != 0)
        TerminateClientSidePath(nClientPathId);
    m_pPathfindInfo->m_goal.m_vPoint.x = m_position.x;
    m_pPathfindInfo->m_goal.m_vPoint.y = m_position.y;
    m_pPathfindInfo->m_goal.m_vPoint.z = m_position.z;
    m_pPathfindInfo->m_oidGoalArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
#if !KOTOR1  // K1: no random re-plan wait after a failed path.
    AddPathfindingWaitActionToFront(pNode->m_nGroupId,
                                    static_cast<float>(rand_wincompatible() % 100) / 50.0f + 2.0f);
#endif
    if (bNoForceMoves != 0)
        return ACTION_FAILED;
    if (nDeadlineDay == 0)
        return ACTION_FAILED;
    if (nDeadlineTod == 0)
        return ACTION_FAILED;
    SetPosition(vDest, 1, 1, 0);
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocationDistance(float fDistance, Vector* pvPosition, Vector* pvDelta,
                                             CExoArrayList<unsigned long long>* pCrossed)
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return 0;

    int nPathCount = m_pPathfindInfo->m_nWayPointFloats;
    int nCursor    = m_pPathfindInfo->m_nWayPointCursor;

    Vector vPoint;
    Vector vLeg;
    Vector vCur = m_position;
    Vector vFrom;
    Vector vTo;

    Vector vFacing = m_orientation;
    *pvDelta = vFacing;

    unsigned long long oidBlocker;

    if (m_pPathfindInfo->m_oidGoalObject != OBJECT_INVALID) {
        Vector vUsePos = {};
        float  fUseRange = 0.0f;
        if (GetUseRange(m_pPathfindInfo->m_oidGoalObject, vUsePos, fUseRange, 0, 0) != 0) {
            float fdx = m_position.x - vUsePos.x;
            float fdy = m_position.y - vUsePos.y;
            float fdz = m_position.z - vUsePos.z;
            if (fdx * fdx + fdy * fdy + fdz * fdz <= fUseRange * fUseRange) {
                SetAnimation(GetPauseReadyAnimation());
                *pvPosition = m_position;
                return 1;
            }
        }
    }

    int bWalk = 1;
    if (m_pPathfindInfo->m_bCheckSightToTarget != 0) {
        Vector vTargetPos = {};
        float  fTargetRange = 0.0f;
        if (m_pPathfindInfo->m_oidGoalObject != OBJECT_INVALID &&
            GetUseRange(m_pPathfindInfo->m_oidGoalObject, vTargetPos, fTargetRange, 0, 0) != 0 &&
            m_pPathfindInfo->m_goal.m_fRange > fTargetRange) {
            Vector vEye = m_position;
            vEye.z = vEye.z + 1.5f;
            Vector vTargetEye = vTargetPos;
            vTargetEye.z = vTargetEye.z + 1.5f;
            Vector vSightHit = {};
            float fgx = m_position.x - vTargetPos.x;
            float fgy = m_position.y - vTargetPos.y;
            float fgz = m_position.z - vTargetPos.z;
            if (fgx * fgx + fgy * fgy + fgz * fgz <=
                m_pPathfindInfo->m_goal.m_fRange * m_pPathfindInfo->m_goal.m_fRange) {
                unsigned long long oidSightHit;
                if (static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vEye, vTargetEye, &vSightHit,
                                                             &oidSightHit, m_objectId,
                                                             m_pPathfindInfo->m_oidGoalObject, 0, 0) != 0 &&
                    static_cast<CSWSArea*>(GetArea())->NoNonWalkPolysDetailed(
                        vEye, vTargetEye, vEye.z - 0.01f, vEye.z + 0.01f,
                        m_pPathfindInfo->m_fPersonalSpace, OBJECT_INVALID, oidSightHit,
                        1, 0, 0, 0, 0) == 1) {
                    SetAnimation(GetPauseReadyAnimation());
                    *pvPosition = m_position;
                    bWalk = 0;
                }
            }
        }
#if KOTOR1  // K1: also stop once the path's last point is in sight range and visible.
        if (bWalk != 0 && nPathCount >= 3) {
            float* paEnd = m_pPathfindInfo->m_paWayPoints;
            Vector vEnd;
            vEnd.x = paEnd[nPathCount - 2];
            vEnd.y = paEnd[nPathCount - 1];
            vEnd.z = 0.0f;
            Vector vToEnd;
            vToEnd.x = vEnd.x - m_position.x;
            vToEnd.y = vEnd.y - m_position.y;
            vToEnd.z = 0.0f;
            if (vToEnd.x * vToEnd.x + vToEnd.y * vToEnd.y + vToEnd.z * vToEnd.z <=
                m_pPathfindInfo->m_goal.m_fRange * m_pPathfindInfo->m_goal.m_fRange) {
                Vector vEye = m_position;
                vEye.z = vEye.z + 1.5f;
                vEnd.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vEnd, 0, 0, 0) + 1.5f;
                Vector vSightHit = {};
                unsigned long long oidSightHit;
                if (static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vEye, vEnd, &vSightHit,
                                                             &oidSightHit, m_objectId,
                                                             OBJECT_INVALID, 0, 0) != 0 &&
                    static_cast<CSWSArea*>(GetArea())->NoNonWalkPolysDetailed(
                        vEye, vEnd, vEye.z - 0.01f, vEye.z + 0.01f,
                        m_pPathfindInfo->m_fPersonalSpace, OBJECT_INVALID, oidSightHit,
                        1, 0, 0, 0, 0) == 1) {
                    SetAnimation(GetPauseReadyAnimation());
                    *pvPosition = m_position;
                    bWalk = 0;
                }
            }
        }
#endif
    }

    if (bWalk == 0)
        return 1;

    {
        // "Nothing left to walk" is itself a completed walk: an empty path, or a cursor already
        // sitting on the end of one.
        int nResult = (nCursor == nPathCount) || (nPathCount == 0);
        float fRemaining = fDistance;
        int   nLastLeg = nPathCount - 2;

        while (nCursor < nPathCount) {
            float fRemSq = fRemaining * fRemaining;
            float* paPath = m_pPathfindInfo->m_paWayPoints;
            float fNextX = paPath[nCursor];
            float fNextY = paPath[nCursor + 1];
            vPoint.x = fNextX;
            vPoint.y = fNextY;
            vPoint.z = 0.0f;
            vLeg.x = fNextX - vCur.x;
            vLeg.y = fNextY - vCur.y;
            vLeg.z = 0.0f;
            float fLegSq = vLeg.x * vLeg.x + vLeg.y * vLeg.y + vLeg.z * vLeg.z;
            // How far along this leg we actually get is the whole leg if the budget covers it, and
            // the whole budget otherwise. The zero-length guard keeps sqrt off a degenerate leg.
            float fLeg = (fRemSq > fLegSq && fLegSq != 0.0f) ? __builtin_sqrtf(fLegSq) : fRemaining;

            vFrom = vCur;
            vFrom.z = pArea->ComputeHeight(vFrom, 1, 0, 0);
            vTo = vCur + normalize(vLeg) * fLeg;
            vTo.z = pArea->ComputeHeight(vTo, 1, 0, 0);

            Vector vTestHit = {};
            if (fLegSq > 0.0f && fLeg > 0.0f &&
                WalkUpdateLocationTestDistance(vFrom, vTo, &vTestHit, &oidBlocker) == 0)
                goto blocked;

            if (!(fRemSq <= fLegSq)) {
                // The budget covers the whole leg, so commit to the path point and carry on.
                pArea->IntersectingLineSegment(vCur, vPoint, pCrossed);
                float fWalked = magnitude(vLeg);
                vCur = vPoint;
                vCur.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vCur, 1, 0, 0);
                m_pPathfindInfo->m_nWayPointCursor = nCursor + 2;
                if (nLastLeg == nCursor) {
                    // That was the final leg: the walk is over, so face the way it went.
                    nResult = 1;
                    if (vLeg.x * vLeg.x + vLeg.y * vLeg.y + vLeg.z * vLeg.z != 0.0f)
                        *pvDelta = normalize(vLeg);
                    break;
                }
                fRemaining = fRemaining - fWalked;
                nCursor += 2;
            } else {
                // The budget runs out part-way along, so stop there, still in progress.
                nResult = 0;
                if (fLegSq != 0.0f) {
                    Vector vStep = vCur + normalize(vLeg) * fRemaining;
                    pArea->IntersectingLineSegment(vCur, vStep, pCrossed);
                    *pvDelta = normalize(vLeg);
                    vCur = vStep;
                    vCur.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vCur, 1, 0, 0);
                }
                break;
            }
        }

        // A two-point path (four floats) is a straight push: if walking it turned us more than
        // ~135 degrees away from the way we were facing, keep facing forward and back up instead.
        if (nPathCount == 4) {
            float* paPath = m_pPathfindInfo->m_paWayPoints;
            vPoint.x = paPath[2];
            vPoint.y = paPath[3];
            vPoint.z = 0.0f;
            vLeg.x = vPoint.x - m_position.x;
            vLeg.y = vPoint.y - m_position.y;
            vLeg.z = 0.0f;
            if (AnimationWalking(static_cast<unsigned short>(m_nAnimation)) != 0) {
                if (vLeg.x * vLeg.x + vLeg.y * vLeg.y + vLeg.z * vLeg.z <= 4.0f) {
                    if (vFacing.x * pvDelta->x + vFacing.y * pvDelta->y < -0.707f) {
                        pvDelta->x = -pvDelta->x;
                        pvDelta->y = -pvDelta->y;
                        pvDelta->z = 0.0f;
                        if (m_nAnimation != SERVER_ANIMATION_WALK_BACKWARD)
                            SetAnimation(SERVER_ANIMATION_WALK_BACKWARD);
                    }
                }
            }
        }

#if !KOTOR1  // K1: no final height snap.
        if (GetArea() != 0)
            vCur.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vCur, 1, 0, 0);
#endif
        *pvPosition = vCur;
        return nResult;
    }

blocked:
    // Something is standing in the way. Count the bump; too many and the whole path is abandoned.
    if (m_pPathfindInfo->m_nBumpCount++ >= 5) {
        m_pPathfindInfo->m_nWayPointFloats = 0;
        delete m_pPathfindInfo->m_paWayPoints;
        m_pPathfindInfo->m_paWayPoints = 0;
        SetAnimation(GetPauseReadyAnimation());
        m_pPathfindInfo->HandlePathfindError(
            1, const_cast<char*>("aborted walking, Maximum number of bumps happened"));
        pvPosition->x = m_position.x;
        pvPosition->y = m_position.y;
        pvPosition->z = m_position.z;
        return 1;
    }

    if (m_pPathfindInfo->m_oidGoalObject != OBJECT_INVALID && oidBlocker == m_pPathfindInfo->m_oidGoalObject) {
        // The blocker IS what we were walking to, so we are as close as we are going to get.
        m_pPathfindInfo->m_nWayPointFloats = 0;
        delete m_pPathfindInfo->m_paWayPoints;
        m_pPathfindInfo->m_paWayPoints = 0;
        SetAnimation(GetPauseReadyAnimation());
        *pvPosition = m_position;
        return 1;
    }

    CSWSCreature* pBlocker =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidBlocker);
    if (pBlocker != 0) {
        // Refuse to bump the same creature at the same spot twice: that is a deadlock, not a step.
        if (oidBlocker == m_pPathfindInfo->m_oidPathTarget) {
            float fbx = m_pPathfindInfo->m_vPathTarget.x - pBlocker->m_position.x;
            float fby = m_pPathfindInfo->m_vPathTarget.y - pBlocker->m_position.y;
            float fbz = m_pPathfindInfo->m_vPathTarget.z - pBlocker->m_position.z;
            if (!(fbx * fbx + fby * fby + fbz * fbz > 0.00001f))
                goto bumpedHere;
        }

        *pvPosition = m_position;

        CAvoidCreature avoid;
        avoid.m_nSide = m_pPathfindInfo->m_nAvoidSide;
        int nAvoid = avoid.PlotPathAroundCreature(this, pBlocker);
        if (nAvoid == CAvoidCreature::AVOIDCREATURE_FAILED) {
            m_pPathfindInfo->m_oidPathTarget = OBJECT_INVALID;
            m_pPathfindInfo->m_vPathTarget.x = 0.0f;
            m_pPathfindInfo->m_vPathTarget.y = 0.0f;
            m_pPathfindInfo->m_vPathTarget.z = 0.0f;
            SetAnimation(GetPauseReadyAnimation());
            m_pPathfindInfo->HandlePathfindError(
                1, const_cast<char*>("aborted walking, we are totaly blocked. can't get around this creature at all."));
            *pvPosition = m_position;
            if (m_objectId == g_pAppManager->m_pServerExoApp->GetPlayerCreatureId())
                g_pAppManager->m_pClientExoApp->GetInGameGui()->SetCombatMessage(STRREF_PATH_BLOCKED);
            return 1;
        }

        int nSide = m_pPathfindInfo->m_nAvoidSide;
        if (nSide != avoid.m_nSide) {
            m_pPathfindInfo->m_nAvoidSide = avoid.m_nSide;
            nSide = m_pPathfindInfo->m_nAvoidSide;
        }
        float fDot = dot(m_orientation, pBlocker->m_orientation);
        pBlocker->m_pPathfindInfo->m_nAvoidSide = fDot < 0.0f ? nSide : (nSide == 0);
        if (nAvoid != CAvoidCreature::AVOIDCREATURE_ENDINHEX)
            return 0;
        SetAnimation(GetPauseReadyAnimation());
        return 1;
    }

bumpedHere:
    SetAnimation(GetPauseReadyAnimation());
    m_pPathfindInfo->HandlePathfindError(
        1, const_cast<char*>("aborted walking, Bumped into this creature at this position already."));
    *pvPosition = m_position;
    return 1;
}

extern int   RenderPartyFollow;
extern int   CreaturePartyIndex;
extern int   FollowLeaderFullLogging;
extern float fSpringFactor;
extern float DriveWalkThreshold;

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader()
{
    unsigned long long oidClient = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(oidClient);
    // Slot 0 is the leader: it has nobody to follow, so the action is finished. A creature that is
    // not in the party at all (-1) is left running whatever it was already doing.
    if (nIndex == 0)
        return ACTION_COMPLETE;
    if (nIndex == -1)
        return ACTION_IN_PROGRESS;

    CSWParty*           pParty  = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWPartyMemberData* pMember = &pParty->m_members[nIndex];

    // On a pending re-seat, drop the member's safe position onto where it actually stands.
    if (pMember->m_bReseatSafePosition != 0) {
        pMember->m_safePosition = m_position;
        pMember->m_bReseatSafePosition = 0;
    }

    // For the follow-path debug overlay, the drawing call does nothing, but its argument side
    // effects (both GetSWParty() results and the CStr()/magnitude() values) must stay.
    if (RenderPartyFollow) {
        g_pAppManager->m_pClientExoApp->GetSWParty();
        g_pAppManager->m_pClientExoApp->GetSWParty();
        CreaturePartyIndex = nIndex;
        m_sTag.CStr();
        magnitude(pMember->m_followPoint - pMember->m_vPathPoint);
    }

    if (m_nWalkDeltaMs == 0)
        return ACTION_IN_PROGRESS;

    float fDx1 = pMember->m_followPoint.x - pMember->m_vPathPoint.x;
    float fDx2 = pMember->m_followPoint.x - pMember->m_safePosition.x;
    float fDy1 = pMember->m_followPoint.y - pMember->m_vPathPoint.y;
    float fDy2 = pMember->m_followPoint.y - pMember->m_safePosition.y;
    float fDz1 = pMember->m_followPoint.z - pMember->m_vPathPoint.z;
    float fDz2 = pMember->m_followPoint.z - pMember->m_safePosition.z;
    float fToPointSq = fDx1 * fDx1 + fDy1 * fDy1 + fDz1 * fDz1;
    float fToSafeSq  = fDx2 * fDx2 + fDy2 * fDy2 + fDz2 * fDz2;

    float fLeaderRate =
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0)->m_swAnimationSpeed;

    float fSpeed;
#if KOTOR1  // K1: each rate is read once, and the debug-logging reads follow every arm.
    if (fToPointSq >= 16.0f || fToSafeSq >= 100.0f) {
        fSpeed = GetRunRate();
        if (fLeaderRate > 0.1f) {
            float fLeader = g_pAppManager->m_pClientExoApp->GetSWParty()
                                ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f;
            fSpeed = (fSpeed > fLeader) ? fSpeed : fLeader;
        }
    } else if (fLeaderRate > 0.1f) {
        fSpeed = g_pAppManager->m_pClientExoApp->GetSWParty()
                     ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f;
    } else {
        float fEx = pMember->m_vPathPoint.x - pMember->m_followPoint.x;
        float fEy = pMember->m_vPathPoint.y - pMember->m_followPoint.y;
        float fEz = pMember->m_vPathPoint.z - pMember->m_followPoint.z;
        if (fEx * fEx + fEy * fEy + fEz * fEz >= 0.001f) {
            float fWalk = GetWalkRate() * 0.9f;
            fSpeed = (pMember->m_followDistance > fWalk) ? pMember->m_followDistance : fWalk;
        } else {
            float fRun = GetRunRate();
            fSpeed = !(fToPointSq <= 4.0f) ? fRun : fRun * DriveWalkThreshold;
        }
    }
    if (FollowLeaderFullLogging) {
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        GetClientCreature();
    }
#else
    if (fToPointSq >= 16.0f || fToSafeSq >= 100.0f) {
        // Well behind (4 units off the path point, or 10 off the safe position), so run.
        fSpeed = GetRunRate();
        if (fLeaderRate > 0.1f) {
            if (fSpeed > g_pAppManager->m_pClientExoApp->GetSWParty()
                             ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f)
                fSpeed = GetRunRate();
            else
                goto leaderPaced;
        }
    } else if (fLeaderRate > 0.1f) {
    leaderPaced:
        fSpeed = g_pAppManager->m_pClientExoApp->GetSWParty()
                     ->GetCharacter(0)->m_swAnimationSpeed * 1000.0f;
    } else {
        float fEx = pMember->m_vPathPoint.x - pMember->m_followPoint.x;
        float fEy = pMember->m_vPathPoint.y - pMember->m_followPoint.y;
        float fEz = pMember->m_vPathPoint.z - pMember->m_followPoint.z;
        if (fEx * fEx + fEy * fEy + fEz * fEz >= 0.001f) {
            if (pMember->m_followDistance > GetWalkRate() * 0.9f)
                fSpeed = pMember->m_followDistance;
            else
                fSpeed = GetWalkRate() * 0.9f;
        } else {
            float fRun = GetRunRate();
            fSpeed = !(fToPointSq <= 4.0f) ? fRun : fRun * DriveWalkThreshold;
        }
        if (FollowLeaderFullLogging) {
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
            GetClientCreature();
        }
    }
#endif
    // A second, consecutive logging block on the same flag.
    if (FollowLeaderFullLogging) {
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        GetClientCreature();
    }

    float fScale    = !(fToPointSq <= 1.0f) ? (!(fToPointSq <= 225.0f) ? 1.5f : 1.2f) : 0.9f;
    float fTimeLeft = fSpeed * fScale * static_cast<float>(m_nWalkDeltaMs) / 1000000.0f;

    Vector vStart = m_position;

    // In marching order, member 1 trails the leader, member 2 trails member 1, and the leader
    // trails nobody.
    int nOther = (nIndex == 2) ? 1 : ((nIndex == 1) ? 2 : 0);
    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
        g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nOther].id);

    // Start the walk with no remembered blocker, and point the area's pathfind scratch at ours.
    m_pPathfindInfo->m_oidIgnoredBlocker = OBJECT_INVALID;
    static_cast<CSWSArea*>(GetArea())->m_pActivePathfindInfo = m_pPathfindInfo;
    pMember->m_fPathSpanRatio = 1.0f;

    int nResult = ACTION_IN_PROGRESS;
    while (fTimeLeft >= 0.0f) {
        nResult = ACTION_IN_PROGRESS;
        ++pMember->m_nStateSteps;
        switch (pMember->m_nFollowState) {
        case CSWPartyMemberData::FOLLOW_STATE_NEW:
            // For a fresh member, seat the safe position on the creature and start it looking for a
            // path.
            pMember->m_safePosition = m_position;
            pMember->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_WAITING_FOR_PATH;
            break;
        case CSWPartyMemberData::FOLLOW_STATE_FIND_PATH:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_FindPath(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        case CSWPartyMemberData::FOLLOW_STATE_OFFSET:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_Offset(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        case CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_Moving_To_Path(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        case CSWPartyMemberData::FOLLOW_STATE_FOLLOWING_PATH:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_Following_Path(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        case CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_Moving_To_Offset(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        case CSWPartyMemberData::FOLLOW_STATE_WAITING_FOR_PATH:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_WaitingForPath(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            break;
        // The five states below settle the member rather than move it, so they clear the
        // transition counter instead of testing it, hence the shared `goto stepReset`.
        case CSWPartyMemberData::FOLLOW_STATE_OFFSET_WAIT_DEST:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_OffsetWaitDest(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            goto stepReset;
        case CSWPartyMemberData::FOLLOW_STATE_PATH_WAIT_DEST:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_PathWaitDest(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            goto stepReset;
        case CSWPartyMemberData::FOLLOW_STATE_TO_OFFSET_WAIT_DEST:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_ToOffsetWaitDest(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            goto stepReset;
        case CSWPartyMemberData::FOLLOW_STATE_TO_PATH_WAIT_DEST:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_ToPathWaitDest(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            goto stepReset;
        case CSWPartyMemberData::FOLLOW_STATE_TELEPORT_FORWARD:
            nResult = WalkUpdateLocation_QuickWalk_FollowLeader_TeleportForward(
                pMember, &pParty->m_followData, fTimeLeft, nIndex);
            goto stepReset;
        }
        // 51 state transitions in one tick means the machine is thrashing: throw the member back
        // to the top and start over.
        if (pMember->m_nStateSteps < 51)
            goto stepChecked;
        pMember->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;
    stepReset:
        pMember->m_nStateSteps = 0;
    stepChecked:
        if (nResult != ACTION_IN_PROGRESS)
            break;
    }

    // Commit whatever position the state machine settled on.
    Vector vDelta = pMember->m_safePosition - m_position;
    float fMoveSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (fMoveSq >= 0.001f) {
        if (pArea->TestDirectLine(pMember->m_safePosition, m_position,
                                  static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                  m_pPathfindInfo->m_fHeight, 0, NULL) == 1) {
            Vector vSprung;
            vSprung.x = m_position.x + vDelta.x * fSpringFactor;
            vSprung.y = m_position.y + vDelta.y * fSpringFactor;
            vSprung.z = m_position.z + 0.0f;
            vSprung.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vSprung, 1, NULL, NULL);
            SetPosition(vSprung, 1, 1, 0);
        } else if (static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(pMember->m_safePosition,
                                                                 m_pPathfindInfo) == 1) {
            SetPosition(pMember->m_safePosition, 1, 1, 0);
        } else {
            ++pMember->m_nStateSteps;
        }
    } else if (pArea->TestDirectLine(pMember->m_safePosition, m_position,
                                     static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                     m_pPathfindInfo->m_fHeight, 0, NULL) == 1) {
        SetPosition(pMember->m_safePosition, 1, 1, 0);
    }

    g_pAppManager->m_pServerExoApp->UpdateSubareasOnMoveTo(m_objectId, vStart, m_position);

    Vector vMoved;
    vMoved.x = m_position.x - vStart.x;
    vMoved.y = m_position.y - vStart.y;
    vMoved.z = 0.0f;
    float fMoved = magnitude(vMoved);

    Vector vToLeader = normalize(pParty->GetCharacter(0)->m_position - m_position);
    if (vMoved.x == 0.0f && vMoved.y == 0.0f) {
        // Having stood still, keep facing where we are, but let the head track the leader as long
        // as the angle to it is still inside the head-turn cone.
        float fDot = vToLeader.x * m_orientation.x + vToLeader.y * m_orientation.y +
                     vToLeader.z * m_orientation.z;
        float fCone = cosf(GetClientCreature()->GetHeadTurnHorizontal() * 0.017453292f);
        if ((fDot >= 0.0f && fDot < fCone) || (fDot <= 0.0f && fDot > -fCone))
            pMember->m_followOrientation = vToLeader;
        GetClientCreature()->LookAt(pParty->GetCharacter(0)->m_objectId, 8.0f);
    } else {
        // Having moved, face the direction of travel and drop any head-turn target.
        GetClientCreature()->LookAt(OBJECT_INVALID, 8.0f);
        SetOrientation(normalize(vMoved));
        pMember->m_nStateSteps = 0;
        pMember->m_followOrientation = m_orientation;
    }

    float fRate = fMoved * 1000000.0f / static_cast<float>(m_nWalkDeltaMs);
    float fSmoothed = (pMember->m_followDistance + fRate) * 0.5f;
    fRate = !(static_cast<double>(fRate) <= 0.0001) ? fSmoothed : fRate;
    fRate = fRate < 10.0f ? 0.0f : fRate;
    pMember->m_followDistance = fSpeed < fRate ? fSpeed : fRate;
    return nResult;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_FindPath(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int)
{
    K2_UNUSED(f);
    g_pAppManager->m_pClientExoApp->GetSWParty()->RecaulateFollowPoint();

    // The follow point and its sample index are read out before the GetPathData call below.
    Vector vFollow = pMemberData->m_followPoint;
    int nSample = pMemberData->m_bFollowPointValid;
    CSWPartyFollowData* pPathData = pFollowData->GetPathData(nSample);

    Vector vOffset;
    Vector vTarget;
    {
    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pPathData->m_fLeaderYaw));
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        Vector p = pMemberData->m_formationOffset;
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        vOffset.x = p.x * m00 + p.y * m01 + p.z * m02;
        vOffset.y = p.x * m10 + p.y * m11 + p.z * m12;
        vOffset.z = p.x * m20 + p.y * m21 + p.z * m22;
    } else {
        vOffset = pMemberData->m_formationOffset;
    }
    vTarget = vOffset + vFollow;
    }
    vTarget.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vTarget, 1, 0, 0);

    // Re-seat the member's current sample from the recomputed follow point; the inverse of
    // WaitingForPath, which seeds the follow point from the current sample.
    pMemberData->m_vPathPoint = vFollow;
    pMemberData->m_nPathIndex = nSample;

    // Aim the pathfinder at a point: clearing the target triple is what makes it stop steering
    // toward whatever object it was last chasing.
    m_pPathfindInfo->m_oidPathTarget = OBJECT_INVALID;
    m_pPathfindInfo->m_vPathTarget.x = 0.0f;
    m_pPathfindInfo->m_vPathTarget.y = 0.0f;
    m_pPathfindInfo->m_vPathTarget.z = 0.0f;

    int nOne = 1;
    float fHalf = 0.5f;
    unsigned long long oidInvalid = OBJECT_INVALID;
    unsigned long long oidArea = static_cast<CSWSArea*>(GetArea())->m_objectId;

    int bWasCommandable = m_bCommandable;
    m_bCommandable = 1;
    CSWSObjectActionNode* pNode = GetAction(AIACTION_PARTY_FOLLOW_LEADER);
    unsigned short nGroupId = ACTION_GROUP_NEW;
    if (pNode != 0)
        nGroupId = pNode->m_nGroupId;

    if (static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vTarget, m_pPathfindInfo) != 0) {
        AddActionToFront(AIACTION_CHECK_MOVE_TO_FOLLOW_RADIUS, nGroupId,
                         ACTION_PARAM_FLOAT, &vTarget.x, ACTION_PARAM_FLOAT, &vTarget.y,
                         ACTION_PARAM_FLOAT, &vTarget.z, ACTION_PARAM_OBJECT, &oidArea,
                         ACTION_PARAM_OBJECT, &oidInvalid, ACTION_PARAM_INT, &nOne,
                         ACTION_PARAM_FLOAT, &fHalf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(ACTION_GROUP_PREVIOUS, vTarget, m_cachedAreaId, OBJECT_INVALID, 1, 0.0f,
                                    30.0f,
                                    0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_OFFSET_WAIT_DEST;
    } else {
        static_cast<CSWSArea*>(GetArea())->ComputeSafeLocation(vFollow, 3.0f, m_pPathfindInfo, 0, &vFollow);
        AddActionToFront(AIACTION_CHECK_MOVE_TO_FOLLOW_RADIUS, nGroupId,
                         ACTION_PARAM_FLOAT, &vFollow.x, ACTION_PARAM_FLOAT, &vFollow.y,
                         ACTION_PARAM_FLOAT, &vFollow.z, ACTION_PARAM_OBJECT, &oidArea,
                         ACTION_PARAM_OBJECT, &oidInvalid, ACTION_PARAM_INT, &nOne,
                         ACTION_PARAM_FLOAT, &fHalf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Vector vZero;
        AddMoveToPointActionToFront(ACTION_GROUP_PREVIOUS, vFollow, m_cachedAreaId, OBJECT_INVALID, 1, 0.0f,
                                    30.0f,
                                    0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_PATH_WAIT_DEST;
    }

    m_nMoveState = -1;
    m_bCommandable = bWasCommandable;

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    unsigned long long oidLeader =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pParty->m_members[0].id);
    CSWSCreature* pLeader = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidLeader);
    if (pLeader != 0)
        pMemberData->m_vLeaderPathOrigin = pLeader->m_position;
    pMemberData->m_bReseatSafePosition = 1;
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_Offset(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& fTimeLeft,
    int nDirection)
{
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    hitInfo.m_hitSubType  = 0;

    Vector vAdvanced;
    int nSample;
    Vector vGoal;

    pFollowData->AdvancePointOnPath(fTimeLeft, nDirection, pMemberData->m_vPathPoint,
                                    pMemberData->m_nPathIndex, pMemberData->m_followPoint,
                                    pMemberData->m_bFollowPointValid, vAdvanced, nSample);

    static_cast<CSWSArea*>(GetArea())->TestDirectLine(pMemberData->m_vPathPoint, vAdvanced,
                                           static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                           m_pPathfindInfo->m_fHeight, 0, &hitInfo);

    int nPrev = nSample < 1 ? nSample + 99 : nSample - 1;

    float fAngleCur  = pFollowData->GetPathData(nSample)->m_fLeaderYaw;
    float fAnglePrev = pFollowData->GetPathData(nPrev)->m_fLeaderYaw;
    Vector vOffset = pMemberData->m_formationOffset;
    Vector vPosCur  = pFollowData->GetPathData(nSample)->m_pos;
    Vector vPosPrev = pFollowData->GetPathData(nPrev)->m_pos;
    Vector vCurrent = vAdvanced;

    Vector vRotCur;
    Vector vTargetCur;
    {
    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fAngleCur));
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        vRotCur.x = vOffset.x * m00 + vOffset.y * m01 + vOffset.z * m02;
        vRotCur.y = vOffset.x * m10 + vOffset.y * m11 + vOffset.z * m12;
        vRotCur.z = vOffset.x * m20 + vOffset.y * m21 + vOffset.z * m22;
    } else {
        vRotCur = vOffset;
    }
    vTargetCur = vRotCur + vPosCur;
    }

    Vector vRotPrev;
    Vector vTargetPrev;
    {
    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fAnglePrev));
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        vRotPrev.x = vOffset.x * m00 + vOffset.y * m01 + vOffset.z * m02;
        vRotPrev.y = vOffset.x * m10 + vOffset.y * m11 + vOffset.z * m12;
        vRotPrev.z = vOffset.x * m20 + vOffset.y * m21 + vOffset.z * m22;
    } else {
        vRotPrev = vOffset;
    }
    vTargetPrev = vRotPrev + vPosPrev;
    }

    float fAlong = magnitude(vCurrent - vPosPrev) / magnitude(vPosCur - vPosPrev);
    vGoal = vTargetPrev + (vTargetCur - vTargetPrev) * fAlong;

    if (GetArea() != 0)
        vGoal.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(vGoal, 1, 0, 0);

    // Squared-length ratio of the offset track to the path track over this segment, written out
    // component-wise.
    Vector vPathSpan   = vPosPrev - vPosCur;
    Vector vTargetSpan = vTargetPrev - vTargetCur;
    pMemberData->m_fPathSpanRatio =
        (vTargetSpan.x * vTargetSpan.x + vTargetSpan.y * vTargetSpan.y +
         vTargetSpan.z * vTargetSpan.z) /
        (vPathSpan.x * vPathSpan.x + vPathSpan.y * vPathSpan.y + vPathSpan.z * vPathSpan.z);

    if (static_cast<CSWSArea*>(GetArea())->TestDirectLine(pMemberData->m_safePosition, vGoal,
                                               static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                               m_pPathfindInfo->m_fHeight, 0, &hitInfo) == 1) {
        int nLine = static_cast<CSWSArea*>(GetArea())->TestDirectLine(vGoal, vAdvanced,
                                                           static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                                           m_pPathfindInfo->m_fHeight, 0, &hitInfo);
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfo.m_hitObjectId);
        if (nLine == 1 ||
            (nLine == -3 &&
             g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(oidClient) != 0)) {
            pMemberData->m_safePosition = vGoal;
            pMemberData->m_nStateSteps = 0;
            pMemberData->m_vPathPoint = vAdvanced;
            pMemberData->m_nPathIndex = nSample;
            return ACTION_COMPLETE;
        }
    }

    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH;
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_Moving_To_Path(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& fTimeLeft, int)
{
    K2_UNUSED(pFollowData);
    float fStep = fTimeLeft * 0.7f;
    Vector vTarget = pMemberData->m_vPathPoint;
    Vector vDelta = vTarget - pMemberData->m_safePosition;
    Vector vDir = normalize(vDelta);
    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;

    Vector vNewPos = {};
    int nState;
    int nResult;
    if (!(fDistSq <= fStep * fStep)) {
        // Too far to reach this tick, so step along the direction and spend the whole budget.
        nState = pMemberData->m_nFollowState;
        vNewPos = pMemberData->m_safePosition + vDir * fStep;
        nResult = ACTION_COMPLETE;
        fTimeLeft = 0.0f;
    } else {
        // We reach it, so snap on target and refund the time we did not need.
        vNewPos = vTarget;
        nState = CSWPartyMemberData::FOLLOW_STATE_FOLLOWING_PATH;
        nResult = ACTION_IN_PROGRESS;
        fTimeLeft = fTimeLeft - __builtin_sqrtf(fDistSq) / 0.7f;
    }

    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    hitInfo.m_hitSubType = 0;
    float fRadius = m_pPathfindInfo->m_fPersonalSpace - 0.1f;
    int nLine = static_cast<CSWSArea*>(GetArea())->TestDirectLine(pMemberData->m_safePosition, vNewPos,
                                                       fRadius, m_pPathfindInfo->m_fHeight,
                                                       0, &hitInfo);
    if (nLine == -3) {
        // Blocked by a non-party object, just switch to TELEPORT_FORWARD, no event (see the header).
        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                    hitInfo.m_hitObjectId)) == 0) {
            pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_TELEPORT_FORWARD;
            return ACTION_COMPLETE;
        }
    } else if (nLine == 1) {
        pMemberData->m_nStuckTimeMs = 0;
        pMemberData->m_safePosition = vNewPos;
        pMemberData->m_nFollowState = nState;
        return nResult;
    }

    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, hitInfo.m_hitObjectId, m_objectId,
        CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_Following_Path(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& fTimeLeft,
    int nDirection)
{
    {
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    hitInfo.m_hitSubType  = 0;

    Vector vAdvanced = {};
    int nSample;
    float fRemaining = pFollowData->AdvancePointOnPath(
        fTimeLeft, nDirection, pMemberData->m_vPathPoint, pMemberData->m_nPathIndex,
        pMemberData->m_followPoint, pMemberData->m_bFollowPointValid, vAdvanced, nSample);
    pMemberData->m_nStuckTimeMs += m_nWalkDeltaMs;

    int nLine = static_cast<CSWSArea*>(GetArea())->TestDirectLine(pMemberData->m_vPathPoint, vAdvanced,
                                                       m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                                                       m_pPathfindInfo->m_fHeight, 0, &hitInfo);
    if (nLine == -3) {
        // Blocked by a creature. A fellow party member is not an obstacle worth re-pathing for.
        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfo.m_hitObjectId)) != 0)
            goto done;

        int nPrevSample = nSample;
        if (pMemberData->m_nPathIndex != nSample) {
            nSample = pMemberData->m_nPathIndex;
            vAdvanced = pFollowData->GetPathData(nSample)->m_pos;
            if (equal(vAdvanced, m_position) != 0 && nSample < nPrevSample) {
                nSample = nSample > 98 ? 0 : nSample + 1;
                vAdvanced = pFollowData->GetPathData(nSample)->m_pos;
            }
        }

        nLine = static_cast<CSWSArea*>(GetArea())->TestDirectLine(pMemberData->m_vPathPoint, vAdvanced,
                                                       m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                                                       m_pPathfindInfo->m_fHeight, 0, &hitInfo);
        if (nLine == -3 &&
            g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(hitInfo.m_hitObjectId)) != 0)
            goto done;
    }
    if (nLine == -3) {
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_TELEPORT_FORWARD;
        goto done;
    }
    if (nLine != 1) {
        // Not blocked by a creature and not clear either, the path itself is gone. Stop the member
        // and signal the blocker so its own AI can react.
        fTimeLeft = 0.0f;
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, hitInfo.m_hitObjectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        goto done;
    }

    pMemberData->m_vPathPoint = vAdvanced;
    pMemberData->m_nPathIndex = nSample;
    pMemberData->m_safePosition = vAdvanced;
    pMemberData->m_nStateSteps = 0;
    fTimeLeft = fRemaining;

    Vector vRot;
    Vector vTarget;
    {
    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pFollowData->GetPathData(nSample)->m_fLeaderYaw));
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        Vector p = pMemberData->m_formationOffset;
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        vRot.x = p.x * m00 + p.y * m01 + p.z * m02;
        vRot.y = p.x * m10 + p.y * m11 + p.z * m12;
        vRot.z = p.x * m20 + p.y * m21 + p.z * m22;
    } else {
        vRot = pMemberData->m_formationOffset;
    }
    vTarget = vRot + vAdvanced;
    }

    CWalkHitInfo hitInfo2 = {};
    hitInfo2.m_hitObjectId = OBJECT_INVALID;
    hitInfo2.m_hitSubType  = 0;
    if (static_cast<CSWSArea*>(GetArea())->TestDirectLine(vAdvanced, vTarget,
                                               static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                               m_pPathfindInfo->m_fHeight, 0, &hitInfo2) != 1)
        goto done;

    // Look ahead, walking forward along the recorded path from the committed formation target,
    // accumulating up to 5.0 units and requiring every leg to be clear.
    int nIdx = pMemberData->m_nPathIndex;
    Vector vCursor = vTarget;
    float fWalked = 0.0f;
    int nAllClear = 1;
    int nStep;
    int bAtFollowPoint;
    do {
        float fAngle = pFollowData->GetPathData(nIdx)->m_fLeaderYaw;
        // Braced zero; vNext's address escapes into equal()/TestDirectLine below.
        Vector vNext = {};
        if (nIdx == pMemberData->m_bFollowPointValid) {
            // On reaching the member's own follow point, the last leg is a partial one, clipped to
            // whatever is left of the 5.0-unit budget rather than running to the sample.
            Vector vRotA;
            {
            Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fAngle));
            float w = q.w;
            if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
                Vector p = pMemberData->m_formationOffset;
                float x = q.x, y = q.y, z = q.z;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, wx = x * w;
                float yz = y * z, wy = y * w;
                float wz = z * w;
                float m00 = 1.0f - ((yy + zz) + (yy + zz));
                float m01 = (xy - wz) + (xy - wz);
                float m02 = (xz + wy) + (xz + wy);
                float m10 = (xy + wz) + (xy + wz);
                float m11 = 1.0f - ((xx + zz) + (xx + zz));
                float m12 = (yz - wx) + (yz - wx);
                float m20 = (xz - wy) + (xz - wy);
                float m21 = (yz + wx) + (yz + wx);
                float m22 = 1.0f - ((xx + yy) + (xx + yy));
                vRotA.x = p.x * m00 + p.y * m01 + p.z * m02;
                vRotA.y = p.x * m10 + p.y * m11 + p.z * m12;
                vRotA.z = p.x * m20 + p.y * m21 + p.z * m22;
            } else {
                vRotA = pMemberData->m_formationOffset;
            }
            vNext = vRotA + pMemberData->m_followPoint;
            }
            vNext = vCursor + normalize(vNext - vCursor) * (5.0f - fWalked);
            bAtFollowPoint = 1;
        } else {
            CSWPartyFollowData* pPathData = pFollowData->GetPathData(nIdx);
            Vector vRotB;
            {
            Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(fAngle));
            float w = q.w;
            if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
                Vector p = pMemberData->m_formationOffset;
                float x = q.x, y = q.y, z = q.z;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, wx = x * w;
                float yz = y * z, wy = y * w;
                float wz = z * w;
                float m00 = 1.0f - ((yy + zz) + (yy + zz));
                float m01 = (xy - wz) + (xy - wz);
                float m02 = (xz + wy) + (xz + wy);
                float m10 = (xy + wz) + (xy + wz);
                float m11 = 1.0f - ((xx + zz) + (xx + zz));
                float m12 = (yz - wx) + (yz - wx);
                float m20 = (xz - wy) + (xz - wy);
                float m21 = (yz + wx) + (yz + wx);
                float m22 = 1.0f - ((xx + yy) + (xx + yy));
                vRotB.x = p.x * m00 + p.y * m01 + p.z * m02;
                vRotB.y = p.x * m10 + p.y * m11 + p.z * m12;
                vRotB.z = p.x * m20 + p.y * m21 + p.z * m22;
            } else {
                vRotB = pMemberData->m_formationOffset;
            }
            vNext = vRotB + pPathData->m_pos;
            }
            bAtFollowPoint = 0;
        }

        if (equal(vCursor, vNext) != 0)
            goto done;

        nStep = static_cast<CSWSArea*>(GetArea())->TestDirectLine(vCursor, vNext,
                                                       static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                                       m_pPathfindInfo->m_fHeight, 0, 0);
        nAllClear = nStep == 1 ? nAllClear : 0;
        float fLeg = magnitude(vNext - vCursor);
        vCursor = vNext;
        nIdx = nIdx > 98 ? 0 : nIdx + 1;
        fWalked += fLeg;
    } while (fWalked <= 5.0f && nStep == 1 && bAtFollowPoint == 0);

    if (nAllClear == 1 && pMemberData->m_nStuckTimeMs >= 3000) {
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET;
        fTimeLeft = 0.0f;
    }
    }
done:
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_Moving_To_Offset(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& fTimeLeft, int a)
{
    K2_UNUSED(a);
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    hitInfo.m_hitSubType = 0;
    float fStep = fTimeLeft * 0.7f;
    CSWPartyFollowData* pPathData = pFollowData->GetPathData(pMemberData->m_nPathIndex);

    Vector vOffset;
    {
    Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pPathData->m_fLeaderYaw));
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        Vector p = pMemberData->m_formationOffset;
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        vOffset.x = p.x * m00 + p.y * m01 + p.z * m02;
        vOffset.y = p.x * m10 + p.y * m11 + p.z * m12;
        vOffset.z = p.x * m20 + p.y * m21 + p.z * m22;
    } else {
        vOffset = pMemberData->m_formationOffset;
    }
    }
    Vector vDest = vOffset + pMemberData->m_vPathPoint;
    Vector vDelta = vDest - pMemberData->m_safePosition;
    normalize(vDelta);
    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;

    Vector vNewPos = {};
    int nState;
    int nResult;
    if (!(fDistSq <= fStep * fStep)) {
        nState = pMemberData->m_nFollowState;
        vNewPos = pMemberData->m_safePosition + normalize(vDelta) * fStep;
        nResult = ACTION_COMPLETE;
        fTimeLeft = 0.0f;
    } else {
        vNewPos = vDest;
        nState = CSWPartyMemberData::FOLLOW_STATE_OFFSET;
        nResult = ACTION_IN_PROGRESS;
        fTimeLeft = fTimeLeft - __builtin_sqrtf(fDistSq) / 0.7f;
    }

    if (static_cast<CSWSArea*>(GetArea())
            ->TestDirectLine(pMemberData->m_safePosition, vNewPos,
                             m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                             m_pPathfindInfo->m_fHeight, 0, &hitInfo) == 1) {
        pMemberData->m_safePosition = vNewPos;
        pMemberData->m_nFollowState = nState;
        return nResult;
    }

    int nLine = static_cast<CSWSArea*>(GetArea())
            ->TestDirectLine(pMemberData->m_safePosition, pMemberData->m_vPathPoint,
                             m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                             m_pPathfindInfo->m_fHeight, 0, &hitInfo) ;
    if (nLine == -3) {
        // Blocked by another party member, hold the current state and just report ACTION_COMPLETE.
        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                    hitInfo.m_hitObjectId)) != 0) {
            return ACTION_COMPLETE;
        }
    } else if (nLine == 1) {
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH;
        return ACTION_IN_PROGRESS;
    }
    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_TELEPORT_FORWARD;
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_WaitingForPath(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int a)
{
    K2_UNUSED(f); K2_UNUSED(a);
    if (pFollowData->m_nSampleCount >= 5) {
        Vector vPoint = {};
        int nSample;
        if (pFollowData->GetPointBackFromStart(pMemberData->m_followPathDistance, vPoint, nSample, 0)
            == 0)
            return ACTION_COMPLETE;
        pMemberData->m_vPathPoint = vPoint;
        pMemberData->m_nPathIndex = nSample;
        pMemberData->m_followPoint = vPoint;
        pMemberData->m_bFollowPointValid = nSample;

        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pMemberData->m_bWaitForLeaderDistance != 0) {
            Vector vDelta = pLeader->m_position - m_position;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z < 49.0f)
                return ACTION_COMPLETE;
        }
        pMemberData->m_bWaitForLeaderDistance = 0;

        CWalkHitInfo hitInfo = {};
        hitInfo.m_hitObjectId = OBJECT_INVALID;
        hitInfo.m_hitSubType = 0;
        CSWPartyFollowData* pPathData = pFollowData->GetPathData(pMemberData->m_nPathIndex);
        Vector vOffset;
        {
        Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pPathData->m_fLeaderYaw));
        float w = q.w;
        if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
            Vector p = pMemberData->m_formationOffset;
            float x = q.x, y = q.y, z = q.z;
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, xz = x * z, wx = x * w;
            float yz = y * z, wy = y * w;
            float wz = z * w;
            float m00 = 1.0f - ((yy + zz) + (yy + zz));
            float m01 = (xy - wz) + (xy - wz);
            float m02 = (xz + wy) + (xz + wy);
            float m10 = (xy + wz) + (xy + wz);
            float m11 = 1.0f - ((xx + zz) + (xx + zz));
            float m12 = (yz - wx) + (yz - wx);
            float m20 = (xz - wy) + (xz - wy);
            float m21 = (yz + wx) + (yz + wx);
            float m22 = 1.0f - ((xx + yy) + (xx + yy));
            vOffset.x = p.x * m00 + p.y * m01 + p.z * m02;
            vOffset.y = p.x * m10 + p.y * m11 + p.z * m12;
            vOffset.z = p.x * m20 + p.y * m21 + p.z * m22;
        } else {
            vOffset = pMemberData->m_formationOffset;
        }
        }
        Vector vDest = vOffset + pMemberData->m_vPathPoint;
        if (static_cast<CSWSArea*>(GetArea())
                ->TestDirectLine(pMemberData->m_safePosition, vDest,
                                 m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                                 m_pPathfindInfo->m_fHeight, 0, &hitInfo) == 1) {
            pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET;
            return ACTION_COMPLETE;
        }
        if (static_cast<CSWSArea*>(GetArea())
                ->TestDirectLine(pMemberData->m_safePosition, pMemberData->m_vPathPoint,
                                 m_pPathfindInfo->m_fPersonalSpace - 0.1f,
                                 m_pPathfindInfo->m_fHeight, 0, &hitInfo) == 1) {
            pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH;
            return ACTION_COMPLETE;
        }
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_FIND_PATH;
        return ACTION_IN_PROGRESS;
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_OffsetWaitDest(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int a)
{
    K2_UNUSED(f); K2_UNUSED(a);
    int nPathIndex = pMemberData->m_nPathIndex;
    if (nPathIndex == pFollowData->m_nWriteIndex)
        return ACTION_COMPLETE;
    int i = nPathIndex;
    while (i != pMemberData->m_bFollowPointValid) {
        i = (i > 98) ? 0 : i + 1;
        if (i == pFollowData->m_nWriteIndex)
            return ACTION_COMPLETE;
    }
    if (pMemberData->m_bFollowPointValid == nPathIndex) {
        CSWPartyFollowData* pPathData = pFollowData->GetPathData(nPathIndex);
        float fDx1 = pPathData->m_pos.x - pMemberData->m_vPathPoint.x;
        float fDx2 = pPathData->m_pos.x - pMemberData->m_followPoint.x;
        float fDy1 = pPathData->m_pos.y - pMemberData->m_vPathPoint.y;
        float fDy2 = pPathData->m_pos.y - pMemberData->m_followPoint.y;
        float fDz1 = pPathData->m_pos.z - pMemberData->m_vPathPoint.z;
        float fDz2 = pPathData->m_pos.z - pMemberData->m_followPoint.z;
        if (!(fDx1 * fDx1 + fDy1 * fDy1 + fDz1 * fDz1 >
              fDx2 * fDx2 + fDy2 * fDy2 + fDz2 * fDz2))
            return ACTION_COMPLETE;
    }
    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_OFFSET;
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_PathWaitDest(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int a)
{
    K2_UNUSED(f); K2_UNUSED(a);
    int nPathIndex = pMemberData->m_nPathIndex;
    if (nPathIndex == pFollowData->m_nWriteIndex)
        return ACTION_COMPLETE;
    int i = nPathIndex;
    while (i != pMemberData->m_bFollowPointValid) {
        i = (i > 98) ? 0 : i + 1;
        if (i == pFollowData->m_nWriteIndex)
            return ACTION_COMPLETE;
    }
    if (pMemberData->m_bFollowPointValid == nPathIndex) {
        CSWPartyFollowData* pPathData = pFollowData->GetPathData(nPathIndex);
        float fDx1 = pPathData->m_pos.x - pMemberData->m_vPathPoint.x;
        float fDx2 = pPathData->m_pos.x - pMemberData->m_followPoint.x;
        float fDy1 = pPathData->m_pos.y - pMemberData->m_vPathPoint.y;
        float fDy2 = pPathData->m_pos.y - pMemberData->m_followPoint.y;
        float fDz1 = pPathData->m_pos.z - pMemberData->m_vPathPoint.z;
        float fDz2 = pPathData->m_pos.z - pMemberData->m_followPoint.z;
        if (!(fDx1 * fDx1 + fDy1 * fDy1 + fDz1 * fDz1 >
              fDx2 * fDx2 + fDy2 * fDy2 + fDz2 * fDz2))
            return ACTION_COMPLETE;
    }
    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_FOLLOWING_PATH;
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_ToOffsetWaitDest(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int a)
{
    K2_UNUSED(f); K2_UNUSED(a);
    int nPathIndex = pMemberData->m_nPathIndex;
    if (nPathIndex == pFollowData->m_nWriteIndex)
        return ACTION_COMPLETE;
    int i = nPathIndex;
    while (i != pMemberData->m_bFollowPointValid) {
        i = (i > 98) ? 0 : i + 1;
        if (i == pFollowData->m_nWriteIndex)
            return ACTION_COMPLETE;
    }
    if (pMemberData->m_bFollowPointValid == nPathIndex) {
        CSWPartyFollowData* pPathData = pFollowData->GetPathData(nPathIndex);
        float fDx1 = pPathData->m_pos.x - pMemberData->m_vPathPoint.x;
        float fDx2 = pPathData->m_pos.x - pMemberData->m_followPoint.x;
        float fDy1 = pPathData->m_pos.y - pMemberData->m_vPathPoint.y;
        float fDy2 = pPathData->m_pos.y - pMemberData->m_followPoint.y;
        float fDz1 = pPathData->m_pos.z - pMemberData->m_vPathPoint.z;
        float fDz2 = pPathData->m_pos.z - pMemberData->m_followPoint.z;
        if (!(fDx1 * fDx1 + fDy1 * fDy1 + fDz1 * fDz1 >
              fDx2 * fDx2 + fDy2 * fDy2 + fDz2 * fDz2))
            return ACTION_COMPLETE;
    }
    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_OFFSET;
    return ACTION_IN_PROGRESS;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_ToPathWaitDest(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& f, int a)
{
    K2_UNUSED(f); K2_UNUSED(a);
    int nPathIndex = pMemberData->m_nPathIndex;
    if (nPathIndex == pFollowData->m_nWriteIndex)
        return ACTION_COMPLETE;
    int i = nPathIndex;
    while (i != pMemberData->m_bFollowPointValid) {
        i = (i > 98) ? 0 : i + 1;
        if (i == pFollowData->m_nWriteIndex)
            return ACTION_COMPLETE;
    }
    if (pMemberData->m_bFollowPointValid == nPathIndex) {
        CSWPartyFollowData* pPathData = pFollowData->GetPathData(nPathIndex);
        float fDx1 = pPathData->m_pos.x - pMemberData->m_vPathPoint.x;
        float fDx2 = pPathData->m_pos.x - pMemberData->m_followPoint.x;
        float fDy1 = pPathData->m_pos.y - pMemberData->m_vPathPoint.y;
        float fDy2 = pPathData->m_pos.y - pMemberData->m_followPoint.y;
        float fDz1 = pPathData->m_pos.z - pMemberData->m_vPathPoint.z;
        float fDz2 = pPathData->m_pos.z - pMemberData->m_followPoint.z;
        if (!(fDx1 * fDx1 + fDy1 * fDy1 + fDz1 * fDz1 >
              fDx2 * fDx2 + fDy2 * fDy2 + fDz2 * fDz2))
            return ACTION_COMPLETE;
    }
    pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH;
    return ACTION_IN_PROGRESS;
}

#if !KOTOR1
int CSWSCreature::CheckItemAttributeRestrictions(CSWSItem* pItem)
{
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_ATTRIBUTE, 0) != 0) {
        for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
            unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
            if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
                if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_ATTRIBUTE) {
                    unsigned short nAbility = pItem->GetPassiveProperty(i)->m_subType;
                    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_ATTRIBUTE);
                    int nRequired;
                    pCostTable->GetINTEntry(pItem->GetPassiveProperty(i)->m_costTableValue,
                                            CExoString("Value"), &nRequired);
                    int nScore;
                    switch (nAbility) {
                    case ABILITY_STRENGTH: nScore = m_pStats->m_nSTRBase; break;
                    case ABILITY_DEXTERITY: nScore = m_pStats->m_nDEXBase; break;
                    case ABILITY_CONSTITUTION: nScore = m_pStats->m_nCONBase; break;
#ifdef K2_FIX_ENGINE_BUGS
                    case ABILITY_INTELLIGENCE: nScore = m_pStats->m_nINTBase; break;
#else
                    case ABILITY_INTELLIGENCE: nScore = m_pStats->m_nCONBase; break;   // (sic) INT is checked against CON
#endif
                    case ABILITY_WISDOM: nScore = m_pStats->m_nWISBase; break;
                    case ABILITY_CHARISMA: nScore = m_pStats->m_nCHABase; break;
#ifdef K2_FIX_ENGINE_BUGS
                    default: continue;
#endif
                    }
                    if (nScore < nRequired)
                        return 0;
                }
            }
        }
    }
    return 1;
}
#endif

int CSWSCreature::CheckItemFeatRestrictions(CSWSItem* pItem)
{
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_FEAT,
                                       0) != 0 && m_pStats->HasFeat(FEAT_PROFICIENCY_ALL) == 0) {
        for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
            unsigned char nSlot = pItem->GetPassiveProperty(i)->m_nUpgradeType;
            if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) {
                if (pItem->GetPassiveProperty(i)->m_type == ITEM_PROPERTY_USE_LIMITATION_FEAT) {
                    unsigned short nFeat = pItem->GetPassiveProperty(i)->m_subType;
                    if (m_pStats->GetFeatRemainingUses(nFeat) == 0)
                        return 0;
                }
            }
        }
    }
    return 1;
}

void CSWSCreature::AddToVisibleList(unsigned long long id, int bSeen, int bHeard,
                                    unsigned char nSanctuaryState, int bInvisible)
{
    int nCount = m_visibilityList.m_count;
    if (nCount > 0) {
        uint64_t n = static_cast<unsigned int>(nCount);
        CSWVisibilityNode** ppNode = m_visibilityList.m_pArray;
        do {
            CSWVisibilityNode* pNode = *ppNode;
            if (pNode->m_objectId == id)
                return;
            ppNode++;
        } while (--n != 0);
    }
    CSWVisibilityNode* pNode = new CSWVisibilityNode();
    pNode->m_objectId = id;
    pNode->m_flags = (bSeen & 1) | ((bHeard & 1) << 1) | ((nSanctuaryState & 3) << 2) | ((bInvisible & 1) << 4);
    m_visibilityList.Add(pNode);
}

void CSWSCreature::AddToInvitationsIgnored(unsigned long long objectId)
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    CSWSInvitationDetails details;
    details.m_objectId = objectId;
    details.m_nDayOffered = nDay;
    details.m_nTimeOfDayOffered = nTimeOfDay;
    if (m_pInvitationsIgnored == 0)
        m_pInvitationsIgnored = new CExoArrayList<CSWSInvitationDetails>();
    m_pInvitationsIgnored->Add(details);
}

// Twin of AddToInvitationsIgnored.
void CSWSCreature::AddToInvitationsOffered(unsigned long long objectId)
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    CSWSInvitationDetails details;
    details.m_objectId = objectId;
    details.m_nDayOffered = nDay;
    details.m_nTimeOfDayOffered = nTimeOfDay;
    if (m_pInvitationsOffered == 0)
        m_pInvitationsOffered = new CExoArrayList<CSWSInvitationDetails>();
    m_pInvitationsOffered->Add(details);
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_TeleportForward(
    CSWPartyMemberData* pMemberData, CSWPartyFollowDataArray* pFollowData, float& fTimeLeft, int a)
{
    K2_UNUSED(a);
    // The first GetPathData call's result is never read.
    int bFollowPointValid = pMemberData->m_bFollowPointValid;
    int nPathIndex = pMemberData->m_nPathIndex;
    int nHead = pFollowData->m_nWriteIndex;
    pFollowData->GetPathData(bFollowPointValid);
    Vector vPos = pFollowData->GetPathData(nPathIndex)->m_pos;
    int i = (nPathIndex > 98) ? 0 : nPathIndex + 1;
    vPos = pFollowData->GetPathData(i)->m_pos;
    while (static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vPos, m_pPathfindInfo) == 0 &&
           i != nHead) {
        i = (i > 98) ? 0 : i + 1;
        vPos = pFollowData->GetPathData(i)->m_pos;
    }
    if (i != nHead) {
        pMemberData->m_nPathIndex = i;
        pMemberData->m_vPathPoint = vPos;
        pMemberData->m_safePosition = vPos;
        fTimeLeft = 0.0f;
        pMemberData->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_MOVING_TO_PATH;
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionChangeFacingObject(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    unsigned long long oidTarget = pNode->m_args.m_facingTargetId;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
        != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if (pObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
        return ACTION_FAILED;
    if (static_cast<CSWSObject*>(pObject)->GetArea() != GetArea())
        return ACTION_FAILED;
    CSWSObject* pTarget = pObject->AsSWSObject();
    Vector vDelta;
    vDelta.x = pTarget->m_position.x - m_position.x;
    vDelta.y = pTarget->m_position.y - m_position.y;
    vDelta.z = 0.0f;
    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z >= 0.00001f) {
        Vector vDir = normalize(vDelta);
        SetOrientation(vDir);
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocationTestDistance(const Vector& vFrom, const Vector& vTo,
                                                 Vector* pvHitPosition,
                                                 unsigned long long* pnBlockingCreature)
{
    if (GetArea() == 0)
        return 1;

    unsigned long long oidSavedTarget = m_pPathfindInfo->m_oidGoalObject;
    m_pPathfindInfo->m_oidGoalObject = CGameObjectArray::INVALIDOBJID;

    float fZLow  = (vFrom.z < vTo.z ? vFrom.z : vTo.z) + -0.1f;
    float fZHigh = (vFrom.z < vTo.z ? vTo.z : vFrom.z) + (m_pPathfindInfo->m_fHeight + 0.1f);

    CWalkHitInfo hitInfo = CWalkHitInfo();
    unsigned long long oidDoor     = OBJECT_INVALID;
    unsigned long long oidCreature = OBJECT_INVALID;
    unsigned long long oidIgnored;
    hitInfo.m_hitSubType  = 0;
    hitInfo.m_hitObjectId = OBJECT_INVALID;

    int bResult;
    int bNoWalls = static_cast<CSWSArea*>(GetArea())->NoNonWalkPolysDetailed(vFrom, vTo, fZLow, fZHigh,
                                                     m_pPathfindInfo->m_fPersonalSpace,
                                                     m_pPathfindInfo->m_oidIgnoreDoor, oidDoor,
                                                     1, 0, 0, &hitInfo, 0);
    if (bNoWalls != 0) {
        bResult = 1;
        if (static_cast<CSWSArea*>(GetArea())->NoCreaturesOnLine(vFrom, vTo, m_pPathfindInfo, 1, 0, &oidCreature, 1,
                                         &hitInfo) != 0)
            goto done;
    }

    if (pnBlockingCreature != 0)
        *pnBlockingCreature = oidCreature;
    if (pvHitPosition != 0)
        *pvHitPosition = hitInfo.m_hitPosition;

    if (oidCreature != OBJECT_INVALID) {
        // A creature is in the way. BumpFriends asks it to step aside; if it agrees, re-test the
        // segment against creatures only (no hit info wanted this time).
        bResult = 0;
        CSWSCreature* pOther =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
        if (pOther != 0 && BumpFriends(pOther, vTo, vFrom) != 0) {
            bResult = static_cast<CSWSArea*>(GetArea())->NoCreaturesOnLine(vFrom, vTo, m_pPathfindInfo, 1, 0,
                                                   &oidIgnored, 0, 0) == 1;
        }
    } else {
        bResult = 0;
        // Nothing but geometry blocks us. If the blocker is a door that is still closed, poke it
        // with a signal event so its OnFailToOpen/AI can react, then give up for this step.
        if (bNoWalls == 0 && oidDoor != OBJECT_INVALID) {
            CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidDoor);
            if (pDoor != 0 && pDoor->m_nAnimation == SERVER_ANIMATION_CLOSED) {
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidDoor, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }

done:
    m_pPathfindInfo->m_oidGoalObject = oidSavedTarget;
    return bResult;
}

int CSWSCreature::GetUserActionsPending()
{
    CExoLinkedListNode* pNode = m_pCombatRound->m_pRoundActionList->m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;
    CSWSCombatRoundAction* pAction = static_cast<CSWSCombatRoundAction*>(
        m_pCombatRound->m_pRoundActionList->m_pInternal->GetAtPos(pNode));
    while (pAction != 0) {
        CGameObject* pObject1 = 0;
        if (pAction->m_oidTarget != OBJECT_INVALID) {
            pObject1 = g_pAppManager->m_pServerExoApp->GetGameObject(pAction->m_oidTarget);
            if (pObject1 != 0 && pObject1->AsSWSCreature() != 0 &&
                pObject1->AsSWSCreature()->GetDead() != 0)
                pObject1 = 0;
        }
        CGameObject* pObject2 = 0;
        if (pAction->m_oidSpellTarget != OBJECT_INVALID) {
            pObject2 = g_pAppManager->m_pServerExoApp->GetGameObject(pAction->m_oidSpellTarget);
            if (pObject2 != 0 && pObject2->AsSWSCreature() != 0 &&
                pObject2->AsSWSCreature()->GetDead() != 0)
                pObject1 = 0;  // the first local is zeroed here
        }
        if (pAction->m_bIsPC != 0 && pObject1 != 0 && pObject2 != 0)
            return 1;
        pAction = static_cast<CSWSCombatRoundAction*>(
            m_pCombatRound->m_pRoundActionList->m_pInternal->GetNext(pNode));
    }
    return 0;
}

int CSWSCreature::AIActionForceFollowObject(CSWSObjectActionNode* pNode)
{
    if (GetDead() != 0)
        return ACTION_FAILED;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() != 0 || GetDeadTemp() != 0)
        return ACTION_FAILED;
#else
    if (GetDeadTemp() != 0)
        return ACTION_FAILED;
#endif

    float fWait = 0.25f;
    Vector vOffset = {};
    unsigned long long oidTarget = pNode->m_args.m_aqTargetId;
    int nMoveParam = pNode->m_args.m_nForceFollowParam;
    vOffset.x = pNode->m_args.m_fForceFollowOffsetX;
    vOffset.y = pNode->m_args.m_fForceFollowOffsetY;
    int nCheckRange = pNode->m_args.m_bForceFollowCheckRange;
    float fLastX = pNode->m_args.m_fForceFollowLastX;
    float fLastY = pNode->m_args.m_fForceFollowLastY;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject == 0)
        return ACTION_FAILED;
    if (pObject->AsSWSObject() == 0)
        return ACTION_FAILED;

    // Two separate GetSWParty() calls, one per half of the anchor pose.
    Vector vAnchor = g_pAppManager->m_pClientExoApp->GetSWParty()->m_followAnchorPosition;
    Vector vDest = vAnchor + RotateVector(
        vOffset, g_pAppManager->m_pClientExoApp->GetSWParty()->m_followAnchorOrientation);

    unsigned long long oidArea = pObject->AsSWSObject()->m_cachedAreaId;

    Vector vLast = {};
    vLast.x = fLastX;
    vLast.y = fLastY;
    Vector vDelta = vDest - vLast;
    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;

    if (nCheckRange != 0) {
        float fRange = g_pAppManager->m_pClientExoApp->GetSWParty()->GetFollowRange();
        if (fDistSq >= fRange * fRange) {
            nCheckRange = 0;
            AddActionToFront(AIACTION_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nMoveParam,
                             ACTION_PARAM_FLOAT, static_cast<void*>(&vOffset),
                             ACTION_PARAM_FLOAT, static_cast<void*>(&vOffset.y),
                             ACTION_PARAM_INT, &nCheckRange,
                             ACTION_PARAM_FLOAT, static_cast<void*>(&vDest),
                             ACTION_PARAM_FLOAT, static_cast<void*>(&vDest.y), 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0);
        } else {
            return ACTION_IN_PROGRESS;
        }
    } else {
        nCheckRange = (fDistSq <= 0.1 * 0.1);
        AddActionToFront(AIACTION_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nMoveParam,
                         ACTION_PARAM_FLOAT, static_cast<void*>(&vOffset),
                         ACTION_PARAM_FLOAT, static_cast<void*>(&vOffset.y),
                         ACTION_PARAM_INT, &nCheckRange,
                         ACTION_PARAM_FLOAT, static_cast<void*>(&vDest),
                         ACTION_PARAM_FLOAT, static_cast<void*>(&vDest.y), 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0);
    }
    // A party member re-evaluates ten times as often as anyone else.
    if (m_bInParty != 0)
        fWait = 0.025f;
    AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea, oidTarget, nMoveParam,
                                0.0f, 0.0f, 0, 0, 0, 0, 0, 0, vOffset, 0, 0, 0);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckForceFollowObject(CSWSObjectActionNode* pNode)
{
    Vector vPos = m_position;
    unsigned long long oidTarget = pNode->m_args.m_attackTargetId;
    unsigned int nMoveParam = pNode->m_args.m_equipSlot;
    int fOffsetX = pNode->m_args.m_equipFlag;
    float fOffsetY = pNode->m_args.m_locX;
    int nZero = 0;
    AddActionToFront(AIACTION_CHECK_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW,
                     ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nMoveParam,
                     ACTION_PARAM_FLOAT, &fOffsetX, ACTION_PARAM_FLOAT, &fOffsetY, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddActionToFront(AIACTION_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW,
                     ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nMoveParam,
                     ACTION_PARAM_FLOAT, &fOffsetX, ACTION_PARAM_FLOAT, &fOffsetY,
                     ACTION_PARAM_INT, &nZero, ACTION_PARAM_FLOAT, static_cast<void*>(&vPos),
                     ACTION_PARAM_FLOAT, static_cast<void*>(&vPos.y), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveAwayFromLocation(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    Vector vTarget;
    vTarget.x = pNode->m_args.m_moveAwayX;
    vTarget.y = pNode->m_args.m_moveAwayY;
    vTarget.z = pNode->m_args.m_moveAwayZ;
    int bRun = pNode->m_args.m_bCheckMoveAwayRun;
    float fRadius = pNode->m_args.m_moveAwayRadius;
    int nCheckParam = pNode->m_args.m_bCheckMoveToRun;
    float fDx = m_position.x - vTarget.x;
    float fDy = m_position.y - vTarget.y;
    float fDz = m_position.z - vTarget.z;
    if (fDx * fDx + fDy * fDy + fDz * fDz <= fRadius * fRadius) {
        Vector vAway = pArea->ComputeAwayVector(m_position, vTarget, fRadius, m_pPathfindInfo);
        unsigned long long oidArea = pArea->m_objectId;
        AddActionToFront(AIACTION_CHECK_MOVE_AWAY_FROM_LOCATION, pNode->m_nGroupId,
                         ACTION_PARAM_FLOAT, &vTarget.x, ACTION_PARAM_FLOAT, &vTarget.y,
                         ACTION_PARAM_FLOAT, &vTarget.z, ACTION_PARAM_INT, &bRun,
                         ACTION_PARAM_FLOAT, &fRadius, ACTION_PARAM_INT, &nCheckParam, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddMoveToPointActionToFront(pNode->m_nGroupId, vAway, oidArea,
                                    OBJECT_INVALID, bRun, 0.0f, 0.0f, 0, 0, 0, 0, 1, 0,
                                    Vector{}, 0, 0, 0);
        AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveToObject(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    unsigned long long oidTarget = pNode->m_args.m_moveToObjectId;
    int nParam = pNode->m_args.m_moveToObjParam;
    CGameObject* pObject;
    Vector vPos;
    float fRange = 0.0f;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    CSWSArea* pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pObject)->GetArea());
    GetUseRange(oidTarget, vPos, fRange, 0, 0);
    if (pArea == 0) {
        if (pObject->AsSWSCreature() == 0)
            return ACTION_FAILED;
        pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        vPos = pObject->AsSWSCreature()->m_vDesiredAreaLocation;
        if (pArea == 0)
            return ACTION_FAILED;
    }
    if (pArea != static_cast<CSWSArea*>(GetArea()) || GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0) {
        unsigned long long oidArea = pArea->m_objectId;
        AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nParam, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, oidArea,
                                    oidTarget, nParam, fRange, 0.0f, 0, 0, 0, 0, 0, 0,
                                    Vector{}, 0, 0, 0);
        AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
    }
    SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveToObjectRadius(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    unsigned long long oidTarget = pNode->m_args.m_moveToObjectId;
    int nParam = pNode->m_args.m_moveToObjParam;
    float fRange = pNode->m_args.m_moveToObjRange;
    float fRadius = pNode->m_args.m_moveToObjRadius;
    int nMoveToObjFlag = pNode->m_args.m_nCheckMoveToObjParam;
    int nUseRangeParam = 0;
    if (pNode->m_args.m_spellTargetId != 0)
        nUseRangeParam = static_cast<int>(pNode->m_args.m_spellTargetId);
    int nParam2 = pNode->m_args.m_moveToObjParam2;
    CGameObject* pObject;
    Vector vPos;
    float fUseRange;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if (pObject == 0 || pObject->AsSWSObject() == 0)
        return ACTION_FAILED;
    GetUseRange(oidTarget, vPos, fUseRange, 0, 0);
    CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSObject()->GetArea());
    if (pObject->AsSWSObject()->GetArea() == 0) {
        if (pObject->AsSWSCreature() == 0)
            return ACTION_FAILED;
        pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        vPos = pObject->AsSWSCreature()->m_vDesiredAreaLocation;
    }
    if (pArea == 0)
        return ACTION_FAILED;
    if (GetArea() == 0)
        return ACTION_FAILED;
    CSWSArea* pSelfArea = static_cast<CSWSArea*>(GetArea());
    float fSlack = fRadius - fUseRange;
    if (fSlack < 0.0f)
        fSlack = 0.0f;
    if ((pArea == pSelfArea)
            & (GetIsInUseRange(oidTarget, fSlack, nUseRangeParam, 0) != 0))
        return ACTION_COMPLETE;
    unsigned long long oidArea = pArea->m_objectId;
    AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                     ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nParam,
                     ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRadius,
                     ACTION_PARAM_INT, &nMoveToObjFlag, ACTION_PARAM_INT, &nUseRangeParam,
                     ACTION_PARAM_INT, &nParam2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Vector vUp;
    AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, oidArea, oidTarget, nParam,
                                fRange, 0.0f, 0, 0, 0, 0, 0, 1, vUp, 0, 0, nParam2);
    AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveToPoint(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    if (!(m_nAIState & AI_STATE_MOBILE))
        return ACTION_FAILED;
    float fX = pNode->m_args.m_moveToX;
    float fY = pNode->m_args.m_moveToY;
    float fZ = pNode->m_args.m_moveToZ;
    unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
    unsigned long long oidTarget = pNode->m_args.m_moveToTargetId;
    int nParam = pNode->m_args.m_bCheckMoveToRun;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidArea, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    float fPersonalSpace = m_pPathfindInfo->m_fPersonalSpace;
    if (static_cast<CSWSArea*>(GetArea()) == 0
            || static_cast<CSWSArea*>(GetArea())->m_objectId == oidArea) {
        float fDx = m_position.x - fX;
        float fDy = m_position.y - fY;
        float fDz = m_position.z - fZ;
        if (!(fDx * fDx + fDy * fDy + fDz * fDz
                >= (fPersonalSpace + fPersonalSpace) * (fPersonalSpace + fPersonalSpace)))
            return ACTION_COMPLETE;
    }
    AddActionToFront(AIACTION_CHECK_MOVE_TO_POINT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fX,
                     ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ,
                     ACTION_PARAM_OBJECT, &oidArea, ACTION_PARAM_OBJECT, &oidTarget,
                     ACTION_PARAM_INT, reinterpret_cast<void*>(static_cast<int64_t>(nParam)), 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Vector vPos = {fX, fY, fZ};
    Vector vUp;
    AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, oidArea, oidTarget, nParam,
                                0.0f, 0.0f, 0, 0, 0, 0, 0, 0, vUp, 0, 0, 0);
    AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveToPointRadius(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    float fX = pNode->m_args.m_moveToX;
    float fY = pNode->m_args.m_moveToY;
    float fZ = pNode->m_args.m_moveToZ;
    unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
    unsigned long long oidTarget = pNode->m_args.m_moveToTargetId;
    int nParam = pNode->m_args.m_bCheckMoveToRun;
    float fRadius = pNode->m_args.m_moveToRadius;
    int nParam2 = pNode->m_args.m_nMoveToClientPathId;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidArea, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if (static_cast<CSWSArea*>(GetArea()) == 0
            || static_cast<CSWSArea*>(GetArea())->m_objectId == oidArea) {
        float fDx = m_position.x - fX;
        float fDy = m_position.y - fY;
        float fDz = 0.0f;
        if (!(fDx * fDx + fDy * fDy + fDz * fDz > (fRadius + 0.01f) * (fRadius + 0.01f)))
            return ACTION_COMPLETE;
    }
    AddActionToFront(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, pNode->m_nGroupId,
                     ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY, ACTION_PARAM_FLOAT, &fZ,
                     ACTION_PARAM_OBJECT, &oidArea, ACTION_PARAM_OBJECT, &oidTarget,
                     ACTION_PARAM_INT, &nParam, ACTION_PARAM_FLOAT, &fRadius,
                     ACTION_PARAM_INT, &nParam2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Vector vPos = {fX, fY, fZ};
    Vector vUp;
    AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, oidArea, oidTarget, nParam,
                                fRadius, 0.0f, 0, 0, 0, 0, 0, 0, vUp, 0, 0, nParam2);
    AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckMoveToFollowRadius(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    float fX = pNode->m_args.m_moveToX;
    float fY = pNode->m_args.m_moveToY;
    float fZ = pNode->m_args.m_moveToZ;
    unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
    float fRadius = pNode->m_args.m_moveToRadius;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidArea, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if ((static_cast<CSWSArea*>(GetArea()) != 0
            && oidArea != static_cast<CSWSArea*>(GetArea())->m_objectId)
        || (m_position.x - fX) * (m_position.x - fX)
             + (m_position.y - fY) * (m_position.y - fY)
             + (m_position.z - fZ) * (m_position.z - fZ)
           > (fRadius + 0.01f) * (fRadius + 0.01f)) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(oidClient);
        if (nIndex >= 0)
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_WAITING_FOR_PATH;
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::WalkUpdateLocation_QuickWalk_ForcePushed()
{
    int nResult = ACTION_IN_PROGRESS;
    float fMove;
    float fSpeed;
    if (m_nMoveState == 4) {
        fMove = 25.0f;
        fSpeed = 5.0f;
    } else {
        float* pPath = m_pPathfindInfo->m_paWayPoints;
        fSpeed = __builtin_sqrtf((pPath[2] - pPath[0]) * (pPath[2] - pPath[0]) +
                                 (pPath[3] - pPath[1]) * (pPath[3] - pPath[1]) + 0.0f);
        fMove = fSpeed / 0.33f;
    }
    fMove = fMove * static_cast<float>(m_nWalkDeltaMs) / 1000.0f;
    if (fMove != 0.0f) {
        Vector vPos;
        Vector vDelta;
        CExoArrayList<unsigned long long> list;
        if (WalkUpdateLocationDistance(fMove, &vPos, &vDelta, &list) != 0) {
            if (m_objectId == g_pAppManager->m_pServerExoApp->GetPlayerCreatureId()) {
                g_pAppManager->m_pClientExoApp->GetSWParty()->m_followData.ProcessPoint(
                    vPos, Yaw(m_orientation), m_pPathfindInfo->m_fPersonalSpace);
            }
            SetPosition(vPos, 1, 1, 0);
            nResult = ACTION_COMPLETE;
            m_fForcePushDistance = 0.0f;
        } else {
            UpdateSubareasOnMoveTo(m_position, vPos, 1, &list, 0);
            float fOld = m_fForcePushDistance;
            if (m_objectId == g_pAppManager->m_pServerExoApp->GetPlayerCreatureId()) {
                g_pAppManager->m_pClientExoApp->GetSWParty()->m_followData.ProcessPoint(
                    vPos, Yaw(m_orientation), m_pPathfindInfo->m_fPersonalSpace);
            }
            SetPosition(vPos, 1, 1, 0);
            float f = fOld / fSpeed;
            m_fForcePushDistance = fMove + fOld;
            m_fForcePushHeight = 0.0f * f + -0.0f * f * f;
        }
    }
    return nResult;
}

int CSWSCreature::AcquireItem(CSWSItem** ppItem, unsigned long long nPossessorId,
                              unsigned long long nRepositoryId, int nFlag)
{
    g_pAppManager->m_pServerExoApp->GetObjectArray();

    CSWSItem* pOldItem;
    if ((*ppItem)->m_possessorId == nPossessorId) {
        pOldItem = NULL;
    } else {
        pOldItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            (*ppItem)->m_possessorId);
        if (pOldItem == NULL || pOldItem->m_possessorId != nPossessorId)
            return 0;
        nPossessorId = (*ppItem)->m_possessorId;
    }

    CItemRepository* pRepository;
    if (nRepositoryId == OBJECT_INVALID) {
        pRepository = GetItemRepository(1);
    } else {
        pOldItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nRepositoryId);
        if (pOldItem != NULL)
            pRepository = pOldItem->m_pItemRepository;
        else
            pRepository = NULL;
    }

    CGameObject* pGameObject =
        g_pAppManager->m_pServerExoApp->GetGameObject(nPossessorId);
    if (pGameObject != NULL) {
        if (pGameObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
#if KOTOR1  // K1 passes 1 as the second RemoveItem argument.
            pGameObject->AsSWSCreature()->RemoveItem(*ppItem, 1, 1, NULL);
#else
            pGameObject->AsSWSCreature()->RemoveItem(*ppItem, 0, 1, NULL);
#endif
        } else if (pGameObject->m_objectType == GAME_OBJECT_TYPE_ITEM) {
            if (pGameObject->AsSWSItem()->m_pItemRepository != NULL)
                pGameObject->AsSWSItem()->m_pItemRepository->RemoveItem(*ppItem);
        } else if (pGameObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE) {
            pGameObject->AsSWSPlaceable()->RemoveItem(*ppItem);
        }
    } else if (pOldItem != NULL) {
        if (g_pRules->GetBaseItem((*ppItem)->m_baseItem)->m_bContainer != 0 &&
            g_pRules->GetBaseItem(pOldItem->m_baseItem)->m_bContainer != 0)
            return 0;
    }

    int nResult = pRepository->AddItem(ppItem, 1, nFlag, 1);
    if (nResult == 1) {
        if (pGameObject == NULL && *ppItem != NULL)
            (*ppItem)->RemoveFromArea();
        CSWSItem* pItem = *ppItem;  // re-loaded after RemoveFromArea
        if (nRepositoryId == CGameObjectArray::INVALIDOBJID) {
            if (pItem != NULL)
                pItem->SetPossessor(m_objectId, 1, nFlag, OBJECT_INVALID);
        } else {
            if (pItem != NULL)
                pItem->SetPossessor(nRepositoryId, 1, nFlag, OBJECT_INVALID);
        }
        return 1;
    }
    return nResult;
}

int CSWSCreature::AIActionCheckMoveAwayFromObject(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    unsigned long long oidTarget = pNode->m_args.m_moveAwayObjectId;
    int nParam = pNode->m_args.m_moveAwayObjParam;
    float fRange = pNode->m_args.m_moveAwayObjRange;
    int nAttempts = pNode->m_args.m_nMoveAwayAttempts;
    CGameObject* pObject;
    Vector vTarget;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    vTarget = static_cast<CSWSObject*>(pObject)->m_position;
    CSWSArea* pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pObject)->GetArea());

    {
        CExoLinkedListNode* pPos;
        CSWSObjectActionNode* pQueued;
    restart:
        pQueued = m_actionList.GetHead();
        pPos = m_actionList.GetHeadPos();
        while (pQueued != 0) {
            if (pQueued->m_nActionId == AIACTION_CHECK_MOVE_AWAY_FROM_OBJECT
                && pQueued->m_args.m_moveAwayObjectId == oidTarget) {
                CancelAction(0, pQueued->m_nGroupId);
                goto restart;
            }
            pQueued = m_actionList.GetNext(pPos);
        }
    }

    Vector vDelta = m_position - vTarget;
    if (pArea == static_cast<CSWSArea*>(GetArea())) {
        if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRange * fRange
                && nAttempts > 0) {
            nAttempts--;
            Vector vAway = pArea->ComputeAwayVector(m_position, vTarget, fRange, m_pPathfindInfo);
            // Read before AddActionToFront, which overwrites the source.
            unsigned long long oidArea = pArea->m_objectId;
            AddActionToFront(AIACTION_CHECK_MOVE_AWAY_FROM_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nParam,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_INT, &nAttempts, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddMoveToPointActionToFront(pNode->m_nGroupId, vAway, oidArea,
                                        OBJECT_INVALID, nParam, 0.0f, 0.0f, 0, 0, 0, 0, 1, 0,
                                        Vector{}, 0, 0, 0);
            AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
        }
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionCheckInterAreaPathfinding(CSWSObjectActionNode* pNode)
{
    if (GetDead() == 1) {
        return ACTION_FAILED;
    }
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetIsPCDying() == 1 || GetDeadTemp() == 1) {
        return ACTION_FAILED;
    }
#else
    if (GetDeadTemp() == 1) {
        return ACTION_FAILED;
    }
#endif
    if (GetArea() == 0) {
        return ACTION_IN_PROGRESS;
    }
    int nCount = m_pPathfindInfo->m_nInterAreaCount;
    if (nCount >= 2) {
        int nNewAreas = nCount - 1;
        int nNewTransitions = nCount - 2;
        unsigned long long* paAreas = new unsigned long long[nNewAreas];
        unsigned long long* paTransitions = 0;
#if KOTOR1
        if (nNewTransitions != 0) {
#else
        if (nNewTransitions > 0) {
#endif
            paTransitions = new unsigned long long[nNewTransitions];
            for (int i = 0; i < nNewTransitions; i++) {
                paTransitions[i] = m_pPathfindInfo->m_paInterAreaIds[i + 1];
            }
        }
        for (int i = 0; i < nNewAreas; i++) {
            paAreas[i] = m_pPathfindInfo->m_paInterAreaIds[i + 1];
        }
        delete[] m_pPathfindInfo->m_paInterAreaIds;
        delete[] m_pPathfindInfo->m_paInterAreaTransitionIds;
        m_pPathfindInfo->m_paInterAreaTransitionIds = paTransitions;
        m_pPathfindInfo->m_paInterAreaIds = paAreas;
        m_pPathfindInfo->m_nInterAreaCount = nNewAreas;
        if (GetArea() != 0) {
            g_pAppManager->m_pServerExoApp->GetModule()->SetIntraAreaGoal(m_pPathfindInfo);
            float fGoalX = m_pPathfindInfo->m_goal.m_vPoint.x;
            Vector vGoal;
            vGoal.x = fGoalX;
            vGoal.y = m_pPathfindInfo->m_goal.m_vPoint.y;
            vGoal.z = m_pPathfindInfo->m_goal.m_vPoint.z;
            unsigned long long oidGoalArea = m_pPathfindInfo->m_oidGoalArea;
            m_pPathfindInfo->m_goal.m_vPoint.x = fGoalX - 5.0f;
            m_pPathfindInfo->m_bKeepInterAreaRoute = 1;
            AddMoveToPointActionToFront(pNode->m_nGroupId, vGoal, oidGoalArea,
                                        OBJECT_INVALID, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0,
                                        Vector{}, 0, 0, 0);
            AddPathfindingWaitActionToFront(pNode->m_nGroupId, 0.0f);
        }
    } else {
        m_pPathfindInfo->ResetInterAreaSearchData();
    }
    float fWait = 0.75f;
    AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionJumpToObject(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    if (m_pPathfindInfo == 0)
        return ACTION_FAILED;

    unsigned short nParams = pNode->m_nNumParameters;
    int nWalkStraightLine = 1;
    if (nParams >= 2)
        nWalkStraightLine = pNode->m_args.m_bJumpWalkStraightLine;

    CSWSObject* pObject =
        static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(pNode->m_args.m_aqTargetId));
    if (pObject == 0)
        return ACTION_FAILED;
    if (pObject->AsSWSObject() == 0)
        return ACTION_FAILED;
    CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSObject()->GetArea());

    Vector vDest;
    Vector vFacing;
    Vector vSafe;

    if (pObject->m_objectType == GAME_OBJECT_TYPE_WAYPOINT) {
        vDest = pObject->AsSWSObject()->m_position;
        vFacing = pObject->AsSWSObject()->m_orientation;
    } else if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR) {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        vDest = pDoor->GetNearestActionPoint(
            pDoor->m_position - pObject->AsSWSDoor()->CSWSObject::m_orientation, 1);
        float fStandOff = m_pPathfindInfo->m_fCrePersonalSpace +
                          m_pPathfindInfo->m_fCrePersonalSpace;
        vDest = vDest - pDoor->CSWSObject::m_orientation * fStandOff;
        vFacing = normalize(vDest - pDoor->m_position);
    } else {
        vDest = pObject->m_position;
        vFacing = normalize(pObject->AsSWSObject()->m_position - vDest);
    }

    // A target that is in limbo has no area; fall back to the area and position it remembers.
    if (pArea == 0) {
        if (pObject->AsSWSCreature() == 0)
            return ACTION_FAILED;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(pCreature->m_currentAreaId);
        vDest = pCreature->m_vDesiredAreaLocation;
        if (pArea == 0)
            return ACTION_FAILED;
    }

    m_pPathfindInfo->ResetInterTileSearchData();
    m_pPathfindInfo->ResetGridSearchData();
    m_pPathfindInfo->m_oidCreature = m_objectId;
    if (pArea->ComputeSafeLocation(vDest, 20.0f, m_pPathfindInfo, nWalkStraightLine, &vSafe) != 1) {
        m_pPathfindInfo->ResetInterAreaSearchData();
        return ACTION_FAILED;
    }

    AutoCloseGUIPanels(0);
    CutWaypointPath();

    float fFacingX = m_orientation.x;
    float fFacingY = m_orientation.y;

    // Only the party leader drags the rest of the party's follow data to the new spot.
    unsigned long long oidLeader =
        g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[0].id;
    if (m_objectId == g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader)) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        Vector vLeaderFacing;
        vLeaderFacing.x = fFacingX;
        vLeaderFacing.y = fFacingY;
        vLeaderFacing.z = 0.0f;
        pParty->ResetAllFollowData(vSafe, Yaw(normalize(vLeaderFacing)), 1);
    }

    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
    SetOrientation(vFacing);
    if (pArea == static_cast<CSWSArea*>(GetArea())) {
        SetPosition(vSafe, 1, 1, 0);
        UpdateSubareasOnJumpPosition(vSafe, pArea->m_objectId, 1);
    } else if (pPlayer == 0) {
        AddToArea(pArea, vSafe.x, vSafe.y, vSafe.z, 0, 0);
        UpdateSubareasOnJumpPosition(vSafe, pArea->m_objectId, 1);
    } else {
        UpdateSubareasOnJumpPosition(vSafe, OBJECT_INVALID, 1);
        AddToArea(pArea, vSafe.x, vSafe.y, vSafe.z, 0, 0);
        AddActionToFront(AIACTION_AREA_WAIT, pNode->m_nGroupId, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    AddActionToFront(AIACTION_CHECK_INTER_AREA_PATHFINDING, pNode->m_nGroupId, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    m_bJumpPending = 1;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionJumpToPoint(CSWSObjectActionNode* pNode)
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1)
#else
    if (GetDead() == 1 || GetDeadTemp() == 1)
#endif
        return ACTION_FAILED;
    if (m_pPathfindInfo == 0)
        return ACTION_FAILED;

    if (GetArea() == 0) {
        if (g_pAppManager->m_pServerExoApp->GetModule()->IsObjectInLimbo(m_objectId) != 0)
            g_pAppManager->m_pServerExoApp->GetModule()->RemoveObjectFromLimbo(m_objectId);
    }

    float fFacingX = m_orientation.x;
    float fFacingY = m_orientation.y;
    float fDestX = pNode->m_args.m_moveToX;
    float fDestY = pNode->m_args.m_moveToY;
    float fDestZ = pNode->m_args.m_moveToZ;
    unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
    unsigned short nParams = pNode->m_nNumParameters;
    int nSafeLocation = 1;
    float fSearchRadius = 20.0f;
    if (nParams >= 5) {
        nSafeLocation = pNode->m_args.m_bJumpSafeLocation;
        if (nParams >= 6) {
            fSearchRadius = pNode->m_args.m_fJumpSearchRadius;
            if (nParams >= 7) {
                fFacingX = pNode->m_args.m_fJumpFacingX;
                fFacingY = pNode->m_args.m_fJumpFacingY;
            }
        }
    }

    GetArea();

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;
    if (pObjectArray->GetGameObject(oidArea, &pObject) != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if (pObject == 0)
        return ACTION_FAILED;
    if (pObject->m_objectType != CGameObject::OBJECT_TYPE_AREA)
        return ACTION_FAILED;
    CSWSArea* pArea = pObject->AsSWSArea();

    m_pPathfindInfo->ResetInterTileSearchData();
    m_pPathfindInfo->ResetGridSearchData();

    Vector vSafe;
    m_pPathfindInfo->m_oidCreature = m_objectId;
    Vector vDest;
    vDest.x = fDestX;
    vDest.y = fDestY;
    vDest.z = fDestZ;
    if (pArea->ComputeSafeLocation(vDest, fSearchRadius, m_pPathfindInfo,
                                   nSafeLocation != 0, &vSafe) != 1) {
        m_pPathfindInfo->ResetInterAreaSearchData();
        return ACTION_FAILED;
    }

    AutoCloseGUIPanels(0);
    CutWaypointPath();

    // Only the party leader drags the rest of the party's follow data to the new spot.
    unsigned long long oidLeader =
        g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[0].id;
    if (m_objectId == g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidLeader)) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        Vector vFacing;
        vFacing.x = fFacingX;
        vFacing.y = fFacingY;
        vFacing.z = 0.0f;
        pParty->ResetAllFollowData(vSafe, Yaw(normalize(vFacing)), 1);
    }

    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
    if (pArea == static_cast<CSWSArea*>(GetArea())) {
        SetPosition(vSafe, 1, 1, 0);
        UpdateSubareasOnJumpPosition(vSafe, pArea->m_objectId, 1);
    } else if (pPlayer == 0) {
        AddToArea(pArea, vSafe.x, vSafe.y, vSafe.z, 0, 0);
        UpdateSubareasOnJumpPosition(vSafe, pArea->m_objectId, 1);
    } else {
        UpdateSubareasOnJumpPosition(vSafe, OBJECT_INVALID, 1);
        AddToArea(pArea, vSafe.x, vSafe.y, vSafe.z, 0, 0);
        AddActionToFront(AIACTION_AREA_WAIT, pNode->m_nGroupId, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    Vector vNewFacing;
    vNewFacing.x = fFacingX;
    vNewFacing.y = fFacingY;
    vNewFacing.z = 0.0f;
    SetOrientation(normalize(vNewFacing));
    AddActionToFront(AIACTION_CHECK_INTER_AREA_PATHFINDING, pNode->m_nGroupId, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    m_bJumpPending = 1;
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionRandomWalk(CSWSObjectActionNode* pNode)
{
    float fAnchorX = pNode->m_args.m_randomWalkX;
    float fAnchorY = pNode->m_args.m_randomWalkY;
    float fAnchorZ = pNode->m_args.m_randomWalkZ;
    unsigned long long oidArea = pNode->m_args.m_randomWalkAreaId;
    float fWait = 3.0f;

    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea == 0)
        return ACTION_FAILED;

    // Everything else the creature was going to do is dropped on the floor.
    while (m_actionList.m_pInternal->m_pHead != 0)
        delete static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());

    if (m_aiLevel > 0) {
        AddActionToFront(AIACTION_RANDOM_WALK, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fAnchorX,
                         ACTION_PARAM_FLOAT, &fAnchorY, ACTION_PARAM_FLOAT, &fAnchorZ,
                         ACTION_PARAM_OBJECT, &oidArea, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0);
        AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        float fBaseX = fAnchorX;
        float fBaseY = fAnchorY;
        Vector vDest;
        vDest.z = 0.0f;
        vDest.x = static_cast<float>(rand_wincompatible() % 15) - 7.0f + fBaseX;
        vDest.y = static_cast<float>(rand_wincompatible() % 15) - 7.0f + fBaseY;
        vDest.z = pArea->ComputeHeight(vDest, 1, 0, 0);

        Vector vDir = vDest - m_position;
        float fSpan = magnitude(vDir);
        float fMin = fSpan * 0.3f;
        vDir = normalize(vDir);
        pArea->m_pActivePathfindInfo = m_pPathfindInfo;

        int nTries = 21;
        float fReach = fSpan;
        while (pArea->TestDirectLine(m_position, vDest,
                                     static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                     m_pPathfindInfo->m_fHeight, 0, 0) != 1) {
            fReach = fReach * 0.75f;
            if (fReach < fMin) {
                rand_wincompatible();
                Vector vUp;
                vUp.x = 0.0f;
                vUp.y = 0.0f;
                vUp.z = 1.0f;
                Vector vKick = crossProduct(vUp, crossProduct(vUp, vDir));
                vDir.x = -vKick.x;
                vDir.y = -vKick.y;
                vDir.z = -vKick.z;
                fReach = fSpan;
            }
            vDest = m_position + vDir * fReach;
            if (fReach < 1.0f)
                return ACTION_COMPLETE;
            if (--nTries < 0)
                return ACTION_COMPLETE;
        }

        Vector vZero;
        AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea, OBJECT_INVALID, 0, 0.0f, 0.0f,
                                    0, 0, 0, 0, 1, 0, vZero, 0, 0, 0);
    } else {
        fWait = 15.0f;
        AddActionToFront(AIACTION_RANDOM_WALK, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fAnchorX,
                         ACTION_PARAM_FLOAT, &fAnchorY, ACTION_PARAM_FLOAT, &fAnchorZ,
                         ACTION_PARAM_OBJECT, &oidArea, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0);
        AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return ACTION_COMPLETE;
}

int CSWSCreature::AIActionItemCastSpell(CSWSObjectActionNode* pAction)
{
    unsigned int nNowDay;
    unsigned int nNowTod;
    unsigned int nElapsedDay;
    unsigned int nElapsedTod;
    unsigned int nStartDay;
    unsigned int nStartTod;
    Vector vTarget;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;

    unsigned long long oidItem   = pAction->m_args.m_useItemPropItemId;
    int                nProperty = pAction->m_args.m_aqItemPropertyIndex;
    unsigned long long oidTarget = pAction->m_args.m_itemCastTargetId;
    float fTargetX = pAction->m_args.m_aqItemPointX;
    float fTargetY = pAction->m_args.m_aqItemPointY;
    float fTargetZ = pAction->m_args.m_aqItemPointZ;

    int            bHasAnim    = 0;
    unsigned short nAnimStored = 0;
    int            nAnimEnd    = 0;
    if (pAction->m_nNumParameters <= 8) {
        g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(
            &nStartDay, &nStartTod);
        pAction->m_args.m_nItemCastStartDay = nStartDay;
        pAction->m_args.m_nItemCastStartTod = nStartTod;
        pAction->m_nNumParameters = 9;
    } else {
        nStartDay = pAction->m_args.m_nItemCastStartDay;
        nStartTod = pAction->m_args.m_nItemCastStartTod;
        if (pAction->m_nNumParameters >= 10) {
            bHasAnim    = pAction->m_args.m_bItemCastHasAnim;
            nAnimStored = pAction->m_args.m_nItemCastAnimation;
            nAnimEnd    = pAction->m_args.m_nItemCastAnimEnd;
        }
    }

    if (m_bInParty != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        unsigned long long oidClient = pClient->ServerToClientObjectId(m_objectId);
        int nIndex = pClient->GetSWParty()->GetIndex(oidClient);
        if (nIndex >= 0)
#if KOTOR1  // K1: strref 48208
            pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_USE_LIGHTSABER);
#else
            pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
#endif
    }

    m_nEndRoundScriptTimer = 3000;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1) {
#else
    if (GetDead() == 1 || GetDeadTemp() == 1) {
#endif
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bItemCastStarted = 0;
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }

    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nNowDay, &nNowTod);
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->SubtractWorldTimes(
        nNowDay, nNowTod, nStartDay, nStartTod, &nElapsedDay, &nElapsedTod);

    if (bHasAnim != 0) {
        if (nElapsedTod < static_cast<unsigned int>(nAnimEnd)) {
            SetAnimation(nAnimStored);
            return CSWSObject::ACTION_IN_PROGRESS;
        }
        SetAnimation(GetPauseReadyAnimation());
        m_bItemCastStarted = 0;
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_COMPLETE;
    }

    {
        CGameObject* pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget));
        if (oidTarget != OBJECT_INVALID && pObj == 0)
            goto castFailed;
        if (pObj != 0 && pObj->AsSWSCreature() != 0) {
            if (pObj->AsSWSCreature()->GetDead() != 0)
                goto castFailed;
            CSWSCreature* pBlocker = pObj->AsSWSCreature();
            if (pBlocker->GetDeadTemp() != 0)
                goto castFailed;
        }

        m_oidSpellTarget = (oidTarget == OBJECT_INVALID) ? m_cachedAreaId : oidTarget;

        CSWSCreature* pTargetCreature = 0;
        if (pObj != 0)
            pTargetCreature = pObj->AsSWSCreature();

        CSWSItem* pItem =
            static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem));
        if (pItem == 0)
            goto castFailed;
        CSWItemProperty* pProperty = pItem->GetActiveProperty(nProperty);
        if (pProperty == 0)
            goto castFailed;
#if KOTOR1  // K1: the property subtype is the spell id (no grenade conversion).
        int nSpellId = pProperty->m_subType;
#else
        int nSpellId = ConvertGrenadeSpellID(pProperty->m_subType);
#endif
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell == 0)
            goto castFailed;
        if (pSpell->m_bHostileActivity != 0) {
            ActivityManager(1);
#if KOTOR1
            SetCombatState(1, 1);
#else
            SetCombatState(1, 1, 0);
#endif
        } else {
            ActivityManager(2);
        }

#if !KOTOR1  // K1 does not re-read the elapsed time here.
        g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(
            &nNowDay, &nNowTod);
        static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->SubtractWorldTimes(
            nNowDay, nNowTod, nStartDay, nStartTod, &nElapsedDay, &nElapsedTod);
#endif

        vTarget.x = fTargetX;
        vTarget.y = fTargetY;
        vTarget.z = fTargetZ;

        CExoString sModelType;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            m_appearance.m_headId, CExoString("MODELTYPE"), &sModelType);
        int bHumanoidModel = sModelType.CompareNoCase("B") || sModelType.CompareNoCase("F");

        pAction->m_bInterruptable = 0;

        int nAnimation;
        int nDelay;
        int nPauseTime;
        int bThrownItem;
        switch (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType) {
#if KOTOR1  // K1: base item 6 only, no mask (0x30) throw animation.
        case ITEM_TYPE_GRENADE:
            nPauseTime = 1500;
            bThrownItem = 1;
            if (bHumanoidModel != 0) {
                float fDX = vTarget.x - m_position.x;
                float fDY = vTarget.y - m_position.y;
                float fDZ = vTarget.z - m_position.z;
                float fDistSq = fDX * fDX + fDY * fDY + fDZ * fDZ;
                nAnimation = !(fDistSq >= GrenadeThrowThreshold * GrenadeThrowThreshold)
                                 ? SERVER_ANIMATION_THROW_LOW : SERVER_ANIMATION_THROW_HIGH;
                nDelay = !(fDistSq >= GrenadeThrowThreshold * GrenadeThrowThreshold)
                             ? 700 : 800;
            } else {
                nAnimation = SERVER_ANIMATION_READY;
                nDelay = 800;
            }
            break;
#else
        case ITEM_TYPE_GRENADE:
        case ITEM_TYPE_ROCKET:
            nPauseTime = 1500;
            bThrownItem = 1;
            if (bHumanoidModel != 0) {
                CSWItem* pMask = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTARM);
                if ((pMask != 0 && pMask->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER)
                    || ((pMask = m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTARM)) != 0
                        && pMask->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER)) {
                    nAnimation = SERVER_ANIMATION_WRIST_LAUNCH;
                    nDelay = 950;
                } else {
                    float fDX = vTarget.x - m_position.x;
                    float fDY = vTarget.y - m_position.y;
                    float fDZ = vTarget.z - m_position.z;
                    float fDistSq = fDX * fDX + fDY * fDY + fDZ * fDZ;
                    nAnimation = !(fDistSq >= GrenadeThrowThreshold * GrenadeThrowThreshold)
                                     ? SERVER_ANIMATION_THROW_LOW : SERVER_ANIMATION_THROW_HIGH;
                    nDelay = !(fDistSq >= GrenadeThrowThreshold * GrenadeThrowThreshold)
                                 ? 700 : 800;
                }
            } else {
                nAnimation = SERVER_ANIMATION_READY;
                nDelay = 800;
            }
            break;
#endif
        case ITEM_TYPE_DROID_UTILITY_DEVICE: {
            CSWCCreature* pClientCreature = GetClientCreature();
            nPauseTime = 1500;
            nDelay = 300;
            if (pClientCreature != 0)
                nDelay =
                        static_cast<unsigned int>(static_cast<CSWCObject*>(pClientCreature)->GetAnimationLength(SERVER_ANIMATION_DROID_UTILITY_CONJURE))
                         - 50;
            bThrownItem = 0;
            nAnimation = SERVER_ANIMATION_DROID_UTILITY_CONJURE + (nElapsedTod >= static_cast<unsigned int>(nDelay));
            break;
        }
        case ITEM_TYPE_FOREARM_BANDS:
            nAnimation = (bHumanoidModel == 0) ? SERVER_ANIMATION_READY : SERVER_ANIMATION_ACTIVATE_ITEM;
            nPauseTime = 1000;
            nDelay = 600;
            bThrownItem = 0;
            break;
        case ITEM_TYPE_STIMULANT:
        case ITEM_TYPE_DROID_REPAIR_EQUIPMENT:
        case ITEM_TYPE_MEDICAL_EQUIPMENT:
            nPauseTime = 1500;
            nDelay = 750;
            bThrownItem = 0;
#if KOTOR1  // K1: always the self-use animation.
            nAnimation = (bHumanoidModel == 0) ? SERVER_ANIMATION_READY : SERVER_ANIMATION_INJECT;
#else
            nAnimation = (bHumanoidModel == 0)
                             ? SERVER_ANIMATION_READY
                             : ((pTargetCreature == this) ? SERVER_ANIMATION_INJECT : SERVER_ANIMATION_THROW_SABER_LOOP);
#endif
            break;
        case ITEM_TYPE_SQUAD_RECOVERY_KIT:
            nAnimation = (bHumanoidModel == 0) ? SERVER_ANIMATION_READY : SERVER_ANIMATION_ACTIVATE_ITEM;
            nPauseTime = 1500;
            nDelay = 750;
            bThrownItem = 0;
            break;
        default:
            nPauseTime = pSpell->m_nCastPauseMS + 1;
            nDelay = 1;
            nAnimation = SERVER_ANIMATION_CAST_1;
            bThrownItem = 0;
            break;
        }

        if (m_pCombatRound->m_bRoundStarted == 0) {
            int bEngaged = 0;
            int bMaster;
            if (bThrownItem != 0) {
                bEngaged = 1;
                bMaster = 1;
            } else {
                bEngaged = m_pCombatRound->ResolveEngagedState(oidTarget);
                bMaster = m_pCombatRound->ResolveMasterState(oidTarget, bEngaged);
            }
            m_pCombatRound->StartCombatRound(oidTarget, bEngaged, bMaster, 0, 0);
            m_pCombatRound->SetSpellCastRound(0, 0);
            m_pCombatRound->SetRoundPaused(1, m_objectId);
            m_pCombatRound->SetPauseTimer(nPauseTime, 0);
            m_pCombatRound->DecrementRoundLength(nPauseTime, 0);
        }

        if (m_bItemCastStarted == 0) {
            // Both operands test STIMULANT. Which item type the second meant is unknown, so it is left
            // as the original has it.
            if (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT
                || static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT) {
                if (m_bPlayerCharacter == 0
                    || g_pAppManager->m_pServerExoApp->GetDifficultyOption(2) == 0)
                    BroadcastAttackOfOpportunity(OBJECT_INVALID);
            }
            m_bItemCastStarted = 1;
        }

        if (nElapsedTod < static_cast<unsigned int>(nDelay)) {
            if (pItem->GetUsedActivePropertyUsesLeft(static_cast<unsigned char>(nProperty)) != 0
                || pProperty->m_costTableValue == IPRP_CHARGECOST_0_CHARGES_PER_USE
                || pProperty->m_costTableValue == IPRP_CHARGECOST_UNLIMITED_USE) {
                m_castSpellId = nSpellId;
                m_castTargetId = oidTarget;
                m_castPosition = vTarget;
                m_bCastBroadcastProjectile = 0;
                m_lastSpellItemId = oidItem;
                m_bItemCastSpell = 0;
                CalculateLastSpellProjectileTime(0);
                m_nItemCastSpellLevel = m_pStats->GetClassLevel(m_pStats->m_numClasses - 1, 0);
                if (pTargetCreature != 0
                    && (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT
                        || static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT
                        || static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT))
                    pTargetCreature->SetAnimation(static_cast<unsigned short>(nAnimation));
                else
                    SetAnimation(static_cast<unsigned short>(nAnimation));
                return CSWSObject::ACTION_IN_PROGRESS;
            }
            SetAnimation(GetPauseReadyAnimation());
            m_bItemCastStarted = 0;
            RemoveSpellActionFromRound();
            ProcessPendingCombatActions();
            SendFeedbackMessage(FEEDBACK_CANNOT_USE_ITEM, 0);
            return CSWSObject::ACTION_FAILED;
        }

        if (m_bCastBroadcastProjectile == 0) {
            m_castSpellId = nSpellId;
            m_castTargetId = oidTarget;
            m_castPosition = vTarget;
            m_bCastBroadcastProjectile = 0;
            m_lastSpellItemId = oidItem;
            m_bItemCastSpell = 1;
            CalculateLastSpellProjectileTime(0);
            m_nItemCastSpellLevel = m_pStats->GetClassLevel(m_pStats->m_numClasses - 1, 0);
            if (pTargetCreature != 0
                && (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT
                    || static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT
                    || static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT))
                pTargetCreature->SetAnimation(static_cast<unsigned short>(nAnimation));
            else
                SetAnimation(static_cast<unsigned short>(nAnimation));

            m_pCombatRound->m_nCastsThisRound += 1;

#if KOTOR1  // K1: one impact call at the target.
            SpellCastAndImpact(nSpellId, vTarget, oidTarget, CSWSObject::CAST_CLASS_NONE, oidItem, 0, 0, 0, 0);
#else
            if (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_GRENADE
                && nSpellId <= SPELL_GRENADE_LAUNCHED_FRAGMENTATION - 1)
                SpellCastAndImpact(nSpellId, vTarget, OBJECT_INVALID, CSWSObject::CAST_CLASS_NONE, oidItem, 0, 0, 0, 0);
            else
                SpellCastAndImpact(nSpellId, vTarget, oidTarget, CSWSObject::CAST_CLASS_NONE, oidItem, 0, 0, 0, 0);
#endif

            if (m_pCombatRound->GetSpellActionPending() == 1)
                m_pCombatRound->RemoveSpellAction();

#if KOTOR1  // K1: no unlimited-uses client option.
            pItem->DecrementUses(nProperty, m_objectId);
#else
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bUnlimitedItemUses == 0)
                pItem->DecrementUses(nProperty, m_objectId);
#endif

            if (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_DROID_UTILITY_DEVICE) {
                pAction->m_args.m_bItemCastHasAnim = 1;
                pAction->m_args.m_nItemCastAnimation = SERVER_ANIMATION_DROID_UTILITY_CAST;
                pAction->m_args.m_nItemCastAnimEnd = nPauseTime;
                pAction->m_nNumParameters = 12;
            }
        }

        if (nElapsedTod < static_cast<unsigned int>(nPauseTime))
            return CSWSObject::ACTION_IN_PROGRESS;

        SetAnimation(GetPauseReadyAnimation());
        m_bItemCastStarted = 0;
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_COMPLETE;
    }

castFailed:
    SetAnimation(GetPauseReadyAnimation());
    m_bItemCastStarted = 0;
    RemoveSpellActionFromRound();
    ProcessPendingCombatActions();
    return CSWSObject::ACTION_FAILED;
}

// EFFECTIVE: the orig's pads and frame in both games; the rest is one spill pair, a tbnz order and an
// unshared ACTION_COMPLETE return tail (K2 +12 B, K1 +4 B).
int CSWSCreature::AIActionCastSpell(CSWSObjectActionNode* pAction)
{
    Vector vTarget = {};
    CExoString sMessage;
    unsigned int nNowDay;
    unsigned int nNowTod;
    unsigned int nElapsedDay;
    unsigned int nElapsedTod;
    unsigned int nSpellId;
    int nClass;
    int nDomainLevel;
    int nMetaMagic;
    int bSkipCharge;
    unsigned long long oidTarget;
    unsigned int nFlags;
    int nFeatId;
    int nReadyKind;
    int nSign;
    int nAttackCast;
    int nAttackBit;
    unsigned char nPathCount;
    CSWSpell* pSpell;
    CGameObject* pObj;
    CSWSCreature* pBlocker;
    int nConjureTime;
    int nCastTime;
    int nRecovery;
    int nPause;
    unsigned short nConjureAnim;

    if (m_bInParty != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        int nIndex = pClient->GetSWParty()->GetIndex(
            pClient->ServerToClientObjectId(m_objectId));
        if (nIndex >= 0) {
            CSWParty* pParty = pClient->GetSWParty();
#if KOTOR1  // K1: strref 48208
            pParty->m_members[nIndex].SetCombatMessage(STRREF_USE_LIGHTSABER);
#else
            pParty->m_members[nIndex].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
#endif
        }
    }

    m_nEndRoundScriptTimer = 3000;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp().
    if (GetDead() == 1 || GetIsPCDying() == 1 || GetDeadTemp() == 1) {
#else
    if (GetDead() == 1 || GetDeadTemp() == 1) {
#endif
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }

    nSpellId = pAction->m_args.m_spellId;
    nClass = pAction->m_args.m_castClassIndex;
    nDomainLevel = pAction->m_args.m_castDomainLevel;
    nMetaMagic = pAction->m_args.m_castMetaMagic;
    bSkipCharge = pAction->m_args.m_bCastSkipReadyCount;
    oidTarget = pAction->m_args.m_castTargetId;
    vTarget = Vector(pAction->m_args.m_castPointX, pAction->m_args.m_castPointY,
                     pAction->m_args.m_castPointZ);
    nFlags = pAction->m_args.m_castFlags;
    nFeatId = pAction->m_args.m_castFeatId;
    nReadyKind = pAction->m_args.m_castReadyKind;

    nSign = nFlags >> 31;
    nAttackCast = nFlags & CAST_FLAG_ATTACK;
    nAttackBit = nAttackCast >> 30;
    nPathCount = (nFlags & 0xff) != 0 ? 7 : 6;

    if (oidTarget == OBJECT_INVALID) {
        Vector vDir = vTarget - m_position;
        if (vDir.x != 0.0f || vDir.y != 0.0f || vDir.z != 0.0f) {
            vDir.Normalize();
            SetOrientation(vDir);
        }
    } else {
        SetLockOrientationToObject(oidTarget, 0);
    }

    pSpell = g_pRules->GetSpell(nSpellId);
    if (pSpell == 0) {
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }
    unsigned int nForceItemMask = GetForceItemMask();
    if ((pSpell->m_nForbidItemMask & nForceItemMask) != 0) {
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }
    if ((pSpell->m_nRequireItemMask & nForceItemMask) != pSpell->m_nRequireItemMask) {
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }

    if (nFeatId != -1 && m_pStats->GetFeatRemainingUses(static_cast<unsigned short>(nFeatId)) == 0) {
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }

    m_oidSpellTarget = (oidTarget == OBJECT_INVALID) ? m_cachedAreaId : oidTarget;

    pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget));
    if (oidTarget != OBJECT_INVALID && pObj == 0) {
        RemoveSpellActionFromRound();
        ProcessPendingCombatActions();
        return CSWSObject::ACTION_FAILED;
    }
    if (pObj != 0 && pObj->AsSWSCreature() != 0) {
        if (pObj->AsSWSCreature()->GetDead() != 0)
            goto castBlocked;
        pBlocker = pObj->AsSWSCreature();
        if (pBlocker->GetDeadTemp() != 0) {
castBlocked:
            RemoveSpellActionFromRound();
            ProcessPendingCombatActions();
            return CSWSObject::ACTION_FAILED;
        }
    }

#if KOTOR1  // K1: no activity-suppress spell set, no in-game GUI gate.
    if (static_cast<int>(nFlags) >= 0)
        ActivityManager(1);
#else
    int bSuppressActivity = 0;
    switch (nSpellId) {
    case FORCE_POWER_MIND_TRICK:
    case FORCE_POWER_BEAST_TRICK:
    case FORCE_POWER_BEAST_CONFUSION:
    case FORCE_POWER_CONFUSION:
    case FORCE_POWER_DROID_TRICK:
    case FORCE_POWER_DROID_CONFUSION:
        if (GetActivity(ACTIVITY_STEALTH) != 0)
            m_nLockedActivityFlags |= 1;
        bSuppressActivity = 1;
        break;
    }

    if (static_cast<int>(nFlags) >= 0) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0)
            ActivityManager(1);
    }
    if (bSuppressActivity != 0 && (m_nLockedActivityFlags & 1) != 0)
        m_nLockedActivityFlags &= ~1u;
#endif

    if (nAttackCast != 0) {
        m_pCombatRound->StartCombatRound(oidTarget, 0, 0, 0, 0);
        m_pCombatRound->SetRoundPaused(1, OBJECT_INVALID);
        m_pCombatRound->SetPauseTimer(1500, 0);
        SetAnimation(SERVER_ANIMATION_ATTACK);
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_nFeatId = FEAT_POWER_ATTACK;
        ResolveAttack(oidTarget, 1, 1500, 0, 0, 4, 0);
        m_bCastBroadcastProjectile = 0;
        Vector vZero = {};
        SpellCastAndImpact(nSpellId, vZero, oidTarget, static_cast<unsigned char>(nClass),
                           OBJECT_INVALID, 0, 0, 0, 0);
        m_bCastBroadcastProjectile = 0;
        return CSWSObject::ACTION_COMPLETE;
    }

    nConjureTime = pSpell->m_nConjureTimeMS;
    nCastTime = pSpell->m_nCastPauseMS;
    nRecovery = pSpell->m_nCastRecoveryMS;

    if (m_pCombatRound->m_bRoundStarted != 0) {
        if (m_pCombatRound->m_bRoundPaused == 0 && m_pCombatRound->m_nCastsThisRound <= 0) {
            m_pCombatRound->SetSpellCastRound(nSpellId, nSign);
            m_pCombatRound->SetRoundPaused(1, m_objectId);
            nPause = nCastTime + nConjureTime + nRecovery;
            m_pCombatRound->SetPauseTimer(nPause, 0);
            m_pCombatRound->DecrementRoundLength(nPause, 0);
            if (pObj != 0 && m_pCombatRound->m_bEngaged != 0 && pObj->AsSWSCreature() != 0) {
                CSWSCreature* pTargetCreature = pObj->AsSWSCreature();
                pTargetCreature->m_pCombatRound->SetRoundPaused(1, pTargetCreature->m_objectId);
                pTargetCreature->m_pCombatRound->SetPauseTimer(nPause, 0);
                pTargetCreature->m_pCombatRound->DecrementRoundLength(nPause, 0);
            }
        }
    } else {
        int nEngaged = m_pCombatRound->ResolveEngagedState(oidTarget);
        int nMaster  = m_pCombatRound->ResolveMasterState(oidTarget, nEngaged);
        m_pCombatRound->StartCombatRound(oidTarget, nEngaged, nMaster, 0, 0);
        m_pCombatRound->SetSpellCastRound(nSpellId, nSign);
        m_pCombatRound->SetRoundPaused(1, m_objectId);
        nPause = nCastTime + nConjureTime + nRecovery;
        m_pCombatRound->SetPauseTimer(nPause, 0);
        m_pCombatRound->DecrementRoundLength(nPause, 0);
        if (pObj != 0 && nEngaged != 0 && pObj->AsSWSCreature() != 0 &&
            static_cast<int>(nFlags) >= 0 && m_objectId != oidTarget) {
            CSWSCreature* pTargetCreature = pObj->AsSWSCreature();
            pTargetCreature->m_pCombatRound->StartCombatRound(
                m_objectId, nEngaged, nMaster == 0, 0, 0);
            pTargetCreature->m_pCombatRound->SetRoundPaused(1, pTargetCreature->m_objectId);
            pTargetCreature->m_pCombatRound->SetPauseTimer(nPause, 0);
            pTargetCreature->m_pCombatRound->DecrementRoundLength(nPause, 0);
        }
        if (nSign == 0)
            BroadcastSpellCast(nSpellId, nMetaMagic, static_cast<unsigned char>(nClass),
                               static_cast<unsigned short>(nFeatId), oidTarget);
    }

    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nNowDay, &nNowTod);
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->SubtractWorldTimes(
        nNowDay, nNowTod, m_nActionStartDay, m_nActionStartTod, &nElapsedDay, &nElapsedTod);

    if (nElapsedTod == 0) {
        m_castSpellId = nSpellId;
        m_nCastDomainLevel = static_cast<unsigned char>(nDomainLevel);
        m_castTargetId = oidTarget;
        m_castPosition = vTarget;
        m_bCastBroadcastProjectile = 0;
#if !KOTOR1  // K1: no remembered force-point cost
        m_nLastSpellForcePointCost = 0;
#endif
    }

    if (nElapsedTod < static_cast<unsigned int>(nConjureTime)) {
        nConjureAnim = pSpell->m_nConjureAnimationKind;
        if (nConjureAnim == CSWSpell::CAST_ANIMATION_SELF)
            SetAnimation(SERVER_ANIMATION_CONJURE_1);
        else if (nConjureAnim == CSWSpell::CAST_ANIMATION_DARK)
            SetAnimation(SERVER_ANIMATION_CONJURE_2);
        else if (nConjureAnim == CSWSpell::CAST_ANIMATION_UP)
            SetAnimation(SERVER_ANIMATION_CONJURE_3);
        else if (nConjureAnim == CSWSpell::CAST_ANIMATION_THROW)
            SetAnimation(SERVER_ANIMATION_THROW_SABER);
        else
            SetAnimation(SERVER_ANIMATION_CONJURE_1);
        return CSWSObject::ACTION_IN_PROGRESS;
    }

    if (pAction->m_bInterruptable == 1) {
        pAction->m_bInterruptable = (m_pCombatRound->m_bEngaged != 0);
        if (nSign == 0) {
            int i;
            if (GetArea() == 0) {
                RemoveSpellActionFromRound();
                ProcessPendingCombatActions();
                return CSWSObject::ACTION_FAILED;
            }
            if (nClass != CSWSObject::CAST_CLASS_NONE && m_bSpellChargeSpent == 0) {
                if (bSkipCharge != 0) {
                    // The level the skipped charge would have cost; nothing reads it.
                    int nSpellLevel = nDomainLevel;
                    if (nDomainLevel == 0)
                        nSpellLevel = pSpell->GetSpellLevel(
                            m_pStats->GetClass(static_cast<unsigned char>(nClass)));
                    nSpellLevel += g_pRules->GetMetaMagicLevelCost(static_cast<unsigned char>(nMetaMagic));
                    K2_UNUSED(nSpellLevel);
                } else {
                    if (DecrementSpellReadyCount(nSpellId, static_cast<unsigned char>(nClass),
                                                 static_cast<unsigned char>(nDomainLevel),
                                                 static_cast<unsigned char>(nMetaMagic),
                                                 static_cast<unsigned char>(nReadyKind)) == 0) {
                        RemoveSpellActionFromRound();
                        ProcessPendingCombatActions();
                        return CSWSObject::ACTION_FAILED;
                    }
                }
                m_bSpellChargeSpent = 1;
            }
            for (i = 0; i < m_effects.m_count; i++) {
                if (m_effects.m_pArray[i]->m_subType ==
                        CSWSEffectListHandler::EFFECT_ENTANGLE) {
                    SetAnimation(SERVER_ANIMATION_READY);
                    SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
                    m_bSpellChargeSpent = 0;
#if KOTOR1  // K1: the feedback is unconditional.
                    SendFeedbackMessage(FEEDBACK_ENTANGLED_CANNOT_CAST, 0);
#else
                    if (nAttackBit == 0)
                        SendFeedbackMessage(FEEDBACK_ENTANGLED_CANNOT_CAST, 0);
#endif
                    RemoveSpellActionFromRound();
                    ProcessPendingCombatActions();
                    return CSWSObject::ACTION_FAILED;
                }
            }
        }
        m_nCastClassIndex = static_cast<unsigned char>(nClass);
        m_lastSpellItemId = CGameObjectArray::INVALIDOBJID;
        m_bCastSkipCharge = bSkipCharge;
        m_nCastMetaMagic = static_cast<unsigned char>(nMetaMagic);
        CalculateLastSpellProjectileTime(static_cast<unsigned char>(nFlags));
    }

    if (nElapsedTod >= static_cast<unsigned int>(nCastTime + nConjureTime) &&
        m_bCastBroadcastProjectile != 0) {
        if (nElapsedTod >= static_cast<unsigned int>(nCastTime + nConjureTime + nRecovery)) {
            SetAnimation(GetPauseReadyAnimation());
            SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
            m_bSpellChargeSpent = 0;
            if (nFeatId != -1)
                m_pStats->DecrementFeatRemainingUses(static_cast<unsigned short>(nFeatId));
            RemoveSpellActionFromRound();
            ProcessPendingCombatActions();
            return CSWSObject::ACTION_COMPLETE;
        }
        if (pSpell->m_nCatchAnimation == 0)
            SetAnimation(SERVER_ANIMATION_CATCH_SABER);
        m_bSpellChargeSpent = 0;
        ProcessPendingCombatActions();
        RemoveSpellActionFromRound();
        return CSWSObject::ACTION_FAILED;
    }

#if KOTOR1  // K1: no cast animations 10..12.
    switch (pSpell->m_nCastAnimationKind) {
    case CSWSpell::CAST_ANIMATION_SELF:
        SetAnimation(SERVER_ANIMATION_CAST_1);
        break;
    case CSWSpell::CAST_ANIMATION_DARK:
        SetAnimation(SERVER_ANIMATION_CAST_2);
        break;
    case CSWSpell::CAST_ANIMATION_UP:
        SetAnimation(SERVER_ANIMATION_CAST_3);
        break;
    case CSWSpell::CAST_ANIMATION_AREA:
        SetAnimation(SERVER_ANIMATION_CAST_4);
        break;
    case CSWSpell::CAST_ANIMATION_THROW:
        SetAnimation(SERVER_ANIMATION_THROW_SABER_LOOP);
        break;
    case CSWSpell::CAST_ANIMATION_JUMP:
    case CSWSpell::CAST_ANIMATION_MONSTER:
        break;
    default:
        SetAnimation(SERVER_ANIMATION_THROW_SABER_LOOP);
        break;
    }
#else
    switch (pSpell->m_nCastAnimationKind) {
    case CSWSpell::CAST_ANIMATION_SELF:
        SetAnimation(SERVER_ANIMATION_CAST_1);
        break;
    case CSWSpell::CAST_ANIMATION_DARK:
        SetAnimation(SERVER_ANIMATION_CAST_2);
        break;
    case CSWSpell::CAST_ANIMATION_UP:
        SetAnimation(SERVER_ANIMATION_CAST_3);
        break;
    case CSWSpell::CAST_ANIMATION_AREA:
        SetAnimation(SERVER_ANIMATION_CAST_4);
        break;
    case CSWSpell::CAST_ANIMATION_THROW:
        SetAnimation(SERVER_ANIMATION_THROW_SABER_LOOP);
        break;
    case CSWSpell::CAST_ANIMATION_JUMP:
    case CSWSpell::CAST_ANIMATION_MONSTER:
        break;
    case CSWSpell::CAST_ANIMATION_FURY:
        SetAnimation(SERVER_ANIMATION_FORCE_RAGE);
        break;
    case CSWSpell::CAST_ANIMATION_MONSTER_FURY:
        // Not a server id: the client animation itself, which ServerToClientAnimation passes
        // through unmapped.
        SetAnimation(CSWCAnimBase::CLIENT_ANIMATION_MONSTER_READY_FIDGET);
        break;
    default:
        if (pSpell->m_nCastAnimationKind == CSWSpell::CAST_ANIMATION_CRUSH)
            SetAnimation(SERVER_ANIMATION_FORCE_CRUSH);
        else
            SetAnimation(SERVER_ANIMATION_THROW_SABER_LOOP);
        break;
    }
#endif

    if (m_bCastBroadcastProjectile == 0) {
        if (static_cast<int>(nFlags) >= 0) {
            m_pCombatRound->m_nCastsThisRound++;
            SpellCastAndImpact(nSpellId, vTarget, oidTarget, static_cast<unsigned char>(nClass),
                               OBJECT_INVALID, 0, 0, static_cast<unsigned char>(nFlags), nAttackBit);
        } else {
            CSWSObject* pTarget = 0;
            CGameObject* pProjTarget =
                static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget));
            if (pProjTarget != 0)
                pTarget = pProjTarget->AsSWSObject();
            if (pTarget != 0)
                BroadcastSafeProjectile(m_objectId, oidTarget, m_position,
                    pTarget->m_position, m_castLevel, nPathCount, nSpellId, 0,
                    static_cast<unsigned char>(nFlags), 0, 0);
            else
                BroadcastSafeProjectile(m_objectId, oidTarget, m_position, m_castPosition,
                    m_castLevel, nPathCount, nSpellId, 0, static_cast<unsigned char>(nFlags), 0, 0);
            m_bCastBroadcastProjectile = 1;
        }
    }
    return CSWSObject::ACTION_IN_PROGRESS;
}



const char* Draw_Marker(char* pszArgs) {
    float fX = 0.0f, fY = 0.0f;
    if (sscanf(pszArgs, "%f %f", &fX, &fY) != 2)
        return "usage: Draw_Marker X Y";

    Vector v(fX, fY, 0.0f);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef, v, 1);
    return " ";
}

const char* Draw_Line(char* pszArgs) {
    float fX1 = 0.0f, fY1 = 0.0f, fX2 = 0.0f, fY2 = 0.0f;
    if (sscanf(pszArgs, "%f %f %f %f", &fX1, &fY1, &fX2, &fY2) != 4)
        return "usage: Draw_Line X Y";

    Vector vFrom(fX1, fY1, 0.0f);
    Vector vTo(fX2, fY2, 0.0f);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    vFrom.z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef, vFrom, 1);
    pApp = g_pAppManager->m_pServerExoApp;
    pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef, vTo, 1);
    return " ";
}

char sRet[255] = "                                                  ";

static int SRetPrintf(int nUnused1, int nUnused2, const char* pszFmt, ...) {
    K2_UNUSED(nUnused1); K2_UNUSED(nUnused2);
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nRet = __vsprintf_chk(sRet, 0, 255, pszFmt, ap);
    __builtin_va_end(ap);
    return nRet;
}

// Debug console handler ("TestLocationIsSafe X Y ObjectId"): height-snaps the point, then asks
// the area whether the creature's pathfind info likes it. Answers "Good"/"Invalid Location".
char* TestLocationIsSafe(char* pszArgs) {
    float fX = 0.0f, fY = 0.0f;
    unsigned long long objectId = OBJECT_INVALID;
#ifdef K2_FIX_ENGINE_BUGS
    if (sscanf(pszArgs, "%f %f %llu", &fX, &fY, &objectId) != 3)
#else
    if (sscanf(pszArgs, "%f %f %d", &fX, &fY, &objectId) != 3)
#endif
        return const_cast<char*>("usage: TestLocationIsSafe X Y ObjectId");
    if (objectId == OBJECT_INVALID)
        return const_cast<char*>("usage: TestLocationIsSafe X Y ObjectId");

    Vector v(fX, fY, 0.0f);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    v.z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef, v, 1);

    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != NULL &&
        g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->TestSafeLocationPoint(
            v, pCreature->m_pPathfindInfo)) {
        SRetPrintf(0, 0, "Good");
    } else {
        SRetPrintf(0, 0, "Invalid Location");
    }
    return sRet;
}

char* LocateNearestPathPoint(char* pszArgs) {
    float fX = 0.0f, fY = 0.0f;
    unsigned long long objectId = OBJECT_INVALID;
#ifdef K2_FIX_ENGINE_BUGS
    if (sscanf(pszArgs, "%f %f %llu", &fX, &fY, &objectId) != 3)
#else
    if (sscanf(pszArgs, "%f %f %d", &fX, &fY, &objectId) != 3)
#endif
        return const_cast<char*>("usage: LocateNearestPathPoint X Y ObjectId");
    if (objectId == OBJECT_INVALID)
        return const_cast<char*>("usage: LocateNearestPathPoint X Y ObjectId");

    Vector v(fX, fY, 0.0f);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    v.z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef, v, 1);

    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);

    unsigned int nPoint = 0xffffffff;
    if (g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->ClosestPathPoint(v, nPoint)) {
        SRetPrintf(0, 0, "Good %d", nPoint);
        CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        Vector vPoint = pArea->m_paPathPoints[static_cast<int>(nPoint)].m_point;
        vPoint.z = 0.0f;
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        pServer->ComputeHeight(pServer->GetModule()->GetArea()->m_resRef, vPoint, 1);
    } else {
        SRetPrintf(0, 0, "Invalid Location");
    }
    return sRet;
}

const char* ValidateCreaturePath(char* pszArgs) {
    unsigned long long objectId = OBJECT_INVALID;
#ifdef K2_FIX_ENGINE_BUGS
    int nConverted = sscanf(pszArgs, "%llu", &objectId);
#else
    int nConverted = sscanf(pszArgs, "%d", &objectId);
#endif
    if (objectId == 99999)
        objectId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();

    const char* pszResult = "usage: LocateNearestPathPoint ObjectId";
    if (nConverted == 1 && objectId != OBJECT_INVALID) {
        g_pAppManager->m_pServerExoApp->DebugValidatePath(objectId, 0);
        pszResult = "done";
    }
    return pszResult;
}
