// Aurora's handle onto a .4pc (16-bit compressed texture) resource, a CResHelper wrapper.

#ifndef CAURORA16BITTEXTURE_H
#define CAURORA16BITTEXTURE_H

#include "CResHelper.h"
#include "res4pc.h"

class CAurora16BitTexture : public CResHelper<CRes4PC, 2059> {
public:
    CAurora16BitTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAurora16BitTexture();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetTextureAttrib(unsigned int* pnWidth, unsigned int* pnHeight, float* pfAlphaTest, int* pnFormat);
    void* GetTextureDataPtr();
    int   GetTextureDataSize();
    int   Load();
    void  Unload();
    int   ReadTextureHeader(int* pnWidth, int* pnHeight, float* pfAlphaTest, int* pnFormat);
    void* GetPtrToResHelperInfo();
};

#endif
