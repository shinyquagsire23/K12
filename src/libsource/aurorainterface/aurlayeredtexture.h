#ifndef CAURORALAYEREDTEXTURE_H
#define CAURORALAYEREDTEXTURE_H

#include "CResHelper.h"
#include "resplt.h"

// Aurora's handle onto a .plt (palette-layered texture) resource, a CResHelper<> wrapper.
//
// A .plt is 2 bytes per pixel and keeps its res-helper info block after the pixel data, at
// GetTextureDataPtr() + nWidth * nHeight * 2 + 4, which is why there is no GetTextureDataSize()
// accessor and why finding the block needs GetTextureAttrib first.
class CAuroraLayeredTexture : public CResHelper<CResPLT, 6> {
public:
    CAuroraLayeredTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraLayeredTexture();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                           unsigned int* pNumLayers);
    void* GetTextureDataPtr();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif // CAURORALAYEREDTEXTURE_H
