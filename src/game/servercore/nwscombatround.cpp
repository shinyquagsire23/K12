#include "k2stdint.h"
#include "nwscombatround.h"
#include "nwscreature.h"
#include "nwscriptconst.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "nwssendplayermessage.h"
#include "nwsitem.h"
#include "nwsrules.h"
#include "nwitem.h"
#include "clientexoapp.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwsobject.h"
#include "gameobjectarray.h"
#include "clientoptions.h"
#include "nwscript.h"
#include "nwsdoor.h"
#include "nwsplaceable.h"
#include "resgff.h"
#include "CExoArrayListImpl.h"
#include "nwseffect.h"
#include "Log_Base2.h"
#include "nwsaiaction.h"

template void CExoArrayList<CSWSCombatSubAttack>::Allocate(int);

int g_nCombatStretch;

#include "port/libcstring.h"

// The GFF struct ids of the combat round's saved list elements.
enum {
    GFF_STRUCT_ATTACK            = 0xaaee,
    GFF_STRUCT_SPECIAL_ATTACK    = 0xdaee,
    GFF_STRUCT_SPECIAL_ATTACK_ID = 0xdaae,
    GFF_STRUCT_ROUND_ACTION      = 0xdaea,
    GFF_STRUCT_DAMAGE            = 0xdaee,   // the attack's DamageList, the same id
};

CSWSCombatRoundAction::CSWSCombatRoundAction()
    : m_vTargetPosition{}
{
    ClearData();
}

void CSWSCombatRoundAction::ClearData()
{
    m_nActionTimer = 0;
    m_nAnimation = 0;
    m_nAnimationTime = 0;
    m_nNumAttacks = 0;
    m_nActionType = 0;
    m_oidTarget = OBJECT_INVALID;
    m_bRetargettable = 1;
    m_nInventorySlot = 0;
    m_oidTargetRepository = OBJECT_INVALID;
    m_nSpellId = 0;
    m_nClassIndex = 0;
    m_nDomainLevel = 0;
    m_nMetaMagic = 0;
    m_bSkipReadyCount = 0;
    m_oidSpellTarget = OBJECT_INVALID;
    m_bNoTarget = 0;
    m_bProjectile = 0;
    m_nPathType = 0;
    m_nActionParam = 0;
    m_nReserved1 = 0;
    m_nFeatId = 0;
    m_nReadyKind = 0;
    m_oidItem = OBJECT_INVALID;
    m_nActivePropertyIndex = 0;
    m_nSubPropertyIndex = 0;
    m_nReserved2 = 0;
    m_bCutsceneAttack = 0;
    m_nCutsceneAnimation = 0;
    m_nCutsceneAttackResult = 4;
    m_nCutsceneDamage = 0;
    m_bIsPC = 0;
}

CSWSCombatRoundAction::~CSWSCombatRoundAction()
{
}

int CSWSCombatRoundAction::SaveData(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldINT(pStruct, m_nActionTimer, const_cast<char*>("ActionTimer"));
    pGFF->WriteFieldWORD(pStruct, m_nAnimation, const_cast<char*>("Animation"));
    pGFF->WriteFieldINT(pStruct, m_nAnimationTime, const_cast<char*>("AnimationTime"));
    pGFF->WriteFieldINT(pStruct, m_nNumAttacks, const_cast<char*>("NumAttacks"));
    pGFF->WriteFieldBYTE(pStruct, m_nActionType, const_cast<char*>("ActionType"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidTarget), const_cast<char*>("Target"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bRetargettable), const_cast<char*>("Retargettable"));
    pGFF->WriteFieldDWORD(pStruct, m_nInventorySlot, const_cast<char*>("InventorySlot"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidTargetRepository), const_cast<char*>("TargetRepository"));
    return 1;
}

int CSWSCombatRoundAction::LoadData(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;
    m_nActionTimer = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ActionTimer"), bSuccess, 0);
    m_nAnimation = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("Animation"), bSuccess, 0);
    m_nAnimationTime = pGFF->ReadFieldINT(pStruct, const_cast<char*>("AnimationTime"), bSuccess, 0);
    m_nNumAttacks = pGFF->ReadFieldINT(pStruct, const_cast<char*>("NumAttacks"), bSuccess, 0);
    m_nActionType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("ActionType"), bSuccess, 0);
    m_oidTarget = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Target"), bSuccess, 0);
    m_bRetargettable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Retargettable"), bSuccess, 0);
    m_nInventorySlot = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("InventorySlot"), bSuccess, 0);
    m_oidTargetRepository = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("TargetRepository"), bSuccess, 0);
    return 1;
}

void CSWSCombatRound::SetDeflectArrow(int deflectArrow) {
    m_deflectArrow = deflectArrow;
}

void CSWSCombatRound::SetOffHandAttack(int offHandAttack) {
    m_offHandAttack = offHandAttack;
}

int CSWSCombatRound::GetOffHandAttack() {
    return m_offHandAttack;
}

unsigned char CSWSCombatRound::GetCurrentAction() {
    return m_currentAction;
}

void CSWSCombatRound::SetCurrentAction(unsigned char action) {
    m_currentAction = action;
}

void CSWSCombatRound::SetCurrentAttack(unsigned char attack) {
    m_currentAttack = attack;
}

void CSWSCombatRound::SetBonusEffectAttacks(int bonusEffectAttacks) {
    m_bonusEffectAttacks = bonusEffectAttacks;
}

int CSWSCombatRound::GetActualBonusEffectAttacks() {
    return m_bonusEffectAttacks;
}

int CSWSCombatRound::GetTotalAttacks() {
    return m_nOnHandAttacks + m_nOffHandAttacks;
}

int CSWSCombatRound::GetFunctionalBonusEffectAttacks() {
    int bonus = m_bonusEffectAttacks;
    return bonus < 2 ? bonus : 2;
}

void CSWSCombatRound::SetSpellCastRound(int spell, int round) {
    m_bSpellCastRound = 1;
    m_spellCastRound = round;
    m_pCreature->m_nCurrentAttackAction = 4;
    m_pCreature->m_nCurrentForcePower = spell;
}

void CSWSCombatRound::StartCombatRoundCast(int a1, int a2, unsigned long long oidTarget,
                                           int a4, int a5) {
    if (m_bRoundStarted != 0)
        return;
    m_bRoundStarted = 1;
    m_bSpellCastRound = 1;
    m_spellCastRound = a2;
    m_nTimer = 0;
    m_nRoundLength = 3000;
    m_bEngaged = a4;
    m_bMaster = a5;

    bool bSkip = false;
    if (a5 != 0) {
        m_oidMaster = m_pCreature->m_objectId;
        if (oidTarget != OBJECT_INVALID && a4 != 0) {
            m_oidSlave = oidTarget;
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
            unsigned long long oidAlly = m_oidMaster;
            unsigned long long oidTarget2 = m_oidSlave;
            pCreature->m_pCombatRound->m_nTimer = 0;
            pCreature->m_pCombatRound->m_oidMaster = oidAlly;
            pCreature->m_pCombatRound->m_oidSlave = oidTarget2;
            bSkip = m_bRoundStarted == 0;
        }
    } else {
        m_oidMaster = oidTarget;
    }

    m_bRoundPaused = 0;
    m_oidPauseSource = OBJECT_INVALID;
    m_nPauseTimer = 0;
    m_pCreature->m_nCurrentAttackAction = 4;
    m_pCreature->m_nCurrentForcePower = a1;
    if (bSkip)
        return;

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_SPELL;
    AddAction(pAction, -1);
}

void CSWSCombatRound::StartCombatRound(unsigned long long oidTarget, int bAttack, int bDefend,
                                       unsigned short nAttacks, int a5) {
    m_bEngaged = bAttack;
    m_bMaster = bDefend;
    if (m_bEngaged != 0 && m_bMaster == 0) {
        EndCombatRound(m_bRoundStarted);
    }
    if (m_bMaster != 0) {
        m_oidMaster = m_pCreature->m_objectId;
        if (m_bEngaged != 0) {
            m_oidSlave = oidTarget;
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
            if (pCreature != 0) {
                pCreature->m_pCombatRound->m_oidMaster = m_oidMaster;
                pCreature->m_pCombatRound->m_oidSlave = m_oidSlave;
                pCreature->m_pCombatRound->m_nTimer = 0;
            }
        }
    } else {
        m_oidMaster = oidTarget;
    }
    m_bRoundStarted = 1;
    m_nTimer = 0;
    m_bRoundPaused = 0;
    m_nPauseTimer = 0;
    m_oidPauseSource = OBJECT_INVALID;
    m_bSpellCastRound = 0;
    m_spellCastRound = 0;
    m_bCutsceneAttack = a5;
    m_nRoundLength = 3000;
    for (int i = 0; i < 5; i++) {
        m_attacks[i].ClearAttackData();
    }
    CalculateOnHandAttacks(nAttacks);
    CalculateOffHandAttacks();
    m_nOffHandTaken = 0;
    m_nExtraTaken = 0;
    m_oidAttackTarget = m_pCreature->m_objectId == oidTarget ? OBJECT_INVALID : oidTarget;
    m_oidDodgeTarget = oidTarget;
    m_deflectArrow = 1;
    m_bWeaponSucks = 0;
    m_pCreature->SetLockOrientationToObject(oidTarget, 0);
    m_pCreature->m_nCurrentAttackAction = 3;
}

