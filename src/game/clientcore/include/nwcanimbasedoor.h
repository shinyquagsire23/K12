// Client-side animation object for a door. No extra fields yet.
#ifndef CSWCANIMBASEDOOR_H
#define CSWCANIMBASEDOOR_H

#include "nwcanimbase.h"

class CAurObject;
class CExoString;
class CResRef;

class CSWCAnimBaseDoor : public CSWCAnimBase {
public:

    // Declared but not defined here: CSWCDoor::LoadModel constructs this type with
    // `new CSWCAnimBaseDoor()` and needs a real constructor call rather than a
    // value-initializing default.
    CSWCAnimBaseDoor();

    virtual ~CSWCAnimBaseDoor();
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual CExoString GetAnimationName(unsigned short);
    virtual unsigned short ServerToClientAnimation(unsigned short);
    virtual int SetAnimationInternal(CAurObject*, unsigned short, float, int, float);
};

#endif // CSWCANIMBASEDOOR_H
