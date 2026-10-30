#ifndef CSWCVISUALEFFECT_H
#define CSWCVISUALEFFECT_H

#include "nwcobject.h"
#include "CExoLinkedList.h"

class CAurScene;
class CResRef;
class CExoSoundSource;
struct Vector;

class CSWCVisualEffect : public CSWCObject {
public:

    virtual ~CSWCVisualEffect();
    virtual void AIUpdate();
    virtual int LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char a);
    virtual void SetPosition(const Vector& v);
    virtual void AttachToScene(CAurScene* pScene);

    CExoSoundSource* m_pSoundSource;
    int  m_nFinishDelayMs;
    int  m_nAnimationTimeLeftMs;
    int  m_nEffectId;
    unsigned short m_nAnimation;
    unsigned short _pad15e;
    int  m_bFinishing;
    int  m_bFinished;
    int  m_bImpactOnly;
    int  m_bDeleteWhenFinished;
    int  m_bOrientWithGround;
    int  m_bDetachWhenFinished;
    int  m_bNoFinishDelay;
    int  m_bAnimateWhilePaused;

    CSWCVisualEffect(int a);
    static void UnloadCachedVFX();
    static void LoadCachedVFX();
    void Reset();
    void SetAnimateWhilePaused(int a);
    int  LoadEffect(unsigned int nEffect);
    using CSWCObject::SetAnimation;
    void SetAnimation(unsigned short n);
    void DoVFXRumble();

    static CExoLinkedList<CAurObject> m_lstCachedVFX;
};

#endif
