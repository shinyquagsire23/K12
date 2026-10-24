#ifndef CAURORAPROCESSEDTEXTURE_H
#define CAURORAPROCESSEDTEXTURE_H

#include "CResHelper.h"
#include "restpc.h"

// The renderer's handle onto a .tpc (BioWare processed texture) resource, which holds mip-mapped,
// optionally DXT-compressed pixels plus a trailing TXI block.
class CAuroraProcessedTexture : public CResHelper<CResTPC, 3007> {
public:
    CAuroraProcessedTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    virtual ~CAuroraProcessedTexture();
    // </VTABLE-GEN>

    // The parameter list mirrors CResTPC::GetTPCAttrib, which this forwards to.
    int   GetProcessedTextureAttrib(int* pWidth, int* pHeight, char* pFormat, int* pnCompressedSize,
                                    int* pnUncompressedSize, int* pTXISize, float* pAlphaTest,
                                    char* pMipCount);
    void* GetProcessedTextureDataPtr();
    int   GetProcessedTextureDataSize();
    void* GetProcessedTextureTXIPtr();
    int   GetProcessedTextureTXISize();
    int   Load();
    void  Unload();
    int   ReadProcessedTextureHeader(int* pWidth, int* pHeight, char* pFormat, int* pnCompressedSize,
                                     int* pnUncompressedSize, float* pAlphaTest,
                                     char* pMipCount);
    void* GetPtrToResHelperInfo();
    int   GetTextureSize();
};

#endif // CAURORAPROCESSEDTEXTURE_H
