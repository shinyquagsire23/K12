#include "nwcanimbaseheadwield.h"
#include "AURORA/aurmodel.h"
#include "swgui.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwcitem.h"

// The head+wield animation base. Each forwarder names the diamond parent whose slice owns the model
// or item it acts on.
CSWCAnimBaseHeadWield::CSWCAnimBaseHeadWield()
{
    // Kind byte: 3 = head+wield.
    m_nAnimBaseType = ANIM_BASE_TYPE_HEAD_WIELD;
}

CSWCAnimBaseHeadWield::~CSWCAnimBaseHeadWield()
{
    UnloadModel();
}

// The class owns both model objects, the shared CSWCAnimBase::m_pModel and the head part's own
// CSWCAnimBaseHead::m_pHeadModel, and drives each through the parent whose slice owns it.
int CSWCAnimBaseHeadWield::ReplaceTexture(unsigned char which, CResRef const& resRefA,
                                          CResRef const& resRefB, int nFlags)
{
    if (which == ANIM_MODEL_BODY)
        return CSWCAnimBaseWield::ReplaceTexture(ANIM_MODEL_BODY, resRefA, resRefB, nFlags);
    if (which == ANIM_MODEL_HEAD)
        return CSWCAnimBaseHead::ReplaceTexture(ANIM_MODEL_HEAD, resRefA, resRefB, nFlags);
    return 0;
}

int CSWCAnimBaseHeadWield::ReplaceTexture(CResRef const& resRefA, CResRef const& resRefB, int nFlags)
{
    if (!CSWCAnimBaseWield::ReplaceTexture(resRefA, resRefB, nFlags))
        return 0;
    return CSWCAnimBaseHead::ReplaceTexture(resRefA, resRefB, nFlags) != 0;
}

int CSWCAnimBaseHeadWield::ReplaceTexture(unsigned char which, CResRef const& resRefA,
                                          CResRef const& resRefB, unsigned int nCount,
                                          unsigned short* paData, int nFlags, unsigned int nOffset)
{
    if (which == ANIM_MODEL_BODY)
        return CSWCAnimBaseWield::ReplaceTexture(ANIM_MODEL_BODY, resRefA, resRefB, nCount, paData, nFlags, nOffset);
    else if (which == ANIM_MODEL_HEAD)
        CSWCAnimBaseHead::ReplaceTexture(ANIM_MODEL_HEAD, resRefA, resRefB, nCount, paData, nFlags, nOffset);
    return 0;
}

int CSWCAnimBaseHeadWield::ReplaceTexture(CResRef const& resRefA, CResRef const& resRefB,
                                          unsigned int nCount, unsigned short* paData, int nFlags,
                                          unsigned int nOffset)
{
    if (!CSWCAnimBaseWield::ReplaceTexture(resRefA, resRefB, nCount, paData, nFlags, nOffset))
        return 0;
    return CSWCAnimBaseHead::ReplaceTexture(resRefA, resRefB, nCount, paData, nFlags, nOffset) != 0;
}

void CSWCAnimBaseHeadWield::UnloadModel()
{
    CSWCAnimBaseWield::UnloadModel();
    CSWCAnimBaseHead::UnloadModel();
}

void CSWCAnimBaseHeadWield::UnloadModel(unsigned char which)
{
    if (which == ANIM_MODEL_BODY)
        CSWCAnimBaseWield::UnloadModel(ANIM_MODEL_BODY);
    else
        CSWCAnimBaseHead::UnloadModel(which);
}

void CSWCAnimBaseHeadWield::HideHeadItem()
{
    // Unused, but its construction and destruction are deliberate.
    CExoString sUnused;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_headItemId);
    if (pItem != 0) {
        Gob* pObject = pItem->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject != 0) {
            pObject->AttachToObject(0, 0, 0);
            pObject->AttachToScene(0);
        }
    }
    m_bHeadItemHidden = 1;   // head item hidden
}

void CSWCAnimBaseHeadWield::ShowHeadItem()
{
    if (m_bHeadItemHidden == 0)
        return;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_headItemId);
    if (pItem != 0) {
        Gob* pObject = pItem->GetModel(ANIM_MODEL_BODY, 1);
        if (pObject != 0)
            pObject->AttachToObject(CSWCAnimBaseHead::m_pHeadModel, const_cast<char*>("GoggleHook"), 0);
    }
    CSWCAnimBase::RefreshHilite();
    m_bHeadItemHidden = 0;
}

int CSWCAnimBaseHeadWield::SetEnvMap(int nIndex, int bCache, CResRef const& resRef)
{
    if (!CSWCAnimBaseWield::SetEnvMap(nIndex, bCache, resRef))
        return 0;
    return CSWCAnimBaseHead::SetEnvMap(nIndex, bCache, resRef) != 0;
}

int CSWCAnimBaseHeadWield::SetEnvMap(unsigned char which, int nIndex, int bCache, CResRef const& resRef)
{
    if (which == ANIM_MODEL_HEAD)
        return CSWCAnimBaseHead::SetEnvMap(ANIM_MODEL_HEAD, nIndex, bCache, resRef);
    return CSWCAnimBaseWield::SetEnvMap(which, nIndex, bCache, resRef);
}

