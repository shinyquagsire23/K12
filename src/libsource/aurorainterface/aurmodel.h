// Aurora's handle onto a .mdl model resource, a CResHelper wrapper.

#ifndef CAURORAMODEL_H
#define CAURORAMODEL_H

#include "CResHelper.h"
#include "resmdl.h"

class CAuroraModel : public CResHelper<CResMDL, 2002> {
public:
    CAuroraModel(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraModel();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetModelDataSize();
    void* GetModelDataPtr();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif
