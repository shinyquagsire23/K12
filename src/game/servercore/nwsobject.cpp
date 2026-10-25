#include "k2stdint.h"
#include "nwsobject.h"
#include "resgff.h"
#include "nwmessagetypes.h"
#include "nwseffect.h"
#include "nwscript.h"
#include "nwsdialog.h"
#include "exotlktable.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "nwsarea.h"
#include "gameobjectarray.h"
#include "exobase.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "Quaternion.h"
#include "nwseffectlist.h"
#include "serverexoapp.h"
#include "worldtimer.h"
#include "nwcobject.h"
#include "nwscreature.h"
#include "nwsplaceable.h"
#include "nwsitem.h"
#include "exolinkedlistinternal.h"
#include "CExoLinkedList.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "nwsplayerluo.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwsaiaction.h"
#include "nwscriptconst.h"
#include "nwcprojectile.h"
#include "enginevfx.h"
#include "serveranimation.h"
#include "restype.h"

// feat.2da rows nwscript.nss does not name.
enum {
    FEAT_IMPROVED_TOUGHNESS = 123,
};
#include "ClientExoAppInternal.h"
#include "clientoptions.h"
#include "nwcmodule.h"

const int CSWSObject::ACTION_IN_PROGRESS = 1;
const int CSWSObject::ACTION_COMPLETE = 2;
const int CSWSObject::ACTION_FAILED = 3;
const int CSWSObject::ACTION_IN_DELAY = 4;
const int CSWSObject::AI_TIME_SLICE = 1000;

extern bool s_oondarDone;
extern bool AspyrHackCarthDialog;
extern int g_bShowIndicator;
#include "nwsspellimpact.h"
#include "nwmessage.h"
#include "nwspell.h"
#include "CExoArrayListImpl.h"
#include "nwscombatround.h"
#include "swparty.h"
#include "pathfindinfo.h"

#include "port/libcmath.h"
#include "port/crtapi.h"

CSWSObjectActionNode::CSWSObjectActionNode()
{
    m_nNumParameters = 0;
    for (int i = 0; i < 13; i++)
    {
        m_paramTypes[i] = 0;
        m_params[i].m_n = 0;
    }
}

CSWSObjectActionNode::~CSWSObjectActionNode()
{
    for (unsigned int i = 0; i < m_nNumParameters; i++)
    {
        if (m_params[i].m_n == 0)
            continue;

        if (m_paramTypes[i] == ACTION_PARAM_STRING)
        {
            delete reinterpret_cast<CExoString*>(m_params[i].m_n);
            m_params[i].m_n = 0;
        }
        else if (m_paramTypes[i] == ACTION_PARAM_POINTER)
        {
            if (g_pVirtualMachine != 0)
            {
                g_pVirtualMachine->DeleteScriptSituation(reinterpret_cast<void*>(m_params[i].m_n));
                m_params[i].m_n = 0;
            }
        }
    }
}

CSWSObject::CSWSObject(unsigned char objectType, unsigned long long objectId, int flags)
    : CGameObject(objectType, objectId), m_position(), m_orientation(), m_castPosition()
{
    SetPosition(Vector(), 1, 0, 0);

    Vector vFacing;
    vFacing.x = 1.0f;
    vFacing.y = 0.0f;
    vFacing.z = 0.0f;
    SetOrientation(vFacing);

    m_nAnimation = SERVER_ANIMATION_PAUSE;
    m_nActionStartDay = 0;
    m_nActionStartTod = 0;
    m_fLastSpeedFactor = 1.0f;
    m_nReserved4 = 0;

    if (objectId == CGameObjectArray::INVALIDOBJID)
        g_pAppManager->m_pServerExoApp->GetObjectArray()->AddInternalObject(m_objectId, this, flags);
    else
        g_pAppManager->m_pServerExoApp->GetObjectArray()->AddObjectAtPos(m_objectId, this);

    m_currentHitPoints = 1;
    m_hitPoints = 1;
    m_hitPointBonus = 0;
    m_pDialog = 0;
    m_dialogOwnerId = CGameObjectArray::INVALIDOBJID;
    m_cachedAreaId = CGameObjectArray::INVALIDOBJID;
    m_bDialogPaused = 0;
    m_nDialogAnimation = 10001;
    m_dialogWaitFlags = 0;
    m_bCommandable = 1;
    m_bListening = 0;
    m_sTag = "";
    m_bDestroyable = 1;
    m_bRaiseable = 1;
    m_bDeadSelectable = 0;
    m_fAnimationSpeed = 1.0f;
    m_aiLevel = -1;
    m_nSavedAILevel = SAVED_AI_LEVEL_NONE;
    m_oidLastSpeaker = OBJECT_INVALID;
    m_dialogOwnerId = OBJECT_INVALID;
    m_killerId = OBJECT_INVALID;
    m_lastHostileActorId = OBJECT_INVALID;
    m_oidLastAttacker = OBJECT_INVALID;
    m_lastDamagerId = OBJECT_INVALID;
    m_oidLastDisturbedBy = OBJECT_INVALID;
    m_bCastSkipCharge = 0;
    m_nCastMetaMagic = CAST_METAMAGIC_NONE;
    m_nCastClassIndex = CAST_CLASS_NONE;
    m_castSpellId = SPELL_ID_NONE;
    m_nCastDomainLevel = 0;
    m_castTargetId = OBJECT_INVALID;
    m_castPosition.x = 0.0f;
    m_castPosition.y = 0.0f;
    m_castPosition.z = 0.0f;
    m_bCastBroadcastProjectile = 0;
    m_nLastSpellId = -1;
    m_bLastSpellHarmful = 0;
    m_oidLastSpellCaster = OBJECT_INVALID;
#if !KOTOR1  // K1 has no force-cost word, NotBlastable, hologram/render/level bytes or talk gate
    m_nLastSpellForcePointCost = 0;
#endif
    m_oidLastDisturbItem = OBJECT_INVALID;
    m_nLastDisturbType = 0;
    m_nImpactSpellId = -1;

    m_paDamageImmunity = new signed char[15];
    for (int i = 0; i < 15; i++)
        m_paDamageImmunity[i] = 0;

    m_paLastDamageAmounts = new int[15];
    for (int i = 0; i < 15; i++)
        m_paLastDamageAmounts[i] = -1;

    m_nLastAttackFeatId = 0;
    m_nLastAttackMode = CSWSCreature::CREATURE_COMBAT_MODE_NONE;
    m_oidLastWeaponUsed = OBJECT_INVALID;
    m_bPlot = 0;
    m_curGroupId = 0;
    m_prevGroupId = 0;
    m_pCurrentActionNode = 0;
    m_nEffectIterator = 0;
    m_bApplyingResistanceEffect = 0;
    m_bApplyingLinkEffect = 0;
    m_portraitId = PORTRAIT_ID_NONE;
    m_bDialogStarted = 0;
    m_bDialogFinished = 0;
    m_nDialogDelayStartDay = 0;
    m_nDialogDelayStartTod = 0;
    m_nReserved3 = 0;
    m_nReserved1 = 0;
    m_nReserved2 = 0;
    m_bPartyInteract = 0;
    m_nFadeOutTime = 0;
    m_bKeepCorpse = 0;
    m_bDeleteNoFade = 0;
    m_bMin1HP = 0;
#if !KOTOR1
    m_bNotBlastable = 0;
#endif
    m_updateFlags[0] = 0;
    m_bVisibleToPlayer = 0;
    m_bSkipDialogRequested = 0;
    m_bOpenDoorPending = 0;
    m_bOrientOnClick = 1;
#if !KOTOR1
    m_bHologram = 0;
    m_nMultiplierSet = 3;
    m_nPCLevelAtSpawn = 0;
    m_bWillNotRender = 0;
    m_bConfused = 0;
#endif
    m_pClientObject = 0;
    m_bDialogSoundStopped = 0;

    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&m_nAnimationStartDay, &m_nAnimationStartTod);
#if !KOTOR1
    m_bForceAlwaysUpdate = 0;
#endif
}

CSWSObject::~CSWSObject()
{
    if (m_pDialog) {
        delete m_pDialog;
        m_pDialog = 0;
    }

    for (int i = 0; i < m_listenExpressions.m_count; i++) {
        CSWSExpression* pExpression = m_listenExpressions.m_pArray[i];
        if (pExpression)
            delete pExpression;
    }
    ClearMatchedExpressionStrings();

    g_pAppManager->m_pServerExoApp->GetObjectArray()->Delete(m_objectId);

    for (int i = m_effects.m_count - 1; i >= 0; i--) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect)
            delete pEffect;
    }
    // inlined CExoArrayList<CGameEffect*>::Clear()
    if (m_effects.m_pArray) {
        delete[] m_effects.m_pArray;
        m_effects.m_pArray = 0;
        m_effects.m_capacity = 0;
    }
    m_effects.m_count = 0;

    if (m_paDamageImmunity) {
        delete m_paDamageImmunity;
        m_paDamageImmunity = 0;
    }

    if (m_paLastDamageAmounts) {
        delete m_paLastDamageAmounts;
        m_paLastDamageAmounts = 0;
    }

    while (m_actionList.m_pInternal->m_pHead != 0)
        delete static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->RemoveHead());

    CSWCObject* pClient = m_pClientObject;
    if (pClient) {
        pClient->DetachFromServerObject();
        m_pClientObject = 0;
    }
}

void CSWSObject::LoadObjectState(CResGFF* pGff, CResStruct* pStruct)
{
    LoadEffectList(pGff, pStruct);
    m_scriptVars.LoadVarTable(pGff, pStruct);
    m_varTable.LoadVarTable(pGff, pStruct);
    LoadActionQueue(pGff, pStruct);

    int bSuccess;
    m_bCommandable = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("Commandable"), bSuccess, 1);
}

void CSWSObject::LoadEffectList(CResGFF* pGff, CResStruct* pStruct)
{
    CResList effectList;
    if (!pGff->GetList(&effectList, pStruct, const_cast<char*>("EffectList")))
        return;

    for (unsigned int i = 0; i < pGff->GetListCount(&effectList); i++)
    {
        CResStruct elem;
        pGff->GetListElement(&elem, &effectList, i);

        CGameEffect* pEffect = new CGameEffect(0);
        pEffect->LoadGameEffect(pGff, &elem);
        if (pEffect->m_bSkipOnLoad == 1 ||
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            delete pEffect;
        }
        else
        {
            ApplyEffect(pEffect, 1, 0);
        }
    }
}

void CSWSObject::SaveEffectList(CResGFF* pGff, CResStruct* pStruct)
{
    if (m_effects.m_count < 1)
        return;

    CResList effectList;
    pGff->AddList(&effectList, pStruct, const_cast<char*>("EffectList"));

    for (int i = 0; i < m_effects.m_count; i++)
    {
        CResStruct elem;
        pGff->AddListElement(&elem, &effectList, 2);
        m_effects.m_pArray[i]->SaveGameEffect(pGff, &elem);
    }
}

void CSWSObject::SaveObjectState(CResGFF* pGff, CResStruct* pStruct)
{
    SaveEffectList(pGff, pStruct);
    m_scriptVars.SaveVarTable(pGff, pStruct);
    m_varTable.SaveVarTable(pGff, pStruct);
    SaveActionQueue(pGff, pStruct);
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bCommandable), const_cast<char*>("Commandable"));
}

void CSWSObject::SaveListenData(CResGFF* pGff, CResStruct* pStruct)
{
    if (m_listenExpressions.m_count < 1)
        return;

    CResList expressionList;
    pGff->AddList(&expressionList, pStruct, const_cast<char*>("ExpressionList"));

    for (int i = 0; i < m_listenExpressions.m_count; i++)
    {
        CResStruct elem;
        pGff->AddListElement(&elem, &expressionList, 5);
        pGff->WriteFieldINT(&elem, m_listenExpressions.m_pArray[i]->m_nId,
                            const_cast<char*>("ExpressionId"));
        pGff->WriteFieldCExoString(&elem, m_listenExpressions.m_pArray[i]->m_sExpression,
                                   const_cast<char*>("ExpressionString"));
    }
}

void CSWSObject::LoadListenData(CResGFF* pGff, CResStruct* pStruct)
{
    CExoString sExpression;
    CResList expressionList;
    if (!pGff->GetList(&expressionList, pStruct, const_cast<char*>("ExpressionList")))
        return;

    CResStruct elem;
    int bSuccess;
    int nExpressionId;
    for (unsigned int i = 0; i < pGff->GetListCount(&expressionList); i++)
    {
        pGff->GetListElement(&elem, &expressionList, i);
        nExpressionId = pGff->ReadFieldINT(&elem, const_cast<char*>("ExpressionId"), bSuccess, 0);
        sExpression = pGff->ReadFieldCExoString(&elem, const_cast<char*>("ExpressionString"), bSuccess,
                                                CExoString(""));
        SetListenExpression(sExpression, nExpressionId);
    }
}

int CSWSObject::SetListenExpression(const CExoString& sExpression, int nExpressionId)
{
    CSWSExpression* pExpression = NULL;
    if (m_listenExpressions.m_count >= 1)
    for (int64_t i = 0; i < m_listenExpressions.m_count; i++)
    {
        if (m_listenExpressions.m_pArray[i]->m_nId == nExpressionId)
        {
            pExpression = m_listenExpressions.m_pArray[i];
            break;
        }
    }

    if (!pExpression)
    {
        pExpression = new CSWSExpression;
        pExpression->m_nId = nExpressionId;
        m_listenExpressions.Add(pExpression);
    }

    return pExpression->ParseString(sExpression);
}

void CSWSObject::SaveActionQueue(CResGFF* pGff, CResStruct* pStruct)
{
    CResList actionList;
    CResList paramList;
    CResStruct actionStruct;
    CResStruct paramStruct;
    CResStruct situationStruct;
    CExoLinkedListNode* pNode;

    pGff->AddList(&actionList, pStruct, const_cast<char*>("ActionList"));

    pNode = m_actionList.m_pInternal->m_pHead;
    while (pNode != 0)
    {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));

        pGff->AddListElement(&actionStruct, &actionList, 0);
        pGff->WriteFieldDWORD(&actionStruct, pAction->m_nActionId, const_cast<char*>("ActionId"));
        pGff->WriteFieldWORD(&actionStruct, pAction->m_nGroupId, const_cast<char*>("GroupActionId"));
        pGff->WriteFieldWORD(&actionStruct, pAction->m_nNumParameters, const_cast<char*>("NumParams"));

        if (pAction->m_nNumParameters != 0)
        {
            pGff->AddList(&paramList, &actionStruct, const_cast<char*>("Paramaters"));

            for (unsigned int i = 0; i < pAction->m_nNumParameters; i++)
            {
                pGff->AddListElement(&paramStruct, &paramList, 1);
                pGff->WriteFieldDWORD(&paramStruct, pAction->m_paramTypes[i], const_cast<char*>("Type"));
                switch (pAction->m_paramTypes[i])
                {
                case ACTION_PARAM_INT:
                    pGff->WriteFieldINT(&paramStruct, static_cast<int>(pAction->m_params[i].m_n), const_cast<char*>("Value"));
                    break;
                case ACTION_PARAM_FLOAT:
                    pGff->WriteFieldFLOAT(&paramStruct, pAction->m_params[i].m_f, const_cast<char*>("Value"));
                    break;
                case ACTION_PARAM_OBJECT:
                    pGff->WriteFieldDWORD(&paramStruct, static_cast<unsigned int>(pAction->m_params[i].m_n), const_cast<char*>("Value"));
                    break;
                case ACTION_PARAM_STRING:
                    pGff->WriteFieldCExoString(&paramStruct, *reinterpret_cast<CExoString*>(pAction->m_params[i].m_n), const_cast<char*>("Value"));
                    break;
                case ACTION_PARAM_POINTER:
                    {
                        pGff->AddStructToStruct(&situationStruct, &paramStruct, const_cast<char*>("Value"), 2);
                        g_pVirtualMachine->SaveScriptSituation(reinterpret_cast<void*>(pAction->m_params[i].m_n), pGff,
                                                               &situationStruct);
                    }
                    break;
                }
            }
        }
        m_actionList.m_pInternal->GetNext(pNode);
    }
}

