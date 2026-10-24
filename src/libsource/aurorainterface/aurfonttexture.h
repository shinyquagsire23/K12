#ifndef CAURORAFONTTEXTURE_H
#define CAURORAFONTTEXTURE_H

// The Aurora renderer's handle onto a .fnt (bitmap font) resource.
//
// A .fnt carries two payloads, the glyph table and the bitmap the glyphs index into, so the wrapper
// exposes each one separately. Its naming maps onto CResFNT as:
//
//     GetFontDataPtr()          -> CResFNT::GetFNTHeaderPtr()   (the header, not GetFNTDataPtr)
//     GetFontDataSize()         -> CResFNT::GetFNTSize()
//     CopyFontData()            -> CResFNT::CopyFNTData()
//     GetFontTextureDataPtr()   -> CResFNT::GetTGADataPtr()
//     GetFontTextureDataSize()  -> CResFNT::GetTGASize()
//     CopyFontTextureData()     -> CResFNT::CopyTGAData()
//     GetFontTextureAttrib()    -> CResFNT::GetTGAAttrib()
//
// The res-helper info block is indexed off the glyph table rather than measured off a pixel buffer:
// it sits one glyph-record past an array whose count is the int at the front of the FNT header.
//
// The class carries no data members and no vtable slots beyond the destructor pair.

#include "CResHelper.h"
#include "resfnt.h"

class CAuroraFontTexture : public CResHelper<CResFNT, 2005> {
public:
    CAuroraFontTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    //   primary vtable, 2 slots
    virtual ~CAuroraFontTexture();  // slots 0-1 (D2+D0) -- THE KEY FUNCTION, body is empty
    // </VTABLE-GEN>

    int   GetFontTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                               unsigned char* pBitsPerPixel);
    int   CopyFontData(unsigned char* pDst);
    int   CopyFontTextureData(unsigned char* pDst);
    void* GetFontDataPtr();
    void* GetFontTextureDataPtr();
    int   GetFontDataSize();
    int   GetFontTextureDataSize();
    int   Load();
    void  Unload();
    void* GetPtrToResHelperInfo();
};

#endif // CAURORAFONTTEXTURE_H
