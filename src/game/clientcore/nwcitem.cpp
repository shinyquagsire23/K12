#include "k2stdint.h"
#include "nwcitem.h"
#include "nwscriptconst.h"
#include "nullname.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "nwchandlemessage.h"
#include "gameobjectarray.h"
#include "nwsobject.h"
#include "nwsitem.h"
#include "nwsitemproperty.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "AURORA/aurmodel.h"
#include "nwcarea.h"
#include "serverexoapp.h"
#include "nwsrules.h"
#include "nwitem.h"
#include "clientaimaster.h"
#include "exosound.h"
#include "Vector.h"
#include "worldtimer.h"
#include "nwcanimbase.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "itemtype.h"
#include "soundprioritygroup.h"

extern CExoSound* g_pExoSound;

#include "port/libcstring.h"

extern int hitcheck_static_trimesh_parts;

#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCItem::CSWCItem(unsigned int nBaseItem)
    : CSWItem(),
      CSWCObject(CGameObject::OBJECT_TYPE_ITEM)
{
    m_nReserved2 = -1;
    m_bInventoryRequestPending = 0;
    m_nPendingEquipSlot = -1;
    m_nReserved1 = -1;
    m_nInventorySlot = -1;
    m_baseItem = nBaseItem;
    m_nRepositoryX = REPOSITORY_POSITION_NONE;
    m_nRepositoryY = REPOSITORY_POSITION_NONE;
    m_equippedByObjectId = OBJECT_INVALID;
    m_possessorObjectId = OBJECT_INVALID;
    m_nReserved3 = 0xff;
    m_nReserved4 = 0xff;
    m_nNumActiveProperties = 0;
    m_paActiveProperties = 0;
    m_bIdentified = 0;
    m_bInContainer = 0;
    m_nStackSize = -1;
    m_bCanUse = 0;
    m_nBaseArmorClass = 0;
    m_nModelVariation = 1;
    m_nTextureVariation = 0;
    for (int i = 0; i < 8; i++)
        m_anPropertyUsesLeft[i] = 0;
    m_nSpellPropertyIndex = SPELL_PROPERTY_NONE;
    m_pPropertySound = 0;
    m_pVisualEffectGob = 0;
    m_bVisualEffectLoaded = 0;
    m_bPowered = 0;
    m_nReserved5 = 0;
}

void CSWCItem::UnloadVisualEffect()
{
    if (m_pModel)
        m_pModel->UnloadModel(ANIM_MODEL_WINGS);
}

CSWCItem* CSWCItem::GetItemByID(unsigned long long id)
{
    CGameObject* pObject;
    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(id, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        return pObject->AsSWCItem();
    return 0;
}

void CSWCItem::SetEquippedBy(CSWCCreature* pCreature)
{
#if KOTOR1  // K1: only the owner id; the model's hologram / render state is not touched
    if (pCreature != 0)
        m_equippedByObjectId = pCreature->m_objectId;
    else
        m_equippedByObjectId = OBJECT_INVALID;
#else
    Gob* pModel = static_cast<Gob*>(GetModel(ANIM_MODEL_BODY, 1));
    if (pCreature != 0)
    {
        m_equippedByObjectId = pCreature->m_objectId;
        CSWSCreature* pServer = pCreature->GetServerCreature();
        if (pModel != 0 && pServer != 0)
        {
            pModel->EnableHologram(pServer->m_bHologram != 0);
            if (pServer->m_bWillNotRender == 0)
                pModel->EnableRendering(1);
            else
                pModel->EnableRendering(0);
        }
        else if (pModel != 0)
        {
            pModel->EnableHologram(0);
            pModel->EnableRendering(1);
        }
    }
    else
    {
        m_equippedByObjectId = OBJECT_INVALID;
        if (pModel != 0)
        {
            pModel->EnableHologram(0);
            pModel->EnableRendering(1);
        }
    }
#endif
}

CSWCCreature* CSWCItem::GetEquippedBy()
{
    if (m_equippedByObjectId == OBJECT_INVALID)
        return 0;
    return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_equippedByObjectId);
}