void CSWSObject::LoadActionQueue(CResGFF* pGff, CResStruct* pStruct)
{
    CResList actionList;
    CResList paramList;
    if (!pGff->GetList(&actionList, pStruct, const_cast<char*>("ActionList")))
        return;

    for (unsigned int i = 0; i < pGff->GetListCount(&actionList); i++)
    {
        CResStruct actionStruct;
        CResStruct paramStruct;
        CResStruct situationStruct;
        int bSuccess;
        void* pSituation;
        if (!pGff->GetListElement(&actionStruct, &actionList, i))
            continue;

        void*        apValues[13];
        unsigned int anTypes[13];
        int          anAlloc[13];
        for (int k = 0; k < 13; k++) {
            anTypes[k] = 0;
            apValues[k] = 0;
#ifdef K2_FIX_ENGINE_BUGS
            anAlloc[k] = 0;
#endif
        }

        unsigned int   nActionId      = pGff->ReadFieldDWORD(&actionStruct, const_cast<char*>("ActionId"), bSuccess, 0);
        unsigned short nGroupActionId = pGff->ReadFieldWORD(&actionStruct, const_cast<char*>("GroupActionId"), bSuccess, 0);
        unsigned short nNumParams     = pGff->ReadFieldWORD(&actionStruct, const_cast<char*>("NumParams"), bSuccess, 0);
#ifdef K2_FIX_ENGINE_BUGS
        if (nNumParams > 13)
            nNumParams = 13;
#endif

        // A zero-parameter action never touches the "Paramaters" list.
        if (nNumParams != 0)
        {
            pGff->GetList(&paramList, &actionStruct, const_cast<char*>("Paramaters"));

            for (unsigned int j = 0; j < nNumParams; j++)
            {
                pGff->GetListElement(&paramStruct, &paramList, j);

                anTypes[j] = pGff->ReadFieldDWORD(&paramStruct, const_cast<char*>("Type"), bSuccess, 0);
                switch (anTypes[j])
                {
                case ACTION_PARAM_INT:
                    {
                        int* pValue = new int;
                        *pValue = pGff->ReadFieldINT(&paramStruct, const_cast<char*>("Value"), bSuccess, 0);
                        apValues[j] = pValue;
                        anAlloc[j] = ACTION_PARAM_INT;
                    }
                    break;
                case ACTION_PARAM_FLOAT:
                    {
                        float* pValue = new float;
                        *pValue = pGff->ReadFieldFLOAT(&paramStruct, const_cast<char*>("Value"), bSuccess, 0.0f);
                        apValues[j] = pValue;
                        anAlloc[j] = ACTION_PARAM_FLOAT;
                    }
                    break;
                case ACTION_PARAM_OBJECT:
                    {
                        unsigned int* pValue = new unsigned int;
                        *pValue = pGff->ReadFieldDWORD(&paramStruct, const_cast<char*>("Value"), bSuccess, 0);
                        apValues[j] = pValue;
                        anAlloc[j] = ACTION_PARAM_OBJECT;
                    }
                    break;
                case ACTION_PARAM_STRING:
                    {
                        CExoString* pValue = new CExoString;
                        *pValue = pGff->ReadFieldCExoString(&paramStruct, const_cast<char*>("Value"), bSuccess, CExoString(""));
                        apValues[j] = pValue;
                        anAlloc[j] = ACTION_PARAM_STRING;
                    }
                    break;
                case ACTION_PARAM_POINTER:
                    {
                        pGff->GetStructFromStruct(&situationStruct, &paramStruct, const_cast<char*>("Value"));

                        g_pVirtualMachine->LoadScriptSituation(&pSituation, pGff, &situationStruct);
                        apValues[j] = pSituation;
                        anAlloc[j] = ACTION_PARAM_POINTER;
                    }
                    break;
                }
            }
        }

        AddAction(nActionId, nGroupActionId,
                  anTypes[0],  apValues[0],  anTypes[1],  apValues[1],
                  anTypes[2],  apValues[2],  anTypes[3],  apValues[3],
                  anTypes[4],  apValues[4],  anTypes[5],  apValues[5],
                  anTypes[6],  apValues[6],  anTypes[7],  apValues[7],
                  anTypes[8],  apValues[8],  anTypes[9],  apValues[9],
                  anTypes[10], apValues[10], anTypes[11], apValues[11],
                  anTypes[12], apValues[12]);

        for (unsigned int j = 0; j < nNumParams; j++)
        {
            switch (anAlloc[j])
            {
            case ACTION_PARAM_INT:    delete static_cast<int*>(apValues[j]);          break;
            case ACTION_PARAM_FLOAT:  delete static_cast<float*>(apValues[j]);        break;
            case ACTION_PARAM_OBJECT: delete static_cast<unsigned int*>(apValues[j]); break;
            case ACTION_PARAM_STRING: delete static_cast<CExoString*>(apValues[j]);   break;
            }
        }
    }
}

short CSWSObject::GetMaxHitPoints(int a) {
    K2_UNUSED(a);
    return static_cast<short>(m_hitPoints);
}

unsigned char CSWSObject::GetDamageLevel() {
    short nCurrentHitPoints;
    short nMaxHitPoints;
    if (AsSWSCreature() != 0) {
        nCurrentHitPoints = AsSWSCreature()->GetCurrentHitPoints(0);
        nMaxHitPoints = AsSWSCreature()->GetMaxHitPoints(1);
    } else {
        nCurrentHitPoints = GetCurrentHitPoints(0);
        nMaxHitPoints = GetMaxHitPoints(1);
    }

    float fFraction = static_cast<float>(nCurrentHitPoints) / static_cast<float>(nMaxHitPoints);
    if (fFraction >= 0.95f)
        return 0;
    if (fFraction >= 0.75f)
        return 1;
    if (fFraction >= 0.5f)
        return 2;
    if (fFraction >= 0.25f)
        return 3;
    if (fFraction <= 0.0f)
        return 5;
    return 4;
}

#if KOTOR1
int CSWSObject::GetIsPCDying()
{
    return 0;
}
#endif

int CSWSObject::GetDead() {
    if (AsSWSCreature() != 0 && AsSWSCreature()->m_bPlayerCharacter != 0)
        return AsSWSCreature()->GetCurrentHitPoints(0) < -9;
    if (AsSWSCreature() != 0)
        return AsSWSCreature()->GetCurrentHitPoints(0) < 1;
    return GetCurrentHitPoints(0) < 1;
}

unsigned short CSWSObject::GetPortraitId() {
    return m_portraitId;
}

void CSWSObject::SetDialogOwner(unsigned long long objectId) {
    m_dialogOwnerId = objectId;
}

void CSWSObject::AttachmentFromClientObject(CSWCObject* pClient) {
    m_pClientObject = pClient;
}

void CSWSObject::DetachFromClientObject() {
    m_pClientObject = 0;
}

void CSWSObject::SetAnimation(int nAnimation) {
    m_nAnimation = nAnimation;
    m_updateFlags[0] |= 1u;
}

void CSWSObject::SetOrientation(const Vector& dir) {
    m_orientation = dir;
    m_updateFlags[0] |= 4u;
}

void CSWSObject::SetDirtyFlag(unsigned int nFlag) {
    m_updateFlags[nFlag >> 5] |= 1u << nFlag;
}

int CSWSObject::IsDirtyFlagSet(unsigned int nFlag) {
    return (m_updateFlags[nFlag >> 5] & (1u << nFlag)) != 0;
}

void CSWSObject::ClearDirtyFlag(unsigned int nFlag) {
    m_updateFlags[nFlag >> 5] &= ~(1u << nFlag);
}

void CSWSObject::SetCurrentHitPoints(int nHitPoints) {
    if (m_bMin1HP && nHitPoints < 1)
        nHitPoints = 1;
    m_currentHitPoints = nHitPoints;
}

short CSWSObject::GetCurrentHitPoints(int a) {
    int cur = m_currentHitPoints;
    if (a == 1)
        return static_cast<short>(cur);
    return static_cast<short>(m_hitPointBonus + cur);
}

unsigned short CSWSObject::GetNewGroupID() {
    unsigned short id = m_curGroupId;
    unsigned short next = id + 1;
    m_curGroupId = (next == ACTION_GROUP_NEW) ? 0 : next;
    m_prevGroupId = id;
    return id;
}

CScriptLocation CSWSObject::GetScriptLocation() {
    CScriptLocation loc;
    loc.m_vPosition = m_position;
    loc.m_vOrientation = m_orientation;
    return loc;
}

CScriptLocation::CScriptLocation() {
    m_vPosition.x = 0.0f;
    m_vPosition.y = 0.0f;
    m_vPosition.z = 0.0f;
    m_vOrientation.x = 0.0f;
    m_vOrientation.y = 0.0f;
    m_vOrientation.z = 0.0f;
}

CScriptLocation::~CScriptLocation() {
}

void CScriptLocation::CopyScriptLocation(CScriptLocation* pSource) {
    if (pSource != 0) {
        m_vOrientation = pSource->m_vOrientation;
        m_vPosition = pSource->m_vPosition;
    }
}

int CScriptLocation::LoadLocation(CResGFF* pGFF, CResStruct* pStruct) {
    int bSuccess;
    Vector v;

    v.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("OrientationX"), bSuccess, 0.0f);
    v.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("OrientationY"), bSuccess, 0.0f);
    v.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("OrientationZ"), bSuccess, 0.0f);
    m_vOrientation = v;

    v.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionX"), bSuccess, 0.0f);
    v.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionY"), bSuccess, 0.0f);
    v.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionZ"), bSuccess, 0.0f);
    m_vPosition = v;

    return 1;
}

int CScriptLocation::SaveLocation(CResGFF* pGFF, CResStruct* pStruct) {
    pGFF->WriteFieldFLOAT(pStruct, m_vOrientation.x, const_cast<char*>("OrientationX"));
    pGFF->WriteFieldFLOAT(pStruct, m_vOrientation.y, const_cast<char*>("OrientationY"));
    pGFF->WriteFieldFLOAT(pStruct, m_vOrientation.z, const_cast<char*>("OrientationZ"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPosition.x, const_cast<char*>("PositionX"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPosition.y, const_cast<char*>("PositionY"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPosition.z, const_cast<char*>("PositionZ"));
    return 1;
}

// Includes 4037 and 6000, which visualeffects.2da does not mark as beams.
int CLoopingVisualEffect::GetIsBeam(unsigned short nVisualEffectId) {
    switch (nVisualEffectId) {
    case VFX_BEAM_DEATH_FIELD_TENTACLE:
    case VFX_BEAM_DROID_DISABLE:
    case VFX_BEAM_DROID_DESTROY:
    case VFX_BEAM_DRAIN_LIFE:
    case VFX_BEAM_LIGHTNING_DARK_S:
    case VFX_BEAM_LIGHTNING_DARK_L:
    case VFX_BEAM_ION_RAY_01:
    case VFX_BEAM_ION_RAY_02:
    case VFX_BEAM_COLD_RAY:
    case VFX_BEAM_STUN_RAY:
    case VFX_BEAM_FLAME_SPRAY:
    case VFX_BEAM_FORCE_STORM_ARC:
    case VFX_BEAM_DROID_STUN:
#if KOTOR1  // K1: the beam family ends at 2066
    case VFX_COM_FORCE_RESISTED:
    case VFX_IMP_MIRV:
        return 1;
    }
    return nVisualEffectId == VFX_BEAM_SHOCK;
#else
    case VFX_BEAM_SHOCK:
    case VFX_BEAM_POISON_SPRAY:
    case VFX_COM_FORCE_RESISTED:
    case VFX_IMP_MIRV:
        return 1;
    }
    return nVisualEffectId == VFX_BEAM_PLASMA_SPRAY;
#endif
}

int CLoopingVisualEffect::GetIsBeam() {
    switch (m_nVisualEffectId) {
    case VFX_BEAM_DEATH_FIELD_TENTACLE:
    case VFX_BEAM_DROID_DISABLE:
    case VFX_BEAM_DROID_DESTROY:
    case VFX_BEAM_DRAIN_LIFE:
    case VFX_BEAM_LIGHTNING_DARK_S:
    case VFX_BEAM_LIGHTNING_DARK_L:
    case VFX_BEAM_ION_RAY_01:
    case VFX_BEAM_ION_RAY_02:
    case VFX_BEAM_COLD_RAY:
    case VFX_BEAM_STUN_RAY:
    case VFX_BEAM_FLAME_SPRAY:
    case VFX_BEAM_FORCE_STORM_ARC:
    case VFX_BEAM_DROID_STUN:
#if KOTOR1  // K1: the beam family ends at 2066
    case VFX_COM_FORCE_RESISTED:
    case VFX_IMP_MIRV:
        return 1;
    }
    return m_nVisualEffectId == VFX_BEAM_SHOCK;
#else
    case VFX_BEAM_SHOCK:
    case VFX_BEAM_POISON_SPRAY:
    case VFX_COM_FORCE_RESISTED:
    case VFX_IMP_MIRV:
        return 1;
    }
    return m_nVisualEffectId == VFX_BEAM_PLASMA_SPRAY;
#endif
}

CScriptTalent::CScriptTalent() {
    m_nType = -1;
    m_nId = -1;
    m_nMultiClass = 0;
    m_oidItem = OBJECT_INVALID;  // OBJECT_INVALID
    m_nItemPropertyIndex = -1;
    m_nCasterLevel = 0xff;
    m_nMetaType = 0xff;
}

CScriptTalent::~CScriptTalent() {
}

void CScriptTalent::CopyScriptTalent(CScriptTalent* pSource) {
    if (pSource != 0) {
        m_nType = pSource->m_nType;
        m_nId = pSource->m_nId;
        m_nMultiClass = pSource->m_nMultiClass;
        m_oidItem = pSource->m_oidItem;
        m_nItemPropertyIndex = pSource->m_nItemPropertyIndex;
        m_nCasterLevel = pSource->m_nCasterLevel;
        m_nMetaType = pSource->m_nMetaType;
    }
}

int CScriptTalent::SaveTalent(CResGFF* pGFF, CResStruct* pStruct) {
    pGFF->WriteFieldINT(pStruct, m_nId, const_cast<char*>("ID"));
    pGFF->WriteFieldINT(pStruct, m_nType, const_cast<char*>("Type"));
    pGFF->WriteFieldBYTE(pStruct, m_nMultiClass, const_cast<char*>("MultiClass"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidItem), const_cast<char*>("Item"));
    pGFF->WriteFieldINT(pStruct, m_nItemPropertyIndex, const_cast<char*>("ItemPropertyIndex"));
    pGFF->WriteFieldBYTE(pStruct, m_nCasterLevel, const_cast<char*>("CasterLevel"));
    pGFF->WriteFieldBYTE(pStruct, m_nMetaType, const_cast<char*>("MetaType"));
    return 1;
}

int CScriptTalent::LoadTalent(CResGFF* pGFF, CResStruct* pStruct) {
    int bSuccess;

    m_nId = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ID"), bSuccess, 0);
    m_nType = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Type"), bSuccess, 0);
    m_nMultiClass = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MultiClass"), bSuccess, 0);
    m_oidItem = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Item"), bSuccess, 0);
    m_nItemPropertyIndex = pGFF->ReadFieldINT(pStruct, const_cast<char*>("ItemPropertyIndex"), bSuccess, 0);
    m_nCasterLevel = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CasterLevel"), bSuccess, 0);
    m_nMetaType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MetaType"), bSuccess, 0);

    return 1;
}

int CScriptTalent::Equals(CScriptTalent* pOther) {
    return m_nType == pOther->m_nType &&
           m_nId == pOther->m_nId &&
           m_nMultiClass == pOther->m_nMultiClass &&
           m_oidItem == pOther->m_oidItem &&
           m_nItemPropertyIndex == pOther->m_nItemPropertyIndex &&
           m_nCasterLevel == pOther->m_nCasterLevel &&
           m_nMetaType == pOther->m_nMetaType;
}

CSWSObject* CSWSObject::AsSWSObject() {
    return this;
}

int CSWSObject::GetDialogInterruptable() {
    return 1;
}

CExoLocString& CSWSObject::GetFirstName() {
    return m_sName;
}

CExoLocString& CSWSObject::GetLastName() {
    return m_sName;
}

unsigned int CSWSObject::GetEffectSpellId() {
    return static_cast<unsigned int>(-1);
}

void CSWSObject::SetEffectSpellId(unsigned int nSpellId) {
    K2_UNUSED(nSpellId);
}

void CSWSObject::SetLockOrientationToObject(unsigned long long objectId) {
    K2_UNUSED(objectId);
}

unsigned long long CSWSObject::GetLockOrientationToObject() {
    return OBJECT_INVALID;
}

CResRef CSWSObject::GetPortrait() {
    return m_portrait;
}

void CSWSObject::SetPortrait(const CResRef& rPortrait) {
    m_portrait = rPortrait;
}

void CSWSObject::RemoveEffectTarget(unsigned long long objectId)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (!pObject)
        return;
    if (!pObject->AsSWSObject())
        return;
    if (m_spellTargets.m_count < 1)
        return;

    CSWSObject* pTarget = pObject->AsSWSObject();
    unsigned n = pTarget->m_effects.m_count;
    if (n != 0)
    {
        CGameEffect* pEffect;
        int i = 0;
        do {
            pEffect = pTarget->m_effects.m_pArray[i];
            i++;
        } while (pEffect->m_creatorId != m_objectId && static_cast<unsigned>(i) < n);
        if (pEffect->m_creatorId == m_objectId)
            return;
    }

    if (m_spellTargets.Contains(objectId))
        m_spellTargets.Remove(objectId);
}

