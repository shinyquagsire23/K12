#ifndef CAURORACOMPRESSEDTEXTURE_H
#define CAURORACOMPRESSEDTEXTURE_H

#include "CResHelper.h"
#include "resdds.h"

// Aurora's handle onto a .dds surface resource.
class CAuroraCompressedTexture : public CResHelper<CResDDS, 2033> {
public:
    CAuroraCompressedTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraCompressedTexture();  // the key function; body is empty
    // </VTABLE-GEN>

    // The fourth parameter is `unsigned int*`, not `unsigned char*`.
    int   GetCompressedTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                                     unsigned char* pBytesPerPixel, unsigned int* pDataSize,
                                     float* pAlphaTest);
    int   GetCompressedTextureDataSize();
    void* GetCompressedTextureDataPtr();
    int   Load();
    void  Unload();
    // Peeks the 20-byte DDS header through CRes::ReadRaw without demanding the resource.
    int   ReadTextureHeader(int* pnWidth, int* pnHeight, int* pnBytesPerPixel, int* pnDataSize,
                            float* pfAlphaTest);
    void* GetPtrToResHelperInfo();
};

#endif // CAURORACOMPRESSEDTEXTURE_H