void CSWCItem::ToggleContainerState(int nState)
{
    K2_UNUSED(nState);
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_UseObject(m_objectId);
}

void CSWCItem::UnloadModel()
{
    if (m_pModel)
    {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCItem::UnloadModel(unsigned char a) {
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

void CSWCItem::SetNumActiveProperties(int nCount)
{
    m_nNumActiveProperties = nCount;
    delete[] m_paActiveProperties;
    m_paActiveProperties = new CSWItemProperty[m_nNumActiveProperties];
}

CSWItemProperty* CSWCItem::GetActiveProperty(int nIndex)
{
    if (nIndex < 0 || m_nNumActiveProperties <= nIndex)
        return 0;
    return &m_paActiveProperties[nIndex];
}

int CSWCItem::GetActivePropertyByTypeExists(unsigned short nType, unsigned short nSubType)
{
    for (int i = 0; i < m_nNumActiveProperties; i++)
    {
        if (m_paActiveProperties[i].m_type == nType
            && (nSubType == 0 || m_paActiveProperties[i].m_subType == nSubType))
            return 1;
    }
    return 0;
}

CSWItemProperty* CSWCItem::GetActivePropertyByType(unsigned short nType, unsigned short nSubType)
{
#if KOTOR1
    CSWItemProperty* pProperty = m_paActiveProperties;
    for (long long i = 0; i < m_nNumActiveProperties; i++, pProperty++)
    {
        if (pProperty->m_type == nType && (nSubType == 0 || pProperty->m_subType == nSubType))
            return pProperty;
    }
    return 0;
#else
    for (int i = 0; i < m_nNumActiveProperties; i++)
    {
        if (m_paActiveProperties[i].m_type == nType
            && (nSubType == 0 || m_paActiveProperties[i].m_subType == nSubType))
            return &m_paActiveProperties[i];
    }
    return 0;
#endif
}

void CSWCItem::ExamineItem()
{
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_Examine(m_objectId);
}

int CSWCItem::GetActivePropertySpellID(int nIndex)
{
    if (m_nNumActiveProperties <= nIndex)
        return -1;
    if (m_paActiveProperties[nIndex].m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
        return -1;
    return m_paActiveProperties[nIndex].m_subType;
}

void CSWCItem::SetOrientation(const Vector& v) {
    m_orientation = v;
    UpdateOrientation(v);
}

void CSWCItem::ComputeZValue(Vector* pPos, float fCurrentZ, Vector* pUnk)
{
    K2_UNUSED(fCurrentZ);
    K2_UNUSED(pUnk);
    pPos->z = 0.0f;
    if (m_pArea != 0)
        pPos->z = g_pAppManager->m_pServerExoApp->ComputeHeight(m_pArea->m_areaResRef, *pPos, 1);
}

void CSWCItem::SetStackSize(int nStackSize)
{
    m_nStackSize = nStackSize;
    int nBase = m_baseItem;
    g_pRules->m_pBaseItemArray->GetBaseItem(nBase);
}

void CSWCItem::SetItemPropertyUsesLeft(int nIndex, unsigned short nUses)
{
    if (nIndex > 7)
        return;
    m_anPropertyUsesLeft[nIndex] = nUses;
    if (nUses != 0)
        return;
    // Only a drained property that was the item's live spell source triggers a re-scan.
    if (m_nSpellPropertyIndex != nIndex)
        return;
    int nFound = SPELL_PROPERTY_NONE;
    for (int i = 0; i < 8; i++)
    {
        CSWItemProperty* pProp = GetActiveProperty(i);
        if (pProp != 0 && pProp->m_type == ITEM_PROPERTY_ACTIVATE_ITEM && m_anPropertyUsesLeft[i] != 0)
        {
            nFound = i;
            break;
        }
    }
    m_nSpellPropertyIndex = static_cast<unsigned char>(nFound);
}

void CSWCItem::ShowFeedbackBubbleText()
{
}

CSWSItem* CSWCItem::GetServerItem()
{
    if (GetServerObject() == 0)
        return 0;
    return GetServerObject()->AsSWSItem();
}

void CSWCItem::ShowVisualEffect()
{
    if (m_pModel == 0)
        return;
    Gob* pEffect = GetModel(ANIM_MODEL_WINGS, 1);
    if (pEffect == 0)
        return;
    Gob* pTarget = GetModel(ANIM_MODEL_BODY, 1);
    if (pTarget == 0)
        return;
    pEffect->AttachToObject(static_cast<CAurObject*>(pTarget), const_cast<char*>("root"), 0);
}

void CSWCItem::HideVisualEffect()
{
    if (m_pModel == 0)
        return;
    Gob* pEffect = GetModel(ANIM_MODEL_WINGS, 1);
    if (pEffect == 0)
        return;
    pEffect->AttachToObject(0, 0, 0);
    pEffect->AttachToScene(0);
}

void CSWCItem::SpawnVisualEffect(const CExoString& sVisual, const CExoString& sHook)
{
    if (m_pVisualEffectGob) {
        delete m_pVisualEffectGob;
        m_pVisualEffectGob = 0;
    }
    m_pVisualEffectGob = static_cast<Gob*>(NewCAurObject(static_cast<char*>(sVisual.CStr()), const_cast<char*>("specialattack"), 0, 0));
    if (m_pVisualEffectGob != 0)
    {
        m_pVisualEffectGob->AttachToObject(static_cast<CAurObject*>(GetModel(ANIM_MODEL_BODY, 1)), static_cast<char*>(sHook.CStr()), 0);
        m_pVisualEffectGob->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
    }
    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(this);
}

// Frees the active properties, the visual-effect model and the property sound.
CSWCItem::~CSWCItem()
{
    UnloadVisualEffect();
    UnloadModel();
    if (m_paActiveProperties != 0)
    {
        delete m_paActiveProperties;
        m_paActiveProperties = 0;
    }
    if (m_pVisualEffectGob != 0)
    {
        Gob* pGob = m_pVisualEffectGob;
        pGob->AttachToObject(0, const_cast<char*>(""), 0);
        delete m_pVisualEffectGob;
        m_pVisualEffectGob = 0;
    }
    delete m_pPropertySound;
    m_pPropertySound = 0;
}

// Builds the item's "Use" submenu, which exists only when the item carries two or more spell
// properties.
int CSWCItem::BuildUseSubmenu()
{
    if (m_nNumActiveProperties < 2)
        return 0;
    int nSpellProperties = 0;    for (int i = 0; i < m_nNumActiveProperties; i++)
    {
        if (m_paActiveProperties[i].m_type == ITEM_PROPERTY_ACTIVATE_ITEM)
            nSpellProperties++;
    }
    if (nSpellProperties <= 1)
        return 0;
    return 1;
}

void CSWCItem::PlayPickupSound()
{
    g_pExoSound->PlayOneShotSound(CResRef("it_pickup"), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
}

void CSWCItem::PlayDropSound()
{
    // Only the armour-like base-item categories get a material cue; the rest use the base row's own
    // drop-sound index.
    CResRef resSound;
    if (GetBaseItem()->m_nItemType >= ITEM_TYPE_ARMOR_CLASS_4 && GetBaseItem()->m_nItemType <= ITEM_TYPE_JEDI_ROBE)
    {
        switch (m_nMaterialClass)
        {
        case 0: resSound = const_cast<char*>("it_armorleather"); break;
        case 1: resSound = const_cast<char*>("it_armorchain"); break;
        case 2: resSound = const_cast<char*>("it_armorplate"); break;
        case 3: resSound = const_cast<char*>("it_materialcloth"); break;
        }
    }
    else
    {
        unsigned char nInvSound =
            g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nInvSoundType;
        resSound = g_pAppManager->m_pClientExoApp->GetItemDropSound(nInvSound);
    }
    g_pExoSound->PlayOneShotSound(resSound, PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
}

int CSWCItem::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(b);
    if (m_pModel == 0)
        m_pModel = new CSWCAnimBase();
    if (GetAnimationBase()->LoadModel(r, a) == 0)
        return 0;
    Gob* pGob = GetAnimationBase()->GetModel(a);
    if (pGob != 0)
    {
        pGob->SetExternalData(this);
        pGob->SetExternalDataType(CGameObject::OBJECT_TYPE_ITEM);
    }
    // A null model dereferences here.
    pGob->SetLightPriority(5);
    if (a == ANIM_MODEL_BODY)
        m_fHeadHeight = 0.5f;
    return 1;
}

void CSWCItem::SetArea(CSWCArea* pArea)
{
    if (pArea != 0)
    {
        HideVisualEffect();
        CSWCArea* pOldArea = m_pArea;
        CSWCObject::SetArea(pArea);
        // Re-push the position only when the object really moved.
        if (pOldArea != pArea)
            SetPosition(m_position);
    }
    else
    {
        ShowVisualEffect();
        CSWCObject::SetArea(0);
    }
}

void CSWCItem::UpdateOrientation(const Vector& v)
{
    K2_UNUSED(v);
    Quaternion q;
    float fPitch, fRoll;
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bRotateOnGround == 1)
    {
        fPitch = 0.0f;
        fRoll = 90.0f;
    }
    else
    {
        fPitch = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bRotateOnGround == 2
                     ? 90.0f
                     : 0.0f;
        fRoll = 0.0f;
    }
    q = YawPitchRoll(Yaw(m_orientation), fPitch, fRoll);
    if (m_pModel != 0)
        m_pModel->SetOrientation(q);
}

void CSWCItem::SetPosition(const Vector& v)
{
    Vector vPos = v;
    // Keep the model from being re-culled while the item is placed.
    int nOldTrimeshParts = hitcheck_static_trimesh_parts;
    hitcheck_static_trimesh_parts = 1;
    ComputeZValue(&vPos, 0.0f, 0);
    hitcheck_static_trimesh_parts = nOldTrimeshParts;
    vPos.z += 0.1f;
    CSWCObject::SetPosition(vPos);
}

void CSWCItem::AIUpdate()
{
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nElapsedDay, &nElapsedTod);
    m_nAIUpdateDeltaMs = nElapsedTod;
    UpdatePopUpBubble();
    if (FadeObject() != 0)
    {
        g_pAppManager->m_pClientExoApp->GetClientAIMaster()->m_list.RemoveObject(this);
        CGameObject* pObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0)
        {
            delete this;
            return;
        }
    }
    // The special-attack effect model retires itself once its "impact" animation has run out.
    if (m_pVisualEffectGob != 0)
    {
        float fTime = 0.0f;
        m_pVisualEffectGob->GetAnimation(const_cast<char*>("impact"), &fTime, 0);
        if (fTime == -1.0f)
        {
            m_pVisualEffectGob->AttachToObject(0, const_cast<char*>(""), 0);
            delete m_pVisualEffectGob;
            m_pVisualEffectGob = 0;
            g_pAppManager->m_pClientExoApp->GetClientAIMaster()->RemoveObject(this);
        }
    }
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
}

void CSWCItem::ResolveCreaturePoweredAnimations(int nAnimation)
{
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_equippedByObjectId);
    if (pCreature == 0)
        return;
    // Which hand holds this item: bit 0 the right, bit 1 the left.
    unsigned char nHandMask;
    if (pCreature->GetEquippedItemID(CSWInventory::SLOT_RIGHTWEAPON) == m_objectId)
        nHandMask = 1;
    else
        nHandMask = (pCreature->GetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON) == m_objectId) << 1;
    CSWCAnimBase* pAnimBase = pCreature->GetAnimationBase();
    if (pAnimBase == 0)
        return;
    if (nAnimation != 0)
    {
        // Raising the powered animation adds this item's hands to the creature's, unless the item
        // was already the powered one.
        if (m_bPowered == 0)
            nHandMask |= pCreature->GetAnimationBase()->m_nPoweredHandMask;
    }
    else
    {
        // Lowering it clears this item's bits, or the whole set when the item was not the powered
        // one.
        if (m_bPowered != 0)
            nHandMask = pCreature->GetAnimationBase()->m_nPoweredHandMask & ~nHandMask;
        else
            nHandMask = 0;
    }
    // The blade-like base categories drop bit 2; everything else raises it.
    if (GetBaseItem()->m_nItemType == ITEM_TYPE_DOUBLE_LIGHTSABER
        || GetBaseItem()->m_nItemType == ITEM_TYPE_SHORT_LIGHTSABER
        || GetBaseItem()->m_nItemType == ITEM_TYPE_LIGHTSABER)
        nHandMask &= ~4;
    else
        nHandMask |= 4;
    pCreature->GetAnimationBase()->m_nPoweredHandMask = nHandMask;
}