void CSWSObject::ApplyEffect(CGameEffect* pEffect, int bLoading, int b)
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);

    pEffect->m_bSkipOnLoad = (m_bApplyingResistanceEffect || pEffect->m_bIsExposed == 0);

    if (g_pAppManager->m_pServerExoApp->GetServerAIMaster()->OnEffectApplied(this, pEffect,
                                                                            bLoading) ==
        CSWSEffectListHandler::DELETE_EFFECT)
    {
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LINK && b == 1 && m_bApplyingLinkEffect == 1)
            RemoveEffectById(pEffect->m_effectId);
        else
            delete pEffect;
        return;
    }

    if ((pEffect->m_subType != CSWSEffectListHandler::EFFECT_VISUALEFFECT &&
         pEffect->m_subType != CSWSEffectListHandler::EFFECT_EFFECT_ICON) ||
        (pEffect->m_subType == CSWSEffectListHandler::EFFECT_VISUALEFFECT &&
         (pEffect->GetInteger(0) == VFX_ARD_LIGHT_YELLOW_10 || pEffect->GetInteger(0) == VFX_ARD_LIGHT_YELLOW_20)))
    {
        m_bApplyingLinkEffect = 0;
    }

    if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_TEMPORARY && bLoading == 0)
    {
        unsigned int nDurationDay = g_pAppManager->m_pServerExoApp->GetWorldTimer()
                                        ->GetCalendarDayFromSeconds(pEffect->m_fDuration);
        unsigned int nDurationTod = g_pAppManager->m_pServerExoApp->GetWorldTimer()
                                        ->GetTimeOfDayFromSeconds(pEffect->m_fDuration);
        unsigned int nExpiryDay;
        unsigned int nExpiryTod;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->AddWorldTimes(
            nDay, nTimeOfDay, nDurationDay, nDurationTod, &nExpiryDay, &nExpiryTod);
        pEffect->SetExpiryTime(nExpiryDay, nExpiryTod);
    }

    int nIndex;
    for (nIndex = 0; nIndex < m_effects.m_count; nIndex++)
    {
        if (pEffect->m_subType < m_effects.m_pArray[nIndex]->m_subType)
            break;
    }
    m_effects.Insert(pEffect, nIndex);

    if (m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
    {
        UpdateEffectPtrs();
        UpdateAttributesOnEffect(pEffect);
    }

    if ((pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) == SUBTYPE_MAGICAL)
    {
        CGameObject* pCreator =
            g_pAppManager->m_pServerExoApp->GetGameObject(pEffect->m_creatorId);
        if (pCreator && pCreator->AsSWSObject())
            pCreator->AsSWSObject()->m_spellTargets.AddUnique(m_objectId);
    }
}

void CSWSObject::ClearSpellEffectsOnOthers()
{
    for (int i = m_spellTargets.m_count - 1; i >= 0; i--)
    {
        CGameObject* pObject =
            g_pAppManager->m_pServerExoApp->GetGameObject(m_spellTargets.m_pArray[i]);
        if (pObject && pObject->AsSWSObject())
        {
            if (pObject->AsSWSAreaOfEffectObject() &&
                pObject->AsSWSAreaOfEffectObject()->m_oidCreator == m_objectId)
            {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, m_spellTargets.m_pArray[i],
                    CServerAIMaster::EVENT_DESTROY_OBJECT, NULL);
            }
            else
            {
                CSWSObject* pTarget = pObject->AsSWSObject();
                for (int j = 0; j < pTarget->m_effects.m_count; j++)
                {
                    if (pTarget->m_effects.m_pArray[j]->m_creatorId == m_objectId &&
                        (pTarget->m_effects.m_pArray[j]->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) == SUBTYPE_MAGICAL)
                    {
                        pObject->AsSWSObject()->RemoveEffectById(
                            pTarget->m_effects.m_pArray[j]->m_effectId);
                        j = 0;
                    }
                }
            }
        }
    }
    m_spellTargets.m_count = 0;
}

void CSWSObject::SetPortraitId(unsigned short nPortraitId) {
    CExoString sPrefix("po_");
    if (nPortraitId >= PORTRAIT_ID_CUSTOM) {
        SetPortrait(CResRef(""));
    } else {
        CExoString sBaseResRef;
        g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(nPortraitId,
                                                              CExoString("BaseResRef"),
                                                              &sBaseResRef);
        sBaseResRef = sPrefix + sBaseResRef;
        SetPortrait(CResRef(sBaseResRef.CStr()));
    }
    m_portraitId = nPortraitId;
}

void CSWSObject::UpdateEffectPtrs() {
}

void CSWSObject::UpdateAttributesOnEffect(CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
}

unsigned char CSWSObject::GetGender() {
    return CTlkTable::GENDER_MALE;
}

int CSWSObject::GetDamageImmunity(unsigned char nDamageType) {
    int nImmunity = m_paDamageImmunity[nDamageType];
#if KOTOR1  // K1 clamps to [-100, 100]; K2 keeps the low clamp as an unsigned byte
    nImmunity = nImmunity < 100 ? nImmunity : 100;
    return nImmunity > -100 ? nImmunity : -100;
#else
    if (nImmunity < -100)
        return static_cast<unsigned char>(-100);
    return nImmunity < 100 ? nImmunity : 100;
#endif
}

void CSWSObject::RemoveEffect(CGameEffect* pEffect) {
    if (g_pAppManager->m_pServerExoApp->GetServerAIMaster()->OnEffectRemoved(this, pEffect)
        != CSWSEffectListHandler::DELETE_EFFECT)
        return;
    int nIndex = -1;
    if (m_effects.m_count >= 1) {
        for (int64_t i = 0; i < m_effects.m_count; i++) {
            if (m_effects.m_pArray[i] == pEffect) {
                nIndex = static_cast<int>(i);
                break;
            }
        }
    }
    // inlined CExoArrayList<CGameEffect*>::DelIndex(nIndex)
    m_effects.m_count--;
    for (int j = nIndex; m_effects.m_count > j; j++)
        m_effects.m_pArray[j] = m_effects.m_pArray[j + 1];
    CGameObject* pCreator = g_pAppManager->m_pServerExoApp->GetGameObject(pEffect->m_creatorId);
    if (pCreator != 0 && pCreator->AsSWSObject() != 0)
        pCreator->AsSWSObject()->RemoveEffectTarget(m_objectId);
    if (m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        UpdateAttributesOnEffect(pEffect);
    delete pEffect;
    if (m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        UpdateEffectPtrs();
}

int CSWSObject::DoSpellLevelAbsorption(CSWSObject* pAttacker) {
    if (pAttacker == 0)
        return -1;
    if (pAttacker->AsSWSCreature() == 0)
        return -1;
    if (m_effects.m_count < 1)
        return -1;
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(pAttacker->m_castSpellId);
    if (pSpell == 0)
        return -1;
    // The caster's spell level for the class the cast was made under; GetClass reports failure
    // when the index is out of range.
    if (pAttacker->AsSWSCreature()->m_bItemCastSpell != 1) {
        CSWSCreatureStats* pStats = pAttacker->AsSWSCreature()->m_pStats;
        unsigned char nClass = pStats->GetClass(pAttacker->m_nCastClassIndex);
        if (nClass != CLASS_TYPE_INVALID)
            pSpell->GetSpellLevel(nClass);
    }
    if (pSpell->m_nUserType != 1)
        return -1;
    for (int i = AsSWSCreature()->m_pStats->m_iFirstSpellLevelAbsorption; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_subType > CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION)
            break;
    }
    for (int j = AsSWSCreature()->m_pStats->m_iFirstSpellLevelAbsorption; j < m_effects.m_count; j++) {
        if (m_effects.m_pArray[j]->m_subType > CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION)
            break;
    }
    if (AsSWSCreature() != 0 && pAttacker->AsSWSCreature() != 0) {
    }
    return -1;
}

int CSWSObject::DoSpellImmunity(CSWSObject* pAttacker) {
    unsigned int nSpellId = pAttacker->m_castSpellId;
    if (AsSWSCreature() == 0)
        return -1;
    if (m_effects.m_count < 1)
        return -1;
    for (int i = AsSWSCreature()->m_pStats->m_iFirstSpellImmunity; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY)
            return -1;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY
            && (pEffect->GetInteger(0) == static_cast<int>(nSpellId) || pEffect->GetInteger(0) == -1)) {
            CSWCCMessageData* pMine = new CSWCCMessageData;
            CSWCCMessageData* pTheirs = new CSWCCMessageData;
            pMine->SetInteger(1, nSpellId);
            pMine->SetObjectID(0, m_objectId);
            pMine->CopyTo(pTheirs);
            if (AsSWSCreature() != 0)
                AsSWSCreature()->SendFeedbackMessage(FEEDBACK_SPELL_IMMUNITY, pMine);
            else
                delete pMine;
            if (pAttacker->AsSWSCreature() != 0)
                pAttacker->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_SPELL_IMMUNITY, pTheirs);
            else
                delete pTheirs;
            return 2;
        }
    }
    return -1;
}

int CSWSObject::DoDamageResistance(CSWSCreature* pAttacker, int nDamage, unsigned short nFlags,
                                   int bNoFeedback, int bQueueOnAttack, int bRecordFeedback)
{
    int nResisted = 0;
    int nResistIndex K2_UNSET;
    int bTypeMatch = 0;
    int nDamageTypeBits = 0;
    if (m_effects.m_count > 0) {
        for (int nBit = 0; nBit < 15; nBit++) {
            if ((nFlags >> nBit) & 1) {
                int i = 0;
                if (AsSWSCreature() != 0)
                    i = AsSWSCreature()->m_pStats->m_iFirstDamageResistance;
                int nBestAmount = 0;
                int nBestIndex = 0;
                for (; i < m_effects.m_count; i++) {
                    CGameEffect* pEffect = m_effects.m_pArray[i];
                    if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE)
                        break;
                    if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE) {
                        int nSchools = pEffect->GetInteger(0);
                        int nAmount = pEffect->GetInteger(1);
                        if ((nSchools & nFlags) != 0 && nAmount > nBestAmount) {
                            nBestIndex = i;
                            nBestAmount = nAmount;
                        }
                        if ((pEffect->GetInteger(3) & nFlags) != 0)
                            bTypeMatch = 1;
                    }
                }
                if (nBestAmount > nResisted) {
                    nResisted = nBestAmount;
                    nResistIndex = nBestIndex;
                }
                if (bRecordFeedback != 0)
                    nDamageTypeBits = nFlags >> nBit;
            }
        }
        if (nResisted != 0) {
            int nUses = m_effects.m_pArray[nResistIndex]->GetInteger(2);
            int nMultiplier = bTypeMatch != 0 ? 2 : 1;
#if !KOTOR1  // K1 has no feat-attack surcharge on the resistance's uses
            if (pAttacker != 0) {
                if (m_effects.m_pArray[nResistIndex]->GetInteger(4) == 1) {
                    int nAttack = pAttacker->m_pCombatRound->m_currentAttack;
                    if (pAttacker->m_pCombatRound->GetAttack(nAttack)->m_nFeatId == FEAT_POWER_ATTACK ||
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_nFeatId == FEAT_IMPROVED_POWER_ATTACK ||
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_nFeatId == FEAT_MASTER_POWER_ATTACK)
                        nMultiplier++;
                }
            }
#endif
            int nCharged = nMultiplier * nDamage;
            if (nUses != 0) {
                if (nUses - nCharged > 0) {
                    if (bNoFeedback == 0)
                        m_effects.m_pArray[nResistIndex]->SetInteger(2, nUses - nCharged);
                } else {
                    if (bNoFeedback == 0)
                        RemoveEffectById(m_effects.m_pArray[nResistIndex]->m_effectId);
                    nResisted = nUses;
                }
            }
            if (bNoFeedback == 0) {
                if (pAttacker != 0 && bQueueOnAttack == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, m_objectId);
                    int nAttack = pAttacker->m_pCombatRound->m_currentAttack;
                    if (nUses != 0) {
                        pData->SetInteger(0, FEEDBACK_DAMAGE_RESISTED_REMAINING);
                        if (nUses - nCharged > 0) {
                            int nSoaked = nCharged < nResisted ? nCharged : nResisted;
                            pData->SetInteger(1, nSoaked);
                            pData->SetInteger(2, nUses - nSoaked);
                        } else {
                            pData->SetInteger(1, nUses);
                            pData->SetInteger(2, 0);
                        }
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_messages.Add(pData);
                    } else {
                        pData->SetInteger(0, FEEDBACK_DAMAGE_RESISTED);
                        pData->SetInteger(1, nCharged < nResisted ? nCharged : nResisted);
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_messages.Add(pData);
                    }
                } else {
                    CSWCCMessageData* pMine = new CSWCCMessageData;
                    CSWCCMessageData* pTheirs = new CSWCCMessageData;
                    pMine->SetObjectID(0, m_objectId);
                    if (nUses != 0) {
                        if (nUses - nCharged > 0) {
                            int nSoaked = nCharged < nResisted ? nCharged : nResisted;
                            pMine->SetInteger(1, nSoaked);
                            pMine->SetInteger(2, nUses - nSoaked);
                        } else {
                            pMine->SetInteger(1, nUses);
                            pMine->SetInteger(2, 0);
                        }
                        pMine->CopyTo(pTheirs);
                        if (AsSWSCreature() != 0)
                            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_DAMAGE_RESISTED_REMAINING, pMine);
                        else
                            delete pMine;
                        if (pAttacker != 0)
                            pAttacker->SendFeedbackMessage(FEEDBACK_DAMAGE_RESISTED_REMAINING, pTheirs);
                        else
                            delete pTheirs;
                    } else {
                        pMine->SetInteger(1, nCharged < nResisted ? nCharged : nResisted);
                        pMine->CopyTo(pTheirs);
                        if (AsSWSCreature() != 0)
                            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_DAMAGE_RESISTED, pMine);
                        else
                            delete pMine;
                        if (pAttacker != 0)
                            pAttacker->SendFeedbackMessage(FEEDBACK_DAMAGE_RESISTED, pTheirs);
                        else
                            delete pTheirs;
                    }
                }
            }
        }
    }
#if KOTOR1
    // K1: the target's own soak is two flat -2 feats, with no percentage or scaling tiers.
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        if (pCreature->m_pStats->HasFeat(FEAT_IMPROVED_TOUGHNESS) != 0) {
            nDamage -= 2;
            if (bRecordFeedback != 0) {
                CSWSCombatAttackData* pAttack = pAttacker->m_pCombatRound->GetAttack(
                    pAttacker->m_pCombatRound->m_currentAttack);
                pAttack->m_damage.m_nDamageAfterSoak = static_cast<unsigned char>(nDamage);
                pAttack->m_damage.m_nFeatSoakAmount = 2;
                pAttack->m_damage.m_nDamageTypeBits = static_cast<unsigned char>(nDamageTypeBits);
            }
        }
        if (pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS) != 0) {
            nDamage -= 2;
            if (bRecordFeedback != 0) {
                CSWSCombatAttackData* pAttack = pAttacker->m_pCombatRound->GetAttack(
                    pAttacker->m_pCombatRound->m_currentAttack);
                pAttack->m_damage.m_nDamageAfterFlatDR = static_cast<unsigned char>(nDamage);
                pAttack->m_damage.m_nFeatFlatDRAmount = 2;
                pAttack->m_damage.m_nDamageTypeBits = static_cast<unsigned char>(nDamageTypeBits);
            }
        }
    }
#else
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        if (pCreature->m_pStats->HasFeat(FEAT_IGNORE_PAIN_3) != 0 || pCreature->m_pStats->HasFeat(FEAT_INNER_STRENGTH_3) != 0)
            nDamage -= static_cast<int>(nDamage * 0.15f);
        else if (pCreature->m_pStats->HasFeat(FEAT_IGNORE_PAIN_2) != 0 || pCreature->m_pStats->HasFeat(FEAT_INNER_STRENGTH_2) != 0)
            nDamage -= static_cast<int>(nDamage * 0.1f);
        else if (pCreature->m_pStats->HasFeat(FEAT_IGNORE_PAIN_1) != 0 || pCreature->m_pStats->HasFeat(FEAT_INNER_STRENGTH_1) != 0)
            nDamage -= static_cast<int>(nDamage * 0.05f);
        if (pCreature->m_pStats->HasFeat(FEAT_IMPROVED_TOUGHNESS) != 0) {
            int nSoak = static_cast<int>(nDamage * 0.1f);
            nSoak = nSoak > 2 ? nSoak : 2;
            nDamage -= nSoak;
            if (bRecordFeedback != 0) {
                CSWSCombatAttackData* pAttack = pAttacker->m_pCombatRound->GetAttack(
                    pAttacker->m_pCombatRound->m_currentAttack);
                pAttack->m_damage.m_nDamageAfterSoak = static_cast<unsigned char>(nDamage);
                pAttack->m_damage.m_nFeatSoakAmount = static_cast<unsigned char>(nSoak);
                pAttack->m_damage.m_nDamageTypeBits = static_cast<unsigned char>(nDamageTypeBits);
            }
        }
        int nFlatDR = 0;
        if (pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS_III) != 0) {
            nFlatDR = 8;
            if (nDamage - 8 > 0)
                nFlatDR = 8 + (nDamage - 8) / 10;
            nDamage -= nFlatDR;
        } else if (pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS_II) != 0) {
            nFlatDR = 5;
            if (nDamage - 5 > 0)
                nFlatDR = 5 + (nDamage - 5) / 20;
            nDamage -= nFlatDR;
        } else if (pCreature->m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS) != 0) {
            nFlatDR = 2;
            nDamage -= nFlatDR;
        }
        if (bRecordFeedback != 0 && nFlatDR > 0) {
            CSWSCombatAttackData* pAttack = pAttacker->m_pCombatRound->GetAttack(
                pAttacker->m_pCombatRound->m_currentAttack);
            pAttack->m_damage.m_nDamageAfterFlatDR = static_cast<unsigned char>(nDamage);
            pAttack->m_damage.m_nFeatFlatDRAmount = static_cast<unsigned char>(nFlatDR);
            pAttack->m_damage.m_nDamageTypeBits = static_cast<unsigned char>(nDamageTypeBits);
        }
    }
