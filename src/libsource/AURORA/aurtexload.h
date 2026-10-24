#pragma once

// The raster loader and releaser wrappers, kept in their own translation unit rather than in
// aurtexture.cpp.

namespace Aurora { enum AuroraTextureFormats : int; }

// Loads `pszFile` into a raster, trying the processed, S3TC, 16-bit and raw readers in that order
// and reporting the format through `pFormat`. `pnNumMipMaps` is a short and the codes are signed
// bytes, the two out-params the processed reader writes.
//
// Declared `void*` to match CAurTextureBasic.cpp's own local declaration of the same symbol, whose
// call sites assign straight into `void*` members; the body casts.
void* AurGetImageWrapper(char* pszFile, int* pnWidth, int* pnHeight, int* pnBPP,
                         int* pnCompressedSize, int* pnRasterSize, int* pnTxiSize,
                         Aurora::AuroraTextureFormats* pFormat, float* pfAlphaMean,
                         bool* pbProcessed, short* pnNumMipMaps, int nFlags);

// Reads only the header, through the same readers, with no raster allocated and nothing released.
// `pnUnknown` is an int* already zeroed before the call that the S3TC reader also writes.
int AurGetImageHeaderWrapper(char* pszFile, int* pnWidth, int* pnHeight, int* pnBPP,
                             int* pnUnknown, Aurora::AuroraTextureFormats* pFormat,
                             float* pfAlphaMean, int nFlags);

// Loads a layered (.plt) raster through a thin front for the `AurGetLayeredRaster` pointer that
// reports the layer count as a constant 2 (the reader's own count out-param is discarded).
unsigned char* AurGetLayeredRasterWrapper(char* pszFile, int* pnWidth, int* pnHeight,
                                          int* pnNumLayers);

// Releases a raster the loader handed out. `pbProcessed` says the image came from one of the three
// reader paths rather than from a plain allocation; the last two flags select the layered releaser
// over the raster one.
void AurTextureFreeImage(unsigned char* pImage, int nWidth, int nHeight, int nBPP,
                         int nCompressedSize, int nRasterSize, int nTxiSize,
                         bool* pbProcessed, bool bCompressed, bool bLayered);