void CSWSCombatRound::EndCombatRound(int bEndRound)
{
    m_bRoundStarted = 0;
    m_nTimer = 0;
    m_nRoundLength = 0;
    m_bRoundPaused = 0;
    m_oidPauseSource = OBJECT_INVALID;
    m_nPauseTimer = 0;
    m_oidDodgeTarget = OBJECT_INVALID;
    m_nAdditAttacks = 0;
    m_nCastsThisRound = 0;

    if (m_pCreature != 0 && m_pCreature->m_bBodyFuel != 0) {
        CSWSCreatureStats* pStats = m_pCreature->m_pStats;
        pStats->m_nCurrentForcePoints = m_pCreature->GetMaxForcePoints();
    }
    K2_ASSUME(m_pCreature != 0);

    if (m_bMaster != 0) {
        CSWSCreature* pOtherCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSlave);
        if (pOtherCreature != 0) {
            pOtherCreature->m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
            pOtherCreature->m_pCombatRound->SetPauseTimer(0, 0);
            pOtherCreature->m_pCombatRound->m_bEngaged = 0;
            pOtherCreature->m_pCombatRound->m_oidMaster = OBJECT_INVALID;
        } else {
            CExoString sWarning;
            sWarning.Format(const_cast<char*>("CSWSCombatRound::EndCombatRound - %x Combat Slave (%x) "
                                   "not found!"),
                            m_pCreature->m_objectId, m_oidSlave);
        }
    }

    int bSetAttemptedAttackTarget = 0;
    unsigned long long oidAttackTarget = OBJECT_INVALID;
    if (m_oidAttackTarget != OBJECT_INVALID) {
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidAttackTarget);
        if (pObject == 0) {
            m_oidAttackTarget = OBJECT_INVALID;
        } else if (pObject->AsSWSCreature() != 0) {
            CSWSCreature* pTarget = pObject->AsSWSCreature();
            if (pTarget->GetDead() != 0 || pTarget->GetDeadTemp() != 0) {
                m_oidAttackTarget = OBJECT_INVALID;
                if (m_bSpellCastRound != 0) {
                    oidAttackTarget = m_pCreature->m_oidAttemptedSpellTarget;
                    bSetAttemptedAttackTarget = 1;
                } else {
                    oidAttackTarget = m_pCreature->m_attemptedAttackTargetId;
                    bSetAttemptedAttackTarget = 1;
                }
            }
        } else if (pObject->AsSWSDoor() != 0 && pObject->AsSWSDoor()->m_curOpenState != 0) {
            m_oidAttackTarget = OBJECT_INVALID;
        } else if (pObject->AsSWSPlaceable() != 0
                   && pObject->AsSWSPlaceable()->m_bInventoryOpen != 0) {
            m_oidAttackTarget = OBJECT_INVALID;
        }
    }

    m_pCreature->SetAnimation(SERVER_ANIMATION_READY);
    m_attacks[0].ClearAttackData();
    m_attacks[1].ClearAttackData();
    m_attacks[2].ClearAttackData();
    m_attacks[3].ClearAttackData();
    m_attacks[4].ClearAttackData();

    CGameObject* pObject =
        g_pAppManager->m_pServerExoApp->GetGameObject(m_pCreature->m_attemptedAttackTargetId);
    if (pObject == 0 || (pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() != 0))
        m_pCreature->SetAttemptedAttackTarget(OBJECT_INVALID);
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_pCreature->m_oidAttemptedSpellTarget);
    if (pObject == 0 || (pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() != 0))
        m_pCreature->m_oidAttemptedSpellTarget = OBJECT_INVALID;
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_pCreature->m_oidAttackTarget);
    if (pObject == 0 || (pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() != 0))
        m_pCreature->m_oidAttackTarget = OBJECT_INVALID;
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_pCreature->m_oidSpellTarget);
    if (pObject == 0 || (pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() != 0))
        m_pCreature->m_oidSpellTarget = OBJECT_INVALID;
    pObject =
        g_pAppManager->m_pServerExoApp->GetGameObject(m_pCreature->m_oidGoingToBeAttackedBy);
    if (pObject == 0 || (pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() != 0))
        m_pCreature->m_oidGoingToBeAttackedBy = OBJECT_INVALID;

    m_pCreature->ClearAttackerList();

    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    int bIsPlayerCharacter = 0;
    if (pLeader != 0
        && g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId)
               == m_pCreature->m_objectId) {
        bIsPlayerCharacter = 1;
    } else {
        m_pCreature->SetCombatMode(CSWSCreature::CREATURE_COMBAT_MODE_NONE, 1);
    }

    int bNoAttackPending = 1;
    CExoLinkedListNode* pNode = m_pCreature->m_actionList.m_pInternal->m_pHead;
    if (bEndRound != 0 && pNode != 0)
        bNoAttackPending =
            static_cast<CSWSObjectActionNode*>(m_pCreature->m_actionList.GetAtPos(pNode))
                ->m_nActionId != AIACTION_MOVE_TO_POINT;

    if (bEndRound != 0 && bNoAttackPending != 0 && m_spellCastRound == 0
        && (m_bCutsceneAttack | bIsPlayerCharacter) == 0
        && m_pCreature->GetDead() == 0
        && m_pCreature->GetDeadTemp() == 0) {
        g_pVirtualMachine->RunScript(&m_pCreature->m_scripts[CSWSCreature::CREATURE_SCRIPT_ENDROUND],
                                     m_pCreature->m_objectId, 1);
    }

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    if (pClientExoApp != 0) {
        int bAutoPause = (bEndRound != 0) & (pClientExoApp->GetPausedByCombat() == 0)
                         & bIsPlayerCharacter;
        if (bAutoPause == 1 && pClientExoApp->GetClientOptions()->m_bAutoPauseEndOfCombatRound != 0
            && (pLeader->m_nStateFlags & 1) != 0)
            pClientExoApp->SetAutoPaused(1, 5);
    }

    int bCreateAttacks = (m_bCutsceneAttack == 0) & bIsPlayerCharacter;
    if (bEndRound != 0 && bCreateAttacks != 0)
        m_pCreature->CreateNewAttackActions(bSetAttemptedAttackTarget, m_bSpellCastRound, oidAttackTarget);

    m_pCreature->m_oidLastHostileTarget = m_pCreature->m_oidAttackTarget;
    m_pCreature->m_nLastAttackAction = m_pCreature->m_nCurrentAttackAction;
    if (m_bSpellCastRound != 0) {
        m_pCreature->m_nLastForcePowerUsed = m_pCreature->m_nCurrentForcePower;
        m_pCreature->m_bSpellChargeSpent = 0;
        m_bSpellCastRound = 0;
    }
    if (m_pCreature->m_nCurrentCombatFeat != 0)
        m_pCreature->m_nLastCombatFeatUsed = m_pCreature->m_nCurrentCombatFeat;
    m_nNumAOOs = 1;
    m_nNumCleaves = 1;
}

void CSWSCombatRound::IncrementTimer(int nDelta)
{
    m_nTimer += nDelta;
    if (m_bRoundStarted != 0 && m_nTimer >= m_nRoundLength && m_bSpellCastRound != 0) {
        CExoLinkedListNode* pHead = m_pCreature->m_actionList.m_pInternal->m_pHead;
        if (pHead == 0 || pHead->m_pData == 0
                || static_cast<CSWSObjectActionNode*>(pHead->m_pData)->m_nActionId != AIACTION_CAST_SPELL) {
            EndCombatRound(1);
        }
        return;
    }
    if (m_bRoundStarted != 0 && m_nTimer >= m_nRoundLength
            && (m_bEngaged == 0 || m_bMaster != 0) && m_bSpellCastRound == 0) {
        EndCombatRound(1);
        return;
    }
    if (m_bRoundStarted != 0 && m_nTimer >= m_nRoundLength
            && m_bEngaged != 0 && m_bMaster == 0) {
        CExoString sWarning;
        CGameObject* pMaster = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidMaster);
        if (pMaster != 0) {
            sWarning.Format(const_cast<char*>("CSWSCombatRound::IncrementTimer - %s Master IS found (%x) "
                                   "and round has expired (%d %d); Resetting"),
                            m_pCreature->m_sTag.CStr(), pMaster->m_objectId,
                            m_nTimer, m_nRoundLength);
            m_oidMaster = OBJECT_INVALID;
            EndCombatRound(1);
        } else {
            sWarning.Format(const_cast<char*>("CSWSCombatRound::IncrementTimer - %s Master cannot be "
                                   "found and round has expired; Resetting"),
                            m_pCreature->m_sTag.CStr());
            m_oidMaster = OBJECT_INVALID;
            EndCombatRound(1);
        }
        return;
    }
    if (m_nTimer < 0) {
        CExoString sWarning;
        sWarning.Format(const_cast<char*>("CSWSCombatRound::IncrementTimer - %s Timer is negative at %d; "
                               "Ending combat round and resetting"),
                        m_pCreature->m_sTag.CStr(), m_nTimer);
        EndCombatRound(1);
    } else if (m_bEngaged != 0 && m_bMaster == 0
                   && m_oidMaster == m_pCreature->m_objectId) {
        m_bEngaged = 0;
        m_oidMaster = OBJECT_INVALID;
    }
}

void CSWSCombatRound::AddSpellAction() {
    if (m_bRoundStarted != 0) {
        CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_SPELL;
        AddAction(pAction, -1);
    }
}

void CSWSCombatRound::AddParryIndex() {
    if (m_bRoundStarted != 0) {
        CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_PARRY_INDEX;
        AddAction(pAction, -1);
    }
}

void CSWSCombatRound::AddReaction(int nDelay, int nAnimationTime, int nIndex) {
    if (m_bRoundStarted != 0) {
        CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_REACTION;
        pAction->m_nAnimationTime = nAnimationTime;
        pAction->m_nActionTimer = m_nTimer + nDelay;
        AddAction(pAction, nIndex);
    }
}

void CSWSCombatRound::AddCombatStepAction(unsigned int nTimer, unsigned long long oidTarget) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
    pAction->m_nActionTimer = nTimer;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_COMBAT_STEP;
    pAction->m_nAnimationTime = 500;
    pAction->m_oidTarget = oidTarget;
    AddAction(pAction, -1);
    DecrementRoundLength(500, 0);
}

void CSWSCombatRound::AddCutsceneAttackAction(unsigned long long oid, int a2, int a3, int a4) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ATTACK;
    pAction->m_oidTarget = oid;
    pAction->m_nAnimation = SERVER_ANIMATION_ATTACK;
    pAction->m_nAnimationTime = (g_nCombatStretch + 3000) / 2;
    pAction->m_bCutsceneAttack = 1;
    pAction->m_nCutsceneAnimation = a2;
    pAction->m_nCutsceneAttackResult = a3;
    pAction->m_nCutsceneDamage = a4;
    AddAction(pAction, -1);
    // Only the first cutscene attack claims the target.
    if (m_pCreature->m_attemptedAttackTargetId == OBJECT_INVALID) {
        m_pCreature->SetAttemptedAttackTarget(oid);
    }
}

void CSWSCombatRound::AddStanceAction(int nStance, unsigned long long oidTarget, int a) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_STANCE;
    pAction->m_bIsPC = a;
    pAction->m_nAnimation = SERVER_ANIMATION_READY;
    pAction->m_nAnimationTime = g_nCombatStretch + 3000;
    pAction->m_nNumAttacks = nStance;
    pAction->m_oidTarget = oidTarget;
    AddAction(pAction, -1);
}

void CSWSCombatRound::AddCutsceneMoveActions(unsigned long long oid, Vector vPos, int nRun) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction();
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_CUTSCENE_MOVE;
    pAction->m_nActionParam = nRun;
    pAction->m_vTargetPosition = vPos;
    pAction->m_oidTarget = oid;
    AddAction(pAction, -1);
}

void CSWSCombatRound::AddAttackAction(unsigned long long objectId, unsigned short nFeat,
                                      int a, int b, int c) {
    K2_UNUSED(b);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject == 0)
        return;
    if (pObject->AsSWSObject() == 0)
        return;
    if (pObject->AsSWSObject()->GetDead() != 0)
        return;

#if KOTOR1  // K1 refuses only an open door; no hit-point tests
    if (pObject->AsSWSDoor() != 0 && pObject->AsSWSDoor()->m_curOpenState != 0)
        return;
#else
    int bAttackable = 1;
    if (pObject->AsSWSDoor() != 0) {
        if (pObject->AsSWSDoor()->m_curOpenState != 0
                || pObject->AsSWSDoor()->GetCurrentHitPoints(0) <= 0) {
            bAttackable = 0;
        }
    }
    if (pObject->AsSWSPlaceable() != 0) {
        short nHitPoints = pObject->AsSWSPlaceable()->GetCurrentHitPoints(0);
        if (bAttackable == 0 || nHitPoints < 1)
            return;
    } else if (bAttackable == 0) {
        return;
    }