#endif
    nDamage -= nResisted;
    if (nDamage < 0)
        nDamage = 0;
    return nDamage;
}

int CSWSObject::DoDamageReduction(CSWSCreature* pAttacker, int nDamage, unsigned char nPower,
                                  int bNoFeedback, int bQueueOnAttack)
{
    int i = 0;
    if (AsSWSCreature() != 0)
        i = AsSWSCreature()->m_pStats->m_iFirstDamageReduction;
    if (m_effects.m_count > 0) {
        int nReduction = 0;
        int nMinPower = 0;
        int nBest = 0;
        int bFound = 0;
        for (; i < m_effects.m_count; i++) {
            CGameEffect* pEffect = m_effects.m_pArray[i];
            if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION)
                break;
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION) {
                int nAmount = pEffect->GetInteger(0);
                int nPowerRequired = pEffect->GetInteger(1);
                if (nAmount > nReduction) {
                    bFound = 1;
                    nBest = i;
                    nMinPower = nPowerRequired;
                    nReduction = nAmount;
                }
            }
        }
        if (bFound != 0) {
            int nUses = m_effects.m_pArray[nBest]->GetInteger(2);
            if (nUses != 0) {
                if (nUses - nDamage > 0) {
                    if (bNoFeedback == 0)
                        m_effects.m_pArray[nBest]->SetInteger(2, nUses - nDamage);
                } else {
                    if (bNoFeedback == 0)
                        RemoveEffectById(m_effects.m_pArray[nBest]->m_effectId);
                    nReduction = nUses;
                }
            }
            if (bNoFeedback == 0 && nMinPower > nPower) {
                if (pAttacker != 0 && bQueueOnAttack == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, m_objectId);
                    int nAttack = pAttacker->m_pCombatRound->m_currentAttack;
                    if (nUses != 0) {
                        pData->SetInteger(0, FEEDBACK_DAMAGE_REDUCED_REMAINING);
                        if (nUses - nDamage > 0) {
                            int nSoaked = nReduction > nDamage ? nDamage : nReduction;
                            pData->SetInteger(1, nSoaked);
                            pData->SetInteger(2, nUses - nSoaked);
                        } else {
                            pData->SetInteger(1, nUses);
                            pData->SetInteger(2, 0);
                        }
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_messages.Add(pData);
                    } else {
                        pData->SetInteger(0, FEEDBACK_DAMAGE_REDUCED);
                        pData->SetInteger(1, nReduction > nDamage ? nDamage : nReduction);
                        pAttacker->m_pCombatRound->GetAttack(nAttack)->m_messages.Add(pData);
                    }
                } else {
                    CSWCCMessageData* pMine = new CSWCCMessageData;
                    CSWCCMessageData* pTheirs = new CSWCCMessageData;
                    pMine->SetObjectID(0, m_objectId);
                    if (nUses != 0) {
                        if (nUses - nDamage > 0) {
                            int nSoaked = nReduction > nDamage ? nDamage : nReduction;
                            pMine->SetInteger(1, nSoaked);
                            pMine->SetInteger(2, nUses - nSoaked);
                        } else {
                            pMine->SetInteger(1, nUses);
                            pMine->SetInteger(2, 0);
                        }
                        pMine->CopyTo(pTheirs);
                        if (AsSWSCreature() != 0)
                            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_DAMAGE_REDUCED_REMAINING, pMine);
                        else
                            delete pMine;
                        if (pAttacker != 0)
                            pAttacker->SendFeedbackMessage(FEEDBACK_DAMAGE_REDUCED_REMAINING, pTheirs);
                        else
                            delete pTheirs;
                    } else {
                        pMine->SetInteger(1, nReduction > nDamage ? nDamage : nReduction);
                        pMine->CopyTo(pTheirs);
                        if (AsSWSCreature() != 0)
                            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_DAMAGE_REDUCED, pMine);
                        else
                            delete pMine;
                        if (pAttacker != 0)
                            pAttacker->SendFeedbackMessage(FEEDBACK_DAMAGE_REDUCED, pTheirs);
                        else
                            delete pTheirs;
                    }
                }
            }
            if (nMinPower > nPower) {
                nDamage -= nReduction;
                if (nDamage < 0)
                    nDamage = 0;
            }
            return nDamage;
        }
    }
    if (AsSWSCreature() == 0 || nDamage > 0)
        return nDamage;
    return 0;
}

int CSWSObject::DoDamageImmunity(CSWSCreature* pAttacker, int nDamage, unsigned short nFlags,
                                 int bNoFeedback, int bQueueOnAttack)
{
    int nImmunity = GetDamageImmunityByFlags(nFlags);
    int nAbsorbed;
    if (nImmunity * nDamage < 100 && nImmunity > 0)
        nAbsorbed = 1;
    else
        nAbsorbed = nImmunity * nDamage / 100;
    if (bNoFeedback == 0 && nImmunity > 0) {
        if (pAttacker != 0 && bQueueOnAttack == 1) {
            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, FEEDBACK_DAMAGE_IMMUNITY);
            pData->SetInteger(1, nAbsorbed);
            pData->SetInteger(2, nFlags);
            pData->SetObjectID(0, m_objectId);
            pAttacker->m_pCombatRound->GetAttack(
                pAttacker->m_pCombatRound->m_currentAttack)->m_messages.Add(pData);
        } else {
            CSWCCMessageData* pMine = new CSWCCMessageData;
            CSWCCMessageData* pTheirs = new CSWCCMessageData;
            pMine->SetInteger(0, FEEDBACK_DAMAGE_IMMUNITY);
            pMine->SetInteger(1, nAbsorbed);
            pMine->SetInteger(2, nFlags);
            pMine->SetObjectID(0, m_objectId);
            pMine->CopyTo(pTheirs);
            if (AsSWSCreature() != 0)
                AsSWSCreature()->SendFeedbackMessage(FEEDBACK_DAMAGE_IMMUNITY, pMine);
            else
                delete pMine;
            if (pAttacker != 0)
                pAttacker->SendFeedbackMessage(FEEDBACK_DAMAGE_IMMUNITY, pTheirs);
            else
                delete pTheirs;
        }
    }
    return nDamage - nAbsorbed;
}

void CSWSObject::DoDamage(int nDamage) {
    int nRemaining = GetCurrentHitPoints(1) - nDamage;
    int bMin1HP = m_bMin1HP;
    if (nRemaining < 1 && bMin1HP)
        nRemaining = 1;
    m_currentHitPoints = nRemaining;
}

int CSWSObject::AnimationPause(unsigned short nAnimation) {
    return nAnimation == SERVER_ANIMATION_PAUSE || nAnimation == SERVER_ANIMATION_PAUSE_INJURED;
}

int CSWSObject::AnimationWalking(unsigned short nAnimation) {
    return nAnimation == SERVER_ANIMATION_WALK || nAnimation == SERVER_ANIMATION_WALK_INJURED || nAnimation == SERVER_ANIMATION_STEALTH;
}

int CSWSObject::AnimationRunning(unsigned short nAnimation) {
    return nAnimation == SERVER_ANIMATION_RUN || nAnimation == SERVER_ANIMATION_RUN_INJURED;
}

int CSWSObject::AnimationStationary(unsigned short nAnimation) {
    switch (nAnimation) {
    case SERVER_ANIMATION_WALK: case SERVER_ANIMATION_WALK_BACKWARD: case SERVER_ANIMATION_RUN:
    case SERVER_ANIMATION_STRAFE_LEFT: case SERVER_ANIMATION_STRAFE_RIGHT:
    case SERVER_ANIMATION_WALK_STRAFE_LEFT: case SERVER_ANIMATION_WALK_STRAFE_RIGHT: case SERVER_ANIMATION_RUN_STRAFE_LEFT: case SERVER_ANIMATION_RUN_STRAFE_RIGHT:
    case SERVER_ANIMATION_WALK_INJURED: case SERVER_ANIMATION_RUN_INJURED:
    case SERVER_ANIMATION_STEALTH:
        return 0;
    }
    return 1;
}

int CSWSObject::HasSpellEffectApplied(unsigned int nSpellId) {
    if (nSpellId != static_cast<unsigned int>(-1)) {
        for (int i = 0; i < m_effects.m_count; i++) {
            if (static_cast<unsigned int>(m_effects.m_pArray[i]->m_spellId) == nSpellId)
                return 1;
        }
    }
    return 0;
}

CSWSExpression* CSWSObject::GetListenExpressionObj(int nExpressionId) {
    for (int i = 0; i < m_listenExpressions.m_count; i++) {
        CSWSExpression* pExpression = m_listenExpressions.m_pArray[i];
        if (pExpression->m_nId == nExpressionId)
            return pExpression;
    }
    return 0;
}

void CSWSObject::LoadVarTable(CResGFF* pGff, CResStruct* pStruct) {
    m_scriptVars.LoadVarTable(pGff, pStruct);
    m_varTable.LoadVarTable(pGff, pStruct);
}

void CSWSObject::SaveVarTable(CResGFF* pGff, CResStruct* pStruct) {
    m_scriptVars.SaveVarTable(pGff, pStruct);
    m_varTable.SaveVarTable(pGff, pStruct);
}

int CSWSObject::IsSoundPlayingInDialog() {
    return g_pAppManager->m_pClientExoApp->IsSoundPlayingInDialog();
}

void CSWSObject::StopSoundPlayingInDialog() {
    m_bDialogSoundStopped = 1;
    g_pAppManager->m_pClientExoApp->StopSoundPlayingInDialog();
}

int CSWSObject::IsStuntAnimationPlayingInDialog() {
    return g_pAppManager->m_pClientExoApp->GetInGameGui()->IsAnimationPlayingInDialog();
}

int CSWSObject::IsFadingInDialog() {
    return g_pAppManager->m_pClientExoApp->GetInGameGui()->IsFadingInDialog();
}

int CSWSObject::IsCameraAnimationPlayingInDialog() {
    return g_pAppManager->m_pClientExoApp->GetInGameGui()->IsCameraAnimationPlayingInDialog();
}

void CSWSObject::SendDialogModels(unsigned int nPlayerId) {
    m_pDialog->SendDialogModels(this, nPlayerId);
}

void CSWSObject::DeleteDialog() {
    if (m_pDialog) {
        delete m_pDialog;
        m_pDialog = 0;
    }
    m_bDialogFinished = 0;
}

CResRef CSWSObject::GetDialogResref() {
    return CResRef("");
}

void CSWSObject::SetArea(CSWSArea* pArea) {
    m_cachedAreaId = pArea ? pArea->m_objectId : CGameObjectArray::INVALIDOBJID;
}

int CSWSObject::ClearAction(CSWSObjectActionNode* pNode, int a) {
    K2_UNUSED(a);
    if (pNode->m_nActionId == AIACTION_DIALOG_OBJECT)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
    else if (pNode->m_nActionId == AIACTION_RESUME_DIALOG)
        m_bDialogPaused = 0;
    return 1;
}

int CSWSObject::IsDialogDelay()
{
    if (g_pAppManager->m_pClientExoApp->GetCanSendDialog(
            m_objectId,
            m_pDialog->m_lstParticipants.m_pInternal->m_pHead != 0 ? m_pDialog->m_oidCurrentPlayer : OBJECT_INVALID,
            m_pDialog) == 0)
        return 1;

    unsigned int nWaitFlags = m_dialogWaitFlags;
    unsigned int nDay, nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);

    int bWaiting = 0;
    if (nWaitFlags & 2)
        bWaiting = g_pAppManager->m_pClientExoApp->IsSoundPlayingInDialog();
    if (nWaitFlags & 4)
        bWaiting |= g_pAppManager->m_pClientExoApp->GetInGameGui()->IsAnimationPlayingInDialog();
    if (nWaitFlags & 1)
    {
        bWaiting |= g_pAppManager->m_pClientExoApp->GetModule()->IsCameraAnimated();
        g_pAppManager->m_pClientExoApp->GetInGameGui();
        g_pExoBase->m_pTimers->GetHighResolutionTimer();
    }
    if (nWaitFlags & 8)
        bWaiting |= g_pAppManager->m_pClientExoApp->GetInGameGui()->IsFadingInDialog();

#if KOTOR1  // K1 has no extra dialog flag test
    if (m_bSkipDialogRequested != 0 && m_pDialog->m_bSkippable != 0)
#else
    if (m_bSkipDialogRequested != 0 && m_pDialog->m_bSkippable != 0 && m_pDialog->m_bNodeUnskippable == 0)
#endif
    {
        m_bDialogSoundStopped = 1;
        g_pAppManager->m_pClientExoApp->StopSoundPlayingInDialog();
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->AddWorldTimes(
            nDay, nTod, 0, 1, &m_nDialogDelayDay, &m_nDialogDelayTod);
        return 0;
    }

    if (bWaiting != 0)
        return 1;

    return g_pAppManager->m_pServerExoApp->GetWorldTimer()->CompareWorldTimes(
               nDay, nTod, m_nDialogDelayDay, m_nDialogDelayTod) == -1;
}

void CSWSObject::TimeStampCameraAnimation() {
    m_nReserved3 = 0;
    unsigned long long nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_nCameraAnimStartMs = static_cast<unsigned int>(nNow / 1000);
}

void CSWSObject::RemoveEffectByCreator(unsigned long long oidCreator) {
    for (int i = 0; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_creatorId == oidCreator)
            RemoveEffect(m_effects.m_pArray[i]);
    }
}

void CSWSObject::RemoveEffectBySpellId(unsigned int nSpellId) {
    if (nSpellId == static_cast<unsigned int>(-1))
        return;
    for (int i = 0; i < m_effects.m_count; i++) {
        if (static_cast<unsigned int>(m_effects.m_pArray[i]->m_spellId) == nSpellId)
            RemoveEffect(m_effects.m_pArray[i]);
    }
}

int CSWSObject::RemoveEffectById(unsigned long long effectId)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    int i = 0;
    int bRemoved = 0;
    while (i < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_effectId == effectId) {
            CGameObject* pCreator =
                g_pAppManager->m_pServerExoApp->GetGameObject(pEffect->m_creatorId);
            if (pAIMaster->OnEffectRemoved(this, pEffect)
                == CSWSEffectListHandler::DELETE_EFFECT) {
                int nIndex = 0;
                while (nIndex < m_effects.m_count && m_effects.m_pArray[nIndex] != pEffect)
                    nIndex++;
                m_effects.DelIndex(nIndex);
                if (m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                    UpdateEffectPtrs();
                if (m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                    UpdateAttributesOnEffect(pEffect);
                delete pEffect;
                if (pCreator != 0 && pCreator->AsSWSObject() != 0)
                    pCreator->AsSWSObject()->RemoveEffectTarget(m_objectId);
                bRemoved = 1;
                continue;
            }
        }
        i++;
    }
    return bRemoved;
}

int CSWSObject::GetHasFeatEffectApplied(unsigned short nFeat) {
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (!pFeat)
        return 0;
    unsigned short nSpellId = static_cast<unsigned short>(pFeat->m_nSpellId);
    for (int i = 0; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_spellId == nSpellId)
            return 1;
    }
    return 0;
}

int CSWSObject::StopDialog() {
    if (AsSWSCreature() != 0)
        AsSWSCreature()->SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
    if (m_dialogOwnerId == OBJECT_INVALID)
        return 0;
    if (m_dialogOwnerId == m_objectId) {
        if (m_pDialog == 0)
            return 0;
        m_pDialog->RunEndConversationScript(this, 0);
        m_nDialogDelayDay = 0;
        m_nDialogDelayTod = 0;
        m_bDialogSendPending = 0;
        m_bDialogPaused = 0;
        delete m_pDialog;
        m_pDialog = 0;
    } else {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_dialogOwnerId,
                                                                           &pObject)
                == CGameObjectArray::SUCCESS) {
            static_cast<CSWSObject*>(pObject)->RemoveObjectFromDialog(m_objectId);
            m_dialogOwnerId = OBJECT_INVALID;
        }
    }
    return 1;
}

int CSWSObject::RemoveObjectFromDialog(unsigned long long objectId) {
    if (!m_pDialog)
        return 0;
    if (m_objectId == objectId)
        return 0;
    if (!m_pDialog->m_lstParticipants.m_pInternal->m_pHead) {
        // The last participant already left: only the OBJECT_INVALID caller may end the dialog.
        if (objectId != OBJECT_INVALID)
            return 1;
        return StopDialog();
    }
    if (m_pDialog->RemovePlayer(objectId))
        return 1;
    return StopDialog();
}