int CSWCAnimBaseHeadWield::RestoreEnvMap()
{
    if (!CSWCAnimBaseWield::RestoreEnvMap())
        return 0;
    return CSWCAnimBaseHead::RestoreEnvMap() != 0;
}

int CSWCAnimBaseHeadWield::SetAnimation(unsigned short nAnimation, float fSpeed, int nFlags,
                                        float fStart)
{
    if (!CSWCAnimBaseWield::SetAnimationInternal(m_pModel, nAnimation, fSpeed, nFlags, fStart))
        return 0;
    return CSWCAnimBaseHead::SetAnimationInternal(CSWCAnimBaseHead::m_pHeadModel, nAnimation, fSpeed,
                                                  nFlags, fStart) != 0;
}

// A null shared model does not skip the first call: the head's own model only gates entry.
int CSWCAnimBaseHeadWield::SetOverlayAnimation(unsigned short nAnimation, int bOverlay)
{
    if (!m_pModel && !CSWCAnimBaseHead::m_pHeadModel)
        return 0;
    if (!CSWCAnimBase::SetOverlayAnimation(m_pModel, nAnimation, bOverlay))
        return 0;
    return CSWCAnimBase::SetOverlayAnimation(CSWCAnimBaseHead::m_pHeadModel, nAnimation, bOverlay) != 0;
}

int CSWCAnimBaseHeadWield::SetAnimationVisemes(float* pfA, float* pfB, int nCount, float fDuration)
{
    if (pfA != 0 && pfB != 0) {
        if (m_pModel != 0)
            m_pModel->PlayOutOfOrderAnimation(const_cast<char*>("talk"), pfA, pfB, nCount, fDuration);
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->PlayOutOfOrderAnimation(const_cast<char*>("talk"), pfA, pfB, nCount,
                                                               fDuration);
    } else {
        if (m_pModel != 0)
            m_pModel->StopOutOfOrderAnimation(const_cast<char*>("talk"));
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->StopOutOfOrderAnimation(const_cast<char*>("talk"));
    }
    return 1;
}

void CSWCAnimBaseHeadWield::SetAlwaysRender(int bAlwaysRender)
{
    if (bAlwaysRender) {
        if (m_pModel != 0)
            m_pModel->EnableAlwaysRender();
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->EnableAlwaysRender();
    } else {
        if (m_pModel != 0)
            m_pModel->DisableAlwaysRender();
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->DisableAlwaysRender();
    }
}

void CSWCAnimBaseHeadWield::SetInCutscene(int bInCutscene)
{
    if (bInCutscene) {
        if (m_pModel != 0)
            m_pModel->EnableInCutscene();
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->EnableInCutscene();
    } else {
        if (m_pModel != 0)
            m_pModel->DisableInCutscene();
        if (CSWCAnimBaseHead::m_pHeadModel != 0)
            CSWCAnimBaseHead::m_pHeadModel->DisableInCutscene();
    }
}

int CSWCAnimBaseHeadWield::SetAnimationOnSubObject(unsigned char which, unsigned short nAnimation,
                                                   float fSpeed, int nFlags, float fStart)
{
    if (which == ANIM_MODEL_BODY)
        return CSWCAnimBaseWield::SetAnimationOnSubObjectInternal(m_pModel, ANIM_MODEL_BODY, nAnimation,
                                                                 fSpeed, nFlags, fStart);
    return CSWCAnimBaseHead::SetAnimationOnSubObjectInternal(CSWCAnimBaseHead::m_pHeadModel, which,
                                                             nAnimation, fSpeed, nFlags, fStart);
}

void CSWCAnimBaseHeadWield::TurnOnShadows()
{
    CSWCAnimBaseHead::TurnOnShadows();
    CSWCAnimBaseWield::TurnOnShadows();
}

void CSWCAnimBaseHeadWield::TurnOffShadows()
{
    CSWCAnimBaseHead::TurnOffShadows();
    CSWCAnimBaseWield::TurnOffShadows();
}

int CSWCAnimBaseHeadWield::AddAnimationFlag(int nFlag)
{
    if (!CSWCAnimBaseWield::AddAnimationFlagInternal(m_pModel, nFlag))
        return 0;
    return CSWCAnimBaseHead::AddAnimationFlagInternal(CSWCAnimBaseHead::m_pHeadModel, nFlag) != 0;
}

int CSWCAnimBaseHeadWield::RemoveAnimationFlag(int nFlag)
{
    if (!CSWCAnimBaseWield::RemoveAnimationFlagInternal(m_pModel, nFlag))
        return 0;
    return CSWCAnimBaseHead::RemoveAnimationFlagInternal(CSWCAnimBaseHead::m_pHeadModel, nFlag) != 0;
}