#endif

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    if (nFeat != 0) {
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_FEAT_ATTACK;
        pAction->m_nFeatId = nFeat;
    } else {
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ATTACK;
    }
    pAction->m_nAnimationTime = (g_nCombatStretch + 3000) / 2;
    pAction->m_nAnimation = SERVER_ANIMATION_ATTACK;
    pAction->m_bIsPC = a;
    pAction->m_oidTarget = objectId;
    pAction->m_nNumAttacks = c;
    AddAction(pAction, -1);

    if (m_pCreature->m_attemptedAttackTargetId == OBJECT_INVALID)
        m_pCreature->SetAttemptedAttackTarget(objectId);
}

void CSWSCombatRound::AddParryAttack(unsigned long long oid) {
    // The head node is read before m_pRoundActionList is null-tested.
    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    int nTimer K2_UNSET;
    if (m_pRoundActionList != 0) {
        if (pNode != 0)
            nTimer = static_cast<CSWSCombatRoundAction*>(pNode->m_pData)->m_nActionTimer - 1;
        else
            nTimer = 0;
    }

    unsigned char nAttacks = static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks);
    float fDivisor = static_cast<float>(nAttacks - 3) * 0.5f + 3.0f;
    int nTime = nAttacks * 750;
    if (nTime > 2500)
        nTime = 2500;
    int nAnimationTime = static_cast<int>(static_cast<float>(nTime + 500) / fDivisor);
    DecrementRoundLength(nAnimationTime, 1);

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ATTACK;
    pAction->m_nAnimation = SERVER_ANIMATION_ATTACK;
    pAction->m_nActionTimer = nTimer;
    pAction->m_nAnimationTime = nAnimationTime;
    pAction->m_nNumAttacks = 1;
    pAction->m_bRetargettable = 0;
    pAction->m_oidTarget = oid;

    if (pNode != 0)
        m_pRoundActionList->AddBefore(pAction, pNode);
    else
        m_pRoundActionList->AddTail(pAction);
}

void CSWSCombatRound::AddCleaveAttack(unsigned long long oid) {
    unsigned char nAttacks = static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks);
    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    float fDivisor = static_cast<float>(nAttacks - 3) * 0.5f + 3.0f;
    int nTime = nAttacks * 750;
    if (nTime > 2500)
        nTime = 2500;
    int nAnimationTime = static_cast<int>(static_cast<float>(nTime + 500) / fDivisor);
    DecrementRoundLength(nAnimationTime, 1);

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ATTACK;
    pAction->m_nActionTimer = 0;
    pAction->m_nAnimation = SERVER_ANIMATION_ATTACK;
    pAction->m_nAnimationTime = nAnimationTime;
    pAction->m_nNumAttacks = 1;
    pAction->m_oidTarget = oid;

    if (pNode != 0)
        m_pRoundActionList->AddBefore(pAction, pNode);
    else
        m_pRoundActionList->AddTail(pAction);
    m_nAdditAttacks++;
}

void CSWSCombatRound::AddAttackOfOpportunity(unsigned long long oid) {
    if (m_bRoundStarted != 0) {
        unsigned char nAttacks = static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks);
        CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
        float fDivisor = static_cast<float>(nAttacks - 3) * 0.5f + 3.0f;
        int nTime = nAttacks * 750;
        if (nTime > 2500)
            nTime = 2500;
        int nAnimationTime = static_cast<int>(static_cast<float>(nTime + 500) / fDivisor);
        DecrementRoundLength(nAnimationTime, 1);

        CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
        pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ATTACK;
        pAction->m_nActionTimer = 0;
        pAction->m_nAnimation = SERVER_ANIMATION_ATTACK;
        pAction->m_nAnimationTime = nAnimationTime;
        pAction->m_nNumAttacks = 1;
        pAction->m_bRetargettable = 0;
        pAction->m_oidTarget = oid;

        if (pNode != 0)
            m_pRoundActionList->AddBefore(pAction, pNode);
        else
            m_pRoundActionList->AddTail(pAction);
    }
    m_nAdditAttacks++;
}

void CSWSCombatRound::AddEquipAction(unsigned long long oidItem, unsigned int nSlot, int a, int b) {
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0)
        return;

    DecrementRoundLength(1500, 1);

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_bIsPC = b;
    pAction->m_nAnimationTime = 1500;
    pAction->m_oidTarget = oidItem;
    pAction->m_nActionTimer = 0;
    pAction->m_nInventorySlot = nSlot;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_EQUIP;
    pAction->m_nActionParam = a;

    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    if (pNode != 0)
        m_pRoundActionList->AddBefore(pAction, pNode);
    else
        m_pRoundActionList->AddTail(pAction);
}

void CSWSCombatRound::AddUnequipAction(unsigned long long oidItem, unsigned long long a2, int nSlot) {
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0 && !m_pCreature->CanUnEquipWeapon(pItem))
        return;

    DecrementRoundLength(1500, 1);

    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_oidTargetRepository = a2;
    pAction->m_nAnimationTime = 1500;
    pAction->m_bIsPC = 0;
    pAction->m_nActionTimer = 0;
    pAction->m_oidTarget = oidItem;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_UNEQUIP;
    pAction->m_nActionParam = nSlot;

    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    if (pNode != 0)
        m_pRoundActionList->AddBefore(pAction, pNode);
    else
        m_pRoundActionList->AddTail(pAction);
}

void CSWSCombatRound::AddSWItemSpellAction(unsigned long long oidItem, int a2, int a3, Vector vTarget,
                                           unsigned long long oidTarget, int a6, int a7) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_oidSpellTarget = oidTarget;
    pAction->m_bIsPC = a7;
    pAction->m_oidItem = oidItem;
    pAction->m_nActivePropertyIndex = a2;
    pAction->m_nSubPropertyIndex = a3;
    pAction->m_vTargetPosition = vTarget;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_ITEM_SPELL;
    pAction->m_bNoTarget = a6;

    if (m_pCreature->m_oidAttemptedSpellTarget == OBJECT_INVALID)
        m_pCreature->m_oidAttemptedSpellTarget = oidTarget;
    AddAction(pAction, -1);
}

void CSWSCombatRound::AddSWSpellAction(unsigned int nSpellId, int nClassIndex, int nDomainLevel,
                                       int nMetaMagic, int bSkipReadyCount, Vector vTarget,
                                       unsigned long long oidTarget, int a8, int a9,
                                       unsigned char a10, int a11, int a12, unsigned char a13,
                                       int a14) {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_nAnimationTime = 500;
    pAction->m_nActionParam = a11;
    pAction->m_nReserved1 = 0;
    pAction->m_nSpellId = nSpellId;
    pAction->m_nClassIndex = nClassIndex;
    pAction->m_nDomainLevel = nDomainLevel;
    pAction->m_nMetaMagic = nMetaMagic;
    pAction->m_vTargetPosition = vTarget;
    pAction->m_bSkipReadyCount = bSkipReadyCount;
    pAction->m_bNoTarget = a8;
    pAction->m_bProjectile = a9;
    pAction->m_bIsPC = a14;
    pAction->m_oidSpellTarget = oidTarget;
    pAction->m_nPathType = a10;
    pAction->m_nFeatId = a12;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_CAST_SPELL;
    pAction->m_nAnimation = SERVER_ANIMATION_CAST_1;
    pAction->m_nReadyKind = a13;

    if (m_pCreature->m_oidAttemptedSpellTarget == OBJECT_INVALID)
        m_pCreature->m_oidAttemptedSpellTarget = oidTarget;
    AddAction(pAction, -1);
}

unsigned short CSWSCombatRound::GetNewAttackID() {
    unsigned short id = m_newAttackId;
    if (id == 0xFFFE) {
        id = 0;
        m_newAttackId = 0;
    }
    m_newAttackId = id + 1;
    return id;
}

CSWSCombatAttackData* CSWSCombatRound::GetAttack(int nAttack) {
    if (nAttack < 0) {
        nAttack = 0;
    }
    return &m_attacks[nAttack];
}

void CSWSCombatRound::ClearAllAttacks() {
    for (int i = 0; i < 5; i++) {
        m_attacks[i].ClearAttackData();
    }
}

void CSWSCombatRound::InitializeNumberOfAttacks(unsigned short nFeat) {
    CalculateOnHandAttacks(nFeat);
    CalculateOffHandAttacks();
    m_nOffHandTaken = 0;
    m_nExtraTaken = 0;
}

#if !KOTOR1
__attribute__((noinline))
#endif
int CSWSCombatRound::CalculateOnHandAttacks(unsigned short nFeat) {
#if KOTOR1  // K1 uses the raw bonus-effect count, unclamped
    m_nOnHandAttacks = m_pCreature->m_pStats->GetAttacksPerRound() + m_bonusEffectAttacks;
#else
    m_nOnHandAttacks = m_pCreature->m_pStats->GetAttacksPerRound()
                     + GetFunctionalBonusEffectAttacks();
#endif
    switch (nFeat) {
    case FEAT_FLURRY:
    case FEAT_MULTI_SHOT:
    case FEAT_RAPID_SHOT:
    case FEAT_WHIRLWIND_ATTACK:
    case FEAT_IMPROVED_FLURRY:
    case FEAT_IMPROVED_RAPID_SHOT:
        m_nOnHandAttacks++;
        break;
    }
#if !KOTOR1  // K1 has no lightsaber forms
    if (m_pCreature->IsWieldingLightsaber() && m_pCreature->m_stanceId == FORM_SABER_VII_JUYO) {
        m_nOnHandAttacks++;
    }
#endif
    return m_nOnHandAttacks;
}

int CSWSCombatRound::CalculateOffHandAttacks() {
    int bOffHand = 0;
    CSWSItem* pWeapon =
            static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pWeapon != 0) {
        int nBaseItem = pWeapon->m_baseItem;
        CSWBaseItem* pBaseItem =
            g_pRules->GetBaseItem(nBaseItem);
        int nWeaponSize = pBaseItem->m_nWeaponSize;
        if (m_pCreature->m_nCreatureSize < nWeaponSize) {
            bOffHand = pWeapon->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED;
        } else {
            pWeapon =
                    static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if (pWeapon != 0) {
                int nOffBaseItem = pWeapon->m_baseItem;
                if (g_pRules->GetBaseItem(nOffBaseItem)->m_nWeaponType != 0) {
                    bOffHand = g_pRules->GetBaseItem(pWeapon->m_baseItem)
                                   ->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_NONE;
                }
            }
        }
    }
    m_nOffHandAttacks = bOffHand;
    return bOffHand;
}

int CSWSCombatRound::GetExtraAttack() {
    if (m_nAdditAttacks == 0 && m_bonusEffectAttacks == 0) {
        return 0;
    }
    return m_currentAttack >= m_nOnHandAttacks;
}