void CSWSObject::DeleteCurrentAIAction() {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return;
    // ClearAction reports 1 when the action really was torn down; anything else means it declined
    // and the node stays queued.
    if (ClearAction(static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->m_pHead->m_pData), 1) != 1)
        return;
    delete static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->RemoveHead());
}

void CSWSObject::TestActionList() {
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    if (m_actionList.m_pInternal->m_pHead == 0)
        return;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->m_pHead->m_pData);
    while (pAction != 0)
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
}

void CSWSObject::SetGroupInterruptable(unsigned short nGroupId, int bInterruptable) {
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    if (m_actionList.m_pInternal->m_pHead == 0)
        return;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->m_pHead->m_pData);
    while (pAction != 0) {
        if (pAction->m_nGroupId == nGroupId)
            pAction->m_bInterruptable = bInterruptable;
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
    }
}

CSWSObjectActionNode* CSWSObject::GetNodeById(unsigned short nId) {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
    while (1) {
        if (pAction->m_nActionId == nId)
            return pAction;
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
        if (pNode == 0)
            return 0;
    }
}

CSWSObjectActionNode* CSWSObject::GetNodeById(unsigned short nId, unsigned short nGroupId) {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
    while (1) {
        if (pAction->m_nActionId == nId && pAction->m_nGroupId == nGroupId)
            return pAction;
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
        if (pNode == 0)
            return 0;
    }
}

int CSWSObject::GetNumActionGroups() {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
    // The head's group id is read before the head is tested for null.
    unsigned short nPrevGroup = pAction->m_nGroupId;
    int nGroups = (pAction != 0);
    while (1) {
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
        if (pNode == 0)
            break;
        if (pAction->m_nGroupId != nPrevGroup)
            nGroups++;
        nPrevGroup = pAction->m_nGroupId;
    }
    return nGroups;
}

CExoLinkedListNode* CSWSObject::GetPositionByGroupIndex(int nIndex) {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return 0;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pNode));
    // Group 0 starts at the head, so the walk is skipped entirely.
    if (nIndex == 0)
        return pNode;
    if (pAction == 0)
        return 0;
    unsigned short nPrevGroup = pAction->m_nGroupId;
    int nGroups = 0;
    while (nGroups != nIndex) {
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetNext(pNode));
        if (pAction == 0)
            return 0;
        if (pAction->m_nGroupId != nPrevGroup)
            nGroups++;
        nPrevGroup = pAction->m_nGroupId;
    }
    return pNode;
}

unsigned short CSWSObject::GetActionByGroupId(unsigned short nGroupId,
                                              CSWSObjectActionNode** ppNode) {
    // Reports the last acceptable action of the run (each hit overwrites), or the no-action
    // sentinel when the group holds none.
    *ppNode = 0;
    unsigned short nActionId = AIACTION_NONE;
    if (m_actionList.m_pInternal->m_pHead == 0)
        return nActionId;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
    if (pAction == 0)
        return nActionId;
    // The run is contiguous, so the first node past it ends the search, but only once the run has
    // been entered (bInGroup).
    int bInGroup = 0;
    while (pAction != 0) {
        if (pAction->m_nGroupId == nGroupId) {
            if (GetAcceptableAction(pAction->m_nActionId) == 1) {
                nActionId = static_cast<unsigned short>(pAction->m_nActionId);
                *ppNode = pAction;
            }
            bInGroup = 1;
        } else if (bInGroup == 1) {
            break;
        }
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
    }
    return nActionId;
}

extern int RenderPartyFollow;

void CSWSObject::ClearAllActions(int bClearCombatRound) {
#if !KOTOR1  // K1 has no self-cast guard here
    if (AsSWSObject() == 0)
        return;
#endif

    if (RenderPartyFollow) {
        if (AsSWSCreature() != 0 && AsSWSCreature()->m_bInParty != 0) {
            const char* pszTag = m_sTag.CStr();
            (void)pszTag;
        }
    }

    // The queue is only cleared for an object that is exactly commandable.
    if (m_bCommandable != 1)
        return;

    // The cursor is advanced before ClearAction runs, which may splice the queue underneath us:
    // the dropped node is held separately and removed by position afterwards.
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
        CExoLinkedListNode* pPos = pNode;
        m_actionList.m_pInternal->GetNext(pNode);
        // The current action is the queue head, read straight off m_pHead rather than looked up
        // again.
        CSWSObjectActionNode* pCurrent = 0;
        if (m_actionList.m_pInternal->m_pHead != 0)
            pCurrent = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->m_pHead->m_pData);
        if (ClearAction(pAction, pCurrent == pAction) == 1) {
            CSWSObjectActionNode* pRemoved = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->Remove(pPos));
            if (pRemoved != 0)
                delete pRemoved;
        }
    }

    if (AsSWSCreature() == 0)
        return;
    AsSWSCreature()->m_pPathfindInfo->ResetAtEndOfPath();
    AsSWSCreature()->m_nMoveState = 1;
    AsSWSCreature()->m_oidAttemptedMovementTarget = OBJECT_INVALID;
    CPathfindInformation* pPathInfo = AsSWSCreature()->m_pPathfindInfo;
    pPathInfo->m_oidPathTarget = OBJECT_INVALID;
    pPathInfo->m_vPathTarget.x = 0.0f;
    pPathInfo->m_vPathTarget.y = 0.0f;
    pPathInfo->m_vPathTarget.z = 0.0f;
    if (bClearCombatRound)
        AsSWSCreature()->m_pCombatRound->ClearAllAttacks();
    const char* pszCreatureTag = AsSWSCreature()->m_sTag.CStr();
    (void)pszCreatureTag;
    if (AsSWSCreature()->m_bInParty != 0) {
        unsigned long long nClientId =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(AsSWSCreature()->m_objectId);
        int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(nClientId);
        if (nIndex != -1)
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;
    }
}

void CSWSObject::RemoveSomeEffectsOfDurationType(unsigned short nDurationType) {
    int i = 0;
    unsigned int nRemoved = 0;
    while (static_cast<unsigned int>(i) < static_cast<unsigned int>(m_effects.m_count)) {
        if ((m_effects.m_pArray[i]->m_durationTypeBits & 7) == nDurationType) {
            RemoveEffectById(m_effects.m_pArray[i]->m_effectId);
            nRemoved++;
            if (nRemoved > 20)
                break;
            i = 0;
        }
        i++;
    }
}

void CSWSObject::RemoveEffectByType(unsigned int nSpellId, int nEffectType, int nInteger) {
    for (int i = 0; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (static_cast<unsigned int>(pEffect->m_spellId) == nSpellId && pEffect->m_subType == nEffectType &&
            pEffect->GetInteger(0) == nInteger) {
            RemoveEffect(m_effects.m_pArray[i]);
        }
    }
}

void CSWSObject::RemoveLoopingVisualEffect(unsigned short nVisualEffectId) {
    // Scans backwards: the last matching entry wins, and the scan stops there.
    for (int i = m_visualEffects.m_count - 1; i >= 0; i--) {
        if (m_visualEffects.m_pArray[i]->m_nVisualEffectId == nVisualEffectId) {
            delete m_visualEffects.m_pArray[i];
            m_visualEffects.m_pArray[i] = 0;
            m_visualEffects.DelIndex(i);
            return;
        }
    }
}

void CSWSObject::AddLoopingVisualEffect(unsigned short nVisualEffectId, unsigned long long targetId,
                                        unsigned char nBodyPart) {
    CLoopingVisualEffect* pEffect = new CLoopingVisualEffect;
    pEffect->m_nVisualEffectId = nVisualEffectId;
    pEffect->m_targetId = targetId;
    pEffect->m_nBodyPart = nBodyPart;
    m_visualEffects.Add(pEffect);
}

int CSWSObject::TestListenExpression(const CExoString& sString) {
    for (int i = 0; i < m_listenExpressions.m_count; i++) {
        CSWSExpression* pExpression = m_listenExpressions.m_pArray[i];
        if (pExpression != 0 && pExpression->TestString(sString, 1))
            return m_listenExpressions.m_pArray[i]->m_nId;
    }
    return -1;
}

void CSWSObject::ClearMatchedExpressionStrings() {
    for (int i = 0; i < m_matchedExpressionStrings.m_count; i++) {
        if (m_matchedExpressionStrings.m_pArray[i] != 0) {
            delete m_matchedExpressionStrings.m_pArray[i];
            m_matchedExpressionStrings.m_pArray[i] = 0;
        }
    }
    m_matchedExpressionStrings.Clear();
}

void CSWSObject::AddMatchedExpressionString(const CExoString& sString) {
    m_matchedExpressionStrings.Add(new CExoString(sString));
}

void CSWSObject::SetDialogDelay(float fDelay) {
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&m_nDialogDelayStartDay,
                                                                 &m_nDialogDelayStartTod);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->AddWorldTimes(
        m_nDialogDelayStartDay, m_nDialogDelayStartTod, 0,
        static_cast<unsigned int>(CWorldTimer::MILLISECONDS_IN_SECOND * fDelay),
        &m_nDialogDelayDay, &m_nDialogDelayTod);
}

void CSWSObject::SetPosition(const Vector& pos, int a, int b, int c) {
    K2_UNUSED(b);
    if (equal(m_position, pos))
        return;
    if (c == 0)
        m_updateFlags[0] |= 2;
    CSWArea* pArea = GetArea();
    m_position = pos;
    if (a != 0) {
        if (AsSWSCreature() != 0)
            AsSWSCreature();
        if (pArea != 0)
            static_cast<CSWSArea*>(pArea)->UpdatePositionInObjectsArray(this);
    }
}

int CSWSObject::GetLastDamageAmountByFlags(int nFlags) {
    for (int i = 0; i < 15; i++) {
        if ((1 << i) == nFlags)
            return m_paLastDamageAmounts[i];
    }
    return 0;
}

void CSWSObject::PlaySoundSet(CSWSPlayer* pPlayer, unsigned char nSoundSet) {
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (pPlayer != 0) {
        pMessage->SendServerToPlayerVoiceChat_Play(pPlayer, m_objectId, nSoundSet);
        return;
    }
    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pEach = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pEach != 0)
            pMessage->SendServerToPlayerVoiceChat_Play(pEach, m_objectId, nSoundSet);
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
}

int CSWSObject::SendDialogEntry(unsigned int nEntry) {
    if (m_pDialog == 0)
        return 0;
    if (nEntry != CSWSDialog::NODE_INDEX_NONE && (m_bDialogPaused != 0 || IsDialogDelay() != 0)) {
        m_bDialogSendPending = 1;
        m_nPendingDialogEntry = nEntry;
        return 1;
    }
    m_bSkipDialogRequested = 0;
    unsigned int nStatus = 0;
    int bSent = m_pDialog->SendDialogEntry(this, CSWSDialog::PLAYER_ID_ALL, nEntry, nStatus);
    if (nStatus == 10) {
        m_bDialogSendPending = 1;
        m_nPendingDialogEntry = nEntry;
        return 1;
    }
    if (bSent != 0)
        return 1;
    if (m_pDialog == 0)
        return 0;
    m_pDialog->RunEndConversationScript(this, 0);
    m_nDialogDelayDay = 0;
    m_nDialogDelayTod = 0;
    m_bDialogSendPending = 0;
    m_bDialogFinished = 1;
    return 0;
}

int CSWSObject::ReplyDialog(unsigned int listenerId, unsigned int replyIndex, int isEndDialog,
                            unsigned int areaId, int flag) {
    if (m_pDialog == 0)
        return 0;
    if (m_pDialog->HandleReply(listenerId, this, replyIndex, isEndDialog, areaId, flag))
        return 1;
    // The reply could not be delivered: run the conversation down and disarm the delay.
    if (m_pDialog != 0) {
        m_pDialog->RunEndConversationScript(this, 0);
        m_nDialogDelayDay = 0;
        m_nDialogDelayTod = 0;
        m_bDialogSendPending = 0;
        m_bDialogFinished = 1;
    }
    return 1;
}

int CSWSObject::UpdateDialog() {
    if (m_dialogOwnerId == OBJECT_INVALID)
        return 0;
    if (m_dialogOwnerId != m_objectId)
        return 0;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateDialogLipSync();
    m_pDialog->ClearAllPlayerHostileActions();
    if (m_bDialogPaused != 0)
        return 1;
    if (!IsDialogDelay() && m_bDialogSendPending != 0) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut == 0
            && m_bDialogSoundStopped != 0
            && g_pAppManager->m_pClientExoApp->IsSoundPlayingInDialog())
            return 1;
        m_bDialogSendPending = 0;
        m_bDialogSoundStopped = 0;
        if (m_nPendingDialogEntry != -1)
            return SendDialogEntry(m_nPendingDialogEntry);
        return SendDialogReplies();
    }
    if (m_pDialog->m_bEnded != 0 && !g_pAppManager->m_pClientExoApp->IsSoundPlayingInDialog())
        StopDialog();
    return 1;
}

int CSWSObject::SendDialogReplies() {
    if (m_pDialog == 0)
        return 0;
    if (m_bDialogPaused != 0 || IsDialogDelay()) {
        m_bDialogSendPending = 1;
        m_nPendingDialogEntry = -1;
        return 1;
    }
    m_bSkipDialogRequested = 0;
    if (!m_pDialog->SendDialogReplies(this, static_cast<unsigned int>(-1))) {
        m_pDialog->RunEndConversationScript(this, 0);
        m_nDialogDelayDay = 0;
        m_nDialogDelayTod = 0;
        m_bDialogSendPending = 0;
        m_bDialogFinished = 1;
    }
    return 1;
}

void CSWSObject::AddActionNodeParameter(CSWSObjectActionNode* pNode, unsigned int nIndex,
                                        unsigned int nType, void* pValue)
{
    pNode->m_nNumParameters = static_cast<unsigned short>(nIndex + 1);
    if (nIndex > 12)
        return;

    pNode->m_paramTypes[nIndex] = nType;
    switch (nType)
    {
    case ACTION_PARAM_INT:
        pNode->m_params[nIndex].m_n = *static_cast<int*>(pValue);
        break;
    case ACTION_PARAM_FLOAT:
        pNode->m_params[nIndex].m_n = *static_cast<int*>(pValue);
        break;
    case ACTION_PARAM_OBJECT:
        pNode->m_params[nIndex].m_n = *static_cast<int*>(pValue);
        break;
    case ACTION_PARAM_STRING:
        {
            CExoString* pString = new CExoString;
            pNode->m_params[nIndex].m_n = reinterpret_cast<unsigned long long>(pString);
            *pString = *static_cast<CExoString*>(pValue);
        }
        break;
    case ACTION_PARAM_POINTER:
        pNode->m_params[nIndex].m_n = reinterpret_cast<unsigned long long>(pValue);
        break;
    }
}