CExoString CSWCItem::GetName(int nForm)
{
    STR_RES strRes;
    CExoString sName;
    if (nForm == 0)
        return m_sDisplayName;
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (m_sDisplayName.GetLength() > 0
        && m_bIdentified != 0)
#else
    if (m_sDisplayName.m_sString != 0 && static_cast<int>(strlen(m_sDisplayName.m_sString)) > 0
        && m_bIdentified != 0)
#endif
        return m_sDisplayName;
    g_pTlkTable->Fetch(static_cast<unsigned int>(g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)
                           ->m_nNameStrRef),
                       strRes, CTlkTable::GENDER_MALE);
    sName = strRes.text;
    if (m_bIdentified == 0)
    {
        // No per-instance name yet: the display name is the base item's.
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        sName = sName + CExoString(" ") + strRes.text;
    }
    return sName;
}

CSWCItem* CSWCItem::AsSWCItem()
{
    return this;
}

int CSWCItem::LoadVisualEffect(unsigned char nWhich)
{
    CExoString sEffect;
    if (m_pModel == 0)
        return 0;
    CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem);
    if (pBaseItem == 0)
        return 0;
    sEffect = pBaseItem->m_resRefStem;
    switch (nWhich)
    {
    case 1: sEffect = sEffect + CExoString("_fxacid"); break;
    case 2: sEffect = sEffect + CExoString("_fxfrost"); break;
    case 3: sEffect = sEffect + CExoString("_fxelect"); break;
    case 4: sEffect = sEffect + CExoString("_fxfire"); break;
    case 5: sEffect = sEffect + CExoString("_fxsonic"); break;
    case 6: sEffect = sEffect + CExoString("_fxholy"); break;
    case 7: sEffect = sEffect + CExoString("_fxneg"); break;
    default: return 0;
    }
    if (m_pModel->LoadModel(CResRef(sEffect), ANIM_MODEL_WINGS) != 0)
    {
        m_bVisualEffectLoaded = 1;
        return 1;
    }
    return 0;
}

