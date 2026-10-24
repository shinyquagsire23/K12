#ifndef CSWCANIMBASEHEADWIELD_H
#define CSWCANIMBASEHEADWIELD_H

#include "nwcanimbasehead.h"
#include "nwcanimbasewield.h"

// The combined head/wield animation class, a virtual diamond over CSWCAnimBaseHead and
// CSWCAnimBaseWield, both of which virtually derive from CSWCAnimBaseTW.
class CSWCAnimBaseHeadWield : public virtual CSWCAnimBaseHead, public virtual CSWCAnimBaseWield {
public:
    CSWCAnimBaseHeadWield();
    virtual ~CSWCAnimBaseHeadWield();

    // Declaration order is the vtable order: the base-object destructor's UnloadModel() call
    // resolves by slot, so a different order would call a different override.
    virtual Gob* GetModel(unsigned char);
    virtual int   LoadModel(CResRef const&, unsigned char);
    virtual int   ReplaceTexture(unsigned char, CResRef const&, CResRef const&, int);
    virtual int   ReplaceTexture(CResRef const&, CResRef const&, int);
    virtual int   ReplaceTexture(unsigned char, CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);
    virtual int   ReplaceTexture(CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);
    virtual int   RestoreTexture();
    virtual int   RestoreTexture(unsigned char);
    virtual int  EnableBumpedOutShell(CResRef const&, float);
    virtual int   DisableBumpedOutShell();
    virtual int   SetMotionBlur(int);
    virtual void  UnloadModel();
    virtual void  UnloadModel(unsigned char);
    virtual int   Wield(unsigned long long, CExoString const&, unsigned char, float);
    virtual void  HideWieldedItems(CResRef const&, CResRef const&);
    virtual void  ShowWieldedItems();
    virtual void  HideHeadItem();
    virtual void  ShowHeadItem();
    virtual void  SetWieldLimited(int);
    virtual int   SetEnvMap(int, int, CResRef const&);
    virtual int   SetEnvMap(unsigned char, int, int, CResRef const&);
    virtual int   RestoreEnvMap();
    virtual int   SetAnimation(unsigned short, float, int, float);
    virtual int   SetAnimationVisemes(float*, float*, int, float);
    virtual int   SetAnimationOnSubObject(unsigned char, unsigned short, float, int, float);
    // Both parents override these two, so this class must declare its own overrider.
    virtual void  TurnOnShadows();
    virtual void  TurnOffShadows();
    virtual void  UpdateSound(int);
    virtual int   AddAnimationFlag(int);
    virtual int   RemoveAnimationFlag(int);
    virtual int   GetBoundingRect(CSWGuiExtent&);
    // These chain to both parents.
    virtual void  EnableDistortion();
    virtual void  DisableDistortion();
    void          SetHeadItem(unsigned long long);
    virtual int   SetAnimationInternal(CAurObject*, unsigned short, float, int, float);
    virtual int   SetAnimationOnSubObjectInternal(CAurObject*, unsigned char, unsigned short, float, int, float);
    virtual int   SetOverlayAnimation(unsigned short, int);
    virtual void  SetAlwaysRender(int);
    virtual void  SetInCutscene(int);
};

#endif // CSWCANIMBASEHEADWIELD_H
