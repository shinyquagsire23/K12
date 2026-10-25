#include "nwmessage.h"
#include "nwcreaturelevelstats.h"
#include "nwupdatestats.h"
#include "nwspell.h"
#include "nwstrigger.h"
#include "nwsdoor.h"
#include "nwsobject.h"
#include "nwsplaceable.h"
#include "serverexoappinternal.h"
#include "nwsparty.h"
#include "nwsitem.h"
#include "nwsjournal.h"
#include "nwsencounter.h"
#include "nwseffectlist.h"
#include <string.h>
#include "CExoArrayListImpl.h"

const int CSWSMessage::GUI_PANEL_PLAYER_DEATH = 0;
const int CSWSMessage::GUI_PANEL_PARTY_INVITATION_RECEIVED = 1;

#include "port/crtapi.h"
extern int g_bEnableCombatDebugging;
int g_bEnableSavingThrowDebugging = 1;
extern int g_bEnableMovementSpeedDebugging;
#include "port/libcmath.h"
#include "nwmessagetypes.h"
#include "feedbackmessage.h"
#include "nwsaiaction.h"
#include "nwsscriptevent.h"
#include "restype.h"
#include "serveranimation.h"
#include "nwscriptconst.h"
static const unsigned char CMessagePlayer_Chat_Talk = MSG_CHAT_TALK;

// The low bits of CGameEffect::m_durationTypeBits that hold the DURATION_TYPE_*.
enum { kEffectDurationTypeMask = 7 };

// The user-defined event the area's script receives when a party member breaks the party table's
// forfeit conditions (CSWSCreature's own copy is in nwscreature.cpp).
enum { USER_EVENT_FORFEIT_VIOLATED = 4001 };

CSWSMessage::CSWSMessage()
{
}

CSWSMessage::~CSWSMessage()
{
}

int CSWSMessage::HandlePlayerToServerQuickChatMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    K2_UNUSED(nType);
    if (MessageReadOverflow(1))
        return 0;

    unsigned short nChat = ReadWORD(16);

    if (MessageReadOverflow(1))
        return 0;

    if (MessageReadUnderflow(1))
        return 0;

    CGameObject* pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    return SendServerToPlayerQuickChatMessage(pObject->m_objectId, nChat);
}

// Route a chat utterance by volume; the tell volume carries a tell target.
int CSWSMessage::HandlePlayerToServerChatMessage(CSWSPlayer* pPlayer, unsigned char nChatVolume)
{
    CExoString sText;
    CExoString sTellTargetName;
    unsigned int nTellPlayerId;
    CGameObject* pObject;

    if (MessageReadOverflow(1))
        return 0;

    if (nChatVolume == MSG_CHAT_TELL)
    {
        if (ReadBOOL())
        {
            sTellTargetName = ReadCExoString(32);
            nTellPlayerId = g_pAppManager->m_pServerExoApp->ResolvePlayerByFirstName(sTellTargetName);
        }
        else
        {
            nTellPlayerId = ReadDWORD(32);
        }
    }
    else
    {
        nTellPlayerId = 0xffffffff;
    }

    sText = ReadCExoString(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;
    if (nChatVolume > MSG_CHAT_SILENT_SHOUT)
        return 0;
    if (((1 << nChatVolume) & ((1 << MSG_CHAT_TALK) | (1 << MSG_CHAT_SHOUT) | (1 << MSG_CHAT_WHISPER) |
                                (1 << MSG_CHAT_TELL) | (1 << MSG_CHAT_PARTY) | (1 << MSG_CHAT_SILENT_SHOUT))) == 0)
        return 0;

    return SendServerToPlayerChatMessage(nChatVolume, pObject->m_objectId, sText, nTellPlayerId,
                                         sTellTargetName);
}

// Dispatches the debug cheat packets, a message-type switch gated on debug mode, plus three
// handlers that run before the gate.
int CSWSMessage::HandlePlayerToServerCheatMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    // Fetched before any type test; only the debug-mode arm uses it.
    CNetLayer* pNetLayer = g_pAppManager->m_pServerExoApp->GetNetLayer();

    // The heal/resurrect cheat, before the debug-mode gate (always allowed).
    if (nType == MSG_CHEAT_RESURRECT)
    {
        CSWSObject* pObject = pPlayer->GetGameObject();
        if (pObject == 0)
            return 0;

        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                      CGameEffect::DURATION_TYPE_INSTANTANEOUS;
        pObject->ApplyEffect(pEffect, 0, 0);

        pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HEAL);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                      CGameEffect::DURATION_TYPE_INSTANTANEOUS;
        pEffect->SetInteger(0, pObject->GetMaxHitPoints(1));
        pObject->ApplyEffect(pEffect, 0, 0);

        if (pObject->AsSWSCreature() != 0)
        {
            CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
            pStats->m_nCurrentForcePoints = pObject->AsSWSCreature()->GetMaxForcePoints();
        }
        return 1;
    }

    // Copy the PC's dead state to the whole party (the "party sync" cheat).
    if (nType == MSG_CHEAT_INVULNERABILITY)
    {
        if (g_pAppManager->m_pServerExoApp != 0 && g_pAppManager->m_pClientExoApp != 0)
        {
            if (g_pAppManager->m_pClientExoApp->GetSWParty() == 0)
                return 0;

            int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;

            CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            if (pPC == 0)
                return 0;

            CSWSCreature* pServer = pPC->GetServerCreature();
            if (pServer == 0)
                return 0;

            int bDead = (pServer->m_bPlot == 0);
            for (int i = 0; i < nCount; i++)
            {
                CSWCCreature* pHero = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                if (pHero == 0)
                    continue;

                CSWSCreature* pHeroServer = pHero->GetServerCreature();
                if (pHeroServer == 0)
                    continue;

                pHeroServer->m_bPlot = bDead;
            }
            return 1;
        }
        return 0;
    }

    // The debug-mode toggle is gated on the admin flag, and it falls through to the debug-mode gate
    // below (it is not a switch case).
    if (nType == MSG_CHEAT_DEBUG_MODE)
    {
        int bDebug = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        if (pNetLayer->GetPlayerInfo(pPlayer->m_nPlayerId)->m_isAdminPermitted != 0)
        {
            g_pAppManager->m_pServerExoApp->SetDebugMode(bDebug);
            SendServerToPlayerCheatDebugMode(bDebug);
        }
        else
        {
            return 1;
        }
    }

    // Every switch case below only runs while cheat/debug mode is on (the debug-mode gate).
    if (g_pAppManager->m_pServerExoApp->GetDebugMode() == 0)
    {
        ClearReadMessage();
        return 1;
    }

    switch (nType)
    {
        case MSG_CHEAT_MOVE_TO_AREA:
        {
            CExoString sAreaName;
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject == 0)
                return 0;

            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;

            sAreaName = ReadCExoString(32);

            float fX = ReadFLOAT(1.0f, 32);
            float fY = ReadFLOAT(1.0f, 32);
            float fZ = ReadFLOAT(1.0f, 32);

            if (MessageReadOverflow(1))
                return 0;

            CSWSArea* pArea = pModule->GetArea();
            if (pArea == 0)
            {
                CSWSArea* pObjArea = static_cast<CSWSArea*>(pCreature->GetArea());
                if (pObjArea == 0)
                {
                    CExoString sError;
                    sError.Format(const_cast<char*>("Area %s not valid."), sAreaName.CStr());
                    SendServerToPlayerChatMessage(5, OBJECT_INVALID, sError, pPlayer->m_nPlayerId,
                                                  CExoString(""));
                    return 0;
                }

#if KOTOR1  // K1: CExoString::GetLength is out of line
                if (sAreaName.GetLength() < 1)
#else
                if (sAreaName.m_sString == 0 || strlen(sAreaName.m_sString) < 1)
#endif
                    pArea = pObjArea;
                else
                {
                    CExoString sError;
                    sError.Format(const_cast<char*>("Area %s is not a valid area."), sAreaName.CStr());
                    SendServerToPlayerChatMessage(5, OBJECT_INVALID, sError, pPlayer->m_nPlayerId,
                                                  CExoString(""));
                    return 0;
                }
            }

            if (fX < 0.0f || fX > pArea->m_nWidth * 10.0f)
            {
                CExoString sError;
                sError.Format(const_cast<char*>("X co-ordinate outside of area, should be in [%f, %f]"), 0.0,
                              static_cast<double>(pArea->m_nWidth * 10.0f));
                SendServerToPlayerChatMessage(5, OBJECT_INVALID, sError, pPlayer->m_nPlayerId,
                                              CExoString(""));
                return 0;
            }

            if (fY < 0.0f || fY > pArea->m_nHeight * 10.0f)
            {
                CExoString sError;
                sError.Format(const_cast<char*>("Y co-ordinate outside of area, should be in [%f, %f]"),
                              static_cast<double>(pArea->m_nHeight * 10.0f));
                SendServerToPlayerChatMessage(5, OBJECT_INVALID, sError, pPlayer->m_nPlayerId,
                                              CExoString(""));
                return 0;
            }

            Vector vPos(fX, fY, fZ);
            Vector vSafe;
            unsigned long long nAreaId = pArea->m_objectId;
            if (pArea->ComputeSafeLocation(vPos, 20.0f, pCreature->m_pPathfindInfo, 1, &vSafe) == 0)
                vSafe = vPos;

            int nZero = 0;
            pCreature->AddAction(AIACTION_JUMP_TO_POINT, ACTION_GROUP_NEW,
                                 ACTION_PARAM_FLOAT, &vSafe.x, ACTION_PARAM_FLOAT, &vSafe.y,
                                 ACTION_PARAM_FLOAT, &vSafe.z, ACTION_PARAM_OBJECT, &nAreaId,
                                 ACTION_PARAM_INT, &nZero);
            return 1;
        }

        // Cast a spell or force power.
        case MSG_CHEAT_CAST_SPELL:
        {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject == 0)
                return 0;

            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;

            unsigned int nSpellId = ReadDWORD(32);
            unsigned char nFlags = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            unsigned long long nTarget = CGameObjectArray::INVALIDOBJID;
            if (nFlags & 1)
            {
                nTarget = ReadOBJECTIDServer();
                if (MessageReadOverflow(1))
                    return 0;
            }

            float vX = 0.0f;
            float vY = 0.0f;
            float vZ = 0.0f;
            int bHasPosition = 0;
            if (nFlags & 2)
            {
                vX = ReadFLOAT(1.0f, 32);
                vY = ReadFLOAT(1.0f, 32);
                vZ = ReadFLOAT(1.0f, 32);
                if (MessageReadOverflow(1))
                    return 0;
                bHasPosition = 1;
            }

            pCreature->ActionManager(1);
            pCreature->AddCastSpellActions(nSpellId, 0xff, 0, 0, 0, Vector(vX, vY, vZ),
                                           nTarget, 0, bHasPosition, 0, 0, 0, 0, -1, 0xff, 0);
            return 1;
        }

        // Apply a death effect from the given creator.
        case MSG_CHEAT_KILL:
        {
            unsigned long long nCreator = ReadOBJECTIDServer();
            unsigned long long nTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return 0;

            CGameObject* pObject = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nTarget,
                                                                                &pObject) !=
                CGameObjectArray::SUCCESS)
            {
                return 0;
            }

            if (pObject == 0)
                return 0;

            if (pObject->AsSWSObject() == 0)
                return 0;

            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
            pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                          CGameEffect::DURATION_TYPE_INSTANTANEOUS;
            pEffect->SetCreator(nCreator);
            pEffect->SetInteger(0, 1);
            pEffect->SetInteger(1, 1);
            pObject->AsSWSObject()->ApplyEffect(pEffect, 0, 0);
            return 1;
        }

        // Run a script.
        case MSG_CHEAT_RUN_SCRIPT:
        {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject == 0)
                return 0;

            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;

            CExoString sScript = ReadCExoString(32);
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            g_pVirtualMachine->RunScript(&sScript, pCreature->m_objectId, 1);
            return 1;
        }

        // Combat and saving-throw debug flags.
        case MSG_CHEAT_TOGGLE_COMBAT_DEBUGGING:
        {
            int bCombat = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;
            g_bEnableCombatDebugging = g_bEnableSavingThrowDebugging = bCombat;
            return 1;
        }

        // Apply a visual effect.
        case MSG_CHEAT_PLAY_VISUAL_EFFECT:
        {
            unsigned long long nCreator = ReadOBJECTIDServer();
            unsigned short nSpellId = ReadWORD(16);
            float fDuration = ReadFLOAT(1.0f, 32);
            unsigned long long nTarget = ReadOBJECTIDServer();
            unsigned char nByte = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            CGameObject* pObject = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nCreator,
                                                                                &pObject) !=
                CGameObjectArray::SUCCESS)
            {
                return 0;
            }

            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pEffect->SetObjectID(0, nTarget);
            pEffect->SetInteger(0, nSpellId & 0xffff);
            pEffect->SetInteger(1, nByte & 0xff);

            if (fDuration == 0.0f)
            {
                pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                              CGameEffect::DURATION_TYPE_INSTANTANEOUS;
            }
            else
            {
                pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                              CGameEffect::DURATION_TYPE_TEMPORARY;
                pEffect->m_fDuration = fDuration;
            }
            static_cast<CSWSObject*>(pObject)->ApplyEffect(pEffect, 0, 0);
            return 1;
        }

        // The "nasty" force-kill, a trigger blast that kills everything inside.
        case MSG_CHEAT_NASTY:
        {
            ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
            CSWSTrigger* pTrigger = new CSWSTrigger(OBJECT_INVALID);
            Vector vTriggerPos = pCreature->m_position - pCreature->m_orientation * 2.5f;
            pTrigger->CreateNewGeometry(2.5f, vTriggerPos, static_cast<CSWSArea*>(pCreature->GetArea()));

            CSWSPlayer* pClient = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                pCreature->m_objectId);
            if (pClient != 0)
            {
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCheatNasty(
                    pClient->m_nPlayerId, pCreature->m_objectId);
            }

            unsigned long long nObjId;
            if (pArea->GetFirstObjectInArea(nObjId) != 0)
            {
                do
                {
                    CGameObject* pScan = 0;
                    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                            nObjId, &pScan) == CGameObjectArray::SUCCESS)
                    {
                        CSWSCreature* pScanCreature = pScan->AsSWSCreature();
                        if (pScanCreature != 0 && pScan->m_objectId != pCreature->m_objectId &&
                            pTrigger->InTrigger(pScan->AsSWSCreature()->m_position) != 0)
                        {
                            CGameEffect* pKill = new CGameEffect(1);
                            pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
                            pKill->m_durationTypeBits = (pKill->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                                        CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                            pKill->SetNumIntegers(21);
                            pKill->SetInteger(0, 10);
                            pKill->SetInteger(16, 1000);
                            pKill->SetObjectID(0, pCreature->m_objectId);
                            pScanCreature->ApplyEffect(pKill, 0, 0);

                            pClient = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                                pScanCreature->m_objectId);
                            if (pClient != 0)
                            {
                                g_pAppManager->m_pServerExoApp->GetSWSMessage()->
                                    SendServerToPlayerCheatNasty(pClient->m_nPlayerId,
                                                                 pCreature->m_objectId);
                            }
                        }
                    }
                } while (pArea->GetNextObjectInArea(nObjId) != 0);
            }

            pCreature->SetAnimation(SERVER_ANIMATION_KNOCKDOWN_FROM_BEHIND);
            delete pTrigger;
            return 0;
        }

        // The "force-blast" around the player.
        case MSG_CHEAT_RAIN_OF_COWS:
        {
            ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
            unsigned long long nObjId;
            if (pArea->GetFirstObjectInArea(nObjId) == 0)
                return 0;

            Vector vBlastPos = pCreature->m_position + pCreature->m_orientation * 4.0f;
            CGameEffect* pBlast = new CGameEffect(1);
            pBlast->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pBlast->m_durationTypeBits = (pBlast->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                         CGameEffect::DURATION_TYPE_INSTANTANEOUS;
            pBlast->SetCreator(pCreature->m_objectId);
            pBlast->SetInteger(0, 32);
            pArea->ApplyEffect(pBlast, vBlastPos, pCreature->m_orientation);

            pCreature->SetAnimation(SERVER_ANIMATION_CAST_1);

            do
            {
                CSWSCreature* pOther = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                    nObjId);
                if (pOther == 0)
                    continue;
                if (pOther->m_objectId == pCreature->m_objectId)
                    continue;
                for (int i = 0; i < rand_wincompatible() % 3; i++)
                {
                    CGameEffect* pKill = new CGameEffect(1);
                    pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
                    pKill->m_durationTypeBits = (pKill->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                                CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                    pKill->SetCreator(pCreature->m_objectId);

                    unsigned int nDelay = rand_wincompatible() % 2300 + 3000;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, nDelay, pCreature->m_objectId, pOther->m_objectId,
                        CServerAIMaster::EVENT_APPLY_EFFECT, pKill);

                    Vector vDelta = pOther->m_position - pCreature->m_position;
                    float fDist = magnitude(vDelta);
                    nDelay += static_cast<unsigned int>((fDist / (logf(fDist) * 3.0f + 2.0f)) * 1000.0f);

                    pKill = new CGameEffect(1);
                    pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
                    pKill->m_durationTypeBits = (pKill->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                                CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                    pKill->SetCreator(pCreature->m_objectId);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, nDelay, pCreature->m_objectId, pOther->m_objectId,
                        CServerAIMaster::EVENT_APPLY_EFFECT, pKill);

                    pKill = new CGameEffect(1);
                    pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
                    pKill->m_durationTypeBits = (pKill->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                                CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                    pKill->SetCreator(pCreature->m_objectId);
                    pKill->SetInteger(0, 61);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, nDelay, pCreature->m_objectId, pOther->m_objectId,
                        CServerAIMaster::EVENT_APPLY_EFFECT, pKill);

                    pKill = new CGameEffect(1);
                    pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
                    pKill->m_durationTypeBits = (pKill->m_durationTypeBits & ~kEffectDurationTypeMask) |
                                                CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                    pKill->SetCreator(pCreature->m_objectId);
                    pKill->SetNumIntegers(21);
                    pKill->SetInteger(0, 50);
                    pKill->SetInteger(1, -1);
                    pKill->SetInteger(2, -1);
                    pKill->SetInteger(3, -1);
                    pKill->SetInteger(4, -1);
                    pKill->SetInteger(5, -1);
                    pKill->SetInteger(6, -1);
                    pKill->SetInteger(7, -1);
                    pKill->SetInteger(8, -1);
                    pKill->SetInteger(9, -1);
                    pKill->SetInteger(10, -1);
                    pKill->SetInteger(11, -1);
                    pKill->SetInteger(12, -1);
                    pKill->SetInteger(16, 1000);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, nDelay, pCreature->m_objectId, pOther->m_objectId,
                        CServerAIMaster::EVENT_APPLY_EFFECT, pKill);
                }
            } while (pArea->GetNextObjectInArea(nObjId) != 0);
            return 0;
        }

        // Spawn the "goon" debug creature.
        case MSG_CHEAT_SUMMON_NASTY:
        {
            CResRef cTemplate;
            ReadOBJECTIDServer();                  // discarded
            unsigned char nHead = ReadBYTE(8);
            cTemplate = ReadCResRef(16);

            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSCreature* pGoon = new CSWSCreature(OBJECT_INVALID, 0);
            pGoon->m_appearance.m_headId = nHead;
            pGoon->m_appearance.m_nPhenotype = 0;
            pGoon->m_appearance.m_nGender = 0;

            CExoLocString sName;
            sName.AddString(0, CExoString("Hostile"), 0);
            pGoon->m_pStats->m_firstName = sName;
            sName.AddString(0, CExoString("Nudist"), 0);
            pGoon->m_pStats->m_lastName = sName;

            CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
            if (pArea != 0)
            {
                CSWSEncounter* pEncounter = new CSWSEncounter(OBJECT_INVALID);
                pEncounter->AddToArea(pArea, pEncounter->m_position.x, pEncounter->m_position.y,
                                      pEncounter->m_position.z, 1);

                CSWSObject* pG = pPlayer->GetGameObject();
                Vector vPos(pG->m_position.x, pG->m_position.y + 15.0f, pG->m_position.z);

                CExoString sMsg;
                Vector vSpawn(0.0f, 0.0f, 0.0f);
                float fHeight = 0.0f;
                if (pArea->ComputeNonVisibleLocation(vPos, pGoon->m_pPathfindInfo, 1, &vSpawn,
                                                     30.0f) == 1)
                {
                    fHeight = pArea->ComputeHeight(vSpawn, 1, 0, 0);
                    sMsg.Format(const_cast<char*>("My spider sense tells me the goon is hiding at (%d, %d, %d)."),
                                static_cast<int>(vSpawn.x), static_cast<int>(vSpawn.y), static_cast<int>(fHeight));
                    pPlayer->GetGameObject()->AsSWSCreature()->AddAction(AIACTION_SPEAK,
                        ACTION_GROUP_NEW, ACTION_PARAM_STRING, &sMsg,
                        ACTION_PARAM_INT, const_cast<unsigned char*>(&CMessagePlayer_Chat_Talk));
                }
                else if (vSpawn.x == -1.0f && vSpawn.y == -1.0f && vSpawn.z == -1.0f)
                {
                    sMsg.Format(const_cast<char*>("There was no place to spawn the goon."));
                    pPlayer->GetGameObject()->AsSWSCreature()->AddAction(AIACTION_SPEAK,
                        ACTION_GROUP_NEW, ACTION_PARAM_STRING, &sMsg,
                        ACTION_PARAM_INT, const_cast<unsigned char*>(&CMessagePlayer_Chat_Talk));
                    return 0;
                }
                else
                {
                    fHeight = pArea->ComputeHeight(vSpawn, 1, 0, 0);
                    sMsg.Format(const_cast<char*>("The goon couldn't find a place to hide, he's at (%d, %d, %d)"),
                                static_cast<int>(vSpawn.x), static_cast<int>(vSpawn.y), static_cast<int>(fHeight));
                    pPlayer->GetGameObject()->AsSWSCreature()->AddAction(AIACTION_SPEAK,
                        ACTION_GROUP_NEW, ACTION_PARAM_STRING, &sMsg,
                        ACTION_PARAM_INT, const_cast<unsigned char*>(&CMessagePlayer_Chat_Talk));
                }

                pGoon->m_scripts[0] = CExoString("b_heart");
                pGoon->m_scripts[3] = CExoString("b_attacked");
                pGoon->m_scripts[4] = CExoString("b_damage");
                pGoon->m_scripts[10] = CExoString("b_death");
                pGoon->AddToArea(pArea, vSpawn.x, vSpawn.y, fHeight, 0, 0);

                // The item spawns only when the template resource exists.
                if (g_pExoResMan->Exists(cTemplate, RESTYPE_UTI, static_cast<unsigned int*>(0)) != 0)
                {
                    CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
                    pItem->LoadFromTemplate(cTemplate);
                    static_cast<CSWSObject*>(pItem)->SetPosition(
                        Vector(vSpawn.x, vSpawn.y, fHeight), 1, 1, 0);
                    pItem->AddToArea(pArea, vSpawn.x, vSpawn.y, fHeight, 1);
                }
                return 1;
            }
            return 0;
        }

        // Toggle the player's own flag.
        case MSG_CHEAT_TOGGLE_FLOATY_EFFECTS:
            pPlayer->m_bCheatToggle = (pPlayer->m_bCheatToggle == 0);
            return 1;

        // Level up to the next level.
        case MSG_CHEAT_LEVEL_UP:
            if (pPlayer->GetGameObject() == 0)
                return 0;
            if (pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->GetLevel(0) > 49)
                return 0;

            pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->SetExperience(
                pPlayer->GetGameObject()->AsSWSCreature()->m_pStats->GetExpNeededForLevelUp(), 1);
            return 0;

        // Movement-speed debug flag.
        case MSG_CHEAT_TOGGLE_MOVEMENT_SPEED_DEBUGGING:
        {
            int bMove = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;
            g_bEnableMovementSpeedDebugging = bMove;
            return 1;
        }

        // Set a module byte.
        case MSG_CHEAT_PLAYER_PATHFIND_RULE:
        {
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            unsigned char nByte = ReadBYTE(8);
            if (MessageReadOverflow(1) == 0 && pModule != 0)
                pModule->m_nCheatValue = nByte;
            return 0;
        }

        // Walk to the nearest safe location.
        case MSG_CHEAT_COMPUTE_SAFE_LOCATION:
        {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject == 0)
                return 0;

            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;

            CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
            if (pArea == 0)
                return 0;

            Vector vSafe;
            unsigned long long nAreaId = pArea->m_objectId;
            float fPersonalSpace = pCreature->m_pPathfindInfo->m_fPersonalSpace;
            int bFlag = 1;
            pCreature->m_pPathfindInfo->m_fPersonalSpace = 0.1f;
            pCreature->m_pPathfindInfo->ComputeStepTolerance();
            if (pArea->ComputeSafeLocation(pCreature->m_position, 10.0f,
                                           pCreature->m_pPathfindInfo, 1, &vSafe) != 0)
            {
                Vector vDelta = vSafe - pCreature->m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z > 0.01f)
                {
                    pCreature->ClearAllActions(1);
                    pCreature->AddAction(AIACTION_JUMP_TO_POINT, ACTION_GROUP_NEW,
                                         ACTION_PARAM_FLOAT, &vSafe, ACTION_PARAM_FLOAT, &vSafe.y,
                                         ACTION_PARAM_FLOAT, &vSafe.z,
                                         ACTION_PARAM_OBJECT, &nAreaId, ACTION_PARAM_INT, &bFlag);
                }
            }
            pCreature->m_pPathfindInfo->m_fPersonalSpace = fPersonalSpace;
            pCreature->m_pPathfindInfo->ComputeStepTolerance();
            return 0;
        }

        default:
            return 0;
    }
}

