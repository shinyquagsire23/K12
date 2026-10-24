// Client-side animation object for an area trigger. No extra fields yet.
#ifndef CSWCANIMBASETRIGGER_H
#define CSWCANIMBASETRIGGER_H

#include "nwcanimbase.h"

class CResRef;

class CSWCAnimBaseTrigger : public CSWCAnimBase {
public:

    // Declared but not defined here: CSWCTrigger::LoadModel constructs this type with
    // `new CSWCAnimBaseTrigger()` and needs a real constructor call rather than a
    // value-initializing default.
    CSWCAnimBaseTrigger();

    virtual ~CSWCAnimBaseTrigger();
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual CExoString GetAnimationName(unsigned short);
    virtual unsigned short ServerToClientAnimation(unsigned short);
};

#endif // CSWCANIMBASETRIGGER_H
