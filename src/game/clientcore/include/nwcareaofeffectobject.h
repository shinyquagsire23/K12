#ifndef CSWCAREAOFEFFECTOBJECT_H
#define CSWCAREAOFEFFECTOBJECT_H

#include "nwcobject.h"
#include "exoresman.h"

class CAurObject;
class CAurScene;
class CExoSoundSource;
class CSWCAnimBase;
class CSWCVisualEffect;

// A persistent area-of-effect effect. It renders as m_nInstances animation models held in the
// base's model slot, each with its own delay, timer and state, so one object draws a whole animated
// cluster.
class CSWCAreaOfEffectObject : public CSWCObject {
public:
    unsigned int*  m_panInstanceDelay;
    unsigned int*  m_panInstanceTimer;
    unsigned int*  m_panInstanceLength;
    int            m_anBandDuration[3];
    int            m_anBandCount[3];
    int            m_nInstances;
    float          m_fRadius;
    float          m_fWidth;
    float          m_fLength;
    int            m_nCessationTimer;
    int            m_bAllInstancesDone;
    int            m_bStopping;
    unsigned char  m_anBandChance[3];
    unsigned char  m_cShape;
    unsigned int*  m_panInstanceDone;
    CAurScene*     m_pScene;
    unsigned short* m_panInstanceState;
    CSWCVisualEffect* m_pVisualEffect;
    int            m_bVisualEffectAttached;
    int            m_nReserved1;
    CExoSoundSource* m_pSoundSource;
    CResRef        m_sHitSound;
    int            m_nHitSoundChance;
    int            m_bOrientWithGround;
    int            m_nPersistentEffect;
    int            m_bSoundStarted;
    unsigned long long m_nCreatorId;

    virtual ~CSWCAreaOfEffectObject();
    virtual CSWCAreaOfEffectObject* AsSWCAreaOfEffectObject();
    virtual void AIUpdate();
    virtual int LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char a);
    virtual void SetPosition(const Vector& v);
    virtual int SetEnvMap(int a, int b, const CResRef& r);
    virtual int SetEnvMap(unsigned char a, int b, int c, const CResRef& r);

    CSWCAreaOfEffectObject();

    using CSWCObject::SetAnimation;
    int  SetAnimation(unsigned short nAnimation, unsigned char nInstance);
    int  SetAnimation(unsigned short nAnimation);   // always returns 0

    // Loads the effect row named by the wire's effect id. Returns nonzero on success.
    int LoadEffect(unsigned int nEffectId);

    // Re-seeds every instance's delay for the band the argument falls into.
    void ResetTimer(int nTimer);
    // Re-arms one instance's millisecond timer when it is the instance currently owned by pObject.
    void SetMoveTimer(unsigned int nTimer, CAurObject* pObject);
    // Randomises one instance's position, returning 1 when it moved.
    int  SetInstanceRandomPosition(unsigned char nInstance);
    // One pass of the per-instance state machine.
    void UpdateInstancePosition();
    // Hides every instance that falls outside the camera frustum.
    void HideClippedInstances();

    // Non-virtual; the add handler's delete message calls it and discards the result.
    void StopEffect();
};

// The model array is reached through the CSWCObject base's model pointer directly; the .cpp spells
// that as a cast so CSWCObject.h can keep its placeholder type.
K2_LAYOUT_ASSERT(sizeof(CSWCAreaOfEffectObject) == 0x1f0, "CSWCAreaOfEffectObject size drift");

#endif