// Handles the client's answer to a dialog node by forwarding the packet to the object's
// ReplyDialog.
int CSWSMessage::HandlePlayerToServerDialogMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    int bOverflow = MessageReadOverflow(1);
    int nResult = 0;
    if (nType != MSG_DIALOG_REPLY || bOverflow)
        return nResult;

    unsigned long long nObjectId = ReadOBJECTIDServer();
    unsigned int nReply = ReadDWORD(32);
    int bFlag = ReadBYTE(8) != 0;
    unsigned int nSomething = ReadDWORD(32);
    if (MessageReadOverflow(1))
        return nResult;
    if (MessageReadUnderflow(1))
        return nResult;

    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nObjectId, &pObject)
            == CGameObjectArray::SUCCESS)
    {
        nResult = static_cast<CSWSObject*>(pObject)->ReplyDialog(pPlayer->m_nPlayerId, nReply, bFlag,
                                                      nSomething, 0);
    }
    return nResult;
}

// Per-object appearance, stats and placement edits from the client, dispatched on a letter byte
// ('A'..'T'); unknown letters fall through to the shared exit.
int CSWSMessage::HandlePlayerToServerGameObjectUpdate(CSWSPlayer* pPlayer, unsigned char nSub)
{
    int nResult = 0;
    int bOverflow = MessageReadOverflow(1);
    if (nSub != MSG_GAME_OBJECT_UPDATE_UPDATE || bOverflow)
        return nResult;

    CResRef resRef;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CNetLayer* pNetLayer = g_pAppManager->m_pServerExoApp->GetNetLayer();
    if (pModule == 0 || pObjectArray == 0 || pNetLayer == 0)
    {
        ClearReadMessage();
        return nResult;
    }

    char nKind = ReadCHAR(8);
    if (MessageReadOverflow(1))
        return nResult;

    switch (nKind)
    {
    case 'A':
    {
        ReadOBJECTIDServer();
        float fX = ReadFLOAT(1.0f, 32);
        float fY = ReadFLOAT(1.0f, 32);
        float fZ = ReadFLOAT(1.0f, 32);
        unsigned char cFlag = ReadBYTE(8);
        ReadBYTE(8);
        ReadBYTE(8);
        resRef = ReadCResRef(16);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        if (g_pExoResMan->Exists(resRef, static_cast<unsigned short>(RESTYPE_UTI), static_cast<unsigned int*>(0)))
        {
            CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
            pItem->LoadFromTemplate(resRef);
            if (cFlag != 0)
                pItem->m_nModelVariation = cFlag;
            static_cast<CSWSObject*>(pItem)->SetPosition(Vector(fX, fY, fZ), 1, 1, 0);
            pItem->AddToArea(pModule->GetArea(), fX, fY, fZ, 1);
        }
        return 1;
    }
    case 'C':
    {
        unsigned long long nObjectId = ReadOBJECTIDServer();
        float fX = ReadFLOAT(1.0f, 32);
        float fY = ReadFLOAT(1.0f, 32);
        float fZ = ReadFLOAT(1.0f, 32);
        CExoString sParam = ReadCExoString(32);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
        {
            nResult = 0;
            break;
        }
        if (pPlayer->GetGameObject() != 0)
            pPlayer->GetGameObject();
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SUMMON_CREATURE);
        pEffect->m_durationTypeBits =
            (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
        pEffect->SetObjectID(0, nObjectId);
        pEffect->SetFloat(0, fX);
        pEffect->SetFloat(1, fY);
        pEffect->SetFloat(2, fZ);
        pEffect->SetString(0, sParam);
        pPlayer->GetGameObject()->ApplyEffect(pEffect, 0, 0);
        nResult = 1;
        break;
    }
    case 'D':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->Delete(nObjectId, &pObject);
        if (pObject != 0)
        {
            CSWSItem* pItem = pObject->AsSWSItem();
            if (pItem != 0)
                delete pItem;
            return 1;
        }
        return 0;
    }
    case 'G':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        unsigned char nByte = ReadBYTE(8);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        pCreature->m_appearance.m_nGender = nByte;
        return 1;
    }
    case 'H':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        unsigned char nByte = ReadBYTE(8);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        pCreature->m_appearance.m_nAppearanceHead = nByte;
        return 1;
    }
    case 'L':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        unsigned char nIndex = ReadBYTE(8);
        unsigned char nByte = ReadBYTE(8);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        nResult = 1;
        switch (nIndex)
        {
        case 0:
            pCreature->m_appearance.m_nColorSkin = nByte;
            break;
        case 1:
            pCreature->m_appearance.m_nColorHair = nByte;
            break;
        case 6:
            pCreature->m_appearance.m_nColorTattoo1 = nByte;
            break;
        case 7:
            pCreature->m_appearance.m_nColorTattoo2 = nByte;
            break;
        }
        break;
    }
    case 'M':
    {
        unsigned long long nObjectId = ReadOBJECTIDServer();
        float fX = ReadFLOAT(1.0f, 32);
        float fY = ReadFLOAT(1.0f, 32);
        float fZ = ReadFLOAT(1.0f, 32);
        CExoString sParam = ReadCExoString(32);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
        {
            nResult = 0;
            break;
        }
        if (pPlayer->GetGameObject() != 0)
            pPlayer->GetGameObject();
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SUMMON_PARTY);
        pEffect->m_durationTypeBits =
            (pEffect->m_durationTypeBits & ~kEffectDurationTypeMask) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
        pEffect->SetObjectID(0, nObjectId);
        pEffect->SetFloat(0, fX);
        pEffect->SetFloat(1, fY);
        pEffect->SetFloat(2, fZ);
        pEffect->SetString(0, sParam);
        pPlayer->GetGameObject()->ApplyEffect(pEffect, 0, 0);
        nResult = 1;
        break;
    }
    case 'P':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        if (MessageReadOverflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
        {
            ClearReadMessage();
            return 0;
        }
        unsigned int nHeadId = ReadWORD(16);
        unsigned char nA = ReadBYTE(8);
        unsigned char nB = ReadBYTE(8);
        unsigned char nC = ReadBYTE(8);
        unsigned char nD = ReadBYTE(8);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pCreature->m_appearance.m_headId = static_cast<unsigned short>(nHeadId);
        pCreature->m_appearance.m_nPhenotype = nA;
        pCreature->m_appearance.m_nGender = nB;
        pCreature->m_appearance.m_nColorSkin = nC;
        pCreature->m_appearance.m_nColorHair = nD;
        pCreature->UpdatePersonalSpace();
        return 1;
    }
    case 'R':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        unsigned int nAppearance = ReadWORD(16);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        pCreature->m_appearance.m_headId = static_cast<unsigned short>(nAppearance);
        pCreature->m_pStats->m_nAppearanceType = static_cast<unsigned short>(nAppearance);
        pCreature->m_pStats->SetMovementRate(7);
        float fValue;
        if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                static_cast<int>(nAppearance), CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE, &fValue) != 0)
        {
            pCreature->m_pPathfindInfo->m_fPersonalSpace = fValue;
            pCreature->m_pPathfindInfo->ComputeStepTolerance();
        }
        return 1;
    }
    case 'T':
    {
        CGameObject* pObject;
        unsigned long long nObjectId = ReadOBJECTIDServer();
        unsigned char nByte = ReadBYTE(8);
        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;
        pObjectArray->GetGameObject(nObjectId, &pObject);
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        pCreature->m_appearance.m_nPhenotype = nByte;
        return 1;
    }
    default:
        return nResult;
    }

    return nResult;
}

// Client open or close of its own inventory screen.
int CSWSMessage::HandlePlayerToServerGuiInventoryMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    unsigned char bOpen = ReadBYTE(8);

    if (MessageReadOverflow(1) || MessageReadUnderflow(1))
        return 0;

    if (nType == MSG_GUI_INVENTORY_STATUS)
        pPlayer->m_pInventoryGUI->SetOpen(bOpen != 0);

    return 1;
}