void CSWCItem::PowerItem(int nPowered, int b, int c, int d)
{
    // A running minigame suppresses the whole effect; the item keeps its unpowered visuals.
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pMiniGame != 0)
        return;
    if (m_equippedByObjectId != OBJECT_INVALID
        && g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_equippedByObjectId) != 0
        && g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_equippedByObjectId)
                   ->m_bOverrideLightsaberState
               == 1
        && b == 0)
        return;
    // While an area transition is still fading in, powering down is deferred: the creature would be
    // re-equipped the moment the area becomes displayable.
    if (nPowered == 1
        && g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay() == 1
        && g_pAppManager->m_pClientExoApp->GetInGameGui()->IsGlobalFading() == 0)
    {
        ResolveCreaturePoweredAnimations(0);
        return;
    }
    ResolveCreaturePoweredAnimations(nPowered);
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bPoweredItem != 1)
        return;
    g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_equippedByObjectId);
#if KOTOR1  // K1 fetches the anim base only on a state change and never null-tests it
    if (m_bPowered != nPowered)
    {
        CSWCAnimBase* pAnimBase = GetAnimationBase();
#else
    CSWCAnimBase* pAnimBase = GetAnimationBase();
    if (pAnimBase == 0)
        return;
    if (m_bPowered != nPowered)
    {
#endif
        // The item's own looping property sound, created once.
        Vector vPos = static_cast<Gob*>(pAnimBase->GetModel(ANIM_MODEL_BODY))->GetPosition();
        if (m_pPropertySound == 0)
        {
            m_pPropertySound = new CExoSoundSource(CResRef(
                g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_sPoweredSnd));
            m_pPropertySound->Set3D(1);
            m_pPropertySound->SetPosition(vPos, 0.0f);
        }
        unsigned short nAnimPowered;
        unsigned short nAnimUnpowered;
        if (nPowered != 0)
        {
            if (d == 0)
            {
                if (m_pPropertySound != 0)
                {
                    m_pPropertySound->SetPriorityGroup(PRIORITY_GROUP_SINGLE_SHOT_GLOBAL);
                    m_pPropertySound->SetLooping(1);
                    m_pPropertySound->Set3D(1);
                    m_pPropertySound->SetPosition(vPos, 0.0f);
                    m_pPropertySound->Play();
                }
            }
            nAnimPowered = CSWCAnimBase::CLIENT_ANIMATION_POWER_UP;
            nAnimUnpowered = CSWCAnimBase::CLIENT_ANIMATION_POWERED;
        }
        else
        {
            if (m_pPropertySound != 0)
                m_pPropertySound->Stop();
            nAnimPowered = CSWCAnimBase::CLIENT_ANIMATION_POWER_DOWN;
            nAnimUnpowered = CSWCAnimBase::CLIENT_ANIMATION_OFF;
        }
        if (c != 0)
        {
            if (m_bPowered != 0)
            {
                // Re-powering after a switch: the blade's own one-shot.
                if (d == 0)
                    g_pExoSound->Play3DOneShotSound(
                        CResRef(
                            g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_sPowerDownSnd),
                        vPos, 0.0f, PRIORITY_GROUP_SINGLE_SHOT_GLOBAL, 0, 0, 0.0f, 0.0f);
            }
            else
            {
                // First power-up: the model plays its placeholder animation and the other resref is
                // the one-shot.
                if (m_pModel->GetModel(ANIM_MODEL_BODY) != 0)
                    m_pModel->GetModel(ANIM_MODEL_BODY)
                        ->PlayAnimation(const_cast<char*>(NULL_NAME), 1.0f, 0, 0.0f);
                if (d == 0)
                    g_pExoSound->Play3DOneShotSound(
                        CResRef(
                            g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_sPowerUpSnd),
                        vPos, 0.0f, PRIORITY_GROUP_SINGLE_SHOT_GLOBAL, 0, 0, 0.0f, 0.0f);
            }
            m_pModel->SetAnimation(nAnimPowered, 1.0f, 1, 0.0f);
            m_pModel->SetAnimation(nAnimUnpowered, 1.0f, AnimRun::kAnimQueue, 0.0f);
        }
        else
        {
            m_pModel->SetAnimation(nAnimUnpowered, 1.0f, 0, 0.0f);
        }
    }
    m_bPowered = nPowered;
}
