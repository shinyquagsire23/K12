#ifndef CSWCANIMBASEVISUALEFFECT_H
#define CSWCANIMBASEVISUALEFFECT_H

#include "nwcanimbase.h"

class CAurObject;
class CResRef;

class CSWCAnimBaseVisualEffect : public CSWCAnimBase {
public:
    // Declared rather than left implicit: `new CSWCAnimBaseVisualEffect[n]` has to call it for
    // every element.
    CSWCAnimBaseVisualEffect();

    virtual ~CSWCAnimBaseVisualEffect();
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual CExoString GetAnimationName(unsigned short);
    virtual int SetAnimationInternal(CAurObject*, unsigned short, float, int, float);
};

#endif // CSWCANIMBASEVISUALEFFECT_H