void CSWSObject::AddAction(unsigned int nActionId, unsigned short nGroupId,
                           unsigned int nType1, void* pParam1,
                           unsigned int nType2, void* pParam2,
                           unsigned int nType3, void* pParam3,
                           unsigned int nType4, void* pParam4,
                           unsigned int nType5, void* pParam5,
                           unsigned int nType6, void* pParam6,
                           unsigned int nType7, void* pParam7,
                           unsigned int nType8, void* pParam8,
                           unsigned int nType9, void* pParam9,
                           unsigned int nType10, void* pParam10,
                           unsigned int nType11, void* pParam11,
                           unsigned int nType12, void* pParam12,
                           unsigned int nType13, void* pParam13)
{
    CSWSObjectActionNode* pNode = new CSWSObjectActionNode;
    pNode->m_nActionId = nActionId;
    pNode->m_bInterruptable = 1;

    if (nGroupId == ACTION_GROUP_NEW)
    {
        unsigned short nNewGroupId = m_curGroupId;
        unsigned short nNextGroupId = nNewGroupId + 1;
        if (nNextGroupId == ACTION_GROUP_NEW)
            nNextGroupId = 0;
        m_curGroupId = nNextGroupId;
        m_prevGroupId = nNewGroupId;
        pNode->m_nGroupId = nNewGroupId;
    }
    else if (nGroupId == ACTION_GROUP_PREVIOUS)
    {
        pNode->m_nGroupId = m_prevGroupId;
    }
    else
    {
        pNode->m_nGroupId = nGroupId;
    }

    if (nType1 != 0)
    {
        AddActionNodeParameter(pNode, 0, nType1, pParam1);
        if (nType2 != 0)
        {
            AddActionNodeParameter(pNode, 1, nType2, pParam2);
            if (nType3 != 0)
            {
                AddActionNodeParameter(pNode, 2, nType3, pParam3);
                if (nType4 != 0)
                {
                    AddActionNodeParameter(pNode, 3, nType4, pParam4);
                    if (nType5 != 0)
                    {
                        AddActionNodeParameter(pNode, 4, nType5, pParam5);
                        if (nType6 != 0)
                        {
                            AddActionNodeParameter(pNode, 5, nType6, pParam6);
                            if (nType7 != 0)
                            {
                                AddActionNodeParameter(pNode, 6, nType7, pParam7);
                                if (nType8 != 0)
                                {
                                    AddActionNodeParameter(pNode, 7, nType8, pParam8);
                                    if (nType9 != 0)
                                    {
                                        AddActionNodeParameter(pNode, 8, nType9, pParam9);
                                        if (nType10 != 0)
                                        {
                                            AddActionNodeParameter(pNode, 9, nType10, pParam10);
                                            if (nType11 != 0)
                                            {
                                                AddActionNodeParameter(pNode, 10, nType11, pParam11);
                                                if (nType12 != 0)
                                                {
                                                    AddActionNodeParameter(pNode, 11, nType12, pParam12);
                                                    if (nType13 != 0)
                                                    {
                                                        AddActionNodeParameter(pNode, 12, nType13, pParam13);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    m_actionList.m_pInternal->AddTail(pNode);
    TestActionList();
}

void CSWSObject::AddActionToFront(unsigned int nActionId, unsigned short nGroupId,
                                  unsigned int nType1, void* pParam1,
                                  unsigned int nType2, void* pParam2,
                                  unsigned int nType3, void* pParam3,
                                  unsigned int nType4, void* pParam4,
                                  unsigned int nType5, void* pParam5,
                                  unsigned int nType6, void* pParam6,
                                  unsigned int nType7, void* pParam7,
                                  unsigned int nType8, void* pParam8,
                                  unsigned int nType9, void* pParam9,
                                  unsigned int nType10, void* pParam10,
                                  unsigned int nType11, void* pParam11,
                                  unsigned int nType12, void* pParam12,
                                  unsigned int nType13, void* pParam13)
{
    CSWSObjectActionNode* pNode = new CSWSObjectActionNode;
    pNode->m_nActionId = nActionId;
    pNode->m_bInterruptable = 1;

    if (nGroupId == ACTION_GROUP_NEW)
    {
        unsigned short nNewGroupId = m_curGroupId;
        unsigned short nNextGroupId = nNewGroupId + 1;
        if (nNextGroupId == ACTION_GROUP_NEW)
            nNextGroupId = 0;
        m_curGroupId = nNextGroupId;
        m_prevGroupId = nNewGroupId;
        pNode->m_nGroupId = nNewGroupId;
    }
    else if (nGroupId == ACTION_GROUP_PREVIOUS)
    {
        pNode->m_nGroupId = m_prevGroupId;
    }
    else
    {
        pNode->m_nGroupId = nGroupId;
    }

    if (nType1 != 0)
    {
        AddActionNodeParameter(pNode, 0, nType1, pParam1);
        if (nType2 != 0)
        {
            AddActionNodeParameter(pNode, 1, nType2, pParam2);
            if (nType3 != 0)
            {
                AddActionNodeParameter(pNode, 2, nType3, pParam3);
                if (nType4 != 0)
                {
                    AddActionNodeParameter(pNode, 3, nType4, pParam4);
                    if (nType5 != 0)
                    {
                        AddActionNodeParameter(pNode, 4, nType5, pParam5);
                        if (nType6 != 0)
                        {
                                   AddActionNodeParameter(pNode, 5, nType6, pParam6);
                                   if (nType7 != 0)
                                   {
                                       AddActionNodeParameter(pNode, 6, nType7, pParam7);
                                       if (nType8 != 0)
                                       {
                                           AddActionNodeParameter(pNode, 7, nType8, pParam8);
                                           if (nType9 != 0)
                                           {
                                               AddActionNodeParameter(pNode, 8, nType9, pParam9);
                                               if (nType10 != 0)
                                               {
                                                   AddActionNodeParameter(pNode, 9, nType10, pParam10);
                                                   if (nType11 != 0)
                                                   {
                                                       AddActionNodeParameter(pNode, 10, nType11, pParam11);
                                                       if (nType12 != 0)
                                                       {
                                                           AddActionNodeParameter(pNode, 11, nType12, pParam12);
                                                           if (nType13 != 0)
                                                           {
                                                               AddActionNodeParameter(pNode, 12, nType13, pParam13);
                                                           }
                                                       }
                                                   }
                                               }
                                           }
                                       }
                                   }
                        }
                    }
                }
            }
        }
    }

    m_actionList.m_pInternal->AddHead(pNode);
    TestActionList();
}

int CSWSObject::AddOpenDoorAction(unsigned long long oidDoor, int bRun)
{
    if (m_bCommandable == 0)
        return 0;

    AddAction(AIACTION_OPEN_DOOR, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidDoor,
              ACTION_PARAM_INT, &bRun);
    return 1;
}

int CSWSObject::AddCloseDoorAction(unsigned long long oidDoor, int bRun)
{
    if (m_bCommandable == 0)
        return 0;

    AddAction(AIACTION_CLOSE_DOOR, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidDoor,
              ACTION_PARAM_INT, &bRun);
    return 1;
}

int CSWSObject::AddDoorSaberAction(unsigned long long oidDoor)
{
    if (m_bCommandable == 0)
        return 0;

    AddAction(AIACTION_DOOR_SABER, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidDoor);
    return 1;
}

int CSWSObject::AddLockObjectAction(unsigned long long oidTarget)
{
    if (AsSWSCreature() != 0 && AsSWSCreature()->m_pStats->GetCanUseSkill(SKILL_SECURITY) == 0)
        return 0;
    if (m_bCommandable == 0)
        return 0;

    AddAction(AIACTION_LOCK_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget);
    return 1;
}

int CSWSObject::AddUnlockObjectAction(unsigned long long oidTarget, unsigned long long a2, int a3)
{
    if (AsSWSCreature() != 0 && AsSWSCreature()->m_pStats->GetCanUseSkill(SKILL_SECURITY) == 0)
    {
        AsSWSCreature()->SendFeedbackMessage(FEEDBACK_CANNOT_USE_SKILL, 0);
        return 0;
    }
    if (m_bCommandable == 0)
        return 0;

    AddAction(AIACTION_UNLOCK_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget,
              ACTION_PARAM_OBJECT, &a2, ACTION_PARAM_INT, &a3);
    return 1;
}

int CSWSObject::AddSwitchWeaponsAction(int bImmediate)
{
    if (m_bCommandable == 0)
        return 0;

    CSWSCreature* pCreature = AsSWSCreature();
    if (pCreature == 0)
        return 0;

    if (bImmediate != 0)
    {
        ClearAllActions(1);
        AddAction(AIACTION_SWITCH_WEAPONS, ACTION_GROUP_NEW);
        return 1;
    }

    if (pCreature->m_bCombatState != 1 && pCreature->m_pCombatRound->GetActionPending() == 0)
        pCreature->ActionManager(4);

    if (pCreature->m_actionList.m_pInternal->m_pHead == 0 ||
        pCreature->m_actionList.m_pInternal->m_pHead->m_pData == 0)
    {
        ClearAllActions(1);
        AddAction(AIACTION_SWITCH_WEAPONS, ACTION_GROUP_NEW);
        return 1;
    }

    pCreature->m_pCombatRound->AddSwitchWeaponsAction();
    if (GetNodeById(63) == 0)
    {
        int nParam = 1;
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nParam);
    }
    return 1;
}

void CSWSObject::SetDeleteNoFade(int bDeleteNoFade)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    if (pPlayer != 0)
    {
        CExoLinkedListNode* pPos = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
        while (pPos != 0)
        {
            CLastUpdateObject* pLast = static_cast<CLastUpdateObject*>(
                pPlayer->m_pLastUpdateObjectList->m_pInternal->GetAtPos(pPos));
            if (pLast->m_objectId == m_objectId)
            {
                pLast->m_bDeleteNoFade = bDeleteNoFade;
                return;
            }
            pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
        }
    }
    m_bDeleteNoFade = bDeleteNoFade;
}

void CSWSObject::SetKeepCorpse(int bKeepCorpse)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    if (pPlayer != 0)
    {
        CExoLinkedListNode* pPos = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
        while (pPos != 0)
        {
            CLastUpdateObject* pLast = static_cast<CLastUpdateObject*>(
                pPlayer->m_pLastUpdateObjectList->m_pInternal->GetAtPos(pPos));
            if (pLast->m_objectId == m_objectId)
            {
                pLast->m_bKeepCorpse = bKeepCorpse;
                return;
            }
            pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
        }
    }
    m_bKeepCorpse = bKeepCorpse;
}

void CSWSObject::SetFadeOutTime(unsigned int nFadeOutTime)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    if (pPlayer != 0)
    {
        CExoLinkedListNode* pPos = pPlayer->m_pLastUpdateObjectList->m_pInternal->m_pHead;
        while (pPos != 0)
        {
            CLastUpdateObject* pLast = static_cast<CLastUpdateObject*>(
                pPlayer->m_pLastUpdateObjectList->m_pInternal->GetAtPos(pPos));
            if (pLast->m_objectId == m_objectId)
            {
                pLast->m_nFadeOutTime = nFadeOutTime;
                return;
            }
            pPlayer->m_pLastUpdateObjectList->m_pInternal->GetNext(pPos);
        }
    }
    m_nFadeOutTime = nFadeOutTime;
}

CGameObject* CSWSObject::GetClientObject() {
    if (m_pClientObject == 0) {
        unsigned int nClientId =
            static_cast<unsigned int>(g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId));
        CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pObject != 0) {
            m_pClientObject = static_cast<CSWCObject*>(pObject->AsSWCObject());
            if (m_pClientObject != 0)
                m_pClientObject->AttachmentFromServerObject(this);
        }
    }
    return static_cast<CGameObject*>(m_pClientObject);
}

CSWArea* CSWSObject::GetArea() {
    CGameObject* pObject = 0;
    g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_cachedAreaId, &pObject);
    if (pObject == 0)
        return 0;
    return static_cast<CSWArea*>(pObject->AsSWSArea());
}

void CSWSObject::SetDamageImmunity(unsigned short nFlags, char nValue) {
    char nClamped = nValue > -100 ? nValue : -100;
    nClamped = nClamped < 100 ? nClamped : 100;
    for (int i = 0; i < 15; i++) {
        if (nFlags & (1 << i)) {
            m_paDamageImmunity[i] = nClamped;
            break;
        }
    }
}

char CSWSObject::GetDamageImmunityByFlags(unsigned short nFlags) {
    short nBest = 0;
    for (int i = 0; i < 15; i++) {
        if (nFlags & (1 << i)) {
            if (nBest == 0)
                nBest = static_cast<char>(GetDamageImmunity(static_cast<unsigned char>(i)));
            else if (static_cast<char>(GetDamageImmunity(static_cast<unsigned char>(i))) < nBest)
                nBest = static_cast<char>(GetDamageImmunity(static_cast<unsigned char>(i)));
        }
    }
    int nResult = nBest < 100 ? nBest : 100;
    return static_cast<char>(nResult > -100 ? nResult : -100);
}

void CSWSObject::ClearAllEffects(int bKeepOwn) {
#if KOTOR1  // K1 has the self-cast guard first
    if (AsSWSObject() == 0)
        return;
#endif
    if (m_effects.m_count == 0)
        return;
    int i = m_effects.m_count;
    while (i > 0) {
        i--;
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect != 0 &&
            (pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_INNATE &&
            (pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED &&
            CSWSEffectListHandler::EFFECT_SETSTATE_INTERNAL != pEffect->m_subType &&
            (bKeepOwn == 0 || pEffect->m_creatorId != m_objectId)) {
            RemoveEffect(pEffect);
            i = m_effects.m_count;
        }
    }
    m_bCommandable = 1;
}

void CSWSObject::RemoveGroup(unsigned short nGroupId) {
    if (m_actionList.m_pInternal->m_pHead == 0)
        return;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
    while (1) {
        CExoLinkedListNode* pCurrentPos = pNode;
        CSWSObjectActionNode* pCurrent = pAction;
        unsigned short nId = pCurrent->m_nGroupId;
        pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetNext(pNode));
        if (nId == nGroupId) {
            m_actionList.m_pInternal->Remove(pCurrentPos);
            pCurrent->~CSWSObjectActionNode();
            operator delete(pCurrent);
        }
        if (pNode == 0)
            return;
    }
}

void CSWSObject::BroadcastSpellData(CSWSpell* pSpell, CSWCCMessageData* pData)
{
    K2_UNUSED(pSpell);
    if (GetArea() == 0)
        return;
    unsigned long long oidPC = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    CSWSCreature* pPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidPC);
    if (pPC == 0)
        return;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidPC);
    if (pPlayer == 0)
        return;
    Vector vDelta = pPC->m_position - m_position;
    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
        if (AsSWSCreature() != 0 && pData->GetObjectID(1) != OBJECT_INVALID) {
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pData->GetObjectID(1));
            if (pItem != 0)
                pData->SetInteger(3, pItem->m_baseItem);
        }
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
            pPlayer->m_nPlayerId, CC_MESSAGE_SPELL, pData, 0);
    }
}

int CSWSObject::RunDialogOneLiner(const CExoString& sResRef, unsigned long long targetId)
{
    CResRef rDialog;
    CSWSDialog dialog;
    CExoLocString sText;
    CResRef rSound;
    CResRef rScript;
    CGameObjectArray* pObjectArray;
    CSWSMessage* pMessage;
    CResGFF* pGff;
    int bLoaded;
    unsigned char nGender = CTlkTable::GENDER_MALE;
    pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (sResRef == "")
        rDialog = GetDialogResref();
    else
        rDialog = sResRef;
    if (rDialog == const_cast<char*>(""))
        return 0;
    pGff = new CResGFF(RESTYPE_DLG, const_cast<char*>("DLG "), rDialog);
    if (pGff->m_nValid == 0) {
        delete pGff;
        return 0;
    }
    bLoaded = dialog.LoadDialog(pGff, 0);
    delete pGff;
    if (bLoaded == 0)
        return 0;
    if (dialog.GetStartEntryOneLiner(this, sText, rSound, rScript) == 0)
        return 0;
    CGameObject* pObject;
    if (pObjectArray->GetGameObject(targetId, &pObject) == CGameObjectArray::SUCCESS
            && pObject->AsSWSObject() != 0)
        nGender = pObject->AsSWSObject()->GetGender();
    pMessage->SendServerToPlayerChatMultiLangMessage(8, m_objectId, sText, targetId, nGender,
        0, 0, 0, rSound, OBJECT_INVALID, 0);
    dialog.RunScript(this, rScript);
    return 1;
}

void CSWSObject::BroadcastSafeProjectile(unsigned long long selfId, unsigned long long targetId,
                                         const Vector& srcPos, const Vector& dstPos,
                                         unsigned int level, unsigned char pathCount,
                                         unsigned int spellId, unsigned char zero1,
                                         unsigned char pathType, int zero2, int zero3)
{
    unsigned char nWeaponBaseItem = 0xff;
    CSWSCreature* pSelf = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(selfId);
    if (pSelf != 0) {
        CSWSItem* pWeapon = static_cast<CSWSItem*>(pSelf->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pWeapon != 0)
            nWeaponBaseItem = static_cast<unsigned char>(pWeapon->m_baseItem);
    }
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return;
    float fMaxDistSq = CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE
                     * CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE;
    for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp
            ->GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
        if (pCreature != 0) {
            CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp
                ->GetClientObjectByObjectId(pCreature->m_objectId);
            if (pPlayer != 0) {
                Vector vSrcDelta = pCreature->m_position - srcPos;
                Vector vDstDelta = pCreature->m_position - dstPos;
                if (vSrcDelta.x * vSrcDelta.x + vSrcDelta.y * vSrcDelta.y
                            + vSrcDelta.z * vSrcDelta.z <= fMaxDistSq
                        || vDstDelta.x * vDstDelta.x + vDstDelta.y * vDstDelta.y
                            + vDstDelta.z * vDstDelta.z <= fMaxDistSq) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()
                        ->SendServerToPlayerSafeProjectileSpell(pPlayer, selfId, targetId, srcPos,
                            dstPos, level, pathCount, spellId, nWeaponBaseItem, zero1, pathType,
                            zero2, zero3);
                }
            }
        }
    }
}

void CSWSObject::BroadcastSafeProjectile(unsigned long long selfId, unsigned long long targetId,
                                         const Vector& srcPos, const Vector& dstPos,
                                         unsigned int level, unsigned char pathCount,
                                         int projectileType, unsigned char zero1,
                                         unsigned char pathType, int zero2)
{
    if (GetArea() == 0)
        return;
    GetArea();
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    float fMaxDistSq = CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE
                     * CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE;
    for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp
            ->GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
        if (pCreature != 0) {
            CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp
                ->GetClientObjectByObjectId(pCreature->m_objectId);
            if (pPlayer != 0) {
                Vector vSrcDelta = pCreature->m_position - srcPos;
                Vector vDstDelta = pCreature->m_position - dstPos;
                if (vSrcDelta.x * vSrcDelta.x + vSrcDelta.y * vSrcDelta.y
                            + vSrcDelta.z * vSrcDelta.z <= fMaxDistSq
                        || vDstDelta.x * vDstDelta.x + vDstDelta.y * vDstDelta.y
                            + vDstDelta.z * vDstDelta.z <= fMaxDistSq) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()
                        ->SendServerToPlayerSafeProjectileProjectile(pPlayer, selfId, targetId,
                            srcPos, dstPos, level, pathCount, projectileType, zero1, pathType,
                            zero2);
                }
            }
        }
    }
}