// Client closing a container; the id resolves the placeable or item to dispatch CloseInventory on.
int CSWSMessage::HandlePlayerToServerGuiContainerMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (nType == MSG_GUI_CONTAINER_CLOSE)
    {
        unsigned long long objectId = ReadOBJECTIDServer();
        int bFlag = ReadBOOL();

        if (MessageReadOverflow(1) || MessageReadUnderflow(1))
            return 0;

        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pObject)
                == CGameObjectArray::SUCCESS)
        {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            {
                CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
                pPlaceable->CloseInventory(pPlayer->GetGameObject()->m_objectId, bFlag);
            }
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
            {
                CSWSItem* pItem = pObject->AsSWSItem();
                pItem->CloseInventory(pPlayer->GetGameObject()->m_objectId, bFlag);
            }
        }
    }

    return 1;
}

// Client request to (re)open its character sheet, permission-gated to the player's own creature.
int CSWSMessage::HandlePlayerToServerCharacterSheetMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    unsigned char nAction = ReadCHAR(8);
    unsigned long long objectId = ReadOBJECTIDServer();
    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;
    if (nType != MSG_GUI_CHARACTER_SHEET_STATUS)
        return 1;

    if (!pPlayer->PermittedToDisplayCharacterSheet(objectId))
    {
        SendServerToPlayerGUICharacterSheet_NotPermitted(pPlayer->m_nPlayerId, objectId);
        nAction = 0xff;
        objectId = OBJECT_INVALID;
    }

    CSWSCreature* pOldCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
        pPlayer->m_pCharSheetGUI->m_objectId);
    if (pOldCreature)
        pOldCreature->m_nCharSheetViewers -= 1;

    CSWSCreature* pNewCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pNewCreature)
        pNewCreature->m_nCharSheetViewers += 1;

    pPlayer->m_pCharSheetGUI->SetCreatureDisplayed(objectId);
    pPlayer->m_pCharSheetGUI->m_nAction = nAction;
    if (nAction == 0xff)
        pPlayer->m_pCharSheetGUI->m_pStatsUpdate->ClearEffectIcons();
    return 1;
}

// Handles a client drive command by resetting combat state and queuing a drive action.
int CSWSMessage::HandlePlayerToServerInputDriveControl(CSWSPlayer* pPlayer)
{
    float fX = ReadFLOAT(1.0f, 32);
    float fY = ReadFLOAT(1.0f, 32);
    unsigned short nHeading = ReadWORD(12);
    unsigned char bFlag1 = ReadBYTE(8);
    unsigned char bFlag2 = ReadBYTE(8);
    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;
    CGameObject* pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;
    Vector vPosition;
    vPosition.x = fX;
    vPosition.y = fY;
    vPosition.z = 0.0f;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 1;
#ifdef K2_HOST_BUILD
    {
        static int s_nLastWhy = -1;
        int nWhy = pCreature->m_inTransit != 0 ? 1 : pCreature->m_bCommandable != 1 ? 2 : 0;
        if (nWhy != s_nLastWhy)
            fprintf(stderr, "k2: drive control %s (commandable %d, in transit %d, actions %d)\n",
                    nWhy == 1 ? "refused: in transit" : nWhy == 2 ? "refused: not commandable" : "accepted",
                    static_cast<int>(pCreature->m_bCommandable), static_cast<int>(pCreature->m_inTransit),
                    pCreature->m_actionList.m_pInternal
                        ? static_cast<int>(pCreature->m_actionList.m_pInternal->m_nCount) : -1);
        s_nLastWhy = nWhy;
    }
#endif
    if (pCreature->m_inTransit != 0)
        return 0;
    if (pCreature->m_bCommandable != 1)
        return 1;
    pCreature->SetCombatMode(CSWSCreature::CREATURE_COMBAT_MODE_NONE, 1);
    pCreature->m_pCombatRound->ClearAllSpecialAttacks();
    pCreature->ClearAllActions(1);
    pCreature->ActionManager(8);
    pCreature->AddDriveAction(ACTION_GROUP_NEW, vPosition, nHeading, bFlag1, bFlag2, 2);
    return 1;
}

// Like the cancel handler below, but it clears only the drive-control action at the head of the
// creature's queue.
int CSWSMessage::HandlePlayerToServerInputAbortDriveControl(CSWSPlayer* pPlayer)
{
    if (MessageReadOverflow(1))
        return 0;

    if (MessageReadUnderflow(1))
        return 0;

    CGameObject* pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0 && pCreature->m_bCommandable == 1)
    {
        CExoLinkedListNode* pHead = pCreature->m_actionList.m_pInternal->m_pHead;
        if (pHead != 0)
        {
            CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(pHead->m_pData);
            if (pAction != 0 && pAction->m_nActionId == AIACTION_DRIVE)
                pAction->m_args.m_bCheckMoveToRun = 0;
        }
    }

    return 1;
}

int CSWSMessage::HandlePlayerToServerInputCancelGuiTimingEvent(CSWSPlayer* pPlayer)
{
    CGameObject* pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0 && pCreature->m_bCommandable == 1)
        pCreature->ClearAllActions(1);

    return 1;
}

// Handles a walk-to-waypoint drive command. It reads a waypoint id, a point, a client path id and
// flags, then walks the creature along its pathfind info.
int CSWSMessage::HandlePlayerToServerInputWalkToWaypoint(CSWSPlayer* pPlayer)
{
    int nResult = 0;

    unsigned long long nWaypointId = ReadOBJECTIDServer();
    float fX = ReadFLOAT(1.0f, 32);
    float fY = ReadFLOAT(1.0f, 32);
    float fZ = ReadFLOAT(1.0f, 32);
    int nClientPathId = ReadBYTE(8);
    int bFlag1 = ReadBOOL();
    int bFlag2 = ReadBOOL();
    unsigned char nGoalWithinReach = ReadBYTE(8);
    unsigned long long nTargetId = ReadOBJECTIDServer();

    if (MessageReadOverflow(1))
        return nResult;
    if (MessageReadUnderflow(1))
        return nResult;

    CSWSObject* pObject = pPlayer->GetGameObject();
    CGameObject* pSecond = pPlayer->GetGameObject();
    if (pObject == 0 || pSecond == 0)
        return nResult;
    if (pObject->GetArea() == 0)
        return nResult;

    CSWSCreature* pWaypointCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pWaypointCreature != 0)
    {
        if (pPlayer->GetGameObject()->AsSWSCreature()->m_inTransit != 0)
            return nResult;

        CPathfindInformation* pPathfind = pPlayer->GetGameObject()->AsSWSCreature()->m_pPathfindInfo;
        // The active path segment's endpoints.
        float* pX;
        float* pY;
        float* pZ;
        unsigned long long* pAreaId;
        if (pPathfind->m_bPointsReversed != 0)
        {
            pX = &pPathfind->m_vStartPoint.x;
            pY = &pPathfind->m_vStartPoint.y;
            pZ = &pPathfind->m_vStartPoint.z;
            pAreaId = &pPathfind->m_oidStartArea;
        }
        else
        {
            pX = &pPathfind->m_goal.m_vPoint.x;
            pY = &pPathfind->m_goal.m_vPoint.y;
            pZ = &pPathfind->m_goal.m_vPoint.z;
            pAreaId = &pPathfind->m_oidGoalArea;
        }

        if (*pAreaId == nWaypointId)
        {
            Vector vDelta = Vector(fX, fY, fZ);
            vDelta.x -= *pX;
            vDelta.y -= *pY;
            vDelta.z -= *pZ;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z < 0.01f)
            {
                pPathfind->m_nClientPathId = nClientPathId;
                return 1;
            }
        }

        pPathfind->ResetAtEndOfPath();
        if (nGoalWithinReach == 1)
            pPathfind->m_bGoalWithinReach = nGoalWithinReach;
    }

    CSWSCreature* pCombat = pObject->AsSWSCreature();
    if (pCombat != 0)
    {
        CSWSCreature* pNear = pObject->AsSWSCreature();
        Vector vDeltaNear = pNear->m_position - Vector(fX, fY, fZ);
        if (vDeltaNear.x * vDeltaNear.x + vDeltaNear.y * vDeltaNear.y + vDeltaNear.z * vDeltaNear.z < 2.25f
            && pNear->m_nAIStateAction == 2)
        {
            unsigned long long nCombatTargetId = static_cast<unsigned int>(pNear->m_oidAIStateTarget);
            CGameObject* pTargetObject = g_pAppManager->m_pServerExoApp->GetGameObject(nCombatTargetId);
            if (pTargetObject == 0)
                return 1;
            if (pTargetObject->AsSWSObject() == 0)
                return 1;
            CSWSObject* pTarget = pTargetObject->AsSWSObject();
            if (pTarget == 0)
                return 1;

            pNear->m_pCombatRound->SetRoundPaused(1, pNear->m_objectId);
            pNear->m_pCombatRound->SetPauseTimer(500, 0);

            Vector vTargetDir = normalize(pTarget->m_position - pNear->m_position);
            Vector vPointDir = normalize(Vector(fX, fY, fZ) - pNear->m_position);
            float fDot = vTargetDir.x * vPointDir.x + vTargetDir.y * vPointDir.y + vTargetDir.z * vTargetDir.z;

            unsigned char nStep = 5;
            if (fDot >= 0.707f)
                nStep = 2;
            else if (fDot <= -0.707f)
                nStep = 3;
            else if (vTargetDir.x * vPointDir.y - vTargetDir.y * vPointDir.x >= 0.0f)
                nStep = 4;

            pNear->DoCombatStep(nStep, 500, nCombatTargetId);
            return 1;
        }
    }

    CSWSCreature* pCommandable = pObject->AsSWSCreature();
    if (pCommandable != 0 && pCommandable->m_bCommandable == 1)
    {
        pCommandable->ComputeAIState();
        unsigned char nAction = pCommandable->m_nAIStateAction;
        pObject->AsSWSCreature()->SetCombatMode(CSWSCreature::CREATURE_COMBAT_MODE_NONE, 1);
        pObject->AsSWSCreature()->m_pCombatRound->ClearAllSpecialAttacks();
        pObject->AsSWSCreature()->ActionManager(8);
        pObject->AsSWSCreature()->AddMoveToPointAction(
            (nAction != 1) ? static_cast<unsigned short>(ACTION_GROUP_NEW) : static_cast<unsigned short>(ACTION_GROUP_PREVIOUS),
            Vector(fX, fY, fZ),
            nWaypointId,
            nTargetId,
            bFlag1,
            0.0f,
            0.0f,
            bFlag2,
            nClientPathId,
            0,
            nGoalWithinReach,
            0,
            0,
            Vector(),
            0,
            0,
            0);
    }
    return 1;
}

// Walks the party lead to the point, as the group twin of the walk-to-waypoint handler; every value
// arrives as a parameter instead of being read off the wire.
int CSWSMessage::HandlePlayerToServerGroupInputWalkToWaypoint(CSWSPlayer* pPlayer,
    CSWSCreature* pCreature, unsigned long long nId, float fX, float fY, float fZ,
    unsigned char bFlag, int a, int b, unsigned long long c)
{
    K2_UNUSED(pPlayer);
    float fPending;
    if (pCreature != 0)
    {
        Vector vPoint(fX, fY, fZ);
        CPathfindInformation* pPathfind = pCreature->m_pPathfindInfo;
        // The active path segment's endpoints.
        float* pX;
        float* pY;
        float* pZ;
        unsigned long long* pAreaId;
        if (pPathfind->m_bPointsReversed != 0)
        {
            pX = &pPathfind->m_vStartPoint.x;
            pY = &pPathfind->m_vStartPoint.y;
            pZ = &pPathfind->m_vStartPoint.z;
            pAreaId = &pPathfind->m_oidStartArea;
        }
        else
        {
            pX = &pPathfind->m_goal.m_vPoint.x;
            pY = &pPathfind->m_goal.m_vPoint.y;
            pZ = &pPathfind->m_goal.m_vPoint.z;
            pAreaId = &pPathfind->m_oidGoalArea;
        }

        if (*pAreaId == nId)
        {
            Vector vDelta = vPoint;
            vDelta.x -= *pX;
            vDelta.y -= *pY;
            vDelta.z -= *pZ;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z < 0.01f)
                pPathfind->m_nClientPathId = bFlag;
        }

        fPending = 2.0f;

        Vector vDeltaNear = pCreature->m_position - vPoint;
        if (vDeltaNear.x * vDeltaNear.x + vDeltaNear.y * vDeltaNear.y + vDeltaNear.z * vDeltaNear.z
                < 2.25f
            && pCreature->m_nAIStateAction == 2)
        {
            unsigned long long nCombatTargetId = static_cast<unsigned int>(pCreature->m_oidAIStateTarget);
            CGameObject* pTargetObject = g_pAppManager->m_pServerExoApp->GetGameObject(nCombatTargetId);
            if (pTargetObject == 0)
                return 1;
            if (pTargetObject->AsSWSObject() == 0)
                return 1;
            CSWSObject* pTarget = pTargetObject->AsSWSObject();
            if (pTarget == 0)
                return 1;

            pCreature->m_pCombatRound->SetRoundPaused(1, pCreature->m_objectId);
            pCreature->m_pCombatRound->SetPauseTimer(500, 0);

            // Engine bug: the z term is vTargetDir.z * vTargetDir.z, not vPointDir.z.
            Vector vTargetDir = normalize(pTarget->m_position - pCreature->m_position);
            Vector vPointDir = normalize(vPoint - pCreature->m_position);
            float fDot = vTargetDir.x * vPointDir.x + vTargetDir.y * vPointDir.y
                       + vTargetDir.z * vTargetDir.z;

            unsigned char nStep = 5;
            if (fDot >= 0.707f)
                nStep = 2;
            else if (fDot <= -0.707f)
                nStep = 3;
            else if (vTargetDir.x * vPointDir.y - vTargetDir.y * vPointDir.x >= 0.0f)
                nStep = 4;

            pCreature->DoCombatStep(nStep, 500, nCombatTargetId);
        }
        else if (pCreature->m_bCommandable == 1)
        {
            pCreature->ClearAllActions(1);
            pCreature->ActionManager(8);
            pCreature->AddMoveToPointAction(ACTION_GROUP_NEW, Vector(fX, fY, fZ), nId, c, a,
                                            2.0f, 0.0f, b, static_cast<int>(bFlag),
                                            0, 0, 0, 0, Vector(), 0, 0, 0);
            pCreature->m_pPathfindInfo->ResetAtEndOfPath();
            pCreature->AddAction(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, ACTION_GROUP_NEW,
                                 ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY,
                                 ACTION_PARAM_FLOAT, &fZ, ACTION_PARAM_OBJECT, &nId,
                                 ACTION_PARAM_OBJECT, &c, ACTION_PARAM_INT, &a,
                                 ACTION_PARAM_FLOAT, &fPending);
        }
    }
    else
    {
        fPending = 2.0f;
    }
    return 1;
}

// Client gold transfer; when the target is the "self" sentinel the drop position follows the
// amount. Note the creature may be uninitialized on the failure path, and TransferGold is called
// regardless.
int CSWSMessage::HandlePlayerToServerGoldMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    Vector vPosition;
#ifdef K2_FIX_ENGINE_BUGS
    CSWSCreature* pCreature = 0;
#else
    CSWSCreature* pCreature;
#endif
    CGameObject* pGameObject = pPlayer->GetGameObject();
    if (pGameObject != 0)
    {
        if (pGameObject->AsSWSCreature() != 0)
            pCreature = pGameObject->AsSWSCreature();
    }

    int bOverflow = MessageReadOverflow(1);
    if (nType != MSG_GOLD_TRANSFER || bOverflow)
        return 0;

    unsigned long long nObjectId = ReadOBJECTIDServer();
    int nGold = ReadINT(32);
    if (MessageReadOverflow(1))
        return 0;

    if (nObjectId == OBJECT_INVALID)
    {
        vPosition.x = ReadFLOAT(1.0f, 32);
        vPosition.y = ReadFLOAT(1.0f, 32);
        vPosition.z = ReadFLOAT(1.0f, 32);
    }
    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadOverflow(1))
        return 0;
#ifdef K2_FIX_ENGINE_BUGS
    if (pCreature == 0)
        return 0;
#endif

    pCreature->TransferGold(nObjectId, vPosition, nGold);
    return 0;
}

