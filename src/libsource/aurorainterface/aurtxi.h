#ifndef CAURORATXI_H
#define CAURORATXI_H

// The Aurora renderer's handle onto a .txi texture-info resource, a CResHelper wrapper that adds
// nothing but its own destructor.

#include "CResHelper.h"
#include "restxi.h"

class CAuroraTXI : public CResHelper<CResTXI, 2022> {
public:
    CAuroraTXI(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraTXI();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetTXIDataSize();
    void* GetTXIDataPtr();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif // CAURORATXI_H
