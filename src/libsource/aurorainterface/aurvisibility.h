// Aurora's handle onto a room-visibility (.vis) resource, one of the ten sibling CResHelper wrappers
// that add a Load/Unload pair and size/pointer accessors and carry no data members.

#ifndef CAURORAVISIBILITY_H
#define CAURORAVISIBILITY_H

#include "CResHelper.h"
#include "resvis.h"

class CAuroraVisibility : public CResHelper<CResVIS, 3001> {
public:
    CAuroraVisibility(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraVisibility();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetVisibilityDataSize();
    void* GetVisibilityDataPtr();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif
