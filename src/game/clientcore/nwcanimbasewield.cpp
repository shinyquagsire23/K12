#include "nwcanimbasewield.h"
#include "AURORA/aurmodel.h"
#include "nwcitem.h"
#include "exosound.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwitem.h"
#include <string.h>
#include "port/libcbase.h"
#include "itemtype.h"
#include "soundprioritygroup.h"
#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBaseWield::CSWCAnimBaseWield()
{
    m_oidWieldedLeft = CGameObjectArray::INVALIDOBJID;
    m_oidWieldedRight = CGameObjectArray::INVALIDOBJID;
    m_nAnimBaseType = ANIM_BASE_TYPE_WIELD;
    m_bWieldLimited = 0;
    m_resDummyRight = const_cast<char*>("");
    m_resDummyLeft = const_cast<char*>("");
    m_pDummyRight = 0;
    m_pDummyLeft = 0;
    m_bWieldedItemsHidden = 0;
    m_bHeadItemHidden = 0;
    m_bTorchLit = 0;
    m_pTorchSound = 0;
}

CSWCAnimBaseWield::~CSWCAnimBaseWield()
{
    if (m_pTorchSound != 0) {
        delete m_pTorchSound;
        m_pTorchSound = 0;
    }
    if (m_pDummyRight != 0) {
        m_pDummyRight->AttachToObject(0, 0, 0);
        m_pDummyRight->AttachToScene(0);
        delete m_pDummyRight;
        m_pDummyRight = 0;
    }
    if (m_pDummyLeft != 0) {
        m_pDummyLeft->AttachToObject(0, 0, 0);
        m_pDummyLeft->AttachToScene(0);
        delete m_pDummyLeft;
        m_pDummyLeft = 0;
    }
    UnloadModel();
}

void CSWCAnimBaseWield::SetWieldLimited(int limited)
{
    m_bWieldLimited = limited;
}

void CSWCAnimBaseWield::TurnOnShadows()
{
    CSWCAnimBase::TurnOnShadows();
}

void CSWCAnimBaseWield::TurnOffShadows()
{
    CSWCAnimBase::TurnOffShadows();
}

void CSWCAnimBaseWield::HideHeadItem() {}

void CSWCAnimBaseWield::ShowHeadItem() {}

Gob* CSWCAnimBaseWield::GetItemAurObject(unsigned long long oid, unsigned char which)
{
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oid);
    if (pItem == 0)
        return 0;
    return pItem->GetModel(which, 1);
}

int CSWCAnimBaseWield::ItemIsValid(unsigned long long oid)
{
    if (!m_bWieldLimited)
        return 1;
    return g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oid) != 0;
}

Gob* CSWCAnimBaseWield::GetModel(unsigned char which)
{
    // The first four selectors address the second wielded item, the next four the first.
    switch (which)
    {
    case ANIM_MODEL_RIGHT_ITEM_WINGS:
        return GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_WINGS);
    case ANIM_MODEL_RIGHT_ITEM_TAIL:
        return GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_TAIL);
    case ANIM_MODEL_RIGHT_ITEM_HEAD:
        return GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_HEAD);
    case ANIM_MODEL_RIGHT_ITEM_BODY:
        return GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_BODY);
    case ANIM_MODEL_LEFT_ITEM_WINGS:
        return GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_WINGS);
    case ANIM_MODEL_LEFT_ITEM_TAIL:
        return GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_TAIL);
    case ANIM_MODEL_LEFT_ITEM_HEAD:
        return GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_HEAD);
    case ANIM_MODEL_LEFT_ITEM_BODY:
        return GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_BODY);
    }
    return CSWCAnimBaseTW::GetModel(which);
}

int CSWCAnimBaseWield::LoadModel(CResRef const& cResRef, unsigned char which)
{
    if (!CSWCAnimBaseTW::LoadModel(cResRef, which))
        return 0;
    ReAttachWieldedObjects();
    return 1;
}