void CSWSCombatRound::InsertSpecialAttack(unsigned short nFeat, int nIndex) {
    if (m_specialAttacks.m_count >= 1) {
        m_specialAttacks.Insert(nFeat, nIndex);
        m_specialAttackIds.Insert(GetNewAttackID(), nIndex);
    } else {
        m_specialAttacks.Add(nFeat);
        m_specialAttackIds.Add(GetNewAttackID());
    }
}

void CSWSCombatRound::ClearAllSpecialAttacks() {
    for (unsigned int i = m_currentAttack;
         i < static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks); i++) {
        if (m_attacks[i].m_nFeatId != 0) {
            m_attacks[i].m_nFeatId = 0;
        }
    }
    m_specialAttacks.SetSize(0);
}

unsigned short CSWSCombatRound::GetSpecialAttack(int nIndex) {
    int i = 0;
    for (unsigned int nAttack = m_currentAttack;
         nAttack < static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks); nAttack++) {
        if (nIndex == i) {
            return m_attacks[nAttack].m_nFeatId;
        }
        i++;
    }
    for (int j = 0; j < m_specialAttacks.m_count; j++) {
        if (nIndex == i) {
            return m_specialAttacks.m_pArray[j];
        }
        i++;
    }
    return 0;
}

unsigned short CSWSCombatRound::GetSpecialAttackID(int nIndex) {
    int i = 0;
    if (static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks) > nIndex) {
        for (unsigned int nAttack = m_currentAttack;
             nAttack < static_cast<unsigned char>(m_nOnHandAttacks + m_nOffHandAttacks); nAttack++) {
            if (m_attacks[nAttack].m_nFeatId != 0) {
                if (i == nIndex) {
                    return m_attacks[nAttack].m_nSpecialAttackId;
                }
                i++;
            }
        }
    }
    for (int j = 0; j < m_specialAttackIds.m_count; j++) {
        if (i == nIndex) {
            return m_specialAttackIds.m_pArray[j];
        }
        i++;
    }
    return 0xffff;
}

void CSWSCombatRound::SetRoundPaused(int bPaused, unsigned long long objectId) {
    m_bRoundPaused = bPaused;
    m_oidPauseSource = objectId;
    if (bPaused == 0 && m_bInfinitePause != 0) {
        m_bInfinitePause = 0;
    }
}

void CSWSCombatRound::SetPauseTimer(int nTime, int bPaused) {
    m_bInfinitePause = bPaused;
    if (bPaused) {
        m_nPauseTimer = nTime;
    } else {
        if (nTime == 0) {
            m_nPauseTimer = 0;
        }
        if (m_nPauseTimer >= 1) {
            m_nPauseTimer += nTime;
        } else {
            m_nPauseTimer = nTime;
        }
    }
}

int CSWSCombatRound::CheckActionLength(unsigned long long oid, int nLength) {
    if (m_bRoundPaused == 1 && m_oidPauseSource != oid) {
        return 0;
    }
    if (m_bRoundStarted != 1) {
        return 1;
    }
    return m_nRoundLength - m_nTimer - m_nOverlapAmount + 1000 >= nLength;
}

int CSWSCombatRound::CheckActionLengthAtTime(unsigned long long oid, int nLength, int nTime) {
    if (m_bRoundPaused == 1 && m_oidPauseSource != oid && m_nPauseTimer > nTime) {
        return 0;
    }
    if (m_bRoundStarted != 1) {
        return 1;
    }
    return m_nRoundLength - nTime - m_nTimer - m_nOverlapAmount + 1000 >= nLength;
}

int CSWSCombatRound::HasCreatureWeapons() {
    return m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0
        && m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON) == 0
        && (m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L) != 0
            || m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_R) != 0
            || m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_B) != 0);
}

void CSWSCombatRound::SignalCombatRoundStarted() {
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
        m_pCreature->m_objectId);
    if (pPlayer != 0) {
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCombatRoundStarted(
            pPlayer);
    }
}

void CSWSCombatRound::AddAction(CSWSCombatRoundAction* pAction, int nIndex) {
    // A round queues at most four actions; anything beyond that is discarded.
    if (m_pRoundActionList != 0 && m_pRoundActionList->m_pInternal->m_nCount <= 3) {
        CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
        if (nIndex == -1) {
            // Sorted insert: splice before the first node whose timer is later, else append.
            while (pNode != 0) {
                CSWSCombatRoundAction* pAt =
                    static_cast<CSWSCombatRoundAction*>(m_pRoundActionList->GetAtPos(pNode));
                if (pAction->m_nActionTimer < pAt->m_nActionTimer) {
                    m_pRoundActionList->AddBefore(pAction, pNode);
                    return;
                }
                m_pRoundActionList->GetNext(pNode);
            }
            m_pRoundActionList->AddTail(pAction);
        } else {
            int i = 0;
            while (pNode != 0 && i < nIndex) {
                m_pRoundActionList->GetNext(pNode);
                i++;
            }
            if (pNode != 0)
                m_pRoundActionList->AddBefore(pAction, pNode);
            else
                m_pRoundActionList->AddTail(pAction);
        }
    } else if (pAction != 0) {
        ::operator delete(pAction);
    }
}

void CSWSCombatRound::RemoveAllActions() {
    if (m_pRoundActionList != 0) {
        for (;;) {
            if (m_pRoundActionList->m_pInternal->m_pHead == 0) {
                return;
            }
            void* pAction = m_pRoundActionList->RemoveHead();
            if (pAction != 0) {
                ::operator delete(pAction);
            }
        }
    }
}

CSWSCombatRound::CSWSCombatRound(CSWSCreature* pOwner)
{
    m_nReserved1 = 0;
    m_bRoundPaused = 0;
    m_specialAttackIds.m_count = 0;
    m_specialAttackIds.m_capacity = 0;
    m_specialAttackIds.m_pArray = 0;
    m_specialAttacks.m_count = 0;
    m_specialAttacks.m_capacity = 0;
    m_specialAttacks.m_pArray = 0;
    m_spellCastRound = 0;
    m_bCutsceneAttack = 0;
    m_bRoundStarted = 0;
    m_bSpellCastRound = 0;
    m_oidPauseSource = OBJECT_INVALID;
    m_nPauseTimer = 0;
    m_bInfinitePause = 0;
    m_pRoundActionList = new CExoLinkedList<CSWSCombatRoundAction>();
    m_nCastsThisRound = 0;
    m_currentAttack = 0;
    m_offHandAttack = 0;
    m_nParryIndex = 0;
    m_deflectArrow = 0;
    m_bWeaponSucks = 0;
    m_nBleedTimer = 0;
    m_nRoundLength = 0;
    m_nOverlapAmount = 0;
    m_nTimer = 0;
    m_bTargetCannotReact = 0;
    m_nParryActions = 0;
    m_nAdditAttacks = 0;
    m_bonusEffectAttacks = 0;
    m_nOffHandTaken = 0;
    m_nExtraTaken = 0;
    m_nOnHandAttacks = 0;
    m_nOffHandAttacks = 0;
    m_nNumAOOs = 1;
    m_nNumCleaves = 1;
    m_oidPendingTargetSwap = OBJECT_INVALID;
    m_nAttackGroupCounter = 0xff;
    m_pCreature = pOwner;
    m_newAttackId = 0;
    m_oidDodgeTarget = OBJECT_INVALID;
    m_bEngaged = 0;
    m_bMaster = 0;
    m_pCurrentAction = 0;
#if !KOTOR1
    m_nExecutingActionType = 0;
#endif
    m_oidAttackTarget = OBJECT_INVALID;
    m_nReserved2 = 0;
    m_currentAction = 0;

    memset(m_attacks, 0, sizeof(m_attacks));
}

CSWSCombatRound::~CSWSCombatRound() {
    RemoveAllActions();
    if (m_pRoundActionList) {
        delete m_pRoundActionList;
        m_pRoundActionList = 0;
    }
    if (m_pCurrentAction != 0) {
        ::operator delete(m_pCurrentAction);
        m_pCurrentAction = 0;
    }
}

int CSWSCombatRound::RemoveLastAction() {
    if (m_pRoundActionList != 0 && m_pRoundActionList->m_pInternal->m_pHead != 0) {
        void* pAction = m_pRoundActionList->RemoveTail();
        if (pAction != 0) {
            ::operator delete(pAction);
        }
        return 1;
    }
    return 0;
}

CSWSCombatRoundAction* CSWSCombatRound::GetAction() {
    if (m_pRoundActionList != 0) {
        CSWSCombatRoundAction* pAction =
            m_pRoundActionList->RemoveHead();
        if (pAction != 0 && m_nTimer >= pAction->m_nActionTimer) {
            return pAction;
        }
    }
    return 0;
}

int CSWSCombatRound::GetActionPending() {
    if (m_pRoundActionList != 0 && m_pRoundActionList->m_pInternal->m_pHead != 0) {
        CSWSCombatRoundAction* pAction = m_pRoundActionList->GetAtPos(m_pRoundActionList->m_pInternal->m_pHead);
        if (m_nTimer >= pAction->m_nActionTimer) {
            return 1;
        }
    }
    return 0;
}

int CSWSCombatRound::GetAttackActionPending() {
    if (m_pRoundActionList != 0 && m_pRoundActionList->m_pInternal->m_pHead != 0) {
        CSWSCombatRoundAction* pAction = m_pRoundActionList->GetAtPos(m_pRoundActionList->m_pInternal->m_pHead);
        if (m_nTimer >= pAction->m_nActionTimer
            && pAction->m_nActionType == CSWSCombatRoundAction::TYPE_ATTACK) {
            return 1;
        }
    }
    return 0;
}

int CSWSCombatRound::GetSpellActionPending() {
    if (m_pRoundActionList != 0) {
        CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
        if (pNode != 0) {
            CSWSCombatRoundAction* pAction =
                m_pRoundActionList->GetAtPos(pNode);
            while (pNode != 0) {
                if (pAction != 0 && m_nTimer >= pAction->m_nActionTimer
                        && pAction->m_nActionType == CSWSCombatRoundAction::TYPE_SPELL) {
                    return 1;
                }
                pAction =
                    m_pRoundActionList->GetNext(pNode);
            }
        }
    }
    return 0;
}

void CSWSCombatRound::UpdateAttackTargetForAllActions(unsigned long long objectId) {
    if (m_pRoundActionList != 0) {
        CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
        while (pNode != 0) {
            CSWSCombatRoundAction* pAction =
                m_pRoundActionList->GetAtPos(pNode);
            if (pAction->m_nActionType == CSWSCombatRoundAction::TYPE_ATTACK
                && pAction->m_bRetargettable == 1) {
                pAction->m_oidTarget = objectId;
            }
            m_pRoundActionList->GetNext(pNode);
        }
    }
}

