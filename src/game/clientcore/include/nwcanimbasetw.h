#ifndef CSWCANIMBASETW_H
#define CSWCANIMBASETW_H

#include "nwcanimbase.h"

class CAurObject;
class CResRef;

class CSWCAnimBaseTW : public CSWCAnimBase {
public:

    virtual ~CSWCAnimBaseTW();
    virtual Gob* GetModel(unsigned char);  
    virtual int LoadModel(CResRef const&, unsigned char);  
    virtual int ReplaceTexture(unsigned char, CResRef const&, CResRef const&, int);  
    virtual int ReplaceTexture(CResRef const&, CResRef const&, int);  
    virtual int ReplaceTexture(unsigned char, CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);  
    virtual int ReplaceTexture(CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);  
    virtual int RestoreTexture();  
    virtual int RestoreTexture(unsigned char);  
    virtual void UnloadModel();  
    virtual void UnloadModel(unsigned char);  
    virtual int SetEnvMap(int, int, CResRef const&);  
    virtual int SetEnvMap(unsigned char, int, int, CResRef const&);  
    virtual int RestoreEnvMap();  
    virtual int SetAnimationInternal(CAurObject*, unsigned short, float, int, float);  
    virtual int SetAnimationOnSubObjectInternal(CAurObject*, unsigned char, unsigned short, float, int, float);  

    CSWCAnimBaseTW();

    // Holds the last ReplaceTexture applied to each sub-object, which RestoreTexture replays: up to
    // ten shorts of its data and the count / offset it was called with.
    struct TextureCache {
        unsigned short m_anData[10];
        unsigned char  m_nCount;
        unsigned char  m_nOffset;
    };
    TextureCache m_tailTextureCache;
    TextureCache m_wingsTextureCache;
    // Four 16-byte resource names, each trailed by an int (all assigned "").
    CResRef m_resTailTexture;
    int     m_bTailTexturePartial;
    CResRef m_resWingsTexture;
    int     m_bWingsTexturePartial;
    CResRef m_resTailEnvMap;
    int     m_bTailEnvMap;
    CResRef m_resWingsEnvMap;
    int     m_bWingsEnvMap;
    // Two CAurObjects the base owns and releases in its destructor.
    CAurObject* m_pTail;
    CAurObject* m_pWings;
};

#endif // CSWCANIMBASETW_H