void CSWCAnimBaseWield::ReAttachWieldedObjects()
{
    Gob* pObject = GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_BODY);
    if (pObject != 0)
        pObject->AttachToObject(m_pModel, static_cast<char*>(m_sWieldedLeftNode.CStr()), 0);
    Gob* pObject2 = GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_BODY);
    if (pObject2 != 0)
        pObject2->AttachToObject(m_pModel, static_cast<char*>(m_sWieldedRightNode.CStr()), 0);
    CSWCAnimBase::RefreshHilite();
}

int CSWCAnimBaseWield::Wield(unsigned long long oid, CExoString const& sName, unsigned char which,
                             float fSpeed)
{
    // Each arm first detaches the outgoing model, then plays its equip or unequip sound.
    Gob* pModel = GetItemAurObject(oid, ANIM_MODEL_BODY);
    if (which == 0)
    {
        Gob* pOldModel3 = GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_BODY);
        if (m_oidWieldedLeft != oid && pOldModel3 != 0)
        {
            CSWCItem* pCheck3 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
            if (pCheck3 != 0 && pCheck3->GetBaseItem()->m_nItemType == ITEM_TYPE_LIGHT_SOURCE)
            {
                pOldModel3->PlayAnimation(const_cast<char*>("unequip"), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimNoBlendIn, 0.0f);
                m_bTorchLit = 0;
            }
            CSWCItem* pCheck4 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
            if (pCheck4 != 0 && pCheck4->GetEquippedBy() == 0)
            {
                pOldModel3->AttachToObject(0, 0, 0);
                pOldModel3->AttachToScene(0);
                pOldModel3->SetObjectScale(1.0f, 1);
            }
        }
        if (pModel == 0)
        {
            if (m_oidWieldedLeft != m_oidWieldedRight && pOldModel3 != 0)
            {
                CSWCItem* pCheck5 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
                if (pCheck5 != 0 && pCheck5->GetEquippedBy() == 0)
                {
                    pOldModel3->AttachToObject(0, 0, 0);
                    pOldModel3->AttachToScene(0);
                    pOldModel3->SetObjectScale(1.0f, 1);
                }
            }
            m_sWieldedLeftNode = const_cast<char*>("");
        }
        else if (ItemIsValid(oid))
        {
            pModel->AttachToObject(m_pModel, sName.CStr(), 0);
            pModel->SetObjectScale(fSpeed, 1);
            m_sWieldedLeftNode = sName;
            CSWCItem* pCheck6 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oid);
            if (pCheck6 != 0 && pCheck6->GetBaseItem()->m_nItemType == ITEM_TYPE_LIGHT_SOURCE)
            {
                pModel->PlayAnimation(const_cast<char*>("equip"), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimNoBlendIn, 0.0f);
                m_bTorchLit = 1;
            }
        }

        m_oidWieldedLeft = oid;
        RefreshHilite();
        return 1;
    }
    if (which == 1)
    {
        Gob* pOldModel = GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_BODY);
        if (m_oidWieldedRight != oid && pOldModel != 0)
        {
            CSWCItem* pCheck = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedRight);
            if (pCheck != 0 && pCheck->GetEquippedBy() == 0)
            {
                pOldModel->AttachToObject(0, 0, 0);
                pOldModel->AttachToScene(0);
                pOldModel->SetObjectScale(1.0f, 1);
            }
        }
        if (pModel == 0)
        {
            if (m_oidWieldedRight != m_oidWieldedLeft && pOldModel != 0)
            {
                CSWCItem* pCheck2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedRight);
                if (pCheck2 != 0 && pCheck2->GetEquippedBy() == 0)
                {
                    pOldModel->AttachToObject(0, 0, 0);
                    pOldModel->AttachToScene(0);
                    pOldModel->SetObjectScale(1.0f, 1);
                }
            }
            m_sWieldedRightNode = const_cast<char*>("");
        }
        else if (ItemIsValid(oid))
        {
            pModel->AttachToObject(m_pModel, sName.CStr(), 0);
            pModel->SetObjectScale(fSpeed, 1);
            m_sWieldedRightNode = sName;
        }
        m_oidWieldedRight = oid;
        RefreshHilite();
        return 1;
    }
    return 0;
}

