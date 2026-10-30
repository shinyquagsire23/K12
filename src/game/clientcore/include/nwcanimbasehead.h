// Client animation base for head and headgear. A virtual CSWCAnimBaseTW derivation, so it is one arm
// of the Head / Wield / HeadWield diamond.
#pragma once

#include "nwcanimbasetw.h"
#include "exoresman.h"

class CSWCAnimBaseHead : public virtual CSWCAnimBaseTW {
public:
    CSWCAnimBaseHead();
    virtual ~CSWCAnimBaseHead();

    // Declaration order is the vtable order.
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
    virtual int   SetEnvMap(int, int, CResRef const&);
    virtual int   SetEnvMap(unsigned char, int, int, CResRef const&);
    virtual int   RestoreEnvMap();
    virtual void  TurnOnShadows();
    virtual void  TurnOffShadows();
    virtual void  EnableDistortion();
    virtual void  DisableDistortion();

    unsigned short     m_anTextureData[10];    // the head record's data block; ReplaceTexture writes here
    unsigned char      m_nTextureCount;        // the head record's count/offset byte pair
    unsigned char      m_nTextureOffset;
    char               _pad1e[2];
    CResRef            m_resTexture;
    int                m_bTextureDataCached;
    CResRef            m_resEnvMap;
    int                m_bEnvMapEnabled;
    // The head part's own model object, a second model pointer beside the shared one on the base.
    CAurObject*        m_pHeadModel;
    unsigned long long m_headItemId;   // game-object id of the equipped head item

    void SetHeadItem(unsigned long long itemId);
};