int CSWSMessage::HandlePlayerToServerInputMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    // volatile keeps the result in memory so the drain cases below stay distinct.
    volatile int nResult = 0;

    CSWSObject* pObject = pPlayer->GetGameObject();
    CSWSCreature* pCreature = 0;
    CSWSCreatureStats* pStats = 0;
    if (pObject != 0)
    {
        CSWSCreature* pc = pObject->AsSWSCreature();
        if (pc != 0)
        {
            pStats = pc->m_pStats;
            if (pc->m_inTransit == 0)
                pCreature = pc;
        }
    }

    if (MessageReadOverflow(1))
        return nResult;

    switch (nType)
    {
        // Move to a waypoint.
        case MSG_INPUT_WALK_TO_WAYPOINT:
            HandlePlayerToServerInputWalkToWaypoint(pPlayer);
            return nResult;

        // Attack.
        case MSG_INPUT_ATTACK:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();
            unsigned short nFeatId = ReadWORD(16);

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

#if KOTOR1
            // K1: no tagged-object choke point and no party-wide aggro relay
            pCreature->AddAttackActions(oidTarget, nFeatId, 1, 0, 0, 0, 0, 1, SERVER_ANIMATION_ATTACK, 1500,
                                        1, 0, 0, 4, 0);
#else
            // At the "darthnihilus" choke point, while that tagged object's local boolean is set,
            // the player refuses to attack anyone else. The target object is re-fetched through
            // GetGameObject before its variable table is consulted.
            unsigned long long oidTagged =
                g_pAppManager->m_pServerExoApp->GetModule()->FindObjectByTagOrdinal(
                    CExoString("darthnihilus"), 0);
            if (oidTagged != OBJECT_INVALID && oidTarget == oidTagged)
            {
                CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
                if (pTarget != 0 && static_cast<CSWSObject*>(pTarget)->m_varTable.GetLocalBoolean(52) != 0)
                {
                    return nResult;
                }
            }

            pCreature->AddAttackActions(oidTarget, nFeatId, 1, 0, 0, 0, 0, 1, SERVER_ANIMATION_ATTACK, 1500,
                                        1, 0, 0, 4, 0);

            // Relay aggro party-wide. When not in solo mode with 2+ members and the target is
            // itself a creature, every other member with no current attack target
            // (m_attemptedAttackTargetId is the invalid-object sentinel) is told to jump in.
            if (pCreature->m_bInParty != 0)
            {
                CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
                int bIsCreature = 0;
                if (pTarget != 0 && pTarget->AsSWSCreature() != 0)
                    bIsCreature = 1;

                int nMembers = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
                CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
                if (bIsCreature != 0 && pPartyTable->m_soloMode == 0 && nMembers > 1)
                {
                    for (int i = 1; i < nMembers; ++i)
                    {
                        CSWCCreature* pClient =
                            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                        CSWSCreature* pPartyC = pClient->GetServerCreature();
                        if (pPartyC->m_attemptedAttackTargetId == OBJECT_INVALID)
                        {
                            pPartyC->AddAttackActions(oidTarget, 0, 0, 0, 1, 1, 1, 1, SERVER_ANIMATION_ATTACK,
                                                      1500, 1, 0, 0, 4, 0);
                            pPartyC->m_oidPendingAttackTarget = oidTarget;
                        }
                    }
                }
            }

#endif
            if (pCreature->m_attemptedAttackTargetId != oidTarget)
                pCreature->m_oidPendingAttackTarget = oidTarget;
            return 1;
        }

        // Open or close a door.
        case MSG_INPUT_CHANGE_DOOR_STATE:
        {
            unsigned long long oidDoor = ReadOBJECTIDServer();
            unsigned short nActionId = ReadWORD(16);

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            pCreature->ActionManager(2);
            if (nActionId == SERVER_ANIMATION_OPEN)
                pCreature->AddOpenDoorAction(oidDoor, 0);
            else
                pCreature->AddCloseDoorAction(oidDoor, 0);
            return 1;
        }

        // Move.
        case MSG_INPUT_PLAY_ANIMATION:
        {
            int nSpeed = ReadWORD(16);
            unsigned long long oidTarget = ReadOBJECTIDServer();
            float fOne = 1.0f;
            Vector vPos(ReadFLOAT(1.0f, 32), ReadFLOAT(1.0f, 32),
                        ReadFLOAT(1.0f, 32));
            int nZero = 0;
            int nOne = 1;

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            if (pCreature->m_bCommandable == 0)
                return 0;

            // While the "window closed / HUD suppressed" flag (activity bit 3, the idle-action bit)
            // is set, the current action chain is dropped before queuing the move.
            if ((pCreature->m_activityFlags & 8) != 0)
                pCreature->ClearAllActions(1);

            if (pCreature->m_objectId == oidTarget)
            {
                nResult = 1;
                pCreature->AddActionToFront(AIACTION_PLAY_ANIMATION, ACTION_GROUP_NEW,
                                            ACTION_PARAM_INT, &nSpeed, ACTION_PARAM_FLOAT, &fOne,
                                            ACTION_PARAM_FLOAT, &nZero, ACTION_PARAM_INT, &nOne, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                return nResult;
            }

            if (oidTarget == OBJECT_INVALID)
            {
                pCreature->AddAction(AIACTION_CHANGE_FACING_POINT, ACTION_GROUP_NEW,
                                     ACTION_PARAM_FLOAT, &vPos.x, ACTION_PARAM_FLOAT, &vPos.y,
                                     ACTION_PARAM_FLOAT, &vPos.z, 0);
            }
            else
            {
                pCreature->AddAction(AIACTION_CHANGE_FACING_OBJECT, ACTION_GROUP_NEW,
                                     ACTION_PARAM_OBJECT, &oidTarget);
            }

            // Re-stamp the movement rate onto the just-queued move for every group.
            pCreature->AddAction(AIACTION_PLAY_ANIMATION, ACTION_GROUP_PREVIOUS,
                                 ACTION_PARAM_INT, &nSpeed, ACTION_PARAM_FLOAT, &fOne,
                                 ACTION_PARAM_FLOAT, &nZero, ACTION_PARAM_INT, &nOne, 0);
            return 1;
        }

        // Examine.
        case MSG_INPUT_EXAMINE:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            CGameObject* pObj = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget,
                                                                                &pObj) !=
                CGameObjectArray::SUCCESS)
            {
                return 1;
            }

            switch (pObj->m_objectType)
            {
                case GAME_OBJECT_TYPE_CREATURE:
                    nResult = SendServerToPlayerExamineGui_CreatureData(pPlayer, oidTarget);
                    break;
                case GAME_OBJECT_TYPE_ITEM:
                    pCreature->UseLoreOnItem(oidTarget);
                    nResult = SendServerToPlayerExamineGui_ItemData(pPlayer, oidTarget);
                    break;
                case GAME_OBJECT_TYPE_TRIGGER:
                {
                    Vector vPos(0.0f, 0.0f, 0.0f);
                    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
                    pCreature->UseSkill(SKILL_DEMOLITIONS, SUBSKILL_EXAMINETRAP, oidTarget, vPos,
                                        pArea->m_objectId,
                                        OBJECT_INVALID, 0);
                    nResult = 1;
                    break;
                }
                default:
                {
                    CExoString sMsg(
                        "Feature not implemented: Examine unhandled for this object type.");
                    SendServerToPlayerChatMessage(5, OBJECT_INVALID, sMsg,
                                                  pPlayer->m_nPlayerId, CExoString(""));
                    nResult = 1;
                    break;
                }
                case GAME_OBJECT_TYPE_PLACEABLE:
                    nResult = SendServerToPlayerExamineGui_PlaceableData(pPlayer, oidTarget);
                    break;
                case GAME_OBJECT_TYPE_DOOR:
                    nResult = SendServerToPlayerExamineGui_DoorData(pPlayer, oidTarget);
                    break;
            }
            break;
        }

        // Use a feat.
        case MSG_INPUT_USE_FEAT:
        {
            unsigned short nFeat = ReadWORD(16);
            unsigned short nSubFeat = ReadWORD(16);
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            if (pCreature->GetArea() == 0)
                return 0;
            unsigned long long nScript =
                static_cast<CSWSArea*>(pCreature->GetArea())->m_objectId;
            pCreature->UseFeat(nFeat, nSubFeat, oidTarget, nScript);
            return 1;
        }

        // Use a skill.
        case MSG_INPUT_USE_SKILL:
        {
            unsigned char nSkill = ReadBYTE(8);
            unsigned char nSubSkill = ReadBYTE(8);
            unsigned long long oidTarget = ReadOBJECTIDServer();
            Vector vPos(ReadFLOAT(1.0f, 32), ReadFLOAT(1.0f, 32),
                        ReadFLOAT(1.0f, 32));

            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;
            if (pCreature == 0)
                return 0;

            CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
            pCreature->UseSkill(nSkill, nSubSkill, oidTarget, vPos,
                                pArea->m_objectId, OBJECT_INVALID,
                                0);
            return 0;
        }

        // Set the AI level and queue a guard action.
        case MSG_INPUT_DIALOG:
        {
            if (pCreature == 0)
                return 0;

            unsigned long long oidGuard = ReadOBJECTIDServer();
            unsigned long long oidInvalid = OBJECT_INVALID;

            pCreature->ActionManager(8);
            CExoString sStep("");
            int nZero = 0;
            int nOne = 1;
            int nZero2 = 0;

            if (pCreature->m_aiLevel == 0)
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pCreature, 1);

            pCreature->AddAction(AIACTION_DIALOG_OBJECT, ACTION_GROUP_NEW,
                                 ACTION_PARAM_OBJECT, &oidGuard, ACTION_PARAM_STRING, &sStep,
                                 ACTION_PARAM_INT, &nZero, ACTION_PARAM_INT, &nOne,
                                 ACTION_PARAM_INT, &nZero2, ACTION_PARAM_OBJECT, &oidInvalid);
            nResult = 1;
            break;
        }

        // Use an item.
        case MSG_INPUT_USE_ITEM:
        {
            Vector vTarget(0.0f, 0.0f, 0.0f);
            unsigned long long oidItem = ReadOBJECTIDServer();
            unsigned char nProp = ReadBYTE(8);
            unsigned char nInvSlot = 0;
            if (ReadBOOL())
                nInvSlot = ReadBYTE(8);
            unsigned long long oidTarget = CGameObjectArray::INVALIDOBJID;
            if (ReadBOOL() == 1)
                oidTarget = ReadOBJECTIDServer();
            if (ReadBOOL() == 1)
            {
                vTarget.x = ReadFLOAT(1.0f, 32);
                vTarget.y = ReadFLOAT(1.0f, 32);
                vTarget.z = ReadFLOAT(1.0f, 32);
            }

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
            if (pCreature->m_bCombatState == 0)
                pCreature->ActionManager(1);
            if (pCreature->UseItem(oidItem, nProp, nInvSlot, oidTarget, vTarget,
                                   pArea->m_objectId, 1) == 0)
            {
                pCreature->SendFeedbackMessage(FEEDBACK_CANNOT_USE_ITEM, 0);
            }
            return nResult;
        }

        // Toggle a creature mode.
        case MSG_INPUT_TOGGLE_MODE:
        {
            unsigned char nMode = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return nResult;

            unsigned long long oidTarget = 0;
            if (nMode == CSWSCreature::TOGGLE_MODE_COUNTERSPELL)
                oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            if (pCreature->m_bCommandable == 0)
                return 0;

            if (nMode == CSWSCreature::TOGGLE_MODE_COUNTERSPELL)
                pCreature->AddCounterSpellActions(oidTarget);
            pCreature->ToggleMode(nMode);
            return 0;
        }

        // Use an object.
        case MSG_INPUT_USE_OBJECT:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            pCreature->ActionManager(2);
            pCreature->AddUseObjectAction(oidTarget);
            return 1;
        }

        // Unlock.
        case MSG_INPUT_UNLOCK_OBJECT:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();
            unsigned long long oidLocker = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;
            if (pCreature->m_bCommandable == 0)
                return 0;

            pCreature->ActionManager(2);

            CGameObject* pObj = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget,
                                                                                &pObj) ==
                CGameObjectArray::SUCCESS)
            {
                if (pObj->AsSWSDoor() != 0 && pObj->AsSWSDoor()->m_bLocked != 0)
                    pCreature->AddUnlockObjectAction(oidTarget, oidLocker, 0);
                if (pObj->AsSWSPlaceable() != 0 && pObj->AsSWSPlaceable()->m_bLocked != 0)
                    pCreature->AddUnlockObjectAction(oidTarget, oidLocker, 0);
            }
            return 1;
        }

        // Rest.
        case MSG_INPUT_REST:
        {
            if (pCreature == 0 || pCreature->m_bCommandable == 0)
            {
                pCreature->SendFeedbackMessage(FEEDBACK_REST_NOT_COMMANDABLE, 0);
            }
            else if (pCreature->GetActivity(CSWSCreature::ACTIVITY_RESTING) == 0)
            {
                pCreature->Rest(0);
            }
            return nResult;
        }

        // Lock.
        case MSG_INPUT_LOCK_OBJECT:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;

            pCreature->ActionManager(2);

            CGameObject* pObj = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget,
                                                                                &pObj) ==
                CGameObjectArray::SUCCESS)
            {
                if (pObj->AsSWSDoor() != 0 && pObj->AsSWSDoor()->m_bLocked == 0)
                    pCreature->AddLockObjectAction(oidTarget);
                if (pObj->AsSWSPlaceable() != 0 && pObj->AsSWSPlaceable()->m_bLocked == 0)
                    pCreature->AddLockObjectAction(oidTarget);
            }
            return 1;
        }

        // Drain a legacy 4-byte acknowledge block.
        case MSG_INPUT_SPELL_MEMORIZE:
        {
            ReadBYTE(8);
            ReadDWORD(32);
            ReadBYTE(8);
            ReadBYTE(8);
            ReadBYTE(8);
            if (MessageReadOverflow(1))
                return nResult;
            MessageReadUnderflow(1);
            return 0;
        }

        // Drain a legacy 3-byte acknowledge block.
        case MSG_INPUT_SPELL_UNMEMORIZE:
        {
            ReadBYTE(8);
            ReadBYTE(8);
            ReadBYTE(8);
            if (MessageReadOverflow(1))
                return nResult;
            MessageReadUnderflow(1);
            return nResult;
        }

        // Cast a spell or force power.
        case MSG_INPUT_CAST_SPELL:
        {
            unsigned int nSpellId = ReadDWORD(32);
            unsigned char nClassIndex = ReadBYTE(8);
            unsigned char nDomainLevel = ReadBYTE(8);
            unsigned char nMetaMagic = ReadBYTE(8);
            unsigned char nFlags = ReadBYTE(8);

            unsigned long long oidTarget = OBJECT_INVALID;
            Vector vTarget(0.0f, 0.0f, 0.0f);
            int bHasLocation = 0;
            if ((nFlags & 1) != 0)
                oidTarget = ReadOBJECTIDServer();
            if ((nFlags & 2) != 0)
            {
                vTarget.x = ReadFLOAT(1.0f, 32);
                vTarget.y = ReadFLOAT(1.0f, 32);
                vTarget.z = ReadFLOAT(1.0f, 32);
                bHasLocation = 1;
            }

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pStats == 0 || pCreature == 0)
                return nResult;

            g_pRules->m_pSpellArray->GetSpell(nSpellId);
            if ((nFlags & 4) == 0 &&
                pStats->GetSpellUsesLeft(nSpellId, nClassIndex, nDomainLevel, nMetaMagic) != 0)
            {
                if (pCreature->m_bCombatState != 1)
                    pCreature->ActionManager(1);
                pCreature->AddCastSpellActions(nSpellId, nClassIndex, nDomainLevel, nMetaMagic,
                                               (nFlags >> 2) & 1, vTarget, oidTarget, 1,
                                               bHasLocation, 1, 0, 0, 0, -1, 0xff, 0);

#if !KOTOR1
                if (pCreature->m_bInParty != 0)
                {
                    CSWPartyTable* pPartyTable =
                        g_pAppManager->m_pServerExoApp->GetPartyTable();
                    if (pPartyTable != 0 && (pPartyTable->m_forfeitConditions & FORFEIT_NO_FORCE_POWERS) != 0)
                    {
                        pPartyTable->m_forfeitViolations = FORFEIT_NO_FORCE_POWERS;
                        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
                        if (pArea != 0)
                        {
                            CScriptEvent* pEvent = new CScriptEvent;
                            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
                            pEvent->SetInteger(0, USER_EVENT_FORFEIT_VIOLATED);
                            g_pAppManager->m_pServerExoApp->GetServerAIMaster()
                                ->AddEventDeltaTime(0, 0, pArea->m_objectId,
                                                    pArea->m_objectId,
                                                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                        }
                    }
                }
#endif
            }
            return nResult;
        }

        // Cancel an action.
        case MSG_INPUT_CANCEL_ACTION:
        {
            unsigned short nActionId = ReadWORD(16);
            unsigned short nGroupId = ReadWORD(16);

            nResult = 0;
            if (MessageReadOverflow(1) == 0 && pCreature != 0)
            {
                pCreature->CancelAction(nActionId, nGroupId);
                return 0;
            }
            break;
        }

        // Debug info.
        case MSG_INPUT_GET_OBJECT_DEBUG_INFO:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
            if (pObj != 0)
            {
                switch (pObj->m_objectType)
                {
                    case GAME_OBJECT_TYPE_AREA:
                        SendServerToPlayerDebugInfo_Area(pPlayer, oidTarget);
                        break;
                    case GAME_OBJECT_TYPE_CREATURE:
                        SendServerToPlayerDebugInfo_Creature(pPlayer, oidTarget);
                        break;
                    case GAME_OBJECT_TYPE_ITEM:
                        SendServerToPlayerDebugInfo_Item(pPlayer, oidTarget);
                        break;
                    case GAME_OBJECT_TYPE_PLACEABLE:
                        SendServerToPlayerDebugInfo_Placeable(pPlayer, oidTarget);
                        break;
                }
            }
            break;
        }

        // Give or merge an item.
        case MSG_INPUT_MERGE_ITEM:
        {
            unsigned long long oidItem1 = ReadOBJECTIDServer();
            unsigned long long oidItem2 = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;
            if (pCreature->m_bCommandable == 0)
                return nResult;

            CSWSItem* pItem1 = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem1);
            CSWSItem* pItem2 =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem2);
            if (pItem1 == 0 || pItem2 == 0)
                return nResult;

            // This is the "picking an item out of a container" flow. oidItem2 is the stack
            // already in our inventory that oidItem1 is trying to merge into / split from, and
            // the possessor-id plumbing below decides where the merged stack should live.
            int bCanMerge = 0;
            CSWSItem* pPossessorItem = 0;
            if (pItem2->m_possessorId == pCreature->m_objectId)
            {
                bCanMerge = 1;
            }
            else
            {
                bCanMerge = (pItem1->m_possessorId == pItem2->m_possessorId) &&
                            (pCreature->m_objectId == pItem1->m_possessorId);
                pPossessorItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                    pItem2->m_possessorId);
                if (pPossessorItem != 0 &&
                    pPossessorItem->m_possessorId == pCreature->m_objectId)
                {
                    bCanMerge = 1;
                }
            }

            unsigned long long uPlayerObj = pPlayer->m_pContainerGUI->m_objectId;
            if (uPlayerObj == pItem2->m_possessorId &&
                pItem1->m_possessorId == pCreature->m_objectId)
            {
                if (pPossessorItem == 0)
                {
                    CSWSPlaceable* pPlaceable =
                        g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(uPlayerObj);
                    if (pPlaceable != 0)
                    {
                        pPlaceable->RemoveItem(pItem2);
                        CItemRepository* pRepo = pCreature->GetItemRepository(1);
                        if (pRepo->AddItem(&pItem2, 0, 0, 1) != 0)
                        {
                            pItem2->SetPossessor(pCreature->m_objectId, 1, 0,
                                                 OBJECT_INVALID);
                            pCreature->MergeItem(pItem1, pItem2);
                            pItem1->SetPossessor(pCreature->m_objectId, 1, 0,
                                                 OBJECT_INVALID);
                            return nResult;
                        }
                        pPlaceable->m_pItemRepository->AddItem(&pItem2, 0, 0, 1);
                    }
                    if (bCanMerge == 0)
                        return nResult;
                }
                else if (bCanMerge == 0 && pPossessorItem->m_objectId == uPlayerObj)
                {
                    return nResult;
                }
            }
            else if (bCanMerge == 0)
            {
                return nResult;
            }

            pCreature->MergeItem(pItem1, pItem2);
            pItem1->SetPossessor(pCreature->m_objectId, 1, 0, OBJECT_INVALID);
            return nResult;
        }

        // Split an item.
        case MSG_INPUT_SPLIT_ITEM:
        {
            unsigned long long oidItem = ReadOBJECTIDServer();
            unsigned short nCount = ReadWORD(16);

            nResult = 0;
            if (MessageReadOverflow(1) == 0 && pCreature != 0)
            {
                if (pCreature->m_bCommandable != 0)
                {
                    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                        oidItem);
                    if (pItem != 0)
                        pCreature->SplitItem(pItem, nCount);
                }
                return 0;
            }
            break;
        }

        // Toggle pause.
        case MSG_INPUT_TOGGLE_PAUSE:
        {
            if (pCreature != 0 && pCreature->m_inTransit == 0 && pCreature->GetArea() != 0 &&
                g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bPauseAndPlay != 0)
            {
                g_pAppManager->m_pServerExoApp->TogglePauseState(2);
            }
            return nResult;
        }

        // Set the pause state.
        case MSG_INPUT_SET_PAUSE_REQUEST:
        {
            int bPause = ReadBOOL();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0 || pCreature->m_inTransit == 0)
                g_pAppManager->m_pServerExoApp->SetPauseState(2, bPause);
            return nResult;
        }

        // Input mode marker.
        case MSG_INPUT_ALWAYS_RUN:
        {
            ReadBOOL();
            if (MessageReadOverflow(1))
                return nResult;
            pPlayer->m_bInputModeReceived = 1;
            return 0;
        }

        // Orient.
        case MSG_INPUT_TURN_ON_SPOT:
        {
            Vector vDir(ReadFLOAT(1.0f, 32), ReadFLOAT(1.0f, 32), 0.0f);
            vDir.Normalize();

            if (MessageReadOverflow(1))
                return nResult;

            CSWSObject* pObj = pPlayer->GetGameObject();
            if (pObj == 0)
                return nResult;

#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
            int bDying = 0;
            if (pObj->AsSWSCreature() != 0)
                bDying = pObj->AsSWSCreature()->GetDeadTemp();
            if (pObj->m_bCommandable != 0 && pObj->GetIsPCDying() != 1 && pObj->GetDead() != 1 &&
                bDying != 1 && (pCreature == 0 || pCreature->m_inTransit == 0))
#else
            if (pObj->AsSWSCreature() != 0)
            {
                CSWSCreature* pC = pObj->AsSWSCreature();
                if (pC->m_bInParty != 0)
                    nResult = (pC->GetCurrentHitPoints(0) < 1);
            }

            if (pObj->m_bCommandable != 0 && nResult == 0 && pObj->GetDead() != 1 &&
                (pCreature == 0 || pCreature->m_inTransit == 0))
#endif
            {
                pObj->SetOrientation(vDir);
            }
            return 0;
        }

        // Drive control.
        case MSG_INPUT_DRIVE_CONTROL:
            HandlePlayerToServerInputDriveControl(pPlayer);
            return nResult;

        // Remove a disguise effect.
        case MSG_INPUT_CANCEL_DISGUISE:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;

            nResult = 0;
            CSWSCreature* pTarget =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
            if (pTarget == 0)
                return nResult;
            if (pTarget->m_inTransit != 0)
                return nResult;

            int nCount = pTarget->m_effects.m_count;
            if (nCount < 1)
                return nResult;

            for (int i = 0; i < nCount; ++i)
            {
                CGameEffect* pEffect = pTarget->m_effects.m_pArray[i];
                if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DISGUISE)
                {
                    pTarget->RemoveEffectById(pEffect->m_effectId);
                    return 0;
                }
            }
            return nResult;
        }

        // Export the reply.
        case MSG_INPUT_EXPORT_REQUEST:
        {
            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                ->SendServerToPlayerModule_ExportReply(pPlayer);
            return nResult;
        }

        // Abort drive control.
        case MSG_INPUT_ABORT_DRIVE_CONTROL:
        {
            HandlePlayerToServerInputAbortDriveControl(pPlayer);
            return nResult;
        }

        // Clear all actions.
        case MSG_INPUT_CANCEL_GUI_TIMING_EVENT:
        {
            CSWSObject* pObj = pPlayer->GetGameObject();
            if (pObj == 0)
                return 0;
            CSWSCreature* pC = pObj->AsSWSCreature();
            if (pC != 0 && pC->m_bCommandable == 1)
            {
                nResult = 1;
                pC->ClearAllActions(1);
            }
            return 1;
        }

        // Use a talent.
        case MSG_INPUT_CAST_SPELL_LIKE_ABILITY:
        {
            Vector vTarget(0.0f, 0.0f, 0.0f);
            unsigned int nSpellId = ReadDWORD(32);
            unsigned char nByte = ReadBYTE(8);
            unsigned char nFlags = ReadBYTE(8);

            unsigned long long oidTarget = OBJECT_INVALID;
            if ((nFlags & 1) != 0)
                oidTarget = ReadOBJECTIDServer();
            if ((nFlags & 2) != 0)
            {
                vTarget.x = ReadFLOAT(1.0f, 32);
                vTarget.y = ReadFLOAT(1.0f, 32);
                vTarget.z = ReadFLOAT(1.0f, 32);
            }

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pStats == 0 || pCreature == 0)
                return nResult;

            g_pRules->m_pSpellArray->GetSpell(nSpellId);
            pCreature->ActionManager(1);
            if (oidTarget == CGameObjectArray::INVALIDOBJID)
            {
                pCreature->AddUseTalentAtLocationActions(0, nSpellId, vTarget, 0xfe,
                                                         OBJECT_INVALID, -1,
                                                         nByte, 0);
            }
            else
            {
                pCreature->AddUseTalentOnObjectActions(0, nSpellId, oidTarget, 0xfe,
                                                       OBJECT_INVALID, -1,
                                                       nByte, 0);
            }
            return nResult;
        }

        // Give items.
        case MSG_INPUT_GIVE_ITEM:
        {
            unsigned long long oidCreature = ReadOBJECTIDServer();
            unsigned long long oidItem = ReadOBJECTIDServer();
            unsigned long long oidPlaceable = ReadOBJECTIDServer();
            int nAmount = ReadINT(32);

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;

            nResult = 0;
            CSWSCreature* pTarget =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
            CSWSPlaceable* pPlaceable =
                g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidPlaceable);
            if (pTarget == 0 || pItem == 0 || pPlaceable == 0)
                return nResult;

            pTarget->AddGiveItemActions(oidItem, oidPlaceable, nAmount, 0);
            return nResult;
        }

