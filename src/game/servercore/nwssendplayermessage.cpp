#include "k2stdint.h"
#include "strref.h"
#include "nwssendplayermessage.h"

#include "appmanager.h"
#include "Vector.h"
#include "exolinkedlistinternal.h"
#include "CExoLinkedList.h"
#include "gameobjectarray.h"
#include "netlayer.h"
#include "serverexoapp.h"
#include "serverinfo.h"
#include "nwsplayerluo.h"
#include "nwitem.h"
#include "nwsrules.h"
#include "nwsspellimpact.h"
#include "nwsitem.h"
#include "nwmessage.h"
#include "nwscreature.h"
#include "pathfindinfo.h"
#include "nwscreaturestats.h"
#include "nwcreaturelevelstats.h"
#include "nwupdatestats.h"
#include "nwclass.h"
#include "nwspell.h"
#include "nwrules.h"
#include "nwstrigger.h"
#include "nwsdoor.h"
#include "guiingame.h"
#include "partytables.h"
#include "exobase.h"
#include "exotimersinternal.h"
#include "nwsscriptevent.h"
#include "serveranimation.h"
#include "serveraimaster.h"
#include "nwsmodule.h"
#include "nwsobject.h"
#include "nwsplaceable.h"
#include "serverexoappinternal.h"
#include "factionmanager.h"
#include "nwsparty.h"
#include "nwsarea.h"
#include "nwsplayer.h"
#include "nwsjournal.h"
#include "exotlktable.h"
#include "resgff.h"
#include "exoresman.h"
#include "exofile.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwsencounter.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "nwscript.h"
#include "exolocstring.h"

#include <string.h>

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcmath.h"
extern int g_bEnableCombatDebugging;
extern int g_bEnableSavingThrowDebugging;
extern int g_bEnableMovementSpeedDebugging;

#include "CExoArrayListImpl.h"
#include "nwmessagetypes.h"
#include "feedbackmessage.h"
#include "nwsaiaction.h"
#include "nwscriptconst.h"
#include "exostringinline.h"

// Resource types (CExoResMan, CExoFile and the directory lister).
enum {
    kResTypeIfo = 2014,
    kResTypeBic = 2015,
    kResTypeAll = 0xffff,   // every type
};

// The armor range of baseitems.2da's itemtype column (CSWBaseItem::m_nItemType).
enum {
    kItemTypeFirstArmor = 31,
    kItemTypeLastArmor  = 38,
};

// The highest reputation that is not yet friendly (friendly starts at 90).
enum { kReputationMaxNeutral = 89 };

// SelectCategoryForGameObject's answer for an object the update never sends.
enum { kUpdateCategoryNone = 0xff };

// The per-class field mask of the known-spell-uses panel.
enum { kClassFieldSpellUsesLeft = 1 << 0 };

// CSWVisibilityNode::m_flags.
enum {
    kVisibleSeen     = 1 << 0,
    kVisibleHeard    = 1 << 1,
    kVisibleInactive = 1 << 4,
};

int equal(const Vector& a, const Vector& b);

// Answers true when the locstring has text in a language the server would ship, trying the player's
// language, then language 0, then the module language, then the string table.
int CSWSMessage::HasValidString(CExoLocString& s, unsigned char nLang)
{
    CExoString sText;
    if (g_pAppManager->m_pServerExoApp == 0)
        return 0;

    int nPlayerLanguage = g_pAppManager->m_pServerExoApp->GetPlayerLanguage(m_nPlayerId);
    unsigned char bReference;
    int nModuleLanguage = g_pAppManager->m_pServerExoApp->GetModuleLanguage();
    if (s.GetStringLoc(nPlayerLanguage, &sText, nLang) ||
        (nPlayerLanguage != 0 && s.GetStringLoc(0, &sText, nLang)) ||
        (nModuleLanguage != 0 && nModuleLanguage != nPlayerLanguage &&
         s.GetStringLoc(nModuleLanguage, &sText, nLang)) ||
        s.GetString(0, &nPlayerLanguage, &sText, &bReference) ||
        s.m_nStrRef == 0xFFFFFFFF)
    {
        return sText.GetLength() > 0;
    }
    return 1;
}

// A static door sends only its appearance row; a non-static one also sends the generic-door type.
void CSWSMessage::AddDoorAppearanceToMessage(CSWSDoor* pDoor)
{
    WriteBOOL(pDoor->m_bStatic);
    WriteBYTE(pDoor->m_appearance, 8);

    if (pDoor->m_appearance == 0)
        WriteBYTE(pDoor->m_genericType, 8);
}

// The subject is the player's own creature unless the in-game GUI is showing a party
// member's sheet, in which case that creature is diffed against the viewing player's snapshot.
int CSWSMessage::ComputeInventoryUpdateRequired(CSWSPlayer* pPlayer, unsigned int nSlot)
{
    if (pPlayer == 0)
        return 0;

    if (pPlayer->GetGameObject() == 0)
        return 0;

    CGameObject* pObject;
#if KOTOR1  // K1: always the player's own creature
    pObject = pPlayer->GetGameObject();
#else
    int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
    if (nOverride != -1)
    {
        unsigned long long nSubjectId =
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nOverride, 0, 1);
        pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nSubjectId);
    }
    else
    {
        pObject = pPlayer->GetGameObject();
    }
#endif

    if (pObject->AsSWSCreature() == 0)
        return 0;

    if (pObject->AsSWSCreature()->m_pInventory == 0)
        return 0;

    CSWSItem* pItem =
        static_cast<CSWSItem*>(pObject->AsSWSCreature()->m_pInventory->GetItemInSlot(1 << nSlot));

    if (pItem != 0)
    {
        unsigned long long nItemId = pItem->m_objectId;
        if (pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[nSlot] != nItemId)
            return 1;

        int nSnapCount = pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemStackCount[nSlot];
        return pItem->m_stackCount != nSnapCount;
    }

    if (pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[nSlot]
        != CGameObjectArray::INVALIDOBJID)
        return 1;

    return 0;
}

// Overwrites the destination list with the source's effects. Unlike AssignCreatureLists, the
// destination is the first argument.
void CSWSMessage::AssignVisualEffectLists(CExoArrayList<CLoopingVisualEffect*>* pDst,
                                          CExoArrayList<CLoopingVisualEffect*>* pSrc)
{
    for (int i = 0; i < pDst->m_count; i++)
    {
        delete pDst->m_pArray[i];
        pDst->m_pArray[i] = 0;
    }
    pDst->SetSize(0);

    for (int i = 0; i < pSrc->m_count; i++)
    {
        int bAlreadyPresent = 0;
        for (int j = 0; j < pDst->m_count; j++)
        {
            if ((pSrc->m_pArray[i]->m_nVisualEffectId ==
                     pDst->m_pArray[j]->m_nVisualEffectId &&
                 pSrc->m_pArray[i]->GetIsBeam() == 0) ||
                (pSrc->m_pArray[i]->m_nVisualEffectId ==
                     pDst->m_pArray[j]->m_nVisualEffectId &&
                 pSrc->m_pArray[i]->m_targetId == pDst->m_pArray[j]->m_targetId &&
                 pSrc->m_pArray[i]->m_nBodyPart == pDst->m_pArray[j]->m_nBodyPart))
                bAlreadyPresent = 1;
        }

        if (bAlreadyPresent == 0)
        {
            CLoopingVisualEffect* pNew = new CLoopingVisualEffect;
            pNew->m_nVisualEffectId = pSrc->m_pArray[i]->m_nVisualEffectId;
            pNew->m_targetId = pSrc->m_pArray[i]->m_targetId;
            pNew->m_nBodyPart = pSrc->m_pArray[i]->m_nBodyPart;

            pDst->AddUnique(pNew);
        }
    }
}

// Tests set equality. Every entry of each list must have a partner in the other - the effect id,
// plus the anchor object for beam effects.
int CSWSMessage::CompareVisualEffectLists(CExoArrayList<CLoopingVisualEffect*>* pA,
                                          CExoArrayList<CLoopingVisualEffect*>* pB)
{
    for (int i = 0; i < pA->m_count; i++)
    {
        CLoopingVisualEffect* pEffect = pA->m_pArray[i];
        unsigned short nVisualEffectId = pEffect->m_nVisualEffectId;
        int bIsBeam = pEffect->GetIsBeam();

        int bFound = 0;
        for (int j = 0; j < pB->m_count; j++)
        {
            CLoopingVisualEffect* pOther = pB->m_pArray[j];
            if (pOther->m_nVisualEffectId == nVisualEffectId &&
                (bIsBeam == 0 || pOther->m_targetId == pEffect->m_targetId))
            {
                bFound = 1;
                break;
            }
        }

        if (!bFound)
            return 0;
    }

    for (int i = 0; i < pB->m_count; i++)
    {
        CLoopingVisualEffect* pEffect = pB->m_pArray[i];
        unsigned short nVisualEffectId = pEffect->m_nVisualEffectId;
        int bIsBeam = pEffect->GetIsBeam();

        int bFound = 0;
        for (int j = 0; j < pA->m_count; j++)
        {
            CLoopingVisualEffect* pOther = pA->m_pArray[j];
            if (pOther->m_nVisualEffectId == nVisualEffectId &&
                (bIsBeam == 0 || pOther->m_targetId == pEffect->m_targetId))
            {
                bFound = 1;
                break;
            }
        }

        if (!bFound)
            return 0;
    }

    return 1;
}

// Overwrites the destination list with the source's contents; the source is the first argument.
void CSWSMessage::AssignCreatureLists(CExoArrayList<unsigned long long>* pSrc,
                                      CExoArrayList<unsigned long long>* pDst)
{
    pDst->Clear();

    for (int i = 0; i < pSrc->m_count; i++)
        pDst->Add(pSrc->m_pArray[i]);
}

// Diffs one object for one client, building the update bitmask that client needs by comparing the
// live object against that player's snapshot. UpdateLastUpdateObject fills every field read here.
// The low bits cover position, orientation, lock target, portrait, animation and the visual
// effects; the trap, trigger, door and placeable state and the creature type gate open the longer
// tails.
int CSWSMessage::ComputeUpdateRequired(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                       CLastUpdateObject* pLast, int bLocalPlayer)
{
    int nFlags = 0;

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;

        pCreature->ComputeAIState();

        // Reputation toward this creature, 2 when the viewing player has no creature.
        unsigned char nRep = 2;
        if (pPlayer && pPlayer->GetGameObject() && pPlayer->GetGameObject()->AsSWSCreature())
            nRep = pPlayer->GetGameObject()->AsSWSCreature()->GetAIStateReputation(pCreature->m_objectId);

        // The appearance and party-state fields.
        if (pLast->m_nAIState != pCreature->m_nAIState ||
            pLast->m_nAIStateAction != pCreature->m_nAIStateAction ||
            pLast->m_nActivityFlags != static_cast<unsigned short>(pCreature->m_activityFlags) ||
            pLast->m_oidAIStateTarget != pCreature->m_oidAIStateTarget ||
            pLast->m_nReputation != nRep ||
            pLast->m_bCombatState != pCreature->m_bCombatState ||
            pLast->m_nStealthMode != pCreature->m_stealthMode)
            nFlags |= MSG_OBJECT_UPDATE_AI_STATE;

        if (CompareVisualEffectLists(&pLast->m_visualEffects, &pCreature->m_visualEffects) == 0)
            nFlags |= MSG_OBJECT_UPDATE_VISUAL_EFFECTS;

        // Animation and playback speed. The flanking/associate bit rides on the same mask and fires
        // only when the chosen animation is one of the two flanking ids; the chosen animation is the
        // live one except on the clean path, where it is the snapshot's.
        int nAnimation = pCreature->m_nAnimation;
        if (pLast->m_fAnimationSpeed != pObject->m_fAnimationSpeed ||
            pLast->m_nAnimation != pCreature->m_nAnimation)
        {
            nFlags |= MSG_OBJECT_UPDATE_ANIMATION;
        }
        else
        {
            nAnimation = pLast->m_nAnimation;
        }
        if (nAnimation == SERVER_ANIMATION_ATTACK_2 || nAnimation == SERVER_ANIMATION_ATTACK)
        {
            if (pCreature->m_oidAttackTarget != OBJECT_INVALID && pCreature->m_oidAttackTarget != pLast->m_oidAttackTarget)
                nFlags |= MSG_OBJECT_UPDATE_ORIENTATION | MSG_OBJECT_UPDATE_ANIMATION;
        }

        // The lock-orientation target.
        if (pCreature->GetLockOrientationToObject() != pLast->m_oidLockOrientationTo)
            nFlags |= MSG_OBJECT_UPDATE_ORIENTATION;

        CPathfindInformation* pPathfindInfo = pCreature->m_pPathfindInfo;

        // The stats block is a per-class walk plus the name, classification, alignment and
        // selectability chain. One dirty-state test owns the bit, so the loop and the chain share
        // it.
        int bClassesDiffer = 0;
        if (pCreature->m_pStats->m_numClasses != 0)
        {
            for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++)
            {
#if KOTOR1  // K1: CSWSCreatureStats::GetClass is out of line
                if (pCreature->m_pStats->GetClass(static_cast<unsigned char>(i)) != reinterpret_cast<unsigned char*>(&pLast->m_nClasses)[i])
#else
                if (((static_cast<unsigned char>(i) < pCreature->m_pStats->m_numClasses)
                         ? pCreature->m_pStats->m_classInfo[i].m_class : CLASS_TYPE_INVALID)
                    != reinterpret_cast<unsigned char*>(&pLast->m_nClasses)[i])
#endif
                {
                    bClassesDiffer = 1;
                    break;
                }
                if (pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0)
                    != reinterpret_cast<unsigned char*>(&pLast->m_nClassLevels)[i])
                {
                    bClassesDiffer = 1;
                    break;
                }
            }
        }
        if (bClassesDiffer != 0 ||
            pLast->m_nRace != pCreature->m_pStats->m_nRace ||
            pCreature->m_pStats->m_sSubrace != pLast->m_sSubrace ||
            pLast->m_nSubraceIndex != pCreature->m_pStats->m_nSubraceIndex ||
            pLast->m_nGender != pCreature->m_pStats->m_nGender ||
            pLast->m_nAlignment != static_cast<unsigned short>(pCreature->m_pStats->m_nAlignment) ||
#if KOTOR1  // K1: no base NPC alignment, stance or tag in the creature update
            pCreature->m_bDeadSelectable != pLast->m_bDeadSelectable)
#else
            pLast->m_nBaseNPCAlignment != static_cast<unsigned short>(pCreature->m_pStats->m_nBaseNPCAlignment) ||
            pCreature->m_bDeadSelectable != pLast->m_bDeadSelectable ||
            pCreature->m_stanceId != pLast->m_nStanceId ||
            pCreature->m_sTag != pLast->m_sTag)
#endif
            nFlags |= MSG_OBJECT_UPDATE_IDENTITY;

        // For movement, a stationary creature is measured against the snapshot position, a moving
        // one against the tail of its plotted path.
        if (pCreature->AnimationStationary(static_cast<unsigned short>(pCreature->m_nAnimation)) == 0)
        {
            unsigned short n = static_cast<unsigned short>(pPathfindInfo->m_nWayPointFloats / 2);
            if (n)
            {
                float fDx = pPathfindInfo->m_paWayPoints[2 * n - 2] - pLast->m_lastPathPoint.x;
                float fDy = pPathfindInfo->m_paWayPoints[2 * n - 1] - pLast->m_lastPathPoint.y;
                float fDz = 0.0f - pLast->m_fLastPathPointZ;
                if (fDx * fDx + fDy * fDy + fDz * fDz >= 0.01f || pPathfindInfo->m_bResendPath != 0)
                {
                    nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION |
                              MSG_OBJECT_UPDATE_ANIMATION;
                    pPathfindInfo->m_bResendPath = 0;
                }
            }
            if (pCreature->m_bDriving == 1)
            {
                float fFacing = pLast->m_fOrientationX * pCreature->m_orientation.x +
                                pLast->m_fOrientationY * pCreature->m_orientation.y +
                                pLast->m_fOrientationZ * pCreature->m_orientation.z;
                if (fFacing < 0.95f)
                    nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;
            }
        }
        else
        {
            if ((nFlags & MSG_OBJECT_UPDATE_ANIMATION) != 0)
            {
                nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;
            }
            else
            {
                float fDx = pLast->m_fPositionX - pCreature->m_position.x;
                float fDy = pLast->m_fPositionY - pCreature->m_position.y;
                float fDz = pLast->m_fPositionZ - pCreature->m_position.z;
                if (fDx * fDx + fDy * fDy + fDz * fDz >= 0.01f)
                    nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;
                else if (pCreature->GetArea() &&
                         static_cast<CSWSArea*>(pCreature->GetArea())->m_objectId != pLast->m_oidArea)
                    nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;
                else
                {
                    float fFacing = pLast->m_fOrientationX * pCreature->m_orientation.x +
                                    pLast->m_fOrientationY * pCreature->m_orientation.y +
                                    pLast->m_fOrientationZ * pCreature->m_orientation.z;
                    if (fFacing < 0.95f)
                        nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;
                }
            }
        }

        // Walk and run rates drifted by more than 10.0.
        if (__builtin_fabsf(pCreature->GetWalkRate() - pLast->m_fWalkRate) > 10.0f ||
            __builtin_fabsf(pCreature->GetRunRate() - pLast->m_fRunRate) > 10.0f)
            nFlags |= MSG_OBJECT_UPDATE_MOVE_RATES;

        // Hit points and force points are gated on the viewer's party. For anyone but the local
        // player, only members of the viewer's faction are checked.
        if (bLocalPlayer != 0 ||
            pPlayer->GetGameObject()->AsSWSCreature()->GetFaction()
                ->GetCreatureInParty(pCreature->m_objectId) != 0)
        {
            if (pLast->m_nHitPoints != static_cast<unsigned short>(pCreature->m_hitPoints) ||
                pLast->m_nCurrentHitPoints != static_cast<unsigned short>(pCreature->GetCurrentHitPoints(1)) ||
                pLast->m_nHitPointBonus != static_cast<unsigned short>(pCreature->m_hitPointBonus) ||
                pLast->m_nMaxHitPoints != static_cast<unsigned short>(pCreature->GetMaxHitPoints(1)))
                nFlags |= MSG_OBJECT_UPDATE_HIT_POINTS;

            if (pLast->m_nBaseForcePoints != pCreature->m_pStats->m_nBaseForcePoints ||
                pLast->m_nForcePoints != pCreature->m_pStats->m_nCurrentForcePoints ||
                pLast->m_nTemporaryForcePoints != pCreature->m_pStats->m_nTemporaryForcePoints ||
                pLast->m_nMaxForcePoints != static_cast<unsigned short>(pCreature->GetMaxForcePoints()))
                nFlags |= MSG_OBJECT_UPDATE_FORCE_POINTS;
        }

        // Faction and standing, run whenever the local player or the server has a client object for
        // this creature.
        CSWSPlayer* pClientObj =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pCreature->m_objectId);
        if (bLocalPlayer != 0 || pClientObj != 0)
        {
            if (pCreature->AsSWSCreature()->m_bPlayerCharacter != pLast->m_bPlayerCharacter ||
                ((pCreature->AsSWSCreature()->m_nAIState >> 6) & 1) != pLast->m_bAIStateControllable ||
                pCreature->GetFaction()->GetSingletonParty() != pLast->m_bSingletonParty ||
                pCreature->AsSWSCreature()->m_bInvitedToParty != pLast->m_bInvitedToParty ||
                (pCreature->AsSWSCreature()->GetFaction()->GetLeader() == pCreature->m_objectId
                    ? 1 : 0) != pLast->m_bFactionLeader)
                nFlags |= MSG_OBJECT_UPDATE_PARTY;
        }

        // In the visibility list, the viewer's own creature gets a party count, everything else the
        // per-node bits.
        if (pPlayer->GetGameObject() != 0)
        {
            CSWSCreature* pPlayerCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pPlayerCreature != 0)
            {
                CSWSCreature* pSubject = pObject->AsSWSCreature();
                if (pSubject != 0)
                {
                    if (pPlayerCreature->m_objectId == pSubject->m_objectId)
                    {
                        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
                        if (pModule)
                        {
                            if (pModule->m_playerList.Contains(pSubject->m_objectId) != pLast->m_bInvisible)
                                nFlags |= MSG_OBJECT_UPDATE_VISIBILITY;
                        }
                    }
                    else
                    {
                        CSWVisibilityNode* pElement =
                            pSubject->GetVisibleListElement(pPlayerCreature->m_objectId);
                        if (pElement)
                        {
                            if (((pElement->m_flags >> 1) & 1) != pLast->m_bHeard ||
                                (pElement->m_flags & 1) != pLast->m_bSeen ||
                                ((pElement->m_flags >> 4) & 1) != pLast->m_bInvisible)
                                nFlags |= MSG_OBJECT_UPDATE_VISIBILITY;
                        }
                        else
                        {
                            if (pLast->m_bHeard == 1 || pLast->m_bSeen == 1 ||
                                pLast->m_bInvisible == 1)
                                nFlags |= MSG_OBJECT_UPDATE_VISIBILITY;
                        }
                    }
                }
            }
        }

        // Damage level and portrait.
        if (pLast->m_nDamageLevel != pCreature->GetDamageLevel())
            nFlags |= MSG_OBJECT_UPDATE_DAMAGE_LEVEL;

        int bPortraitChanged = pLast->m_nPortraitId != pObject->GetPortraitId();
        if (bPortraitChanged == 0 && pLast->m_portrait != pObject->GetPortrait())
            bPortraitChanged = 1;
        if (bPortraitChanged != 0)
            nFlags |= MSG_OBJECT_UPDATE_PORTRAIT;

        // The path-state byte, for the local player only; all three portrait outcomes share this tail.
        if (bLocalPlayer != 0)
        {
            if (pPathfindInfo->m_nClientPathId != static_cast<int>(pLast->m_nClientPathId))
                nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION |
                          MSG_OBJECT_UPDATE_ANIMATION;
        }

        // The object-removal trailer.
        if (pLast->m_bDeleteNoFade != pCreature->m_bDeleteNoFade ||
            pLast->m_nFadeOutTime != pCreature->m_nFadeOutTime ||
            pLast->m_bKeepCorpse != pCreature->m_bKeepCorpse)
            nFlags |= MSG_OBJECT_UPDATE_REMOVAL;
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT)
    {
        if (pLast->m_fAnimationSpeed != pObject->m_fAnimationSpeed ||
            pLast->m_nAnimation != pObject->m_nAnimation)
            nFlags |= MSG_OBJECT_UPDATE_ANIMATION;

        float fDx = pLast->m_fPositionX - pObject->m_position.x;
        float fDy = pLast->m_fPositionY - pObject->m_position.y;
        float fDz = pLast->m_fPositionZ - pObject->m_position.z;
        if (fDx * fDx + fDy * fDy + fDz * fDz >= 0.01f)
            nFlags |= MSG_OBJECT_UPDATE_POSITION;

        float fFacing = pLast->m_fOrientationX * pObject->m_orientation.x +
                        pLast->m_fOrientationY * pObject->m_orientation.y +
                        pLast->m_fOrientationZ * pObject->m_orientation.z;
        if (fFacing < 0.95f)
            nFlags |= MSG_OBJECT_UPDATE_ORIENTATION;

        int bPortraitChanged = pLast->m_nPortraitId != pObject->GetPortraitId();
        if (bPortraitChanged == 0 && pLast->m_portrait != pObject->GetPortrait())
            bPortraitChanged = 1;
        if (bPortraitChanged != 0)
            nFlags |= MSG_OBJECT_UPDATE_PORTRAIT;

        if (pObject->GetArea() &&
            static_cast<CSWSArea*>(pObject->GetArea())->m_objectId != pLast->m_oidArea)
            nFlags |= MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION;

        if (CompareVisualEffectLists(&pLast->m_visualEffects, &pObject->m_visualEffects) == 0)
            nFlags |= MSG_OBJECT_UPDATE_VISUAL_EFFECTS;

        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
        {
            CExoArrayList<unsigned long long>* pDoorList =
                &pObject->AsSWSDoor()->m_objectsAtDoor;
            if (pObject->AsSWSDoor()->m_bLocked != pLast->m_bLocked ||
                pObject->AsSWSDoor()->m_bPlot != pLast->m_bPlot ||
                pObject->AsSWSDoor()->m_bTrapFlag != pLast->m_bTrapFlag ||
                static_cast<unsigned char>(pObject->AsSWSDoor()->m_bMineFlagged) != pLast->m_bMineFlagged ||
                pDoorList->m_count != pLast->m_lstObjectsOnObject.m_count)
                nFlags |= MSG_OBJECT_UPDATE_STATE;
            else
            {
                for (int i = 0; i < static_cast<int>(pLast->m_lstObjectsOnObject.m_count); i++)
                {
                    if (pDoorList->m_pArray[i] != pLast->m_lstObjectsOnObject.m_pArray[i])
                    {
                        nFlags |= MSG_OBJECT_UPDATE_STATE;
                        break;
                    }
                }
            }
        }
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            CExoArrayList<unsigned long long>* pPlaceableList =
                &pObject->AsSWSPlaceable()->m_objectsOnPlaceable;
            if (pObject->AsSWSPlaceable()->m_bLocked != pLast->m_bLocked ||
                pObject->AsSWSPlaceable()->m_bPlot != pLast->m_bPlot ||
                pObject->AsSWSPlaceable()->m_bTrapFlag != pLast->m_bTrapFlag ||
                static_cast<unsigned char>(pObject->AsSWSPlaceable()->m_bMineFlagged) != pLast->m_bMineFlagged ||
                pPlaceableList->m_count != pLast->m_lstObjectsOnObject.m_count)
                nFlags |= MSG_OBJECT_UPDATE_STATE;
            else
            {
                for (int i = 0; i < static_cast<int>(pLast->m_lstObjectsOnObject.m_count); i++)
                {
                    if (pPlaceableList->m_pArray[i] != pLast->m_lstObjectsOnObject.m_pArray[i])
                    {
                        nFlags |= MSG_OBJECT_UPDATE_STATE;
                        break;
                    }
                }
            }
        }
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
    {
        if (pObject->AsSWSTrigger()->m_bTrap)
        {
            if (pObject->AsSWSTrigger()->m_bMineFlagged != pLast->m_bMineFlagged)
                nFlags |= MSG_OBJECT_UPDATE_STATE;
#ifdef K2_FIX_ENGINE_BUGS
            // A mine's spotted/hidden animation otherwise only reaches the client on the first update after
            // the add.
            if (pLast->m_nAnimation != pObject->m_nAnimation)
                nFlags |= MSG_OBJECT_UPDATE_ANIMATION;
#endif

            CSWSCreature* pOwner = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pObject->AsSWSTrigger()->m_creatorId);
            if (pOwner)
            {
                if (pObject->AsSWSTrigger()->m_nFaction != static_cast<unsigned int>(pOwner->m_pStats->m_factionId))
                    pObject->AsSWSTrigger()->m_nFaction = pOwner->m_pStats->m_factionId;
                if (pLast->m_nFaction != pOwner->m_pStats->m_factionId)
                    nFlags |= MSG_OBJECT_UPDATE_STATE;
            }
            else
            {
                if (static_cast<unsigned int>(pLast->m_nFaction) != pObject->AsSWSTrigger()->m_nFaction)
                    nFlags |= MSG_OBJECT_UPDATE_STATE;
            }
        }
    }

    return nFlags;
}

// The only Add*AppearanceToMessage that is not a plain field dump. The description is resolved only
// to send the "has examine text" bit, and the string itself never goes out.
void CSWSMessage::AddPlaceableAppearanceToMessage(CSWSPlaceable* pPlaceable)
{
#if KOTOR1  // K1: a byte appearance
    WriteBYTE(pPlaceable->m_appearanceType, 8);
#else
    WriteINT(pPlaceable->m_appearanceType, 32);
#endif
    WriteWORD(static_cast<unsigned short>(pPlaceable->m_nAnimation), 16);
    WriteBOOL(pPlaceable->m_bPlot);
    WriteBOOL(pPlaceable->m_bUseable);
    WriteBOOL(pPlaceable->m_bTrapDisarmable);
    WriteBOOL(pPlaceable->m_bLockable);
    WriteBOOL(pPlaceable->m_bLocked);
#if !KOTOR1  // K1: no NotBlastable bit on the wire
    WriteBOOL(pPlaceable->m_bNotBlastable);
#endif
    WriteBOOL(pPlaceable->m_bReserved1);

    CExoString sDescription;
    CExoLocString locDescription;
    GetLocStringServer(0xffffffff, pPlaceable->m_description, locDescription, sDescription, 0);

    int bEmpty = sDescription.IsEmpty();
    WriteBOOL(bEmpty == 0);
}

void CSWSMessage::AddItemAppearanceToMessage(CSWSItem* pItem)
{
    int nBaseItem = pItem->m_baseItem;
    WriteDWORD(nBaseItem, 32);

    CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem);
    if (pBaseItem == 0)
        return;

    WriteBYTE(pItem->m_nModelVariation, 8);

    if (pBaseItem->m_nModelType == 1) {
        WriteBYTE(pItem->m_nBodyVariation, 8);
        WriteBYTE(pItem->m_nTextureVar, 8);
    }
}

// Ships the item's active property records and their charge state: a leading "which entries
// follow" mask, then every byte.
void CSWSMessage::AddActiveItemPropertiesToMessage(CSWSItem* pItem, CSWSCreature* pCreature)
{
    if (pItem->GetBaseItem()->m_nItemType >= kItemTypeFirstArmor &&
        pItem->GetBaseItem()->m_nItemType <= kItemTypeLastArmor)
        WriteSHORT(static_cast<short>(pItem->m_nBaseArmorClass), 16);

    WriteCExoLocStringServer(pItem->m_localizedName, 0);
    WriteBOOL(1);
    WriteDWORD(pItem->GetCost(), 32);
    WriteDWORD(pItem->m_stackCount, 32);
    WriteBOOL(pCreature != 0 ? pCreature->CanUseItem(pItem, 1, 1) : 0);
    WriteBOOL(pItem->m_bits.m_bStolen);

    int nActiveProperties = pItem->m_nActiveProperties;
    WriteBYTE(static_cast<unsigned char>(nActiveProperties), 8);
    for (int i = 0; i < nActiveProperties; i++) {
        WriteWORD(pItem->GetActiveProperty(i)->m_type, 16);
        WriteWORD(pItem->GetActiveProperty(i)->m_subType, 16);
        WriteWORD(pItem->GetActiveProperty(i)->m_costTableValue, 16);
        WriteBYTE(pItem->GetActiveProperty(i)->m_nParam1Value, 8);
    }

    WriteBYTE(pItem->m_nUsedActivePropertyMask, 8);
    WriteBYTE(0xff, 8);
    for (int i = 0; i < 8; i++)
        WriteBYTE(pItem->m_nUsedActivePropertyUsesLeft[i], 8);
}

void CSWSMessage::AddAreaOfEffectObjectToMessage(CSWSAreaOfEffectObject* pAreaOfEffect)
{
    WriteINT(pAreaOfEffect->m_nAreaEffectId, 32);
    WriteOBJECTIDServer(pAreaOfEffect->m_oidLinkedToObject);
}

// The "item added to inventory slot" frame; the Delete twin sends the invalid-object sentinel
// in place of an item and stops there.
void CSWSMessage::WriteGameObjUpdate_WriteInventorySlotAdd(CSWSCreature* pCreature, CSWSItem* pItem,
                                                           unsigned int nSlot)
{
    WriteCHAR('A', 8);
    WriteOBJECTIDServer(pItem->m_objectId);
    WriteDWORD(nSlot, 32);

    if (nSlot == MSG_SLOT_LEFT_WEAPON || nSlot == MSG_SLOT_RIGHT_WEAPON)
        WriteBOOL(pCreature->GetLastInstantEquip(nSlot));

    AddItemAppearanceToMessage(pItem);
    AddActiveItemPropertiesToMessage(pItem, pCreature);
}

void CSWSMessage::WriteGameObjUpdate_WriteInventorySlotDelete(CSWSCreature* pCreature,
                                                              unsigned int nSlot)
{
    WriteCHAR('D', 8);
    WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
    WriteDWORD(nSlot, 32);

    if (nSlot == MSG_SLOT_LEFT_WEAPON || nSlot == MSG_SLOT_RIGHT_WEAPON)
        WriteBOOL(pCreature->GetLastInstantEquip(nSlot));
}

// Writes the appearance-update frame, made of the header, the changed appearance fields the mask
// names, and the inventory-slot refresh when the update is unconditional or the appearance block
// changed.
void CSWSMessage::WriteGameObjUpdate_UpdateAppearance(CSWSObject* pObject, CLastUpdateObject* pLast,
                                                      unsigned int nFlags)
{
    unsigned char nType = pObject->m_objectType;
    if (nType != CGameObject::OBJECT_TYPE_CREATURE)
        return;
    unsigned long long nObjectId = pObject->m_objectId;

    CSWSCreature* pCreature = static_cast<CSWSCreature*>(pObject);

    WriteCHAR('P', 8);
    WriteBYTE(nType, 8);
    WriteOBJECTIDServer(nObjectId);
    WriteWORD(static_cast<unsigned short>(nFlags), 16);

    CSWSCreatureAppearanceInfo* pApp = &pCreature->m_appearance;
    CSWSCreatureAppearanceInfo* pSnap = &pLast->m_appearance;

    if (nFlags & MSG_APPEARANCE_UPDATE_NAME)
    {
        WriteCExoLocStringServer(pCreature->m_pStats->m_firstName, 0);
        WriteCExoLocStringServer(pCreature->m_pStats->m_lastName, 0);
    }
    if (nFlags & MSG_APPEARANCE_UPDATE_APPEARANCE_ID)
        WriteWORD(pApp->m_headId, 16);
    if (nFlags & MSG_APPEARANCE_UPDATE_PHENOTYPE)
        WriteBYTE(pApp->m_nPhenotype, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_GENDER)
        WriteBYTE(pApp->m_nGender, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_HEAD)
        WriteBYTE(pApp->m_nAppearanceHead, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_USE_BACKUP_HEAD)
        WriteBOOL(pApp->m_bUseBackupHead);
    if (nFlags & MSG_APPEARANCE_UPDATE_DUPLICATING_HEAD)
        WriteBYTE(pApp->m_nDuplicatingHead, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_TAIL)
        WriteBYTE(pApp->m_nTail, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_WINGS)
        WriteBYTE(pApp->m_nWings, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_SKIN_COLOR)
        WriteBYTE(pApp->m_nColorSkin, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_HAIR_COLOR)
        WriteBYTE(pApp->m_nColorHair, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_TATTOO_COLOR_1)
        WriteBYTE(pApp->m_nColorTattoo1, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_TATTOO_COLOR_2)
        WriteBYTE(pApp->m_nColorTattoo2, 8);
    if (nFlags & MSG_APPEARANCE_UPDATE_SOUND_SET)
        WriteWORD(pCreature->m_nSoundSetFile, 16);

    if (nFlags == MSG_APPEARANCE_UPDATE_ALL)
    {
        int nCount = 4;
        if (pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY))
            nCount++;
        if (pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD))
            nCount++;
        if (pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_LEFT_WEAPON))
            nCount++;
        if (pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_RIGHT_WEAPON))
            nCount++;
        WriteBYTE(static_cast<unsigned char>(nCount), 8);

        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_BODY, 32);
        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_HEAD, 32);
        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_LEFT_WEAPON, 32);
        WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_LEFT_WEAPON));
        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_RIGHT_WEAPON, 32);
        WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON));

#if KOTOR1  // K1: the slot adds go through WriteGameObjUpdate_WriteInventorySlotAdd
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_BODY);
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_HEAD);
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_LEFT_WEAPON));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_LEFT_WEAPON);
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_RIGHT_WEAPON));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_RIGHT_WEAPON);
#else
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_BODY, 32);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_HEAD, 32);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_LEFT_WEAPON));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_LEFT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_LEFT_WEAPON));
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_RIGHT_WEAPON));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_RIGHT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON));
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
#endif

        pCreature->SetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON, 0);
        pCreature->SetLastInstantEquip(MSG_SLOT_LEFT_WEAPON, 0);
        return;
    }

    if (!(nFlags & MSG_APPEARANCE_UPDATE_EQUIPMENT))
        return;

    // Count the slot writes that follow. A changed slot costs one (an empty delete) or two (an item
    // add).
    int nCount = 0;
    bool bChangedLeft = false;
    bool bChangedRight = false;
    CExoArrayList<unsigned int> aSlots;
    if (pApp->m_itemId[0].m_a != pSnap->m_itemId[0].m_a)
        nCount = (pApp->m_itemId[0].m_a == OBJECT_INVALID) ? 1 : 2;
    if (pApp->m_itemId[0].m_b != pSnap->m_itemId[0].m_b)
    {
        if (pApp->m_itemId[0].m_b == OBJECT_INVALID)
            nCount++;
        else
            nCount += 2;
    }
    if (pApp->m_headId == pSnap->m_headId &&
        pApp->m_nGender == pSnap->m_nGender &&
        pApp->m_nPhenotype == pSnap->m_nPhenotype &&
        pApp->m_nColorSkin == pSnap->m_nColorSkin &&
        pApp->m_nColorHair == pSnap->m_nColorHair &&
        pApp->m_nColorTattoo1 == pSnap->m_nColorTattoo1 &&
        pApp->m_nColorTattoo2 == pSnap->m_nColorTattoo2)
    {
        if (pApp->m_itemId[1].m_a != pSnap->m_itemId[1].m_a)
        {
            if (pApp->m_itemId[1].m_a == OBJECT_INVALID)
                nCount++;
            else
                nCount += 2;
        }
        if (pApp->m_itemId[1].m_b != pSnap->m_itemId[1].m_b)
        {
            if (pApp->m_itemId[1].m_b == OBJECT_INVALID)
                nCount++;
            else
                nCount += 2;
        }
    }
    else
    {
        nCount += 2;
        CSWItem* pItem1 = pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD);
        CSWItem* pItem2 = pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY);
        if (pItem1) nCount++;
        if (pItem2) nCount++;
    }

    WriteBYTE(static_cast<unsigned char>(nCount), 8);

    // Left weapon. Each changed slot writes its delete frames in both arms.
    if (pApp->m_itemId[0].m_a != pSnap->m_itemId[0].m_a)
    {
        if (pApp->m_itemId[0].m_a == OBJECT_INVALID)
        {
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
            WriteDWORD(MSG_SLOT_LEFT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_LEFT_WEAPON));
            bChangedRight = true;
        }
        else
        {
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
            WriteDWORD(MSG_SLOT_LEFT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_LEFT_WEAPON));
            aSlots.Allocate(16);
            aSlots.Add(MSG_SLOT_LEFT_WEAPON);
            bChangedRight = true;
        }
    }

    // Right weapon.
    if (pApp->m_itemId[0].m_b != pSnap->m_itemId[0].m_b)
    {
        if (pApp->m_itemId[0].m_b == OBJECT_INVALID)
        {
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
            WriteDWORD(MSG_SLOT_RIGHT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON));
            bChangedLeft = true;
        }
        else
        {
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
            WriteDWORD(MSG_SLOT_RIGHT_WEAPON, 32);
            WriteBOOL(pCreature->GetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON));
            aSlots.Add(MSG_SLOT_RIGHT_WEAPON);
            bChangedLeft = true;
        }
    }

    if (pApp->m_headId == pSnap->m_headId &&
        pApp->m_nGender == pSnap->m_nGender &&
        pApp->m_nPhenotype == pSnap->m_nPhenotype &&
        pApp->m_nColorSkin == pSnap->m_nColorSkin &&
        pApp->m_nColorHair == pSnap->m_nColorHair &&
        pApp->m_nColorTattoo1 == pSnap->m_nColorTattoo1 &&
        pApp->m_nColorTattoo2 == pSnap->m_nColorTattoo2)
    {
        if (pApp->m_itemId[1].m_a != pSnap->m_itemId[1].m_a)
        {
            if (pApp->m_itemId[1].m_a == OBJECT_INVALID)
            {
                WriteCHAR('D', 8);
                WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
                WriteDWORD(MSG_SLOT_BODY, 32);
            }
            else
            {
                WriteCHAR('D', 8);
                WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
                WriteDWORD(MSG_SLOT_BODY, 32);
                aSlots.Add(MSG_SLOT_BODY);
            }
        }
        if (pApp->m_itemId[1].m_b != pSnap->m_itemId[1].m_b)
        {
            if (pApp->m_itemId[1].m_b == OBJECT_INVALID)
            {
                WriteCHAR('D', 8);
                WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
                WriteDWORD(MSG_SLOT_HEAD, 32);
            }
            else
            {
                WriteCHAR('D', 8);
                WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
                WriteDWORD(MSG_SLOT_HEAD, 32);
                aSlots.Add(MSG_SLOT_HEAD);
            }
        }
    }
    else
    {
        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_BODY, 32);
        WriteCHAR('D', 8);
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
        WriteDWORD(MSG_SLOT_HEAD, 32);

#if KOTOR1
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_HEAD);
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY));
        if (pItem)
            WriteGameObjUpdate_WriteInventorySlotAdd(pCreature, pItem, MSG_SLOT_BODY);
#else
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_HEAD));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_HEAD, 32);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
        pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(MSG_SLOT_BODY));
        if (pItem)
        {
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(pItem->m_objectId);
            WriteDWORD(MSG_SLOT_BODY, 32);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pCreature);
        }
#endif
    }

    if (aSlots.m_count > 0)
    {
        for (int64_t nOff = 0; nOff != static_cast<int64_t>(aSlots.m_count) * 4; nOff += 4)
            WriteGameObjUpdate_WriteInventorySlotAdd(
                pCreature,
                static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(
                    *reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(aSlots.m_pArray) + nOff))),
                *reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(aSlots.m_pArray) + nOff));
    }

    if (bChangedLeft)
        pCreature->SetLastInstantEquip(MSG_SLOT_RIGHT_WEAPON, 0);
    if (bChangedRight)
        pCreature->SetLastInstantEquip(MSG_SLOT_LEFT_WEAPON, 0);
}

// Emits a relative update frame for one object as observed by one player. The header is the
// opcode, the object type and id, then the update bitset; each low bit of nFlags selects one
// payload block (position, orientation and lock target, portrait, animation and the
// combat-round switch, the looping-visual-effect diff, the trap/trigger/door/placeable state),
// and the creature type gate opens the long relative tail. nUpdateFlags is forwarded to
// WriteGameObjUpdate_UpdateAppearance.
void CSWSMessage::WriteGameObjUpdate_UpdateObject(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                                  CLastUpdateObject* pLast, unsigned int nMask,
                                                  unsigned int nUpdateFlags)
{
    CSWSObject* pPlayerObject = pPlayer->GetGameObject();
    if (pPlayerObject == 0)
        return;

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE && pLast != 0 && nUpdateFlags != 0)
    {
        WriteGameObjUpdate_UpdateAppearance(pObject, pLast, nUpdateFlags);
    }

    WriteCHAR('U', 8);
    WriteBYTE(pObject->m_objectType, 8);
    WriteOBJECTIDServer(pObject->m_objectId);
    WriteDWORD(nMask, 32);

    if (nMask & MSG_OBJECT_UPDATE_POSITION)
    {
        float fPosX = pObject->m_position.x;
        float fPosY = pObject->m_position.y;
        float fPosZ = pObject->m_position.z;
        WriteFLOAT(fPosX, 1.0f, 32);
        WriteFLOAT(fPosY, 1.0f, 32);
        WriteFLOAT(fPosZ, 1.0f, 32);
    }

    if (nMask & MSG_OBJECT_UPDATE_ORIENTATION)
    {
        float fOrX = pObject->m_orientation.x;
        float fOrY = pObject->m_orientation.y;
        float fOrZ = pObject->m_orientation.z;
        WriteFLOAT(fOrX, 1.0f, 32);
        WriteFLOAT(fOrY, 1.0f, 32);
        WriteFLOAT(fOrZ, 1.0f, 32);

        if (pObject->AsSWSCreature())
        {
            if (pObject->AsSWSCreature()->GetLockOrientationToObject() ==
                CGameObjectArray::INVALIDOBJID)
            {
                WriteBYTE(0, 8);
            }
            else
            {
                WriteBYTE(1, 8);
                WriteOBJECTIDServer(pObject->AsSWSCreature()->GetLockOrientationToObject());
            }
        }
    }

    if (nMask & MSG_OBJECT_UPDATE_PORTRAIT)
    {
        unsigned short nPortraitId = pObject->GetPortraitId();
        WriteWORD(nPortraitId, 16);
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            WriteCResRef(pObject->GetPortrait(), 16);
    }

    int nAnimation K2_UNSET;
    if (nMask & MSG_OBJECT_UPDATE_ANIMATION)
    {
        WriteFLOAT(pObject->m_fAnimationSpeed, 1.0f, 32);
        nAnimation = pObject->m_nAnimation;
        WriteWORD(static_cast<unsigned short>(nAnimation), 16);

        if (pObject->AsSWSCreature())
        {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            unsigned short nAnim = static_cast<unsigned short>(nAnimation);
            switch (nAnim)
            {
            case SERVER_ANIMATION_ATTACK:
            case SERVER_ANIMATION_ATTACK_2:
            {
                int nAttackCount = pCreature->m_pCombatRound->m_currentAttack + 1;
                WriteBYTE(static_cast<unsigned char>(nAttackCount), 2);
                for (int i = 0; i < nAttackCount; i++)
                {
                    WriteOBJECTIDServer(pCreature->m_pCombatRound->GetAttack(i)->m_targetObjectId);
                    WriteWORD(pCreature->m_pCombatRound->GetAttack(i)->m_nReaxnAnimation, 16);
                    WriteWORD(pCreature->m_pCombatRound->GetAttack(i)->m_nReaxnAnimLength, 16);
                    WriteWORD(pCreature->m_pCombatRound->GetAttack(i)->m_nAnimationLength, 16);
                    WriteBYTE(pCreature->m_pCombatRound->GetAttack(i)->m_nAttackResult, 4);
                    WriteWORD(pCreature->m_pCombatRound->GetAttack(i)->m_nFeatId, 16);
                    WriteWORD(static_cast<unsigned short>(pCreature->m_pCombatRound->GetAttack(i)->GetTotalDamage(1)), 9);
                    WriteBOOL(pCreature->m_pCombatRound->GetAttack(i)->m_bRangedAttack);
                    WriteBOOL(pCreature->m_pCombatRound->GetAttack(i)->m_bKillingBlow);
                    WriteBYTE(pCreature->m_pCombatRound->GetAttack(i)->m_nWeaponAttackType, 4);
                }
                if (!((nAnim >= SERVER_ANIMATION_CONJURE_1 && nAnim <= SERVER_ANIMATION_CAST_4) || nAnim == SERVER_ANIMATION_THROW_SABER_LOOP || nAnim == SERVER_ANIMATION_POWER_ATTACK ||
                      nAnim == SERVER_ANIMATION_CONJURE_3))
                    break;
            }
            case SERVER_ANIMATION_CONJURE_1:
            case SERVER_ANIMATION_CONJURE_2:
            case SERVER_ANIMATION_CAST_1:
            case SERVER_ANIMATION_CAST_2:
            case SERVER_ANIMATION_CAST_3:
            case SERVER_ANIMATION_CAST_4:
            case SERVER_ANIMATION_THROW_SABER_LOOP:
            case SERVER_ANIMATION_POWER_ATTACK:
            case SERVER_ANIMATION_CONJURE_3:
            {
                WriteDWORD(pCreature->m_castSpellId, 32);
                if (pCreature->m_castSpellId == FORCE_POWER_LIGHT_SABER_THROW)
                {
                    CSWItem* pCastItem = pCreature->m_pInventory->GetItemInSlot(16);
                    WriteOBJECTIDServer(pCastItem != 0 ? static_cast<CSWSItem*>(pCastItem)->m_objectId
                                                       : OBJECT_INVALID);
                }
                else
                {
                    WriteOBJECTIDServer(pCreature->m_lastSpellItemId);
                }

                if ((nAnim >= SERVER_ANIMATION_CAST_1 && nAnim <= SERVER_ANIMATION_CAST_4) || nAnim == SERVER_ANIMATION_THROW_SABER_LOOP || nAnim == SERVER_ANIMATION_POWER_ATTACK)
                {
                    if (pCreature->m_castTargetId == OBJECT_INVALID)
                    {
                        WriteBYTE(2, 8);
                        WriteFLOAT(pCreature->m_castPosition.x, 1.0f, 32);
                        WriteFLOAT(pCreature->m_castPosition.y, 1.0f, 32);
                        WriteFLOAT(pCreature->m_castPosition.z, 1.0f, 32);
                    }
                    else
                    {
                        WriteBYTE(1, 8);
                        WriteOBJECTIDServer(pCreature->m_castTargetId);
                    }
                    WriteDWORD(pCreature->m_castLevel, 32);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    if (nMask & MSG_OBJECT_UPDATE_VISUAL_EFFECTS)
    {
        struct Change
        {
            CLoopingVisualEffect* pVisualEffect;
            unsigned char nChangeType;
            char _pad[7];
        };
        CExoArrayList<Change> changes;
        CExoArrayList<unsigned short> ids;

        int nObjectFxCount = pObject->m_visualEffects.m_count;

        for (int i = 0; i < nObjectFxCount; i++)
        {
            CLoopingVisualEffect* pVis = pObject->m_visualEffects.m_pArray[i];
            unsigned short nVisId = pVis->m_nVisualEffectId;
            int bDuplicate = 0;

            if (pVis->GetIsBeam())
            {
                for (int j = 0; j < static_cast<int>(changes.m_count); j++)
                {
                    CLoopingVisualEffect* pExisting = changes.m_pArray[j].pVisualEffect;
                    if (pExisting->m_nVisualEffectId == nVisId && pExisting->m_nBodyPart == pVis->m_nBodyPart &&
                        pExisting->m_targetId == pVis->m_targetId)
                    {
                        bDuplicate = 1;
                        break;
                    }
                }
            }
            else
            {
                if (ids.Contains(nVisId) > 0)
                    bDuplicate = 1;
            }

            if (bDuplicate == 0)
            {
                int bFoundInSnapshot = 0;
                for (int j = 0; j < static_cast<int>(pLast->m_visualEffects.m_count); j++)
                {
                    CLoopingVisualEffect* pLastVis = pLast->m_visualEffects.m_pArray[j];
                    if (pVis->GetIsBeam())
                    {
                        if (pLastVis->m_nVisualEffectId == nVisId && pLastVis->m_nBodyPart == pVis->m_nBodyPart &&
                            pLastVis->m_targetId == pVis->m_targetId)
                        {
                            bFoundInSnapshot = 1;
                            break;
                        }
                    }
                    else if (pLastVis->m_nVisualEffectId == nVisId)
                    {
                        bFoundInSnapshot = 1;
                        break;
                    }
                }
                if (bFoundInSnapshot == 0)
                {
                    int bHasAdd = 0;
                    for (int j = 0; j < static_cast<int>(changes.m_count); j++)
                        bHasAdd |= (changes.m_pArray[j].pVisualEffect == pVis &&
                                    changes.m_pArray[j].nChangeType == 'A');
                    if (bHasAdd == 0)
                    {
                        Change addChg;
                        addChg.pVisualEffect = pVis;
                        addChg.nChangeType = 'A';
                        changes.Add(addChg);
                    }
                    ids.Add(nVisId);
                }
            }
        }

        for (int i = 0; i < static_cast<int>(pLast->m_visualEffects.m_count); i++)
        {
            CLoopingVisualEffect* pLastVis = pLast->m_visualEffects.m_pArray[i];
            int bStillActive = 0;
            for (int j = 0; j < nObjectFxCount; j++)
            {
                CLoopingVisualEffect* pCur = pObject->m_visualEffects.m_pArray[j];
                if (pLastVis->GetIsBeam())
                {
                    if (pCur->m_nVisualEffectId == pLastVis->m_nVisualEffectId &&
                        pCur->m_nBodyPart == pLastVis->m_nBodyPart && pCur->m_targetId == pLastVis->m_targetId)
                    {
                        bStillActive = 1;
                        break;
                    }
                }
                else if (pCur->m_nVisualEffectId == pLastVis->m_nVisualEffectId)
                {
                    bStillActive = 1;
                    break;
                }
            }
            if (bStillActive == 0)
            {
                int bHasDelete = 0;
                for (int j = 0; j < static_cast<int>(changes.m_count); j++)
                    bHasDelete |= (changes.m_pArray[j].pVisualEffect == pLastVis &&
                                   changes.m_pArray[j].nChangeType == 'D');
                if (bHasDelete == 0)
                {
                    Change delChg;
                    delChg.pVisualEffect = pLastVis;
                    delChg.nChangeType = 'D';
                    changes.Add(delChg);
                }
            }
        }

        WriteWORD(static_cast<unsigned short>(changes.m_count), 16);
        for (int i = 0; i < static_cast<int>(changes.m_count); i++)
        {
            CLoopingVisualEffect* pVis = changes.m_pArray[i].pVisualEffect;
            WriteBYTE(changes.m_pArray[i].nChangeType, 8);
            WriteWORD(pVis->m_nVisualEffectId, 16);
            if (pVis->GetIsBeam())
            {
                WriteOBJECTIDServer(pVis->m_targetId);
                WriteBYTE(pVis->m_nBodyPart, 8);
            }
        }
    }

    if (nMask & MSG_OBJECT_UPDATE_STATE)
    {
        if (pObject->AsSWSTrigger())
        {
            if (pObject->AsSWSTrigger()->m_bTrap != 0)
            {
                int bDirty = 0;
                if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                           0) <= kReputationMaxNeutral)
                {
                    bDirty = (pObject->AsSWSTrigger()->m_nFaction !=
                              static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId));
                }
                WriteBOOL(bDirty);
            }
        }
        else if (pObject->AsSWSDoor())
        {
            int bFactionDrift = 0;
            if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                       0) <= kReputationMaxNeutral)
            {
                bFactionDrift = (pObject->AsSWSDoor()->m_nFaction !=
                                 static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId));
            }

            int nState;
            if (static_cast<unsigned char>(pObject->AsSWSDoor()->m_bMineFlagged) == 0 &&
                pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                       0) <= kReputationMaxNeutral &&
                pObject->AsSWSDoor()->m_nFaction !=
                    static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId))
            {
                if (pObject->AsSWSDoor()->m_objectsAtDoor.Contains(
                        pPlayer->GetGameObject()->AsSWSCreature()->m_objectId) != 0)
                    nState = pObject->AsSWSDoor()->m_bTrapFlag != 0;
                else
                    nState = 0;
            }
            else
            {
                nState = pObject->AsSWSDoor()->m_bTrapFlag != 0;
            }
            int nDoorLocked = pObject->AsSWSDoor()->m_bLocked;
            int nDoorPlot = pObject->AsSWSDoor()->m_bPlot;
            WriteBOOL(bFactionDrift);
            WriteBOOL(nState);
            WriteBOOL(nDoorLocked == 1);
            WriteBOOL(nDoorPlot == 1);
        }
        else if (pObject->AsSWSPlaceable())
        {
            int bFactionDrift = 0;
            if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                       0) <= kReputationMaxNeutral)
            {
                bFactionDrift = (pObject->AsSWSPlaceable()->m_nFaction !=
                                 static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId));
            }

            int nState;
            if (static_cast<unsigned char>(pObject->AsSWSPlaceable()->m_bMineFlagged) == 0 &&
                pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                       0) <= kReputationMaxNeutral &&
                pObject->AsSWSPlaceable()->m_nFaction !=
                    static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId))
            {
                if (pObject->AsSWSPlaceable()->m_objectsOnPlaceable.Contains(
                        pPlayer->GetGameObject()->AsSWSCreature()->m_objectId) != 0)
                    nState = pObject->AsSWSPlaceable()->m_bTrapFlag != 0;
                else
                    nState = 0;
            }
            else
            {
                nState = pObject->AsSWSPlaceable()->m_bTrapFlag != 0;
            }
            int nPlaceLocked = pObject->AsSWSPlaceable()->m_bLocked;
            int nPlacePlot = pObject->AsSWSPlaceable()->m_bPlot;
            WriteBOOL(bFactionDrift);
            WriteBOOL(nState);
            WriteBOOL(nPlaceLocked == 1);
            WriteBOOL(nPlacePlot == 1);
        }
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        CSWSCreature* pCreature = static_cast<CSWSCreature*>(pObject);
        if (nMask & MSG_OBJECT_UPDATE_ANIMATION)
        {
            int nViewerId = 0;
            if ((pCreature->m_pPathfindInfo->m_nClientPathId & 0xff) != 0 &&
                pObject->m_objectId == pPlayerObject->m_objectId && pCreature->m_bJumpPending == 1)
            {
                pCreature->m_bJumpPending = 0;
            }
            else if ((pCreature->m_pPathfindInfo->m_nClientPathId & 0xff) != 0 &&
                     pObject->m_objectId == pPlayerObject->m_objectId)
            {
                nViewerId = pCreature->m_pPathfindInfo->m_nClientPathId;
            }
            WriteBYTE(static_cast<unsigned char>(nViewerId), 8);

            CPathfindInformation* pPathfindInfo = pCreature->m_pPathfindInfo;
            if (pObject->AnimationStationary(static_cast<unsigned short>(nAnimation)) == 1)
            {
                WriteWORD(0, 16);
            }
            else
            {
                int nPathSegCount = pPathfindInfo->m_nWayPointFloats / 2;
                int nPathSegOffset = pPathfindInfo->m_nWayPointCursor / 2;
                if (static_cast<unsigned short>(nPathSegCount) < nPathSegOffset)
                {
                    WriteWORD(0, 16);
                }
                else
                {
                    unsigned short nRemaining = static_cast<unsigned short>(nPathSegCount - nPathSegOffset);
                    WriteWORD(nRemaining, 16);
                    if (nRemaining != 0)
                    {
                        WriteBOOL(pCreature->m_bDriving);
                        if (pCreature->m_bDriving != 0)
                            WriteFLOAT(pCreature->m_fDriveSpeedFactor, 1.0f, 32);
                    }
                    nPathSegCount = nRemaining + nPathSegOffset;
                }
                if (static_cast<unsigned short>(nPathSegCount) != 0 &&
                    nPathSegOffset < static_cast<unsigned short>(nPathSegCount))
                {
                    for (int i = nPathSegOffset; i < static_cast<unsigned short>(nPathSegCount); i++)
                    {
                        WriteFLOAT(pPathfindInfo->m_paWayPoints[i * 2], 1.0f, 32);
                        WriteFLOAT(pPathfindInfo->m_paWayPoints[i * 2 + 1], 1.0f, 32);
                        WriteFLOAT(0.0f, 1.0f, 32);
                    }
                }
            }
        }

        if (nMask & MSG_OBJECT_UPDATE_AI_STATE)
        {
            unsigned short nState1 = pCreature->m_nAIState;
            unsigned int nState3 = pCreature->m_activityFlags;
            int nState4 = pCreature->m_bCombatState;
            unsigned char nState5 = pCreature->m_stealthMode;
            unsigned char nState2 = pCreature->m_nAIStateAction;
            int nState6 = pCreature->m_bStealthNoAnimation;

            int nAIState = 2;
            if (pPlayer != 0)
            {
                if (pPlayer->GetGameObject() != 0)
                {
                    if (pPlayer->GetGameObject()->AsSWSCreature() != 0)
                    {
                        nAIState = pPlayer->GetGameObject()->AsSWSCreature()
                                       ->GetAIStateReputation(pObject->m_objectId);
                    }
                }
            }
            WriteWORD(nState1, 16);
            WriteBYTE(nState2, 8);
            WriteWORD(static_cast<unsigned short>(nState3), 16);
            WriteBYTE(static_cast<unsigned char>(nAIState), 8);
            WriteBYTE(nState5, 8);
            WriteBOOL(nState6);
            WriteBOOL(nState4);
            if (nState2 == 2)
                WriteOBJECTIDServer(pCreature->m_oidAIStateTarget);
        }

        if (nMask & MSG_OBJECT_UPDATE_MOVE_RATES)
        {
            WriteFLOAT(static_cast<CSWSCreature*>(pObject)->GetWalkRate(), 1.0f, 32);
            WriteFLOAT(static_cast<CSWSCreature*>(pObject)->GetRunRate(), 1.0f, 32);
        }

        if (nMask & MSG_OBJECT_UPDATE_AREA)
            WriteOBJECTIDServer(pObject->m_cachedAreaId);

        if (nMask & MSG_OBJECT_UPDATE_COARSE_POSITION)
        {
            WriteINT(static_cast<int>(pObject->m_position.x), 10);
            WriteINT(static_cast<int>(pObject->m_position.y), 10);
        }

        if (nMask & MSG_OBJECT_UPDATE_HIT_POINTS)
        {
            WriteSHORT(pObject->GetCurrentHitPoints(1), 16);
            WriteSHORT(static_cast<short>(pObject->m_hitPoints), 16);
            WriteSHORT(static_cast<short>(pObject->m_hitPointBonus), 16);
            WriteSHORT(pObject->GetMaxHitPoints(1), 16);
        }

        if (nMask & MSG_OBJECT_UPDATE_FORCE_POINTS)
        {
            WriteSHORT(pCreature->m_pStats->m_nCurrentForcePoints, 16);
            WriteSHORT(pCreature->m_pStats->m_nBaseForcePoints, 16);
            WriteSHORT(pCreature->m_pStats->m_nTemporaryForcePoints, 16);
            WriteSHORT(static_cast<CSWSCreature*>(pObject)->GetMaxForcePoints(), 16);
        }

        if (nMask & MSG_OBJECT_UPDATE_NAME)
        {
            WriteCExoLocStringServer(pCreature->m_pStats->m_firstName, 0);
            WriteCExoLocStringServer(pCreature->m_pStats->m_lastName, 0);
        }

        if (nMask & MSG_OBJECT_UPDATE_PARTY)
        {
            WriteBOOL(pCreature->m_bPlayerCharacter);
            WriteBOOL((pCreature->m_nAIState >> 6) & 1);
            WriteBOOL(static_cast<CSWSCreature*>(pObject)->GetFaction()->GetSingletonParty());
            WriteBOOL(static_cast<CSWSCreature*>(pObject)->GetFaction()->GetLeader() == pObject->m_objectId);
            WriteBOOL(pCreature->m_bInvitedToParty);
            if (pCreature->m_bInvitedToParty != 0)
            {
                WriteOBJECTIDServer(pCreature->m_oidInvitedToPartyBy);
                CSWSCreature* pOther = g_pAppManager->m_pServerExoApp
                                           ->GetCreatureByGameObjectID(pCreature->m_oidInvitedToPartyBy);
                WriteCExoLocStringServer(pOther->m_pStats->m_firstName, 0);
                WriteCExoLocStringServer(pOther->m_pStats->m_lastName, 0);
            }
            WriteBOOL(pCreature->m_bDiseased);
            WriteBOOL(pCreature->m_bPoisoned);
        }

        if (nMask & MSG_OBJECT_UPDATE_VISIBILITY)
        {
            CSWSCreature* pViewer = pPlayer->GetGameObject()->AsSWSCreature();
            if (pViewer != 0)
            {
                int nMembers = 0;
                if (pObject->m_objectId == pViewer->m_objectId)
                {
                    WriteBOOL(1);
                    WriteBOOL(1);
                    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
                    if (pModule != 0)
                        nMembers = pModule->m_playerList.Contains(pObject->m_objectId);
                    WriteBOOL(nMembers);
                }
                else
                {
                    CSWVisibilityNode* pFound =
                        static_cast<CSWSCreature*>(pObject)->GetVisibleListElement(pViewer->m_objectId);
                    if (pFound != 0)
                    {
                        WriteBOOL((pFound->m_flags >> 1) & 1);
                        WriteBOOL(pFound->m_flags & 1);
                        nMembers = (pFound->m_flags >> 4) & 1;
                    }
                    else
                    {
                        WriteBOOL(0);
                        WriteBOOL(0);
                        nMembers = 0;
                    }
                    WriteBOOL(nMembers);
                }
            }
        }

        if (nMask & MSG_OBJECT_UPDATE_ASSOCIATE_STATE)
        {
            WriteBYTE(ComputeLastUpdate_AssociateState(pCreature), 8);
        }

        if (nMask & MSG_OBJECT_UPDATE_DAMAGE_LEVEL)
        {
            WriteBYTE(pObject->GetDamageLevel(), 8);
        }

        if (nMask & MSG_OBJECT_UPDATE_IDENTITY)
        {
            WriteWORD(pCreature->m_pStats->m_nRace, 16);
            WriteCExoString(pCreature->m_pStats->m_sSubrace, 32);
            WriteBYTE(pCreature->m_pStats->m_nSubraceIndex, 8);
            WriteBYTE(pCreature->m_pStats->m_nGender, 8);
            WriteSHORT(pCreature->m_pStats->m_nAlignment, 16);
#if !KOTOR1  // K1: not on the wire
            WriteSHORT(pCreature->m_pStats->m_nBaseNPCAlignment, 16);
            WriteINT(pCreature->m_stanceId, 32);
            WriteCExoString(pObject->m_sTag, 32);
#endif
            WriteBOOL(pObject->m_bDeadSelectable);
            WriteBYTE(pCreature->m_pStats->m_numClasses, 8);

            if (pCreature->m_pStats->m_numClasses != 0)
            {
                for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++)
                {
#if KOTOR1  // K1: CSWSCreatureStats::GetClass is out of line
                    WriteBYTE(pCreature->m_pStats->GetClass(static_cast<unsigned char>(i)), 8);
#else
                    WriteBYTE((static_cast<unsigned char>(i) < pCreature->m_pStats->m_numClasses)
                                  ? pCreature->m_pStats->m_classInfo[i].m_class
                                  : CLASS_TYPE_INVALID,
                              8);
#endif
                    WriteBYTE(pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0), 8);
                }
            }
        }
    }
}

// Writes the trigger's polygon, a vertex count byte, then three floats per vertex.
void CSWSMessage::AddTriggerGeometryToMessage(CSWSTrigger* pTrigger)
{
    WriteBYTE(static_cast<unsigned char>(pTrigger->m_nGeomVertCount), 8);

    for (int i = 0; i < pTrigger->m_nGeomVertCount; i++)
    {
        WriteFLOAT(pTrigger->m_pGeomVerts[i].x, 1.0f, 32);
        WriteFLOAT(pTrigger->m_pGeomVerts[i].y, 1.0f, 32);
        WriteFLOAT(pTrigger->m_pGeomVerts[i].z, 1.0f, 32);
    }
}

// Always reports that the container needs re-sending.
int CSWSMessage::ComputeRepositoryUpdateRequired(CSWSPlayer* pPlayer, CExoLinkedListNode* pA,
                                                 CExoLinkedListNode* pB)
{
    K2_UNUSED(pPlayer); K2_UNUSED(pA); K2_UNUSED(pB);
    return 1;
}

int CSWSMessage::CompareCreatureLists(CExoArrayList<unsigned long long>* pA,
                                      CExoArrayList<unsigned long long>* pB)
{
    int nCount = pA->m_count;
    if (nCount != pB->m_count)
        return 0;

    for (int i = 0; i < nCount; i++)
    {
        if (pA->m_pArray[i] != pB->m_pArray[i])
            return 0;
    }

    return 1;
}

// Packs which skill buttons the client should show into a bitmask.
int CSWSMessage::ComputeLastUpdate_GuiSkills(CSWSCreature* pCreature)
{
    CSWSCreatureStats* pStats = pCreature->m_pStats;

    int nFlags = 0;

    if (pStats->GetCanUseSkill(SKILL_DEMOLITIONS))
        nFlags |= MSG_GUI_SKILL_DEMOLITIONS;

    if (pStats->GetCanUseSkill(SKILL_STEALTH))
        nFlags |= MSG_GUI_SKILL_STEALTH;

    if (pStats->GetCanUseSkill(SKILL_STEALTH))
        nFlags |= MSG_GUI_SKILL_STEALTH;

    if (pStats->GetCanUseSkill(SKILL_SECURITY))
        nFlags |= MSG_GUI_SKILL_SECURITY;

    return nFlags;
}

int CSWSMessage::ComputeLastUpdate_GuiMemorizedSpells(CSWSCreature* pCreature,
                                                      CSWSPlayerLastUpdateObject* pLast)
{
    K2_UNUSED(pCreature); K2_UNUSED(pLast);
    return 0;
}

int CSWSMessage::ComputeLastUpdate_GuiNumberMemorizedSpells(CSWSCreature* pCreature,
                                                            CSWSPlayerLastUpdateObject* pLast)
{
    K2_UNUSED(pCreature); K2_UNUSED(pLast);
    return 0;
}

// Diffs the creature's three feat lists against the client's snapshot: the two id lists are
// searched (a feat may move without counting as a change), the remaining-uses list is compared
// index by index.
int CSWSMessage::ComputeLastUpdate_GuiFeats(CSWSCreature* pCreature,
                                            CSWSPlayerLastUpdateObject* pLast)
{
    if (pLast->m_feats.m_count != static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count))
        return 1;

    if (pLast->m_bonusFeats.m_count != static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count))
        return 1;

    if (pCreature->m_pStats->m_featRemainingUses.m_count != pLast->m_featRemainingUses.m_count)
        return 1;

    for (unsigned short i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); i++)
    {
        unsigned short nFeat = pCreature->m_pStats->GetFeat(i);

        int bFound = 0;
        for (int j = 0; j < pLast->m_feats.m_count; j++)
            if (pLast->m_feats.m_pArray[j] == nFeat)
                bFound = 1;

        if (!bFound)
            return 1;
    }

    for (unsigned short i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); i++)
    {
        unsigned short nFeat = pCreature->m_pStats->GetBonusFeat(i);

        int bFound = 0;
        for (int j = 0; j < pLast->m_bonusFeats.m_count; j++)
            if (pLast->m_bonusFeats.m_pArray[j] == nFeat)
                bFound = 1;

        if (!bFound)
            return 1;
    }

    for (unsigned short i = 0; i < pCreature->m_pStats->m_featRemainingUses.m_count; i++)
    {
        if (pCreature->m_pStats->m_featRemainingUses.m_pArray[i]->m_nFeat
            != pLast->m_featRemainingUses.m_pArray[i]->m_nFeat)
            return 1;

        if (pCreature->m_pStats->m_featRemainingUses.m_pArray[i]->m_nUsesUsed
            != pLast->m_featRemainingUses.m_pArray[i]->m_nUsesUsed)
            return 1;
    }

    return 0;
}

int CSWSMessage::ComputeLastUpdate_GuiKnownSpellUses(CSWSCreature* pCreature,
                                                     CSWSPlayerLastUpdateObject* pLast)
{
    for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++)
    {
        if (CSWClass::IsJedi(pCreature->m_pStats->GetClass(static_cast<unsigned char>(i))))
        {
            if (pLast->GetKnownSpellUsesLeft(static_cast<unsigned char>(i), 0)
                != pCreature->m_pStats->GetSpellsPerDayLeft(static_cast<unsigned char>(i), 0))
                return 1;
        }
    }

    return 0;
}

// Diffs the live known-spell lists against the snapshot, one merge per spellcasting class, and
// files the spells to delete and to add back into the snapshot. Returns 1 when the client's
// known-spell panel needs re-sending.
int CSWSMessage::ComputeLastUpdate_GuiKnownSpells(CSWSCreature* pCreature,
                                                  CSWSPlayerLastUpdateObject* pLast)
{
    CSWSCreatureStats* pStats = pCreature->m_pStats;

    for (unsigned char nClass = 0; nClass < 2; nClass++)
    {
        unsigned char nLive = 0;
        CExoArrayList<unsigned int>* pDeleteList = &pLast->m_spellDeleteList[nClass][0];
        CExoArrayList<unsigned int>* pAddList = &pLast->m_spellAddList[nClass][0];

        for (unsigned char i = 0; i < pLast->GetNumberKnownSpells(nClass, 0); i++)
        {
            int nSnapshotSpell = pLast->GetKnownSpell(nClass, 0, i);

            if (nSnapshotSpell == pStats->GetKnownSpell(nClass, 0, nLive))
                nLive++;
            else
                pDeleteList->Add(nSnapshotSpell);
        }

        // Everything the merge did not consume on the live side is new to the client.
        for (; nLive < pStats->GetNumberKnownSpells(nClass, 0); nLive++)
            pAddList->Add(pStats->GetKnownSpell(nClass, 0, nLive));
    }

    // The add list is shipped before the delete list.
    for (unsigned char nClass = 0; nClass <= 1; nClass++)
    {
        if (pLast->m_spellAddList[nClass][0].m_count != 0
            || pLast->m_spellDeleteList[nClass][0].m_count != 0)
            return 1;
    }

    return 0;
}

// Refreshes the snapshot's spell-like-ability list; despite the name it is a writer, not a differ.
void CSWSMessage::ComputeLastUpdate_StoreUpdateSpellLikeAbility(CSWSCreature* pCreature,
                                                                CSWSPlayerLastUpdateObject* pLast)
{
    CExoArrayList<CSWSStats_SpellLikeAbility>* pSrc = pCreature->m_pStats->m_pSpellLikeAbilityList;
    int nCount = pSrc->m_count;
    CExoArrayList<CSWSStats_SpellLikeAbility>* pDst = pLast->m_pSpellLikeAbilityList;

    if (pDst == 0)
    {
        pDst = new CExoArrayList<CSWSStats_SpellLikeAbility>;
        pLast->m_pSpellLikeAbilityList = pDst;
    }

    if (nCount != 0)
    {
        pDst->Allocate(nCount);
        pDst->m_count = nCount;

        CSWSStats_SpellLikeAbility* paDst = pDst->m_pArray;
        CSWSStats_SpellLikeAbility* paSrc = pSrc->m_pArray;

        for (int i = 0; i < nCount; i++)
        {
            paDst[i].m_nCasterLevel = paSrc[i].m_nCasterLevel;
            paDst[i].m_nSpellId     = paSrc[i].m_nSpellId;
            paDst[i].m_nReady       = paSrc[i].m_nReady;
        }
    }
}

// Packs the party-state byte: the detect mode, the stealth latch and one bit per reported
// combat mode.
char CSWSMessage::ComputeLastUpdate_AssociateState(CSWSCreature* pCreature)
{
    char nState = 0;

    if (pCreature->GetDetectMode() == 1)
        nState |= MSG_ASSOCIATE_STATE_DETECT;
    if (pCreature->m_stealthMode == 1)
        nState |= MSG_ASSOCIATE_STATE_STEALTH;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_PARRY)
        nState |= MSG_ASSOCIATE_STATE_PARRY;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_POWER_ATTACK)
        nState |= MSG_ASSOCIATE_STATE_POWER_ATTACK;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK)
        nState |= MSG_ASSOCIATE_STATE_IMPROVED_POWER_ATTACK;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_FLURRY)
        nState |= MSG_ASSOCIATE_STATE_FLURRY;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_RAPID_SHOT)
        nState |= MSG_ASSOCIATE_STATE_RAPID_SHOT;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_COUNTERSPELL)
        nState |= MSG_ASSOCIATE_STATE_COUNTERSPELL;

    return nState;
}

// True when the creature's visibility list differs from the player's snapshot.
int CSWSMessage::ComputeVisibilityLists(CSWSCreature* pCreature, CSWSPlayerLastUpdateObject* pLast)
{
    int nCount = pCreature->m_visibilityList.m_count;
    if (nCount != pLast->m_visibilityList.m_count)
        return 1;

    for (int i = 0; i < nCount; i++)
    {
        CSWVisibilityNode* pLive = pCreature->m_visibilityList.m_pArray[i];
#ifdef K2_FIX_ENGINE_BUGS
        CSWVisibilityNode* pMatch = nullptr;
        for (int j = 0; j < nCount; j++)
        {
            CSWVisibilityNode* pSnap = pLast->m_visibilityList.m_pArray[j];
            if (pLive->m_objectId == pSnap->m_objectId)
                pMatch = pSnap;
        }

        if (pMatch == nullptr)
            return 1;
#else
        CSWVisibilityNode* pSnap = pLast->m_visibilityList.m_pArray[i];

        int bFound = 0;
        CSWVisibilityNode* pMatch;
        for (int j = 0; j < nCount; j++)
        {
            if (pLive->m_objectId == pSnap->m_objectId)  // the snapshot is not indexed by j
            {
                bFound = 1;
                pMatch = pSnap;
            }
        }

        if (bFound != 1)
            return 1;
#endif

        unsigned char nDelta = pLive->m_flags ^ pMatch->m_flags;
        if (nDelta & 3)
            return 1;
        if (nDelta & kVisibleInactive)
            return 1;
    }

    return 0;
}

// Forwards to the snapshotter with the first two arguments swapped.
void CSWSMessage::StoreValuesInLastUpdateObject(CSWSPlayer* pPlayer, CLastUpdateObject* pLast,
                                                CSWSObject* pObject, unsigned int a, unsigned int b)
{
    UpdateLastUpdateObject(pPlayer, pObject, pLast, a);
    UpdateLastUpdateObjectAppearance(pObject, pLast, b);
}

// Fills the player's snapshot record for one object from the live object; the flag word selects
// which field groups to ship.
void CSWSMessage::UpdateLastUpdateObject(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                         CLastUpdateObject* pLast, unsigned int a)
{
    pLast->m_objectType = pObject->m_objectType;

    if (a & MSG_OBJECT_UPDATE_ANIMATION)
    {
        pLast->m_nAnimation = pObject->m_nAnimation;
        pLast->m_fAnimationSpeed = pObject->m_fAnimationSpeed;
    }

    if (a & MSG_OBJECT_UPDATE_POSITION)
    {
        pLast->m_fPositionX = pObject->m_position.x;
        pLast->m_fPositionY = pObject->m_position.y;
        pLast->m_fPositionZ = pObject->m_position.z;
        if (pObject->GetArea())
            pLast->m_oidArea = static_cast<CSWSArea*>(pObject->GetArea())->m_objectId;
        else
            pLast->m_oidArea = OBJECT_INVALID;
    }

    if (a & MSG_OBJECT_UPDATE_ORIENTATION)
    {
        pLast->m_fOrientationX = pObject->m_orientation.x;
        pLast->m_fOrientationY = pObject->m_orientation.y;
        pLast->m_fOrientationZ = pObject->m_orientation.z;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
            pLast->m_oidLockOrientationTo = pObject->AsSWSCreature()->GetLockOrientationToObject();
        if (pObject->m_nAnimation == SERVER_ANIMATION_ATTACK_2 || pObject->m_nAnimation == SERVER_ANIMATION_ATTACK)
            pLast->m_oidAttackTarget = pObject->AsSWSCreature()->m_oidAttackTarget;
    }

    if (a & MSG_OBJECT_UPDATE_PORTRAIT)
    {
        pLast->m_nPortraitId = pObject->GetPortraitId();
        pLast->m_portrait = pObject->GetPortrait();
    }

    if (a & MSG_OBJECT_UPDATE_VISUAL_EFFECTS)
        AssignVisualEffectLists(&pLast->m_visualEffects, &pObject->m_visualEffects);

    if (a & MSG_OBJECT_UPDATE_STATE)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
        {
            AssignCreatureLists(&pObject->AsSWSDoor()->m_objectsAtDoor, &pLast->m_lstObjectsOnObject);
            pLast->m_bTrapFlag = pObject->AsSWSDoor()->m_bTrapFlag;
            pLast->m_bMineFlagged = static_cast<unsigned char>(pObject->AsSWSDoor()->m_bMineFlagged);
            pLast->m_bLocked = pObject->AsSWSDoor()->m_bLocked;
            pLast->m_bPlot = pObject->AsSWSDoor()->m_bPlot;
        }
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            AssignCreatureLists(&pObject->AsSWSPlaceable()->m_objectsOnPlaceable,
                                &pLast->m_lstObjectsOnObject);
            pLast->m_bTrapFlag = pObject->AsSWSPlaceable()->m_bTrapFlag;
            pLast->m_bMineFlagged = static_cast<unsigned char>(pObject->AsSWSPlaceable()->m_bMineFlagged);
            pLast->m_bLocked = pObject->AsSWSPlaceable()->m_bLocked;
            pLast->m_bPlot = pObject->AsSWSPlaceable()->m_bPlot;
        }
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        {
            if (pObject->AsSWSTrigger()->m_bTrap)
            {
                pLast->m_nFaction = pObject->AsSWSTrigger()->m_nFaction;
                pLast->m_bMineFlagged = pObject->AsSWSTrigger()->m_bMineFlagged;
            }
        }
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE && (a & MSG_OBJECT_UPDATE_POSITION))
    {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        pLast->m_nFadeOutTime = pPlaceable->m_nFadeOutTime;
        pLast->m_bDeleteNoFade = pPlaceable->m_bDeleteNoFade;
        pLast->m_bKeepCorpse = pPlaceable->m_bKeepCorpse;
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        CSWSCreature* pCreature = pObject->AsSWSCreature();

        if (a & MSG_OBJECT_UPDATE_ANIMATION)
            pLast->m_nClientPathId = static_cast<unsigned char>(pCreature->m_pPathfindInfo->m_nClientPathId);

        if (a & MSG_OBJECT_UPDATE_AI_STATE)
        {
            pLast->m_nAIState = pCreature->m_nAIState;
            pLast->m_nAIStateAction = pCreature->m_nAIStateAction;
            pLast->m_nActivityFlags = static_cast<unsigned short>(pCreature->m_activityFlags);
            pLast->m_oidAIStateTarget = pCreature->m_oidAIStateTarget;
            pLast->m_bCombatState = pCreature->m_bCombatState;
            pLast->m_nStealthMode = pCreature->m_stealthMode;
            if (pPlayer && pPlayer->GetGameObject() && pPlayer->GetGameObject()->AsSWSCreature())
                pLast->m_nReputation = pPlayer->GetGameObject()->AsSWSCreature()->GetAIStateReputation(pCreature->m_objectId);
            else
                pLast->m_nReputation = 2;
        }

        if (a & MSG_OBJECT_UPDATE_ANIMATION)
        {
            if (pCreature->AnimationStationary(static_cast<unsigned short>(pLast->m_nAnimation)) == 0)
            {
                if (pCreature->m_pPathfindInfo->m_paWayPoints)
                {
                    unsigned short n = static_cast<unsigned short>(pCreature->m_pPathfindInfo->m_nWayPointFloats / 2);
                    if (n)
                    {
                        pLast->m_fLastPathPointZ = 0.0f;
                        memcpy(&pLast->m_lastPathPoint, &pCreature->m_pPathfindInfo->m_paWayPoints[2 * n - 2],
                               sizeof(pLast->m_lastPathPoint));
                    }
                }
            }
        }

        if (a & MSG_OBJECT_UPDATE_MOVE_RATES)
        {
            pLast->m_fWalkRate = pCreature->GetWalkRate();
            pLast->m_fRunRate = pCreature->GetRunRate();
        }

        if (a & MSG_OBJECT_UPDATE_DAMAGE_LEVEL)
            pLast->m_nDamageLevel = pCreature->GetDamageLevel();

        if (a & MSG_OBJECT_UPDATE_HIT_POINTS)
        {
            pLast->m_nCurrentHitPoints = pCreature->GetCurrentHitPoints(1);
            pLast->m_nHitPoints = static_cast<unsigned short>(pCreature->m_hitPoints);
            pLast->m_nHitPointBonus = static_cast<unsigned short>(pCreature->m_hitPointBonus);
            pLast->m_nMaxHitPoints = pCreature->GetMaxHitPoints(1);
        }

        if (a & MSG_OBJECT_UPDATE_FORCE_POINTS)
        {
            pLast->m_nForcePoints = pCreature->m_pStats->m_nCurrentForcePoints;
            pLast->m_nBaseForcePoints = pCreature->m_pStats->m_nBaseForcePoints;
            pLast->m_nTemporaryForcePoints = pCreature->m_pStats->m_nTemporaryForcePoints;
            pLast->m_nMaxForcePoints = pCreature->GetMaxForcePoints();
        }

        if (a & MSG_OBJECT_UPDATE_PARTY)
        {
            pLast->m_bPlayerCharacter = pCreature->m_bPlayerCharacter;
            pLast->m_bAIStateControllable = (pCreature->m_nAIState >> 6) & 1;
            pLast->m_bSingletonParty = pCreature->GetFaction()->GetSingletonParty();
            pLast->m_bFactionLeader = (pCreature->GetFaction()->GetLeader() == pCreature->m_objectId);
            pLast->m_bInvitedToParty = pCreature->m_bInvitedToParty;
        }

        if (a & MSG_OBJECT_UPDATE_VISIBILITY)
        {
            CSWSCreature* pPlayerCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pPlayerCreature)
            {
                if (pPlayerCreature->m_objectId == pCreature->m_objectId)
                {
                    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
                    if (pModule)
                    {
                        pLast->m_bInvisible = pModule->m_playerList.Contains(pCreature->m_objectId);
                    }
                }
                else
                {
                    CSWVisibilityNode* pElement = pCreature->GetVisibleListElement(pPlayerCreature->m_objectId);
                    if (pElement)
                    {
                        pLast->m_bHeard = (pElement->m_flags >> 1) & 1;
                        pLast->m_bSeen = pElement->m_flags & 1;
                        pLast->m_bInvisible = (pElement->m_flags >> 4) & 1;
                    }
                    else
                    {
                        pLast->m_bSeen = 0;
                        pLast->m_bHeard = 0;
                        pLast->m_bInvisible = 0;
                    }
                }
            }
        }

        if (a & MSG_OBJECT_UPDATE_ASSOCIATE_STATE)
        {
            pLast->m_nAssociateState = ComputeLastUpdate_AssociateState(pCreature);
        }

        if (a & MSG_OBJECT_UPDATE_IDENTITY)
        {
            pLast->m_nRace = pCreature->m_pStats->m_nRace;
            pLast->m_sSubrace = pCreature->m_pStats->m_sSubrace;
            pLast->m_nSubraceIndex = pCreature->m_pStats->m_nSubraceIndex;
            pLast->m_nGender = pCreature->m_pStats->m_nGender;
            pLast->m_nAlignment = pCreature->m_pStats->m_nAlignment;
#if !KOTOR1
            pLast->m_nBaseNPCAlignment = pCreature->m_pStats->m_nBaseNPCAlignment;
            pLast->m_nStanceId = pCreature->m_stanceId;
            pLast->m_sTag = pCreature->m_sTag;
#endif
            pLast->m_bDeadSelectable = pCreature->m_bDeadSelectable;
            if (pCreature->m_pStats->m_numClasses)
            {
                for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++)
                {
                    // The bounds guard is redundant but cannot be removed, because the index is
                    // truncated to a byte, so the guard cannot be proven true.
#if KOTOR1  // K1: CSWSCreatureStats::GetClass is out of line
                    reinterpret_cast<unsigned char*>(&pLast->m_nClasses)[i] = pCreature->m_pStats->GetClass(static_cast<unsigned char>(i));
#else
                    reinterpret_cast<unsigned char*>(&pLast->m_nClasses)[i] = (static_cast<unsigned char>(i) < pCreature->m_pStats->m_numClasses)
                        ? pCreature->m_pStats->m_classInfo[i].m_class : CLASS_TYPE_INVALID;
#endif
                    reinterpret_cast<unsigned char*>(&pLast->m_nClassLevels)[i] = pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0);
                }
            }
        }

        if (a & MSG_OBJECT_UPDATE_REMOVAL)
        {
            pLast->m_nFadeOutTime = pCreature->m_nFadeOutTime;
            pLast->m_bDeleteNoFade = pCreature->m_bDeleteNoFade;
            pLast->m_bKeepCorpse = pCreature->m_bKeepCorpse;
        }
    }
}

// Copies the creature's live appearance block into the player's snapshot, field by field - a
// whole-struct assignment would copy the alignment hole.
#if KOTOR1  // K1 calls this from both snapshot paths instead of inlining it
__attribute__((noinline))
#endif
void CSWSMessage::UpdateLastUpdateObjectAppearance(CSWSObject* pObject, CLastUpdateObject* pLast,
                                                   unsigned int nMask)
{
    if (pObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE)
        return;

    CSWSCreature* pCreature = pObject->AsSWSCreature();

    // The name update is the only part of the snapshot that comes off the stats block.
    if (nMask & MSG_APPEARANCE_UPDATE_NAME)
    {
        pLast->m_firstName = pCreature->m_pStats->m_firstName;
        pLast->m_lastName = pCreature->m_pStats->m_lastName;
    }

    pLast->m_appearance.m_itemId[0] = pCreature->m_appearance.m_itemId[0];
    pLast->m_appearance.m_itemId[1] = pCreature->m_appearance.m_itemId[1];
    pLast->m_appearance.m_headId = pCreature->m_appearance.m_headId;
    pLast->m_appearance.m_nPhenotype = pCreature->m_appearance.m_nPhenotype;
    pLast->m_appearance.m_nGender = pCreature->m_appearance.m_nGender;
    pLast->m_appearance.m_nColorSkin = pCreature->m_appearance.m_nColorSkin;
    pLast->m_appearance.m_nColorHair = pCreature->m_appearance.m_nColorHair;
    pLast->m_appearance.m_nColorTattoo1 = pCreature->m_appearance.m_nColorTattoo1;
    pLast->m_appearance.m_nColorTattoo2 = pCreature->m_appearance.m_nColorTattoo2;
    pLast->m_appearance.m_nAppearanceHead = pCreature->m_appearance.m_nAppearanceHead;
    pLast->m_appearance.m_nTail = pCreature->m_appearance.m_nTail;
    pLast->m_appearance.m_nWings = pCreature->m_appearance.m_nWings;
    pLast->m_appearance.m_bUseBackupHead = pCreature->m_appearance.m_bUseBackupHead;
    pLast->m_appearance.m_nDuplicatingHead = pCreature->m_appearance.m_nDuplicatingHead;
}

// Returns the appearance change bitmask WriteGameObjUpdate_UpdateAppearance dispatches on, the
// mirror of UpdateLastUpdateObjectAppearance.
int CSWSMessage::ComputeAppearanceUpdateRequired(CSWSObject* pObject, CLastUpdateObject* pLast)
{
    if (pObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    pCreature->UpdateAppearanceForEquippedItems();

    int nMask = 0;
    if (pLast->m_appearance.m_headId != pCreature->m_appearance.m_headId)
        nMask |= MSG_APPEARANCE_UPDATE_APPEARANCE_ID;
    if (pLast->m_appearance.m_nPhenotype != pCreature->m_appearance.m_nPhenotype)
        nMask |= MSG_APPEARANCE_UPDATE_PHENOTYPE;
    if (pLast->m_appearance.m_nGender != pCreature->m_appearance.m_nGender)
        nMask |= MSG_APPEARANCE_UPDATE_GENDER;
    if (pLast->m_appearance.m_nColorSkin != pCreature->m_appearance.m_nColorSkin)
        nMask |= MSG_APPEARANCE_UPDATE_SKIN_COLOR;
    if (pLast->m_appearance.m_nColorHair != pCreature->m_appearance.m_nColorHair)
        nMask |= MSG_APPEARANCE_UPDATE_HAIR_COLOR;
    if (pLast->m_appearance.m_nColorTattoo1 != pCreature->m_appearance.m_nColorTattoo1)
        nMask |= MSG_APPEARANCE_UPDATE_TATTOO_COLOR_1;
    if (pLast->m_appearance.m_nColorTattoo2 != pCreature->m_appearance.m_nColorTattoo2)
        nMask |= MSG_APPEARANCE_UPDATE_TATTOO_COLOR_2;
    if (pLast->m_appearance.m_nAppearanceHead != pCreature->m_appearance.m_nAppearanceHead)
        nMask |= MSG_APPEARANCE_UPDATE_HEAD;
    if (pLast->m_appearance.m_bUseBackupHead != pCreature->m_appearance.m_bUseBackupHead)
        nMask |= MSG_APPEARANCE_UPDATE_USE_BACKUP_HEAD;
    if (pLast->m_appearance.m_nDuplicatingHead != pCreature->m_appearance.m_nDuplicatingHead)
        nMask |= MSG_APPEARANCE_UPDATE_DUPLICATING_HEAD;
    if (pLast->m_appearance.m_nTail != pCreature->m_appearance.m_nTail)
        nMask |= MSG_APPEARANCE_UPDATE_TAIL;
    if (pLast->m_appearance.m_nWings != pCreature->m_appearance.m_nWings)
        nMask |= MSG_APPEARANCE_UPDATE_WINGS;
    if (pLast->m_appearance.m_itemId[0].m_a != pCreature->m_appearance.m_itemId[0].m_a ||
        pLast->m_appearance.m_itemId[0].m_b != pCreature->m_appearance.m_itemId[0].m_b ||
        pLast->m_appearance.m_itemId[1].m_a != pCreature->m_appearance.m_itemId[1].m_a ||
        pLast->m_appearance.m_itemId[1].m_b != pCreature->m_appearance.m_itemId[1].m_b)
        nMask |= MSG_APPEARANCE_UPDATE_EQUIPMENT;
    if (pLast->m_nSoundSetFile != pCreature->m_nSoundSetFile)
        nMask |= MSG_APPEARANCE_UPDATE_SOUND_SET;

    return nMask;
}

int CSWSMessage::WriteGameObjUpdate_MinorGUIPanels(CSWSPlayer* pPlayer)
{
    int nResult;
#ifdef K2_FIX_ENGINE_BUGS
    nResult = 0;
#endif

    CGameObject* pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return nResult;

    if (pObject->AsSWSCreature() == 0)
        return nResult;

    int nUpdate = pPlayer->m_pCharSheetGUI->ComputeCharacterSheetUpdateRequired(pPlayer);
    if (nUpdate == 0)
        return nResult;

    WriteGameObjUpdate_CharacterSheet(pPlayer, nUpdate);
    return nResult;
}

// Ships the character-sheet fields the client's mask says changed, then hands the same mask to
// the stats writer so the client's snapshot stays in step.
void CSWSMessage::WriteGameObjUpdate_CharacterSheet(CSWSPlayer* pPlayer, unsigned int nMask)
{
    CSWSPlayerCharSheetGUI* pGUI = pPlayer->m_pCharSheetGUI;
    CSWCreatureStatsUpdate* pStatsUpdate = pGUI->m_pStatsUpdate;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pGUI->m_objectId);
    CSWSCreatureStats* pStats = pCreature->m_pStats;

    WriteCHAR('G', 8);
    WriteCHAR('S', 8);
    WriteOBJECTIDServer(pCreature->m_objectId);
    WriteDWORD(nMask, 32);

    if (nMask & MSG_CHARACTER_SHEET_ABILITIES)
    {
        WriteBYTE(pStats->GetSTRStat(), 8);
        WriteBYTE(pStats->GetDEXStat(), 8);
        WriteBYTE(pStats->GetCONStat(), 8);
        WriteBYTE(pStats->GetINTStat(), 8);
        WriteBYTE(pStats->GetWISStat(), 8);
        WriteBYTE(pStats->GetCHAStat(), 8);
        WriteCHAR(pStats->GetTotalSTRBonus(), 8);
        WriteCHAR(pStats->GetTotalDEXBonus(), 8);
        WriteCHAR(pStats->GetTotalCONBonus(), 8);
        WriteCHAR(pStats->GetTotalINTBonus(), 8);
        WriteCHAR(pStats->GetTotalWISBonus(), 8);
        WriteCHAR(pStats->GetTotalCHABonus(), 8);
        WriteCHAR(pStats->m_nSTRBase, 8);
        WriteCHAR(pStats->m_nDEXBase, 8);
        WriteCHAR(pStats->m_nCONBase, 8);
        WriteCHAR(pStats->m_nINTBase, 8);
        WriteCHAR(pStats->m_nWISBase, 8);
        WriteCHAR(pStats->m_nCHABase, 8);
        WriteCHAR(static_cast<char>(pStats->GetDEXMod(1) - pStats->GetDEXMod(0)), 8);
    }
    if (nMask & MSG_CHARACTER_SHEET_FORTITUDE_SAVE)
        WriteCHAR(pStats->GetFortSavingThrow(0), 8);
    if (nMask & MSG_CHARACTER_SHEET_WILL_SAVE)
        WriteCHAR(pStats->GetWillSavingThrow(0), 8);
    if (nMask & MSG_CHARACTER_SHEET_REFLEX_SAVE)
        WriteCHAR(pStats->GetReflexSavingThrow(0), 8);
    if (nMask & MSG_CHARACTER_SHEET_EXPERIENCE)
        WriteDWORD(pStats->m_nExperience, 32);
    if (nMask & MSG_CHARACTER_SHEET_BASE_ATTACK_BONUS)
    {
        WriteBYTE(static_cast<unsigned char>(pStats->GetBaseAttackBonus()), 8);
        WriteBOOL(pStats->GetUseMonkAttackTables(0));
    }
    if (nMask & MSG_CHARACTER_SHEET_COMBAT_INFORMATION)
    {
        CCombatInformation* pCombat = pStats->m_pCombatInformation;
        WriteBYTE(pCombat->m_nNumOnHandAttacks, 3);
        WriteCHAR(pCombat->m_nAttackModOnHand, 8);
        WriteCHAR(pCombat->m_nStrDamageModOnHand, 8);
        WriteCHAR(pCombat->m_nForceResistance, 8);
        WriteBYTE(pCombat->m_nArcaneSpellFailure, 7);
        WriteBYTE(pCombat->m_bArmorCheckPenalty, 5);
        WriteBYTE(pCombat->m_nDefaultDamageDie, 5);
        WriteBYTE(pCombat->m_nDefaultDamageDiceCount, 5);
        WriteBYTE(pCombat->m_nCritRangeOnHand, 4);
        WriteBYTE(pCombat->m_nCritMultiplierOnHand, 3);
        WriteBOOL(pCombat->m_bOffHandWeaponEquipped);
        if (pCombat->m_bOffHandWeaponEquipped == 1)
        {
            WriteCHAR(pCombat->m_nAttackModOffHand, 8);
            WriteCHAR(pCombat->m_nStrDamageModOffHand, 8);
            WriteBYTE(pCombat->m_nCritRangeOffHand, 4);
            WriteBYTE(pCombat->m_nCritMultiplierOffHand, 3);
        }
        WriteBYTE(static_cast<unsigned char>(pCombat->m_attackBonusMods.m_count), 8);
        for (int i = 0; i < pCombat->m_attackBonusMods.m_count; i++)
        {
            WriteCHAR(pCombat->m_attackBonusMods.m_pArray[i]->m_cValue, 8);
            WriteBYTE(pCombat->m_attackBonusMods.m_pArray[i]->m_nType, 3);
            if (pCombat->m_attackBonusMods.m_pArray[i]->m_nClassReq == g_pRules->m_nNumRaces)
                WriteBOOL(0);
            else
            {
                WriteBOOL(1);
                WriteBYTE(pCombat->m_attackBonusMods.m_pArray[i]->m_nClassReq, 8);
            }
            if (pCombat->m_attackBonusMods.m_pArray[i]->m_nFlag != 0)
            {
                WriteBOOL(1);
                WriteBYTE(pCombat->m_attackBonusMods.m_pArray[i]->m_nFlag, 3);
            }
            else
                WriteBOOL(0);
        }
        WriteBYTE(static_cast<unsigned char>(pCombat->m_damageBonusMods.m_count), 8);
        for (int i = 0; i < pCombat->m_damageBonusMods.m_count; i++)
        {
            WriteCHAR(pCombat->m_damageBonusMods.m_pArray[i]->m_cValue, 8);
            WriteBYTE(pCombat->m_damageBonusMods.m_pArray[i]->m_nModifierType, 4);
            WriteBYTE(pCombat->m_damageBonusMods.m_pArray[i]->m_nType, 3);
            if (pCombat->m_damageBonusMods.m_pArray[i]->m_nClassReq == g_pRules->m_nNumRaces)
                WriteBOOL(0);
            else
            {
                WriteBOOL(1);
                WriteBYTE(pCombat->m_damageBonusMods.m_pArray[i]->m_nClassReq, 8);
            }
            if (pCombat->m_damageBonusMods.m_pArray[i]->m_nFlag != 0)
            {
                WriteBOOL(1);
                WriteBYTE(pCombat->m_damageBonusMods.m_pArray[i]->m_nFlag, 3);
            }
            else
                WriteBOOL(0);
        }
    }
    if (nMask & MSG_CHARACTER_SHEET_ARMOR_CLASS)
        WriteSHORT(pCreature->GetArmorClass(), 16);
    if (nMask & MSG_CHARACTER_SHEET_SKILLS)
    {
        WriteDWORD(static_cast<unsigned int>(pStatsUpdate->m_nSkillsChanged), g_pRules->m_nNumSkills);
        for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
        {
            if (static_cast<unsigned int>(pStatsUpdate->m_nSkillsChanged) & (1 << i))
            {
                WriteCHAR(pStats->GetSkillRank(i, 0, 0), 8);
                WriteBOOL(pStats->GetCanUseSkill(i));
            }
        }
    }
    if (nMask & MSG_CHARACTER_SHEET_FEATS)
    {
        WriteWORD(static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count), 16);
        for (unsigned short i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); i++)
            WriteWORD(pCreature->m_pStats->GetFeat(i), 16);
        WriteWORD(static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count), 16);
        for (unsigned short i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); i++)
            WriteWORD(pCreature->m_pStats->GetBonusFeat(i), 16);
    }

    pCreature->m_pStats->UpdateLastStatsObject(nMask, pStatsUpdate);
}

// Stamps the standard three-byte header over an already-built payload and ships it, the shared hub
// for the payload sends. An empty payload gets a bare three-byte header, allocated and freed here.
int CSWSMessage::SendServerToPlayerMessage(unsigned int nPlayerId, unsigned char nCategory,
                                           unsigned char nSubOpcode, unsigned char* pMsg,
                                           unsigned int nSize)
{
    unsigned int nSend = nSize;
    if (nSize == 0)
    {
        pMsg = new unsigned char[3];
        m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
        nSend = 3;
    }

    int bSent = 0;
    if (pMsg != 0)
    {
        pMsg[0] = MSG_SERVER_TO_PLAYER;
        pMsg[1] = nCategory;
        pMsg[2] = nSubOpcode;

        bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   nSend, 0);
        if (nSize == 0)
            delete[] pMsg;
    }

    return bSent;
}

int CSWSMessage::SendServerToPlayerInventory_Drop(unsigned int nPlayerId,
                                                  unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_DROP, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerInventory_DropCancel(unsigned int nPlayerId,
                                                        unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_DROP_CANCEL,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerInventory_ConfirmDrop(unsigned int nPlayerId,
                                                         unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_CONFIRM_DROP,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerInventory_Pickup(unsigned int nPlayerId,
                                                    unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_PICKUP, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerInventory_PickupCancel(unsigned int nPlayerId,
                                                          unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_PICKUP_CANCEL,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerInventory_UnequipCancel(unsigned int nPlayerId,
                                                           unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_UNEQUIP_CANCEL,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerGameObjUpdate_ObjControl(unsigned int nPlayerId,
                                                            unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                     MSG_GAME_OBJECT_UPDATE_OBJECT_CONTROL, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerCheatNasty(unsigned int nPlayerId,
                                              unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHEAT, MSG_CHEAT_NASTY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerGUICharacterSheet_NotPermitted(unsigned int nPlayerId,
                                                                  unsigned long long objectId)
{
    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_CHARACTER_SHEET,
                                     MSG_GUI_CHARACTER_SHEET_NOT_PERMITTED, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientMusicPlay(unsigned long long nPlayerId, int bPlay)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_MUSIC_PLAY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientBattleMusicPlay(unsigned long long nPlayerId, int bPlay)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_BATTLE_MUSIC_PLAY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientSoundLoopPlay(unsigned long long nPlayerId, int bPlay)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_SOUND_LOOP_PLAY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientMusicSetDelay(unsigned long long nPlayerId, int nDelay)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nDelay, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_MUSIC_SET_DELAY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientBattleMusicChange(unsigned long long nPlayerId, int nTrack)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nTrack, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_BATTLE_MUSIC_CHANGE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerSoundObject_Play(CSWSPlayer* pPlayer,
                                                    unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SOUND, MSG_SOUND_OBJECT_PLAY,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerSoundObject_Stop(CSWSPlayer* pPlayer,
                                                    unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SOUND, MSG_SOUND_OBJECT_STOP,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerPlayerItemUpdate_DestroyItem(CSWSPlayer* pPlayer,
                                                        unsigned long long objectId)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                     MSG_GAME_OBJECT_UPDATE_DESTROY_ITEM, pMsg, nSize);
}

// The sub-opcode is a parameter here, and the send's result is discarded.
int CSWSMessage::SendServerToPlayerInventory_LearnScroll(unsigned int nPlayerId,
                                                         unsigned long long objectId,
                                                         unsigned char nSubOpcode)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, nSubOpcode, pMsg, nSize);
    return 1;
}

int CSWSMessage::SendServerToPlayerCheatPonyRide(unsigned int nPlayerId,
                                                 unsigned long long objectId)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(1);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHEAT, MSG_CHEAT_PONY_RIDE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerCheatRainOfCows(unsigned int nPlayerId,
                                                   unsigned long long objectId)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(1);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHEAT, MSG_CHEAT_RAIN_OF_COWS, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerShutDownServer(unsigned int nPlayerId,
                                                  unsigned int nSecondsRemaining)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nSecondsRemaining, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_SHUTDOWN_SERVER, MSG_SHUTDOWN_SERVER_REQUEST,
                              pMsg, nSize);
    return 1;
}

int CSWSMessage::SendServerToPlayerLogin_Fail(unsigned int nPlayerId, uint64_t nReason)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(static_cast<unsigned int>(nReason), 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_LOGIN, MSG_LOGIN_FAIL, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerLoadBar_StartStallEvent(unsigned int nStall)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nStall, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_LOAD_BAR,
                                     MSG_LOAD_BAR_START_STALL_EVENT, pMsg, nSize);
}

// The payload is the departing player's id; the first parameter is the recipient.
int CSWSMessage::SendServerToPlayerPlayerList_Delete(unsigned int nPlayerId, CSWSPlayer* pPlayer)
{
    unsigned int nDeparting = pPlayer->m_nPlayerId;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nDeparting, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_PLAYER_LIST, MSG_PLAYER_LIST_DELETE,
                                     pMsg, nSize);
}

// GetServerMode picks the sub-opcode: mode 2 ships 3, mode 1 ships 1, mode 0 ships 2, and any
// other mode sends nothing.
int CSWSMessage::SendServerToPlayerServerStatus_Status(unsigned int nPlayerId)
{
    short mode = g_pAppManager->m_pServerExoApp->GetServerMode();
    switch (mode) {
    case 0: {
        unsigned char* pMsg = new unsigned char[3];
        m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
        pMsg[0] = MSG_SERVER_TO_PLAYER;
        pMsg[1] = MSG_MAJOR_SERVER_STATUS;
        pMsg[2] = MSG_SERVER_STATUS_IDLE;
        int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                       3, 0);
        delete[] pMsg;
        return bSent;
    }
    case 1: {
        unsigned char* pMsg = new unsigned char[3];
        m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
        pMsg[0] = MSG_SERVER_TO_PLAYER;
        pMsg[1] = MSG_MAJOR_SERVER_STATUS;
        pMsg[2] = MSG_SERVER_STATUS_MODULE_LOADING;
        int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                       3, 0);
        delete[] pMsg;
        return bSent;
    }
    case 2: {
        unsigned char* pMsg = new unsigned char[3];
        m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
        pMsg[0] = MSG_SERVER_TO_PLAYER;
        pMsg[1] = MSG_MAJOR_SERVER_STATUS;
        pMsg[2] = MSG_SERVER_STATUS_MODULE_RUNNING;
        int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                       3, 0);
        delete[] pMsg;
        return bSent;
    }
    default:
        return 0;
    }
}

int CSWSMessage::SendServerToPlayerCheatDebugMode(int bOn)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bOn);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_CHEAT,
                                     MSG_CHEAT_DEBUG_MODE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerModule_EndStartNewModule(int bOk)
{
    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bOk);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_MODULE,
                                     MSG_MODULE_END_START_NEW_MODULE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerModule_SaveGameStatus(unsigned char nStatus)
{
    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nStatus, 8);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_MODULE,
                                     MSG_MODULE_SAVE_GAME_STATUS, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerModule_ExportReply(CSWSPlayer* pPlayer)
{
    pPlayer->PackCreatureIntoMessage();

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_MODULE,
                                     MSG_MODULE_EXPORT_REPLY, pMsg, nSize);
}

// Builds the id list of the exclusion list's active members, then hands it to the client app's
// SetPauseState.
int CSWSMessage::SendServerToPlayerModule_SetPauseState(unsigned char pauseFlag, int paused)
{
    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    CExoLinkedList<unsigned long long>* pExclusionList =
        g_pAppManager->m_pServerExoApp->GetActiveExclusionList();
    CExoLinkedList<unsigned long long>* pList = 0;
    if (pExclusionList != 0 && pExclusionList->m_pInternal->m_pHead != 0)
    {
        pList = new CExoLinkedList<unsigned long long>();
        CExoLinkedListNode* pNode = pExclusionList->m_pInternal->m_pHead;
        while (pNode != 0)
        {
            pList->AddTail(reinterpret_cast<unsigned long long*>(pClientExoApp->ServerToClientObjectId(
                reinterpret_cast<unsigned long long>(pExclusionList->GetAtPos(pNode)))));
            pExclusionList->GetNext(pNode);
        }
    }
    pClientExoApp->SetPauseState(pauseFlag, paused, pList);
    return 1;
}

// Fans the payload out to every player in range of the speaker, the broadcast twin of
// SendServerToPlayerChat_StrRef.
int CSWSMessage::SendServerToPlayerChatStrRefMessage(unsigned char nVolume,
                                                     unsigned long long speakerId,
                                                     unsigned int nStrRef)
{
    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pList == 0)
        return 0;

    if (nVolume != 8 && nVolume != 10)
    {
        if (nVolume != 9)
            return 0;
        return SendServerToPlayerChat_StrRef(CNetLayer::PLAYERID_ALL_PLAYERS, speakerId, nVolume,
                                             nStrRef) & 1;
    }

    CExoLinkedListNode* pNode = 0;
    float range = (nVolume == 8) ? CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE
                                 : CSWSArea::MAX_OBJECT_CHATWHISPER_DISTANCE;
    CSWArea* pSrcArea K2_UNSET;
    Vector srcPos;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()
            ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS)
    {
        if (pObject->AsSWSObject() != 0)
        {
            pSrcArea = pObject->AsSWSObject()->GetArea();
            srcPos = pObject->AsSWSObject()->m_position;
            pNode = pList->m_pInternal->m_pHead;
        }
    }

    int bSent = 1;
    while (pNode != 0)
    {
        CSWSPlayer* pOther = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
        if (pOther->GetGameObject() != 0)
        {
            Vector vDelta = pOther->GetGameObject()->m_position - srcPos;
            if (pSrcArea == pOther->GetGameObject()->GetArea())
            {
                float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                + vDelta.z * vDelta.z;
                if (fDistSq <= range * range)
                    bSent &= SendServerToPlayerChat_StrRef(pOther->m_nPlayerId, speakerId,
                                                           nVolume, nStrRef);
            }
        }
        pList->m_pInternal->GetNext(pNode);
    }
    return bSent;
}

// Routes one utterance, deciding who hears it by volume. 1 talk fans out to everyone within
// chatting distance of the speaker, 2 shout also broadcasts the visual within visible distance, 3
// whisper is the talk loop at whisper distance, 4 tell goes to the named player and the speaker's
// own client, 5 server-tell goes to the named player as a server message, 6 faction goes to every
// member of the speaker's faction, and 13 and 14 are the silent variants of 1 and 2. Any other
// volume sends nothing.
__attribute__((noinline)) int CSWSMessage::SendServerToPlayerChatMessage(unsigned char nChatVolume,
                                               unsigned long long speakerId,
                                               CExoString sText, unsigned int nTellPlayerId,
                                               const CExoString& sTellTargetName)
{
    K2_UNUSED(sTellTargetName);
    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pList == 0)
        return 0;

    switch (nChatVolume)
    {
    case MSG_CHAT_TALK:
    {
        CGameObject* pObject;
        int bSent = 0;
        CSWArea* pSrcArea K2_UNSET;
        Vector srcPos;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS
            && pObject != 0)
        {
            pSrcArea = static_cast<CSWSObject*>(pObject)->GetArea();
            srcPos = static_cast<CSWSObject*>(pObject)->m_position;
            static_cast<CSWSObject*>(pObject)->BroadcastDialog(sText,
                                                    CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE, 0);
            bSent = 1;
        }

        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        while (bSent && pNode != 0)
        {
            CSWSPlayer* pOther = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
            bSent = 1;   // keep looping by default; a send's result below may kill the loop
            if (pOther->GetGameObject() != 0)
            {
                Vector vDelta = pOther->GetGameObject()->m_position - srcPos;
                if (pSrcArea == pOther->GetGameObject()->GetArea())
                {
                    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                    + vDelta.z * vDelta.z;
                    if (fDistSq <= CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE
                                       * CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE)
                        bSent = SendServerToPlayerChat_Talk(pOther->m_nPlayerId, speakerId,
                                                            sText) & 1;
                }
            }
            pList->GetNext(pNode);
        }
        return bSent;
    }

    case MSG_CHAT_SHOUT:
    {
        int bSent = SendServerToPlayerChat_Shout(CNetLayer::PLAYERID_ALL_PLAYERS, speakerId,
                                                 sText) & 1;
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS
            && pObject != 0)
            static_cast<CSWSObject*>(pObject)->BroadcastDialog(sText,
                                                    CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE, 0);
        return bSent;
    }

    case MSG_CHAT_WHISPER:
    {
        CGameObject* pObject;
        int bSent = 0;
        CSWArea* pSrcArea K2_UNSET;
        Vector srcPos;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS
            && pObject->AsSWSCreature() != 0)
        {
            pSrcArea = pObject->AsSWSCreature()->GetArea();
            srcPos = pObject->AsSWSCreature()->m_position;
            static_cast<CSWSObject*>(pObject)->BroadcastDialog(sText,
                                                    CSWSArea::MAX_OBJECT_CHATWHISPER_DISTANCE, 0);
            bSent = 1;
        }

        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        while (bSent && pNode != 0)
        {
            CSWSPlayer* pOther = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
            bSent = 1;   // keep looping by default; a send's result below may kill the loop
            if (pOther->GetGameObject() != 0)
            {
                Vector vDelta = pOther->GetGameObject()->m_position - srcPos;
                if (pSrcArea == pOther->GetGameObject()->GetArea())
                {
                    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                    + vDelta.z * vDelta.z;
                    if (fDistSq <= CSWSArea::MAX_OBJECT_CHATWHISPER_DISTANCE
                                       * CSWSArea::MAX_OBJECT_CHATWHISPER_DISTANCE)
                        bSent = SendServerToPlayerChat_Whisper(pOther->m_nPlayerId, speakerId,
                                                               sText) & 1;
                }
            }
            pList->GetNext(pNode);
        }
        return bSent;
    }

    case MSG_CHAT_TELL:
    {
        CSWSPlayer* pClient = g_pAppManager->m_pServerExoApp
                                  ->GetClientObjectByObjectId(speakerId);
        int bClientMatch = 1;
        if (pClient != 0 && pClient->m_nPlayerId != nTellPlayerId)
            bClientMatch = SendServerToPlayerChat_Tell(pClient->m_nPlayerId, speakerId,
                                                       sText) & 1;
        return SendServerToPlayerChat_Tell(nTellPlayerId, speakerId, sText) & bClientMatch;
    }

    case MSG_CHAT_SERVER_TELL:
        SendServerToPlayerChat_ServerTell(nTellPlayerId, sText);
        return 1;

    case MSG_CHAT_PARTY:  // fan out to every member of the speaker's faction.
    {
        CSWSFaction* pFaction = g_pAppManager->m_pServerExoApp
                                    ->GetClientObjectByObjectId(speakerId)
                                    ->GetGameObject()
                                    ->AsSWSCreature()
                                    ->GetFaction();
        if (pFaction != 0)
            pFaction->SendChatMessage(speakerId, sText);
        return 1;
    }

    case MSG_CHAT_SILENT_TALK:
    {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS
            && pObject != 0)
            static_cast<CSWSObject*>(pObject)->BroadcastDialog(
                sText, CSWSArea::MAX_OBJECT_CHATTALKSILENT_DISTANCE, 1);
        return 1;
    }

    case MSG_CHAT_SILENT_SHOUT:
    {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS
            && pObject != 0)
            static_cast<CSWSObject*>(pObject)->BroadcastDialog(
                sText, CSWSArea::MAX_OBJECT_CHATSHOUTSILENT_DISTANCE, 0);
        return 1;
    }

    default:
        return 0;
    }
}

int CSWSMessage::SendServerToPlayerJournalDeleteWorldAll(CSWSPlayer* pPlayer)
{
    unsigned char* pMsg;
    unsigned int nSize = 1;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(1, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_DELETE_WORLD_ALL, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerCombatRoundStarted(CSWSPlayer* pPlayer)
{
    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(1, 8);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_COMBAT_ROUND,
                                     MSG_COMBAT_ROUND_STARTED, pMsg, nSize);
}

// Fans the quick-chat line out to every player in the speaker's area within chatting distance.
int CSWSMessage::SendServerToPlayerQuickChatMessage(unsigned long long objectId, unsigned short nChat)
{
    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pList == 0)
        return 0;

    CExoLinkedListNode* pNode;
    CGameObject* pObject;
    CSWArea* pArea = 0;
    int bResult = 0;
    Vector vPos;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()
            ->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
    {
        pArea = pObject->AsSWSCreature()->GetArea();
        vPos = pObject->AsSWSCreature()->m_position;
        bResult = 1;
    }

    pNode = pList->GetHeadPos();
    while (bResult != 0 && pNode != 0)
    {
        float fMaxDistSq = CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE * CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE;
        CSWSPlayer* pPlayer = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
        bResult = 1;   // keep looping by default; a send's result below may kill the loop
        if (pPlayer->GetGameObject() != 0)
        {
            Vector vPlayerPos = pPlayer->GetGameObject()->m_position;
            if (pArea == pPlayer->GetGameObject()->GetArea())
            {
                Vector vDelta = vPlayerPos - vPos;
                float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
                if (fDistSq <= fMaxDistSq)
                    bResult = SendServerToPlayerQuickChat(pPlayer->m_nPlayerId, objectId, nChat) & 1;
            }
        }
        pList->GetNext(pNode);
    }
    return bResult;
}

int CSWSMessage::SendServerToPlayerQuickChat(unsigned int nPlayerId, unsigned long long objectId,
                                            unsigned short nChat)
{
    CreateWriteMessage(14, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteWORD(nChat, 16);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_QUICK_CHAT, MSG_QUICK_CHAT_MESSAGE, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerJournalDeleteWorld(CSWSPlayer* pPlayer, int nId)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nId, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_DELETE_WORLD, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerJournalDeleteWorldStrref(CSWSPlayer* pPlayer, unsigned int nStrRef)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nStrRef, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_DELETE_WORLD_STRREF, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerMapPinEnabled(CSWSPlayer* pPlayer, unsigned long long objectId,
                                                int bEnabled)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(bEnabled);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_MAP_PIN, MSG_MAP_PIN_ENABLED,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientSoundLoopChange(unsigned long long nPlayerId, int bPlay,
                                                         int nTrack)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);
    WriteINT(nTrack, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_SOUND_LOOP_CHANGE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAmbientSoundVolumeChange(unsigned long long nPlayerId, int bPlay,
                                                           int nVolume)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);
    WriteINT(nVolume, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_SOUND_VOLUME, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerLoadBar_UpdateStallEvent(unsigned int a, unsigned int b)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(a, 32);
    WriteDWORD(b, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_LOAD_BAR,
                                     MSG_LOAD_BAR_UPDATE_STALL_EVENT, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerLoadBar_EndStallEvent(unsigned int a, unsigned int b)
{
    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(a, 32);
    WriteDWORD(b, 3);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(CNetLayer::PLAYERID_ALL_PLAYERS, MSG_MAJOR_LOAD_BAR,
                                     MSG_LOAD_BAR_END_STALL_EVENT, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerSaveLoad_Status(CSWSPlayer* pPlayer, unsigned char nStatus,
                                                  unsigned int nFlag)
{
    // The one member that null-checks its recipient before doing anything.
    if (pPlayer == 0)
        return 0;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nStatus, 8);
    WriteDWORD(nFlag, 4);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SAVE_LOAD,
                                     MSG_SAVE_LOAD_STATUS, pMsg, nSize);
}

#if KOTOR1
int CSWSMessage::SendServerToPlayerInventory_Unequip(unsigned int nPlayerId, unsigned long long objectId,
                                                    int bA)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(bA);
#else
int CSWSMessage::SendServerToPlayerInventory_Unequip(unsigned int nPlayerId, unsigned long long objectId,
                                                    int bA, int bB)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(bA);
    WriteBOOL(bB);
#endif

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_UNEQUIP, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerVoiceChat_Play(CSWSPlayer* pPlayer, unsigned long long objectId,
                                                 unsigned char nSound)
{
    unsigned char* pMsg;
    unsigned int nSize = 9;

    CreateWriteMessage(9, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBYTE(nSound, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_VOICE_CHAT,
                                     MSG_VOICE_CHAT_PLAY, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerGuiContainerObject_Open(CSWSPlayer* pPlayer, unsigned long long objectId,
                                                          int nFlag)
{
    unsigned char* pMsg;
    unsigned int nSize = 12;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteINT(nFlag, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GUI_CONTAINER,
                                     MSG_GUI_CONTAINER_OPEN, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerInventory_EquipCancel(unsigned int nPlayerId, unsigned long long objectId,
                                                        unsigned int nSlot)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteDWORD(nSlot, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    // The send's result is discarded, as is the client-object lookup that follows.
    SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_EQUIP_CANCEL, pMsg,
                              nSize);
    g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(nPlayerId, 0);
    return 1;
}

int CSWSMessage::SendServerToPlayerCamera_SetMode(CSWSPlayer* pPlayer, unsigned char nMode)
{
    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nMode, 8);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CAMERA, MSG_CAMERA_SET_MODE,
                                     pMsg, nSize);
}

#if KOTOR1
// K1: the message carries no intensity byte.
int CSWSMessage::SendServerToPlayerArea_Weather(CSWSPlayer* pPlayer, unsigned char nWeather, int bActive)
{
    CreateWriteMessage(5, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nWeather, 8);
    WriteBOOL(bActive);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_AREA, MSG_AREA_WEATHER, pMsg,
                                     nSize);
}
#else
int CSWSMessage::SendServerToPlayerArea_Weather(CSWSPlayer* pPlayer, unsigned char nWeather,
                                                int bActive, unsigned char nIntensity)
{
    CreateWriteMessage(5, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nWeather, 8);
    WriteBOOL(bActive);
    WriteBYTE(nIntensity, 8);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_AREA, MSG_AREA_WEATHER, pMsg,
                                     nSize);
}
#endif

#if KOTOR1
int CSWSMessage::SendServerToPlayerAmbientMusicChangeTrack(unsigned long long nPlayerId, int bPlay,
                                                           int nTrack)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);
    WriteINT(nTrack, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_MUSIC_CHANGE_TRACK, pMsg, nSize);
}
#else
int CSWSMessage::SendServerToPlayerAmbientMusicChangeTrack(unsigned long long nPlayerId, int bPlay,
                                                           int nTrack, int bLoop)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPlay);
    WriteINT(nTrack, 32);
    WriteBOOL(bLoop);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(static_cast<unsigned int>(nPlayerId), MSG_MAJOR_AMBIENT,
                                     MSG_AMBIENT_MUSIC_CHANGE_TRACK, pMsg, nSize);
}
#endif

// A "stop" event ships the flag alone; both payload fields sit inside its own `if`.
int CSWSMessage::SendServerToPlayerGuiTimingEvent(CSWSPlayer* pPlayer, int bStart,
                                                  unsigned char nType, unsigned int nDuration)
{
    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bStart);
    if (bStart)
    {
        WriteDWORD(nDuration, 32);
        WriteBYTE(nType, 8);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GUI_TIMING_EVENT,
                                     MSG_GUI_TIMING_EVENT_INFO, pMsg, nSize);
}

// The journal sync, in two arms selected by the player's
// CSWSPlayerJournalState::m_bFullUpdateNeeded: the full arm dumps every journal entry, the partial
// arm ships only the quests the journal's mutators queued and only the fields their recorded mask
// names. The word that leads each entry is a CSWSPlayerJournalQuestUpdates::m_updateFlags value in
// both arms.
int CSWSMessage::SendServerToPlayerJournalFullUpdate(CSWSPlayer* pPlayer)
{
    unsigned char* pMsg;
    unsigned int nSize;

    if (pPlayer->GetGameObject() == 0)
        return SendServerToPlayerJournalFullUpdateNotNeeded(pPlayer, 1);

    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return SendServerToPlayerJournalFullUpdateNotNeeded(pPlayer, 1);

    CSWSJournal* pJournal = pCreature->GetJournal();
    if (pJournal == 0)
        return SendServerToPlayerJournalFullUpdateNotNeeded(pPlayer, 1);

    CSWSPlayerJournalState* pState = pPlayer->m_pJournalState;

    int nCount;
    int nJournalCount K2_UNSET;
    int bFullUpdate;
    CExoArrayList<CSWSPlayerJournalQuestUpdates>* pUpdates;

    if (pState->m_bFullUpdateNeeded)
    {
        nCount = pJournal->m_entries.m_count;
        bFullUpdate = 1;
        pUpdates = 0;
    }
    else
    {
        nCount = pState->m_updates.m_count;
        if (nCount <= 0)
            return SendServerToPlayerJournalFullUpdateNotNeeded(pPlayer, 0);

        nJournalCount = pJournal->m_entries.m_count;
        bFullUpdate = 0;
        pUpdates = &pState->m_updates;
    }

    nSize = nCount * 34 + 12;
    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(1);
    WriteBOOL(bFullUpdate);
    WriteINT(nCount, 32);

    if (bFullUpdate)
    {
        for (int i = 0; i < nCount; i++)
        {
            WriteWORD(pJournal->m_entries.m_pArray[i].m_bNewUnobserved
                          ? MSG_JOURNAL_QUEST_ALL
                          : MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_OBSERVED,
                      16);
            WriteCExoString(pJournal->m_entries.m_pArray[i].m_sPlotId, 32);
            WriteCExoLocStringServer(pJournal->m_entries.m_pArray[i].m_locName, 0);
            WriteCExoLocStringServer(pJournal->m_entries.m_pArray[i].m_locText, 0);
            WriteDWORD(pJournal->m_entries.m_pArray[i].m_nDate, 32);
            WriteDWORD(pJournal->m_entries.m_pArray[i].m_nTime, 32);
            WriteINT(pJournal->m_entries.m_pArray[i].m_nState, 32);
            WriteDWORD(pJournal->m_entries.m_pArray[i].m_nPriority, 32);
            WriteWORD(pJournal->m_entries.m_pArray[i].m_nPicture, 16);
            WriteBOOL(pJournal->m_entries.m_pArray[i].m_bEnd);
            WriteINT(pJournal->m_entries.m_pArray[i].m_nPlotIndex, 32);
            WriteINT(pJournal->m_entries.m_pArray[i].m_nPlanetId, 32);
            WriteINT(pJournal->m_entries.m_pArray[i].m_nQuestEntryXP, 32);

            // Consume the flag the word above reported, since the client has now been told.
            pJournal->m_entries.m_pArray[i].m_bNewUnobserved = 0;
        }
    }
    else
    {
        for (int i = 0; i < nCount; i++)
        {
            unsigned short nFlags = pUpdates->m_pArray[i].m_updateFlags;

            // A destroyed quest has no entry left to look up; ship the tag and let the client drop its own copy.
            if (nFlags & MSG_JOURNAL_QUEST_REMOVED)
            {
                WriteWORD(nFlags, 16);
                WriteCExoString(pUpdates->m_pArray[i].m_sQuestTag, 32);
                continue;
            }

            int j;
            for (j = 0; j < nJournalCount; j++)
            {
                if (pJournal->m_entries.m_pArray[j].m_sPlotId == pUpdates->m_pArray[i].m_sQuestTag)
                    break;
            }

            if (j >= nJournalCount)
                continue;

            WriteWORD(nFlags, 16);
            WriteCExoString(pJournal->m_entries.m_pArray[j].m_sPlotId, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_NAME | MSG_JOURNAL_QUEST_ALL))
                WriteCExoLocStringServer(pJournal->m_entries.m_pArray[j].m_locName, 0);
            if (nFlags & (MSG_JOURNAL_QUEST_TEXT | MSG_JOURNAL_QUEST_ALL))
                WriteCExoLocStringServer(pJournal->m_entries.m_pArray[j].m_locText, 0);
            if (nFlags & (MSG_JOURNAL_QUEST_DATE | MSG_JOURNAL_QUEST_ALL))
                WriteDWORD(pJournal->m_entries.m_pArray[j].m_nDate, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_TIME | MSG_JOURNAL_QUEST_ALL))
                WriteDWORD(pJournal->m_entries.m_pArray[j].m_nTime, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_STATE | MSG_JOURNAL_QUEST_ALL))
                WriteINT(pJournal->m_entries.m_pArray[j].m_nState, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_PRIORITY | MSG_JOURNAL_QUEST_ALL))
                WriteDWORD(pJournal->m_entries.m_pArray[j].m_nPriority, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_PICTURE | MSG_JOURNAL_QUEST_ALL))
                WriteWORD(pJournal->m_entries.m_pArray[j].m_nPicture, 16);
            if (nFlags & (MSG_JOURNAL_QUEST_END | MSG_JOURNAL_QUEST_ALL))
                WriteBOOL(pJournal->m_entries.m_pArray[j].m_bEnd);
            if (nFlags & (MSG_JOURNAL_QUEST_PLOT_INDEX | MSG_JOURNAL_QUEST_ALL))
                WriteINT(pJournal->m_entries.m_pArray[j].m_nPlotIndex, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_PLANET | MSG_JOURNAL_QUEST_ALL))
                WriteINT(pJournal->m_entries.m_pArray[j].m_nPlanetId, 32);
            if (nFlags & (MSG_JOURNAL_QUEST_EXPERIENCE | MSG_JOURNAL_QUEST_ALL))
                WriteINT(pJournal->m_entries.m_pArray[j].m_nQuestEntryXP, 32);
        }
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    if (pUpdates != 0)
        pUpdates->Clear();

    pPlayer->m_pJournalState->m_bFullUpdateNeeded = 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_FULL_UPDATE, pMsg, nSize);
}

// The two arms build different messages. The "needed" arm ships a three-field constant payload, the
// other a single false bool.
int CSWSMessage::SendServerToPlayerJournalFullUpdateNotNeeded(CSWSPlayer* pPlayer, int bNeeded)
{
    if (bNeeded)
    {
        CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteBOOL(1);
        WriteBOOL(1);
        WriteINT(0, 32);
    }
    else
    {
        CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteBOOL(0);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_FULL_UPDATE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerChat_ServerTell(unsigned int nPlayerId, const CExoString& sText)
{
    CreateWriteMessage(sText.GetLength() + 4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sText, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_SERVER_TELL, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerChat_Talk(unsigned int nPlayerId, unsigned long long speakerId,
                                             const CExoString& sText)
{
    CreateWriteMessage(sText.GetLength() + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(speakerId);
    WriteCExoString(sText, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_TALK, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerChat_Whisper(unsigned int nPlayerId,
                                                unsigned long long speakerId,
                                                const CExoString& sText)
{
    CreateWriteMessage(sText.GetLength() + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(speakerId);
    WriteCExoString(sText, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_WHISPER, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerAIActionPlaySound(unsigned int nPlayerId,
                                                     unsigned long long objectId,
                                                     const CExoString& sSound)
{
    CreateWriteMessage(sSound.GetLength() + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteCExoString(sSound, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_AI_ACTION_PLAY_SOUND,
                                     pMsg, nSize);
}

// The one member whose write message is built against the recipient's own id rather than
// "all players".
int CSWSMessage::SendServerToPlayerSetCustomToken(unsigned int nPlayerId, int nToken,
                                                  const CExoString& sValue)
{
    unsigned char* pMsg;
    unsigned int nSize = sValue.GetLength() + 8;

    CreateWriteMessage(nSize, nPlayerId, 1);
    WriteINT(nToken, 32);
    WriteCExoString(sValue, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CUSTOM_TOKEN, MSG_CUSTOM_TOKEN_SET, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerJournalRemoveQuest(CSWSPlayer* pPlayer,
                                                      const CExoString& sQuestTag)
{
    unsigned char* pMsg;
    unsigned int nSize = sQuestTag.GetLength();

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sQuestTag, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_REMOVE_QUEST, pMsg, nSize);
}

// The sub-opcode is a parameter here, selecting which chat channel the strref goes out on.
int CSWSMessage::SendServerToPlayerChat_StrRef(unsigned int nPlayerId,
                                               unsigned long long speakerId,
                                               unsigned char nSubOpcode, unsigned int nStrRef)
{
    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(speakerId);
    WriteDWORD(nStrRef, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, nSubOpcode, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerArea_VisualEffect(CSWSPlayer* pPlayer, unsigned short nEffect,
                                                     Vector vPos)
{
    CreateWriteMessage(14, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteWORD(nEffect, 16);
    WriteFLOAT(vPos.x, 1.0f, 32);
    WriteFLOAT(vPos.y, 1.0f, 32);
    WriteFLOAT(vPos.z, 1.0f, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_AREA, MSG_AREA_VISUAL_EFFECT,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerSoundObject_ChangePosition(CSWSPlayer* pPlayer,
                                                              unsigned long long objectId,
                                                              Vector vPos)
{
    CreateWriteMessage(20, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteFLOAT(vPos.x, 1.0f, 32);
    WriteFLOAT(vPos.y, 1.0f, 32);
    WriteFLOAT(vPos.z, 1.0f, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SOUND,
                                     MSG_SOUND_OBJECT_CHANGE_POSITION, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerJournalUpdated(CSWSPlayer* pPlayer, int a, int bEnd,
                                                  CExoLocString& locName)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoLocStringServer(locName, 0);
    WriteBOOL(a);
    WriteBOOL(bEnd);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL, MSG_JOURNAL_UPDATED,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerJournalAddWorldStrref(CSWSPlayer* pPlayer, unsigned int nA,
                                                         unsigned int nB, unsigned int nC,
                                                         unsigned int nD)
{
    unsigned char* pMsg;
    unsigned int nSize = 16;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nA, 32);
    WriteDWORD(nB, 32);
    WriteDWORD(nC, 32);
    WriteDWORD(nD, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_ADD_WORLD_STRREF, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerDebugInfo_Area(CSWSPlayer* pPlayer, unsigned long long objectId)
{
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(objectId);
    if (pArea == 0)
        return 0;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteCExoString(pArea->m_sDebugName, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_DEBUG_INFO,
                                     MSG_DEBUG_INFO_AREA, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerExamineGui_PlaceableData(CSWSPlayer* pPlayer,
                                                            unsigned long long objectId)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    CreateWriteMessage(128, nPlayerId, 1);

    // A failed lookup skips only the writes; the empty message is still sent.
    CGameObject* pObject = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pObject)
            == CGameObjectArray::SUCCESS)
    {
        if (CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable())
        {
            WriteOBJECTIDServer(pPlaceable->m_objectId);
            WriteCExoLocStringServer(pPlaceable->m_description, 0);
        }
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_INFO_POPUP,
                                     MSG_GUI_INFO_POPUP_PLACEABLE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerExamineGui_DoorData(CSWSPlayer* pPlayer,
                                                       unsigned long long objectId)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    CreateWriteMessage(128, nPlayerId, 1);

    // A failed lookup skips only the writes; the empty message is still sent.
    CGameObject* pObject = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pObject)
            == CGameObjectArray::SUCCESS)
    {
        if (CSWSDoor* pDoor = pObject->AsSWSDoor())
        {
            WriteOBJECTIDServer(pDoor->m_objectId);
            WriteCExoLocStringServer(pDoor->m_description, 0);
        }
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_INFO_POPUP, MSG_GUI_INFO_POPUP_DOOR,
                                     pMsg, nSize);
}

// Sends the item examine panel, shipping the item id, a "usable" bit and a minimum equip level, the
// identified description when it has text and the base one otherwise, and the item's active
// property records. A failed lookup still builds and sends the message.
int CSWSMessage::SendServerToPlayerExamineGui_ItemData(CSWSPlayer* pPlayer,
                                                       unsigned long long objectId)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    CreateWriteMessage(128, nPlayerId, 1);

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
    if (pItem == 0) {
        WriteOBJECTIDServer(CGameObjectArray::INVALIDOBJID);
    } else {
        CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();

        WriteOBJECTIDServer(pItem->m_objectId);
        WriteBOOL(1);
        WriteBOOL(pCreature != 0 ? pCreature->CanUseItem(pItem, 0, 1) : 0);

        CExoLocString locDescription(pItem->m_descriptionIdentified);
        if (HasValidString(locDescription, 0)) {
            WriteCExoLocStringServer(locDescription, 0);
        } else {
            locDescription = pItem->m_description;
            WriteCExoLocStringServer(locDescription, 0);
        }

        int bMeetsLevel = 0;
        if (pCreature != 0)
            bMeetsLevel = (pCreature->m_pStats->GetLevel(0) >= pItem->GetMinEquipLevel());
        WriteBOOL(bMeetsLevel);
        if (bMeetsLevel == 0)
            WriteBYTE(pItem->GetMinEquipLevel(), 8);

        WriteINT(pItem->m_nCharges, 32);
        WriteINT(pItem->GetWeight(), 32);

        int nActive = pItem->m_nActiveProperties;
        int nPassive = pItem->m_nPassiveProperties;

        for (int i = 0; i < nPassive; i++)
            pItem->GetPassiveProperty(i);

        WriteWORD(static_cast<unsigned short>(nActive + nPassive), 16);

        for (int i = 0; i < nActive; i++) {
            CSWItemProperty* pProperty = pItem->GetActiveProperty(i);
            WriteWORD(pProperty->m_type, 16);
            WriteWORD(pProperty->m_subType, 16);
            WriteWORD(pProperty->m_costTableValue, 16);
            WriteBYTE(pProperty->m_nParam1Value, 8);
        }

        for (int i = 0; i < nPassive; i++)
            pItem->GetPassiveProperty(i);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_INFO_POPUP, MSG_GUI_INFO_POPUP_ITEM,
                                     pMsg, nSize);
}

// Sends the trap/mine examine panel, shipping the trap type byte and a rating of how hard the mine
// is to disarm for the examining creature, derived from its Demolitions skill rank. The three
// object-type arms each end in the same trailing write.
int CSWSMessage::SendServerToPlayerExamineGui_MineData(CSWSPlayer* pPlayer,
                                                       unsigned long long objectId,
                                                       CSWSCreature* pCreature, int bDetected)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject == 0)
        return 0;

    int nRank = pCreature->m_pStats->GetSkillRank(1, 0, 0);
    int nRankMax = nRank + 20;

    g_pAppManager->m_pServerExoApp->GetNetLayer()->GetPlayerInfo(nPlayerId);

    CreateWriteMessage(14, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBOOL(bDetected);

    int nDifficulty;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
        int nDisarmDC = pObject->AsSWSTrigger()->GetDisarmDC();
        WriteBYTE(pObject->AsSWSTrigger()->m_trapType, 8);
        if (pObject->AsSWSTrigger()->m_bTrapDisarmable == 0)
            nDifficulty = 4;
#if KOTOR1  // K1: the rating gives up above DC 35
        else if (nDisarmDC > 35)
#else
        else if (nDisarmDC > 65)
#endif
            nDifficulty = 4;
        else if (nDisarmDC <= nRank + 5)
            nDifficulty = 0;
        else if (nDisarmDC <= nRank + 10)
            nDifficulty = 1;
        else if (nDisarmDC <= nRank + 15)
            nDifficulty = 2;
        else if (nDisarmDC <= nRankMax)
            nDifficulty = 3;
        else
            nDifficulty = 4;
        WriteBYTE(static_cast<unsigned char>(nDifficulty), 8);
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
        unsigned char nDisarmDC = pObject->AsSWSDoor()->GetDisarmDC();
        WriteBYTE(pObject->AsSWSDoor()->m_trapType, 8);
        if (pObject->AsSWSDoor()->m_bTrapDisarmable == 0)
            nDifficulty = 4;
#if KOTOR1
        else if (nDisarmDC > 35)
#else
        else if (nDisarmDC > 65)
#endif
            nDifficulty = 4;
        else if (nDisarmDC <= nRank + 5)
            nDifficulty = 0;
        else if (nDisarmDC <= nRank + 10)
            nDifficulty = 1;
        else if (nDisarmDC <= nRank + 15)
            nDifficulty = 2;
        else if (nDisarmDC <= nRankMax)
            nDifficulty = 3;
        else
            nDifficulty = 4;
        WriteBYTE(static_cast<unsigned char>(nDifficulty), 8);
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        unsigned char nDisarmDC = pObject->AsSWSPlaceable()->GetDisarmDC();
        WriteBYTE(pObject->AsSWSPlaceable()->m_trapType, 8);
        if (pObject->AsSWSPlaceable()->m_bTrapDisarmable == 0)
            nDifficulty = 4;
#if KOTOR1
        else if (nDisarmDC > 35)
#else
        else if (nDisarmDC > 65)
#endif
            nDifficulty = 4;
        else if (nDisarmDC <= nRank + 5)
            nDifficulty = 0;
        else if (nDisarmDC <= nRank + 10)
            nDifficulty = 1;
        else if (nDisarmDC <= nRank + 15)
            nDifficulty = 2;
        else if (nDisarmDC <= nRankMax)
            nDifficulty = 3;
        else
            nDifficulty = 4;
        WriteBYTE(static_cast<unsigned char>(nDifficulty), 8);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_INFO_POPUP, MSG_GUI_INFO_POPUP_MINE,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerCamera_ChangeLocation(CSWSPlayer* pPlayer, int nMask, float fX,
                                                         float fY, float fZ, int nMode)
{
    CreateWriteMessage(256, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(static_cast<unsigned char>(nMask), 8);
    if (nMask & 1)
        WriteFLOAT(fX, 1.0f, 32);
    if (nMask & 2)
        WriteFLOAT(fY, 1.0f, 32);
    if (nMask & 4)
        WriteFLOAT(fZ, 1.0f, 32);
    if (nMask & 8)
        WriteBYTE(static_cast<unsigned char>(nMode), 8);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CAMERA,
                                     MSG_CAMERA_CHANGE_LOCATION, pMsg, nSize);
}

// The fields go out in bit order, the reverse of the parameter order, and three of the
// unsigned int parameters are sent as bytes.
int CSWSMessage::SendServerToPlayerModuleUpdate_Time(CSWSPlayer* pPlayer, unsigned char nMask,
                                                     unsigned int nDword, unsigned int nByteB,
                                                     unsigned int nByteC, unsigned int nByteD,
                                                     unsigned char nKind, unsigned int nKindArg)
{
    CreateWriteMessage(13, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nMask, 8);
    if (nMask & MSG_MODULE_TIME_TIME_OF_DAY)
    {
        WriteBYTE(nKind, 8);
        if (nKind == 3 || nKind == 4)
            WriteDWORD(nKindArg, 32);
    }
    if (nMask & MSG_MODULE_TIME_HOUR)
        WriteBYTE(static_cast<unsigned char>(nByteD), 8);
    if (nMask & MSG_MODULE_TIME_DAY)
        WriteBYTE(static_cast<unsigned char>(nByteC), 8);
    if (nMask & MSG_MODULE_TIME_MONTH)
        WriteBYTE(static_cast<unsigned char>(nByteB), 8);
    if (nMask & MSG_MODULE_TIME_YEAR)
        WriteDWORD(nDword, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_MODULE, MSG_MODULE_TIME, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerMapPinAdded(CSWSPlayer* pPlayer, Vector vPos,
                                               const CExoString& sLabel, unsigned int nPin)
{
    CreateWriteMessage(sLabel.GetLength() + 16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteFLOAT(vPos.x, 1.0f, 32);
    WriteFLOAT(vPos.y, 1.0f, 32);
    WriteFLOAT(vPos.z, 1.0f, 32);
    WriteCExoString(sLabel, 32);
    WriteDWORD(nPin, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_MAP_PIN, MSG_MAP_PIN_ADDED,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerJournalAddWorld(CSWSPlayer* pPlayer, int nId,
                                                   const CExoString& sTitle,
                                                   const CExoString& sText, unsigned int nA,
                                                   unsigned int nB)
{
    unsigned char* pMsg;
    int nTitleLen = sTitle.GetLength();
    int nTextLen = sText.GetLength();
    unsigned int nSize = nTitleLen + 12 + nTextLen;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nId, 32);
    WriteCExoString(sTitle, 32);
    WriteCExoString(sText, 32);
    WriteDWORD(nA, 32);
    WriteDWORD(nB, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_ADD_WORLD, pMsg, nSize);
}

// The family's only broadcast member, sending one copy per player who already holds a snapshot of
// the object, built and sent inside the loop.
int CSWSMessage::SendServerToAllPlayersCreatureUpdate_StripEquippedItems(unsigned long long objectId)
{
    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    if (pNode != 0) {
        do {
            CSWSPlayer* pOther = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
            if (pOther->GetLastUpdateObject(objectId) != 0) {
                CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
                WriteOBJECTIDServer(objectId);

                unsigned char* pMsg;
                unsigned int nSize;
                if (GetWriteMessage(&pMsg, &nSize) != 0)
                    SendServerToPlayerMessage(pOther->m_nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                              MSG_GAME_OBJECT_UPDATE_STRIP_EQUIPPED_ITEMS, pMsg, nSize);
            }
            pList->m_pInternal->GetNext(pNode);
        } while (pNode != 0);
    }

    return 1;
}

int CSWSMessage::SendServerToPlayerDebugInfo_Item(CSWSPlayer* pPlayer, unsigned long long objectId)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
    if (pItem == 0)
        return 0;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteCExoString(pItem->m_sTag, 32);
    WriteINT(pItem->m_nPassiveProperties, 32);

    for (int i = 0; i < pItem->m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProperty = pItem->GetPassiveProperty(i);
        if (pProperty != 0)
            WriteWORD(pProperty->m_type, 16);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_DEBUG_INFO,
                                     MSG_DEBUG_INFO_ITEM, pMsg, nSize);
}

// Ships the resolved placeable's faction name, after a four-deep null guard over the
// placeable, the app internals, the faction manager and the faction.
int CSWSMessage::SendServerToPlayerDebugInfo_Placeable(CSWSPlayer* pPlayer,
                                                       unsigned long long objectId)
{
    CSWSPlaceable* pPlaceable =
        g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(objectId);
    if (pPlaceable == 0)
        return 0;

    if (g_pAppManager->m_pServerExoApp->m_pInternal == 0)
        return 0;

    CFactionManager* pFactionManager =
        g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
    if (pFactionManager == 0)
        return 0;

    CSWSFaction* pFaction = pFactionManager->GetFaction(pPlaceable->m_nFaction);
    if (pFaction == 0)
        return 0;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteCExoLocStringServer(pPlaceable->GetFirstName(), 0);
    WriteCExoString(pPlaceable->m_sTag, 32);
    WriteBYTE(pPlaceable->m_hardness, 8);
    WriteSHORT(pPlaceable->GetCurrentHitPoints(1), 16);
    WriteBYTE(pPlaceable->m_fortSave, 8);
    WriteBYTE(pPlaceable->m_refSave, 8);
    WriteBYTE(pPlaceable->m_willSave, 8);
    WriteCResRef(pPlaceable->m_templateResRef, 16);
    WriteCExoString(pFaction->m_sName, 32);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_DEBUG_INFO,
                                     MSG_DEBUG_INFO_PLACEABLE, pMsg, nSize);
}

// Sends one roster entry with the player id, name and the character they are driving, or the
// invalid-object sentinel when the driven object is absent or is not a player character.
int CSWSMessage::SendServerToPlayerPlayerList_Add(unsigned int nPlayerId, CSWSPlayer* pPlayer)
{
    unsigned char* pMsg;
    unsigned int nSize;

    unsigned int nSubjectId = pPlayer->m_nPlayerId;
    unsigned long long objectId = pPlayer->GetGameObject()->m_objectId;

    CExoString sPlayerName(
        g_pAppManager->m_pServerExoApp->GetNetLayer()->GetPlayerInfo(nSubjectId)->m_sName);

    unsigned long long characterId;
    CSWSCreature* pDriven = pPlayer->GetGameObject()->AsSWSCreature();
    if (pDriven != 0 && pDriven->m_pStats->m_bIsPC == 1)
        characterId = pDriven->m_objectId;
    else
        characterId = OBJECT_INVALID;

    CExoLocString locFirstName;
    CExoLocString locLastName;
    CResRef resPortrait;

    unsigned short nPortraitId;
    int bFound = 0;
    CSWSCreature* pCreature;
    {
        CSWSCreature* pFoundCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(characterId);
        pCreature = pFoundCreature;
        if (pCreature != 0)
        {
            locFirstName = pCreature->m_pStats->m_firstName;
            locLastName = pCreature->m_pStats->m_lastName;
            CResRef resStatsPortrait = pCreature->m_pStats->m_portrait;
            resPortrait = resStatsPortrait;
            bFound = 1;
        }
    }

    CreateWriteMessage(256, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nSubjectId, 32);
    WriteOBJECTIDServer(objectId);
    WriteCExoString(sPlayerName, 32);
    WriteBOOL(bFound);

    if (bFound != 0)
    {
        WriteOBJECTIDServer(characterId);
        WriteCExoLocStringServer(locFirstName, 0);
        WriteCExoLocStringServer(locLastName, 0);

        nPortraitId = pCreature->GetPortraitId();
        WriteWORD(nPortraitId, 16);
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            WriteCResRef(resPortrait, 16);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_PLAYER_LIST, MSG_PLAYER_LIST_ADD, pMsg,
                                     nSize);
}

// Sends the creature examine panel, with names, portrait, damage level, reputation and a level
// rating on top of the id-and-description pair. The rating is the recipient's level for a player
// character and the stored challenge rating otherwise.
int CSWSMessage::SendServerToPlayerExamineGui_CreatureData(CSWSPlayer* pPlayer,
                                                           unsigned long long objectId)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;
    g_pAppManager->m_pServerExoApp->GetNetLayer()->GetPlayerInfo(nPlayerId);

    CGameObject* pObject = 0;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();

    CExoLocString locDescription;

    if (pArray->GetGameObject(objectId, &pObject) != CGameObjectArray::SUCCESS)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    locDescription = pCreature->m_pStats->m_description;

    unsigned int nSize = 12;
    unsigned char* pMsg;
    unsigned short nPortraitId;

    pPlayer->GetLastUpdateObject(objectId);

    CreateWriteMessage(12, nPlayerId, 1);
    WriteOBJECTIDServer(pCreature->m_objectId);
    WriteCExoLocStringServer(locDescription, 0);
    WriteBOOL(1);
    WriteCExoLocStringServer(pCreature->GetFirstName(), 0);
    WriteCExoLocStringServer(pCreature->GetLastName(), 0);

    nPortraitId = pCreature->GetPortraitId();
    WriteWORD(nPortraitId, 16);
    if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
        WriteCResRef(pCreature->m_pStats->m_portrait, 16);

    WriteBYTE(pCreature->GetDamageLevel(), 8);
    WriteBYTE(pPlayer->GetGameObject()->AsSWSCreature()
                  ->GetAIStateReputation(pCreature->m_objectId), 8);

    float fRating;
    if (pCreature->m_pStats->m_bIsPC != 0)
        fRating = static_cast<float>(pCreature->m_pStats->GetLevel(0));
    else
        fRating = pCreature->m_pStats->m_fChallengeRating;
    WriteFLOAT(fRating, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GUI_INFO_POPUP,
                                     MSG_GUI_INFO_POPUP_CREATURE, pMsg, nSize);
}

// Sends the whole roster in one message, the add payload repeated once per connected player behind
// a leading count. The list is walked twice, once to count and once to ship.
int CSWSMessage::SendServerToPlayerPlayerList_All(CSWSPlayer* pPlayer)
{
    CreateWriteMessage(512, MSG_PLAYERID_ALL_CLIENTS, 1);

    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pList == 0)
        return 0;

    unsigned char nCount = 0;
    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    if (pNode != 0) {
        do {
            CSWSPlayer* pOther = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
            if (pOther->GetGameObject() != 0)
                nCount++;
            pList->GetNext(pNode);
        } while (pNode != 0);
    }

    WriteBYTE(nCount, 8);

    pNode = pList->m_pInternal->m_pHead;
    if (pNode != 0) {
        do {
            CSWSPlayer* pOther = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
            unsigned int nSubjectId = pOther->m_nPlayerId;
            CGameObject* pGameObject = pOther->GetGameObject();
            if (pGameObject != 0)
            {
                unsigned long long objectId = pGameObject->m_objectId;

                CExoString sPlayerName(
                    g_pAppManager->m_pServerExoApp->GetNetLayer()
                        ->GetPlayerInfo(nSubjectId)->m_sName);

                unsigned long long characterId;
                CSWSCreature* pDriven = pOther->GetGameObject()->AsSWSCreature();
                if (pDriven != 0 && pDriven->m_pStats->m_bIsPC == 1)
                    characterId = pDriven->m_objectId;
                else
                    characterId = OBJECT_INVALID;

                CExoLocString locFirstName;
                CExoLocString locLastName;
                CResRef resPortrait;

                unsigned short nPortraitId;
                int bFound = 0;
                CSWSCreature* pCreature =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(characterId);
                if (pCreature != 0)
                {
                    locFirstName = pCreature->m_pStats->m_firstName;
                    locLastName = pCreature->m_pStats->m_lastName;
                    CResRef resStatsPortrait = pCreature->m_pStats->m_portrait;
                    resPortrait = resStatsPortrait;
                    bFound = 1;
                }

                WriteDWORD(nSubjectId, 32);
                WriteOBJECTIDServer(objectId);
                WriteCExoString(sPlayerName, 32);
                WriteBOOL(bFound);

                if (bFound != 0)
                {
                    WriteOBJECTIDServer(characterId);
                    WriteCExoLocStringServer(locFirstName, 0);
                    WriteCExoLocStringServer(locLastName, 0);

                    nPortraitId = pCreature->GetPortraitId();
                    WriteWORD(nPortraitId, 16);
                    if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
                        WriteCResRef(resPortrait, 16);
                }
            }
            pList->GetNext(pNode);
        } while (pNode != 0);
    }

    if (nCount == 0)
        return 0;

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_PLAYER_LIST,
                                     MSG_PLAYER_LIST_ALL, pMsg, nSize);
}

// Sends the biggest debug panel, with a five-deep null guard, a fixed header, then the
// equipped-slot bitmask, the equipped-slot tags and the whole item repository.
int CSWSMessage::SendServerToPlayerDebugInfo_Creature(CSWSPlayer* pPlayer,
                                                      unsigned long long objectId)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature == 0)
        return 0;

    if (pCreature->GetFaction() == 0)
        return 0;

    if (pCreature->m_pInventory == 0)
        return 0;

    if (pCreature->GetItemRepository(1) == 0)
        return 0;

    if (pCreature->m_pStats == 0)
        return 0;

    CreateWriteMessage(256, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteCExoString(pCreature->m_sTag, 32);
    WriteCExoString(pCreature->GetFaction()->m_sName, 32);
    WriteCResRef(pCreature->GetDialogResref(), 16);
    WriteSHORT(pCreature->GetCurrentHitPoints(1), 16);
    WriteSHORT(pCreature->GetMaxHitPoints(1), 16);
    WriteSHORT(pCreature->GetArmorClass(), 16);
    WriteINT(pCreature->m_aiLevel, 32);
    WriteFLOAT(pCreature->m_pStats->m_fChallengeRating, 1.0f, 32);

    if (pCreature->GetArea() != 0)
        WriteCExoString(static_cast<CSWSArea*>(pCreature->GetArea())->m_sDebugName, 32);
    else
        WriteCExoString(CExoString(""), 32);

    unsigned int nEquippedMask = 0;
#if KOTOR1  // K1: 18 equipment slots
    for (int nSlot = 0; nSlot < 18; nSlot++)
#else
    for (int nSlot = 0; nSlot < 20; nSlot++)
#endif
    {
        if (pCreature->m_pInventory->GetItemInSlot(1 << nSlot) != 0)
            nEquippedMask |= 1 << nSlot;
    }
    WriteDWORD(nEquippedMask, 32);

#if KOTOR1
    for (int nSlot = 0; nSlot < 18; nSlot++)
#else
    for (int nSlot = 0; nSlot < 20; nSlot++)
#endif
    {
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(1 << nSlot));
        if (pItem != 0)
            WriteCExoString(pItem->m_sTag, 32);
    }

    WriteDWORD(pCreature->GetItemRepository(1)->ItemListGetCount(), 32);

    for (unsigned int i = 0;
         i < static_cast<unsigned int>(pCreature->GetItemRepository(1)->ItemListGetCount());
         i++)
    {
        CSWSItem* pItem = pCreature->GetItemRepository(1)->ItemListGetItem(i);
        if (pItem != 0)
            WriteCExoString(pItem->m_sTag, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_DEBUG_INFO,
                                     MSG_DEBUG_INFO_CREATURE, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerModule_EndGame(unsigned int nPlayerId,
                                                  const CExoString& sModule)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sModule, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_MODULE, MSG_MODULE_END_GAME, pMsg, nSize);
}

int CSWSMessage::SendPlayerToServerGuiInventory_Status(CSWSPlayer* pPlayer, int nStatus)
{
    unsigned char* pMsg;
    unsigned int nSize = 1;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nStatus == 1, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GUI_INVENTORY,
                                     MSG_GUI_INVENTORY_STATUS, pMsg, nSize);
}

// The area and vector parameters are unused; the payload is the item id plus the two shared
// appearance and property helpers.
int CSWSMessage::SendServerToPlayerItemScriptAdd(unsigned int nPlayerId, CSWSArea* pArea,
                                                 CSWSItem* pItem, Vector vPos)
{
    K2_UNUSED(pArea); K2_UNUSED(vPos);
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(pItem->m_objectId);
    AddItemAppearanceToMessage(pItem);
    AddActiveItemPropertiesToMessage(pItem, 0);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                     MSG_GAME_OBJECT_UPDATE_ITEM_SCRIPT_ADD, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerInventory_Equip(unsigned int nPlayerId,
                                                   unsigned long long objectId,
                                                   unsigned int nSlot, int bA)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteDWORD(nSlot, 32);
    WriteBOOL(bA);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_INVENTORY, MSG_INVENTORY_EQUIP, pMsg,
                                     nSize);
}

int CSWSMessage::SendServerToPlayerJournalSetQuestPicture(CSWSPlayer* pPlayer,
                                                          const CExoString& sPicture, int nId)
{
    unsigned char* pMsg;
    unsigned int nSize = sPicture.GetLength() + 4;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sPicture, 32);
    WriteINT(nId, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_SET_QUEST_PICTURE, pMsg, nSize);
}

// The count arrives by reference, so the latch re-reads it each iteration.
int CSWSMessage::SendServerToPlayerLogin_CharacterQuery(CSWSPlayer* pPlayer, unsigned char& nCount,
                                                        int* pIds, unsigned char* pTypes,
                                                        unsigned int& nTail)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nCount, 8);

    for (unsigned char i = 0; i < nCount; i++)
    {
        WriteINT(pIds[i], 32);
        WriteBYTE(pTypes[i], 8);
    }

    WriteDWORD(nTail, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_LOGIN,
                                     MSG_LOGIN_CHARACTER_QUERY, pMsg, nSize);
}

// Builds only when a module is loaded and the server mode is 1 or 2, and the mode is queried
// with two separate calls.
int CSWSMessage::SendServerToPlayerModule_Info(unsigned int nPlayerId)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();

    if ((g_pAppManager->m_pServerExoApp->GetServerMode() == 1 ||
         g_pAppManager->m_pServerExoApp->GetServerMode() == 2) && pModule != 0)
    {
        unsigned char* pMsg;
        unsigned int nSize;

        CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
        pModule->PackModuleIntoMessage(nPlayerId);

        if (GetWriteMessage(&pMsg, &nSize) != 0)
            return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_MODULE, MSG_MODULE_INFO, pMsg,
                                             nSize);
    }

    return 0;
}

int CSWSMessage::SendServerToPlayerChat_Tell(unsigned int nPlayerId, unsigned long long speakerId,
                                             const CExoString& sText)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(speakerId);
    if (pCreature == 0)
        return 0;

    CreateWriteMessage(sText.GetLength() + 24, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(speakerId);
    WriteCExoString(sText, 32);
    WriteFLOAT(pCreature->m_position.x, 1.0f, 32);
    WriteFLOAT(pCreature->m_position.y, 1.0f, 32);
    WriteFLOAT(pCreature->m_position.z, 1.0f, 32);
    WriteCExoLocStringServer(pCreature->GetFirstName(), 0);
    WriteCExoLocStringServer(pCreature->GetLastName(), 0);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_TELL, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerChat_Party(unsigned int nPlayerId, unsigned long long speakerId,
                                              const CExoString& sText)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(speakerId);
    if (pCreature == 0)
        return 0;

    // The size addend here is smaller than Chat_Tell's.
    CreateWriteMessage(sText.GetLength() + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(speakerId);
    WriteCExoString(sText, 32);
    WriteFLOAT(pCreature->m_position.x, 1.0f, 32);
    WriteFLOAT(pCreature->m_position.y, 1.0f, 32);
    WriteFLOAT(pCreature->m_position.z, 1.0f, 32);
    WriteCExoLocStringServer(pCreature->GetFirstName(), 0);
    WriteCExoLocStringServer(pCreature->GetLastName(), 0);

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_PARTY, pMsg, nSize);
}

// Sends the party roster, a counted payload per member, with the sub-opcode passed in.
int CSWSMessage::SendServerToPlayerParty_List(unsigned int nPlayerId, int nCount,
                                              unsigned long long* pIds, unsigned char nSubOpcode,
                                              unsigned long long excludeId)
{
    int nSend = 0;

    for (int i = 0; i < nCount; i++)
    {
        if (pIds[i] != excludeId)
            nSend++;
    }

    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nSend, 32);

    for (int i = 0; i < nCount; i++)
    {
        if (pIds[i] != excludeId)
            WriteOBJECTIDServer(pIds[i]);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_PARTY, nSubOpcode, pMsg, nSize);
}

// The area-change message. It picks the loading screen the client will show, stamps it on the
// player, then ships the destination position and facing and the packed area. The pick is a
// two-pass reservoir choice over the Loadscreens table: count the rows whose TileSet column
// matches, then keep the (rand() % count)-th match. Rows 0 and 1 are reserved. The tile-set
// string is never filled in, so the match is really against an empty column.
int CSWSMessage::SendServerToPlayerArea_ClientArea(CSWSPlayer* pPlayer, CSWSArea* pArea, float fX,
                                                   float fY, float fZ, const Vector& vFacing,
                                                   int bFlag)
{
    unsigned char* pMsg;
    unsigned int nSize;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();

    if (g_pAppManager->m_pServerExoApp->GetServerMode() != 2 || pModule == 0)
        return 0;

    CSWSModuleEntryInfo* pModuleEntry = pModule->m_pEntryInfo;

    if (pPlayer->GetGameObject() == 0 || pArea == 0)
        return 0;

    CreateWriteMessage(65536, MSG_PLAYERID_ALL_CLIENTS, 1);

    if (pPlayer->m_nAreaTransitionBMP == 1 || pPlayer->m_nAreaTransitionBMP == -1)
        pPlayer->SetAreaTransitionBMP(pArea->m_nLoadScreenID, "");

    if (pPlayer->m_nAreaTransitionBMP >= g_pRules->m_p2DArrays->m_pAreaTransition->m_nRows)
        pPlayer->SetAreaTransitionBMP(0, "");

    if (pPlayer->m_nAreaTransitionBMP != 0)
    {
        WriteBYTE(static_cast<unsigned char>(pPlayer->m_nAreaTransitionBMP), 8);
    }
    else
    {
        CExoString sAreaTileSet;
        int nRows = g_pRules->m_p2DArrays->m_pAreaTransition->m_nRows;
        CExoString sEntry;

        int nMatches = 0;
        for (int i = 2; i < nRows; i++)
        {
            if (g_pRules->m_p2DArrays->m_pAreaTransition->GetCExoStringEntry(i, "TileSet", &sEntry)
                == 1)
            {
                if (sAreaTileSet.CompareNoCase(sEntry) == 1)
                    nMatches++;
            }
        }

        int nBmp = 0;
        if (!(nMatches < 1))
        {
            int nPick = rand_wincompatible() % nMatches;
            int nSeen = 0;

            for (int i = 2; i < nRows; i++)
            {
                if (g_pRules->m_p2DArrays->m_pAreaTransition->GetCExoStringEntry(i, "TILESET",
                                                                                &sEntry) == 1)
                {
                    if (sAreaTileSet.CompareNoCase(sEntry) == 1)
                    {
                        if (nSeen == nPick)
                            nBmp = i;

                        nSeen++;
                    }
                }
            }
        }

#if KOTOR1  // K1: no row-count guard on the random fallback
        if (nBmp == 0)
#else
        if (nRows >= 3 && nBmp == 0)
#endif
            nBmp = rand_wincompatible() % (nRows - 2) + 2;

        WriteBYTE(static_cast<unsigned char>(nBmp), 8);
    }

    pPlayer->SetAreaTransitionBMP(-1, "");

    float fFacing = atan2f(vFacing.y, vFacing.x);

    WriteFLOAT(fX, 1.0f, 32);
    WriteFLOAT(fY, 1.0f, 32);
    WriteFLOAT(fZ, 1.0f, 32);
    WriteFLOAT(fFacing, 1.0f, 32);
    WriteBOOL(bFlag);

    if (pArea->PackAreaIntoMessage(static_cast<int>(pModuleEntry->m_entryX), static_cast<int>(pModuleEntry->m_entryY),
                                   static_cast<int>(pModuleEntry->m_entryZ), pPlayer)
        != 1)
        return 0;

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_AREA, MSG_AREA_CLIENT_AREA,
                                     pMsg, nSize);
}

// Sends one dialog node, with the speaker and listener ids, the camera shot, the fade and the
// animation participants. The reserve is the only conditional one in the family - a fixed part, one
// per animation entry, and a further block when a fade is set. The locstring is not counted.
int CSWSMessage::SendServerToPlayerDialogEntry(unsigned int nPlayerId,
                                               unsigned long long objectIdA,
                                               unsigned long long objectIdSpeaker,
                                               unsigned long long objectIdListener,
                                               unsigned long long objectIdB, unsigned char nLang,
                                               CExoLocString& sText, unsigned int nAnimCount,
                                               unsigned long long* pAnimParticipants,
                                               unsigned short* pAnimations,
                                               unsigned int nCameraAngle, int nCameraId,
                                               float fCameraFieldOfView, float fCameraHeight,
                                               unsigned int nWaitFlags,
                                               unsigned short nCameraAnimation,
                                               float fTargetHeightOffset, unsigned char nFadeType,
                                               Vector vFadeColor, float fFadeLength,
                                               float fFadeDelay)
{
    unsigned char* pMsg = 0;
    unsigned int nSize = 63 + nAnimCount * 8 + nAnimCount * 2;
    if (nFadeType != 0)
        nSize += 20;

    CreateWriteMessage(nSize, nPlayerId, 1);
    WriteOBJECTIDServer(objectIdA);
    WriteOBJECTIDServer(objectIdSpeaker);
    WriteOBJECTIDServer(objectIdListener);
    WriteOBJECTIDServer(objectIdB);
    WriteCExoLocStringServer(sText, nLang);
    WriteDWORD(nAnimCount, 32);

    for (unsigned int i = 0; i < nAnimCount; i++)
    {
        WriteOBJECTIDServer(pAnimParticipants[i]);
        WriteWORD(pAnimations[i], 16);
    }

    WriteDWORD(nCameraAngle, 32);
    WriteINT(nCameraId, 32);
    WriteFLOAT(fCameraFieldOfView, 1.0f, 32);
    WriteFLOAT(fCameraHeight, 1.0f, 32);
    WriteDWORD(nWaitFlags, 32);
    WriteWORD(nCameraAnimation, 16);
    WriteFLOAT(fTargetHeightOffset, 1.0f, 32);
    WriteBYTE(nFadeType, 8);

    if (nFadeType != 0)
    {
        WriteFLOAT(vFadeColor.x, 1.0f, 32);
        WriteFLOAT(vFadeColor.y, 1.0f, 32);
        WriteFLOAT(vFadeColor.z, 1.0f, 32);
        WriteFLOAT(fFadeLength, 1.0f, 32);
        WriteFLOAT(fFadeDelay, 1.0f, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_DIALOG, MSG_DIALOG_ENTRY, pMsg, nSize);
}

// DialogEntry's plural twin sends the same fields for many replies out of parallel arrays. The
// reserve is a per-reply sum plus the header, and it under-counts against what the header really
// ships; the runtime buffer extension covers the shortfall.
int CSWSMessage::SendServerToPlayerDialogReplies(unsigned int nPlayerId,
                                                 unsigned long long objectIdSpeaker,
                                                 unsigned char nLang, CExoLocString* pText,
                                                 unsigned int* pnReplyIds,
                                                 unsigned long long* pObjectIds,
                                                 unsigned int* pnCameraAngles, int* pnCameraIds,
                                                 float* pfCameraFieldOfViews,
                                                 float* pfCameraHeights,
                                                 unsigned int* pnWaitFlags,
                                                 unsigned short* pnCameraAnimations,
                                                 float* pfTargetHeightOffsets,
                                                 unsigned char* pnFadeTypes, Vector* pvFadeColors,
                                                 float* pfFadeLengths, float* pfFadeDelays,
                                                 unsigned int nReplyCountA,
                                                 unsigned int nReplyCountB,
                                                 unsigned int nEntry,
                                                 unsigned int* pnAnimCounts,
                                                 unsigned long long** ppAnimParticipants,
                                                 unsigned short** ppAnimations, int bFlag)
{
    unsigned char* pMsg = 0;
    unsigned int nSize = 16;
    unsigned int nCount = nReplyCountB + nReplyCountA;

    for (unsigned int i = 0; i < nCount; i++)
        nSize += pnAnimCounts[i] * 6 + 63;

    CreateWriteMessage(nSize, nPlayerId, 1);
    WriteDWORD(nEntry, 32);
    WriteBYTE(bFlag != 0, 8);
    WriteOBJECTIDServer(objectIdSpeaker);
    WriteDWORD(nReplyCountA, 32);
    WriteDWORD(nReplyCountB, 32);

    for (unsigned int i = 0; i < nCount; i++)
    {
        WriteCExoLocStringServer(pText[i], nLang);
        WriteDWORD(pnReplyIds[i], 32);
        WriteOBJECTIDServer(pObjectIds[i]);
        WriteDWORD(pnCameraAngles[i], 32);
        WriteINT(pnCameraIds[i], 32);
        WriteFLOAT(pfCameraFieldOfViews[i], 1.0f, 32);
        WriteFLOAT(pfCameraHeights[i], 1.0f, 32);
        WriteDWORD(pnWaitFlags[i], 32);
        WriteWORD(pnCameraAnimations[i], 16);
        WriteFLOAT(pfTargetHeightOffsets[i], 1.0f, 32);
        WriteBYTE(pnFadeTypes[i], 8);
        WriteFLOAT(pvFadeColors[i].x, 1.0f, 32);
        WriteFLOAT(pvFadeColors[i].y, 1.0f, 32);
        WriteFLOAT(pvFadeColors[i].z, 1.0f, 32);
        WriteFLOAT(pfFadeLengths[i], 1.0f, 32);
        WriteFLOAT(pfFadeDelays[i], 1.0f, 32);
        WriteDWORD(pnAnimCounts[i], 32);

        for (int j = 0; j < static_cast<int>(pnAnimCounts[i]); j++)
        {
            WriteOBJECTIDServer(ppAnimParticipants[i][j]);
            WriteWORD(ppAnimations[i][j], 16);
        }
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_DIALOG, MSG_DIALOG_REPLIES, pMsg, nSize);
}

// The reserve is a per-element sum: a fixed part, one object id and one resource ref per model,
// plus the trailing resource ref.
int CSWSMessage::SendServerToPlayerDialogModels(unsigned int nPlayerId, unsigned int nCount,
                                                unsigned long long* pIds, CResRef* pResRefs,
                                                const CResRef& rLast)
{
    unsigned char* pMsg = 0;
    unsigned int nSize = 4 + nCount * 8 + nCount * 16 + 16;

    CreateWriteMessage(nSize, nPlayerId, 1);
    WriteDWORD(nCount, 32);

    for (unsigned int i = 0; i < nCount; i++)
    {
        WriteOBJECTIDServer(pIds[i]);
        WriteCResRef(pResRefs[i], 16);
    }

    WriteCResRef(rLast, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_DIALOG, MSG_DIALOG_MODELS, pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerDialogReplyChosen(unsigned int nPlayerId, unsigned int nA,
                                                     unsigned int nB, CExoLocString& sText,
                                                     unsigned long long objectId,
                                                     unsigned char nLang, int bFlag)
{
    unsigned char* pMsg;
    unsigned int nSize = 21;

    CreateWriteMessage(21, nPlayerId, 1);
    WriteBYTE(bFlag != 0, 8);
    WriteDWORD(nA, 32);
    WriteDWORD(nB, 32);
    WriteOBJECTIDServer(objectId);
    WriteCExoLocStringServer(sText, nLang);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_DIALOG, MSG_DIALOG_REPLY_CHOSEN, pMsg,
                                     nSize);
}

// Ships a mask byte and then one byte per set bit, in bit order.
int CSWSMessage::SendServerToPlayerUpdateActiveItemProperties(CSWSPlayer* pPlayer,
                                                              unsigned long long objectId,
                                                              unsigned char nSlot,
                                                              unsigned char nMask,
                                                              unsigned char* pValues)
{
    CreateWriteMessage(9, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(objectId);
    WriteBYTE(nSlot, 8);
    WriteBYTE(nMask, 8);

    for (int i = 0; i < 8; i++)
    {
        if (nMask & (1 << i))
            WriteBYTE(pValues[i], 8);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_ITEM_PROPERTY,
                                     MSG_ITEM_PROPERTY_UPDATE_ACTIVE, pMsg, nSize);
}

// One arm per panel, where the panel id picks both the payload and the sub-opcode. Every lookup
// failure returns 1; only a write-message failure returns 0.
int CSWSMessage::SendServerToPlayerPopUpGUIPanel(unsigned long long objectId, int nPanel, int bA,
                                                 int bB, int nC, const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
    if (pPlayer == 0)
        return 1;

    if (nPanel == GUI_PANEL_PLAYER_DEATH)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
        if (pCreature == 0)
            return 1;
        if (pCreature->m_pStats->m_bIsPC == 0)
            return 1;

        CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteBOOL(bA);
        WriteBOOL(bB);
        WriteINT(nC, 32);
        WriteCExoString(sText, 32);

        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_POPUP_GUI_PANEL,
                                         MSG_POPUP_GUI_PANEL_SHOW, pMsg, nSize);
    }
    else if (nPanel == GUI_PANEL_PARTY_INVITATION_RECEIVED)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
        if (pCreature == 0)
            return 1;
        if (pCreature->m_pStats->m_bIsPC == 0)
            return 1;

        CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteCExoString(sText, 32);

        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_POPUP_GUI_PANEL,
                                         MSG_POPUP_GUI_PANEL_MESSAGE, pMsg, nSize);
    }

    return 1;
}

// The multiplayer character-transfer reply. nMode selects where the character file comes from:
// 1 the resource named by cResRef, 4 the module's Mod_PlayerList indexed by cResRef read as a
// decimal, 5 the servervault directory entry matching the player's own name. All three arms end
// with a freshly built creature whose stats came out of a GFF struct; the shared tail writes it
// back out to a temporary .bic, re-reads the file and ships its bytes. The creature is never
// deleted on the success path.
int CSWSMessage::SendServerToPlayerUpdateCharResponse(CSWSPlayer* pPlayer, unsigned char nMode,
                                                      CResRef cResRef)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CResStruct topStruct;
    CResStruct playerStruct;
    CResList playerList;

    CExoArrayList<CExoString> vaultFiles;
    CExoString sVaultDir;
    CExoString sCharIndex;
    CExoString sPlayerName;

    CSWSCreature* pCreature;

    if (nMode == 5)
    {
        sPlayerName = pPlayer->GetPlayerName();
#if KOTOR1  // K1: GetDirectoryList takes a sixth flag
        g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeAll, 1, 0, 0);
#else
        g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeAll, 1, 0);
#endif

        // The comparand is a copy of the player name taken once before the loop.
        if (vaultFiles.Contains(sPlayerName) == 0)
            return 0;

        sVaultDir.Format(const_cast<char*>("SERVERVAULT:%s"), sPlayerName.CStr());
        g_pExoResMan->AddResourceDirectory(sVaultDir);

        CResGFF* pGff = new CResGFF(kResTypeBic, const_cast<char*>("BIC "), cResRef);
        if (pGff->m_nValid == 0)
        {
            delete pGff;
            g_pExoResMan->RemoveResourceDirectory(sVaultDir);
            return 0;
        }

        pGff->GetTopLevelStruct(&topStruct);

        pCreature = new CSWSCreature(OBJECT_INVALID, 1);
        if (pCreature->m_pStats->ReadStatsFromGff(pGff, &topStruct, &pCreature->m_appearance))
        {
            delete pCreature;
            delete pGff;
            g_pExoResMan->RemoveResourceDirectory(sVaultDir);
            return 0;
        }

        delete pGff;
        g_pExoResMan->RemoveResourceDirectory(sVaultDir);
    }
    else if (nMode == 4)
    {
        cResRef.CopyToString(sCharIndex);

        CRes* pRes = g_pExoResMan->GetResObject(CResRef("Module"), kResTypeIfo);
        if (pRes == 0)
            return 0;

        pRes->Demand();

        CResGFF* pGff = static_cast<CResGFF*>(pRes);
        pGff->GetTopLevelStruct(&topStruct);

        if (pGff->GetList(&playerList, &topStruct, const_cast<char*>("Mod_PlayerList")) == 0)
        {
            pRes->Release();
            g_pExoResMan->ReleaseResObject(pRes);
            return 0;
        }

        // The resource ref doubles as a decimal index into the module's player list.
        if (pGff->GetListElement(&playerStruct, &playerList, sCharIndex.AsINT()) == 0)
        {
            pRes->Release();
            g_pExoResMan->ReleaseResObject(pRes);
            return 0;
        }

        pCreature = new CSWSCreature(OBJECT_INVALID, 1);
        if (pCreature->m_pStats->ReadStatsFromGff(pGff, &playerStruct, &pCreature->m_appearance))
        {
            delete pCreature;
            pRes->Release();
            g_pExoResMan->ReleaseResObject(pRes);
            return 0;
        }

        pRes->Release();
        g_pExoResMan->ReleaseResObject(pRes);
    }
    else if (nMode == 1)
    {
        CResGFF* pGff = new CResGFF(kResTypeBic, const_cast<char*>("BIC "), cResRef);
        if (pGff->m_nValid == 0)
        {
            delete pGff;
            return 0;
        }

        pGff->GetTopLevelStruct(&topStruct);

        pCreature = new CSWSCreature(OBJECT_INVALID, 1);
        if (pCreature->m_pStats->ReadStatsFromGff(pGff, &topStruct, &pCreature->m_appearance))
        {
            delete pCreature;
            delete pGff;
            return 0;
        }

        delete pGff;
    }
    else
    {
        return 0;
    }

    // In the shared tail, write the loaded creature back out and ship the file's bytes.
    CResGFF* pOutGff = new CResGFF();
    CResStruct* pOutStruct = new CResStruct();
    if (pOutGff->CreateGFFFile(pOutStruct, CExoString("BIC "), CExoString("V2.0")) == 0)
    {
        delete pOutStruct;
        delete pOutGff;
        return 0;
    }

    {
        CExoString sTempFile;
        CExoString sUnused;  // Constructed and destroyed, never touched.

        pCreature->m_pStats->SaveStats(pOutGff, pOutStruct);

        sTempFile = "TEMP:temp_charfile";
        pOutGff->WriteGFFFile(sTempFile, kResTypeBic);
        g_pExoResMan->AddResourceDirectory(CExoString("TEMP:"));

        CExoFile* pFile = new CExoFile(sTempFile, kResTypeBic, CExoString("rb"));
        if (pFile->FileOpened() == 0)
        {
            delete pFile;
            delete pOutStruct;
            delete pOutGff;
            return 0;
        }

        unsigned int nFileSize = pFile->GetSize();
        unsigned char* pFileBytes = new unsigned char[nFileSize];
        pFile->Read(pFileBytes, 1, nFileSize);
        delete pFile;

        CreateWriteMessage(21, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteBYTE(nMode, 8);
        WriteCResRef(cResRef, 16);
        WriteDWORD(nFileSize, 32);
        WriteVOIDPtr(pFileBytes, nFileSize);

        g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));
        g_pExoResMan->RemoveFile(sTempFile, kResTypeBic);
        delete [] pFileBytes;
    }

    delete pOutStruct;
    delete pOutGff;

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CHAR_LIST,
                                     MSG_CHAR_LIST_UPDATE_RESPONSE, pMsg, nSize);
}

// Packs the player's creature into the write buffer and ships it; m_bPlot mirrors the in-transit
// latch.
int CSWSMessage::SendServerToPlayerCharacterDownloadReply(CSWSPlayer* pPlayer)
{
    if (pPlayer->GetGameObject() == 0)
        return 0;

    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    int bFlag = 0;
    if (pCreature->m_inTransit != 0) {
        pCreature->m_bPlot = 0;
        bFlag = 1;
    }

    if (pPlayer->PackCreatureIntoMessage() != 0) {
        if (bFlag != 0)
            pCreature->m_bPlot = 1;

        unsigned char* pMsg;
        unsigned int nLen;
        if (GetWriteMessage(&pMsg, &nLen) == 0)
            return 0;

        return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CHARACTER_DOWNLOAD,
                                         MSG_CHARACTER_DOWNLOAD_REPLY, pMsg, nLen);
    }

    if (bFlag != 0)
        pCreature->m_bPlot = 1;

    return 0;
}

int CSWSMessage::SendServerToPlayerCharacterDownloadFail(CSWSPlayer* pPlayer)
{
    unsigned char* pMsg;
    unsigned int nSize;
    CExoString sResRef;

    if (pPlayer->SaveServerCharacter() == 0)
        return 0;

    pPlayer->m_resRef.CopyToString(sResRef);

    CreateWriteMessage(sResRef.GetLength(), MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sResRef, 32);
    WriteBOOL(pPlayer->m_state == 4);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CHARACTER_DOWNLOAD,
                                     MSG_CHARACTER_DOWNLOAD_FAIL, pMsg, nSize);
}

// Sends the multiplayer character-selection list, built from two sequential resource scans. The
// servervault directory listing is searched for an entry matching the player's own name and, when
// it is found, that subdirectory's .bic files are loaded; then the servervault's own .bic files are
// loaded unconditionally. The module's Mod_PlayerList is appended last, and the assembled list is
// shipped and freed entry by entry.
int CSWSMessage::SendServerToPlayerCharList(CSWSPlayer* pPlayer)
{
    CExoString sCharName;
    CExoArrayList<CExoString> vaultFiles;
    unsigned char* pMsg;
    unsigned int nSize = 2;
    CExoString sPlayerName;
    CExoString sVaultDir;
    CExoArrayList<NWPlayerCharacterList_st*> charList;

    CResStruct topStruct;
    CResStruct classStruct;
    CResList classList;
    int bSuccess;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);

    g_pAppManager->m_pServerExoApp->GetNetLayer()->GetPlayerInfo(pPlayer->m_nPlayerId);

    sPlayerName = pPlayer->GetPlayerName();
#if KOTOR1  // K1: GetDirectoryList takes a sixth flag
    g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeAll, 1, 0, 0);
#else
    g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeAll, 1, 0);
#endif

    if (vaultFiles.Contains(sPlayerName) != 0)
    {
        vaultFiles.SetSize(0);
        sVaultDir.Format(const_cast<char*>("SERVERVAULT:%s"), sPlayerName.CStr());
#if KOTOR1  // K1: GetDirectoryList takes a sixth flag
        g_pExoBase->GetDirectoryList(&vaultFiles, sVaultDir, kResTypeBic, 0, 0, 0);
#else
        g_pExoBase->GetDirectoryList(&vaultFiles, sVaultDir, kResTypeBic, 0, 0);
#endif

        if (vaultFiles.m_count != 0)
        {
            g_pExoResMan->AddResourceDirectory(sVaultDir);

            for (int64_t i = 0; i < vaultFiles.m_count; i++)
            {
                sCharName = vaultFiles.m_pArray[i];
                sCharName = sCharName.SubString(
                    0, sCharName.GetLength() - 4);

                CResGFF* pGff = new CResGFF(kResTypeBic, const_cast<char*>("BIC "),
                                            CResRef(sCharName));
                if (pGff->m_nValid != 0)
                {
                    pGff->GetTopLevelStruct(&topStruct);

                    NWPlayerCharacterList_st* pEntry = new NWPlayerCharacterList_st;
                    pEntry->m_firstName = pGff->ReadFieldCExoLocString(
                        &topStruct, const_cast<char*>("FirstName"), bSuccess, CExoLocString());
                    pEntry->m_lastName = pGff->ReadFieldCExoLocString(
                        &topStruct, const_cast<char*>("LastName"), bSuccess, CExoLocString());
                    pEntry->m_resRef = sCharName;
                    pEntry->m_nLocation = MSG_LOGIN_SERVER_CHARACTER_CREATE;
                    pEntry->m_nPortraitId =
                        pGff->ReadFieldWORD(&topStruct, const_cast<char*>("PortraitId"), bSuccess, 0xffff);
                    pEntry->m_portrait = pGff->ReadFieldCResRef(&topStruct, const_cast<char*>("Portrait"),
                                                                bSuccess, CResRef(""));

                    if (pGff->GetList(&classList, &topStruct, const_cast<char*>("ClassList")))
                    {
                        unsigned int nClasses = pGff->GetListCount(&classList);
                        if (nClasses != 0)
                        {
                            for (unsigned int j = 0; j < nClasses; j++)
                            {
                                if (pGff->GetListElement(&classStruct, &classList, j))
                                {
                                    NWPlayerCharacterListClass_st cls;
                                    cls.m_nClass = pGff->ReadFieldINT(&classStruct, const_cast<char*>("Class"),
                                                                      bSuccess, -1);
                                    cls.m_nLevel = static_cast<unsigned char>(pGff->ReadFieldSHORT(
                                        &classStruct, const_cast<char*>("ClassLevel"), bSuccess, 0));
                                    pEntry->m_classList.Add(cls);
                                }
                            }
                        }
                    }

                    if (charList.DerefContains(pEntry))
                        delete pEntry;
                    else
                        charList.AddUnique(pEntry);
                }
                delete pGff;
            }

            g_pExoResMan->RemoveResourceDirectory(sVaultDir);
        }
    }

#if KOTOR1  // K1: GetDirectoryList takes a sixth flag
    g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeBic, 0, 0, 0);
#else
    g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), kResTypeBic, 0, 0);
#endif

    for (int64_t i = 0; i < vaultFiles.m_count; i++)
    {
        sCharName = vaultFiles.m_pArray[i];
        sCharName = sCharName.SubString(
            0, sCharName.GetLength() - 4);

        CResGFF* pGff = new CResGFF(kResTypeBic, const_cast<char*>("BIC "), CResRef(sCharName));
        if (pGff->m_nValid != 0)
        {
            pGff->GetTopLevelStruct(&topStruct);

            NWPlayerCharacterList_st* pEntry = new NWPlayerCharacterList_st;
            pEntry->m_firstName = pGff->ReadFieldCExoLocString(&topStruct, const_cast<char*>("FirstName"),
                                                              bSuccess, CExoLocString());
            pEntry->m_lastName = pGff->ReadFieldCExoLocString(&topStruct, const_cast<char*>("LastName"),
                                                             bSuccess, CExoLocString());
            pEntry->m_resRef = sCharName;
            pEntry->m_nLocation = MSG_LOGIN_SERVER_CHARACTER;
            pEntry->m_nPortraitId =
                pGff->ReadFieldWORD(&topStruct, const_cast<char*>("PortraitId"), bSuccess, 0xffff);
            pEntry->m_portrait =
                pGff->ReadFieldCResRef(&topStruct, const_cast<char*>("Portrait"), bSuccess, CResRef(""));

            if (pGff->GetList(&classList, &topStruct, const_cast<char*>("ClassList")))
            {
                unsigned int nClasses = pGff->GetListCount(&classList);
                if (nClasses != 0)
                {
                    for (unsigned int j = 0; j < nClasses; j++)
                    {
                        if (pGff->GetListElement(&classStruct, &classList, j))
                        {
                            NWPlayerCharacterListClass_st cls;
                            cls.m_nClass =
                                pGff->ReadFieldINT(&classStruct, const_cast<char*>("Class"), bSuccess, -1);
                            cls.m_nLevel = static_cast<unsigned char>(pGff->ReadFieldSHORT(&classStruct, const_cast<char*>("ClassLevel"),
                                                                bSuccess, 0));
                            pEntry->m_classList.Add(cls);
                        }
                    }
                }
            }

            if (charList.DerefContains(pEntry))
                delete pEntry;
            else
                charList.AddUnique(pEntry);
        }
        delete pGff;
    }

    if (g_pAppManager->m_pServerExoApp->GetModule()->m_bIsSaveGame != 0)
    {
        g_pAppManager->m_pServerExoApp->GetModule()->PackPlayerCharacterListIntoMessage(pPlayer,
                                                                                       charList);
    }

    WriteWORD(static_cast<unsigned short>(charList.m_count), 16);

    for (int64_t i = 0; i < charList.m_count; i++)
    {
        NWPlayerCharacterList_st* pEntry = charList.m_pArray[i];

        WriteCExoLocStringServer(pEntry->m_firstName, 0);
        WriteCExoLocStringServer(pEntry->m_lastName, 0);
        WriteCResRef(pEntry->m_resRef, 16);
        WriteBYTE(pEntry->m_nLocation, 8);
        WriteWORD(pEntry->m_nPortraitId, 16);
        WriteCResRef(pEntry->m_portrait, 16);

        unsigned int nClasses = pEntry->m_classList.m_count;
        WriteBYTE(static_cast<unsigned char>(nClasses), 8);
        for (unsigned int j = 0; j < nClasses; j++)
        {
            WriteINT(pEntry->m_classList.m_pArray[j].m_nClass, 32);
            WriteBYTE(pEntry->m_classList.m_pArray[j].m_nLevel, 8);
        }

        delete pEntry;
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_CHAR_LIST, MSG_CHAR_LIST_LIST,
                                     pMsg, nSize);
}

// The three-byte send family builds a buffer, writes the three header bytes and the payload, sends
// and frees. Members differ only in the header bytes, where the player id comes from, and whether
// the send's result is returned.
int __attribute__((noinline)) CSWSMessage::SendServerToPlayerLogin_NeedCharacter(unsigned int nPlayerId)
{
    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_LOGIN;
    pMsg[2] = MSG_LOGIN_NEED_CHARACTER;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

int __attribute__((noinline)) CSWSMessage::SendServerToPlayerModule_Loading(CSWSPlayer* pPlayer)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_MODULE;
    pMsg[2] = MSG_MODULE_START_START_NEW_MODULE;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

// The only member of the family that broadcasts rather than targeting one player.
int CSWSMessage::SendServerToPlayerModule_StartStartNewModule()
{
    unsigned int nPlayerId = CNetLayer::PLAYERID_ALL_PLAYERS;

    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_MODULE;
    pMsg[2] = MSG_MODULE_DUMP_PLAYER;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

// The sub-opcode is computed rather than constant: 1 on success, 2 on failure. The send is
// written out once per arm.
int CSWSMessage::SendServerToPlayerLevelUp_Confirmation(unsigned int nPlayerId, int bSuccess)
{
    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_LEVEL_UP;

    int bSent;
    if (bSuccess)
    {
        pMsg[2] = MSG_LEVEL_UP_CONFIRMED;
        bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    }
    else
    {
        pMsg[2] = MSG_LEVEL_UP_REFUSED;
        bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    }

    delete[] pMsg;
    return bSent;
}

int CSWSMessage::SendServerToPlayerModule_DumpPlayer(CSWSPlayer* pPlayer)
{
    unsigned int nPlayerId = pPlayer->m_nPlayerId;

    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_MODULE;
    pMsg[2] = MSG_MODULE_LOADING;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

void CSWSMessage::ComputeGameObjectUpdateForYourself(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                                     CGameObjectArray* pArray,
                                                     unsigned long long objectId)
{
    ComputeGameObjectUpdateForObject(pPlayer, pObject, pArray, objectId);
}

// Resolves the object by id and picks a full update, a partial update, or deleting the stale
// snapshot. An object that always updates is marked visible unconditionally.
void CSWSMessage::ComputeGameObjectUpdateForObject(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                                   CGameObjectArray* pArray,
                                                   unsigned long long objectId)
{
    CGameObject* pGameObject;
    if (pArray->GetGameObject(objectId, &pGameObject) == CGameObjectArray::SUCCESS)
    {
        if (pGameObject != 0)
        {
            CSWSObject* pObj = pGameObject->AsSWSObject();
            int nVisible = TestObjectVisible(pObj, pObject);
#if !KOTOR1  // K1: no ForceAlwaysUpdate override
            if (pObj->m_bForceAlwaysUpdate != 0)
                nVisible = 1;
#endif
            pObj->m_bVisibleToPlayer = nVisible;
            if (nVisible == 1)
            {
                CLastUpdateObject* pLastUpdateObject = 0;
                unsigned int nUpdateType;
                unsigned int nUpdateFlags;
                TestObjectUpdateDifferences(pPlayer, pObj, &pLastUpdateObject, &nUpdateType,
                                            &nUpdateFlags);
                if ((nUpdateFlags | nUpdateType) != 0)
                    WriteGameObjUpdate_UpdateObject(pPlayer, pObj, pLastUpdateObject, nUpdateType,
                                                    nUpdateFlags);
                UpdateLastUpdateObject(pPlayer, pObj, pLastUpdateObject, nUpdateType);
                UpdateLastUpdateObjectAppearance(pObj, pLastUpdateObject, nUpdateFlags);
            }
            else
            {
                DeleteLastUpdateObjectsForObject(pPlayer, pObj->m_objectId);
            }
        }
    }
}

// Decides whether one object should be shipped to the player that owns the viewer: a 2-D range
// test against the viewer's spot and listen radius, plus per-object-type overrides.
int CSWSMessage::TestObjectVisible(CSWSObject* pObject, CSWSObject* pViewer)
{
    Vector vObject(pObject->m_position.x, pObject->m_position.y, 0.0f);
    Vector vViewer(pViewer->m_position.x, pViewer->m_position.y, 0.0f);

    // Off-area viewers - a party member carried between modules - fall back to the creature's cached
    // move-to position.
    int bUseVisibilityList = 1;
    if (pViewer->GetArea() == 0 && pViewer->AsSWSCreature() != 0)
    {
        CSWSCreature* pCreature = pViewer->AsSWSCreature();
        vViewer = Vector(pCreature->m_vDesiredAreaLocation.x, pCreature->m_vDesiredAreaLocation.y, 0.0f);
        bUseVisibilityList = 0;
    }

    float fDistanceSquared = (vObject.x - vViewer.x) * (vObject.x - vViewer.x)
                           + (vObject.y - vViewer.y) * (vObject.y - vViewer.y)
                           + (vObject.z - vViewer.z) * (vObject.z - vViewer.z);

    float fRange;
    if (pViewer->AsSWSCreature() != 0)
    {
        if (pViewer->AsSWSCreature()->GetSpotCheckDistance()
                >= pViewer->AsSWSCreature()->GetListenCheckDistance())
            fRange = pViewer->AsSWSCreature()->GetSpotCheckDistance();
        else
            fRange = pViewer->AsSWSCreature()->GetListenCheckDistance();
    }
    else
    {
        fRange = 250.0f;
    }

    // Object classes the client never needs a game-object update for.
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_WAYPOINT)
        return 0;
    if (pObject->AsSWSEncounter() != 0)
        return 0;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_STORE)
        return 0;
    if (pObject->AsSWSSoundObject() != 0)
        return 0;

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        pObject->GetArea();

        if (pViewer->AsSWSCreature() == 0)
            return 0;
        if (pObject->m_objectId == pViewer->m_objectId)
            return 1;
        if (pObject->AsSWSCreature()->m_bInParty != 0)
            return 1;

        if (bUseVisibilityList)
        {
            CSWVisibilityNode* pNode =
                pViewer->AsSWSCreature()->GetVisibleListElement(pObject->m_objectId);
            if (pNode == 0)
                return 0;
            return (pNode->m_flags & 3) != 0;
        }
        else
        {
#if KOTOR1  // K1: no ForceAlwaysUpdate override
            if (fDistanceSquared <= fRange * fRange)
                return pObject->AsSWSCreature()->GetInvisible(pViewer->AsSWSCreature()) == 0;
#else
            if (fDistanceSquared <= fRange * fRange || pObject->m_bForceAlwaysUpdate != 0)
                return pObject->AsSWSCreature()->GetInvisible(pViewer->AsSWSCreature()) == 0;
#endif
            return 0;
        }
    }
#if KOTOR1  // K1: no ForceAlwaysUpdate override
    else if (fDistanceSquared <= fRange * fRange)
    {
#else
    else if (fDistanceSquared <= fRange * fRange || pObject->m_bForceAlwaysUpdate != 0)
    {
#endif
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        {
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pViewer->m_objectId);

            if (pObject->AsSWSTrigger()->m_bTrap)
            {
                if (pObject->AsSWSTrigger()->m_bMineFlagged != 0)
                    return 1;
                if (pObject->AsSWSTrigger()->m_nFaction
                        == static_cast<unsigned int>(pViewer->AsSWSCreature()->m_pStats->m_factionId))
                    return 1;
                if (pObject->GetReputation(pViewer->m_objectId, 0) > 89)
                    return 1;
            }
            else
            {
                if (pObject->AsSWSTrigger()->m_bAreaTransition != 0)
                    return 1;
            }

            pObject->AsSWSTrigger();
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

// Looks the object's snapshot up in the player's list; when it is there, marks it dirty and
// computes the update type and flags, otherwise creates a fresh one.
void CSWSMessage::TestObjectUpdateDifferences(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                             CLastUpdateObject** ppLastUpdateObject,
                                             unsigned int* pnUpdateType, unsigned int* pnUpdateFlags)
{
    CGameObject* pGameObject = pPlayer->GetGameObject();
    CExoLinkedListNode* pNode = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
    while (pNode != 0)
    {
        CLastUpdateObject* pLUO = static_cast<CLastUpdateObject*>(pPlayer->m_pLastUpdateObjectList->m_pInternal->GetAtPos(pNode));
        *ppLastUpdateObject = pLUO;
        if (pLUO->m_objectId == pObject->m_objectId)
        {
            pLUO->m_bTouchedThisFrame = 1;
            *pnUpdateType = ComputeUpdateRequired(pPlayer, pObject, pLUO,
                                                  pGameObject->m_objectId == pLUO->m_objectId);
            *pnUpdateFlags = ComputeAppearanceUpdateRequired(pObject, *ppLastUpdateObject);
            return;
        }
        pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pNode);
    }
    *ppLastUpdateObject = CreateNewLastUpdateObject(pPlayer, pObject, pnUpdateType, pnUpdateFlags);
}

// Builds a snapshot record for an object the player has never seen and ships the full
// serialization of it, where the diffing family ships only the changed groups. The appearance
// block is seeded from the live creature for creatures and zeroed otherwise.
CLastUpdateObject* CSWSMessage::CreateNewLastUpdateObject(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                                         unsigned int* pnUpdateType,
                                                         unsigned int* pnUpdateFlags)
{
    CLastUpdateObject* pLast = new CLastUpdateObject();
    pLast->m_bTouchedThisFrame = 1;
    pLast->m_objectId = static_cast<unsigned int>(pObject->m_objectId);

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        CSWSCreature* pCreature = static_cast<CSWSCreature*>(pObject);
        pCreature->UpdateAppearanceForEquippedItems();
        pLast->m_appearance.m_headId = pCreature->m_appearance.m_headId;
        pLast->m_appearance.m_nPhenotype = pCreature->m_appearance.m_nPhenotype;
        pLast->m_appearance.m_nGender = pCreature->m_appearance.m_nGender;
        pLast->m_appearance.m_nColorSkin = pCreature->m_appearance.m_nColorSkin;
        pLast->m_appearance.m_nColorHair = pCreature->m_appearance.m_nColorHair;
        pLast->m_appearance.m_nColorTattoo1 = pCreature->m_appearance.m_nColorTattoo1;
        pLast->m_appearance.m_nColorTattoo2 = pCreature->m_appearance.m_nColorTattoo2;
        pLast->m_appearance.m_nAppearanceHead = pCreature->m_appearance.m_nAppearanceHead;
        pLast->m_appearance.m_nTail = pCreature->m_appearance.m_nTail;
        pLast->m_appearance.m_nWings = pCreature->m_appearance.m_nWings;
        pLast->m_appearance.m_itemId[1] = pCreature->m_appearance.m_itemId[1];
        pLast->m_appearance.m_itemId[0] = pCreature->m_appearance.m_itemId[0];
        pLast->m_appearance.m_bUseBackupHead = pCreature->m_appearance.m_bUseBackupHead;
        pLast->m_appearance.m_nDuplicatingHead = pCreature->m_appearance.m_nDuplicatingHead;
        pLast->m_nSoundSetFile = pCreature->m_nSoundSetFile;

        for (int i = 0; i < 36; i++)
        {
            pLast->m_aReserved1[i].m_nReserved1 = 0;
            pLast->m_aReserved1[i].m_objectId = CGameObjectArray::INVALIDOBJID;
            pLast->m_aReserved1[i].m_nReserved2 = 0xff;
            pLast->m_aReserved1[i].m_nReserved3 = 0;
        }
        *pnUpdateFlags = MSG_APPEARANCE_UPDATE_ALL;
    }
    else
    {
        pLast->m_appearance = CSWSCreatureAppearanceInfo();
    }

    unsigned char nType = pObject->m_objectType;
    if (CGameObject::OBJECT_TYPE_DOOR == nType)
        *pnUpdateType = static_cast<unsigned int>(~MSG_OBJECT_UPDATE_VISUAL_EFFECTS);
    else if (CGameObject::OBJECT_TYPE_ITEM == nType)
        *pnUpdateType = static_cast<unsigned int>(
            ~(MSG_OBJECT_UPDATE_ANIMATION | MSG_OBJECT_UPDATE_VISUAL_EFFECTS));
    else if (CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT == nType)
        *pnUpdateType = static_cast<unsigned int>(-1);
    else if (CGameObject::OBJECT_TYPE_TRIGGER == nType)
        *pnUpdateType = static_cast<unsigned int>(
            ~(MSG_OBJECT_UPDATE_ANIMATION | MSG_OBJECT_UPDATE_VISUAL_EFFECTS));
    else if (CGameObject::OBJECT_TYPE_PLACEABLE == nType)
        *pnUpdateType = static_cast<unsigned int>(~MSG_OBJECT_UPDATE_VISUAL_EFFECTS);
    else
    {
        *pnUpdateType = MSG_OBJECT_UPDATE_POSITION | MSG_OBJECT_UPDATE_ORIENTATION |
                        MSG_OBJECT_UPDATE_ANIMATION | MSG_OBJECT_UPDATE_PORTRAIT |
                        MSG_OBJECT_UPDATE_AI_STATE | MSG_OBJECT_UPDATE_MOVE_RATES |
                        MSG_OBJECT_UPDATE_DAMAGE_LEVEL | MSG_OBJECT_UPDATE_IDENTITY |
                        MSG_OBJECT_UPDATE_AREA_CHANGED | MSG_OBJECT_UPDATE_REMOVAL;
        if (pPlayer != 0)
            pPlayer->GetGameObject();
    }
    K2_ASSUME(pPlayer != 0);

    UpdateLastUpdateObject(pPlayer, pObject, pLast, *pnUpdateType);
    WriteCHAR('A', 8);
    WriteBYTE(nType, 8);
    WriteOBJECTIDServer(pObject->m_objectId);

    if (CGameObject::OBJECT_TYPE_CREATURE == nType)
    {
        WriteBYTE(pObject->AsSWSCreature()->m_bInParty != 0, 8);
        WriteFLOAT(pObject->m_position.x, 1.0f, 32);
        WriteFLOAT(pObject->m_position.y, 1.0f, 32);
        WriteFLOAT(pObject->m_position.z, 1.0f, 32);
        WriteFLOAT(pObject->m_orientation.x, 1.0f, 32);
        WriteFLOAT(pObject->m_orientation.y, 1.0f, 32);
        WriteFLOAT(pObject->m_orientation.z, 1.0f, 32);
        WriteWORD(static_cast<unsigned short>(pObject->m_nAnimation), 16);
    }
    else if (CGameObject::OBJECT_TYPE_ITEM == nType)
    {
        AddItemAppearanceToMessage(pObject->AsSWSItem());
        AddActiveItemPropertiesToMessage(pObject->AsSWSItem(),
                                        pPlayer->GetGameObject()->AsSWSCreature());
    }
    else if (CGameObject::OBJECT_TYPE_DOOR == nType)
    {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        WriteBOOL(pDoor->m_bStatic);
        WriteBYTE(pDoor->m_appearance, 8);
        if (pDoor->m_appearance == 0)
            WriteBYTE(pDoor->m_genericType, 8);
        int bIsOwned;
        if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId, 0) <= kReputationMaxNeutral
            && pObject->AsSWSDoor()->m_nFaction
                   != static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId))
            bIsOwned = 1;
        else
            bIsOwned = 0;
        WriteCExoLocStringServer(pObject->AsSWSDoor()->GetFirstName(), 0);
        WriteBOOL(bIsOwned);
        WriteWORD(static_cast<unsigned short>(pObject->AsSWSDoor()->m_nAnimation), 16);
        WriteBOOL(pObject->AsSWSDoor()->m_bLocked);
        WriteBOOL(pObject->AsSWSDoor()->m_bLockable);
        WriteBOOL(pObject->m_bPlot);
#if !KOTOR1  // K1: no NotBlastable bit on the wire
        WriteBOOL(pObject->AsSWSDoor()->m_bNotBlastable);
#endif
        WriteBOOL(pObject->AsSWSDoor()->GetIsLinked());
        WriteCExoLocStringServer(pObject->AsSWSDoor()->m_transitionDestination, 0);
    }
    else if (CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT == nType)
    {
        CSWSAreaOfEffectObject* pAOE = pObject->AsSWSAreaOfEffectObject();
        WriteINT(pAOE->m_nAreaEffectId, 32);
        WriteOBJECTIDServer(pAOE->m_oidLinkedToObject);
    }
    else if (CGameObject::OBJECT_TYPE_TRIGGER == nType)
    {
        WriteCExoLocStringServer(pObject->AsSWSTrigger()->GetFirstName(), 0);
        WriteBOOL(pObject->AsSWSTrigger()->m_bTrap);
        WriteBOOL(pObject->AsSWSTrigger()->m_bAreaTransition);
        if (pObject->AsSWSTrigger()->m_bTrap != 0)
            WriteBYTE(10, 8);
        else
            WriteBYTE(pObject->AsSWSTrigger()->m_cursor, 8);
        if (pObject->AsSWSTrigger()->m_bTrap != 0)
        {
            int bIsOwned;
            if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId,
                                       0) <= kReputationMaxNeutral
                && pObject->AsSWSTrigger()->m_nFaction
                       != static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId))
                bIsOwned = 1;
            else
                bIsOwned = 0;
            WriteBOOL(bIsOwned);
            WriteBYTE(pObject->AsSWSTrigger()->m_trapType, 8);
        }
        WriteFLOAT(pObject->AsSWSTrigger()->m_fHighlightHeight, 1.0f, 32);
        WriteCExoLocStringServer(pObject->AsSWSTrigger()->m_transitionDestination, 0);
        AddTriggerGeometryToMessage(pObject->AsSWSTrigger());
    }
    else if (CGameObject::OBJECT_TYPE_PLACEABLE == nType)
    {
        WriteCExoLocStringServer(pObject->AsSWSPlaceable()->GetFirstName(), 0);
        int bIsOwned;
        if (pObject->GetReputation(pPlayer->GetGameObject()->m_objectId, 0) <= kReputationMaxNeutral
            && pObject->AsSWSPlaceable()->m_nFaction
                   != static_cast<unsigned int>(pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->m_factionId))
            bIsOwned = 1;
        else
            bIsOwned = 0;
        WriteBOOL(bIsOwned);
        AddPlaceableAppearanceToMessage(pObject->AsSWSPlaceable());
    }

    pPlayer->m_pLastUpdateObjectList->m_pInternal->AddHead(pLast);
    return pLast;
}

int CSWSMessage::SendServerToPlayerDialogClose(unsigned int nPlayerId)
{
    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_DIALOG;
    pMsg[2] = MSG_DIALOG_CLOSE;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

int CSWSMessage::SendServerToPlayerLogin_Confirm(unsigned int nPlayerId)
{
    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_LOGIN;
    pMsg[2] = MSG_LOGIN_CONFIRM;

    int bSent = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg,
                                                                                   3, 0);
    delete[] pMsg;
    return bSent;
}

// Party invites are a dead multiplayer hook; the body is a bare `return 1`.
int CSWSMessage::SendServerToPlayerParty_Invite(unsigned int nPlayerId,
                                                unsigned long long objectId)
{
    K2_UNUSED(nPlayerId); K2_UNUSED(objectId);
    return 1;
}

int CSWSMessage::SendServerToPlayerDestroyDeathGUI(unsigned int nPlayerId)
{
    unsigned char* pMsg = new unsigned char[3];
    m_nPlayerId = MSG_PLAYERID_ALL_CLIENTS;
    pMsg[0] = MSG_SERVER_TO_PLAYER;
    pMsg[1] = MSG_MAJOR_POPUP_GUI_PANEL;
    pMsg[2] = MSG_POPUP_GUI_PANEL_DESTROY_DEATH;

    g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nPlayerId, pMsg, 3, 0);
    delete[] pMsg;
    return 1;
}

// Copies the live action queue over the snapshot field by field - a block move would copy the
// alignment hole.
void CSWSMessage::UpdateLastUpdateActionQueue(CSWSCreature* pCreature,
                                              CSWSPlayerLastUpdateObject* pLast)
{
    for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
    {
        pLast->m_actionQueue[i].m_nActionId = pCreature->m_actionQueue[i].m_nActionId;
        pLast->m_actionQueue[i].m_nGroupId  = pCreature->m_actionQueue[i].m_nGroupId;
        pLast->m_actionQueue[i].m_nAltActionId   = pCreature->m_actionQueue[i].m_nAltActionId;
        pLast->m_actionQueue[i].m_targetId  = pCreature->m_actionQueue[i].m_targetId;
        pLast->m_actionQueue[i].m_vTarget   = pCreature->m_actionQueue[i].m_vTarget;
        pLast->m_actionQueue[i].m_nSpellId  = pCreature->m_actionQueue[i].m_nSpellId;
    }
}

// True as soon as any snapshot slot disagrees with the live queue. The spell id is not compared.
int CSWSMessage::ComputeLastUpdate_ActionQueue(CSWSCreature* pCreature,
                                               CSWSPlayerLastUpdateObject* pLast)
{
    for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
    {
        if (pCreature->m_actionQueue[i].m_nActionId != pLast->m_actionQueue[i].m_nActionId)
            return 1;
        if (pCreature->m_actionQueue[i].m_nGroupId != pLast->m_actionQueue[i].m_nGroupId)
            return 1;
        if (pCreature->m_actionQueue[i].m_nAltActionId != pLast->m_actionQueue[i].m_nAltActionId)
            return 1;
        if (pCreature->m_actionQueue[i].m_targetId != pLast->m_actionQueue[i].m_targetId)
            return 1;
        if (!equal(pCreature->m_actionQueue[i].m_vTarget, pLast->m_actionQueue[i].m_vTarget))
            return 1;
    }

    return 0;
}

// Replaces the player's visibility snapshot wholesale: drain the old nodes, then deep-copy the
// creature's live list into fresh ones.
void CSWSMessage::UpdateLastUpdateVisibilityList(CSWSCreature* pCreature,
                                                 CSWSPlayerLastUpdateObject* pLast)
{
    for (int i = pLast->m_visibilityList.m_count - 1; i >= 0; i--)
    {
        CSWVisibilityNode* pNode = pLast->m_visibilityList.m_pArray[i];
        pLast->m_visibilityList.DelIndex(i);
        delete pNode;
    }

    for (int i = 0; i < pCreature->m_visibilityList.m_count; i++)
    {
        CSWVisibilityNode* pSrc = pCreature->m_visibilityList.m_pArray[i];

        CSWVisibilityNode* pNew = new CSWVisibilityNode;
        pNew->m_objectId = pSrc->m_objectId;
        pNew->m_flags = pSrc->m_flags & 2;
        pNew->m_flags |= pSrc->m_flags & 1;
        pNew->m_flags |= pSrc->m_flags & kVisibleInactive;

        pLast->m_visibilityList.Add(pNew);
    }
}

// Packs the player-state word the client's GUI mirrors: detect mode, the solo latch, one bit
// per reported combat mode and five loose flags.
int CSWSMessage::ComputeLastUpdate_PlayerState(CSWSCreature* pCreature)
{
    int nState = 0;

    if (pCreature->GetDetectMode() == 1)
        nState |= MSG_PLAYER_STATUS_DETECT;
    if (pCreature->m_stealthMode == 1)
        nState |= MSG_PLAYER_STATUS_STEALTH;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_PARRY)
        nState |= MSG_PLAYER_STATUS_PARRY;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_POWER_ATTACK)
        nState |= MSG_PLAYER_STATUS_POWER_ATTACK;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK)
        nState |= MSG_PLAYER_STATUS_IMPROVED_POWER_ATTACK;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_FLURRY)
        nState |= MSG_PLAYER_STATUS_FLURRY;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_RAPID_SHOT)
        nState |= MSG_PLAYER_STATUS_RAPID_SHOT;
    if (pCreature->m_bMovementSpeedLimited == 1)
        nState |= MSG_PLAYER_STATUS_MOVEMENT_SPEED_LIMITED;
    if (pCreature->m_nSpecialWalkAnimation != 0)
        nState |= MSG_PLAYER_STATUS_SPECIAL_WALK_ANIMATION;
    if (pCreature->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_COUNTERSPELL)
        nState |= MSG_PLAYER_STATUS_COUNTERSPELL;
    if (pCreature->m_bDisguised != 0)
        nState |= MSG_PLAYER_STATUS_DISGUISED;
    if (pCreature->m_bCommandable != 0)
        nState |= MSG_PLAYER_STATUS_COMMANDABLE;
    if (pCreature->m_nExcitedStateTimer != 0)
        nState |= MSG_PLAYER_STATUS_EXCITED;

    return nState;
}

// The broadcast entry point for localized chat. Fans one line out to the players in range, with the
// optional id list narrowing who may receive it.
int CSWSMessage::SendServerToPlayerChatMultiLangMessage(unsigned char nLanguage,
                                                        unsigned long long speakerId,
                                                        CExoLocString& sText,
                                                        unsigned long long targetId,
                                                        unsigned char nLang,
                                                        unsigned int* pnPlayers,
                                                        unsigned int nNumPlayers, int bShout,
                                                        const CResRef& rSound,
                                                        unsigned long long soundId, int nVolume)
{
    if (nLanguage != 8)
        return 0;

    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pList == 0)
        return 0;

    CExoLinkedListNode* pNode;
    CGameObject* pObject;
    CSWArea* pSrcArea K2_UNSET;
    Vector srcPos;
    int bSent;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()
            ->GetGameObject(speakerId, &pObject) == CGameObjectArray::SUCCESS)
    {
        CSWSObject* pSpeaker = static_cast<CSWSObject*>(pObject);
        pSrcArea = pSpeaker->GetArea();
        srcPos = pSpeaker->m_position;
        bSent = 1;
    }
    else
    {
        srcPos = Vector(0.0f, 0.0f, 0.0f);
        bSent = 0;
    }

    pNode = pList->GetHeadPos();
    while (bSent == 1 && pNode != 0)
    {
        CSWSPlayer* pOther = reinterpret_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
        bSent = 1;   // keep looping by default; a send's result below may kill the loop
        if (pOther->GetGameObject() != 0)
        {
            int bFound = 0;
            for (unsigned int i = 0; i < nNumPlayers; i++)
            {
                if (pnPlayers[i] == pOther->m_nPlayerId)
                {
                    bFound = 1;
                    break;
                }
            }

            int bOnList;
            int bShoutScope;
            if (bFound)
            {
                bShoutScope = 0;
                bOnList = 1;
            }
            else
            {
                bOnList = 0;
                bShoutScope = bShout;
            }

            CSWSObject* pOtherObject = pOther->GetGameObject();
            if (bShoutScope == 0)
            {
                Vector vDelta = pOtherObject->m_position - srcPos;
                if (pSrcArea == pOther->GetGameObject()->GetArea())
                {
                    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                    + vDelta.z * vDelta.z;
                    if (fDistSq <= CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE
                                       * CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE)
                        bSent = SendServerToPlayerChatMultiLang_Helper(
                                    pOther->m_nPlayerId, 8, speakerId, sText, targetId,
                                    nLang, bOnList, rSound, soundId, nVolume);
                }
            }
        }
        pList->GetNext(pNode);
    }
    return bSent;
}

// The entire body is gated on language 8; any other language sends nothing.
int CSWSMessage::SendServerToPlayerChatMultiLang_Helper(unsigned int nPlayerId,
                                                        unsigned char nLanguage,
                                                        unsigned long long speakerId,
                                                        CExoLocString& sText,
                                                        unsigned long long targetId,
                                                        unsigned char nLang, int bShout,
                                                        const CResRef& rSound,
                                                        unsigned long long soundId, int nVolume)
{
    if (nLanguage != 8)
        return 0;

    unsigned char* pMsg;
    unsigned int nSize = 52;

    CreateWriteMessage(52, nPlayerId, 1);
    WriteOBJECTIDServer(speakerId);
    WriteOBJECTIDServer(targetId);
    WriteCExoLocStringServer(sText, nLang);
    WriteCResRef(rSound, 16);
    WriteOBJECTIDServer(soundId);
    WriteINT(nVolume, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT,
                                     bShout != 0 ? MSG_CHAT_MULTI_LANGUAGE_SHOUT
                                                 : MSG_CHAT_MULTI_LANGUAGE_TALK,
                                     pMsg, nSize);
}

int CSWSMessage::SendServerToPlayerSafeProjectileProjectile(CSWSPlayer* pPlayer,
                                                            unsigned long long sourceId,
                                                            unsigned long long targetId,
                                                            Vector vSource, Vector vTarget,
                                                            unsigned int nA, unsigned char nB,
                                                            int nC, unsigned char nD,
                                                            unsigned char nE, int nF)
{
    unsigned char* pMsg = 0;
    unsigned int nSize = 0;

    CreateWriteMessage(55, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(sourceId);
    WriteOBJECTIDServer(targetId);
    WriteVector(vSource);
    WriteVector(vTarget);
    WriteDWORD(nA, 32);
    WriteBYTE(nB, 8);
    WriteINT(nC, 32);
    WriteBYTE(nD, 8);
    WriteBYTE(nE, 8);
    WriteINT(nF, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SAFE_PROJECTILE,
                                     MSG_SAFE_PROJECTILE_PROJECTILE, pMsg, nSize);
}

// The payload order is not the parameter order. The two locstrings lead, then the date and time
// pair, then the plot tag and the scalars.
int CSWSMessage::SendServerToPlayerJournalAddQuest(CSWSPlayer* pPlayer, const CExoString& sPlotId,
                                                   int nState, unsigned int nPriority,
                                                   unsigned short nPicture, int bEnd,
                                                   unsigned int nDate, unsigned int nTime,
                                                   CExoLocString& locName, CExoLocString& locText,
                                                   int nPlotIndex, int nPlanetId,
                                                   int nQuestEntryXP)
{
    unsigned char* pMsg;
    unsigned int nSize = sPlotId.GetLength() + 22;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoLocStringServer(locName, 0);
    WriteCExoLocStringServer(locText, 0);
    WriteDWORD(nDate, 32);
    WriteDWORD(nTime, 32);
    WriteCExoString(sPlotId, 32);
    WriteINT(nState, 32);
    WriteDWORD(nPriority, 32);
    WriteWORD(nPicture, 16);
    WriteBOOL(bEnd);
    WriteINT(nPlotIndex, 32);
    WriteINT(nPlanetId, 32);
    WriteINT(nQuestEntryXP, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_JOURNAL,
                                     MSG_JOURNAL_ADD_QUEST, pMsg, nSize);
}

extern CTlkTable* g_pTlkTable;

// Broadcasts every non-reserved custom token to one client. An empty table, or one whose
// entries are all reserved, sends nothing and reports success.
int CSWSMessage::SendServerToPlayerSetCustomTokenList(unsigned int nPlayerId)
{
    unsigned int nCount = g_pTlkTable->m_nCustomTokenCount;
    int nSend = nCount;
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (g_pTlkTable->m_pCustomTokens[i].m_nId <= CTlkTable::CUSTOMTOKEN_RESERVED_END)
            nSend--;
    }

    if (nSend == 0)
        return 1;

    unsigned char* pMsg;
    unsigned int nSize = 4 + nSend * 8;

    CreateWriteMessage(nSize, nPlayerId, 1);
    WriteDWORD(nSend, 32);

    for (unsigned int i = 0; i < nCount; i++)
    {
        if (g_pTlkTable->m_pCustomTokens[i].m_nId > CTlkTable::CUSTOMTOKEN_RESERVED_END)
        {
            WriteINT(g_pTlkTable->m_pCustomTokens[i].m_nId, 32);
            WriteCExoString(g_pTlkTable->m_pCustomTokens[i].m_sValue, 32);
        }
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CUSTOM_TOKEN, MSG_CUSTOM_TOKEN_SET_LIST,
                                     pMsg, nSize);
}

// The only speaker-chat member that does not bail when the speaker is gone. It substitutes the
// invalid-object id, a zero position and two default locstrings and sends anyway.
int CSWSMessage::SendServerToPlayerChat_Shout(unsigned int nPlayerId, unsigned long long speakerId,
                                              const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(speakerId);

    CreateWriteMessage(sText.GetLength() + 24, MSG_PLAYERID_ALL_CLIENTS, 1);

    if (pCreature != 0)
    {
        WriteOBJECTIDServer(speakerId);
        WriteCExoString(sText, 32);
        WriteFLOAT(pCreature->m_position.x, 1.0f, 32);
        WriteFLOAT(pCreature->m_position.y, 1.0f, 32);
        WriteFLOAT(pCreature->m_position.z, 1.0f, 32);
        WriteCExoLocStringServer(pCreature->GetFirstName(), 0);
        WriteCExoLocStringServer(pCreature->GetLastName(), 0);
    }
    else
    {
        CExoLocString locFirstName;
        CExoLocString locLastName;

        locFirstName.m_nStrRef = STRREF_BLANK;

        WriteOBJECTIDServer(OBJECT_INVALID);
        WriteCExoString(sText, 32);
        WriteFLOAT(0.0f, 1.0f, 32);
        WriteFLOAT(0.0f, 1.0f, 32);
        WriteFLOAT(0.0f, 1.0f, 32);
        WriteCExoLocStringServer(locFirstName, 0);
        WriteCExoLocStringServer(locLastName, 0);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CHAT, MSG_CHAT_SHOUT, pMsg, nSize);
}

// Unlike its projectile twin, both vectors go out as six individual floats.
int CSWSMessage::SendServerToPlayerSafeProjectileSpell(CSWSPlayer* pPlayer,
                                                       unsigned long long sourceId,
                                                       unsigned long long targetId, Vector vSource,
                                                       Vector vTarget, unsigned int nA,
                                                       unsigned char nType, unsigned int nB,
                                                       unsigned char nC, unsigned char nD,
                                                       unsigned char nE, unsigned int nF, int nG)
{
    CreateWriteMessage(57, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(sourceId);
    WriteOBJECTIDServer(targetId);
    WriteFLOAT(vSource.x, 1.0f, 32);
    WriteFLOAT(vSource.y, 1.0f, 32);
    WriteFLOAT(vSource.z, 1.0f, 32);
    WriteFLOAT(vTarget.x, 1.0f, 32);
    WriteFLOAT(vTarget.y, 1.0f, 32);
    WriteFLOAT(vTarget.z, 1.0f, 32);
    WriteDWORD(nA, 32);
    WriteBYTE(nType, 8);
    WriteDWORD(nF, 32);
    WriteINT(nG, 32);

    if (nType == 6)
    {
        WriteDWORD(nB, 32);
    }
    else if (nType == 7)
    {
        WriteDWORD(nB, 32);
        WriteBYTE(nE, 8);
    }
    else
    {
        WriteBYTE(nE, 8);
        WriteBYTE(nC, 8);
        WriteBYTE(nD, 8);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_SAFE_PROJECTILE,
                                     MSG_SAFE_PROJECTILE_SPELL, pMsg, nSize);
}

// One entry of the "who else is in this module" roster; the lookup's result gates everything
// after the two leading fields.
int CSWSMessage::SendServerToPlayerPlayModuleCharacterListResponse(unsigned int nPlayerId,
                                                                   unsigned long long objectId,
                                                                   int bFound)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bFound);
    WriteDWORD(static_cast<unsigned int>(objectId), 32);

    if (bFound != 0)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
        if (pCreature == 0)
            return 0;

        CExoLocString locFirstName;
        CExoLocString locLastName;

        unsigned char nGender = pCreature->GetGender();
        locFirstName = pCreature->GetFirstName();
        locLastName = pCreature->GetLastName();
        WriteCExoLocStringServer(locFirstName, nGender);
        WriteCExoLocStringServer(locLastName, nGender);

        unsigned short nPortraitId = pCreature->GetPortraitId();
        WriteWORD(nPortraitId, 16);
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            WriteCResRef(pCreature->GetPortrait(), 16);

        unsigned char nClasses = pCreature->m_pStats->m_numClasses;
        WriteBYTE(nClasses, 8);

        for (int i = 0; i < nClasses; i++)
        {
            unsigned char nClass = pCreature->m_pStats->GetClass(static_cast<unsigned char>(i));
            unsigned char nLevel = pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0);
            WriteBYTE(nClass, 8);
            WriteBYTE(nLevel, 8);
        }
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST,
                                     MSG_PLAY_MODULE_CHARACTER_LIST_RESPONSE, pMsg, nSize);
}

// Both the source and the target are resolved through the object array. A failed source lookup
// only skips the conversion, so the body runs on with a null pointer.
int CSWSMessage::SendServerToPlayerGameObjUpdateVisEffect(CSWSPlayer* pPlayer,
                                                          unsigned short nEffectId,
                                                          unsigned long long targetId,
                                                          unsigned long long sourceId,
                                                          unsigned long long beamEndId,
                                                          unsigned char nA, unsigned char nB,
                                                          Vector vPos, float fValue)
{
    CGameObject* pGameObjSource = g_pAppManager->m_pServerExoApp->GetGameObject(sourceId);
    CGameObject* pGameObjTarget = g_pAppManager->m_pServerExoApp->GetGameObject(targetId);

    CSWSObject* pSource = (pGameObjSource != 0) ? pGameObjSource->AsSWSObject() : 0;
    CSWSObject* pTarget = (pGameObjTarget != 0) ? pGameObjTarget->AsSWSObject() : 0;

    CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDServer(targetId);
    WriteWORD(nEffectId, 16);

    if (pTarget != 0)
    {
        WriteFLOAT(pTarget->m_position.x, 1.0f, 32);
        WriteFLOAT(pTarget->m_position.y, 1.0f, 32);
        WriteFLOAT(pTarget->m_position.z, 1.0f, 32);
    }
    else
    {
        WriteFLOAT(vPos.x, 1.0f, 32);
        WriteFLOAT(vPos.y, 1.0f, 32);
        WriteFLOAT(vPos.z, 1.0f, 32);
    }

    if (CLoopingVisualEffect::GetIsBeam(nEffectId) != 0)
    {
        WriteOBJECTIDServer(sourceId);
        WriteOBJECTIDServer(beamEndId);
        WriteBYTE(nA, 8);
        WriteBYTE(nB, 8);
#ifdef K2_FIX_ENGINE_BUGS
        if (pSource == 0)
        {
            WriteFLOAT(vPos.x, 1.0f, 32);
            WriteFLOAT(vPos.y, 1.0f, 32);
            WriteFLOAT(vPos.z, 1.0f, 32);
        }
        else
#endif
        {
            WriteFLOAT(pSource->m_position.x, 1.0f, 32);
            WriteFLOAT(pSource->m_position.y, 1.0f, 32);
            WriteFLOAT(pSource->m_position.z, 1.0f, 32);
        }

        if (targetId == OBJECT_INVALID)
            WriteFLOAT(fValue, 1.0f, 32);
    }

    unsigned char* pMsg;
    unsigned int nSize;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                     MSG_GAME_OBJECT_UPDATE_VISUAL_EFFECT, pMsg, nSize);
}

// Buckets one object into the update category ladder: creature, door, placeable, item, area of
// effect, trigger, and a skip value.
unsigned int CSWSMessage::SelectCategoryForGameObject(CGameObject* pGameObject, CSWSObject* pObject)
{
    if (pGameObject->AsSWSCreature() != 0)
    {
        CSWSCreature* pCreature = pGameObject->AsSWSCreature();
        if (pObject->m_objectId == pGameObject->m_objectId)
            return 0;
        return (pCreature->m_bCombatState == 1) ? 1 : 4;
    }

    if (pGameObject->AsSWSDoor() != 0)
        return 6;

    if (pGameObject->AsSWSPlaceable() != 0)
        return (pGameObject->AsSWSPlaceable()->m_bStatic == 0) ? 8 : kUpdateCategoryNone;

    if (pGameObject->AsSWSItem() != 0)
        return 7;

    if (pGameObject->AsSWSAreaOfEffectObject() != 0)
        return 9;

    if (pGameObject->AsSWSTrigger() != 0)
        return 5;

    return kUpdateCategoryNone;
}

// Builds the per-frame work list: every object in the viewer's area that is close enough, or
// that the viewer already has a snapshot of, bucketed by category and insertion-sorted by
// category and distance into a `new[]` array freed once the frame is built. The array grows in
// blocks of 32.
CSWSPlayerLUOSortedObjectList* CSWSMessage::SortObjectsForGameObjectUpdate(
    CSWSPlayer* pPlayer, CSWSObject* pObject, CGameObjectArray* pArray, int* pnCount)
{
    CSWSPlayerLUOSortedObjectList* pSorted = 0;
    int nCount = 0;

    CSWSArea* pArea = 0;
    if (pObject != 0)
        pArea = static_cast<CSWSArea*>(pObject->GetArea());

    float fBaseX = 0.0f;
    float fBaseY = 0.0f;
    int bHaveBase = 0;

    if (pArea != 0)
    {
        fBaseX = pObject->m_position.x;
        fBaseY = pObject->m_position.y;
        bHaveBase = 1;
    }
#ifdef K2_FIX_ENGINE_BUGS
    else if (pObject != 0)
#else
    else
#endif
    {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
        {
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            if (pModule != 0)
            {
                pArea = pModule->GetArea();
                if (pArea != 0)
                {
                    fBaseX = pCreature->m_vDesiredAreaLocation.x;
                    fBaseY = pCreature->m_vDesiredAreaLocation.y;
                    bHaveBase = 1;
                }
            }
        }
    }

    if (bHaveBase)
    {
        unsigned long long objectId;
        if (pArea->GetFirstObjectInArea(objectId) == 1)
        {
            CGameObject* pGameObject = 0;
            int nCapacity = 0;
            do
            {
                if (CGameObjectArray::SUCCESS != pArray->GetGameObject(objectId, &pGameObject))
                    continue;
                if (pGameObject == 0)
                    continue;
                if (pGameObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
                    continue;

                float fDX = static_cast<CSWSObject*>(pGameObject)->m_position.x - fBaseX;
                float fDY = static_cast<CSWSObject*>(pGameObject)->m_position.y - fBaseY;
                // The trailing + z*z is real. The range is measured in 2D but the expression is
                // 3-D, so z is 0 and the term adds nothing.
                float fDZ = 0.0f;
                float fDistanceSquared = fDX * fDX + fDY * fDY + fDZ * fDZ;

                float fRange;
                if (pObject->AsSWSCreature() != 0)
                {
                    float fSpot = pObject->AsSWSCreature()->GetSpotCheckDistance();
                    float fListen = pObject->AsSWSCreature()->GetListenCheckDistance();
                    if (fSpot >= fListen)
                        fRange = pObject->AsSWSCreature()->GetSpotCheckDistance();
                    else
                        fRange = pObject->AsSWSCreature()->GetListenCheckDistance();
                }
                else
                {
                    fRange = 250.0f;
                }

#if KOTOR1  // K1: no ForceAlwaysUpdate override
                int bKeep = fDistanceSquared < fRange * fRange ||
                            pPlayer->GetLastUpdateObject(pObject->m_objectId) != 0;
#else
                int nAlwaysSend = 0;
                if (pGameObject->AsSWSObject() != 0)
                    nAlwaysSend = pGameObject->AsSWSObject()->m_bForceAlwaysUpdate;

                int bKeep = fDistanceSquared < fRange * fRange || nAlwaysSend != 0 ||
                            pPlayer->GetLastUpdateObject(pObject->m_objectId) != 0;
#endif

                unsigned int nCategory = SelectCategoryForGameObject(pGameObject, pObject);
                if (bKeep && nCategory != kUpdateCategoryNone)
                {
                    if (nCapacity == nCount)
                    {
                        nCapacity = nCount + 32;
                        CSWSPlayerLUOSortedObjectList* pNew =
                            new CSWSPlayerLUOSortedObjectList[nCapacity];
                        for (int i = 0; i < nCapacity; i++)
                        {
                            pNew[i].m_objectId = OBJECT_INVALID;
                            pNew[i].m_fDistance = 0.0f;
                            pNew[i].m_nCategory = 0;
                        }
                        if (nCount > 0)
                        {
                            for (int i = 0; i < nCount; i++)
                            {
                                pNew[i].m_fDistance = pSorted[i].m_fDistance;
                                pNew[i].m_nCategory = pSorted[i].m_nCategory;
                                pNew[i].m_objectId = pSorted[i].m_objectId;
                            }
                            delete[] pSorted;
                        }
                        pSorted = pNew;
                    }

                    int nSlot = nCount;
                    while (nSlot >= 1 &&
                           (nCategory < static_cast<unsigned int>(pSorted[nSlot - 1].m_nCategory) ||
                            (nCategory == static_cast<unsigned int>(pSorted[nSlot - 1].m_nCategory) &&
                             fDistanceSquared < pSorted[nSlot - 1].m_fDistance *
                                                pSorted[nSlot - 1].m_fDistance)))
                    {
                        pSorted[nSlot].m_fDistance = pSorted[nSlot - 1].m_fDistance;
                        pSorted[nSlot].m_nCategory = pSorted[nSlot - 1].m_nCategory;
                        pSorted[nSlot].m_objectId = pSorted[nSlot - 1].m_objectId;
                        nSlot--;
                    }

                    pSorted[nSlot].m_fDistance = __builtin_sqrtf(fDistanceSquared);
                    pSorted[nSlot].m_nCategory = nCategory;
                    pSorted[nSlot].m_objectId = objectId;
                    nCount++;
                }
            } while (pArea->GetNextObjectInArea(objectId) == 1);
        }
    }

    *pnCount = nCount;
    return pSorted;
}

// Walks the player's snapshot list and drops every record whose object has left the viewer's
// area, telling the client to forget it on the way out.
void CSWSMessage::DeleteLastUpdateObjectsInOtherAreas(CSWSPlayer* pPlayer)
{
    CSWSObject* pPlayerObject = pPlayer->GetGameObject();

    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray == 0)
        return;

    CExoLinkedListNode* pPos = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
    while (pPos != 0)
    {
        CLastUpdateObject* pLast = static_cast<CLastUpdateObject*>(
            pPlayer->m_pLastUpdateObjectList->GetAtPos(pPos));

        if (pPlayerObject != 0 && pLast->m_objectId == pPlayerObject->m_objectId)
        {
            pLast->m_bTouchedThisFrame = 0;
            pPlayer->m_pLastUpdateObjectList->GetNext(pPos);
            continue;
        }

        CGameObject* pGameObject;
        if (CGameObjectArray::SUCCESS == pArray->GetGameObject(pLast->m_objectId, &pGameObject)
            && pGameObject->AsSWSObject() != 0
            && pPlayerObject != 0
            && (pGameObject->AsSWSObject()->GetArea() == pPlayerObject->GetArea()
                || (pGameObject->AsSWSObject()->GetArea() != 0
                    && pPlayerObject->GetArea() == 0
                    && pPlayerObject->AsSWSCreature() != 0
                    && static_cast<CSWSArea*>(pGameObject->AsSWSObject()->GetArea())->m_objectId
                           == pPlayerObject->AsSWSCreature()->m_currentAreaId)))
        {
            pPlayer->m_pLastUpdateObjectList->GetNext(pPos);
            continue;
        }

        WriteCHAR('D', 8);
        WriteBYTE(pLast->m_objectType, 8);
        WriteOBJECTIDServer(pLast->m_objectId);

        if (pLast->m_objectType == CGameObject::OBJECT_TYPE_CREATURE
            || pLast->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            WriteBOOL(1);
            WriteBOOL(pLast->m_bDeleteNoFade);
            WriteDWORD(pLast->m_nFadeOutTime, 32);
            if (pLast->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                WriteBOOL(pLast->m_bKeepCorpse);
        }
        else if (pLast->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
        {
            WriteBOOL(1);
        }
        else if (pLast->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        {
            WriteBYTE(static_cast<unsigned char>(pLast->m_nFadeOutTime), 8);
        }

        CExoLinkedListNode* pDead = pPos;
        pPlayer->m_pLastUpdateObjectList->GetNext(pPos);
        pPlayer->m_pLastUpdateObjectList->Remove(pDead);
        delete pLast;
    }
}

// The single-object twin of the sweep above, keyed on one object id; the loop returns once it
// has removed the record it was looking for.
void CSWSMessage::DeleteLastUpdateObjectsForObject(CSWSPlayer* pPlayer, unsigned long long nObjectId)
{
    CSWSObject* pPlayerObject = pPlayer->GetGameObject();

    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray == 0)
        return;

    CExoLinkedListNode* pPos = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
    while (pPos != 0)
    {
        CLastUpdateObject* pLast = static_cast<CLastUpdateObject*>(
            pPlayer->m_pLastUpdateObjectList->m_pInternal->GetAtPos(pPos));

        if (pLast->m_objectId != nObjectId || pPlayerObject->m_objectId == nObjectId)
        {
            pLast->m_bTouchedThisFrame = 0;
            pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
            continue;
        }

        WriteCHAR('D', 8);
        WriteBYTE(pLast->m_objectType, 8);
        WriteOBJECTIDServer(pLast->m_objectId);

        if (pLast->m_objectType == CGameObject::OBJECT_TYPE_ITEM
            || pLast->m_objectType == CGameObject::OBJECT_TYPE_CREATURE
            || pLast->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            int bGone = 1;
            CGameObject* pGameObject;
            if (pArray->GetGameObject(pLast->m_objectId, &pGameObject) == CGameObjectArray::SUCCESS
                && pGameObject->AsSWSObject() != 0)
            {
                bGone = pGameObject->AsSWSObject()->GetArea() != pPlayer->GetGameObject()->GetArea();
            }
            WriteBOOL(bGone);

            if (pLast->m_objectType == CGameObject::OBJECT_TYPE_CREATURE
                || pLast->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            {
                WriteBOOL(pLast->m_bDeleteNoFade);
                WriteDWORD(pLast->m_nFadeOutTime, 32);
                if (pLast->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                    WriteBOOL(pLast->m_bKeepCorpse);
            }
        }
        else if (pLast->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        {
            WriteBYTE(3, 8);
        }

        CExoLinkedListNode* pDead = pPos;
        pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
        pPlayer->m_pLastUpdateObjectList->m_pInternal->Remove(pDead);
        delete pLast;
        return;
    }
}

// Finds or creates one party member's snapshot record and returns it with a bitmask naming every
// group whose live value has drifted. A record that has never been seen is allocated and reported
// with the seed mask the writer understands. The search walks the party list but advances the
// object list's cursor; do not correct it.
void CSWSMessage::TestPartyObjectUpdateDifferences(CSWSPlayer* pPlayer, CSWSCreature* pCreature,
                                                   CLastUpdatePartyObject** ppLast,
                                                   unsigned int* pnMask)
{
    CSWSCreature* pViewerCreature = pPlayer->GetGameObject()->AsSWSCreature();

    CExoLinkedListNode* pPos = pPlayer->m_pLastUpdatePartyObjectList->m_pInternal->m_pHead;
    *pnMask = 0;
    while (pPos != 0)
    {
        *ppLast = static_cast<CLastUpdatePartyObject*>(
            pPlayer->m_pLastUpdatePartyObjectList->m_pInternal->GetAtPos(pPos));
        if ((*ppLast)->m_objectId == pCreature->m_objectId)
        {
            (*ppLast)->m_bTouchedThisFrame = 1;
            *pnMask = 0;

            pCreature->ComputeAIState();

            unsigned char nReputation = 2;
            if (pViewerCreature != 0)
                nReputation = pViewerCreature->GetAIStateReputation(pCreature->m_objectId);

            if ((*ppLast)->m_nAIState != pCreature->m_nAIState
                || (*ppLast)->m_nAIStateAction != pCreature->m_nAIStateAction
                || (*ppLast)->m_nActivityFlags != static_cast<unsigned short>(pCreature->m_activityFlags)
                || (*ppLast)->m_oidAIStateTarget != pCreature->m_oidAIStateTarget
                || (*ppLast)->m_nAIStateReputation != nReputation
                || (*ppLast)->m_bCombatState != pCreature->m_bCombatState
                || (*ppLast)->m_nStealthMode != pCreature->m_stealthMode)
                *pnMask |= MSG_OBJECT_UPDATE_AI_STATE;

            // An area change raises the area-id bit and the position bit together.
            if (pCreature->m_cachedAreaId != (*ppLast)->m_oidArea)
                *pnMask |= MSG_OBJECT_UPDATE_COARSE_POSITION | MSG_OBJECT_UPDATE_AREA;

#ifdef K2_FIX_ENGINE_BUGS
            if (pViewerCreature != 0 && pCreature->m_cachedAreaId == pViewerCreature->m_cachedAreaId)
#else
            if (pCreature->m_cachedAreaId == pViewerCreature->m_cachedAreaId)
#endif
            {
                float dx = (*ppLast)->m_fPositionX - pCreature->m_position.x;
                float dy = (*ppLast)->m_fPositionY - pCreature->m_position.y;
                float dz = (*ppLast)->m_fPositionZ - pCreature->m_position.z;
                if (dx * dx + dy * dy + dz * dz >= 4.0f)
                    *pnMask |= MSG_OBJECT_UPDATE_COARSE_POSITION;
            }

            if ((*ppLast)->m_nCurrentHitPoints != pCreature->GetCurrentHitPoints(1)
                || (*ppLast)->m_nHitPoints != static_cast<short>(pCreature->m_hitPoints)
                || (*ppLast)->m_nHitPointBonus != static_cast<short>(pCreature->m_hitPointBonus)
                || (*ppLast)->m_nMaxHitPoints != pCreature->GetMaxHitPoints(1))
                *pnMask |= MSG_OBJECT_UPDATE_HIT_POINTS;

            if ((*ppLast)->m_nCurrentForcePoints != static_cast<short>(pCreature->m_pStats->m_nCurrentForcePoints)
                || (*ppLast)->m_nBaseForcePoints != static_cast<short>(pCreature->m_pStats->m_nBaseForcePoints)
                || (*ppLast)->m_nTemporaryForcePoints != static_cast<short>(pCreature->m_pStats->m_nTemporaryForcePoints)
                || (*ppLast)->m_nMaxForcePoints != pCreature->GetMaxForcePoints())
                *pnMask |= MSG_OBJECT_UPDATE_FORCE_POINTS;

            if (pCreature->AsSWSCreature()->m_bPlayerCharacter != (*ppLast)->m_bPlayerCharacter
                || ((pCreature->AsSWSCreature()->m_nAIState >> 6) & 1) != (*ppLast)->m_bAIStateControllable
                || (pCreature->AsSWSCreature()->GetFaction()->GetLeader()
                        == pCreature->m_objectId) != (*ppLast)->m_bIsPartyLeader
                || pCreature->GetFaction()->GetSingletonParty() != (*ppLast)->m_nSingletonParty
                || pCreature->AsSWSCreature()->m_bInvitedToParty != (*ppLast)->m_bInvitedToParty
                || pCreature->m_bPoisoned != (*ppLast)->m_bPoisoned
                || pCreature->m_bDiseased != (*ppLast)->m_bDiseased)
                *pnMask |= MSG_OBJECT_UPDATE_PARTY;

            if ((*ppLast)->m_nPortraitId != pCreature->GetPortraitId()
                || (*ppLast)->m_portrait != pCreature->GetPortrait())
                *pnMask |= MSG_OBJECT_UPDATE_PORTRAIT;
            return;
        }
        // The bug is that the search advances the object list's cursor, not the party list's.
        pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
    }

    // Never seen before, so allocate the record, seed it, and push it on the party list.
    CLastUpdatePartyObject* pNew = new CLastUpdatePartyObject;
    pNew->m_fPositionX = 0.0f;
    pNew->m_fPositionY = 0.0f;
    pNew->m_fPositionZ = 0.0f;
    *ppLast = pNew;
    pNew->m_bTouchedThisFrame = 1;
    pNew->m_objectId = pCreature->m_objectId;
    *pnMask = MSG_OBJECT_UPDATE_PORTRAIT | MSG_OBJECT_UPDATE_AI_STATE |
              MSG_OBJECT_UPDATE_COARSE_POSITION | MSG_OBJECT_UPDATE_HIT_POINTS |
              MSG_OBJECT_UPDATE_AREA_CHANGED | MSG_OBJECT_UPDATE_PARTY | MSG_OBJECT_UPDATE_NAME |
              MSG_OBJECT_UPDATE_AREA;
    pPlayer->m_pLastUpdatePartyObjectList->m_pInternal->AddHead(*ppLast);
}

// Re-stamps one party member's snapshot with the values that just changed: seven independent
// groups, one mask bit each.
void CSWSMessage::StoreValuesInLastUpdatePartyObject(CSWSCreature* pViewerCreature,
                                                     CLastUpdatePartyObject* pLast,
                                                     CSWSCreature* pCreature,
                                                     unsigned int nMask)
{
    if (nMask & MSG_OBJECT_UPDATE_AI_STATE)
    {
        pLast->m_nAIState = pCreature->m_nAIState;
        pLast->m_nAIStateAction = pCreature->m_nAIStateAction;
        pLast->m_nActivityFlags = static_cast<unsigned short>(pCreature->m_activityFlags);
        pLast->m_oidAIStateTarget = pCreature->m_oidAIStateTarget;
        pLast->m_nAIStateReputation =
            pViewerCreature->GetAIStateReputation(pCreature->m_objectId);
        pLast->m_bCombatState = pCreature->m_bCombatState;
        pLast->m_nStealthMode = pCreature->m_stealthMode;
    }

    if (nMask & MSG_OBJECT_UPDATE_AREA)
        pLast->m_oidArea = pCreature->m_cachedAreaId;

    if (nMask & MSG_OBJECT_UPDATE_COARSE_POSITION)
    {
        pLast->m_fPositionX = pCreature->m_position.x;
        pLast->m_fPositionY = pCreature->m_position.y;
    }

    if (nMask & MSG_OBJECT_UPDATE_HIT_POINTS)
    {
        pLast->m_nHitPoints = static_cast<unsigned short>(pCreature->m_hitPoints);
        pLast->m_nCurrentHitPoints = pCreature->GetCurrentHitPoints(1);
        pLast->m_nHitPointBonus = static_cast<unsigned short>(pCreature->m_hitPointBonus);
        pLast->m_nMaxHitPoints = pCreature->GetMaxHitPoints(1);
    }

    if (nMask & MSG_OBJECT_UPDATE_FORCE_POINTS)
    {
        pLast->m_nBaseForcePoints = pCreature->m_pStats->m_nBaseForcePoints;
        pLast->m_nCurrentForcePoints = pCreature->m_pStats->m_nCurrentForcePoints;
        pLast->m_nTemporaryForcePoints = pCreature->m_pStats->m_nTemporaryForcePoints;
        pLast->m_nMaxForcePoints = pCreature->GetMaxForcePoints();
    }

    if (nMask & MSG_OBJECT_UPDATE_PARTY)
    {
        pLast->m_bPlayerCharacter = pCreature->m_bPlayerCharacter;
        pLast->m_bAIStateControllable = (pCreature->m_nAIState >> 6) & 1;
        pLast->m_nSingletonParty = pCreature->GetFaction()->GetSingletonParty();
        pLast->m_bIsPartyLeader =
            (pCreature->GetFaction()->GetLeader() == pCreature->m_objectId);
        pLast->m_bInvitedToParty = pCreature->m_bInvitedToParty;
        pLast->m_bPoisoned = pCreature->m_bPoisoned;
        pLast->m_bDiseased = pCreature->m_bDiseased;
    }

    if (nMask & MSG_OBJECT_UPDATE_PORTRAIT)
    {
        pLast->m_nPortraitId = pCreature->GetPortraitId();
        pLast->m_portrait = pCreature->GetPortrait();
    }
}

// Runs the frame's party pass. Diffs the viewer's faction members against their party snapshots and
// ships the changed fields, then marks and sweeps the snapshot list - a record not touched this
// frame is removed, a touched one is reset and kept.
int CSWSMessage::WriteGameObjUpdate_PartyAIState(CSWSPlayer* pPlayer)
{
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CSWSObject* pPlayerObject = pPlayer->GetGameObject();
    CSWSCreature* pPlayerCreature = pPlayerObject->AsSWSCreature();
    CSWSFaction* pFaction = pPlayerCreature->GetFaction();

    if (pFaction != 0)
    {
        unsigned long long* paMembers;
        unsigned int nMembers = pFaction->GetMemberList(&paMembers);
        if (nMembers != 0)
        {
            for (unsigned int i = 0; i < nMembers; i++)
            {
                if (paMembers[i] == pPlayerObject->m_objectId)
                    continue;

                CGameObject* pGameObject;
                if (CGameObjectArray::SUCCESS !=
                    pArray->GetGameObject(paMembers[i], &pGameObject))
                    continue;

                CSWSCreature* pMember = pGameObject->AsSWSCreature();
                if (pMember->m_bSkipPartyAIStateUpdate == 1)
                    continue;

                CLastUpdatePartyObject* pLast = 0;
                unsigned int nOutMask;
                TestPartyObjectUpdateDifferences(pPlayer, pMember, &pLast, &nOutMask);
                unsigned int nMask = nOutMask;
                if (nMask != 0)
                    WriteGameObjUpdate_UpdateObject(pPlayer, pMember, 0, nMask, 0);
                StoreValuesInLastUpdatePartyObject(pPlayerCreature, pLast, pMember, nMask);
            }
        }
    }

    CExoLinkedListNode* pPos = pPlayer->m_pLastUpdatePartyObjectList->m_pInternal->m_pHead;
    while (pPos != 0)
    {
        CLastUpdatePartyObject* pLast = static_cast<CLastUpdatePartyObject*>(
            pPlayer->m_pLastUpdatePartyObjectList->GetAtPos(pPos));

        if (pLast->m_bTouchedThisFrame != 0)
        {
            pLast->m_bTouchedThisFrame = 0;
            pPlayer->m_pLastUpdatePartyObjectList->GetNext(pPos);
            continue;
        }

        CExoLinkedListNode* pDead = pPos;
        pPlayer->m_pLastUpdatePartyObjectList->GetNext(pPos);
        pPlayer->m_pLastUpdatePartyObjectList->Remove(pDead);
        // A bare operator delete with no destructor call, since CLastUpdatePartyObject is trivially
        // destructible.
        delete pLast;
    }

    return 1;
}

// The write twin of ComputeInventoryUpdateRequired makes the same subject selection, then the live
// values are stamped into the snapshot instead of diffed.
void CSWSMessage::UpdateLastUpdateInventory(CSWSPlayer* pPlayer, unsigned int nSlot)
{
    if (pPlayer == 0)
        return;

    if (pPlayer->GetGameObject() == 0)
        return;

    CGameObject* pObject;
#if KOTOR1  // K1: always the player's own creature
    pObject = pPlayer->GetGameObject();
#else
    int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
    if (nOverride != -1)
    {
        unsigned long long nSubjectId =
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nOverride, 0, 1);
        pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nSubjectId);
    }
    else
    {
        pObject = pPlayer->GetGameObject();
    }
#endif

    if (pObject->AsSWSCreature() == 0)
        return;

    if (pObject->AsSWSCreature()->m_pInventory == 0)
        return;

    CSWSItem* pItem =
        static_cast<CSWSItem*>(pObject->AsSWSCreature()->m_pInventory->GetItemInSlot(1 << nSlot));

    if (pItem != 0)
    {
        pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[nSlot] = pItem->m_objectId;
        pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemStackCount[nSlot] = pItem->m_stackCount;
    }
    else
    {
        pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemId[nSlot] =
            CGameObjectArray::INVALIDOBJID;
        pPlayer->m_pInventoryGUI->m_pInventory->m_slotItemStackCount[nSlot] = -1;
    }
}

// One container's contents, diffed against the client's snapshot of that container and shipped
// as add, update and delete frames. Both lists are walked backwards in lockstep: equal ids
// compare stack size and cost, a snapshot entry with no live counterpart is deleted, an
// unconsumed repository slot is an add, and leftover snapshot entries are deleted. The first
// parameter is never read.
void CSWSMessage::WriteRepositoryUpdate(CSWSPlayer* pPlayer, CSWSObject* pObject,
                                        CItemRepository* pRepository,
                                        CSWSPlayerLUOInventory* pInventory,
                                        unsigned char nList, char nOpcode)
{
    K2_UNUSED(pPlayer);
    CExoLinkedListNode* pNode = 0;
    // ItemListGetCount is unsigned, so the loop keeps an index test of its own.
    int nItem = pRepository->ItemListGetCount() - 1;
    pInventory->ItemListSetEnd(pNode, nList);

    CSWSPlayerLUOInventoryItem* pUpdateItem = 0;

    while (nItem >= 0 || pNode != 0)
    {
        unsigned long long objectId K2_UNSET;
        CSWSItem* pItem = 0;

        if (nItem >= 0)
        {
            objectId = pRepository->ItemListGetItemObjectID(nItem);
            pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
        }
        if (pNode != 0)
        {
            pUpdateItem = pInventory->ItemListGetUpdateItem(pNode, nList);
        }

        if (nItem < 0)
        {
            // The client still believes in items the repository no longer has.
            WriteCHAR('G', 8);
            WriteCHAR(nOpcode, 8);
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(pUpdateItem->m_objectId);

            CExoLinkedListNode* pDead = pNode;
            pInventory->ItemListGetPrev(pNode, nList);
            pInventory->ItemListRemove(pDead, nList);
        }
        else if (pNode == 0)
        {
            // The snapshot ran out first, so everything left in the repository is new to the client.
            WriteCHAR('G', 8);
            WriteCHAR(nOpcode, 8);
            WriteCHAR('A', 8);
            WriteOBJECTIDServer(objectId);

            // Only an item-typed object carries the appearance and property blocks.
            K2_ASSUME(pItem != 0);
            if (pItem->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
            {
                AddItemAppearanceToMessage(pItem);
                AddActiveItemPropertiesToMessage(pItem, pObject->AsSWSCreature());
            }

            pInventory->ItemListAddHead(objectId, nList, pItem->m_stackCount, pItem->GetCost());
            nItem--;
        }
        else if (objectId != pUpdateItem->m_objectId)
        {
            // The two lists have diverged, so drop the snapshot entry and retry this repository
            // slot against the next one.
            WriteCHAR('G', 8);
            WriteCHAR(nOpcode, 8);
            WriteCHAR('D', 8);
            WriteOBJECTIDServer(pUpdateItem->m_objectId);

            CExoLinkedListNode* pDead = pNode;
            pInventory->ItemListGetPrev(pNode, nList);
            pInventory->ItemListRemove(pDead, nList);
        }
        else
        {
            // Same item on both sides, so an update goes out only if the client's numbers are stale.
            K2_ASSUME(pItem != 0);
            if (static_cast<unsigned int>(pItem->m_stackCount) != pUpdateItem->m_nStackCount
                || pItem->GetCost() != pUpdateItem->m_nCost)
            {
                WriteCHAR('G', 8);
                WriteCHAR(nOpcode, 8);
                WriteCHAR('U', 8);
                WriteOBJECTIDServer(pItem->m_objectId);
                WriteDWORD(pItem->m_stackCount, 32);
                WriteDWORD(pItem->GetCost(), 32);

                pUpdateItem->m_nStackCount = pItem->m_stackCount;
                pUpdateItem->m_nCost = pItem->GetCost();
            }

            nItem--;
            pInventory->ItemListGetPrev(pNode, nList);
        }
    }
}

// Diffs the creature's live spell-like-ability list against the snapshot's copy and returns a
// count: one for a length mismatch plus one per differing element. Every consumer only tests it
// against zero.
int CSWSMessage::ComputeLastUpdate_CompareSpellLikeAbility(CSWSCreature* pCreature,
                                                           CSWSPlayerLastUpdateObject* pLast)
{
    CExoArrayList<CSWSStats_SpellLikeAbility>* pLive =
        pCreature->m_pStats->m_pSpellLikeAbilityList;
    CExoArrayList<CSWSStats_SpellLikeAbility>* pSnapshot = pLast->m_pSpellLikeAbilityList;

    // The snapshot container is allocated lazily, so it can be null here; the live one is dereferenced
    // unchecked.
    int nLiveCount = pLive->m_count;

    int nSnapshotCount = 0;
    if (pSnapshot != 0)
        nSnapshotCount = pSnapshot->m_count;

    int nDiff = (nSnapshotCount != nLiveCount) ? 1 : 0;

    for (int i = 0; i < nLiveCount; i++)
    {
        if (i >= nSnapshotCount
            || pLive->m_pArray[i].m_nReady != pSnapshot->m_pArray[i].m_nReady
            || pLive->m_pArray[i].m_nCasterLevel != pSnapshot->m_pArray[i].m_nCasterLevel
            || pLive->m_pArray[i].m_nSpellId != pSnapshot->m_pArray[i].m_nSpellId)
        {
            nDiff++;
        }
    }

    return nDiff;
}

// The wire twin of ComputeLastUpdate_CompareSpellLikeAbility. Ships the spell-like-ability group's
// changes - a count-changed header when the lengths differ, and one record per entry that moved.
void CSWSMessage::ComputeLastUpdate_WriteSpellLikeAbility(CSWSCreature* pCreature,
                                                          CSWSPlayerLastUpdateObject* pLast)
{
    WriteWORD(static_cast<unsigned short>(ComputeLastUpdate_CompareSpellLikeAbility(pCreature, pLast)), 16);

    CExoArrayList<CSWSStats_SpellLikeAbility>* pLive =
        pCreature->m_pStats->m_pSpellLikeAbilityList;
    CExoArrayList<CSWSStats_SpellLikeAbility>* pSnapshot = pLast->m_pSpellLikeAbilityList;

    int nLiveCount = pLive->m_count;

    int nSnapshotCount = 0;
    if (pSnapshot != 0)
        nSnapshotCount = pSnapshot->m_count;

    if (nSnapshotCount != nLiveCount)
    {
        WriteBYTE('S', 8);
        WriteWORD(static_cast<unsigned short>(nLiveCount), 16);
    }

    for (int i = 0; i < nLiveCount; i++)
    {
        if (i >= nSnapshotCount
            || pLive->m_pArray[i].m_nReady != pSnapshot->m_pArray[i].m_nReady
            || pLive->m_pArray[i].m_nCasterLevel != pSnapshot->m_pArray[i].m_nCasterLevel
            || pLive->m_pArray[i].m_nSpellId != pSnapshot->m_pArray[i].m_nSpellId)
        {
            WriteBYTE('U', 8);
            WriteWORD(static_cast<unsigned short>(i), 16);
            WriteBOOL(pLive->m_pArray[i].m_nReady);
            WriteBYTE(pLive->m_pArray[i].m_nCasterLevel, 8);
            WriteWORD(static_cast<unsigned short>(pLive->m_pArray[i].m_nSpellId), 16);
        }
    }
}

// The panel differ compares every character-sheet value the client is currently showing against the
// live one and raises a bit in nMask for each group that moved. WriteGameObjUpdate_PlayerUpdate
// then ships exactly those groups.
void CSWSMessage::TestPlayerUpdateDifferences(CSWSPlayer* pPlayer, CSWSPlayerLastUpdateObject* pLast,
                                              CLastUpdateObject* pLastObject, unsigned int& nMask)
{
    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return;

    // The groups that are each their own recomputed word or list diff.
    if (pLast->m_nGuiSkills != ComputeLastUpdate_GuiSkills(pCreature))
        nMask |= MSG_PLAYER_UPDATE_GUI_SKILLS;

    if (pLast->m_nPlayerState != static_cast<unsigned short>(ComputeLastUpdate_PlayerState(pCreature)))
        nMask |= MSG_PLAYER_UPDATE_STATUS;

    if (ComputeLastUpdate_GuiKnownSpells(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_KNOWN_SPELLS;

    if (ComputeLastUpdate_GuiKnownSpellUses(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_KNOWN_SPELL_USES;

    if (ComputeLastUpdate_ActionQueue(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_ACTION_QUEUE;

    if (ComputeVisibilityLists(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_VISIBILITY;

    if (ComputeLastUpdate_GuiFeats(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_FEATS;

    // The loose character-sheet scalars, all in one bit. m_bForcePlayerUpdate is tested against 1 rather than for
    // nonzero, and it raises the bit when it matches where every other term here raises it on a
    // difference.
    if (pCreature->GetGold() != pLast->m_nGold
#if !KOTOR1  // K1: no components or chemicals
        || pCreature->GetComponent() != pLast->m_nComponent
        || pCreature->GetChemical() != pLast->m_nChemical
#endif
        || pLast->m_nArmorClass != static_cast<unsigned short>(pCreature->GetArmorClass())
        || pLast->m_nEncumbranceState != pCreature->m_nEncumbranceState
        || pCreature->m_bForcePlayerUpdate == 1
        || pLast->m_nExperience != pCreature->m_pStats->m_nExperience
        || pLast->m_bCanLevelUp != pCreature->m_pStats->CanLevelUp()
        || pLast->m_nLevel != pCreature->m_pStats->GetLevel(0))
    {
        nMask |= MSG_PLAYER_UPDATE_CHARACTER;
    }

    // Everything below lives on the per-object snapshot and needs the stats block.
    if (pLastObject != 0 && pCreature->m_pStats != 0)
    {
        // The six base ability scores, at stride 1 here and stride 2 on the stats side.
        if (pCreature->m_pStats->m_nSTRBase != pLastObject->m_nAbilityBase[0]
            || pCreature->m_pStats->m_nDEXBase != pLastObject->m_nAbilityBase[1]
            || pCreature->m_pStats->m_nCONBase != pLastObject->m_nAbilityBase[2]
            || pCreature->m_pStats->m_nINTBase != pLastObject->m_nAbilityBase[3]
            || pCreature->m_pStats->m_nWISBase != pLastObject->m_nAbilityBase[4]
            || pCreature->m_pStats->m_nCHABase != pLastObject->m_nAbilityBase[5])
        {
            nMask |= MSG_PLAYER_UPDATE_ABILITIES;
        }

        // The six effective ability scores share the same bit, but use accessor calls rather than
        // field loads.
        if (pCreature->m_pStats->GetSTRStat() != pLastObject->m_nAbilityStat[0]
            || pCreature->m_pStats->GetDEXStat() != pLastObject->m_nAbilityStat[1]
            || pCreature->m_pStats->GetCONStat() != pLastObject->m_nAbilityStat[2]
            || pCreature->m_pStats->GetINTStat() != pLastObject->m_nAbilityStat[3]
            || pCreature->m_pStats->GetWISStat() != pLastObject->m_nAbilityStat[4]
            || pCreature->m_pStats->GetCHAStat() != pLastObject->m_nAbilityStat[5])
        {
            nMask |= MSG_PLAYER_UPDATE_ABILITIES;
        }

        // Both readings of every skill raise the same bit, and neither breaks out of the loop.
        for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
        {
            // GetSkillRank returns a signed byte; this differ treats a rank as unsigned, so it zero-extends.
            if (static_cast<unsigned char>(pCreature->m_pStats->GetSkillRank(i, 0, 1))
                != pLastObject->m_nSkillRank[i])
                nMask |= MSG_PLAYER_UPDATE_SKILL_RANKS;

            if (static_cast<unsigned char>(pCreature->m_pStats->GetSkillRank(i, 0, 0))
                != pLastObject->m_nSkillRankBase[i])
                nMask |= MSG_PLAYER_UPDATE_SKILL_RANKS;
        }

        // GetPortrait returns by value, so the compare runs against a temporary.
        if (pCreature->GetPortrait() != pLastObject->m_portrait
            || pCreature->GetPortraitId() != pLastObject->m_nPortraitId)
        {
            nMask |= MSG_PLAYER_UPDATE_PORTRAIT;
        }

        // The hit-point terms are narrowed to 16 bits to match the snapshot, which is 16-bit unsigned.
        if (pLastObject->m_nHitPoints != static_cast<unsigned short>(pCreature->m_hitPoints)
            || pLastObject->m_nCurrentHitPoints != static_cast<unsigned short>(pCreature->GetCurrentHitPoints(0))
            || pLastObject->m_nHitPointBonus != static_cast<unsigned short>(pCreature->m_hitPointBonus)
            || pLastObject->m_nMaxHitPoints != static_cast<unsigned short>(pCreature->GetMaxHitPoints(1)))
        {
            nMask |= MSG_PLAYER_UPDATE_HIT_POINTS;
        }

        // The force-point sum is computed in 16 bits, so it wraps rather than promoting.
        if (pLastObject->m_nBaseForcePoints != pCreature->m_pStats->m_nBaseForcePoints
            || pLastObject->m_nForcePoints != static_cast<unsigned short>(pCreature->m_pStats->m_nCurrentForcePoints
                                                               + pCreature->m_pStats->m_nTemporaryForcePoints)
            || pLastObject->m_nTemporaryForcePoints != pCreature->m_pStats->m_nTemporaryForcePoints
            || pLastObject->m_nMaxForcePoints != static_cast<unsigned short>(pCreature->GetMaxForcePoints()))
        {
            nMask |= MSG_PLAYER_UPDATE_FORCE_POINTS;
        }
    }

    if (ComputeLastUpdate_CompareSpellLikeAbility(pCreature, pLast) != 0)
        nMask |= MSG_PLAYER_UPDATE_SPELL_LIKE_ABILITIES;
}

// The ship half of the player-panel trio, a flat `if (nMask & BIT)` per group, in a body order that
// matches neither the mask nor the differ. Two of the groups are write-only: one writes four zero
// words and reads nothing, the other counts a value array it never fills.
void CSWSMessage::WriteGameObjUpdate_PlayerUpdate(CSWSPlayer* pPlayer,
                                                  CSWSPlayerLastUpdateObject* pLast,
                                                  CLastUpdateObject* pLastObject,
                                                  unsigned int nMask)
{
    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return;

    WriteCHAR('I', 8);
    WriteOBJECTIDServer(pCreature->m_objectId);
    WriteDWORD(nMask, 32);

    // The loose character-sheet scalars, then the level-up preview while the creature can level up.
    if (nMask & MSG_PLAYER_UPDATE_CHARACTER)
    {
        WriteSHORT(pCreature->GetArmorClass(), 16);
        WriteDWORD(pCreature->GetGold(), 32);
#if !KOTOR1  // K1: no components or chemicals
        WriteDWORD(pCreature->GetComponent(), 32);
        WriteDWORD(pCreature->GetChemical(), 32);
#endif
        WriteINT(pCreature->m_nEncumbranceState, 32);
        WriteBOOL(pCreature->m_pStats->CanLevelUp());
        WriteDWORD(pCreature->m_pStats->m_nExperience, 32);

        // CanLevelUp is re-issued rather than reused: it is opaque, so the value shipped above cannot be
        // carried across it. The gate is `== 1`, not a truth test.
        if (pCreature->m_pStats->CanLevelUp() == 1)
        {
            WriteWORD(pCreature->m_pStats->m_nSkillPoints, 16);

            for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
                WriteCHAR(pCreature->m_pStats->GetSkillRank(i, 0, 1), 8);

            WriteBYTE(pCreature->m_pStats->m_nStartingPackage, 8);
            WriteBYTE(pCreature->m_pStats->m_numClasses - 1, 8);
        }
    }

    if (nMask & MSG_PLAYER_UPDATE_GUI_SKILLS)
        WriteWORD(static_cast<unsigned short>(ComputeLastUpdate_GuiSkills(pCreature)), 16);

    if (nMask & MSG_PLAYER_UPDATE_STATUS)
        WriteWORD(static_cast<unsigned short>(ComputeLastUpdate_PlayerState(pCreature)), 16);

    // This group is write-only and vestigial. nValues is never assigned between its zero-init and
    // the scan, so nCount is always 0 and the second pass never runs.
    if (nMask & MSG_PLAYER_UPDATE_MEMORIZED_SPELL_COUNTS)
    {
        unsigned char bChanged[2] = { 0, 0 };
        unsigned short nValues[2] = { 0, 0 };

        unsigned char nCount = 0;
        for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++)
        {
            if (nValues[i] != 0)
            {
                bChanged[i] = 1;
                nCount++;
            }
        }

        WriteBYTE(nCount, 8);

        for (unsigned char i = 0; i < pCreature->m_pStats->m_numClasses; i++)
        {
            if (bChanged[i])
            {
                WriteBYTE(i, 8);
                WriteWORD(nValues[i], 16);
            }
        }
    }

    // The known-spell delta the snapshot has been accumulating, per class the spells to drop then
    // the spells to add. Both lists live on the snapshot, not on the creature.
    if (nMask & MSG_PLAYER_UPDATE_KNOWN_SPELLS)
    {
        for (unsigned char nClass = 0; nClass != 2; nClass++)
        {
            WriteWORD(static_cast<unsigned short>(pLast->m_spellDeleteList[nClass][0].m_count), 16);
            for (unsigned char i = 0; i < pLast->m_spellDeleteList[nClass][0].m_count; i++)
                WriteDWORD(pLast->m_spellDeleteList[nClass][0].m_pArray[i], 32);

            WriteWORD(static_cast<unsigned short>(pLast->m_spellAddList[nClass][0].m_count), 16);
            for (unsigned char i = 0; i < pLast->m_spellAddList[nClass][0].m_count; i++)
                WriteDWORD(pLast->m_spellAddList[nClass][0].m_pArray[i], 32);
        }
    }

    // This group is write-only and constant. It reads nothing and ships four literal zero words.
    if (nMask & MSG_PLAYER_UPDATE_MEMORIZED_SPELLS)
    {
        for (unsigned char nClass = 0; nClass != 2; nClass++)
        {
            WriteWORD(0, 16);
            WriteWORD(0, 16);
        }
    }

    // Per-class remaining force-power uses, in three passes (decide, ship, then write the live
    // value back).
    if (nMask & MSG_PLAYER_UPDATE_KNOWN_SPELL_USES)
    {
        unsigned char bChanged[2] = { 0, 0 };
        unsigned short nFields[2] = { 0, 0 };

        unsigned char nCount = 0;
        for (int nClass = 0; nClass < pCreature->m_pStats->m_numClasses; nClass++)
        {
            if (CSWClass::IsJedi(pCreature->m_pStats->GetClass(static_cast<unsigned char>(nClass))))
            {
                if (pLast->GetKnownSpellUsesLeft(static_cast<unsigned char>(nClass), 0)
                    != pCreature->m_pStats->GetSpellsPerDayLeft(static_cast<unsigned char>(nClass), 0))
                    nFields[nClass] |= kClassFieldSpellUsesLeft;

                if (nFields[nClass] != 0)
                {
                    bChanged[nClass] = 1;
                    nCount++;
                }
            }
        }

        WriteBYTE(nCount, 8);

        for (unsigned char nClass = 0; nClass < pCreature->m_pStats->m_numClasses; nClass++)
        {
            if (bChanged[nClass])
            {
                WriteBYTE(nClass, 8);
                WriteWORD(nFields[nClass], 16);

                if (nFields[nClass] & kClassFieldSpellUsesLeft)
                    WriteBYTE(pCreature->m_pStats->GetSpellsPerDayLeft(nClass, 0), 8);
            }
        }

        // The snapshot write-back lives here, which is why StoreValuesInLastPlayerUpdateObject has no arm
        // for this group.
        for (int nClass = 0; nClass < pCreature->m_pStats->m_numClasses; nClass++)
        {
            if (CSWClass::IsJedi(pCreature->m_pStats->GetClass(static_cast<unsigned char>(nClass))))
                pLast->SetKnownSpellUsesLeft(static_cast<unsigned char>(nClass), 0,
                                             pCreature->m_pStats->GetSpellsPerDayLeft(static_cast<unsigned char>(nClass), 0));
        }
    }

    // The feat delta, built as two parallel lists - an opcode stream and an id stream - by four
    // set-difference passes, then shipped as opcode/id pairs followed by the remaining-uses table.
    // The opcodes are 'A' add feat, 'D' drop feat, 'B' add bonus feat, 'X' drop bonus feat.
    if (nMask & MSG_PLAYER_UPDATE_FEATS)
    {
        CExoArrayList<char> listOps;
        CExoArrayList<unsigned short> listFeats;

        // None of the four searches breaks early: each runs the whole inner list accumulating a found flag.
        for (unsigned short i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); i++)
        {
            unsigned short nFeat = pCreature->m_pStats->GetFeat(i);

            int bFound = 0;
            for (int j = 0; j < pLast->m_feats.m_count; j++)
                if (pLast->m_feats.m_pArray[j] == nFeat)
                    bFound = 1;

            if (!bFound)
            {
                listOps.Add('A');
                listFeats.Add(nFeat);
            }
        }

        for (int i = 0; i < pLast->m_feats.m_count; i++)
        {
            unsigned short nFeat = pLast->m_feats.m_pArray[i];

            int bFound = 0;
            for (unsigned short j = 0; j < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); j++)
                if (pCreature->m_pStats->GetFeat(j) == nFeat)
                    bFound = 1;

            if (!bFound)
            {
                listOps.Add('D');
                listFeats.Add(nFeat);
            }
        }

        for (unsigned short i = 0;
             i < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); i++)
        {
            unsigned short nFeat = pCreature->m_pStats->GetBonusFeat(i);

            int bFound = 0;
            for (int j = 0; j < pLast->m_bonusFeats.m_count; j++)
                if (pLast->m_bonusFeats.m_pArray[j] == nFeat)
                    bFound = 1;

            if (!bFound)
            {
                listOps.Add('B');
                listFeats.Add(nFeat);
            }
        }

        for (int i = 0; i < pLast->m_bonusFeats.m_count; i++)
        {
            unsigned short nFeat = pLast->m_bonusFeats.m_pArray[i];

            int bFound = 0;
            for (unsigned short j = 0;
                 j < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); j++)
                if (pCreature->m_pStats->GetBonusFeat(j) == nFeat)
                    bFound = 1;

            if (!bFound)
            {
                listOps.Add('X');
                listFeats.Add(nFeat);
            }
        }

        WriteWORD(static_cast<unsigned short>(listOps.m_count), 16);
        for (int i = 0; i < listOps.m_count; i++)
        {
            WriteCHAR(listOps.m_pArray[i], 8);
            WriteWORD(listFeats.m_pArray[i], 16);
        }

        // The remaining-uses table is shipped whole rather than as a delta, and the feat id is re-read from
        // the entry to key the lookup.
        WriteWORD(static_cast<unsigned short>(pCreature->m_pStats->m_featRemainingUses.m_count), 16);
        for (int i = 0; i < pCreature->m_pStats->m_featRemainingUses.m_count; i++)
        {
            WriteWORD(pCreature->m_pStats->m_featRemainingUses.m_pArray[i]->m_nFeat, 16);
            WriteBYTE(pCreature->m_pStats->GetFeatRemainingUses(
                          pCreature->m_pStats->m_featRemainingUses.m_pArray[i]->m_nFeat), 8);
        }
    }

    // The action-queue delta, the same two-parallel-list shape over the ten fixed slots of the queue.
    // These searches do break on the first match, unlike the feat ones above.
    if (nMask & MSG_PLAYER_UPDATE_ACTION_QUEUE)
    {
        CExoArrayList<char> listOps;
        CExoArrayList<unsigned char> listSlots;

        // Slots the client still shows that no live action matches.
        for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
        {
            if (pLast->m_actionQueue[i].m_nGroupId == CSWActionQueueEntry::SLOT_EMPTY)
                continue;

            int bFound = 0;
            for (int j = 0; j < NUM_ACTION_QUEUE_ENTRIES; j++)
            {
                if (pCreature->m_actionQueue[j].m_nActionId == pLast->m_actionQueue[i].m_nActionId
                    && pCreature->m_actionQueue[j].m_nGroupId == pLast->m_actionQueue[i].m_nGroupId
                    && pCreature->m_actionQueue[j].m_nAltActionId == pLast->m_actionQueue[i].m_nAltActionId)
                {
                    bFound = 1;
                    break;
                }
            }

            if (!bFound)
            {
                listOps.Add('D');
                listSlots.Add(static_cast<unsigned char>(i));
            }
        }

        // Live actions the client does not have. An entry whose action id is the empty sentinel but
        // whose group id is not ships as 'S' rather than 'A'.
        for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
        {
            if (pCreature->m_actionQueue[i].m_nGroupId == CSWActionQueueEntry::SLOT_EMPTY)
                continue;

            int bFound = 0;
            for (int j = 0; j < NUM_ACTION_QUEUE_ENTRIES; j++)
            {
                if (pLast->m_actionQueue[j].m_nActionId == pCreature->m_actionQueue[i].m_nActionId
                    && pLast->m_actionQueue[j].m_nGroupId == pCreature->m_actionQueue[i].m_nGroupId
                    && pLast->m_actionQueue[j].m_nAltActionId == pCreature->m_actionQueue[i].m_nAltActionId)
                {
                    bFound = 1;
                    break;
                }
            }

            if (!bFound)
            {
                if (pCreature->m_actionQueue[i].m_nActionId == CSWActionQueueEntry::SLOT_EMPTY
                    && pCreature->m_actionQueue[i].m_nAltActionId != CSWActionQueueEntry::SLOT_EMPTY)
                {
                    listOps.Add('S');
                    listSlots.Add(static_cast<unsigned char>(i));
                }
                else
                {
                    listOps.Add('A');
                    listSlots.Add(static_cast<unsigned char>(i));
                }
            }
        }

        WriteBYTE(static_cast<unsigned char>(listOps.m_count), 8);
        for (int i = 0; i < listOps.m_count; i++)
        {
            unsigned char nSlot = listSlots.m_pArray[i];

            WriteCHAR(listOps.m_pArray[i], 8);

            // The three opcode arms are written D, S, A; the emitted comparison order is the reverse, so
            // do not reorder them.
            if (listOps.m_pArray[i] == 'D')
            {
                if (pLast->m_actionQueue[nSlot].m_nActionId != CSWActionQueueEntry::SLOT_EMPTY)
                {
                    WriteWORD(pLast->m_actionQueue[nSlot].m_nActionId, 16);
                    WriteWORD(pLast->m_actionQueue[nSlot].m_nGroupId, 16);
                }
                else
                {
                    WriteWORD(pLast->m_actionQueue[nSlot].m_nAltActionId, 16);
                    WriteWORD(pLast->m_actionQueue[nSlot].m_nGroupId, 16);
                }
            }
            else if (listOps.m_pArray[i] == 'S')
            {
                WriteWORD(pCreature->m_actionQueue[nSlot].m_nAltActionId, 16);
                WriteWORD(pCreature->m_actionQueue[nSlot].m_nGroupId, 16);
                WriteOBJECTIDServer(pCreature->m_actionQueue[nSlot].m_targetId);
            }
            else if (listOps.m_pArray[i] == 'A')
            {
                WriteWORD(pCreature->m_actionQueue[nSlot].m_nActionId, 16);
                WriteWORD(pCreature->m_actionQueue[nSlot].m_nGroupId, 16);

                if (pCreature->m_actionQueue[nSlot].m_nActionId != AQACTION_MOVE_TO_POINT
                    && pCreature->m_actionQueue[nSlot].m_nActionId != AQACTION_DROP_ITEM)
                    WriteOBJECTIDServer(pCreature->m_actionQueue[nSlot].m_targetId);

                if (pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_MOVE_TO_POINT
                    || pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_DROP_ITEM
                    || pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_CAST_SPELL
                    || pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_SET_MINE
                    || pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_ITEM_CAST_SPELL)
                {
                    WriteFLOAT(pCreature->m_actionQueue[nSlot].m_vTarget.x, 1.0f, 32);
                    WriteFLOAT(pCreature->m_actionQueue[nSlot].m_vTarget.y, 1.0f, 32);
                    WriteFLOAT(pCreature->m_actionQueue[nSlot].m_vTarget.z, 1.0f, 32);
                }

                // Two action types carry a spell id; without the second term the second of them
                // never ships it.
#if KOTOR1
                if (pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_ITEM_CAST_SPELL)
                    WriteDWORD(pCreature->m_actionQueue[nSlot].m_nSpellId, 32);
                else if (pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_CAST_SPELL)
                    WriteDWORD(pCreature->m_actionQueue[nSlot].m_nSpellId, 32);
#else
                if (pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_CAST_SPELL
                    || pCreature->m_actionQueue[nSlot].m_nActionId == AQACTION_ITEM_CAST_SPELL)
                    WriteDWORD(pCreature->m_actionQueue[nSlot].m_nSpellId, 32);
#endif
            }
        }
    }

    // The visibility delta ships who dropped out of sight, then who came into it or changed state.
    if (nMask & MSG_PLAYER_UPDATE_VISIBILITY)
    {
        CExoArrayList<CSWVisibilityNode*> listGone;
        CExoArrayList<CSWVisibilityNode*> listNew;

        // The two searches are not symmetric. Leaving is decided on the object id alone, while
        // arriving also re-ships anyone whose low three state bits moved.
        for (int i = 0; i < pLast->m_visibilityList.m_count; i++)
        {
            CSWVisibilityNode* pNode = pLast->m_visibilityList.m_pArray[i];

            int bFound = 0;
            for (int j = 0; j < pCreature->m_visibilityList.m_count; j++)
            {
                if (pCreature->m_visibilityList.m_pArray[j]->m_objectId == pNode->m_objectId)
                {
                    bFound = 1;
                    break;
                }
            }

            if (!bFound)
                listGone.Add(pNode);
        }

        WriteDWORD(listGone.m_count, 32);
        for (int i = 0; i < listGone.m_count; i++)
            WriteOBJECTIDServer(listGone.m_pArray[i]->m_objectId);

        for (int i = 0; i < pCreature->m_visibilityList.m_count; i++)
        {
            CSWVisibilityNode* pNode = pCreature->m_visibilityList.m_pArray[i];

            int bFound = 0;
            for (int j = 0; j < pLast->m_visibilityList.m_count; j++)
            {
                if (pLast->m_visibilityList.m_pArray[j]->m_objectId == pNode->m_objectId
                    && ((pLast->m_visibilityList.m_pArray[j]->m_flags ^ pNode->m_flags) &
                  (kVisibleSeen | kVisibleHeard | kVisibleInactive)) == 0)
                {
                    bFound = 1;
                    break;
                }
            }

            if (!bFound)
                listNew.Add(pNode);
        }

        WriteDWORD(listNew.m_count, 32);
        for (int i = 0; i < listNew.m_count; i++)
        {
            WriteOBJECTIDServer(listNew.m_pArray[i]->m_objectId);
            WriteBOOL((listNew.m_pArray[i]->m_flags >> 1) & 1);
            WriteBOOL(listNew.m_pArray[i]->m_flags & 1);
            WriteBOOL((listNew.m_pArray[i]->m_flags >> 4) & 1);
        }
    }

    // Both failing arms write their own WriteBOOL(0) rather than branching to a shared one.
    if (nMask & MSG_PLAYER_UPDATE_ABILITIES)
    {
        if (pLastObject == 0)
        {
            WriteBOOL(0);
        }
        else if (pCreature->m_pStats == 0)
        {
            WriteBOOL(0);
        }
        else
        {
            WriteBOOL(1);
            // The six base scores are field reads at stride 2; the six effective ones are accessor calls.
            WriteBYTE(pCreature->m_pStats->m_nSTRBase, 8);
            WriteBYTE(pCreature->m_pStats->m_nDEXBase, 8);
            WriteBYTE(pCreature->m_pStats->m_nCONBase, 8);
            WriteBYTE(pCreature->m_pStats->m_nINTBase, 8);
            WriteBYTE(pCreature->m_pStats->m_nWISBase, 8);
            WriteBYTE(pCreature->m_pStats->m_nCHABase, 8);
            WriteBYTE(pCreature->m_pStats->GetSTRStat(), 8);
            WriteBYTE(pCreature->m_pStats->GetDEXStat(), 8);
            WriteBYTE(pCreature->m_pStats->GetCONStat(), 8);
            WriteBYTE(pCreature->m_pStats->GetINTStat(), 8);
            WriteBYTE(pCreature->m_pStats->GetWISStat(), 8);
            WriteBYTE(pCreature->m_pStats->GetCHAStat(), 8);
            WriteCHAR(pCreature->m_pStats->GetWillSavingThrow(0), 8);
            WriteCHAR(pCreature->m_pStats->GetReflexSavingThrow(0), 8);
            WriteCHAR(pCreature->m_pStats->GetFortSavingThrow(0), 8);
        }
    }

    // The skill-rank dump is gated on three conditions and ships nothing if any fails; there is no else
    // arm, unlike the ability group above.
    if (nMask & MSG_PLAYER_UPDATE_SKILL_RANKS)
    {
        if (pLastObject != 0 && pCreature->m_pStats != 0 && g_pRules->m_nNumSkills != 0)
        {
            for (int i = 0; i < g_pRules->m_nNumSkills; i++)
            {
                WriteCHAR(pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(i), 0, 1), 8);
                WriteCHAR(pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(i), 0, 0), 8);
            }
        }
    }

    // GetPortraitId is issued twice - once for the payload and once for the gate - because it is a
    // virtual call.
    if (nMask & MSG_PLAYER_UPDATE_PORTRAIT)
    {
        WriteWORD(pCreature->GetPortraitId(), 16);

        if (pCreature->GetPortraitId() >= MSG_PORTRAIT_ID_CUSTOM)
            WriteCResRef(CResRef(pCreature->GetPortrait().GetResRefStr()), 16);
    }

    if (nMask & MSG_PLAYER_UPDATE_HIT_POINTS)
    {
        WriteSHORT(static_cast<short>(pCreature->m_hitPoints), 16);
        WriteSHORT(pCreature->GetCurrentHitPoints(0), 16);
        WriteSHORT(static_cast<short>(pCreature->m_hitPointBonus), 16);
        WriteSHORT(pCreature->GetMaxHitPoints(1), 16);
    }

    // The second field is a sum computed in 16 bits, so force points wrap rather than promoting.
    if (nMask & MSG_PLAYER_UPDATE_FORCE_POINTS)
    {
        WriteSHORT(pCreature->m_pStats->m_nBaseForcePoints, 16);
        WriteSHORT(pCreature->m_pStats->m_nTemporaryForcePoints + pCreature->m_pStats->m_nCurrentForcePoints, 16);
        WriteSHORT(pCreature->m_pStats->m_nTemporaryForcePoints, 16);
        WriteSHORT(pCreature->GetMaxForcePoints(), 16);
    }

    if (nMask & MSG_PLAYER_UPDATE_SPELL_LIKE_ABILITIES)
        ComputeLastUpdate_WriteSpellLikeAbility(pCreature, pLast);

    // Outside every mask test, since the add and delete lists the spell-like-ability group ships
    // are consumed by this send either way.
    if (pLast != 0)
        pLast->ClearSpellAddDeleteLists();
}

// The write half of the player-panel trio. For every group the differ flagged, copies the live
// value back over the snapshot so the next frame diffs against what the client now has.
void CSWSMessage::StoreValuesInLastPlayerUpdateObject(CSWSPlayer* pPlayer,
                                                      CSWSPlayerLastUpdateObject* pLast,
                                                      CLastUpdateObject* pLastObject,
                                                      unsigned int nMask)
{
    K2_ASSUME(pLastObject != 0);
    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return;

    // The loose character-sheet scalars. The write order is not the declaration order. Armor class
    // and gold lead, and the two currencies trail.
    if (nMask & MSG_PLAYER_UPDATE_CHARACTER)
    {
        pLast->m_nArmorClass = pCreature->GetArmorClass();
        pLast->m_nGold = pCreature->GetGold();
        pLast->m_nEncumbranceState = static_cast<short>(pCreature->m_nEncumbranceState);
        pLast->m_bCanLevelUp = pCreature->m_pStats->CanLevelUp();
        pLast->m_nLevel = pCreature->m_pStats->GetLevel(0);
        pLast->m_nExperience = pCreature->m_pStats->m_nExperience;
#if !KOTOR1
        pLast->m_nComponent = pCreature->GetComponent();
        pLast->m_nChemical = pCreature->GetChemical();
#endif
        // The one write that lands on the creature rather than the snapshot. The differ raises this
        // bit while m_bForcePlayerUpdate == 1, so acknowledging the group clears the latch.
        pCreature->m_bForcePlayerUpdate = 0;
    }

    if (nMask & MSG_PLAYER_UPDATE_GUI_SKILLS)
        pLast->m_nGuiSkills = static_cast<unsigned short>(ComputeLastUpdate_GuiSkills(pCreature));

    if (nMask & MSG_PLAYER_UPDATE_STATUS)
        pLast->m_nPlayerState = static_cast<unsigned short>(ComputeLastUpdate_PlayerState(pCreature));

    if (nMask & MSG_PLAYER_UPDATE_FEATS)
    {
        // The two id lists are truncated by hand rather than through Clear, so the backing storage
        // is kept and refilled.
        pLast->m_feats.m_count = 0;
        for (int i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); i++)
            pLast->m_feats.Add(pCreature->m_pStats->GetFeat(static_cast<unsigned short>(i)));

        pLast->m_bonusFeats.m_count = 0;
        for (int i = 0; i < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); i++)
            pLast->m_bonusFeats.Add(pCreature->m_pStats->GetBonusFeat(static_cast<unsigned short>(i)));

        // The remaining-uses list is owned by pointer, so it drains entry by entry from the back. The loop
        // index is the count minus one, not a separate counter.
        for (int i = pLast->m_featRemainingUses.m_count - 1; i >= 0; i--)
        {
            delete pLast->m_featRemainingUses.m_pArray[i];
            pLast->m_featRemainingUses.m_pArray[i] = 0;
            pLast->m_featRemainingUses.DelIndex(i);
        }

        CSWSCreatureStats* pStats = pCreature->m_pStats;
        for (int i = 0; i < pStats->m_featRemainingUses.m_count; i++)
        {
            CSWSStats_FeatRemainingUses* pEntry = new CSWSStats_FeatRemainingUses;
            pEntry->m_nFeat = pStats->m_featRemainingUses.m_pArray[i]->m_nFeat;
            pEntry->m_nUsesUsed = pStats->m_featRemainingUses.m_pArray[i]->m_nUsesUsed;
            pLast->m_featRemainingUses.Add(pEntry);
        }
    }

    if (nMask & MSG_PLAYER_UPDATE_KNOWN_SPELLS)
    {
        pLast->ClearKnownSpells();

        for (unsigned char nClass = 0; nClass != 2; nClass++)
            for (unsigned char i = 0;
                 i < pCreature->m_pStats->GetNumberKnownSpells(nClass, 0); i++)
                pLast->AddKnownSpell(nClass, 0,
                                     pCreature->m_pStats->GetKnownSpell(nClass, 0, i));
    }

    if (nMask & MSG_PLAYER_UPDATE_ACTION_QUEUE)
        UpdateLastUpdateActionQueue(pCreature, pLast);

    if (nMask & MSG_PLAYER_UPDATE_VISIBILITY)
        UpdateLastUpdateVisibilityList(pCreature, pLast);

    // The six base scores are field reads; the six effective ones are accessor calls.
    if (pLastObject != 0 && (nMask & MSG_PLAYER_UPDATE_ABILITIES) && pCreature->m_pStats != 0)
    {
        CSWSCreatureStats* pStats = pCreature->m_pStats;
        pLastObject->m_nAbilityBase[0] = pStats->m_nSTRBase;
        pLastObject->m_nAbilityBase[1] = pStats->m_nDEXBase;
        pLastObject->m_nAbilityBase[2] = pStats->m_nCONBase;
        pLastObject->m_nAbilityBase[3] = pStats->m_nINTBase;
        pLastObject->m_nAbilityBase[4] = pStats->m_nWISBase;
        pLastObject->m_nAbilityBase[5] = pStats->m_nCHABase;
        pLastObject->m_nAbilityStat[0] = pStats->GetSTRStat();
        pLastObject->m_nAbilityStat[1] = pCreature->m_pStats->GetDEXStat();
        pLastObject->m_nAbilityStat[2] = pCreature->m_pStats->GetCONStat();
        pLastObject->m_nAbilityStat[3] = pCreature->m_pStats->GetINTStat();
        pLastObject->m_nAbilityStat[4] = pCreature->m_pStats->GetWISStat();
        pLastObject->m_nAbilityStat[5] = pCreature->m_pStats->GetCHAStat();
    }

    if (pLastObject != 0 && (nMask & MSG_PLAYER_UPDATE_SKILL_RANKS) && pCreature->m_pStats != 0)
    {
        for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
        {
            pLastObject->m_nSkillRank[i] = pCreature->m_pStats->GetSkillRank(i, 0, 1);
            pLastObject->m_nSkillRankBase[i] = pCreature->m_pStats->GetSkillRank(i, 0, 0);
        }
    }

    if (nMask & MSG_PLAYER_UPDATE_PORTRAIT)
    {
        pLastObject->m_nPortraitId = pCreature->GetPortraitId();
        pLastObject->m_portrait = pCreature->GetPortrait();
    }

    if (nMask & MSG_PLAYER_UPDATE_HIT_POINTS)
    {
        pLastObject->m_nHitPoints = static_cast<unsigned short>(pCreature->m_hitPoints);
        pLastObject->m_nCurrentHitPoints = pCreature->GetCurrentHitPoints(0);
        pLastObject->m_nHitPointBonus = static_cast<unsigned short>(pCreature->m_hitPointBonus);
        pLastObject->m_nMaxHitPoints = pCreature->GetMaxHitPoints(1);
    }

    if (nMask & MSG_PLAYER_UPDATE_FORCE_POINTS)
    {
        pLastObject->m_nBaseForcePoints = pCreature->m_pStats->m_nBaseForcePoints;
        pLastObject->m_nForcePoints = pCreature->m_pStats->m_nCurrentForcePoints
                                      + pCreature->m_pStats->m_nTemporaryForcePoints;
        pLastObject->m_nTemporaryForcePoints = pCreature->m_pStats->m_nTemporaryForcePoints;
        pLastObject->m_nMaxForcePoints = pCreature->GetMaxForcePoints();
    }

    if (nMask & MSG_PLAYER_UPDATE_SPELL_LIKE_ABILITIES)
        ComputeLastUpdate_StoreUpdateSpellLikeAbility(pCreature, pLast);
}

// Writes the frame's big panels, the character-sheet, known-spell and feat panel snapshot, then the
// equipped-inventory diff, then the two repository updates. The subject is the player's own
// creature unless the in-game GUI is showing a party member's sheet. The repository pointer is left
// uninitialized when the open container is neither an item nor a placeable.
void CSWSMessage::WriteGameObjUpdate_MajorGUIPanels(CSWSPlayer* pPlayer)
{
    CExoArrayList<unsigned int> aChangedSlots;

    CSWSObject* pObject;
#if KOTOR1  // K1: always the player's own creature
    pObject = pPlayer->GetGameObject();
#else
    int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
    if (nOverride != -1)
    {
        unsigned long long nSubjectId =
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nOverride, 0, 1);
        // CGameObject is CSWSObject's base, and the object array only holds server objects here.
        pObject = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(nSubjectId));
    }
    else
    {
        pObject = pPlayer->GetGameObject();
    }
#endif

    if (pObject == 0)
        return;

    if (pObject->AsSWSCreature() == 0)
        return;

    // -- the panel snapshot --
    CSWSPlayerLastUpdateObject* pLast = pPlayer->m_pLastUpdateObject;
    CLastUpdateObject* pLastObject =
        static_cast<CLastUpdateObject*>(pPlayer->GetLastUpdateObject(pObject->m_objectId));
    if (pLastObject != 0)
    {
        unsigned int nMask = 0;
        if (pLast != 0)
        {
            TestPlayerUpdateDifferences(pPlayer, pLast, pLastObject, nMask);
        }
        else
        {
            // On first sight of this player's panels, build a snapshot and send everything.
            pLast = pPlayer->CreateNewPlayerLastUpdateObject();
            ComputeLastUpdate_GuiKnownSpells(pObject->AsSWSCreature(), pLast);
            pObject->AsSWSCreature();
            nMask = MSG_PLAYER_UPDATE_ALL;
        }

        if (nMask != 0)
        {
            WriteGameObjUpdate_PlayerUpdate(pPlayer, pLast, pLastObject, nMask);
            StoreValuesInLastPlayerUpdateObject(pPlayer, pLast, pLastObject, nMask);
        }
    }

    // -- the inventory panel, only while the client actually has it open --
    if (pPlayer->m_pInventoryGUI->m_bOpen != 1)
        return;

#ifdef K2_FIX_ENGINE_BUGS
    CItemRepository* pRepository = 0;
#else
    CItemRepository* pRepository;  // a container that is neither an item nor a placeable leaves it unset
#endif
    unsigned long long nContainerId = pPlayer->m_pContainerGUI->m_objectId;
#if KOTOR1  // K1: in the intro module with the party panel up, the slots are the equip-screen NPC's
    CSWSObject* pSubject = pObject;
    if (nContainerId == CGameObjectArray::INVALIDOBJID)
    {
        unsigned long long oidPlayer = pPlayer->m_oidSWSObject;
        int bSwapped;
        if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->GetPartyAccessPanelUp(1))
        {
            int nNPC = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCurrentEquipScreenNPC();
            unsigned long long oidNPC =
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nNPC, 0, 1);
            CSWSCreature* pNPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidNPC);
            if (pNPC != 0)
            {
                pPlayer->m_oidSWSObject = oidNPC;
                bSwapped = 1;
                pSubject = static_cast<CSWSObject*>(pNPC);
            }
            else
            {
                bSwapped = 0;
            }
        }
        else
        {
            bSwapped = 0;
        }

        for (int nSlot = 0; nSlot < 18; nSlot++)
        {
            if (ComputeInventoryUpdateRequired(pPlayer, nSlot) != 1)
                continue;

            UpdateLastUpdateInventory(pPlayer, nSlot);

            unsigned int nSlotFlag = 1 << nSlot;
            if (pSubject->AsSWSCreature()->m_pInventory->GetItemInSlot(nSlotFlag) != 0)
            {
                aChangedSlots.Add(nSlotFlag);
            }
            else
            {
                WriteCHAR('G', 8);
                WriteCHAR('I', 8);
                WriteCHAR('D', 8);
                WriteDWORD(nSlotFlag, 32);
            }
        }

        for (int i = 0; i < aChangedSlots.m_count; i++)
        {
            WriteCHAR('G', 8);
            WriteCHAR('I', 8);
            WriteCHAR('A', 8);
            WriteDWORD(aChangedSlots.m_pArray[i], 32);

            CSWSItem* pItem = static_cast<CSWSItem*>(pSubject->AsSWSCreature()->m_pInventory
                                  ->GetItemInSlot(aChangedSlots.m_pArray[i]));
            WriteOBJECTIDServer(pItem->m_objectId);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pSubject->AsSWSCreature());
        }

        if (bSwapped)
        {
            pPlayer->m_oidSWSObject = oidPlayer;
            pSubject = pObject;
        }
    }
    else
    {
        CGameObject* pContainer;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nContainerId,
                                                                           &pContainer)
            == CGameObjectArray::SUCCESS)
        {
            if (pContainer->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
                pRepository = pContainer->AsSWSItem()->m_pItemRepository;
            if (pContainer->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                pRepository = pContainer->AsSWSPlaceable()->m_pItemRepository;

#ifdef K2_FIX_ENGINE_BUGS
            if (pRepository != 0)
#endif
            WriteRepositoryUpdate(pPlayer, pObject, pRepository,
                                  pPlayer->m_pInventoryGUI->m_pInventory, 0, 'C');
        }
    }

    WriteRepositoryUpdate(pPlayer, pSubject, pSubject->AsSWSCreature()->GetItemRepository(1),
                          pPlayer->m_pInventoryGUI->m_pInventory, 1, 'R');
#else
    if (nContainerId == CGameObjectArray::INVALIDOBJID)
    {
        // No container open, so the subject is the creature's own twenty equipment slots.
        for (int nSlot = 0; nSlot < 20; nSlot++)
        {
            if (ComputeInventoryUpdateRequired(pPlayer, nSlot) != 1)
                continue;

            UpdateLastUpdateInventory(pPlayer, nSlot);

            unsigned int nSlotFlag = 1 << nSlot;
            if (pObject->AsSWSCreature()->m_pInventory->GetItemInSlot(nSlotFlag) != 0)
            {
                // The add frames all go out together below, after the deletes.
                aChangedSlots.Add(nSlotFlag);
            }
            else
            {
                WriteCHAR('G', 8);
                WriteCHAR('I', 8);
                WriteCHAR('D', 8);
                WriteDWORD(nSlotFlag, 32);
            }
        }

        for (int i = 0; i < aChangedSlots.m_count; i++)
        {
            WriteCHAR('G', 8);
            WriteCHAR('I', 8);
            WriteCHAR('A', 8);
            WriteDWORD(aChangedSlots.m_pArray[i], 32);

            // GetItemInSlot is declared to return the CSWItem base; on the server the object is a CSWSItem.
            CSWSItem* pItem = static_cast<CSWSItem*>(pObject->AsSWSCreature()->m_pInventory
                                  ->GetItemInSlot(aChangedSlots.m_pArray[i]));
            WriteOBJECTIDServer(pItem->m_objectId);
            AddItemAppearanceToMessage(pItem);
            AddActiveItemPropertiesToMessage(pItem, pObject->AsSWSCreature());
        }
    }
    else
    {
        // A container is open, so its repository is the subject instead.
        CGameObject* pContainer;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nContainerId,
                                                                           &pContainer)
            == CGameObjectArray::SUCCESS)
        {
            // Two sequential tests, not an else-if chain, so the object type is re-read after the
            // item arm.
            if (pContainer->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
                pRepository = pContainer->AsSWSItem()->m_pItemRepository;
            if (pContainer->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                pRepository = pContainer->AsSWSPlaceable()->m_pItemRepository;

#ifdef K2_FIX_ENGINE_BUGS
            if (pRepository != 0)
#endif
            WriteRepositoryUpdate(pPlayer, pObject, pRepository,
                                  pPlayer->m_pInventoryGUI->m_pInventory, 0, 'C');
        }
    }

    WriteRepositoryUpdate(pPlayer, pObject, pObject->AsSWSCreature()->GetItemRepository(1),
                          pPlayer->m_pInventoryGUI->m_pInventory, 1, 'R');
#endif
}

// The byte budget of one game-object update message.
enum { kGameObjUpdateBudget = 2000 };

// The per-frame "here is everything that changed near you" send. It hands a fixed byte budget to
// a ladder of writers and stops at the first one that runs out of room, then tells the client how
// far it got so the next frame can resume. The object id argument is never read.
int CSWSMessage::SendServerToPlayerGameObjUpdate(CSWSPlayer* pPlayer, unsigned long long objectId)
{
    K2_UNUSED(objectId);
    g_pExoBase->m_pTimers->GetHighResolutionTimer();

    CSWSObject* pObject = static_cast<CSWSObject*>(pPlayer->GetGameObject());
    CSWSArea* pArea = 0;
    if (pObject != 0)
        pArea = static_cast<CSWSArea*>(pObject->GetArea());

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray == 0)
        return 0;

    CreateWriteMessage(1024, pPlayer->m_nPlayerId, 1);
    DeleteLastUpdateObjectsInOtherAreas(pPlayer);
    g_pExoBase->m_pTimers->GetHighResolutionTimer();

    unsigned int nSize;
    unsigned char* pMsg;
    CSWSPlayerLUOSortedObjectList* pSorted = 0;
    int nSortedCount;
    int nStage = 0;
    int bSorted = 0;

    if (PeekAtWriteMessageSize() < kGameObjUpdateBudget)
    {
        int bHaveArea = (pObject != 0) && (pArea != 0);

        if (bHaveArea)
            ComputeGameObjectUpdateForObject(pPlayer, pObject, pObjectArray, pObject->m_objectId);

        if (PeekAtWriteMessageSize() >= kGameObjUpdateBudget)
        {
            nStage = 1;
        }
        else
        {
            if (bHaveArea)
                WriteGameObjUpdate_MajorGUIPanels(pPlayer);

            if (PeekAtWriteMessageSize() >= kGameObjUpdateBudget)
            {
                nStage = 2;
            }
            else
            {
                pSorted = SortObjectsForGameObjectUpdate(pPlayer, pObject, pObjectArray,
                                                         &nSortedCount);
                bSorted = 1;

                int nCount = nSortedCount;

                if (ComputeGameObjectUpdateForCategory(1, kGameObjUpdateBudget, pPlayer, pObject,
                                                       pObjectArray,
                                                       pSorted, nCount) != 0)
                {
                    nStage = 3;
                }
                else
                {
                    WriteGameObjUpdate_MinorGUIPanels(pPlayer);

                    if (PeekAtWriteMessageSize() > kGameObjUpdateBudget - 1)
                        nStage = 4;
                    else if (ComputeGameObjectUpdateForCategory(3, kGameObjUpdateBudget, pPlayer,
                                                                pObject,
                                                                pObjectArray, pSorted,
                                                                nCount) != 0)
                        nStage = 6;
                    else if (ComputeGameObjectUpdateForCategory(4, kGameObjUpdateBudget, pPlayer,
                                                                pObject,
                                                                pObjectArray, pSorted,
                                                                nCount) != 0)
                        nStage = 7;
                    else
                    {
                        WriteGameObjUpdate_PartyAIState(pPlayer);

                        if (PeekAtWriteMessageSize() > kGameObjUpdateBudget - 1)
                            nStage = 8;
                        else if (ComputeGameObjectUpdateForCategory(5, kGameObjUpdateBudget,
                                                                    pPlayer, pObject,
                                                                    pObjectArray, pSorted,
                                                                    nCount) != 0)
                            nStage = 9;
                        else if (ComputeGameObjectUpdateForCategory(6, kGameObjUpdateBudget,
                                                                    pPlayer, pObject,
                                                                    pObjectArray, pSorted,
                                                                    nCount) != 0)
                            nStage = 10;
                        else if (ComputeGameObjectUpdateForCategory(7, kGameObjUpdateBudget,
                                                                    pPlayer, pObject,
                                                                    pObjectArray, pSorted,
                                                                    nCount) != 0)
                            nStage = 11;
                        else if (ComputeGameObjectUpdateForCategory(8, kGameObjUpdateBudget,
                                                                    pPlayer, pObject,
                                                                    pObjectArray, pSorted,
                                                                    nCount) != 0)
                            nStage = 12;
                        else if (ComputeGameObjectUpdateForCategory(9, kGameObjUpdateBudget,
                                                                    pPlayer, pObject,
                                                                    pObjectArray, pSorted,
                                                                    nCount) != 0)
                            nStage = 13;
                        else
                            nStage = 14;
                    }
                }
            }
        }
    }

    if (bSorted == 0)
        pSorted = SortObjectsForGameObjectUpdate(pPlayer, pObject, pObjectArray, &nSortedCount);

    WriteGameObjUpdate_WorkRemaining(pObject, pArea, nStage, 14);
    delete[] pSorted;

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    // A frame with nothing to say is a success, not a send.
    if (nSize == 0)
        return 1;

    return SendServerToPlayerMessage(pPlayer->m_nPlayerId, MSG_MAJOR_GAME_OBJECT_UPDATE,
                                     MSG_GAME_OBJECT_UPDATE_UPDATE, pMsg, nSize);
}

// Ships every entry of the frame's sorted list that belongs to one category, stopping when the
// message crosses the budget. Returns 1 when it ran out of room, which is what stops the ladder
// in SendServerToPlayerGameObjUpdate.
int CSWSMessage::ComputeGameObjectUpdateForCategory(unsigned int nCategory, unsigned int nBudget,
                                                    CSWSPlayer* pPlayer, CSWSObject* pObject,
                                                    CGameObjectArray* pArray,
                                                    CSWSPlayerLUOSortedObjectList* pSorted,
                                                    int nCount)
{
    CGameObject* pTarget = 0;

    CSWSArea* pArea = 0;
    if (pObject != 0)
        pArea = static_cast<CSWSArea*>(pObject->GetArea());

    if (pArea == 0)
    {
        if (pObject->AsSWSCreature() == 0)
            return 0;

        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        if (pModule == 0)
            return 0;

        pArea = pModule->GetArea();
    }

    if (pArea == 0)
        return 0;

    for (int i = 0; i < nCount; i++)
    {
        if (static_cast<unsigned int>(pSorted[i].m_nCategory) != nCategory)
            continue;

        unsigned long long objectId = pSorted[i].m_objectId;
        if (pArray->GetGameObject(objectId, &pTarget) != CGameObjectArray::SUCCESS)
            continue;

        ComputeGameObjectUpdateForObject(pPlayer, pObject, pArray, objectId);

        if (PeekAtWriteMessageSize() >= nBudget)
            return 1;
    }

    return 0;
}

// Closes a frame with the pair "how far we got, how far there was to go". m_bDesiredAreaUpdateComplete is the latch:
// once a complete frame has been acknowledged, an in-area creature stops being told anything.
void CSWSMessage::WriteGameObjUpdate_WorkRemaining(CSWSObject* pObject, CSWSArea* pArea,
                                                   int nStage, int nStageCount)
{
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return;

    if (pArea != 0 && pCreature->m_bDesiredAreaUpdateComplete != 0)
        return;

    if (nStage != nStageCount)
    {
        WriteBYTE('W', 8);
        WriteBYTE(static_cast<unsigned char>(nStage), 8);
        WriteBYTE(static_cast<unsigned char>(nStageCount), 8);
        return;
    }

    if (pCreature->m_bDesiredAreaUpdateComplete != 0)
        return;

    WriteBYTE('W', 8);
    WriteBYTE(static_cast<unsigned char>(nStage), 8);
    WriteBYTE(static_cast<unsigned char>(nStageCount), 8);
    pCreature->m_bDesiredAreaUpdateComplete = 1;
}

// Forwards to the overload above with an invalid object id.
int CSWSMessage::SendServerToPlayerGameObjUpdate(CSWSPlayer* pPlayer)
{
    return SendServerToPlayerGameObjUpdate(pPlayer, CGameObjectArray::INVALIDOBJID);
}

extern int g_bEnableCombatDebugging;

// Sends the combat-feedback message. The server hands over one CSWCCMessageData bag (four parallel
// lists plus a type byte) and this packs the subset the feedback UI needs for that message type, at
// that field's bit width. Unhandled types fall through to the bare write-message tail with no
// message built, and the attack-data parameter is never read.
int CSWSMessage::SendServerToPlayerCCMessage(unsigned int nPlayerId, unsigned char nType,
                                             CSWCCMessageData* pData,
                                             CSWSCombatAttackData* pAttackData)
{
    K2_UNUSED(pAttackData);
    unsigned char* pMsg = 0;
    unsigned int nSize = 0;

    if (pData == 0)
        return 0;

    switch (nType)
    {
    case 1:
        CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteDWORD(pData->GetInteger(0), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_DAMAGE:
        CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        for (int i = 0; i < 15; i++)
        {
            if (pData->GetInteger(i) >= 0)
            {
                WriteBOOL(1);
                WriteWORD(static_cast<unsigned short>(pData->GetInteger(i)), 14);
            }
            else
            {
                WriteBOOL(0);
            }
        }
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_DAMAGE_FROM_ATTACKER:
        CreateWriteMessage(18, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        for (int i = 0; i < 15; i++)
        {
            if (pData->GetInteger(i) < 0)
            {
                WriteBOOL(0);
            }
            else
            {
                WriteBOOL(1);
                WriteWORD(static_cast<unsigned short>(pData->GetInteger(i)), 14);
            }
        }
        WriteBOOL(g_bEnableCombatDebugging);
        if (g_bEnableCombatDebugging)
            WriteCExoString(pData->GetString(0), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_DEATH:
        CreateWriteMessage(20, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteINT(pData->GetInteger(0), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case 5:
    case 6:
        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, 0, 0);

    case CC_MESSAGE_SAVING_THROW:
        CreateWriteMessage(14, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(0)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(1)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(2)), 5);
        WriteCHAR(static_cast<char>(pData->GetInteger(3)), 8);
        WriteCHAR(static_cast<char>(pData->GetInteger(4)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(5)), 7);
        WriteOBJECTIDServer(pData->GetInteger(6));
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_SPELL:
#if KOTOR1  // K1: three trailing INTs, not five
        CreateWriteMessage(40, MSG_PLAYERID_ALL_CLIENTS, 1);
#else
        CreateWriteMessage(48, MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(2));
        WriteINT(pData->GetInteger(0), 32);
        if (pData->GetObjectID(1) == OBJECT_INVALID)
        {
            WriteBOOL(0);
        }
        else
        {
            WriteBOOL(1);
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pData->GetObjectID(1));
            if (pItem)
                WriteCExoLocStringServer(pItem->m_localizedName, 0);
        }
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
#if !KOTOR1
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
#endif
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_SKILL:
        CreateWriteMessage(44, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteDWORD(pData->GetInteger(0), 32);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(1)), 5);
        WriteCHAR(static_cast<char>(pData->GetInteger(2)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(3)), 7);
        WriteBOOL(pData->GetInteger(4));
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(5)), 8);
        WriteDWORD(pData->GetInteger(6), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_FORCE_RESIST:
        CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(0)), 8);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_FEEDBACK:
        CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteWORD(static_cast<unsigned short>(pData->GetInteger(9)), 16);
        if (pData->GetObjectID(0) != OBJECT_INVALID
            && pData->GetInteger(9) != FEEDBACK_ITEM_ACQUIRED
            && pData->GetInteger(9) != FEEDBACK_ITEM_LOST
            && pData->GetInteger(9) != FEEDBACK_SCRIPT_DEBUG_MESSAGE
            && pData->GetInteger(9) != FEEDBACK_ACTION_LIST_OVERFLOW
            && pData->GetInteger(9) != FEEDBACK_EFFECT_LIST_OVERFLOW
            && pData->GetInteger(9) != FEEDBACK_ACTION_LIST_ERASED
            && pData->GetInteger(9) != FEEDBACK_EFFECT_LIST_ERASED
            && pData->GetInteger(9) != FEEDBACK_AI_UPDATE_TIME_OVERFLOW)
        {
            WriteOBJECTIDServer(pData->GetObjectID(0));
        }
        if (pData->GetInteger(9) == FEEDBACK_SKILL_NOT_READY || pData->GetInteger(9) == 5
            || pData->GetInteger(9) == 71 || pData->GetInteger(9) == FEEDBACK_ALIGNMENT_SHIFT
            || pData->GetInteger(9) == FEEDBACK_CREDITS_ACQUIRED
            || pData->GetInteger(9) == FEEDBACK_CREDITS_LOST
            || pData->GetInteger(9) == FEEDBACK_HEALED
            || pData->GetInteger(9) == FEEDBACK_FORCE_POINTS_REGAINED
            || pData->GetInteger(9) == FEEDBACK_XP_GAINED
            || pData->GetInteger(9) == FEEDBACK_XP_LOST
#if KOTOR1  // K1: no components / chemicals feedback
            || pData->GetInteger(9) == 222)
#else
            || pData->GetInteger(9) == 222
            || pData->GetInteger(9) == FEEDBACK_COMPONENTS_ACQUIRED
            || pData->GetInteger(9) == FEEDBACK_CHEMICALS_ACQUIRED)
#endif
        {
            WriteINT(pData->GetInteger(0), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_DAMAGE_IMMUNITY
            || pData->GetInteger(9) == FEEDBACK_SPELL_IMMUNITY
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_RESISTED
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_RESISTED_REMAINING
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_REDUCED
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_REDUCED_REMAINING
            || pData->GetInteger(9) == 69 || pData->GetInteger(9) == 70
            || pData->GetInteger(9) == 71 || pData->GetInteger(9) == FEEDBACK_ALIGNMENT_SHIFT
            || pData->GetInteger(9) == FEEDBACK_ATTRIBUTE_REDUCED)
        {
            WriteINT(pData->GetInteger(1), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_DAMAGE_IMMUNITY
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_RESISTED_REMAINING
            || pData->GetInteger(9) == FEEDBACK_DAMAGE_REDUCED_REMAINING
            || pData->GetInteger(9) == 70
            || pData->GetInteger(9) == FEEDBACK_ATTRIBUTE_REDUCED)
        {
            WriteINT(pData->GetInteger(2), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_ATTRIBUTE_REDUCED)
            WriteFLOAT(pData->GetFloat(0), 1.0f, 32);
        if (pData->GetInteger(9) == FEEDBACK_ITEM_ACQUIRED
            || pData->GetInteger(9) == FEEDBACK_ITEM_LOST)
        {
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pData->GetObjectID(0));
            if (pItem)  // no item: no BOOL, though the client reads one
            {
                WriteBOOL(1);
                WriteCExoLocStringServer(pItem->m_localizedName, 0);
            }
        }
        if (pData->GetInteger(9) == 4 || pData->GetInteger(9) == 47)
        {
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pData->GetObjectID(1));
            if (pItem)  // no item: no BOOL, though the client reads one
            {
                WriteBOOL(1);
                WriteCExoLocStringServer(pItem->m_localizedName, 0);
            }
        }
        if (pData->GetInteger(9) == FEEDBACK_FLOATING_TEXT_STRREF
            && pData->GetObjectID(0) != OBJECT_INVALID)
        {
            WriteINT(pData->GetInteger(0), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_FLOATING_TEXT_STRING
            && pData->GetObjectID(0) != OBJECT_INVALID)
        {
            WriteCExoString(pData->GetString(0), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_SCRIPT_DEBUG_MESSAGE)
        {
            WriteOBJECTIDServer(pData->GetObjectID(1));
            WriteCExoString(pData->GetString(0), 32);
            WriteCExoString(pData->GetString(1), 32);
            WriteINT(pData->GetInteger(0), 32);
        }
        if (pData->GetInteger(9) == FEEDBACK_ACTION_LIST_OVERFLOW
            || pData->GetInteger(9) == FEEDBACK_EFFECT_LIST_OVERFLOW
            || pData->GetInteger(9) == FEEDBACK_ACTION_LIST_ERASED
            || pData->GetInteger(9) == FEEDBACK_EFFECT_LIST_ERASED
            || pData->GetInteger(9) == FEEDBACK_AI_UPDATE_TIME_OVERFLOW)
        {
            WriteOBJECTIDServer(pData->GetObjectID(1));
            WriteCExoString(pData->GetString(1), 32);
            WriteINT(pData->GetInteger(0), 32);
            WriteINT(pData->GetInteger(1), 32);
            if (pData->GetInteger(9) == FEEDBACK_AI_UPDATE_TIME_OVERFLOW)
            {
                WriteCExoString(pData->GetString(2), 32);
                WriteINT(pData->GetInteger(2), 32);
                WriteINT(pData->GetInteger(3), 32);
            }
        }
        if (pData->GetInteger(9) == FEEDBACK_SCRIPT_MESSAGE)
            WriteCExoString(pData->GetString(0), 32);
        if (pData->GetInteger(9) == FEEDBACK_EXPERIENCE_RECEIVED)
            WriteINT(pData->GetInteger(0), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_FORCE_POINT_USAGE:
        CreateWriteMessage(22, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteCExoString(pData->GetString(0), 32);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_ATTACK_ROLL:
        CreateWriteMessage(20, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteCHAR(static_cast<char>(pData->GetInteger(0)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(1)), 5);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(2)), 4);
        WriteBOOL(pData->GetInteger(3));
        if (pData->GetInteger(4) >= 1)
        {
            WriteBOOL(1);
            WriteBYTE(static_cast<unsigned char>(pData->GetInteger(4)), 7);
        }
        else
        {
            WriteBOOL(0);
        }
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case 14:
        CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(0)), 5);
        WriteCHAR(static_cast<char>(pData->GetInteger(1)), 8);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_DISPEL:
        CreateWriteMessage(pData->GetInteger(0) * 2 + 9, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(0)), 8);
        for (unsigned char i = 0; i < static_cast<unsigned char>(pData->GetInteger(0)); i++)
            WriteWORD(static_cast<unsigned short>(pData->GetInteger(i + 1)), 9);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_STEALTH_DETECTION:
        CreateWriteMessage(40, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteBOOL(pData->GetInteger(0));
        WriteSHORT(static_cast<short>(pData->GetInteger(1)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(2)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(3)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(4)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(5)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(6)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(7)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(8)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(9)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(10)), 16);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_FORCE_RESISTANCE_ROLL:
        if (pData->GetInteger(2) == 0)
            return 1;
        CreateWriteMessage(28, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(0)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(1)), 8);
        WriteBYTE(static_cast<unsigned char>(pData->GetInteger(2)), 8);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_ATTACK_SUMMARY:
        CreateWriteMessage(50, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteINT(pData->GetInteger(0), 32);
        WriteWORD(static_cast<unsigned short>(pData->GetInteger(1)), 16);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        WriteBOOL(pData->GetInteger(9));
        WriteBOOL(pData->GetInteger(10));
        WriteBOOL(pData->GetInteger(11));
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

    case CC_MESSAGE_MINE_DETECTION:
        CreateWriteMessage(20, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteOBJECTIDServer(pData->GetObjectID(1));
        WriteSHORT(static_cast<short>(pData->GetInteger(0)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(1)), 16);
        WriteSHORT(static_cast<short>(pData->GetInteger(2)), 16);
        if (GetWriteMessage(&pMsg, &nSize) == 0)
            return 0;

        return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg,
                                         nSize);

#if KOTOR1  // K1: 20 and 23 are separate records (21 and 28 INTs)
    case CC_MESSAGE_ATTACK_BREAKDOWN:
        CreateWriteMessage(84, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        WriteINT(pData->GetInteger(9), 32);
        WriteINT(pData->GetInteger(10), 32);
        WriteINT(pData->GetInteger(11), 32);
        WriteINT(pData->GetInteger(12), 32);
        WriteINT(pData->GetInteger(13), 32);
        WriteINT(pData->GetInteger(14), 32);
        WriteINT(pData->GetInteger(15), 32);
        WriteINT(pData->GetInteger(16), 32);
        WriteINT(pData->GetInteger(17), 32);
        WriteINT(pData->GetInteger(18), 32);
        WriteINT(pData->GetInteger(19), 32);
        WriteINT(pData->GetInteger(20), 32);
        break;

    case CC_MESSAGE_DAMAGE_BREAKDOWN:
        CreateWriteMessage(112, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        WriteINT(pData->GetInteger(9), 32);
        WriteINT(pData->GetInteger(10), 32);
        WriteINT(pData->GetInteger(11), 32);
        WriteINT(pData->GetInteger(12), 32);
        WriteINT(pData->GetInteger(13), 32);
        WriteINT(pData->GetInteger(14), 32);
        WriteINT(pData->GetInteger(15), 32);
        WriteINT(pData->GetInteger(16), 32);
        WriteINT(pData->GetInteger(17), 32);
        WriteINT(pData->GetInteger(18), 32);
        WriteINT(pData->GetInteger(19), 32);
        WriteINT(pData->GetInteger(20), 32);
        WriteINT(pData->GetInteger(21), 32);
        WriteINT(pData->GetInteger(22), 32);
        WriteINT(pData->GetInteger(23), 32);
        WriteINT(pData->GetInteger(24), 32);
        WriteINT(pData->GetInteger(25), 32);
        WriteINT(pData->GetInteger(26), 32);
        WriteINT(pData->GetInteger(27), 32);
        break;

#else
    case CC_MESSAGE_ATTACK_BREAKDOWN:
    case CC_MESSAGE_DAMAGE_BREAKDOWN:
        CreateWriteMessage(124, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        WriteINT(pData->GetInteger(9), 32);
        WriteINT(pData->GetInteger(10), 32);
        WriteINT(pData->GetInteger(11), 32);
        WriteINT(pData->GetInteger(12), 32);
        WriteINT(pData->GetInteger(13), 32);
        WriteINT(pData->GetInteger(14), 32);
        WriteINT(pData->GetInteger(15), 32);
        WriteINT(pData->GetInteger(16), 32);
        WriteINT(pData->GetInteger(17), 32);
        WriteINT(pData->GetInteger(18), 32);
        WriteINT(pData->GetInteger(19), 32);
        WriteINT(pData->GetInteger(20), 32);
        WriteINT(pData->GetInteger(21), 32);
        WriteINT(pData->GetInteger(22), 32);
        WriteINT(pData->GetInteger(23), 32);
        WriteINT(pData->GetInteger(24), 32);
        WriteINT(pData->GetInteger(25), 32);
        WriteINT(pData->GetInteger(26), 32);
        WriteINT(pData->GetInteger(27), 32);
        WriteINT(pData->GetInteger(28), 32);
        WriteINT(pData->GetInteger(29), 32);
        WriteINT(pData->GetInteger(30), 32);
        break;
#endif

    case CC_MESSAGE_THREAT_BREAKDOWN:
        CreateWriteMessage(32, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        break;

    case CC_MESSAGE_DEFENSE_BREAKDOWN:
        CreateWriteMessage(36, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        break;

    case CC_MESSAGE_EFFECT_BREAKDOWN:
#if KOTOR1  // K1: 26 is the same record as 25
    case CC_MESSAGE_DEFLECT_BREAKDOWN:
#endif
        CreateWriteMessage(44, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        break;

#if !KOTOR1
    case CC_MESSAGE_DEFLECT_BREAKDOWN:
        CreateWriteMessage(76, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteINT(pData->GetInteger(0), 32);
        WriteINT(pData->GetInteger(1), 32);
        WriteINT(pData->GetInteger(2), 32);
        WriteINT(pData->GetInteger(3), 32);
        WriteINT(pData->GetInteger(4), 32);
        WriteINT(pData->GetInteger(5), 32);
        WriteINT(pData->GetInteger(6), 32);
        WriteINT(pData->GetInteger(7), 32);
        WriteINT(pData->GetInteger(8), 32);
        WriteINT(pData->GetInteger(9), 32);
        WriteINT(pData->GetInteger(10), 32);
        WriteINT(pData->GetInteger(11), 32);
        WriteINT(pData->GetInteger(12), 32);
        WriteINT(pData->GetInteger(13), 32);
        WriteINT(pData->GetInteger(14), 32);
        WriteINT(pData->GetInteger(15), 32);
        WriteINT(pData->GetInteger(16), 32);
        break;
#endif

    case CC_MESSAGE_POISONED:
        CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
        WriteOBJECTIDServer(pData->GetObjectID(0));
        WriteINT(pData->GetInteger(0), 32);
        break;
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendServerToPlayerMessage(nPlayerId, MSG_MAJOR_CLIENT_SIDE_MESSAGE, nType, pMsg, nSize);
}