void CSWSCombatRound::RemoveSpellAction() {
    if (m_pRoundActionList != 0) {
        CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
        while (pNode != 0) {
            CSWSCombatRoundAction* pAction =
                m_pRoundActionList->GetAtPos(pNode);
            if (pAction != 0 && pAction->m_nActionType == CSWSCombatRoundAction::TYPE_SPELL) {
                CExoLinkedListNode* pDoomed = pNode;
                m_pRoundActionList->GetNext(pNode);
                m_pRoundActionList->Remove(pDoomed);
                ::operator delete(pAction);
            } else {
                m_pRoundActionList->GetNext(pNode);
            }
        }
    }
}

int CSWSCombatRound::GetWeaponAttackType() {
    int bCreatureWeapons = HasCreatureWeapons();
    int nOffHandAttack = m_offHandAttack;
    int bExtraAttack = GetExtraAttack();
    if (nOffHandAttack == 0 && bCreatureWeapons == 0 && bExtraAttack == 0) {
        return m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0
                   ? WEAPON_ATTACK_TYPE_UNARMED : WEAPON_ATTACK_TYPE_ONHAND;
    }
    if (bCreatureWeapons == 0 && nOffHandAttack != 0) {
        return WEAPON_ATTACK_TYPE_OFFHAND;
    }
    if (bCreatureWeapons == 0 && bExtraAttack != 0) {
        return m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0
                   ? WEAPON_ATTACK_TYPE_UNARMED_EXTRA : WEAPON_ATTACK_TYPE_EXTRA;
    }
    if (bCreatureWeapons != 0
        && m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L) != 0) {
        return WEAPON_ATTACK_TYPE_CWEAPON1;
    }
    return WEAPON_ATTACK_TYPE_NONE;
}

CSWSItem* CSWSCombatRound::GetCurrentAttackWeapon(int nAttackType) {
    if (nAttackType == WEAPON_ATTACK_TYPE_NONE) {
        nAttackType = GetWeaponAttackType();
    }
    switch (nAttackType) {
    case WEAPON_ATTACK_TYPE_ONHAND:
    case WEAPON_ATTACK_TYPE_EXTRA:
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    case WEAPON_ATTACK_TYPE_OFFHAND: {
        CSWSItem* pWeapon =
                static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pWeapon != 0
                && g_pRules->GetBaseItem(pWeapon->m_baseItem)
                       ->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
            return pWeapon;
        }
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    }
    case WEAPON_ATTACK_TYPE_CWEAPON1:
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L));
    case WEAPON_ATTACK_TYPE_CWEAPON2:
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_R));
    case WEAPON_ATTACK_TYPE_CWEAPON3:
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_B));
    }
    // The unarmed attack types fight with whatever is in the hands slot.
    if (nAttackType == WEAPON_ATTACK_TYPE_UNARMED || nAttackType == WEAPON_ATTACK_TYPE_UNARMED_EXTRA) {
        return static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
    }
    return 0;
}

int CSWSCombatRound::ResolveMasterState(unsigned long long objectId, int& bCheckMaster)
{
    if (bCheckMaster == 0)
        return 0;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject == 0 || pObject->AsSWSCreature() == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int bResult = pCreature->m_attemptedAttackTargetId == OBJECT_INVALID;
    if (pCreature->m_attemptedAttackTargetId == m_pCreature->m_objectId) {
        if (pCreature->m_pCombatRound->m_bMaster == 0)
            bResult = 1;
    }
    if (m_pCreature->m_bInParty != 0)
        return 1;

    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pLeader != 0)
        pLeader->GetServerCreature();
    return bResult;
}

int CSWSCombatRound::ResolveChoreographedState(unsigned long long objectId)
{
    CSWSItem* pWeapon =
            static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pWeapon != 0) {
        unsigned char nWield = pWeapon->GetBaseItem()->m_weaponWieldType;
        if (pWeapon->GetBaseItem()->m_bRangedWeapon != 0 || nWield == 1 || nWield == 8)
            return 0;

        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
        if (pObject == 0 || pObject->AsSWSCreature() == 0)
            return 0;

        CSWSItem* pOther =
            static_cast<CSWSItem*>(pObject->AsSWSCreature()->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pOther == 0)
            return 0;

        unsigned char nOtherWield = pOther->GetBaseItem()->m_weaponWieldType;
        if (pOther->GetBaseItem()->m_bRangedWeapon != 0 || nOtherWield == 1 || nOtherWield == 8)
            return 0;
        return 1;
    }

#if KOTOR1  // K1 has no unarmed choreography
    return 0;
#else
    if (m_pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS) == 0)
        return 0;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject == 0 || pObject->AsSWSCreature() == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) != 0)
        return 0;
    return pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS) != 0;
#endif
}

int CSWSCombatRound::ResolveEngagedState(unsigned long long objectId)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject == 0 || pObject->AsSWSCreature() == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int bInCombat = pCreature->m_pCombatRound->m_bRoundStarted;
    unsigned long long idAttack = pCreature->m_attemptedAttackTargetId;

    int bEngaged = 0;
    if (bInCombat == 0 && idAttack == OBJECT_INVALID)
        bEngaged = 1;
    if (idAttack == m_pCreature->m_objectId || pCreature->m_oidAttemptedSpellTarget == m_pCreature->m_objectId)
        bEngaged = 1;
    if (bInCombat != 0 && idAttack == OBJECT_INVALID)
        bEngaged = 1;

    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pLeader != 0 && pLeader->GetServerCreature()->m_objectId == objectId) {
        if (pCreature->m_attemptedAttackTargetId != OBJECT_INVALID &&
            pCreature->m_attemptedAttackTargetId != m_pCreature->m_objectId) {
            bEngaged = 0;
        } else {
            if (pCreature->m_attemptedAttackTargetId == OBJECT_INVALID &&
                pCreature->m_oidAttemptedSpellTarget == OBJECT_INVALID)
                bEngaged = 0;
            if (pCreature->m_oidAttemptedSpellTarget != OBJECT_INVALID &&
                pCreature->m_oidAttemptedSpellTarget != m_pCreature->m_objectId)
                bEngaged = 0;
            else if (pCreature->m_oidPendingAttackTarget != OBJECT_INVALID &&
                     pCreature->m_oidPendingAttackTarget != m_pCreature->m_objectId)
                bEngaged = 0;
        }
    }

    if (pCreature->IsDebilitated(1) != 0)
        return 0;
    return bEngaged;
}

void CSWSCombatRound::DecrementPauseTimer(int nDelta)
{
    CExoString sWarning;

    if (g_pAppManager->m_pServerExoApp->GetGameObject(m_oidMaster) != 0) {
        m_nPauseTimer -= nDelta;
        if (m_nPauseTimer <= 0 && m_bInfinitePause == 0) {
            m_nPauseTimer = 0;
            m_bRoundPaused = 0;
            m_oidPauseSource = OBJECT_INVALID;
        }
    } else {
        sWarning.Format(const_cast<char*>("CSWSCombatRound::DecrementPauseTimer - %s Master cannot be found "
                               "expire the round; Resetting"),
                        m_pCreature->m_sTag.CStr());
        m_oidMaster = OBJECT_INVALID;
        EndCombatRound(1);
    }
}

void CSWSCombatRound::DecrementRoundLength(int nTime, int bAllowShorten)
{
    if (m_bRoundStarted == 0)
        return;

    if (m_nRoundLength < nTime) {
        if (1000 - m_nOverlapAmount >= nTime - m_nRoundLength)
            m_nRoundLength = nTime + 1;
        else if (bAllowShorten != 0)
            m_nRoundLength = nTime;
    }

    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSCombatRoundAction* pAction =
            m_pRoundActionList->GetAtPos(pNode);
        int nTimer = 0;
        if (m_nRoundLength > 0)
            nTimer = static_cast<int>(static_cast<float>(pAction->m_nActionTimer) / static_cast<float>(m_nRoundLength) *
                           static_cast<float>(m_nRoundLength - nTime));
        pAction->m_nActionTimer = nTimer;
        m_pRoundActionList->GetNext(pNode);
    }

    float fScale = 1.0f;
    if (m_nRoundLength != 0)
        fScale = static_cast<float>(m_nTimer) / static_cast<float>(m_nRoundLength);
    m_nTimer = static_cast<int>(fScale * static_cast<float>(m_nRoundLength - nTime));
    if (m_nRoundLength - nTime < 0)
        m_nRoundLength = nTime;
    m_nRoundLength = m_nRoundLength - nTime;
}

void CSWSCombatRound::DEBUGDumpCombatRoundToScreen(int nMode, const CExoString& sLabel)
{
    const char* pszLabel = sLabel.CStr();
    K2_UNUSED(nMode);
    (void)pszLabel;

    CExoLinkedListNode* pNode = m_pRoundActionList->m_pInternal->m_pHead;
    CSWSCombatRoundAction* pAction = 0;
    if (pNode != 0)
        pAction = m_pRoundActionList->GetAtPos(pNode);
    while (pAction != 0)
        pAction = m_pRoundActionList->GetNext(pNode);
}

int CSWSCombatRound::GetCombatStepRequired(unsigned long long objectId)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject != 0 && pObject->AsSWSObject() != 0 && pObject->AsSWSObject()->GetDead() == 0) {
#if KOTOR1  // K1 also tests GetIsPCDying
        if (pObject->AsSWSObject()->GetIsPCDying() == 0 && (m_pCreature->m_nAIState & CSWSCreature::AI_STATE_MOBILE) != 0) {
#else
        pObject->AsSWSObject();
        if ((m_pCreature->m_nAIState & CSWSCreature::AI_STATE_MOBILE) != 0) {
#endif
            CSWSObject* pTarget = pObject->AsSWSObject();
            CSWSItem* pWeapon =
                    static_cast<CSWSItem*>(m_pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pWeapon == 0 ||
                g_pRules->GetBaseItem(pWeapon->m_baseItem)->m_bRangedWeapon == 0) {
                float fRange = m_pCreature->DesiredAttackRange(objectId, 0);
                Vector vDelta = m_pCreature->m_position - pTarget->m_position;
                float fDist2 =
                    vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
                if (fDist2 < (fRange - 0.25f) * (fRange - 0.25f) ||
                    fDist2 > (fRange + 0.25f) * (fRange + 0.25f))
                    return 1;
            }
        }
    }
    return 0;
}

void CSWSCombatRound::AddSwitchWeaponsAction() {
    CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
    pAction->m_nActionType = CSWSCombatRoundAction::TYPE_SWITCH_WEAPONS;
    pAction->m_nAnimationTime = 1000;
    pAction->m_nActionParam = 0;
    AddAction(pAction, -1);
}