#if !KOTOR1  // K1 has no door-saber / switch-weapons / stance input messages
        // Door saber.
        case MSG_INPUT_DOOR_SABER:
        {
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;
            if (pCreature == 0)
                return nResult;
            if (pCreature->m_bCommandable == 0)
                return 0;

            pCreature->ActionManager(2);

            CGameObject* pObj = 0;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget,
                                                                                &pObj) ==
                CGameObjectArray::SUCCESS)
            {
                CSWSDoor* pDoor = pObj->AsSWSDoor();
                if (pDoor == 0)
                    return 0;
                if (pDoor->m_bLocked != 0)
                    pCreature->AddDoorSaberAction(oidTarget);
            }
            return 1;
        }

        // Switch weapons.
        case MSG_INPUT_SWITCH_WEAPONS:
        {
            ReadOBJECTIDServer();
            int bImmediate = ReadBOOL();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;

            CSWSCreature* pC = pPlayer->GetGameObject()->AsSWSCreature();
            if (pC != 0)
                return pC->AddSwitchWeaponsAction(bImmediate);
            return nResult;
        }

        // Stance.
        case MSG_INPUT_STANCE:
        {
            char cStance = ReadCHAR(8);
            unsigned long long oidTarget = ReadOBJECTIDServer();

            if (MessageReadOverflow(1))
                return nResult;
            if (MessageReadUnderflow(1))
                return nResult;

            nResult = 1;
            pCreature->AddStanceActions(static_cast<int>(cStance), oidTarget, 1, 0, 0, 0);
            return nResult;
        }
#endif
    }

    return nResult;
}

int CSWSMessage::HandlePlayerToServerInventoryMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case MSG_INVENTORY_EQUIP:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            unsigned int nFlags = ReadDWORD(32);
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSObject* pObject;
#if KOTOR1  // K1 has no top-menu override character
            pObject = pPlayer->GetGameObject()->AsSWSCreature();
#else
            int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
            if (nOverride != -1)
            {
                unsigned long long nSubjectId =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nOverride, 0, 1);
                pObject = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(nSubjectId));
                if (pObject == 0)
                    return 0;
            }
            else
            {
                pObject = pPlayer->GetGameObject()->AsSWSCreature();
            }
#endif

            unsigned int nEqFlags = (nFlags == MSG_SLOT_LEFT_WEAPON_2)
                                   ? MSG_SLOT_LEFT_WEAPON_2 | MSG_SLOT_LEFT_WEAPON
                                   : nFlags;
            unsigned int nEqFlags2 = (nEqFlags == MSG_SLOT_RIGHT_WEAPON_2)
                                    ? MSG_SLOT_RIGHT_WEAPON_2 | MSG_SLOT_RIGHT_WEAPON
                                    : nEqFlags;

            CGameObject* pGameObj;
            if (pObject != 0 &&
                g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nObjectId, &pGameObj)
                == CGameObjectArray::SUCCESS)
            {
                if (pGameObj->AsSWSItem() != 0)
                {
                    CSWBaseItem* pBase = g_pRules->m_pBaseItemArray->GetBaseItem(pGameObj->AsSWSItem()->m_baseItem);
                    if ((pBase->m_nEquipableSlots & nEqFlags2) != 0)
                    {
                        pObject->ClearAllActions(1);
                        static_cast<CSWSCreature*>(pObject)->m_pCombatRound->RemoveAllActions();
                        static_cast<CSWSCreature*>(pObject)->AddEquipItemActions(pGameObj->AsSWSItem(), nFlags, 1, 1, 0, 1, 0, 0);

                        // Same combat-round/attack tail as UNEQUIP; the projectile id means no
                        // attack follows.
                        unsigned long long nTargetId = static_cast<CSWSCreature*>(pObject)->m_pCombatRound->m_oidAttackTarget;
                        if (nTargetId != OBJECT_INVALID)
                            static_cast<CSWSCreature*>(pObject)->AddAttackActions(nTargetId, 0,
                                0, 0, 0, 0, 1, 1, SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0, 4, 0);
                        return 1;
                    }
                }
            }

            CSWSMessage* pSWS = g_pAppManager->m_pServerExoApp->GetSWSMessage();
            pSWS->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, nObjectId, nFlags);
            return 0;
        }

        case MSG_INVENTORY_EQUIP_CANCEL:
        case MSG_INVENTORY_DROP_CANCEL:
        case MSG_INVENTORY_PICKUP_CANCEL:
        case MSG_INVENTORY_UNEQUIP_CANCEL:
        case 9:
        case 10:
            return 0;

        case MSG_INVENTORY_DROP:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            float fX = ReadFLOAT(1.0f, 32);
            float fY = ReadFLOAT(1.0f, 32);
            float fZ = ReadFLOAT(1.0f, 32);
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            Vector vPos;
            vPos.x = fX;
            vPos.y = fY;
            vPos.z = fZ + 0.1f;

            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0)
            {
                pCreature->ActionManager(2);
                pCreature->AddDropItemActions(nObjectId, vPos, 0);
            }
            else
            {
                CSWSMessage* pSWS = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                pSWS->SendServerToPlayerInventory_DropCancel(pPlayer->m_nPlayerId, nObjectId);
                return 0;
            }
            return 1;
        }

        case MSG_INVENTORY_PICKUP:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            unsigned long long nContainerId = ReadOBJECTIDServer();
            unsigned char nUnknown = ReadBYTE(8);
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0)
            {
                pCreature->ActionManager(2);
                pCreature->AddPickUpItemActions(nObjectId, nContainerId, nUnknown);
            }
            else
            {
                CSWSMessage* pSWS = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                pSWS->SendServerToPlayerInventory_PickupCancel(pPlayer->m_nPlayerId, nObjectId);
                return 0;
            }
            return 1;
        }

        case MSG_INVENTORY_UNEQUIP:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            unsigned long long nContainerId = ReadOBJECTIDServer();
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSObject* pObject;
#if KOTOR1  // K1 has no top-menu override character
            pObject = pPlayer->GetGameObject()->AsSWSCreature();
#else
            int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
            if (nOverride != -1)
            {
                unsigned long long nSubjectId =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nOverride, 0, 1);
                pObject = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(nSubjectId));
                if (pObject == 0)
                    return 0;
            }
            else
            {
                pObject = pPlayer->GetGameObject()->AsSWSCreature();
            }
