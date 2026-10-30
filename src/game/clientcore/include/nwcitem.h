#pragma once

// The client-side item object, deriving from CSWItem and CSWCObject.

#include "nwitem.h"
#include "nwcobject.h"
#include "exoresman.h"
#include "exostring.h"

class CSWSItem;
class CSWCCreature;
class CSWBaseItem;
class CSWItemProperty;
class Gob;
class CExoSoundSource;

class CSWCItem : public CSWItem, public CSWCObject {
public:
    // m_nRepositoryX / m_nRepositoryY outside a repository grid, and m_nSpellPropertyIndex with no
    // spell property left to cast.
    enum {
        REPOSITORY_POSITION_NONE = 0xff,
        SPELL_PROPERTY_NONE      = 0xff,
    };

    int           m_nBaseArmorClass;
    unsigned char m_nMaterialClass;
    int           m_bIdentified;
    int           m_bStolen;
    unsigned char m_nModelVariation;
    unsigned char m_nTextureVariation;
    unsigned char _pad172[0x174 - 0x172];
    int           m_nNumActiveProperties;
    CSWItemProperty* m_paActiveProperties;
    CResRef       m_modelResRef;
    CResRef       m_iconResRef;
    int           m_bInventoryRequestPending;
    int           m_nPendingEquipSlot;
    int           m_nReserved1;
    int           m_nInventorySlot;
    int           m_nReserved2;
    unsigned char m_nRepositoryX;
    unsigned char m_nRepositoryY;
    unsigned char m_nReserved3;
    unsigned char m_nReserved4;
    unsigned long long m_equippedByObjectId;
    unsigned long long m_possessorObjectId;
    int           m_bInContainer;
    unsigned short m_anPropertyUsesLeft[8];
    unsigned char  m_nSpellPropertyIndex;
    int           m_nCost;
    int           m_nStackSize;
    int           m_bCanUse;
    CExoString    m_sDisplayName;
    Gob*          m_pVisualEffectGob;
    int           m_bVisualEffectLoaded;
    int           m_bPowered;
    int           m_nReserved5;
    CExoSoundSource* m_pPropertySound;

    // nBaseItem is a baseitems.2da row index.
    CSWCItem(unsigned int nBaseItem);

    // Declaration order is the vtable order; do not sort.
    virtual ~CSWCItem();
    virtual CSWCItem* AsSWCItem();
    virtual void      AIUpdate();
    virtual int       LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    void SetIcon(const CResRef& resRef) { m_iconResRef = resRef; }
    void SetModelResRef(const CResRef& resRef) { m_modelResRef = resRef; }
    virtual void      UnloadModel();
    virtual void      UnloadModel(unsigned char a);
    virtual void      SetOrientation(const Vector& v);
    virtual void      SetArea(CSWCArea* pArea);
    virtual void      SetPosition(const Vector& v);

    // Loads the effect model for the effect slot nWhich, 1 to 7; returns 1 only when it really
    // loaded.
    int       LoadVisualEffect(unsigned char nWhich);
    // Shows or hides the render model's effect sub-model; hiding also clears its scene attachment.
    void      ShowVisualEffect();
    void      HideVisualEffect();
    // Rewrites the animation ids this item's model plays for the creature wearing it.
    void      ResolveCreaturePoweredAnimations(int nAnimation);
    // Returns the client item for a game-object id, or null when that id is not a CSWCItem.
    static CSWCItem* GetItemByID(unsigned long long id);
    // Drops the effect sub-model from the render model.
    void      UnloadVisualEffect();
    // Menu and one-shot forwarders into the player-to-server message stream.
    void      ToggleContainerState(int nState);
    void      ExamineItem();
    // No-op stand-in for the feedback-bubble hook; not an override, since CSWCObject has no such
    // virtual.
    void      ShowFeedbackBubbleText();
    // Writes pPos->z from the server height query over the item's area, or 0 when the item is
    // loose. fCurrentZ and pUnk are unused.
    void      ComputeZValue(Vector* pPos, float fCurrentZ, Vector* pUnk);
    // The first active property matching (nType, nSubType), or whether one exists. nSubType == 0
    // matches any subtype.
    CSWItemProperty* GetActivePropertyByType(unsigned short nType, unsigned short nSubType);
    int       GetActivePropertyByTypeExists(unsigned short nType, unsigned short nSubType);
    // Spell id granted by the nIndex-th active property, -1 when the property is not a spell grant
    // or nIndex is out of range.
    int       GetActivePropertySpellID(int nIndex);
    // Returns this item's server twin, null when it has no server object.
    CSWSItem* GetServerItem();
    // Pushes the world orientation into the render model and area bookkeeping.
    void      UpdateOrientation(const Vector& v);
    // Toggles the lightsaber or blade power visual.
    void      PowerItem(int a, int b, int c, int d);
    // Stamps the equipping creature; unequipping clears it to null.
    void      SetEquippedBy(CSWCCreature* pCreature);
    // Returns the creature currently wearing or wielding this item, null when it is loose.
    CSWCCreature* GetEquippedBy();
    // Spawns a visual on the item. The return value is unused.
    void      SpawnVisualEffect(const CExoString& sVisual, const CExoString& sHook);
    void      SetStackSize(int nStackSize);
    // The drop and handling one-shot for this item's material class.
    void      PlayDropSound();
    // The pickup counterpart, always "it_pickup".
    void      PlayPickupSound();
    // True when the item carries two or more spell properties, so the inventory menu's Use action
    // needs a submenu.
    int       BuildUseSubmenu();
    // The nIndex-th active property, or null when nIndex is out of range.
    CSWItemProperty* GetActiveProperty(int nIndex);
    // Returns the display name; nForm selects the name form and every call site passes 0.
    using CSWCObject::GetName;
    CExoString GetName(int nForm);
    // Sizes the active-property array ahead of a bulk refresh.
    void      SetNumActiveProperties(int nCount);
    void      SetItemPropertyUsesLeft(int nIndex, unsigned short nUses);
};

K2_LAYOUT_ASSERT(sizeof(CSWCItem) == 0x220, "CSWCItem layout drifted (bases must end at 0x160)");