int CSWSCombatRound::SaveCombatRound(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct elem;
    CExoLinkedListNode* pNode;

    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bRoundStarted), const_cast<char*>("RoundStarted"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bSpellCastRound), const_cast<char*>("SpellCastRound"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_deflectArrow), const_cast<char*>("DeflectArrow"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bWeaponSucks), const_cast<char*>("WeaponSucks"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidDodgeTarget), const_cast<char*>("DodgeTarget"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidPendingTargetSwap), const_cast<char*>("NewAttackTarget"));
    pGFF->WriteFieldINT(pStruct, m_bEngaged, const_cast<char*>("Engaged"));
    pGFF->WriteFieldINT(pStruct, m_bMaster, const_cast<char*>("Master"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidMaster), const_cast<char*>("MasterID"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bRoundPaused), const_cast<char*>("RoundPaused"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidPauseSource), const_cast<char*>("RoundPausedBy"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bInfinitePause), const_cast<char*>("InfinitePause"));
    pGFF->WriteFieldINT(pStruct, m_nPauseTimer, const_cast<char*>("PauseTimer"));
    pGFF->WriteFieldINT(pStruct, m_nTimer, const_cast<char*>("Timer"));
    pGFF->WriteFieldINT(pStruct, m_nRoundLength, const_cast<char*>("RoundLength"));
    pGFF->WriteFieldINT(pStruct, m_nOverlapAmount, const_cast<char*>("OverlapAmount"));
    pGFF->WriteFieldINT(pStruct, m_nBleedTimer, const_cast<char*>("BleedTimer"));
    pGFF->WriteFieldBYTE(pStruct, m_currentAttack, const_cast<char*>("CurrentAttack"));
    pGFF->WriteFieldWORD(pStruct, m_newAttackId, const_cast<char*>("AttackID"));
    pGFF->WriteFieldBYTE(pStruct, m_nAttackGroupCounter, const_cast<char*>("AttackGroup"));
    pGFF->WriteFieldINT(pStruct, m_nParryIndex, const_cast<char*>("ParryIndex"));
    pGFF->WriteFieldINT(pStruct, m_nNumAOOs, const_cast<char*>("NumAOOs"));
    pGFF->WriteFieldINT(pStruct, m_nNumCleaves, const_cast<char*>("NumCleaves"));
    pGFF->WriteFieldINT(pStruct, m_nOnHandAttacks, const_cast<char*>("OnHandAttacks"));
    pGFF->WriteFieldINT(pStruct, m_nOffHandAttacks, const_cast<char*>("OffHandAttacks"));
    pGFF->WriteFieldINT(pStruct, m_nAdditAttacks, const_cast<char*>("AdditAttacks"));
    pGFF->WriteFieldINT(pStruct, m_bonusEffectAttacks, const_cast<char*>("EffectAttacks"));
    pGFF->WriteFieldBYTE(pStruct, m_nParryActions, const_cast<char*>("ParryActions"));
    pGFF->WriteFieldINT(pStruct, m_nOffHandTaken, const_cast<char*>("OffHandTaken"));
    pGFF->WriteFieldINT(pStruct, m_nExtraTaken, const_cast<char*>("ExtraTaken"));

    pGFF->AddList(&list, pStruct, const_cast<char*>("AttackList"));
    for (int i = 0; i < 5; i++) {
        pGFF->AddListElement(&elem, &list, GFF_STRUCT_ATTACK);
        m_attacks[i].SaveData(pGFF, &elem);
    }

    pGFF->AddList(&list, pStruct, const_cast<char*>("SpecAttackList"));
    for (int i = 0; i < m_specialAttacks.m_count; i++) {
        pGFF->AddListElement(&elem, &list, GFF_STRUCT_SPECIAL_ATTACK);
        pGFF->WriteFieldWORD(&elem, m_specialAttacks.m_pArray[i], const_cast<char*>("SpecialAttack"));
    }

    pGFF->AddList(&list, pStruct, const_cast<char*>("SpecAttackIdList"));
    for (int i = 0; i < m_specialAttackIds.m_count; i++) {
        pGFF->AddListElement(&elem, &list, GFF_STRUCT_SPECIAL_ATTACK_ID);
        pGFF->WriteFieldWORD(&elem, m_specialAttackIds.m_pArray[i], const_cast<char*>("SpecialAttackId"));
    }

    pGFF->AddList(&list, pStruct, const_cast<char*>("SchedActionList"));
    pNode = m_pRoundActionList->m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSCombatRoundAction* pAction =
            m_pRoundActionList->GetAtPos(pNode);
        if (pAction != 0) {
            pGFF->AddListElement(&elem, &list, GFF_STRUCT_ROUND_ACTION);
            pAction->SaveData(pGFF, &elem);
        }
        m_pRoundActionList->GetNext(pNode);
    }
    return 1;
}

int CSWSCombatRound::LoadCombatRound(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    int bSuccess;
    CResStruct elem;

    if ((m_bRoundStarted = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("RoundStarted"), bSuccess,
                                          static_cast<unsigned char>(m_bRoundStarted))) == 0)
        return 0;
    m_bSpellCastRound = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("SpellCastRound"), bSuccess, static_cast<unsigned char>(m_bSpellCastRound));
    m_deflectArrow = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DeflectArrow"), bSuccess, static_cast<unsigned char>(m_deflectArrow));
    m_bWeaponSucks = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("WeaponSucks"), bSuccess, static_cast<unsigned char>(m_bWeaponSucks));
    m_oidDodgeTarget = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("DodgeTarget"), bSuccess, static_cast<unsigned int>(m_oidDodgeTarget));
    m_oidPendingTargetSwap = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("NewAttackTarget"), bSuccess,
                                                  static_cast<unsigned int>(m_oidPendingTargetSwap));
    m_bRoundPaused = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("RoundPaused"), bSuccess, static_cast<unsigned char>(m_bRoundPaused));
    m_oidPauseSource = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("RoundPausedBy"), bSuccess,
                                            static_cast<unsigned int>(m_oidPauseSource));
    m_bInfinitePause = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("InfinitePause"), bSuccess, static_cast<unsigned char>(m_bInfinitePause));
    m_nPauseTimer = pGFF->ReadFieldINT(pStruct, const_cast<char*>("PauseTimer"), bSuccess, m_nPauseTimer);
    m_bInfinitePause = 0;
    m_bEngaged = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Engaged"), bSuccess, 0);
    m_bMaster = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Master"), bSuccess, 0);
    m_oidMaster = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("MasterID"), bSuccess, OBJECT_INVALID);
    m_nTimer = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Timer"), bSuccess, m_nTimer);
    m_nRoundLength = pGFF->ReadFieldINT(pStruct, const_cast<char*>("RoundLength"), bSuccess, m_nRoundLength);
    m_nOverlapAmount = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OverlapAmount"), bSuccess, m_nOverlapAmount);
    m_nBleedTimer = pGFF->ReadFieldINT(pStruct, const_cast<char*>("BleedTimer"), bSuccess, m_nBleedTimer);
    m_currentAttack = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CurrentAttack"), bSuccess, m_currentAttack);
    m_newAttackId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("AttackID"), bSuccess, m_newAttackId);
    m_nAttackGroupCounter = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AttackGroup"), bSuccess,
                                                m_nAttackGroupCounter);
    m_nParryIndex = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ParryIndex"), bSuccess, m_nParryIndex);
    m_nNumAOOs = pGFF->ReadFieldINT(pStruct, const_cast<char*>("NumAOOs"), bSuccess, m_nNumAOOs);
    m_nNumCleaves = pGFF->ReadFieldINT(pStruct, const_cast<char*>("NumCleaves"), bSuccess, m_nNumCleaves);
    m_nOnHandAttacks = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OnHandAttacks"), bSuccess,
                                          m_nOnHandAttacks);
    m_nOffHandAttacks = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OffHandAttacks"), bSuccess,
                                           m_nOffHandAttacks);
    m_nAdditAttacks = pGFF->ReadFieldINT(pStruct, const_cast<char*>("AdditAttacks"), bSuccess, m_nAdditAttacks);
    pGFF->ReadFieldINT(pStruct, const_cast<char*>("EffectAttacks"), bSuccess, m_bonusEffectAttacks);
    m_bonusEffectAttacks = 0;
    m_nParryActions = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("ParryActions"), bSuccess, m_nParryActions);
    m_nOffHandTaken = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OffHandTaken"), bSuccess, m_nOffHandTaken);
    m_nExtraTaken = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ExtraTaken"), bSuccess, m_nExtraTaken);

    if (pGFF->GetList(&list, pStruct, const_cast<char*>("AttackList")) == 1) {
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list)); i++) {
            if (pGFF->GetListElement(&elem, &list, i) == 1)
                m_attacks[i].LoadData(pGFF, &elem);
        }
    }

    if (pGFF->GetList(&list, pStruct, const_cast<char*>("SpecAttackList")) == 1) {
        m_specialAttacks.SetSize(pGFF->GetListCount(&list));
        for (int i = 0; i < m_specialAttacks.m_count; i++) {
            if (pGFF->GetListElement(&elem, &list, i) == 1)
                m_specialAttacks.m_pArray[i] =
                    pGFF->ReadFieldWORD(&elem, const_cast<char*>("SpecialAttack"), bSuccess, 0);
        }
    }

    if (pGFF->GetList(&list, pStruct, const_cast<char*>("SpecAttackIdList")) == 1) {
        m_specialAttackIds.SetSize(pGFF->GetListCount(&list));
        // Bounded by the SpecAttackList count above rather than this list's count; copied from
        // the block above.
        for (int i = 0; i < m_specialAttacks.m_count; i++) {
            if (pGFF->GetListElement(&elem, &list, i) == 1)
                m_specialAttackIds.m_pArray[i] =
                    pGFF->ReadFieldWORD(&elem, const_cast<char*>("SpecialAttackId"), bSuccess, 0);
        }
    }

    if (pGFF->GetList(&list, pStruct, const_cast<char*>("SchedActionList")) == 1) {
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list)); i++) {
            if (pGFF->GetListElement(&elem, &list, i) == 1) {
                CSWSCombatRoundAction* pAction = new CSWSCombatRoundAction;
                pAction->LoadData(pGFF, &elem);
                m_pRoundActionList->AddTail(pAction);
            }
        }
    }
    return 1;
}

// One attack's data within a combat round.

CSWSCombatAttackData::CSWSCombatAttackData()
    : m_vMissLocation{}
{
    ClearAttackData();
}

void CSWSCombatAttackData::ClearAttackData()
{
    m_nAttackGroup = 0xff;
    m_bAttackRolled = 0;
    m_nAnimationLength = 0;
    m_targetObjectId = OBJECT_INVALID;
    m_nReaxnDelay = 0;
    m_nReaxnAnimation = SERVER_ANIMATION_READY;
    m_nReaxnAnimLength = 0;
    m_nMissedBy = 0;
    for (int i = 0; i < 15; i++)
        m_anDamage[i] = -1;
    m_nWeaponAttackType = CSWSCombatRound::WEAPON_ATTACK_TYPE_NONE;
    m_nAttackMode = CSWSCreature::CREATURE_COMBAT_MODE_NONE;
    m_nConcealment = 0;
    m_bRangedAttack = 0;
    m_bSneakAttack = 0;
    m_bKillingBlow = 0;
    m_bCoupDeGrace = 0;
    m_bCriticalThreat = 0;
    m_nReserved1 = 0;
    m_bAttackDeflected = 0;
    m_nAttackResult = 0;
    m_vMissLocation.x = 0.0f;
    m_vMissLocation.y = 0.0f;
    m_vMissLocation.z = 0.0f;
    m_nReserved2 = 0;
    m_nFeatId = 0;
    m_nSpecialAttackId = 0xff;
    m_oidAmmoItem = OBJECT_INVALID;
    m_sAttackDebugText = "";
    m_sDamageDebugText = "";
    // This attack owns its on-hit effects, so clearing deletes them. The other lists are
    // drained by their consumers, which take ownership.
    for (int64_t i = 0; i < m_effects.m_count; i++)
    {
        delete m_effects.m_pArray[i];
    }
    m_effects.m_count = 0;
    m_onHitSpellImpacts.m_count = 0;
    m_messages.m_count = 0;
    m_subAttacks.m_count = 0;
    m_bForcedAnimation = 0;
    m_nForcedAnimation = 0;
    m_attack.Clear();
    m_threat.Clear();
    m_defense.Clear();
    m_damage.Clear();
    m_effect.Clear();
    m_deflect.Clear();
}