void CSWSObject::SpellCastAndImpact(unsigned int spellId, const Vector& loc,
                                    unsigned long long targetId, unsigned char a5,
                                    unsigned long long itemId, int a7, int a8,
                                    unsigned char pathType, int bSuppressFeedback)
{
    K2_UNUSED(a5); K2_UNUSED(a7); K2_UNUSED(a8);
    CExoString sUnused;
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(spellId);
    if (pSpell == 0)
        return;
    unsigned char nPathCount = 6 + (pathType != 0);
    if (m_bCastBroadcastProjectile == 0) {
#if !KOTOR1
        int nFinalForceCost = 0;
#endif
        if (bSuppressFeedback == 0 && AsSWSCreature() != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, m_objectId);
            pData->SetObjectID(1, itemId);
            pData->SetObjectID(2, targetId);
            pData->SetInteger(0, spellId);
            pData->SetInteger(1, pSpell->m_nBaseForcePoints);
#if KOTOR1
            // K1: no room force rating, no Force Body vitality cost, no HP field.
            int nAlignment = AsSWSCreature()->m_pStats->m_nAlignment;
            pData->SetInteger(2, pSpell->GetAdjustedForcePoints(nAlignment,
                AsSWSCreature()->m_bInParty));
            CSWSCreatureStats* pStats = AsSWSCreature()->m_pStats;
            pData->SetInteger(3, static_cast<short>(pStats->m_nCurrentForcePoints + pStats->m_nTemporaryForcePoints));
#else
            int nAlignment = AsSWSCreature()->m_pStats->m_nAlignment;
            CSWSArea* pArea = static_cast<CSWSArea*>(AsSWSCreature()->GetArea());
            int nRoomForceRating = 0;
            if (pArea != 0)
                nRoomForceRating = pArea->GetRoomForceRating(AsSWSCreature()->m_position);
            int nVitalityCost = 0;
            CSWSCreature* pCreature = AsSWSCreature();
            CExoArrayList<CGameEffect*>& aEffects = pCreature->m_effects;
            for (int i = 0; i < aEffects.m_count; i++) {
                CGameEffect* pEffect = aEffects[i];
                if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
                    int nTier = pEffect->GetInteger(0);
                    if (nTier == CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE)
                        goto noForceBody;
                    int nBaseCost = pSpell->GetAdjustedForcePoints(nAlignment,
                        AsSWSCreature()->m_bInParty, nRoomForceRating,
                        AsSWSCreature()->m_stanceId, AsSWSCreature()->m_pStats->m_nCHAMod);
                    int nPercent = 0;
                    switch (nTier) {
                    case CSWSEffectListHandler::FORCE_BODY_LEVEL_BASIC:    nPercent = 50; break;
                    case CSWSEffectListHandler::FORCE_BODY_LEVEL_IMPROVED: nPercent = 40; break;
                    case CSWSEffectListHandler::FORCE_BODY_LEVEL_MASTER:   nPercent = 30; break;
                    }
                    nVitalityCost = nBaseCost * nPercent / 100;
                    nFinalForceCost = nVitalityCost;
                    goto haveForceCost;
                }
            }
noForceBody:
            nFinalForceCost = pSpell->GetAdjustedForcePoints(nAlignment,
                AsSWSCreature()->m_bInParty, nRoomForceRating,
                AsSWSCreature()->m_stanceId, AsSWSCreature()->m_pStats->m_nCHAMod);
            nVitalityCost = 0;
haveForceCost:
            pData->SetInteger(2, nFinalForceCost);
            CSWSCreatureStats* pStats = AsSWSCreature()->m_pStats;
            pData->SetInteger(3, static_cast<short>(pStats->m_nTemporaryForcePoints + pStats->m_nCurrentForcePoints));
            pData->SetInteger(4, nVitalityCost);
            pData->SetInteger(5, AsSWSCreature()->GetCurrentHitPoints(0));
#endif
            BroadcastSpellData(pSpell, pData);
            delete pData;
        }
        CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp
            ->GetCreatureByGameObjectID(targetId);
        if (pTarget != 0)
            BroadcastSafeProjectile(m_objectId, targetId, m_position, pTarget->m_position,
                m_castLevel, nPathCount, spellId, 0, pathType, 0, 0);
        else
            BroadcastSafeProjectile(m_objectId, targetId, m_position, m_castPosition,
                m_castLevel, nPathCount, spellId, 0, pathType, 0, 0);
        CSWSSpellScriptData* pImpact = new CSWSSpellScriptData;
        pImpact->m_nSpellId = spellId;
        pImpact->m_sScript = pSpell->m_sImpactScript;
        pImpact->m_targetId = targetId;
        pImpact->m_targetPosition = loc;
        pImpact->m_casterId = m_objectId;
        pImpact->m_itemId = itemId;
#if !KOTOR1  // K1's impact event carries no force cost
        pImpact->m_nFinalForceCost = nFinalForceCost;
#endif
        if (GetArea() != 0)
            pImpact->m_areaId = static_cast<CSWSArea*>(GetArea())->m_objectId;
        else
            pImpact->m_areaId = CGameObjectArray::INVALIDOBJID;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(0, m_castLevel,
            m_objectId, m_objectId, CServerAIMaster::EVENT_SPELL_IMPACT, pImpact);
    }
    m_bCastBroadcastProjectile = 1;
}

int CSWSObject::StartDialog(unsigned long long targetId, const CExoString& sResRef, int a3)
{
    CSWSObject* pObject = 0;
    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    g_pAppManager->m_pClientExoApp->SetDialogSent(0);
    g_pAppManager->m_pClientExoApp->SetDialogModelsSent(0);
    if (m_pDialog != 0) {
        if (m_bDialogFinished != 0)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
        return 1;
    }
    m_bDialogPaused = 0;
    m_nDialogDelayDay = 0;
    m_nDialogDelayTod = 0;
    m_nDialogDelayStartDay = 0;
    m_nDialogDelayStartTod = 0;
    m_bDialogSendPending = 0;
    m_bDialogStarted = 1;
    m_nReserved1 = 0;
    m_nReserved2 = 0;
    m_nReserved3 = 0;
    m_nCameraAnimStartMs = 0;

    CResRef rDialog;
    if (sResRef == "")
        rDialog = GetDialogResref();
    else
        rDialog = sResRef;
    if (rDialog == const_cast<char*>(""))
        return 0;
#if !KOTOR1  // the Aspyr dialog hacks are K2-only
    if (s_oondarDone != 0 && rDialog == const_cast<char*>("oondar"))
        return 0;
    AspyrHackCarthDialog = rDialog == const_cast<char*>("carth");
#endif

    CResGFF* pGff = new CResGFF(RESTYPE_DLG, const_cast<char*>("DLG "), rDialog);
    if (pGff->m_nValid != 0) {
        m_pDialog = new CSWSDialog();
        if (m_pDialog->LoadDialog(pGff, 1) != 0) {
            delete pGff;
            m_pDialog->m_bPrivate = a3;
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(1);
            // An unused copy; the string is not read after this.
            {
                CExoString sDialogName;
                rDialog.CopyToString(sDialogName);
            }
            if (g_pAppManager->m_pClientExoApp->m_pInternal->m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
                g_pAppManager->m_pClientExoApp->SetInputClass(0, 1);
            }
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                    targetId, reinterpret_cast<CGameObject**>(&pObject)) != CGameObjectArray::SUCCESS)
                return 0;
            unsigned char nGender = CTlkTable::GENDER_MALE;
            CGameObject* pPlayerCreature =
                g_pAppManager->m_pServerExoApp->GetGameObject(pPlayer->m_gameObjectId);
            if (pPlayerCreature != 0 && pPlayerCreature->AsSWSCreature() != 0)
                nGender = pPlayerCreature->AsSWSCreature()->GetGender();
            CSWSObject* pTarget = pObject;
            if (pPlayer != 0) {
                CSWSDialogPlayer* pNode = new CSWSDialogPlayer;
                pNode->m_nPlayerId = pPlayer->m_nPlayerId;
                pNode->m_objectId = targetId;
                pNode->m_nGender = nGender;
                m_pDialog->m_lstParticipants.AddTail(pNode);
            }
            m_pDialog->m_oidCurrentPlayer = targetId;
            m_pDialog->m_nPlayerGender = nGender;
            pTarget->m_dialogOwnerId = m_objectId;
            if (pTarget->AsSWSCreature() != 0) {
                pTarget->AsSWSCreature()->ActionManager(2);
                pTarget->AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 1, 0);
            }
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_oidDialogOwner = m_objectId;
            m_dialogOwnerId = m_objectId;
            m_dialogWaitFlags = 0;
            if (AsSWSCreature() != 0) {
                AsSWSCreature()->ActionManager(2);
                AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 1, 0);
            }
            m_pDialog->SetDialogOwner(this);
            int bSent = SendDialogEntry(m_pDialog->GetStartEntry(this));
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideContainerGui();
            if (bSent != 0) {
                g_pAppManager->m_pClientExoApp->ClearEnemySighted();
                return bSent;
            }
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog != 0) {
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 0;
                g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
                    1, 0.0f, 1.0f, Vector());
            }
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
            return 0;
        }
        if (m_pDialog != 0)
            delete m_pDialog;
        m_pDialog = 0;
    }
    delete pGff;
    return 0;
}

void CSWSObject::BroadcastDialog(const CExoString& sString, float fRange, int bShout)
{
    int nIndex = -1;
    CSWSObject* pObject = 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArea == 0)
        return;
    pArea->GetFirstObjectIndiceByX(&nIndex, m_position.x - fRange);
    if (nIndex < 0)
        return;
    float fRangeSq = fRange * fRange;
    while (nIndex >= 0 && nIndex < pArea->m_aObjectsByX.m_count) {
        int nCurrent = nIndex++;
        unsigned long long oid = pArea->m_aObjectsByX.m_pArray[nCurrent];
        if (pObjectArray->GetGameObject(oid, reinterpret_cast<CGameObject**>(&pObject))
                != CGameObjectArray::SUCCESS)
            return;
        if (pObject == 0)
            continue;
        if (oid == m_objectId)
            continue;
        Vector vTargetPos = pObject->m_position;
        // The index is X-sorted, so the first candidate past the far edge ends the whole scan.
        if (vTargetPos.x > m_position.x + fRange)
            return;
        float fDx = m_position.x - vTargetPos.x;
        float fDy = m_position.y - vTargetPos.y;
        float fDz = m_position.z - vTargetPos.z;
        float fDistSq = fDx * fDx + fDy * fDy + fDz * fDz;
        if (fDistSq <= fRangeSq && pObject->m_bListening != 0) {
            if (pObject->AsSWSCreature() != 0 && AsSWSCreature() != 0) {
                CSWVisibilityNode* pNode =
                    pObject->AsSWSCreature()->GetVisibleListElement(m_objectId);
                if (pNode == 0)
                    continue;
                if ((pNode->m_flags & 2) == 0)
                    continue;
                if (bShout != 0 && (pNode->m_flags & 1) == 0 && fDistSq >= 100.0f)
                    continue;
            } else if (pObject->AsSWSCreature() != 0 && AsSWSCreature() == 0) {
                float fHearDx = m_position.x - vTargetPos.x;
                float fHearDy = m_position.y - vTargetPos.y;
                float fHearDz = m_position.z - vTargetPos.z;
                float fHearSq = fHearDx * fHearDx + fHearDy * fHearDy + fHearDz * fHearDz;
                float fHearRange = pObject->AsSWSCreature()->GetListenCheckDistance();
                if (fHearSq > fHearRange * fHearRange)
                    continue;
            }
            int nExpressionId = pObject->TestListenExpression(sString);
            if (nExpressionId < 0)
                continue;
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE;
            pEvent->SetInteger(0,
                pObject->GetListenExpressionObj(nExpressionId)->m_aMatchedStrings.m_count);
            pEvent->SetInteger(1, nExpressionId);
            for (int i = 1;
                 i < pObject->GetListenExpressionObj(nExpressionId)->m_aMatchedStrings.m_count; i++) {
                pEvent->SetString(i,
                    *pObject->GetListenExpressionObj(nExpressionId)->m_aMatchedStrings.m_pArray[i]);
            }
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oid, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }
}

unsigned long long CSWSObject::SpawnBodyBag()
{
    if (m_objectType != CGameObject::OBJECT_TYPE_CREATURE
        && m_objectType != CGameObject::OBJECT_TYPE_PLACEABLE)
        return OBJECT_INVALID;

    CSWSCreature* pCreature = AsSWSCreature();
    CSWSPlaceable* pPlaceable = AsSWSPlaceable();
    if (pCreature == 0 && pPlaceable == 0)
        return OBJECT_INVALID;

    g_pAppManager->m_pServerExoApp->GetSWSMessage()
        ->SendServerToAllPlayersCreatureUpdate_StripEquippedItems(m_objectId);

    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return OBJECT_INVALID;

    int nAppearance K2_UNSET;
    int bIsCorpse = 0;
    if (pCreature != 0) {
        nAppearance = pCreature->GetBodyBagAppearance();
        int nCorpse;
        int nBodyBagRow = pCreature->m_bBodyBag;
        if (nBodyBagRow == 0) {
            C2DA* pAppearanceTable = g_pRules->m_p2DArrays->m_pAppearance;
            if (pAppearanceTable->GetINTEntry(
                    pCreature->m_appearance.m_headId, CExoString("BODY_BAG"), &nBodyBagRow) == 0) {
                nBodyBagRow = 0;
            }
        }
        C2DA* pBodyBagTable = g_pRules->m_p2DArrays->m_pBodyBag;
        int bFound = pBodyBagTable->GetINTEntry(nBodyBagRow, CExoString("Corpse"), &nCorpse);
        if (bFound != 0 && nCorpse != 0)
            bIsCorpse = 1;
        else if (pCreature->GetItemCount(1, 0, 0) == 0 && pCreature->GetGold() == 0)
            return OBJECT_INVALID;
    } else if (pPlaceable != 0) {
        if (pPlaceable->GetItemCount(0) == 0)
            return OBJECT_INVALID;
        nAppearance = pPlaceable->GetBodyBagAppearance();
    }

    CSWSPlaceable* pBodyBag = new CSWSPlaceable(OBJECT_INVALID);
#if KOTOR1
    if (pBodyBag->LoadBodyBag(static_cast<unsigned char>(nAppearance)) == 0) {
#else
    if (pBodyBag->LoadBodyBag(nAppearance) == 0) {
#endif
        delete pBodyBag;
        return OBJECT_INVALID;
    }

    pBodyBag->AcquireItemsFromObject(m_objectId, 1);
    pBodyBag->m_bIsBodyBag = 1;
    if (pCreature != 0) {
        pBodyBag->m_oidCorpse = m_objectId;
        pBodyBag->m_bIsBodyBagVisible = 0;
    }

    struct BodyBagEventData {
        unsigned long long m_bodyBagId;
        Vector m_vPosition;
    };
    BodyBagEventData* pEventData = new BodyBagEventData;
    pEventData->m_bodyBagId = pBodyBag->m_objectId;
    pEventData->m_vPosition = m_position;

    CExoLocString sName;
    int nNameStrRef;
    if (pCreature != 0) {
        if (g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(
                pCreature->m_bBodyBag, CExoString("Name"), &nNameStrRef) != 0) {
            sName.m_nStrRef = static_cast<unsigned long long>(static_cast<long long>(nNameStrRef));
            pBodyBag->m_firstName = sName;
        }
    } else if (pPlaceable != 0) {
        if (g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(
                pPlaceable->m_bodyBagAppearanceRow, CExoString("Name"), &nNameStrRef) != 0) {
            sName.m_nStrRef = static_cast<unsigned long long>(static_cast<long long>(nNameStrRef));
            pBodyBag->m_firstName = sName;
        }
    }
    pBodyBag->m_corpse.m_bIsCorpse = bIsCorpse;
    pBodyBag->m_bDieWhenEmpty = !bIsCorpse;
    if (bIsCorpse != 0 && pBodyBag->m_pItemRepository->ItemListGetCount() == 0)
        pBodyBag->m_bHasInventory = 0;
    pBodyBag->m_bPlot = !bIsCorpse;

    pBodyBag->SetOrientation(
        YawPitchRoll(Yaw(m_orientation), Pitch(m_orientation), 0.0f));

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(0, 500, m_objectId,
        static_cast<CSWSArea*>(GetArea())->m_objectId,
        CServerAIMaster::EVENT_SPAWN_BODY_BAG, pEventData);

    pBodyBag->m_bPartyInteract = 1;
    return pBodyBag->m_objectId;
}

unsigned long long CSWSObject::GetNearestObjectByName(const CExoString& sName, float fRange)
{
    int bFound = fRange != 0.0f;
    unsigned long long oidNearest = OBJECT_INVALID;
    CSWArea* pArea = GetArea();
    if (pArea == 0)
        return oidNearest;
    if (sName == "")
        return oidNearest;
    Vector vSelf = m_position;
    unsigned long long oid;
    if (static_cast<CSWSArea*>(pArea)->GetFirstObjectInArea(oid) != 1)
        return oidNearest;
    float fNearest = fRange * fRange;
    do {
        CGameObject* pGameObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oid, &pGameObject)
            == CGameObjectArray::SUCCESS) {
            CSWSObject* pObject = static_cast<CSWSObject*>(pGameObject);
            Vector vPos = pObject->m_position;
            if (pObject->GetArea() == pArea && pObject->m_sTag.CompareNoCase(sName) != 0) {
                Vector vDelta = vPos - vSelf;
                float fDistance =
                    vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
#if KOTOR1  // K1: no ForceAlwaysUpdate override
                if (bFound == 0 || fNearest >= fDistance) {
#else
                if (bFound == 0 || fNearest >= fDistance || pObject->m_bForceAlwaysUpdate != 0) {
#endif
                    oidNearest = pObject->m_objectId;
                    fNearest = __builtin_sqrtf(fDistance);
                    bFound = 1;
                }
            }
        }
    } while (static_cast<CSWSArea*>(pArea)->GetNextObjectInArea(oid) == 1);
    return oidNearest;
}