#endif

            CGameObject* pGameObj;
            if (pObject != 0
                && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(nObjectId, &pGameObj)
                    == CGameObjectArray::SUCCESS
                && pGameObj->AsSWSItem() != 0)
            {
                pObject->ClearAllActions(1);
                static_cast<CSWSCreature*>(pObject)->m_pCombatRound->RemoveAllActions();
                static_cast<CSWSCreature*>(pObject)->AddUnequipActions(pGameObj->AsSWSItem(), nContainerId, 1, 1, 0, 0, 0);

                // Same combat-round/attack tail as EQUIP; an invalid projectile id means no attack.
                unsigned long long nTargetId = static_cast<CSWSCreature*>(pObject)->m_pCombatRound->m_oidAttackTarget;
                if (nTargetId != OBJECT_INVALID)
                    static_cast<CSWSCreature*>(pObject)->AddAttackActions(nTargetId, 0,
                        0, 0, 0, 0, 1, 1, SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0, 4, 0);
                return 1;
            }

            CSWSMessage* pSWS = g_pAppManager->m_pServerExoApp->GetSWSMessage();
            pSWS->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, nObjectId);
            return 0;
        }

        case MSG_INVENTORY_EQUIP_TOGGLE:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            int bReadExtra = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;

            unsigned long long nTargetId;
            if (bReadExtra)
                nTargetId = ReadOBJECTIDServer();
            else
                nTargetId = CGameObjectArray::INVALIDOBJID;

            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature == 0)
                return 0;

            CSWInventory* pInventory = pCreature->m_pInventory;
            if (pInventory == 0)
                return 0;

            CSWSItem* pSrcItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nObjectId));
            // Resolved even when the target id is invalid, which just yields null.
            CSWSItem* pDstItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nTargetId));

            bool bSrcInInventory = false;
            if (pSrcItem != 0 && pInventory->GetItemInInventory(pSrcItem))
            {
                CSWBaseItem* pSrcBase = g_pRules->m_pBaseItemArray->GetBaseItem(pSrcItem->m_baseItem);
                if (pSrcBase != 0)
                {
#if KOTOR1  // K1 has 14 equipment slots
                    for (int i = 0; i < 14; i++)
#else
                    for (int i = 0; i < 16; i++)
#endif
                    {
                        unsigned int nSlotBit = 1 << i;
                        if ((pSrcBase->m_nEquipableSlots & nSlotBit) != 0
                            && pInventory->GetSlotFromItem(pSrcItem) == nSlotBit
                            && (nTargetId == CGameObjectArray::INVALIDOBJID
                                || pInventory->GetSlotFromItem(pSrcItem) == MSG_SLOT_RIGHT_WEAPON))
                        {
                            bSrcInInventory = true;
                        }
                    }
                }
            }

            // With no destination target there is nothing to check, so the flag starts set.
            int bDstInInventory = (nTargetId == CGameObjectArray::INVALIDOBJID) ? 1 : 0;
            if (!bDstInInventory && pInventory->GetItemInInventory(pDstItem))
            {
                CSWBaseItem* pDstBase = g_pRules->m_pBaseItemArray->GetBaseItem(pDstItem->m_baseItem);
                if (pDstBase != 0)
                {
#if KOTOR1  // K1: the target counts when it sits in one of its own slots
                    for (int i = 0; i < 14; i++)
                    {
                        unsigned int nSlotBit = 1 << i;
                        if ((pDstBase->m_nEquipableSlots & nSlotBit) != 0
                            && pInventory->GetSlotFromItem(pDstItem) == nSlotBit)
                        {
                            bDstInInventory = 1;
                        }
                    }
#else
                    for (int i = 0; i < 16; i++)
                    {
                        unsigned int nSlotBit = 1 << i;
                        if ((pDstBase->m_nEquipableSlots & nSlotBit) != 0
                            && pInventory->GetSlotFromItem(pDstItem) != nSlotBit)
                        {
                            bDstInInventory = 1;
                        }
                    }
#endif
                }
            }

            if (pSrcItem == 0)
                return 0;

            if (!(pCreature->GetItemRepository(1)->GetItemInRepository(pSrcItem, 1)
                  || pInventory->GetItemInInventory(pSrcItem)))
                return 0;

            if (bSrcInInventory && bDstInInventory)
            {
                pCreature->AddUnequipActions(pSrcItem, CGameObjectArray::INVALIDOBJID, 1, 0, 0, 0, 0);
                if (pDstItem != 0)
                    pCreature->AddUnequipActions(pDstItem, CGameObjectArray::INVALIDOBJID, 1, 0, 0, 0, 0);
                return 1;
            }

            // When equipping the destination, the base item's m_nWeaponType capability byte forces
            // the second weapon slot ahead of the descending chain.
            if (pDstItem != 0
                && (pCreature->GetItemRepository(1)->GetItemInRepository(pDstItem, 1)
                    || pInventory->GetItemInInventory(pDstItem)))
            {
                if (g_pRules->m_pBaseItemArray->GetBaseItem(pDstItem->m_baseItem)->m_nWeaponType)
                {
                    pCreature->AddEquipItemActions(pDstItem, MSG_SLOT_LEFT_WEAPON, 1, 0, 0, 1, 0,
                                                   0);
                }
                else
                {
                    CSWBaseItem* pDstBase2 = g_pRules->m_pBaseItemArray->GetBaseItem(pDstItem->m_baseItem);
                    if (pDstBase2 == 0)
                        return 0;
                    unsigned int nEquip = pDstBase2->m_nEquipableSlots;
                    unsigned int nEquipSlot = 0;
                    if ((nEquip & 1) != 0) nEquipSlot = 1;
                    else if ((nEquip & 2) != 0) nEquipSlot = 2;
                    else if ((nEquip & 4) != 0) nEquipSlot = 4;
                    else if ((nEquip & 8) != 0) nEquipSlot = 8;
                    else if ((nEquip & MSG_SLOT_RIGHT_WEAPON) != 0) nEquipSlot = MSG_SLOT_RIGHT_WEAPON;
                    else if ((nEquip & MSG_SLOT_LEFT_WEAPON) != 0) nEquipSlot = MSG_SLOT_LEFT_WEAPON;
                    else if ((nEquip & MSG_SLOT_CLOAK) != 0) nEquipSlot = MSG_SLOT_CLOAK;
                    else if ((nEquip & MSG_SLOT_LEFT_ARM) != 0) nEquipSlot = MSG_SLOT_LEFT_ARM;
                    else if ((nEquip & MSG_SLOT_RIGHT_ARM) != 0) nEquipSlot = MSG_SLOT_RIGHT_ARM;
                    else if ((nEquip & MSG_SLOT_IMPLANT) != 0) nEquipSlot = MSG_SLOT_IMPLANT;
                    else if ((nEquip & MSG_SLOT_BELT) != 0) nEquipSlot = MSG_SLOT_BELT;
                    else if ((nEquip & MSG_SLOT_ARROWS) != 0) nEquipSlot = MSG_SLOT_ARROWS;
                    else if ((nEquip & MSG_SLOT_BULLETS) != 0) nEquipSlot = MSG_SLOT_BULLETS;
                    else if ((nEquip & MSG_SLOT_BOLTS) != 0) nEquipSlot = MSG_SLOT_BOLTS;
                    else if ((nEquip & MSG_SLOT_CREATURE_WEAPON_L) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_L;
                    else if ((nEquip & MSG_SLOT_CREATURE_WEAPON_R) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_R;
                    else if ((nEquip & MSG_SLOT_CREATURE_WEAPON_B) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_B;
                    else if ((nEquip & MSG_SLOT_CREATURE_ARMOR) != 0) nEquipSlot = MSG_SLOT_CREATURE_ARMOR;
                    else if ((nEquip & MSG_SLOT_RIGHT_WEAPON_2) != 0) nEquipSlot = MSG_SLOT_RIGHT_WEAPON_2;
                    else if ((nEquip & MSG_SLOT_LEFT_WEAPON_2) != 0) nEquipSlot = MSG_SLOT_LEFT_WEAPON_2;
                    if (nEquipSlot != 0)
                        pCreature->AddEquipItemActions(pDstItem, nEquipSlot, 1, 0, 0, 1, 0, 0);
                }
            }

            // The exchange re-equips the source item here. It was already vetted above, so there is
            // no null guard.
            if (pCreature->GetItemRepository(1)->GetItemInRepository(pSrcItem, 1)
                || pInventory->GetItemInInventory(pSrcItem))
            {
                CSWBaseItem* pSrcBase1 = g_pRules->m_pBaseItemArray->GetBaseItem(pSrcItem->m_baseItem);
                if (pSrcBase1 == 0)
                    return 0;
                unsigned int nEquip2 = pSrcBase1->m_nEquipableSlots;
                unsigned int nEquipSlot = 0;
                if (g_pRules->m_pBaseItemArray->GetBaseItem(pSrcItem->m_baseItem)->m_nWeaponType
                    && (nEquip2 & MSG_SLOT_RIGHT_WEAPON) != 0)
                    nEquipSlot = MSG_SLOT_RIGHT_WEAPON;
                else if ((nEquip2 & 1) != 0) nEquipSlot = 1;
                else if ((nEquip2 & 2) != 0) nEquipSlot = 2;
                else if ((nEquip2 & 4) != 0) nEquipSlot = 4;
                else if ((nEquip2 & 8) != 0) nEquipSlot = 8;
                else if ((nEquip2 & MSG_SLOT_RIGHT_WEAPON) != 0) nEquipSlot = MSG_SLOT_RIGHT_WEAPON;
                else if ((nEquip2 & MSG_SLOT_LEFT_WEAPON) != 0) nEquipSlot = MSG_SLOT_LEFT_WEAPON;
                else if ((nEquip2 & MSG_SLOT_CLOAK) != 0) nEquipSlot = MSG_SLOT_CLOAK;
                else if ((nEquip2 & MSG_SLOT_LEFT_ARM) != 0) nEquipSlot = MSG_SLOT_LEFT_ARM;
                else if ((nEquip2 & MSG_SLOT_RIGHT_ARM) != 0) nEquipSlot = MSG_SLOT_RIGHT_ARM;
                else if ((nEquip2 & MSG_SLOT_IMPLANT) != 0) nEquipSlot = MSG_SLOT_IMPLANT;
                else if ((nEquip2 & MSG_SLOT_BELT) != 0) nEquipSlot = MSG_SLOT_BELT;
                else if ((nEquip2 & MSG_SLOT_ARROWS) != 0) nEquipSlot = MSG_SLOT_ARROWS;
                else if ((nEquip2 & MSG_SLOT_BULLETS) != 0) nEquipSlot = MSG_SLOT_BULLETS;
                else if ((nEquip2 & MSG_SLOT_BOLTS) != 0) nEquipSlot = MSG_SLOT_BOLTS;
                else if ((nEquip2 & MSG_SLOT_CREATURE_WEAPON_L) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_L;
                else if ((nEquip2 & MSG_SLOT_CREATURE_WEAPON_R) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_R;
                else if ((nEquip2 & MSG_SLOT_CREATURE_WEAPON_B) != 0) nEquipSlot = MSG_SLOT_CREATURE_WEAPON_B;
                else if ((nEquip2 & MSG_SLOT_CREATURE_ARMOR) != 0) nEquipSlot = MSG_SLOT_CREATURE_ARMOR;
                else if ((nEquip2 & MSG_SLOT_RIGHT_WEAPON_2) != 0) nEquipSlot = MSG_SLOT_RIGHT_WEAPON_2;
                else if ((nEquip2 & MSG_SLOT_LEFT_WEAPON_2) != 0) nEquipSlot = MSG_SLOT_LEFT_WEAPON_2;
                if (nEquipSlot != 0)
                    pCreature->AddEquipItemActions(pSrcItem, nEquipSlot, 1, 0, 0, 1, 0, 0);
            }

            // Sweep the slots so no half-done swap is left behind; the armour and second-weapon
            // slots act as the staging area.
            if (pDstItem != 0)
            {
                CSWSItem* pRightWeapon = static_cast<CSWSItem*>(pInventory->GetItemInSlot(MSG_SLOT_RIGHT_WEAPON));
                CSWSItem* pLeftWeapon = static_cast<CSWSItem*>(pInventory->GetItemInSlot(MSG_SLOT_LEFT_WEAPON));

                if (pRightWeapon != 0 && pRightWeapon->m_objectId != pSrcItem->m_objectId)
                    pCreature->AddUnequipActions(pRightWeapon, CGameObjectArray::INVALIDOBJID, 1, 0, 0, 0, 0);
                if (pLeftWeapon != 0 && pLeftWeapon->m_objectId != pDstItem->m_objectId)
                    pCreature->AddUnequipActions(pLeftWeapon, CGameObjectArray::INVALIDOBJID, 1, 0, 0, 0, 0);
            }

            return 1;
        }

        case MSG_INVENTORY_USE: // RunScript "itemuse"
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CExoString sScript("itemuse");
            g_pVirtualMachine->RunScript(&sScript, nObjectId, 1);
            return 0;
        }

        case MSG_INVENTORY_LEARN_SCROLL:
        {
            unsigned long long nObjectId = ReadOBJECTIDServer();
            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            CSWSMessage* pSWS = g_pAppManager->m_pServerExoApp->GetSWSMessage();
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature == 0)
                return 0;
            pCreature->LearnScroll(nObjectId);
            pSWS->SendServerToPlayerInventory_LearnScroll(pPlayer->m_nPlayerId, nObjectId,
                                                          MSG_INVENTORY_LEARNED_SCROLL);
            return 1;
        }

        default:
            return 0;
    }
}

// Character load/create handshake; every arm forwards to LoadCharacterStart or LoadCharacterFinish.
int CSWSMessage::HandlePlayerToServerLoginMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    int nResult = 0;
    g_pAppManager->m_pServerExoApp->GetModule();
    if (MessageReadOverflow(1))
        return nResult;

    switch (nType)
    {
    case MSG_LOGIN_SERVER_CHARACTER:
    case MSG_LOGIN_SERVER_CHARACTER_JOIN:
    case MSG_LOGIN_SERVER_CHARACTER_CREATE:
    {
        CResRef sResRef = ReadCResRef(16);
        if (MessageReadOverflow(1))
            return nResult;
        if (MessageReadUnderflow(1))
            return nResult;
        g_pAppManager->m_pServerExoApp->LoadCharacterStart(nType, pPlayer, sResRef, 0, 0);
        nResult = 1;
        break;
    }
    case MSG_LOGIN_IFO_CHARACTER:
        if (MessageReadUnderflow(1))
            return nResult;
        if (MessageReadOverflow(1))
            return nResult;
        if (pPlayer->m_saveListIndex != -1)
            g_pAppManager->m_pServerExoApp->LoadCharacterStart(nType, pPlayer, CResRef(""), 0, 0);
        else
            SendServerToPlayerLogin_NeedCharacter(pPlayer->m_nPlayerId);
        nResult = 1;
        break;
    case MSG_LOGIN_LOCAL_CHARACTER:
    case MSG_LOGIN_LOCAL_CHARACTER_JOIN:
    {
        unsigned int nSize = ReadDWORD(32);
        void* pData = ReadVOIDPtr(nSize);
        if (MessageReadOverflow(1))
            return nResult;
        if (MessageReadUnderflow(1))
            return nResult;
        g_pAppManager->m_pServerExoApp->LoadCharacterStart(nType, pPlayer, CResRef(""), pData, nSize);
        nResult = 1;
        break;
    }
    case MSG_LOGIN_CHARACTER_RESPONSE:
    {
        int bSuccess = ReadBOOL();
        if (MessageReadOverflow(1))
            return nResult;
        if (MessageReadUnderflow(1))
            return nResult;
        g_pAppManager->m_pServerExoApp->LoadCharacterFinish(pPlayer, bSuccess);
        nResult = 1;
        break;
    }
    }
    return nResult;
}