CSWSCombatAttackData::~CSWSCombatAttackData()
{
}

void CSWSCombatSubAttackAttack::Clear()
{
    m_nReserved1 = 0;
    m_nAttackRoll = 0;
    m_nAttackModifier = 0;
    m_nStrAttackMod = 0;
    m_nDexAttackMod = 0;
    m_nReserved2 = 0;
    m_nSpecialAttackBonus = 0;
    m_nBaseAttackBonus = 0;
    m_bOffhandAttack = 0;
    m_nTwoWeaponPenalty = 0;
    m_nLightOffhandBonus = 0;
    m_nTwoWeaponFeatBonus = 0;
    m_nTwoWeaponFeat = 0;
    m_nDuelingBonus = 0;
    m_nDuelingFeat = 0;
    m_nGoodAimBonus = 0;
    m_nOffensiveTrainingBonus = 0;
    m_nWeaponFocusBonus = 0;
    m_nEffectAttackBonus = 0;
    m_bAutomaticHit = 0;
    m_bAutomaticMiss = 0;
#if !KOTOR1
    m_nBattleMeditationBonus = 0;
    m_nTargetingFeat = 0;
    m_nTargetingBonus = 0;
    m_nCrushOppositionSpell = 0;
    m_nCrushOppositionPenalty = 0;
    m_nInspireFollowersSpell = 0;
    m_nInspireFollowersBonus = 0;
    m_nPCCharismaBonus = 0;
    m_nSaberFormId = 0;
    m_nSaberFormBonus = 0;
#endif
}

void CSWSCombatSubAttackThreat::Clear()
{
    m_nThreatRange = 0;
    m_bCriticalThreat = 0;
    m_nConfirmRoll = 0;
    m_bCriticalConfirmed = 0;
    m_nReserved1 = 0;
    m_nCriticalMultiplier = 0;
}

void CSWSCombatSubAttackDefense::Clear()
{
    m_nACDebilitatedPenalty = 0;
    m_nArmorClass = 0;
    m_nACArmor = 0;
    m_nACDex = 0;
    m_nACDexUncapped = 0;
    m_nACClass = 0;
    m_nACNatural = 0;
    m_nACDodge = 0;
    m_nACFeatMod = 0;
}

void CSWSCombatSubAttackDamage::Clear()
{
    m_nDamageRoll = 0;
    m_nDamageBonus = 0;
    m_nReserved1 = 0;
    m_nSneakAttackDamage = 0;
    m_nStrDamageBonus = 0;
    m_nWeaponSpecializationBonus = 0;
    m_nFeatSoakAmount = 0;
    m_nFeatFlatDRAmount = 0;
    m_nDamageTypeBits = 0;
    m_nDamageAfterSoak = 0;
    m_nDamageAfterFlatDR = 0;
#if KOTOR1
    m_nEffectDamageBonus = 0;
#else
    m_nSpellDamageReduction = 0;
    m_nReductionSpellId = FORM_SABER_I_SHII_CHO;
    m_nEffectDamageBonus = 0;
#endif
    for (int i = 0; i < 15; i++)
        m_anReserved[i] = -1;
}

void CSWSCombatSubAttackEffect::Clear()
{
    m_objectId = OBJECT_INVALID;
    m_nSaveType = 0;
    m_nFeedbackEffectType = EFFECT_STATE_NONE;
    m_nSaveVsType = 0;
    m_nSaveRoll = 0;
    m_nSaveEffectBonus = 0;
    m_nSaveBase = 0;
    m_nSaveTotal = 0;
    m_nSaveDC = 0;
    m_nSaveResult = -1;
    m_bFeedbackEffectData = 0;
    m_bHasApplicationData = 0;
}

void CSWSCombatSubAttackDeflect::Clear()
{
#if KOTOR1
    m_nDeflectRoll = 0;
    m_nAttackTotal = 0;
    m_nDeflectFeat = 0;
    m_nDeflectFeatBonus = 0;
    m_nEffectBonus = 0;
    m_nDeflectResult = 0;
    m_nBaseAttackBonus = 0;
    m_nArmourPenalty = 0;
    m_nDeflectTotal = 0;
#else
    m_nDeflectRoll = 0;
    m_nDeflectTotal = 0;
    m_nPowerBonus = 0;
    m_nPowerId = 0;
    m_nAttackTotal = 0;
    m_nDeflectFeat = 0;
    m_nDeflectFeatBonus = 0;
    m_nEffectBonus = 0;
    m_nDeflectResult = 0;
    m_nBaseAttackBonus = 0;
    m_nArmourPenalty = 0;
    m_nStanceBonus = 0;
    m_nStanceId = 0;
    m_nClassFeat = 0;
    m_nClassFeatBonus = 0;
    m_nAttackerFeat = 0;
    m_nAttackerFeatPenalty = 0;
#endif
}

int CSWSCombatAttackData::GetBaseDamage()
{
    return m_anDamage[14];
}

void CSWSCombatAttackData::SetBaseDamage(int amount)
{
    short cur = m_anDamage[14];
    if (cur < 0)
        cur = 0;
    m_anDamage[14] = static_cast<short>(cur + amount);
}

void CSWSCombatAttackData::Copy(CSWSCombatAttackData* pSrc)
{
    m_nAttackGroup = pSrc->m_nAttackGroup;
    m_nAnimationLength = pSrc->m_nAnimationLength;
    m_nMissedBy = pSrc->m_nMissedBy;
    for (int i = 0; i < 15; i++)
        m_anDamage[i] = pSrc->m_anDamage[i];
    m_nAttackResult = pSrc->m_nAttackResult;
    m_nReserved2 = pSrc->m_nReserved2;
    m_targetObjectId = pSrc->m_targetObjectId;
    m_nReaxnDelay = pSrc->m_nReaxnDelay;
    m_nReaxnAnimation = pSrc->m_nReaxnAnimation;
    m_nReaxnAnimLength = pSrc->m_nReaxnAnimLength;
    m_nConcealment = pSrc->m_nConcealment;
    m_nFeatId = pSrc->m_nFeatId;
    m_nAttackMode = pSrc->m_nAttackMode;
    m_nSpecialAttackId = pSrc->m_nSpecialAttackId;
    m_vMissLocation = pSrc->m_vMissLocation;
    m_nWeaponAttackType = pSrc->m_nWeaponAttackType;
    m_bRangedAttack = pSrc->m_bRangedAttack;
    m_bSneakAttack = pSrc->m_bSneakAttack;
    m_bKillingBlow = pSrc->m_bKillingBlow;
    m_bCoupDeGrace = pSrc->m_bCoupDeGrace;
    m_bCriticalThreat = pSrc->m_bCriticalThreat;
    m_nReserved1 = pSrc->m_nReserved1;
    m_bAttackDeflected = pSrc->m_bAttackDeflected;
    m_oidAmmoItem = pSrc->m_oidAmmoItem;
    m_bForcedAnimation = pSrc->m_bForcedAnimation;
    m_nForcedAnimation = pSrc->m_nForcedAnimation;
}

int CSWSCombatAttackData::GetDamage(unsigned short nFlags)
{
    if (nFlags & DAMAGE_TYPE_ACID) return m_anDamage[4];
    if (nFlags & DAMAGE_TYPE_COLD) return m_anDamage[5];
    if (nFlags & DAMAGE_TYPE_LIGHT_SIDE) return m_anDamage[6];
    if (nFlags & DAMAGE_TYPE_ELECTRICAL) return m_anDamage[7];
    if (nFlags & DAMAGE_TYPE_FIRE) return m_anDamage[8];
    if (nFlags & DAMAGE_TYPE_UNIVERSAL) return m_anDamage[3];
    if (nFlags & DAMAGE_TYPE_DARK_SIDE) return m_anDamage[9];
    if (nFlags & DAMAGE_TYPE_SONIC) return m_anDamage[10];
    if (nFlags & DAMAGE_TYPE_ION) return m_anDamage[11];
    if (nFlags & DAMAGE_TYPE_BLASTER) return m_anDamage[12];
    if (nFlags & DAMAGE_TYPE_BLUDGEONING) return m_anDamage[0];
    if (nFlags & DAMAGE_TYPE_PIERCING) return m_anDamage[1];
    if (nFlags & DAMAGE_TYPE_SLASHING) return m_anDamage[2];
    if (nFlags & DAMAGE_TYPE_POISON) return m_anDamage[13];
    return 0;
}

void CSWSCombatAttackData::SetDamage(unsigned short nFlags, int nAmount)
{
    if (nAmount < 0)
        nAmount = 0;
    m_anDamage[Log_Base2(nFlags)] = static_cast<short>(nAmount);
}

void CSWSCombatAttackData::AddDamage(unsigned short nFlags, int nAmount, int bAddToBase)
{
    int nIndex = Log_Base2(nFlags);
    short cur = m_anDamage[nIndex];
    if (cur > 0)
    {
        int nTotal = cur + nAmount;
        if (nTotal < 1)
            nTotal = 1;
        m_anDamage[nIndex] = static_cast<short>(nTotal);
    }
    else
    {
        SetDamage(nFlags, nAmount);
    }
    if (bAddToBase)
        SetBaseDamage(nAmount);
}

int CSWSCombatAttackData::GetTotalDamage(int a)
{
    K2_UNUSED(a);
    int nTotal = 0;
    for (int i = 0; i < 14; i++)
    {
        if (m_anDamage[i] >= 0)
            nTotal += m_anDamage[i];
    }
    return nTotal;
}