// The union of the two models' rects.
// EFFECTIVE: frame layout only; the calls, the union and the canary are byte-exact.
int CSWCAnimBaseHeadWield::GetBoundingRect(CSWGuiExtent& extent)
{
    if (CSWCAnimBaseHead::m_pHeadModel != 0) {
        CSWGuiExtent extentHead;
        CSWGuiExtent extentBody;
        if (!CSWCAnimBase::GetBoundingRect(CSWCAnimBaseHead::m_pHeadModel, extentHead))
            return 0;
        if (!CSWCAnimBase::GetBoundingRect(extentBody))
            return 0;
        extent = CSWGuiExtent::Union(extentHead, extentBody);
        return 1;
    }
    return CSWCAnimBase::GetBoundingRect(extent);
}

// The head-only texture and icon overrides, forwarded to the CSWCAnimBaseHead slice.
int CSWCAnimBaseHeadWield::RestoreTexture()
{
    return CSWCAnimBaseHead::RestoreTexture();
}

int CSWCAnimBaseHeadWield::RestoreTexture(unsigned char which)
{
    return CSWCAnimBaseHead::RestoreTexture(which);
}

int CSWCAnimBaseHeadWield::EnableBumpedOutShell(CResRef const& resRef, float f)
{
    return CSWCAnimBaseHead::EnableBumpedOutShell(resRef, f);
}

int CSWCAnimBaseHeadWield::DisableBumpedOutShell()
{
    return CSWCAnimBaseHead::DisableBumpedOutShell();
}

int CSWCAnimBaseHeadWield::SetMotionBlur(int n)
{
    return CSWCAnimBaseHead::SetMotionBlur(n);
}

void CSWCAnimBaseHeadWield::SetHeadItem(unsigned long long itemId)
{
    m_headItemId = itemId;
}

// Both diamond parents override these, so naming the base whose slice the call needs is the whole
// body.
Gob* CSWCAnimBaseHeadWield::GetModel(unsigned char which)
{
    // The eight wielded-item slots and the wielded pair route to the wield base; the head slot
    // routes to the head base.
    switch (which) {
        case ANIM_MODEL_RIGHT_ITEM_WINGS: case ANIM_MODEL_RIGHT_ITEM_TAIL: case ANIM_MODEL_RIGHT_ITEM_HEAD:
        case ANIM_MODEL_RIGHT_ITEM_BODY:
        case ANIM_MODEL_LEFT_ITEM_WINGS: case ANIM_MODEL_LEFT_ITEM_TAIL: case ANIM_MODEL_LEFT_ITEM_HEAD:
        case ANIM_MODEL_LEFT_ITEM_BODY:
            return CSWCAnimBaseWield::GetModel(which);
        case ANIM_MODEL_HEAD:
            return CSWCAnimBaseHead::GetModel(ANIM_MODEL_HEAD);
        case ANIM_MODEL_BODY:
            return CSWCAnimBaseWield::GetModel(ANIM_MODEL_BODY);
    }
    return 0;
}

int CSWCAnimBaseHeadWield::LoadModel(CResRef const& resRef, unsigned char which)
{
    int bLoaded = (which == ANIM_MODEL_HEAD)
        ? CSWCAnimBaseHead::LoadModel(resRef, ANIM_MODEL_HEAD)
        : CSWCAnimBaseWield::LoadModel(resRef, which);
    if (!bLoaded)
        return 0;
    CSWCAnimBaseWield::ReAttachWieldedObjects();
    return 1;
}

void CSWCAnimBaseHeadWield::EnableDistortion()
{
    CSWCAnimBaseHead::EnableDistortion();
    CSWCAnimBaseWield::EnableDistortion();
}

void CSWCAnimBaseHeadWield::DisableDistortion()
{
    CSWCAnimBaseHead::DisableDistortion();
    CSWCAnimBaseWield::DisableDistortion();
}

// The shared animation-parameter block, forwarded to the CSWCAnimBaseTW slice.
int CSWCAnimBaseHeadWield::SetAnimationInternal(CAurObject* pObject, unsigned short a, float b, int c, float d)
{
    return CSWCAnimBase::SetAnimationInternal(pObject, a, b, c, d);
}

int CSWCAnimBaseHeadWield::SetAnimationOnSubObjectInternal(CAurObject* pObject, unsigned char a, unsigned short b, float c, int d, float e)
{
    return CSWCAnimBase::SetAnimationOnSubObjectInternal(pObject, a, b, c, d, e);
}

// The wielded-item overrides, forwarded to the CSWCAnimBaseWield slice.
int CSWCAnimBaseHeadWield::Wield(unsigned long long a, CExoString const& b, unsigned char c, float d)
{
    return CSWCAnimBaseWield::Wield(a, b, c, d);
}

void CSWCAnimBaseHeadWield::HideWieldedItems(CResRef const& a, CResRef const& b)
{
    CSWCAnimBaseWield::HideWieldedItems(a, b);
}

void CSWCAnimBaseHeadWield::ShowWieldedItems()
{
    CSWCAnimBaseWield::ShowWieldedItems();
}

void CSWCAnimBaseHeadWield::SetWieldLimited(int limited)
{
    CSWCAnimBaseWield::SetWieldLimited(limited);
}

void CSWCAnimBaseHeadWield::UpdateSound(int n)
{
    CSWCAnimBaseWield::UpdateSound(n);
}