// Packet dispatcher over the header's major, minor and sub bytes.
int CSWSMessage::HandlePlayerToServerMessage(unsigned int nPlayerId, unsigned char* pMessage,
                                             unsigned int nMessageLength)
{
    CExoString sMessageType;
    unsigned char nMajor = pMessage[0];
    unsigned char nMinor = pMessage[1];
    unsigned char nSub = pMessage[2];
    int bOverflow;
    int nRet;
    CSWSPlayer* pPlayer;

    SetReadMessage(pMessage + 3, nMessageLength - 3, MSG_PLAYERID_ALL_CLIENTS, 1);
    bOverflow = MessageReadOverflow(1);
    if (bOverflow)
        return 0;
    if (nMajor != MSG_PLAYER_TO_SERVER)
        return 0;

    pPlayer = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(nPlayerId, 1));
    if (pPlayer == 0)
        nRet = 1;
    else
    {
        sMessageType.Format(const_cast<char*>("unknown Major (0x%.2X)"), nMinor);
        nRet = 0;
        switch (nMinor - 1)
        {
            case MSG_MAJOR_SERVER_STATUS - 1:
                sMessageType = "ServerStatus";
                bOverflow = MessageReadOverflow(1);
                nRet = 0;
                if (nSub == MSG_SERVER_STATUS_REQUEST && !bOverflow)
                    nRet = SendServerToPlayerServerStatus_Status(pPlayer->m_nPlayerId);
                break;
            case MSG_MAJOR_CHAR_LIST - 1:
                sMessageType = "CharList";
                nRet = HandlePlayerToServerCharListMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_JOURNAL - 1:
                sMessageType = "Journal";
                nRet = HandlePlayerToServerJournalMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_MAP_PIN - 1:
                sMessageType = "MapPin";
                if (nSub == MSG_MAP_PIN_ADD)
                    nRet = HandlePlayerToServerMapPinSetMapPinAt(pPlayer);
                else if (nSub == MSG_MAP_PIN_REMOVE)
                    nRet = HandlePlayerToServerMapPinDestroyMapPin(pPlayer);
                else if (nSub == MSG_MAP_PIN_CHANGE)
                    nRet = HandlePlayerToServerMapPinChangePin(pPlayer);
                else
                    nRet = 1;
                break;
            case MSG_MAJOR_LOGIN - 1:
                sMessageType = "Login";
                nRet = HandlePlayerToServerLoginMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_MODULE - 1:
                sMessageType = "Module";
                nRet = HandlePlayerToServerModuleMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_AREA - 1:
                sMessageType = "Area";
                nRet = HandlePlayerToServerAreaMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GAME_OBJECT_UPDATE - 1:
                sMessageType = "GameObjUpdate";
                nRet = HandlePlayerToServerGameObjectUpdate(pPlayer, nSub);
                break;
            case MSG_MAJOR_INPUT - 1:
                sMessageType = "Input";
                nRet = HandlePlayerToServerInputMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GOLD - 1:
                sMessageType = "Gold";
                HandlePlayerToServerGoldMessage(pPlayer, nSub);
                nRet = 0;
                break;
            case MSG_MAJOR_CHAT - 1:
                sMessageType = "Chat";
                nRet = HandlePlayerToServerChatMessage(pPlayer, nSub);
                break;
            case 10: // major 11
                sMessageType = "Chat";
                nRet = 1;
                break;
            case MSG_MAJOR_INVENTORY - 1:
                sMessageType = "Inventory";
                nRet = HandlePlayerToServerInventoryMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GUI_INVENTORY - 1:
                sMessageType = "GuiInventory";
                nRet = ReadBYTE(8);
                if (MessageReadOverflow(1) || MessageReadUnderflow(1))
                    nRet = 0;
                else
                {
                    if (nSub == MSG_GUI_INVENTORY_STATUS)
                        pPlayer->m_pInventoryGUI->SetOpen(nRet != 0);
                    nRet = 1;
                }
                break;
            case MSG_MAJOR_PARTY - 1:
                sMessageType = "Party";
                nRet = HandlePlayerToServerParty(pPlayer, nSub);
                break;
            case MSG_MAJOR_CHEAT - 1:
                sMessageType = "Cheat";
                nRet = HandlePlayerToServerCheatMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_DIALOG - 1:
                sMessageType = "Dialog";
                nRet = HandlePlayerToServerDialogMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GUI_CHARACTER_SHEET - 1:
                sMessageType = "GuiCharacterSheet";
                nRet = HandlePlayerToServerCharacterSheetMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GUI_CONTAINER - 1:
                sMessageType = "GuiContainer";
                nRet = HandlePlayerToServerGuiContainerMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_LEVEL_UP - 1:
                sMessageType = "LevelUp";
                nRet = HandlePlayerToServerLevelUpMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_DEATH - 1:
                sMessageType = "Death";
                HandlePlayerToServerPlayerDeath(pPlayer, nSub);
                nRet = 1;
                break;
            case MSG_MAJOR_CHARACTER_DOWNLOAD - 1:
                sMessageType = "Character_Download";
                bOverflow = MessageReadOverflow(1);
                nRet = 0;
                if (nSub == MSG_CHARACTER_DOWNLOAD_REQUEST && !bOverflow)
                {
                    if (pPlayer->m_state == 1)
                        nRet = SendServerToPlayerCharacterDownloadReply(pPlayer);
                    else
                        nRet = SendServerToPlayerCharacterDownloadFail(pPlayer);
                }
                break;
            case MSG_MAJOR_SHUTDOWN_SERVER - 1:
                sMessageType = "ShutDownServer";
                nRet = HandlePlayerToServerShutDownServer(pPlayer, nSub);
                break;
            case MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST - 1:
                sMessageType = "PlayModuleCharacterList";
                if (MessageReadOverflow(1))
                    nRet = 0;
                else if (nSub == MSG_PLAY_MODULE_CHARACTER_LIST_START)
                    nRet = HandlePlayerToServerPlayModuleCharacterList_Start(pPlayer);
                else if (nSub == MSG_PLAY_MODULE_CHARACTER_LIST_STOP)
                {
                    if (MessageReadOverflow(1) || MessageReadUnderflow(1))
                        nRet = 0;
                    else
                    {
                        pPlayer->m_bPlayModuleListingCharacters = 0;
                        nRet = 1;
                    }
                }
                else
                    nRet = 0;
                break;
            case MSG_MAJOR_QUICK_CHAT - 1:
                sMessageType = "QuickChat";
                nRet = HandlePlayerToServerQuickChatMessage(pPlayer, nSub);
                break;
            case MSG_MAJOR_GUI_QUICKBAR - 1:
                sMessageType = "GuiQuickbar";
                nRet = 1;
                break;
        }

        CExoString sOverflow;
        if (MessageReadOverflow(0))
            sOverflow.Format(const_cast<char*>("Server message OVERFLOW in %s (0x%.2X)\nFIX this or dream up something nasty to do to the person responsible!"), sMessageType.CStr(), nSub);
        if (MessageReadUnderflow(0))
            sOverflow.Format(const_cast<char*>("Server message UNDERFLOW in %s (0x%.2X)\nFIX this or dream up something nasty to do to the person responsible!"), sMessageType.CStr(), nSub);
    }
    return nRet;
}

int CSWSMessage::HandlePlayerToServerServerStatusMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    int bOverflow = MessageReadOverflow(1);
    if (nType || bOverflow)
        return 0;

    return SendServerToPlayerServerStatus_Status(pPlayer->m_nPlayerId);
}

// Module sub-opcode dispatcher: 2 = DM, 4 = save, 5 = load, 7 = run.
int CSWSMessage::HandlePlayerToServerModuleMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    CExoString sSaveGameName;
    CExoString sModuleName;
    CExoString sModuleName2;
    unsigned int nSaveGameNameLength;

    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case MSG_MODULE_LOADED:
            if (pPlayer->m_connectionState != 1)
                return 1;
            if (g_pAppManager->m_pServerExoApp->GetServerMode() != 2)
                return 1;
            pPlayer->m_characterLoaded = 1;
            g_pAppManager->m_pServerExoApp->InitiateModuleForPlayer(pPlayer);
            return (SendServerToPlayerPlayerList_Add(CNetLayer::PLAYERID_ALL_PLAYERS, pPlayer) &
                    SendServerToPlayerPlayerList_All(pPlayer)) & 1;

        case MSG_MODULE_SAVE_GAME:
            nSaveGameNameLength = ReadDWORD(32);
            sSaveGameName = ReadCExoString(32);
            sModuleName2 = ReadCExoString(32);
            g_pAppManager->m_pServerExoApp->SetSaveGameName(sModuleName2);
            if (MessageReadUnderflow(1))
                return 0;
            if (!pPlayer->GetIsAllowedToSave())
                return 0;
            if (g_pAppManager->m_pServerExoApp->GetLoadingModule() == 1)
                return 0;
#if KOTOR1  // K1: the 4-argument SaveGame
            return g_pAppManager->m_pServerExoApp->SaveGame(nSaveGameNameLength, sSaveGameName, pPlayer, 0);
#else
            return g_pAppManager->m_pServerExoApp->SaveGame(nSaveGameNameLength, sSaveGameName, pPlayer, 0, 0);
#endif

        case MSG_MODULE_LOAD_GAME:
            nSaveGameNameLength = ReadDWORD(32);
            sSaveGameName = ReadCExoString(32);
            sModuleName = ReadCExoString(32);
            if (MessageReadUnderflow(1))
                return 0;
            if (!pPlayer->GetIsAllowedToSave())
                return 0;
            if (g_pAppManager->m_pServerExoApp->GetLoadingModule() == 1)
                return 0;
            g_pAppManager->m_pServerExoApp->SetLoadingModule(1);
            SendServerToPlayerModule_Loading(pPlayer);
            return g_pAppManager->m_pServerExoApp->LoadGame(nSaveGameNameLength, sSaveGameName, sModuleName, pPlayer);

        case MSG_MODULE_RUN_MODULE:
            if (MessageReadUnderflow(1))
                return 0;
            g_pAppManager->m_pServerExoApp->RunModule();
            return 1;

        default:
            return 0;
    }
}

int CSWSMessage::HandlePlayerToServerServerChatMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    K2_UNUSED(pPlayer); K2_UNUSED(nType);
    return 1;
}

// Party sub-opcode dispatcher; most arms send a CSWCCMessageData feedback message.
int CSWSMessage::HandlePlayerToServerParty(CSWSPlayer* pPlayer, unsigned char nType)
{
    CExoString sPartyText;
    CResRef sRosterResRef;
    CExoString sMemberName;
    int nResult = 0;

    if (MessageReadOverflow(1) || pPlayer == 0)
        return nResult;

    switch (nType)
    {
    case MSG_PARTY_LIST_REQUEST:
    {
        if (pPlayer->GetGameObject() == 0)
        {
            SendServerToPlayerParty_List(pPlayer->m_nPlayerId, 0, 0, MSG_PARTY_LIST,
                                         OBJECT_INVALID);
        }
        else
        {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            CSWSFaction* pFaction = pCreature->GetFaction();
            SendServerToPlayerParty_List(pPlayer->m_nPlayerId, pFaction->m_members.m_count,
                                         pFaction->m_members.m_pArray, MSG_PARTY_LIST,
                                         pCreature->m_objectId);
        }
        return 1;
    }

    // The invitee ignores the invitation: a notice to each side on success, refusal otherwise.
    // GetClientObjectByObjectId's result is deliberately discarded.
    case MSG_PARTY_IGNORE_INVITATION:
    {
        unsigned long long nMemberId = ReadOBJECTIDServer();
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nMemberId);
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nMemberId);
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
#ifdef K2_FIX_ENGINE_BUGS
        if (pSelfCreature == 0 || pCreature == 0)
            return 1;
#endif
        if (pSelfCreature != 0 && pSelfCreature->ReplyToInvitation(2, pCreature->m_objectId) == 1)
        {
            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, pCreature->m_objectId);
            pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_YOU_IGNORED_INVITATION, pData);

            pData = new CSWCCMessageData;
            pData->SetObjectID(0, pSelfCreature->m_objectId);
            pCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_IGNORED, pData);
        }
        else
        {
            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, pSelfCreature->m_objectId);
            pCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_EXPIRED, pData);
        }
        return 1;
    }

    // Leave party. The member id comes from the player; the wire id is consumed and discarded.
    // A server that blocks party changes refuses, otherwise the full RemoveMember fan-out.
    case MSG_PARTY_LEAVE:
    {
        ReadOBJECTIDServer();
        CSWSCreature* pRemover =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
#if !KOTOR1  // K1: no server-info party gate, always the faction path
        if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bBlockPartyChanges == 1)
        {
            if (pRemover != 0)
            {
                new CSWCCMessageData;  // discarded
                pRemover->SendFeedbackMessage(FEEDBACK_PARTY_LEAVE_BLOCKED, 0);
            }
        }
        else
#endif
        {
            CSWSPlayer* pClientObject =
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pPlayer->m_gameObjectId);
            if (pRemover != 0 && pClientObject != 0)
            {
                CSWSFaction* pFaction = pRemover->GetFaction();
                if (pFaction != 0)
                {
                    pFaction->RemoveMember(pPlayer->m_gameObjectId);
                    if (pFaction->m_members.m_count > 0)
                    {
                        unsigned long long* paMembers = pFaction->m_members.m_pArray;
                        for (int i = 0; i < pFaction->m_members.m_count; i++)
                        {
                            CSWSCreature* pMember = g_pAppManager->m_pServerExoApp
                                                        ->GetCreatureByGameObjectID(paMembers[i]);
                            if (pMember != 0)
                            {
                                CSWCCMessageData* pData = new CSWCCMessageData;
                                pData->SetObjectID(0, pPlayer->m_gameObjectId);
                                pMember->SendFeedbackMessage(FEEDBACK_PARTY_MEMBER_LEFT, pData);
                            }
                        }
                    }
                    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp
                                                  ->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
                    if (pCreature != 0)
                        pCreature->SendFeedbackMessage(FEEDBACK_PARTY_YOU_LEFT, 0);
                }
            }
        }
        return 1;
    }

    // The invitee accepts; the "none" sentinel falls back to the pending inviter.
    // ReplyToInvitation(0) on self, then the inviter's faction is told.
    case MSG_PARTY_ACCEPT_INVITATION:
    {
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
        if (pSelfCreature != 0)
        {
            unsigned long long nMemberId = ReadOBJECTIDServer();
            if (nMemberId == OBJECT_INVALID)
                nMemberId = pSelfCreature->m_oidInvitedToPartyBy;
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nMemberId);
            if (pCreature != 0)
            {
                if (pSelfCreature->ReplyToInvitation(0, pCreature->m_objectId) == 1)
                {
                    unsigned long long* paMembers = pCreature->GetFaction()->m_members.m_pArray;
                    for (int i = 0; i < pCreature->GetFaction()->m_members.m_count; i++)
                    {
                        CSWSCreature* pMember = g_pAppManager->m_pServerExoApp
                                                    ->GetCreatureByGameObjectID(paMembers[i]);
                        if (pMember != 0)
                        {
                            CSWCCMessageData* pData = new CSWCCMessageData;
                            pData->SetObjectID(0, pSelfCreature->m_objectId);
                            pMember->SendFeedbackMessage(FEEDBACK_PARTY_JOINED, pData);
                        }
                    }
                }
                else
                {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_EXPIRED, pData);
                }
            }
        }
        return 1;
    }

    // The invitee declines: a notice to each side on success, refusal otherwise.
    case MSG_PARTY_DECLINE_INVITATION:
    {
        unsigned long long nMemberId = ReadOBJECTIDServer();
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nMemberId);
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
        CSWSPlayer* pClientObject =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nMemberId);
        if (pSelfCreature != 0 && pCreature != 0 && pClientObject != 0)
        {
            if (pSelfCreature->ReplyToInvitation(1, pCreature->m_objectId) == 1)
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, pCreature->m_objectId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_YOU_REJECTED_INVITATION, pData);

                pData = new CSWCCMessageData;
                pData->SetObjectID(0, pSelfCreature->m_objectId);
                pCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_REJECTED, pData);
            }
            else
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, pCreature->m_objectId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_EXPIRED, pData);
            }
        }
        return 1;
    }
    // A server that blocks party changes refuses, otherwise the full AttemptRemove flow (member
    // fan-out, the removal reply, and the m_bPassiveAttackBehaviour latch clear).
    case MSG_PARTY_KICK:
    {
        unsigned long long nMemberId = ReadOBJECTIDServer();
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
#if !KOTOR1  // K1: no server-info party gate, always the faction path
        if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bBlockPartyChanges == 1)
        {
            new CSWCCMessageData;  // discarded
            pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_KICK_BLOCKED, 0);
        }
        else
#endif
        {
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nMemberId);
            CSWSPlayer* pClientObject =
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nMemberId);
            CSWSFaction* pFaction = pSelfCreature->GetFaction();
            if (pSelfCreature != 0 && pCreature != 0 && pClientObject != 0 && pFaction != 0)
            {
                if (pFaction->AttemptRemove(nMemberId, pSelfCreature->m_objectId) == 1)
                {
                    if (pFaction->m_members.m_count > 0)
                    {
                        unsigned long long* paMembers = pFaction->m_members.m_pArray;
                        for (int i = 0; i < pFaction->m_members.m_count; i++)
                        {
                            CSWSCreature* pMember = g_pAppManager->m_pServerExoApp
                                                        ->GetCreatureByGameObjectID(paMembers[i]);
                            if (pMember != 0)
                            {
                                CSWCCMessageData* pData = new CSWCCMessageData;
                                pData->SetObjectID(0, nMemberId);
                                pMember->SendFeedbackMessage(FEEDBACK_PARTY_MEMBER_KICKED, pData);
                            }
                        }
                    }
                    CSWSCreature* pResolved =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nMemberId);
                    if (pResolved != 0)
                        pResolved->SendFeedbackMessage(FEEDBACK_PARTY_KICKED_YOU, 0);
                }
                if (pCreature->m_bPassiveAttackBehaviour != 0)
                    pCreature->m_bPassiveAttackBehaviour = 0;
            }
        }
        return 1;
    }

    // Promoting to party transfers leadership to the candidate, then tells the members.
    case MSG_PARTY_TRANSFER_LEADERSHIP:
    {
        unsigned long long nCandidateId = ReadOBJECTIDServer();
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCandidateId);
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
        CSWSFaction* pFaction = pSelfCreature->GetFaction();
        if (pCreature != 0 && pSelfCreature != 0 && pFaction != 0)
        {
            pFaction->TransferLeadership(nCandidateId, pSelfCreature->m_objectId);
            if (pFaction->m_members.m_count > 0)
            {
                unsigned long long* paMembers = pFaction->m_members.m_pArray;
                for (int i = 0; i < pFaction->m_members.m_count; i++)
                {
                    CSWSCreature* pMember = g_pAppManager->m_pServerExoApp
                                                ->GetCreatureByGameObjectID(paMembers[i]);
                    if (pMember != 0)
                    {
                        CSWCCMessageData* pData = new CSWCCMessageData;
                        pData->SetObjectID(0, nCandidateId);
                        pMember->SendFeedbackMessage(FEEDBACK_PARTY_NEW_LEADER, pData);
                    }
                }
            }
        }
        return 1;
    }

    // The invitation state machine invites when either side is already flagged, else runs the
    // GetNumInvited / GetIsInInvitationsIgnored / GetSingletonParty ladder ending in the
    // self-invite. The top rung sends the leadership pair and pops the full-name panel.
    case MSG_PARTY_INVITE:
    {
        unsigned long long nCandidateId = ReadOBJECTIDServer();
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCandidateId);
        CSWSCreature* pSelfCreature = pPlayer->GetGameObject()->AsSWSCreature();
        if (pCreature != 0 && pSelfCreature != 0)
        {
            if (pCreature->m_bInvitedToParty != 0 || pSelfCreature->m_bInvitedToParty != 0)
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, nCandidateId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_PENDING, pData);
            }
            else if (pCreature->GetNumInvited() != 0)
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, nCandidateId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITEE_INVITING, pData);
            }
            else if (pCreature->GetIsInInvitationsIgnored(pSelfCreature->m_objectId) == 1)
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, nCandidateId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITATION_IGNORED, pData);
            }
            else if (pCreature->GetFaction()->GetSingletonParty() != 0)
            {
                CSWSFaction* pFaction = pSelfCreature->GetFaction();
                CSWSPlayer* pClientObject =
                    g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nCandidateId);
                if (pFaction != 0 && pClientObject != 0)
                {
                    if (pFaction->InviteMember(nCandidateId, pSelfCreature->m_objectId) == 1)
                    {
                        CSWCCMessageData* pData = new CSWCCMessageData;
                        pData->SetObjectID(0, pSelfCreature->m_objectId);
                        pCreature->SendFeedbackMessage(FEEDBACK_PARTY_RECEIVED_INVITATION, pData);

                        pData = new CSWCCMessageData;
                        pData->SetObjectID(0, nCandidateId);
                        pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_SENT_INVITATION, pData);

                        CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                        CExoString sPanelName = pSelfCreature->m_pStats->GetFullName();
                        pMessage->SendServerToPlayerPopUpGUIPanel(nCandidateId, 1, 0, 0, 0,
                                                                 sPanelName);
                    }
                }
            }
            else
            {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetObjectID(0, nCandidateId);
                pSelfCreature->SendFeedbackMessage(FEEDBACK_PARTY_INVITEE_IN_OTHER_PARTY, pData);
            }
        }
        return 1;
    }
    }
    return nResult;
}