int CSWSCombatAttackData::SaveData(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldBYTE(pStruct, m_nAttackGroup, const_cast<char*>("AttackGroup"));
    pGFF->WriteFieldWORD(pStruct, m_nAnimationLength, const_cast<char*>("AnimationLength"));
    pGFF->WriteFieldDWORD(pStruct, m_nMissedBy, const_cast<char*>("MissedBy"));
    pGFF->WriteFieldBYTE(pStruct, m_nAttackResult, const_cast<char*>("AttackResult"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_targetObjectId), const_cast<char*>("ReactObject"));
    pGFF->WriteFieldWORD(pStruct, m_nReaxnDelay, const_cast<char*>("ReaxnDelay"));
    pGFF->WriteFieldWORD(pStruct, m_nReaxnAnimation, const_cast<char*>("ReaxnAnimation"));
    pGFF->WriteFieldWORD(pStruct, m_nReaxnAnimLength, const_cast<char*>("ReaxnAnimLength"));
    pGFF->WriteFieldBYTE(pStruct, m_nConcealment, const_cast<char*>("Concealment"));
    pGFF->WriteFieldWORD(pStruct, m_nFeatId, const_cast<char*>("AttackType"));
    pGFF->WriteFieldBYTE(pStruct, m_nAttackMode, const_cast<char*>("AttackMode"));
    pGFF->WriteFieldINT(pStruct, m_bRangedAttack, const_cast<char*>("RangedAttack"));
    pGFF->WriteFieldINT(pStruct, m_bSneakAttack, const_cast<char*>("SneakAttack"));
    pGFF->WriteFieldBYTE(pStruct, m_nWeaponAttackType, const_cast<char*>("WeaponAttackType"));
    pGFF->WriteFieldFLOAT(pStruct, m_vMissLocation.x, const_cast<char*>("RangedTargetX"));
    pGFF->WriteFieldFLOAT(pStruct, m_vMissLocation.y, const_cast<char*>("RangedTargetY"));
    pGFF->WriteFieldFLOAT(pStruct, m_vMissLocation.z, const_cast<char*>("RangedTargetZ"));

    CResList damageList;
    pGFF->AddList(&damageList, pStruct, const_cast<char*>("DamageList"));
    for (int i = 0; i < 15; i++)
    {
        CResStruct element;
        pGFF->AddListElement(&element, &damageList, GFF_STRUCT_DAMAGE);
        pGFF->WriteFieldSHORT(&element, m_anDamage[i], const_cast<char*>("DamageValue"));
    }

    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bKillingBlow), const_cast<char*>("KillingBlow"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bCoupDeGrace), const_cast<char*>("CoupDeGrace"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bCriticalThreat), const_cast<char*>("CriticalThreat"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bAttackDeflected), const_cast<char*>("AttackDeflected"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidAmmoItem), const_cast<char*>("AmmoItem"));
    pGFF->WriteFieldCExoString(pStruct, m_sAttackDebugText, const_cast<char*>("AttackDebugText"));
    pGFF->WriteFieldCExoString(pStruct, m_sDamageDebugText, const_cast<char*>("DamageDebugText"));
    return 1;
}

int CSWSCombatAttackData::LoadData(CResGFF* pGFF, CResStruct* pStruct)
{
    CResStruct element;
    int bSuccess;
    m_nAttackGroup = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AttackGroup"), bSuccess, 0);
    m_nAnimationLength = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("AnimationLength"), bSuccess, 0);
    m_nMissedBy = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("MissedBy"), bSuccess, 0);
    m_nAttackResult = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AttackResult"), bSuccess, 0);
    m_targetObjectId = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("ReactObject"), bSuccess, 0);
    m_nReaxnDelay = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("ReaxnDelay"), bSuccess, 0);
    m_nReaxnAnimation = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("ReaxnAnimation"), bSuccess, 0);
    m_nReaxnAnimLength = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("ReaxnAnimLength"), bSuccess, 0);
    m_nConcealment = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Concealment"), bSuccess, 0);
    m_nFeatId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("AttackType"), bSuccess, 0);
    m_nAttackMode = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AttackMode"), bSuccess, 0);
    m_bRangedAttack = pGFF->ReadFieldINT(pStruct, const_cast<char*>("RangedAttack"), bSuccess, 0);
    m_bSneakAttack = pGFF->ReadFieldINT(pStruct, const_cast<char*>("SneakAttack"), bSuccess, 0);
    m_nWeaponAttackType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("WeaponAttackType"), bSuccess, 0);
    m_vMissLocation.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("RangedTargetX"), bSuccess, 0.0f);
    m_vMissLocation.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("RangedTargetY"), bSuccess, 0.0f);
    m_vMissLocation.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("RangedTargetZ"), bSuccess, 0.0f);

    CResList damageList;
    if (pGFF->GetList(&damageList, pStruct, const_cast<char*>("DamageList")) == 1)
    {
        for (unsigned int i = 0; i < pGFF->GetListCount(&damageList); i++)
        {
            if (pGFF->GetListElement(&element, &damageList, i) == 1)
                m_anDamage[i] = pGFF->ReadFieldSHORT(&element, const_cast<char*>("DamageValue"), bSuccess, 0);
        }
    }

    m_bKillingBlow = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("KillingBlow"), bSuccess, 0);
    m_bCoupDeGrace = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CoupDeGrace"), bSuccess, 0);
    m_bCriticalThreat = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CriticalThreat"), bSuccess, 0);
    m_bAttackDeflected = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AttackDeflected"), bSuccess, 0);
    m_oidAmmoItem = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("AmmoItem"), bSuccess, 0);
    m_sAttackDebugText = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("AttackDebugText"), bSuccess, CExoString(""));
    m_sDamageDebugText = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("DamageDebugText"), bSuccess, CExoString(""));
    return 1;
}

void CSWSCombatAttackData::ResolveAttackToMeleeSubAttack(int nAttackIndex, unsigned int nDelay)
{
    CSWSCombatSubAttack subAttack;

    subAttack.m_bRangedAttack = 0;
    subAttack.m_nAttackIndex = nAttackIndex;
    subAttack.m_nReadyDelay = nDelay;
    subAttack.m_nAttackResult = m_nAttackResult;
    subAttack.m_nWeaponAttackType = m_nWeaponAttackType;
    subAttack.m_nFeatId = m_nFeatId;

    for (int i = 0; i < 15; i++)
    {
        subAttack.m_anDamage[i] = m_anDamage[i];
        m_anDamage[i] = -1;
    }

    subAttack.m_attack = m_attack;
    subAttack.m_threat = m_threat;
    subAttack.m_defense = m_defense;
    subAttack.m_damage = m_damage;
    subAttack.m_effect = m_effect;

    m_subAttacks.Add(subAttack);
}

void CSWSCombatAttackData::AddMeleeSubAttack(unsigned int nDelay)
{
    CSWSCombatSubAttack subAttack;

    subAttack.m_bRangedAttack = 0;
    subAttack.m_nReadyDelay = nDelay;

    m_subAttacks.Add(subAttack);
}

void CSWSCombatAttackData::ResolveAttackToRangedSubAttack(int nAttackIndex, unsigned int nDelay,
                                                          int a, int b, int nProjectileType)
{
    CSWSCombatSubAttack subAttack;

    subAttack.m_bRangedAttack = 1;
    subAttack.m_nAttackIndex = nAttackIndex;
    subAttack.m_nReadyDelay = nDelay;
    subAttack.m_bAttackRolled = a;
    subAttack.m_bProjectileHit = b;
    subAttack.m_nDischargeHand = nProjectileType;
    subAttack.m_nAttackResult = m_nAttackResult;
    subAttack.m_nWeaponAttackType = m_nWeaponAttackType;
    subAttack.m_nFeatId = m_nFeatId;
    subAttack.m_vMissLocation = m_vMissLocation;
    subAttack.m_nReaxnDelay = m_nReaxnDelay;

    for (int i = 0; i < 15; i++)
    {
        subAttack.m_anDamage[i] = m_anDamage[i];
        m_anDamage[i] = -1;
    }

    subAttack.m_attack = m_attack;
    subAttack.m_threat = m_threat;
    subAttack.m_defense = m_defense;
    subAttack.m_damage = m_damage;
    subAttack.m_effect = m_effect;

    m_subAttacks.Add(subAttack);
}

void CSWSCombatAttackData::AddRangedSubAttack(int nAttackIndex, unsigned char nAttackResult,
                                              unsigned int nDelay, int a, Vector vMissLocation,
                                              unsigned int nReaxnDelay, int nProjectileType)
{
    CSWSCombatSubAttack subAttack;

    subAttack.m_bRangedAttack = 1;
    subAttack.m_nAttackIndex = nAttackIndex;
    subAttack.m_nAttackResult = nAttackResult;
    subAttack.m_nReadyDelay = nDelay;
    subAttack.m_bProjectileHit = a;
    subAttack.m_nReaxnDelay = nReaxnDelay;
    subAttack.m_vMissLocation = vMissLocation;
    subAttack.m_nDischargeHand = nProjectileType;

    m_subAttacks.Add(subAttack);
}

CSWSCombatSubAttack CSWSCombatAttackData::GetTopSubAttack()
{
    CSWSCombatSubAttack subAttack;
    if (m_subAttacks.m_count > 0)
        memcpy(static_cast<void*>(&subAttack), &m_subAttacks.m_pArray[0], sizeof(CSWSCombatSubAttack));
    return subAttack;
}

void CSWSCombatAttackData::RemoveTopSubAttack()
{
    if (m_subAttacks.m_count > 0)
    {
        // inlined CExoArrayList<CSWSCombatSubAttack>::DelIndex(0)
        m_subAttacks.m_count--;
        for (int64_t i = 0; i < m_subAttacks.m_count; i++)
        {
            memcpy(static_cast<void*>(&m_subAttacks.m_pArray[i]), &m_subAttacks.m_pArray[i + 1],
                   sizeof(CSWSCombatSubAttack));
        }
    }
}

void CSWSCombatAttackData::ClearSubAttacks()
{
    while (m_subAttacks.m_count > 0)
        RemoveTopSubAttack();
}

void CSWSCombatAttackData::ResolveSubAttackToAttack(const CSWSCombatSubAttack& subAttack)
{
    m_nAttackResult = subAttack.m_nAttackResult;
    m_nFeatId = subAttack.m_nFeatId;
    m_nWeaponAttackType = subAttack.m_nWeaponAttackType;
    m_vMissLocation = subAttack.m_vMissLocation;
    m_nReaxnDelay = static_cast<unsigned short>(subAttack.m_nReaxnDelay);
    m_bAttackRolled = subAttack.m_bAttackRolled;
    for (int i = 0; i < 15; i++)
        m_anDamage[i] = subAttack.m_anDamage[i];
    m_attack = subAttack.m_attack;
    m_threat = subAttack.m_threat;
    m_defense = subAttack.m_defense;
    m_damage = subAttack.m_damage;
    m_effect = subAttack.m_effect;
}

void CSWSCombatAttackData::ResolveTopSubAttackToAttack()
{
    ResolveSubAttackToAttack(GetTopSubAttack());
}

int CSWSCombatAttackData::IsSubAttackReady(int nTime)
{
    CSWSCombatSubAttack subAttack = GetTopSubAttack();
    return static_cast<unsigned int>(m_nAnimationLength - nTime) > subAttack.m_nReadyDelay;
}

CSWSCombatSubAttack::CSWSCombatSubAttack()
    : m_vMissLocation{}
{
}

CSWSCombatSubAttack::~CSWSCombatSubAttack()
{
}