int CSWCAnimBaseWield::SetAnimationOnSubObjectInternal(CAurObject* pObject, unsigned char nSubObject,
                                                       unsigned short nAnimation, float fSpeed,
                                                       int nFlags, float fStart)
{
    if (nSubObject != ANIM_MODEL_BODY)
        return 0;
    return SetAnimationInternal(pObject, nAnimation, fSpeed, nFlags, fStart);
}

void CSWCAnimBaseWield::HideWieldedItems(CResRef const& cResRef1, CResRef const& cResRef2)
{
    // Drops both wielded models and rebuilds one dummy sub-object per hand; an empty resref
    // just clears the cached dummy.
    CExoString sName;
    CSWCItem* pItem2;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
    if (pItem != 0)
    {
        Gob* pObject = pItem->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject != 0)
        {
            pObject->AttachToObject(0, 0, 0);
            pObject->AttachToScene(0);
            if (m_pTorchSound != 0)
                m_pTorchSound->Stop();
        }
    }
    pItem2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedRight);
    if (pItem2 != 0)
    {
        Gob* pObject2 = pItem2->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject2 != 0)
        {
            pObject2->AttachToObject(0, 0, 0);
            pObject2->AttachToScene(0);
        }
    }
    if (m_resDummyRight != cResRef1)
    {
        if (m_pDummyRight != 0)
        {
            m_pDummyRight->AttachToObject(0, 0, 0);
            m_pDummyRight->AttachToScene(0);
            delete m_pDummyRight;
            m_pDummyRight = 0;
        }
        if (cResRef1.GetResRef()[0] != 0)
        {
            cResRef1.CopyToString(sName);
            m_pDummyRight = NewCAurObject(sName.CStr(), const_cast<char*>("dummyright"), 0, 0);
            if (m_pDummyRight != 0)
                m_pDummyRight->AttachToObject(m_pModel, const_cast<char*>("rhand"), 0);
        }
        m_resDummyRight = cResRef1;
    }
    if (m_resDummyLeft != cResRef2)
    {
        if (m_pDummyLeft != 0)
        {
            m_pDummyLeft->AttachToObject(0, 0, 0);
            m_pDummyLeft->AttachToScene(0);
            delete m_pDummyLeft;
            m_pDummyLeft = 0;
        }
        if (cResRef2.GetResRef()[0] != 0)
        {
            cResRef2.CopyToString(sName);
            m_pDummyLeft = NewCAurObject(sName.CStr(), const_cast<char*>("dummyleft"), 0, 0);
            if (m_pDummyLeft != 0)
                m_pDummyLeft->AttachToObject(m_pModel, const_cast<char*>("lhand"), 0);
        }
        m_resDummyLeft = cResRef2;
    }
    m_bWieldedItemsHidden = 1;
}

int CSWCAnimBaseWield::SetAnimationInternal(CAurObject* pObject, unsigned short nAnimation, float fSpeed,
                                            int nFlags, float fStart)
{
    Gob* pChild;
    Gob* pChild2;
    Gob* pChild3;
    Gob* pChild4;
    Gob* pChild5;
    Gob* pChild6;
    if (fSpeed == 0.0f)
        fSpeed = 1.0f;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
    CSWCItem* pItem2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedRight);
    if (pItem != 0 || pItem2 != 0)
    {
        CExoString sName = GetAnimationName(nAnimation);
        if (pItem != 0)
        {
            if (pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_LIGHT_SOURCE)
            {
                pChild = pItem->GetModel(ANIM_MODEL_BODY, 1);
                if (pChild != 0)
                    pChild->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
                pChild2 = pItem->GetModel(ANIM_MODEL_HEAD, 1);
                if (pChild2 != 0)
                    pChild2->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
                pChild3 = pItem->GetModel(ANIM_MODEL_TAIL, 1);
                if (pChild3 != 0)
                    pChild3->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
            }
        }
        if (pItem2 != 0)
        {
            pChild4 = pItem2->GetModel(ANIM_MODEL_BODY, 1);
            if (pChild4 != 0)
                pChild4->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
            pChild5 = pItem2->GetModel(ANIM_MODEL_HEAD, 1);
            if (pChild5 != 0)
                pChild5->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
            pChild6 = pItem2->GetModel(ANIM_MODEL_TAIL, 1);
            if (pChild6 != 0)
                pChild6->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
        }
    }
    return CSWCAnimBaseTW::SetAnimationInternal(pObject, nAnimation, fSpeed, nFlags, fStart);
}