// Character-list request: a character was picked, or the list itself is wanted.
int CSWSMessage::HandlePlayerToServerCharListMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    CResRef sResRef;
    int nResult = 0;
    if (MessageReadOverflow(1))
        return nResult;

    if (nType == MSG_CHAR_LIST_REQUEST)
    {
        if (MessageReadUnderflow(1))
            return nResult;
        if (g_pAppManager->m_pServerExoApp->GetServerMode() == 1 ||
            g_pAppManager->m_pServerExoApp->GetServerMode() == 2)
        {
            nResult = SendServerToPlayerCharList(pPlayer);
        }
        else
        {
            g_pAppManager->m_pServerExoApp->AddCharListRequest(pPlayer->m_nPlayerId);
            nResult = 1;
        }
    }
    else if (nType == MSG_CHAR_LIST_REQUEST_UPDATE)
    {
        unsigned char bAction = ReadBYTE(8);
        sResRef = ReadCResRef(16);
        if (MessageReadUnderflow(1))
            return nResult;
        nResult = SendServerToPlayerUpdateCharResponse(pPlayer, bAction, sResRef);
    }
    return nResult;
}

// Client has entered a new area; the player is moved and the party list re-synced.
int CSWSMessage::HandlePlayerToServerAreaMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    int bOverflow = MessageReadOverflow(1);
    int nResult = 0;
    if (nType != MSG_AREA_LOADED || bOverflow)
        return nResult;

    if (pPlayer->m_connectionState == 1)
        pPlayer->m_connectionState = 2;

    CAppManager** const ppAppManager = &g_pAppManager;
    if ((*ppAppManager)->m_pServerExoApp->GetServerMode() != 2)
        nResult = 1;
    else
    {
        (*ppAppManager)->m_pServerExoApp->MovePlayerToArea(pPlayer);
        nResult = SendServerToPlayerPlayerList_Add(CNetLayer::PLAYERID_ALL_PLAYERS, pPlayer);
        nResult &= SendServerToPlayerPlayerList_All(pPlayer);
        nResult &= 1;

        if ((*ppAppManager)->m_pServerExoApp->GetPauseState(1) == 1)
            SendServerToPlayerModule_SetPauseState(1, 1);
        if ((*ppAppManager)->m_pServerExoApp->GetPauseState(2) == 1)
            SendServerToPlayerModule_SetPauseState(2, 1);
    }
    return nResult;
}

// Level-up accept flow; the new level's record is validated and then confirmed to the client.
int CSWSMessage::HandlePlayerToServerLevelUpMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    int nResult = 0;
    if (MessageReadOverflow(1))
        return nResult;
    if (nType != MSG_LEVEL_UP_REQUEST)
        return nResult;

    CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
    if (pCreature == 0)
        return nResult;

    CSWLevelStats* pStats = new CSWLevelStats();
    pStats->m_nClass = ReadBYTE(8);

    if (((pCreature->m_pStats->GetLevel(0) + 1) & 3) == 0)
        pStats->m_nAbilityScoreIncrease = ReadBYTE(8);

    CSWClass* pClass = &g_pRules->m_paClasses[pStats->m_nClass];
    pStats->m_nHitPointGain = pClass->m_nHitDie;
    pStats->m_nForcePointGain = pClass->m_nForcePointGain;

    unsigned int nSkillMask = ReadDWORD(32);
    for (int i = 0; i < g_pRules->m_nNumSkills; i++)
    {
        if ((nSkillMask & (1 << i)) != 0)
            pStats->SetSkillRankChange(static_cast<unsigned short>(i), ReadCHAR(8));
    }

    pStats->m_nSkillPoints = ReadWORD(16);

    for (unsigned char n = ReadBYTE(8); n != 0; n--)
        pStats->AddFeat(ReadWORD(16));

    for (unsigned char n = ReadBYTE(8); n != 0; n--)
        pStats->m_spellsGained.Add(ReadDWORD(32));

    for (unsigned char n = ReadBYTE(8); n != 0; n--)
        pStats->m_spellsLost.Add(ReadDWORD(32));

    if (MessageReadOverflow(1))
        return nResult;
    if (MessageReadUnderflow(1))
        return nResult;

    int bValid = pCreature->m_pStats->ValidateLevelUp(pStats);
    if (bValid == 0)
        delete pStats;

    nResult = SendServerToPlayerLevelUp_Confirmation(pPlayer->m_nPlayerId, bValid);
    if (pCreature->m_pStats->CanLevelUp() != 0)
        pCreature->SendFeedbackMessage(FEEDBACK_LEVEL_UP_AVAILABLE, 0);

#if !KOTOR1  // K1: no influence
    if (pCreature->m_bPlayerMadeCharacter != 0)
    {
        for (int i = 0; i < 12; i++)
        {
            unsigned long long nNpcId =
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1);
            if (nNpcId != OBJECT_INVALID)
            {
                CSWSCreature* pNpc =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nNpcId);
                if (pNpc != 0)
                    pNpc->HandleAlignmentInfluence();
            }
        }
    }
#endif

    return nResult;
}

// Handles the journal GUI requests 0xa-0xe, screen open/close plus the two world-journal edits.
int CSWSMessage::HandlePlayerToServerJournalMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    CExoString sName;
    CExoString sText;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();

    int nResult = 1;
    switch (nType)
    {
    case MSG_JOURNAL_QUEST_SCREEN_OPEN:
        pPlayer->m_pJournalState->m_bQuestScreenOpen = 1;
        SendServerToPlayerJournalFullUpdate(pPlayer);
        break;
    case MSG_JOURNAL_QUEST_SCREEN_CLOSED:
        pPlayer->m_pJournalState->m_bQuestScreenOpen = 0;
        pPlayer->m_pJournalState->m_updates.Clear();
        break;
    case MSG_JOURNAL_REQUEST_ADD:
    {
        unsigned int nDay;
        unsigned int nTime;
        sName = ReadCExoString(32);
        sText = ReadCExoString(32);
        if (MessageReadOverflow(1)) {
            nResult = 0;
            break;
        }
        if (MessageReadUnderflow(1)) {
            nResult = 0;
            break;
        }
        if (pModule != 0) {
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
            pModule->AddWorldJournalEntry(pModule->GetWorldJournalIndexUnique(), sText, sName,
                                          nDay, nTime);
        }
        break;
    }
    case MSG_JOURNAL_REQUEST_DELETE:
    {
        int nIndex = ReadINT(32);
        int bByStrRef = ReadBOOL();
        if (MessageReadOverflow(1)) {
            nResult = 0;
            break;
        }
        if (MessageReadUnderflow(1)) {
            nResult = 0;
            break;
        }
        if (pModule != 0) {
            if (bByStrRef)
                pModule->DeleteWorldJournalEntryStrref(nIndex);
            else
                pModule->DeleteWorldJournalEntry(nIndex);
        }
        break;
    }
    }
    return nResult;
}

int CSWSMessage::HandlePlayerToServerMapPinMessage(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (nType == 2)
        return HandlePlayerToServerMapPinSetMapPinAt(pPlayer);
    else if (nType == 3)
        return HandlePlayerToServerMapPinDestroyMapPin(pPlayer);
    else if (nType == 5)
        return HandlePlayerToServerMapPinChangePin(pPlayer);

    return 1;
}

// Client reporting its player character's death; queues one script event.
int CSWSMessage::HandlePlayerToServerPlayerDeath(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (nType == MSG_DEATH_RESPAWN)
    {
        unsigned long long nModuleId = g_pAppManager->m_pServerExoApp->GetModule()->m_objectId;

        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_RESPAWN_BUTTON_PRESSED;

        CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        pAI->AddEventDeltaTime(0, 0, pPlayer->GetGameObject()->m_objectId, nModuleId,
                               CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }

    return 1;
}

int CSWSMessage::HandlePlayerToServerCharacterDownload(CSWSPlayer* pPlayer, unsigned char nType)
{
    int bOverflow = MessageReadOverflow(1);
    if (nType != 1 || bOverflow)
        return 0;

    if (pPlayer->m_state == 1)
        return SendServerToPlayerCharacterDownloadReply(pPlayer);

    return SendServerToPlayerCharacterDownloadFail(pPlayer);
}

int CSWSMessage::HandlePlayerToServerShutDownServer(CSWSPlayer* pPlayer, unsigned char nType)
{
    K2_UNUSED(pPlayer); K2_UNUSED(nType);
    if (MessageReadOverflow(1))
        return 0;

    int bValue = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pServerExoApp->Shutdown(bValue == 0, 1);
    return 1;
}

// Character-list sub-opcode dispatcher.
int CSWSMessage::HandlePlayerToServerPlayModuleCharacterList(CSWSPlayer* pPlayer, unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == 1)
        return HandlePlayerToServerPlayModuleCharacterList_Start(pPlayer);
    else if (nType == 2)
        return HandlePlayerToServerPlayModuleCharacterList_Stop(pPlayer);

    return 0;
}

int CSWSMessage::HandlePlayerToServerMapPinSetMapPinAt(CSWSPlayer* pPlayer)
{
    CExoString sLabel;
    CExoString sVarName;
    int bOverflow;
    float fX;
    float fY;
    float fZ;
    CSWSObject* pObject;
    CSWSCreature* pCreature;
    CSWSScriptVarTable* pScriptVars;
    int nCount;
    bOverflow = MessageReadOverflow(1);
    if (bOverflow)
        return 0;

    fX = ReadFLOAT(1.0f, 32);
    fY = ReadFLOAT(1.0f, 32);
    fZ = ReadFLOAT(1.0f, 32);
    sLabel = ReadCExoString(32);

    if (MessageReadUnderflow(1))
        return 0;

    pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    pScriptVars = &pCreature->m_scriptVars;
    CExoString sName("NW_TOTAL_MAP_PINS");
    nCount = pScriptVars->GetInt(sName) + 1;
    pScriptVars->SetInt(sName, nCount, 0);

    sVarName.Format(const_cast<char*>("NW_MAP_PIN_NTRY_%i"), nCount);
    pScriptVars->SetString(sVarName, sLabel);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_XPOS_%i"), nCount);
    pScriptVars->SetFloat(sVarName, fX);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_YPOS_%i"), nCount);
    pScriptVars->SetFloat(sVarName, fY);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_ZPOS_%i"), nCount);
    pScriptVars->SetFloat(sVarName, fZ);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_AREA_%i"), nCount);
    pScriptVars->SetObject(sVarName, static_cast<CSWSArea*>(pCreature->GetArea())->m_objectId);

    SendServerToPlayerMapPinAdded(pPlayer, Vector(fX, fY, fZ), sLabel, nCount);
    return 1;
}

// Deletes the pin's five script variables.
int CSWSMessage::HandlePlayerToServerMapPinDestroyMapPin(CSWSPlayer* pPlayer)
{
    CExoString sVarName;
    int nIndex;
    CSWSObject* pObject;
    CSWSCreature* pCreature;
    CSWSScriptVarTable* pScriptVars;
    if (MessageReadOverflow(1))
        return 0;

    nIndex = ReadINT(32);

    if (MessageReadUnderflow(1))
        return 0;

    pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    pScriptVars = &pCreature->m_scriptVars;
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_NTRY_%i"), nIndex);
    pScriptVars->DestroyString(sVarName);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_XPOS_%i"), nIndex);
    pScriptVars->DestroyFloat(sVarName);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_YPOS_%i"), nIndex);
    pScriptVars->DestroyFloat(sVarName);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_ZPOS_%i"), nIndex);
    pScriptVars->DestroyFloat(sVarName);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_AREA_%i"), nIndex);
    pScriptVars->DestroyObject(sVarName);
    return 1;
}

// Overwrites the pin's five script variables.
int CSWSMessage::HandlePlayerToServerMapPinChangePin(CSWSPlayer* pPlayer)
{
    CExoString sLabel;
    CExoString sVarName;
    float fX;
    float fY;
    float fZ;
    int nIndex;
    CSWSObject* pObject;
    CSWSCreature* pCreature;
    CSWSScriptVarTable* pScriptVars;
    if (MessageReadOverflow(1))
        return 0;

    fX = ReadFLOAT(1.0f, 32);
    fY = ReadFLOAT(1.0f, 32);
    fZ = ReadFLOAT(1.0f, 32);
    sLabel = ReadCExoString(32);
    nIndex = ReadDWORD(32);

    if (MessageReadUnderflow(1))
        return 0;

    pObject = pPlayer->GetGameObject();
    if (pObject == 0)
        return 0;

    pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    pScriptVars = &pCreature->m_scriptVars;
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_NTRY_%i"), nIndex);
    pScriptVars->SetString(sVarName, sLabel);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_XPOS_%i"), nIndex);
    pScriptVars->SetFloat(sVarName, fX);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_YPOS_%i"), nIndex);
    pScriptVars->SetFloat(sVarName, fY);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_ZPOS_%i"), nIndex);
    pScriptVars->SetFloat(sVarName, fZ);
    sVarName.Format(const_cast<char*>("NW_MAP_PIN_AREA_%i"), nIndex);
    pScriptVars->SetObject(sVarName, static_cast<CSWSArea*>(pCreature->GetArea())->m_objectId);

    return 1;
}

// Walks the connected players and answers with each one's game object.
int CSWSMessage::HandlePlayerToServerPlayModuleCharacterList_Start(CSWSPlayer* pPlayer)
{
    if (MessageReadOverflow(1) || MessageReadUnderflow(1))
        return 0;

    CAppManager** const ppMgr = &g_pAppManager;
    CExoLinkedList<CSWSClient>* pList = (*ppMgr)->m_pServerExoApp->GetPlayerList();
    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    if (pNode != 0) {
        do {
            CSWSPlayer* pOther = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
            if (pOther != 0 && pOther->GetGameObject() != 0) {
                unsigned long long objectId = pOther->GetGameObject()->m_objectId;
                if (objectId != CGameObjectArray::INVALIDOBJID) {
                    CSWSMessage* pMsg = (*ppMgr)->m_pServerExoApp->GetSWSMessage();
                    if (pMsg != 0)
                        pMsg->SendServerToPlayerPlayModuleCharacterListResponse(
                            pPlayer->m_nPlayerId, objectId, 1);
                }
            }
            pList->m_pInternal->GetNext(pNode);
        } while (pNode != 0);
    }

    pPlayer->m_bPlayModuleListingCharacters = 1;
    return 1;
}

int CSWSMessage::HandlePlayerToServerPlayModuleCharacterList_Stop(CSWSPlayer* pPlayer)
{
    if (MessageReadOverflow(1) || MessageReadUnderflow(1))
        return 0;

    pPlayer->m_bPlayModuleListingCharacters = 0;
    return 1;
}
