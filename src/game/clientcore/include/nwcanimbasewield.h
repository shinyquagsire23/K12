// Client animation base for wielded items. A virtual CSWCAnimBaseTW derivation, the other arm of the
// Head / Wield / HeadWield diamond.
#pragma once

#include "nwcanimbasetw.h"
#include "exostring.h"
#include "gameobjectarray.h"

class CAurObject;
class CExoSoundSource;

class CSWCAnimBaseWield : public virtual CSWCAnimBaseTW {
public:
    CSWCAnimBaseWield();
    virtual ~CSWCAnimBaseWield();

    CResRef            m_resDummyRight;
    CResRef            m_resDummyLeft;
    CExoString         m_sWieldedLeftNode;
    CExoString         m_sWieldedRightNode;
    CAurObject*        m_pDummyRight;
    CAurObject*        m_pDummyLeft;
    unsigned long long m_oidWieldedLeft;       // CGameObjectArray::INVALIDOBJID
    unsigned long long m_oidWieldedRight;       // CGameObjectArray::INVALIDOBJID
    int                m_bWieldLimited;
    int                m_bWieldedItemsHidden;
    int                m_bHeadItemHidden;
    int                m_bTorchLit;
    // The looping wielded-item sound source, owned.
    CExoSoundSource*   m_pTorchSound;

    // Declaration order is the vtable order.
    virtual Gob* GetModel(unsigned char);
    virtual int   LoadModel(CResRef const&, unsigned char);
    virtual int   Wield(unsigned long long, CExoString const&, unsigned char, float);
    virtual void  HideWieldedItems(CResRef const&, CResRef const&);
    virtual void  ShowWieldedItems();
    virtual void  HideHeadItem();
    virtual void  ShowHeadItem();
    virtual void  SetWieldLimited(int);
    virtual void  TurnOnShadows();
    virtual void  TurnOffShadows();
    virtual void  UpdateSound(int);
    virtual void  EnableDistortion();
    virtual void  DisableDistortion();
    virtual int   SetAnimationInternal(CAurObject*, unsigned short, float, int, float);
    virtual int   SetAnimationOnSubObjectInternal(CAurObject*, unsigned char, unsigned short, float, int, float);

    // Non-virtual; CSWCAnimBaseHeadWield::LoadModel reaches it through a virtual-base adjustment,
    // so the declaration lives here.
    void ReAttachWieldedObjects();

    // Resolves a game-object id to the item's own model and hands the model selector straight to it.
    Gob* GetItemAurObject(unsigned long long oid, unsigned char which);
    // Whether the wielding is still allowed: always 1 while the wield limit is off, otherwise
    // whether the game-object id still resolves.
    int ItemIsValid(unsigned long long oid);
};