void CSWCAnimBaseWield::EnableDistortion()
{
    Gob* pObject = GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_BODY);
    if (pObject != 0)
        pObject->EnableDistortion();
    Gob* pObject2 = GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_BODY);
    if (pObject2 != 0)
        pObject2->EnableDistortion();
    CSWCAnimBaseTW::EnableDistortion();
}

void CSWCAnimBaseWield::DisableDistortion()
{
    Gob* pObject = GetItemAurObject(m_oidWieldedRight, ANIM_MODEL_BODY);
    if (pObject != 0)
        pObject->DisableDistortion();
    Gob* pObject2 = GetItemAurObject(m_oidWieldedLeft, ANIM_MODEL_BODY);
    if (pObject2 != 0)
        pObject2->DisableDistortion();
    CSWCAnimBaseTW::DisableDistortion();
}

void CSWCAnimBaseWield::ShowWieldedItems()
{
    if (m_bWieldedItemsHidden == 0)
        return;
    if (m_pDummyRight != 0)
    {
        m_pDummyRight->AttachToObject(0, 0, 0);
        m_pDummyRight->AttachToScene(0);
        delete m_pDummyRight;
        m_pDummyRight = 0;
    }
    if (m_pDummyLeft != 0)
    {
        m_pDummyLeft->AttachToObject(0, 0, 0);
        m_pDummyLeft->AttachToScene(0);
        delete m_pDummyLeft;
        m_pDummyLeft = 0;
    }
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedLeft);
    if (pItem != 0)
    {
        Gob* pObject = pItem->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject != 0)
            pObject->AttachToObject(m_pModel, static_cast<char*>(m_sWieldedLeftNode.CStr()), 0);
        if (m_pTorchSound != 0)
            m_pTorchSound->Play();
    }
    CSWCItem* pItem2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidWieldedRight);
    if (pItem2 != 0)
    {
        Gob* pObject2 = pItem2->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject2 != 0)
            pObject2->AttachToObject(m_pModel, static_cast<char*>(m_sWieldedRightNode.CStr()), 0);
    }
    CSWCAnimBase::RefreshHilite();
    m_resDummyRight = const_cast<char*>("");
    m_resDummyLeft = const_cast<char*>("");
    m_bWieldedItemsHidden = 0;
}

void CSWCAnimBaseWield::UpdateSound(int bPlaying)
{
    if (bPlaying == 0 || m_bTorchLit == 0)
    {
        if (m_pTorchSound != 0)
        {
            delete m_pTorchSound;
            m_pTorchSound = 0;
        }
        return;
    }
    if (m_pTorchSound == 0)
    {
        m_pTorchSound = new CExoSoundSource(CResRef("al_cv_firetorch1"));
        m_pTorchSound->Set3D(1);
        m_pTorchSound->SetPriorityGroup(PRIORITY_GROUP_LOOPING_PLAYER);
        m_pTorchSound->SetVolume(10);
        m_pTorchSound->SetLooping(1);
        m_pTorchSound->Play();
    }
    if (m_pTorchSound == 0)
        return;
    unsigned long long nOid = m_oidWieldedLeft;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(nOid);
    if (pItem != 0 && pItem->GetModel(ANIM_MODEL_BODY, 1) != 0)
        m_pTorchSound->SetPosition(pItem->GetModel(ANIM_MODEL_BODY, 1)->GetPosition(), 0.0f);
    else if (m_pModel != 0)
        m_pTorchSound->SetPosition(m_pModel->GetPosition(), 0.0f);
    else
        return;
}

