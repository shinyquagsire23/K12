#ifndef CSWCANIMBASEPLACEABLE_H
#define CSWCANIMBASEPLACEABLE_H

#include "nwcanimbase.h"

class CResRef;

class CSWCAnimBasePlaceable : public CSWCAnimBase {
public:

    // Declared, never defined here: callers create one with new and the body lives in another TU.
    // Leaving it implicit would make new T() value-initialize the whole object.
    CSWCAnimBasePlaceable();

    virtual ~CSWCAnimBasePlaceable();
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual CExoString GetAnimationName(unsigned short);
    virtual int SetAnimation(unsigned short, float, int, float);
    virtual unsigned short ServerToClientAnimation(unsigned short);

    // Forwards to the inherited aurora model.
    void EnableAlwaysRender();
};

#endif // CSWCANIMBASEPLACEABLE_H
