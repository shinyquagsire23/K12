#ifndef CAURORAMODELX_H
#define CAURORAMODELX_H

// The renderer's handle onto a .mdx model mesh-data resource, a CResHelper wrapper that adds no
// data members and no vtable slots beyond the destructor pair.

#include "CResHelper.h"
#include "resmdx.h"

class CAuroraModelX : public CResHelper<CResMDX, 3008> {
public:
    CAuroraModelX(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraModelX();  // slots 0-1 (D2+D0)
    // </VTABLE-GEN>

    int   GetModelDataSize();
    void* GetModelDataPtr();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif // CAURORAMODELX_H