void CSWSObject::SetLastHostileActor(unsigned long long oidActor, int bForce)
{
    if (bForce == 1) {
        m_lastHostileActorId = oidActor;
        return;
    }
    if (m_bPlot != 1) {
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidActor);
        if (pObject == 0)
            return;
        CSWSCreature* pCreature = 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT) {
            unsigned long long oidCreator = pObject->AsSWSAreaOfEffectObject()->m_oidCreator;
            pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreator);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE
                   && pObject->m_objectId != m_objectId) {
            pCreature = pObject->AsSWSCreature();
        }
#if KOTOR1  // K1's dying test is GetDeadTemp()
        if (pCreature == 0 || pCreature->GetDead() == 1 || pCreature->GetDeadTemp() == 1) {
#else
        if (pCreature == 0 || pCreature->GetDead() == 1
            || (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) <= 0)) {
#endif
            m_lastHostileActorId = OBJECT_INVALID;
            return;
        }
        if (AsSWSCreature() != 0 && GetArea() != 0
            && pCreature->m_pStats->m_factionId == AsSWSCreature()->m_pStats->m_factionId)
            return;
        m_lastHostileActorId = pCreature->m_objectId;
        return;
    }
    m_lastHostileActorId = OBJECT_INVALID;
}

void CSWSObject::CopyScriptVars(CSWSScriptVarTable* pTable)
{
    CExoString sName;
    CExoString sValue;
    CScriptLocation loc = {};
    for (int i = 0; i < pTable->m_vars.m_count; i++) {
        sName = pTable->m_vars.m_pArray[i].m_sName;
        switch (pTable->m_vars.m_pArray[i].m_nType) {
        case 1:  // VARTYPE_INTEGER
            m_scriptVars.SetInt(sName, pTable->GetInt(sName), 1);
            break;
        case 2:  // VARTYPE_FLOAT
            m_scriptVars.SetFloat(sName, pTable->GetFloat(sName));
            break;
        case 3:  // VARTYPE_STRING
            sValue = pTable->GetString(sName);
            m_scriptVars.SetString(sName, sValue);
            break;
        case 4:  // VARTYPE_OBJECT
            m_scriptVars.SetObject(sName, pTable->GetObjectA(sName));
            break;
        case 5:  // VARTYPE_LOCATION
            loc = pTable->GetLocation(sName);
            m_scriptVars.SetLocation(sName, loc);
            break;
        }
    }
}

void CSWSObject::ReportOverflow(int nStrRef, int nAmount, int nLimit, int nUnused)
{
    K2_UNUSED(nUnused);
    if (g_pAppManager->m_pServerExoApp->GetDebugMode() != 1)
        return;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (pMessage == 0)
        return;
    if (ppPlayers == 0)
        return;
    CExoLinkedListNode* pNodePos = ppPlayers->GetHeadPos();
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = reinterpret_cast<CSWSPlayer*>(ppPlayers->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0) {
                CSWCCMessageData* pData = new CSWCCMessageData;
                pData->SetInteger(0, nAmount);
                pData->SetInteger(1, nLimit);
                pData->SetObjectID(0, OBJECT_INVALID);
                pData->SetObjectID(1, m_objectId);
                pData->SetString(1, m_sTag);
                if (nStrRef == FEEDBACK_AI_UPDATE_TIME_OVERFLOW) {
                    CServerAIMaster* pAIMaster =
                        g_pAppManager->m_pServerExoApp->GetServerAIMaster();
                    pData->SetString(2, pAIMaster->m_sScriptsRun);
                    pData->SetInteger(2, pAIMaster->m_nLastActionId);
                    pData->SetInteger(3, pAIMaster->m_bRanInterAreaPath + pAIMaster->m_bRanGridPath * 2
                                             + pAIMaster->m_bRanVisibilityUpdate * 4
                                             + pAIMaster->m_bReserved1 * 8
                                             + pAIMaster->m_fChallengeRating * 16);
                }
                pCreature->SendFeedbackMessage(static_cast<unsigned short>(nStrRef), pData);
            }
        }
        ppPlayers->GetNext(pNodePos);
    }
}

void CSWSObject::CalculateLastSpellProjectileTime(unsigned char nProjectileType)
{
    Vector vDelta = {};
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(m_castSpellId);
    if (pSpell == 0)
        return;
    if (pSpell->m_bHasProjectile == 0) {
        m_castLevel = 0;
        return;
    }
    if (m_castTargetId == CGameObjectArray::INVALIDOBJID) {
        vDelta = m_castPosition - m_position;
    } else {
        CSWSObject* pTarget;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_castTargetId,
                                                                           reinterpret_cast<CGameObject**>(&pTarget))
                == CGameObjectArray::SUCCESS)
            vDelta = pTarget->m_position - m_position;
    }
    float fTime = logf(magnitude(vDelta)) * 3.0f + 2.0f;
    if (nProjectileType == 0)
        nProjectileType = static_cast<unsigned char>(pSpell->m_nProjectileType);
    switch (nProjectileType) {
    case CSWCProjectile::PROJECTILE_TYPE_HOMING:
        fTime *= 2.0f;
        break;
    case CSWCProjectile::PROJECTILE_TYPE_HIGH_BALLISTIC:
        m_castLevel = 2000;
        return;
    case CSWCProjectile::PROJECTILE_TYPE_ACCELERATING:
        fTime *= 1.5f;
        break;
    case CSWCProjectile::PROJECTILE_TYPE_LINKED:
        fTime = magnitude(vDelta) * 0.5f;
        break;
    case CSWCProjectile::PROJECTILE_TYPE_BOUNCE:
        fTime *= 0.4f;
        break;
    default:
        break;
    }
    if (fTime <= 0.0f) {
        m_castLevel = 1;
        return;
    }
    m_castLevel = static_cast<unsigned int>((magnitude(vDelta) / fTime) * 1000.0f);
    if (pSpell->m_nProjectileType == CSWCProjectile::PROJECTILE_TYPE_SPIRAL)
        m_castLevel += 2500;
}

Vector CSWSObject::CalculateSpellRangedMissTarget(unsigned long long casterId,
                                                  unsigned long long targetId)
{
    float fMissDistance = 0.5f;
    Vector vMissTarget;

    CGameObject* pCasterObject = g_pAppManager->m_pServerExoApp->GetGameObject(casterId);
    if (pCasterObject == 0)
        return Vector();
    if (pCasterObject->AsSWSObject() == 0)
        return Vector();
    CSWSObject* pCaster = pCasterObject->AsSWSObject();

    CGameObject* pTargetObject = g_pAppManager->m_pServerExoApp->GetGameObject(targetId);
    if (pTargetObject == 0)
        return Vector();
    if (pTargetObject->AsSWSObject() == 0)
        return Vector();
    CSWSObject* pTarget = pTargetObject->AsSWSObject();

    if (pTarget->AsSWSCreature() != 0)
    {
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            pTarget->AsSWSCreature()->m_pStats->m_nAppearanceType,
            CExoString("creperspace"), &fMissDistance);
        fMissDistance -= 0.1f;
    }

    float fScale = (rand_wincompatible() % 2000 + 1) / 1000.0f;

    float fDx = pCaster->m_position.x - pTarget->m_position.x;
    float fDy = pCaster->m_position.y - pTarget->m_position.y;
    float fDz = pCaster->m_position.z - pTarget->m_position.z;

    if (fDx * fDx + fDy * fDy + fDz * fDz > 100.0f)
    {
        Vector vDelta;
        vDelta.x = fDx;
        vDelta.y = fDy;
        vDelta.z = fDz;

        Vector vDirection = normalize(vDelta);
        float fBearing = atanf(vDirection.y / vDirection.x);

        int nJitterSign = rand_wincompatible();
        float fJitter = (rand_wincompatible() % 2094) / 1000.0f;
        if (nJitterSign & 1)
            fBearing -= fJitter;
        else
            fBearing += fJitter;
        if (fBearing > 6.2831855f)
            fBearing -= 6.2831855f;
        if (fBearing < 0.0f)
            fBearing += 6.2831855f;

        vMissTarget.x = cosf(fBearing);
        vMissTarget.y = sinf(fBearing);
        if (vDirection.x < 0.0f)
        {
            vMissTarget.x = -vMissTarget.x;
            vMissTarget.y = -vMissTarget.y;
            vMissTarget.z = -vMissTarget.z;
        }

        vMissTarget.x *= fScale;
        vMissTarget.y *= fScale;
        vMissTarget.z *= fScale;
        vMissTarget.x += pTarget->m_position.x;
        vMissTarget.y += pTarget->m_position.y;
        vMissTarget.z += pTarget->m_position.z;
        vMissTarget.z = pTarget->m_position.z + 1.0f;
    }
    else
    {
        vMissTarget.x = pTarget->m_position.x - pCaster->m_position.x;
        vMissTarget.y = pTarget->m_position.y - pCaster->m_position.y;
        vMissTarget.z = pTarget->m_position.z - pCaster->m_position.z;

        Vector vPerpendicular;
        int nSide = rand_wincompatible();
        if (nSide & 1)
        {
            vPerpendicular.x = normalize(vMissTarget).y;
            vPerpendicular.y = -normalize(vMissTarget).x;
        }
        else
        {
            vPerpendicular.x = -normalize(vMissTarget).y;
            vPerpendicular.y = normalize(vMissTarget).x;
        }
        vPerpendicular.z = 0.0f;

        Vector vOffset;
        vOffset.x = vPerpendicular.x + vPerpendicular.x * fMissDistance;
        vOffset.y = vPerpendicular.y + vPerpendicular.y * fMissDistance;
        vOffset.z = vPerpendicular.z + vPerpendicular.z * fMissDistance;

        float fEx = pTarget->m_position.x - pCaster->m_position.x;
        float fEy = pTarget->m_position.y - pCaster->m_position.y;
        float fEz = pTarget->m_position.z - pCaster->m_position.z;
        if (fEx * fEx + fEy * fEy + fEz * fEz > 25.0f)
        {
            float fGrow = (rand_wincompatible() % static_cast<int>(fScale * 1000.0f)) / 1000.0f;
            vOffset.x += vOffset.x * fGrow;
            vOffset.y += vOffset.y * fGrow;
            vOffset.z += vOffset.z * fGrow;
        }

        vMissTarget.x += vOffset.x;
        vMissTarget.y += vOffset.y;
        vMissTarget.z += vOffset.z;

        Vector vDirection = normalize(vMissTarget);
        float fRange = (rand_wincompatible() % 10) + 10.0f;
        vMissTarget.x = vDirection.x * fRange;
        vMissTarget.y = vDirection.y * fRange;
        vMissTarget.z = vDirection.z * fRange;
        vMissTarget.z = pTarget->m_position.z + 1.0f;
        vMissTarget.x += pTarget->m_position.x;
        vMissTarget.y += pTarget->m_position.y;
        vMissTarget.z += pTarget->m_position.z;
    }

    return Vector();
}

void CSWSObject::UpdateEffectList(unsigned int nCalendarDay, unsigned int nTimeOfDay)
{
    // One (day, time) pair, shared by the regenerate arm and the expiry test below.
    unsigned int nOutDay = 0;
    unsigned int nOutTime = 0;
    CWorldTimer* pTimer = g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId);

    int i = 0;
    while (static_cast<unsigned int>(i) < static_cast<unsigned int>(m_effects.m_count))
    {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        int bRestart = 0;

        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_REGENERATE)
        {
            int nLastDay = pEffect->GetInteger(2);
            int nLastTime = pEffect->GetInteger(3);
            pTimer->SubtractWorldTimes(nCalendarDay, nTimeOfDay, nLastDay, nLastTime,
                                       &nOutDay, &nOutTime);

            int bBelowCap;
            if (AsSWSCreature() != 0)
            {
                bBelowCap = 0;
                if (AsSWSCreature()->GetDead() == 0)
                {
                    CSWSCreature* pCreature = AsSWSCreature();
#if KOTOR1  // K1's dying test is GetDeadTemp()
                    if (pCreature->GetDeadTemp() == 0)
#else
                    if (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) > 0)
#endif
                    {
                        if (pEffect->GetInteger(4) == ITEM_PROPERTY_REGENERATION_FORCE_POINTS)
                            bBelowCap = (static_cast<short>(AsSWSCreature()->m_pStats->m_nCurrentForcePoints) <
                                         AsSWSCreature()->GetMaxForcePoints());
                        else
                            bBelowCap = (AsSWSCreature()->GetCurrentHitPoints(1) <
                                         AsSWSCreature()->GetMaxHitPoints(1));
                    }
                }
            }
            else
            {
                bBelowCap = (GetCurrentHitPoints(1) < GetMaxHitPoints(1) && GetDead() == 0);
            }

            if (bBelowCap && nOutTime > static_cast<unsigned int>(pEffect->GetInteger(1)))
            {
                CGameEffect* pHeal = new CGameEffect(1);
                pHeal->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HEAL);
                pHeal->SetInteger(0, pEffect->GetInteger(0));
                pHeal->SetInteger(1, pEffect->GetInteger(4));
#if !KOTOR1  // K1's regeneration heal sets no integer 2
                pHeal->SetInteger(2, 1);
#endif
                pHeal->m_durationTypeBits = (pHeal->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                            CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                pHeal->SetCreator(pEffect->m_creatorId);
                ApplyEffect(pHeal, 0, 0);
                pEffect->SetInteger(2, nCalendarDay);
                pEffect->SetInteger(3, nTimeOfDay);
                bRestart = 1;
            }
        }
        else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_POISON)
        {
            int nStartDay = pEffect->GetInteger(1);
            int nStartTime = pEffect->GetInteger(2);
            int nDurationSeconds = pEffect->GetInteger(5);
            unsigned int nLifeDay;
            unsigned int nLifeTime;
            pTimer->SubtractWorldTimes(nCalendarDay, nTimeOfDay, nStartDay, nStartTime,
                                       &nLifeDay, &nLifeTime);
            if (nLifeTime > static_cast<unsigned int>(nDurationSeconds * 1000))
            {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            }
            else
            {
                int nTickDay = pEffect->GetInteger(3);
                int nTickTime = pEffect->GetInteger(4);
                int nTickSeconds = pEffect->GetInteger(6);
                unsigned int nSinceDay;
                unsigned int nSinceTime;
                pTimer->SubtractWorldTimes(nCalendarDay, nTimeOfDay, nTickDay, nTickTime,
                                           &nSinceDay, &nSinceTime);
                if (nSinceTime > static_cast<unsigned int>(nTickSeconds * 1000))
                {
                    // While the in-game GUI reports it is busy the tick is banked: the elapsed
                    // seconds are added to parameter 5 instead of being applied.
                    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
                    {
                        pEffect->SetInteger(5, nDurationSeconds + nTickSeconds);
                    }
                    else
                    {
                        AsSWSCreature()->ApplyPoisonDamage(pEffect->GetInteger(0), pEffect,
                                                           pEffect->GetInteger(7),
                                                           pEffect->GetFloat(0));
                        pEffect->SetFloat(0, pEffect->GetFloat(0) + 1.0f);
                    }
                    pEffect->SetInteger(3, nCalendarDay);
                    pEffect->SetInteger(4, nTimeOfDay);
                }
            }
        }
        else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DISEASE)
        {
            int nNextDay = pEffect->GetInteger(3);
            int nNextTime = pEffect->GetInteger(4);
            if (pTimer->CompareWorldTimes(nCalendarDay, nTimeOfDay, nNextDay, nNextTime) >= 0 &&
                AsSWSCreature() != 0)
            {
                AsSWSCreature()->ApplyDiseasePayload(pEffect, nCalendarDay, nTimeOfDay);
                bRestart = 1;
            }
        }

        if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_TEMPORARY)
        {
            pEffect->GetExpiryTime(&nOutDay, &nOutTime);
            if (pTimer->CompareWorldTimes(nCalendarDay, nTimeOfDay, nOutDay, nOutTime) == 1)
            {
                RemoveEffectById(pEffect->m_effectId);
                bRestart = 1;
            }
        }

        if (bRestart)
            i = 0;
        else
            i++;
    }
}
