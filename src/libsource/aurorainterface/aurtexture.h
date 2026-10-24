#pragma once

#ifndef CAURORATEXTURE_H
#define CAURORATEXTURE_H

// Handle onto a .tga (Targa) texture resource, one of the CResHelper<> wrappers.

#include "CResHelper.h"
#include "restga.h"

class CAuroraTexture : public CResHelper<CResTGA, 3> {
public:
    CAuroraTexture(CResRef resRef, int bRequest);

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class adds no slots but 0/1 */
    virtual ~CAuroraTexture();
    // </VTABLE-GEN>

    int   GetTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                           unsigned char* pBitsPerPixel);
    void* GetTextureDataPtr();
    int   GetTextureDataSize();
    int   Load();
    void  Unload();
    int   ReadTextureHeader(int* pnWidth, int* pnHeight, int* pnBytesPerPixel);
    void* GetPtrToResHelperInfo();
};

#endif // CAURORATEXTURE_H

// The free-function Aurora texture API.

#include "aurtexman.h"

class CAurTextureBasic;

void BinaryInsertTexture(CAurTextureBasic* pTexture, int nLow, int nHigh);
void InsertTexture(CAurTextureBasic* pTexture);

CAurTextureBasic* BinarySearchTexture(char* pszName, int nLow, int nHigh);
CAurTextureBasic* BinaryFindTexture(char* pszName);

void GetNewDimensions(int* panDimensions);

void InsertionSortTexture(CAurTextureBasic** papTexture, int nCount);

// Rotates an nBPP-byte-per-pixel image by nQuarter right angles into a fresh buffer, which the
// caller owns; the source image is not touched.
unsigned char* ImageQuarterRotated(unsigned char* pImage, int nWidth, int nHeight, int nBPP,
                                   int nQuarter);

struct TEXTUREMEMORYSTATS;

void AurTextureCalculateNewTexMem(List<CAurTextureBasic*>& listTexture,
                                  TEXTUREMEMORYSTATS* pStats);

void AurTextureMemCheck(List<CAurTextureBasic*>& listTexture, int nMaxSize,
                        TEXTUREMEMORYSTATS* pStats, float fRatio);

// The live TID index, 0 when there is no texture.
int  AurTextureGetTID(CAurTexture* pTexture);
// Deletes only a texture nothing is watching.
void AurTextureRelease(CAurTexture* pTexture);

// The palette state and the TID binder. The raster loaders for this block live in
// aurtexload/aurtexload.h.

class TextureController;
namespace Aurora { enum AuroraTextureFormats : int; }

// Drops the palette image and the height that tracks it.
void AurClearPalette();

// Loads pszName as palette slot nIndex.
int AurSetPalette(int nIndex, char* pszName);

TextureController* AurTextureGetController(CAurTexture* pTexture);

// Makes nTID the live texture binding. bCubeMap asks for the cube-map target (honoured only when
// the device has cube maps), bCubeStrip for the 3D/6:1-strip target.
#if KOTOR1
void AurTextureBindTID(unsigned int nTID, bool bCubeMap);  // K1 has no 3D strip
inline void AurTextureBindTID(unsigned int nTID, bool bCubeMap, bool) { AurTextureBindTID(nTID, bCubeMap); }
#else
void AurTextureBindTID(unsigned int nTID, bool bCubeMap, bool bCubeStrip);
#endif
