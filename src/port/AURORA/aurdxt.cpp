// The DXT/S3TC software decompressor and the 16-bit format swappers.
#include "k2stdint.h"
#include "aurdxt.h"

// A1R5G5B5 keeps its one alpha bit at 15; GL's packed 16-bit format (GL_UNSIGNED_SHORT_5_5_5_1)
// wants it at bit 0, so the whole 16-bit word rotates left by one.
void SwapA1R5G5B5toRGBA(void* pData, unsigned int nBytes)
{
    unsigned short* q = static_cast<unsigned short*>(pData);
    for (int n = 0; n < static_cast<int>(nBytes / 2); n++, q++)
        *q = static_cast<unsigned short>((*q << 1) | (*q >> 15));
}

// In A4R4G4B4, alpha occupies bits 15..12, so the rotate is by four.
void SwapA4R4G4B4toRGBA(void* pData, unsigned int nBytes)
{
    unsigned short* q = static_cast<unsigned short*>(pData);
    for (int n = 0; n < static_cast<int>(nBytes / 2); n++, q++)
        *q = static_cast<unsigned short>((*q << 4) | (*q >> 12));
}

// No swizzle needed: the 24-bit source already matches the destination layout, and the entry point
// is kept so all four formats present the same interface.
void SwapR8G8B8toRGB(void* pData, unsigned int nBytes)
{
    K2_UNUSED(pData);
    K2_UNUSED(nBytes);
}

// 32-bit elements rotated by one byte inside the word.
void SwapA8R8G8B8toRGBA(void* pData, unsigned int nBytes)
{
    unsigned int* q = static_cast<unsigned int*>(pData);
    for (int n = 0; n < static_cast<int>(nBytes / 4); n++, q++)
        *q = (*q << 8) | (*q >> 24);
}

// Expands a block's two endpoint colours into the four interpolation colours. Red is (c >> 11) << 3,
// and each blend goes through an unsigned short temporary. The blends are C0 * 2 + C1 for index 2
// and C0 + C1 * 2 for index 3, which divides by three through a reciprocal multiply.
void GetColorBlockColors(DXTColBlock* pBlock, Color8888* pColor0, Color8888* pColor1,
                         Color8888* pColor2, Color8888* pColor3, bool bDXT1)
{
    unsigned short c0 = pBlock->m_rgb[0];
    unsigned short c1 = pBlock->m_rgb[1];

    pColor0->r = static_cast<unsigned char>((c0 >> 11) << 3);
    pColor0->g = static_cast<unsigned char>((c0 >> 3) & 0xfc);
    pColor0->b = static_cast<unsigned char>(c0 << 3);
    pColor0->a = 0xff;

    pColor1->a = 0xff;
    pColor1->r = static_cast<unsigned char>((c1 >> 11) << 3);
    pColor1->g = static_cast<unsigned char>((c1 >> 3) & 0xfc);
    pColor1->b = static_cast<unsigned char>(c1 << 3);

    unsigned short wrd;
    if (c0 > c1 || !bDXT1)
    {
        wrd = static_cast<unsigned short>((pColor0->r * 2 + pColor1->r) / 3);
        pColor2->r = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->g * 2 + pColor1->g) / 3);
        pColor2->g = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->b * 2 + pColor1->b) / 3);
        pColor2->b = static_cast<unsigned char>(wrd);
        pColor2->a = 0xff;

        wrd = static_cast<unsigned short>((pColor0->r + pColor1->r * 2) / 3);
        pColor3->r = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->g + pColor1->g * 2) / 3);
        pColor3->g = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->b + pColor1->b * 2) / 3);
        pColor3->b = static_cast<unsigned char>(wrd);
        pColor3->a = 0xff;
    }
    else
    {
        wrd = static_cast<unsigned short>((pColor0->r + pColor1->r) / 2);
        pColor2->r = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->g + pColor1->g) / 2);
        pColor2->g = static_cast<unsigned char>(wrd);
        wrd = static_cast<unsigned short>((pColor0->b + pColor1->b) / 2);
        pColor2->b = static_cast<unsigned char>(wrd);
        pColor2->a = 0xff;

        pColor3->r = 40;
        pColor3->g = 96;
        pColor3->b = 40;
        pColor3->a = 0;
    }
}

// The two block-index decoders. The lookup tables are part of the format, and both decoders resolve
// the destination column's 2-bit field through them rather than shifting by a constant.
// The 16-bit destination pixels as bitfield structs, low field first. Only the fields a body
// assigns are written: the rest keep the previous block's bits.
struct Color4444
{
    unsigned short a : 4;
    unsigned short b : 4;
    unsigned short g : 4;
    unsigned short r : 4;
};

struct Color565
{
    unsigned short b : 5;
    unsigned short g : 6;
    unsigned short r : 5;
};

// The Color1555 struct's red field is one bit wide (an `r:1` typo), so a 1555 pixel keeps red's
// lowest bit at bit 11 and bits 12..15 are never initialised. K2_FIX_ENGINE_BUGS declares the
// intended 5-bit field.
struct Color1555
{
    unsigned short a : 1;
    unsigned short b : 5;
    unsigned short g : 5;
#ifdef K2_FIX_ENGINE_BUGS
    unsigned short r : 5;
#else
    unsigned short r : 1;
#endif
};

static const unsigned int aColorMask[4] = { 0x3, 0xC, 0x30, 0xC0 };
static const unsigned int aColorShift[4] = { 0, 2, 4, 6 };

// nX is the destination row stride in pixels and min(nX, 4) of them are written, so a partial
// right-edge block leaves the rest of the row alone. Both clamps are (n < 4) ? n : 4.
void DecodeColorBlock(unsigned int* pDst, DXTColBlock* pBlock, int nX, int nY,
                      unsigned int* pColor0, unsigned int* pColor1,
                      unsigned int* pColor2, unsigned int* pColor3)
{
    int nW = (nX < 4) ? nX : 4;
    if (nY < 1)
        return;
    int nH = (nY < 4) ? nY : 4;

    for (int y = 0; y < nH; y++)
    {
        for (int x = 0; x < nW; x++)
        {
            unsigned int nIndex = (pBlock->m_row[y] & aColorMask[x]) >> aColorShift[x];
            switch (nIndex)
            {
            case 0: *pDst = *pColor0; break;
            case 1: *pDst = *pColor1; break;
            case 2: *pDst = *pColor2; break;
            case 3: *pDst = *pColor3; break;
            }
            pDst++;
        }
        pDst += nX - nW;
    }
}

void DecodeColorBlock16(unsigned short* pDst, DXTColBlock* pBlock, int nX, int nY,
                        unsigned short* pColor0, unsigned short* pColor1,
                        unsigned short* pColor2, unsigned short* pColor3)
{
    int nW = (nX < 4) ? nX : 4;
    if (nY < 1)
        return;
    int nH = (nY < 4) ? nY : 4;

    for (int y = 0; y < nH; y++)
    {
        for (int x = 0; x < nW; x++)
        {
            unsigned int nIndex = (pBlock->m_row[y] & aColorMask[x]) >> aColorShift[x];
            switch (nIndex)
            {
            case 0: *pDst = *pColor0; break;
            case 1: *pDst = *pColor1; break;
            case 2: *pDst = *pColor2; break;
            case 3: *pDst = *pColor3; break;
            }
            pDst++;
        }
        pDst += nX - nW;
    }
}

// The 8-byte alpha block is copied to a local before the walk. The copy matters: the destination
// stores may alias the block, so the per-row read has to stay inside the inner loop. The alpha byte
// is the nibble replicated across the byte, i.e. nibble * 17.
void DecodeAlphaExplicit(unsigned int* pDst, DXTAlphaBlockExplicit* pBlock, int nX, int nY,
                         unsigned int nMask)
{
    DXTAlphaBlockExplicit aBlock = *pBlock;

    int nW = (nX < 4) ? nX : 4;
    if (nY < 1)
        return;
    int nH = (nY < 4) ? nY : 4;

    for (int y = 0; y < nH; y++)
    {
        unsigned short nBits = aBlock.m_row[y];
        for (int x = 0; x < nW; x++)
        {
            unsigned int nPixel = *pDst;
            unsigned int nAlpha = (nBits & 0xF) * 0x11;
            nBits >>= 4;
            *pDst = (nPixel & nMask) | (nAlpha << 24);
            pDst++;
        }
        pDst += nX - nW;
    }
}

// The 16-bit flavour puts a 4-bit alpha in the low nibble (the A4R4G4B4 layout), so the nibble needs
// no replication here. It walks the destination by pointer and then adds the nX - nW remainder,
// where its 32-bit sibling indexes the destination by column.
void DecodeAlphaExplicit16(unsigned short* pDst, DXTAlphaBlockExplicit* pBlock, int nX, int nY,
                           unsigned short nMask)
{
    DXTAlphaBlockExplicit aBlock = *pBlock;

    int nW = (nX < 4) ? nX : 4;
    if (nY < 1)
        return;
    int nH = (nY < 4) ? nY : 4;

    unsigned int nAlpha = 0;
    for (int y = 0; y < nH; y++)
    {
        unsigned short nBits = aBlock.m_row[y];
        for (int x = 0; x < nW; x++)
        {
            nAlpha = (nAlpha & ~0xF) | (nBits & 0xF);
            *pDst = static_cast<unsigned short>((*pDst & nMask) | nAlpha);
            nBits >>= 4;
            pDst++;
        }
        pDst += nX - nW;
    }
}

// DXT5's 3-bit alpha interpolator in the Nvidia DDS-loader shape: a 4x4 index block, an 8-entry
// palette built from the two endpoints, and a 4x4 block of pixel values, all zero-initialised. Each
// pixel is masked and merged, and each row closes with pDst += nX - nW.
void DecodeAlpha3BitLinear(unsigned int* pDst, DXTAlphaBlock3BitLinear* pBlock, int nX, int nY,
                           unsigned int nMask)
{
    unsigned char aIndex[4][4] = {{0}};
    unsigned short aPalette[8] = {0};
    Color8888 aValues[4][4] = {};

    aPalette[0] = pBlock->m_data[0];
    aPalette[1] = pBlock->m_data[1];

    if (aPalette[0] > aPalette[1])
    {
        aPalette[2] = (6 * aPalette[0] + 1 * aPalette[1]) / 7;
        aPalette[3] = (5 * aPalette[0] + 2 * aPalette[1]) / 7;
        aPalette[4] = (4 * aPalette[0] + 3 * aPalette[1]) / 7;
        aPalette[5] = (3 * aPalette[0] + 4 * aPalette[1]) / 7;
        aPalette[6] = (2 * aPalette[0] + 5 * aPalette[1]) / 7;
        aPalette[7] = (1 * aPalette[0] + 6 * aPalette[1]) / 7;
    }
    else
    {
        aPalette[2] = (4 * aPalette[0] + 1 * aPalette[1]) / 5;
        aPalette[3] = (3 * aPalette[0] + 2 * aPalette[1]) / 5;
        aPalette[4] = (2 * aPalette[0] + 3 * aPalette[1]) / 5;
        aPalette[5] = (1 * aPalette[0] + 4 * aPalette[1]) / 5;
        aPalette[6] = 0;
        aPalette[7] = 255;
    }

    unsigned int nBits = *reinterpret_cast<unsigned int*>(&pBlock->m_data[2]);  // raw-offset: DXT block bit fields
    unsigned int nBitsHi = *reinterpret_cast<unsigned short*>(&pBlock->m_data[6]);  // raw-offset: DXT block bit fields

    aIndex[0][0] = static_cast<unsigned char>((nBits >> 0) & 7);
    aIndex[0][1] = static_cast<unsigned char>((nBits >> 3) & 7);
    aIndex[0][2] = static_cast<unsigned char>((nBits >> 6) & 7);
    aIndex[0][3] = static_cast<unsigned char>((nBits >> 9) & 7);
    aIndex[1][0] = static_cast<unsigned char>((nBits >> 12) & 7);
    aIndex[1][1] = static_cast<unsigned char>((nBits >> 15) & 7);
    aIndex[1][2] = static_cast<unsigned char>((nBits >> 18) & 7);
    aIndex[1][3] = static_cast<unsigned char>((nBits >> 21) & 7);
    aIndex[2][0] = static_cast<unsigned char>((nBits >> 24) & 7);
    aIndex[2][1] = static_cast<unsigned char>((nBits >> 27) & 7);
    aIndex[2][2] = static_cast<unsigned char>(((nBits >> 30) | (nBitsHi << 2)) & 7);
    aIndex[2][3] = static_cast<unsigned char>((nBitsHi >> 1) & 7);
    aIndex[3][0] = static_cast<unsigned char>((nBitsHi >> 4) & 7);
    aIndex[3][1] = static_cast<unsigned char>((nBitsHi >> 7) & 7);
    aIndex[3][2] = static_cast<unsigned char>((nBitsHi >> 10) & 7);
    aIndex[3][3] = static_cast<unsigned char>((nBitsHi >> 13) & 7);

    int nH = (nY < 4) ? nY : 4;
    int nW = (nX < 4) ? nX : 4;

    for (int y = 0; y < nH; y++)
        for (int x = 0; x < nW; x++)
            aValues[y][x].a = static_cast<unsigned char>(aPalette[aIndex[y][x]]);

    for (int y = 0; y < nH; y++)
    {
        for (int x = 0; x < nW; x++)
        {
            *pDst &= nMask;
            *pDst |= *reinterpret_cast<unsigned int*>(&aValues[y][x]);
            pDst++;
        }
        pDst += nX - nW;
    }
}

// The row loop does not advance through the image: the pixel pointer is re-derived from pData + 1 at
// the top of every row, so the same nWidth pixels are rescanned nHeight times. The loop also tests
// its counter before decrementing it.
void UnPremultiplyAlpha_8888(unsigned char* pData, int nWidth, int nHeight)
{
    if (nHeight == 0)
        return;

    int nRows = nHeight - 1;
    do
    {
        int n = nWidth;
        unsigned char* p = pData + 1;
        while (n != 0)
        {
            unsigned int nAlpha = p[2];
            n--;
            if (nAlpha != 0)
            {
                p[-1] = static_cast<unsigned char>((static_cast<unsigned int>(p[-1]) << 8) / nAlpha);
                p[0] = static_cast<unsigned char>((static_cast<unsigned int>(p[0]) << 8) / nAlpha);
                p[1] = static_cast<unsigned char>((static_cast<unsigned int>(p[1]) << 8) / nAlpha);
            }
            p += 4;
        }
    } while (nRows--);
}

// The switch on the four-character format tag. DXT4 has no case of its own: it falls to the same
// return 0 as the default.
unsigned char* DecompressDXTnTo8888(unsigned int nFormat, unsigned char* pData, unsigned int nA,
                                   unsigned int nB, unsigned int nC)
{
    switch (nFormat)
    {
    case 'D' | ('X' << 8) | ('T' << 16) | ('1' << 24):
        return DecompressDXT1_8888(pData, nA, nB, nC);
    case 'D' | ('X' << 8) | ('T' << 16) | ('2' << 24):
        return DecompressDXT2_8888(pData, nA, nB, nC);
    case 'D' | ('X' << 8) | ('T' << 16) | ('3' << 24):
        return DecompressDXT3_8888(pData, nA, nB, nC);
    case 'D' | ('X' << 8) | ('T' << 16) | ('5' << 24):
        return DecompressDXT5_8888(pData, nA, nB, nC);
    }
    return 0;
}

// Per block, GetColorBlockColors then DecodeColorBlock run on four Color8888 locals. The
// destination is nWidth * nHeight * 4 bytes with a row stride of nWidth * 16, four times what a row
// holds; that overrun is an engine bug, left as it is.
unsigned char* DecompressDXT1_8888(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 4];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 8 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;
    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++, pBlock++)
        {
            unsigned int* pDst = reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(pBuf) + x * 16 + y * 4 * nWidth * 4);
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, true);
            DecodeColorBlock(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned int*>(&col0),
                             reinterpret_cast<unsigned int*>(&col1), reinterpret_cast<unsigned int*>(&col2), reinterpret_cast<unsigned int*>(&col3));
        }
    }

    return pBuf;
}

// Decodes DXT3 in the Nvidia DDS-loader shape into a 16-bit A4R4G4B4 destination: per 16-byte
// block, the explicit alpha block at +0 and the colour block at +8; GetColorBlockColors with the
// four-colour form, each Color8888 narrowed into a Color4444 bitfield struct, DecodeColorBlock16,
// then DecodeAlphaExplicit16 with the keep-mask built from c16_0 (a = 0, r = g = b = 0xf, Nvidia's
// alpha-zero idiom). Only r/g/b are assigned per block, so each struct's alpha nibble carries over
// from the previous block; the alpha pass masks it off, so the output is the same. Destination row
// stride is nWidth * 8 bytes.
unsigned char* DecompressDXT3_4444(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 2];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 16 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;
    Color4444 c16_0, c16_1, c16_2, c16_3;

    // The alpha decoder's keep-mask is a Color4444 with a = 0 and r = g = b = 0xf.
    c16_0.a = 0;
    c16_0.r = c16_0.g = c16_0.b = 0xf;
    unsigned short nAlphaZero = *reinterpret_cast<unsigned short*>(&c16_0);

    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++)
        {
            DXTAlphaBlockExplicit* pAlphaBlock = reinterpret_cast<DXTAlphaBlockExplicit*>(pBlock);
            pBlock++;
            unsigned short* pDst = reinterpret_cast<unsigned short*>(reinterpret_cast<uintptr_t>(pBuf) + x * 8 + y * 4 * nWidth * 2);
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, false);
            c16_0.r = col0.r >> 4;
            c16_0.g = col0.g >> 4;
            c16_0.b = col0.b >> 4;
            c16_1.r = col1.r >> 4;
            c16_1.g = col1.g >> 4;
            c16_1.b = col1.b >> 4;
            c16_2.r = col2.r >> 4;
            c16_2.g = col2.g >> 4;
            c16_2.b = col2.b >> 4;
            c16_3.r = col3.r >> 4;
            c16_3.g = col3.g >> 4;
            c16_3.b = col3.b >> 4;
            DecodeColorBlock16(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned short*>(&c16_0),
                               reinterpret_cast<unsigned short*>(&c16_1), reinterpret_cast<unsigned short*>(&c16_2),
                               reinterpret_cast<unsigned short*>(&c16_3));
            DecodeAlphaExplicit16(pDst, pAlphaBlock, nWidth, nHeight, nAlphaZero);
            pBlock++;
        }
    }

    return pBuf;
}

// Decodes DXT1 in the Nvidia DDS-loader shape into a 16-bit A1R5G5B5 destination:
// GetColorBlockColors in the three-colour form, each Color8888 narrowed into a Color1555 bitfield
// struct (alpha = a >> 7, so the three-colour arm's index 3 is transparent), then
// DecodeColorBlock16. The Color1555 red-field typo above applies here too: red keeps only its
// lowest bit and bits 12..15 are uninitialised.
unsigned char* DecompressDXT1_1555(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 2];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 8 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;
    Color1555 c16_0, c16_1, c16_2, c16_3;
    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++, pBlock++)
        {
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, true);
            unsigned short* pDst = reinterpret_cast<unsigned short*>(reinterpret_cast<uintptr_t>(pBuf) + x * 8 + y * 4 * nWidth * 2);
            c16_0.r = col0.r >> 3;
            c16_0.g = col0.g >> 3;
            c16_0.b = col0.b >> 3;
            c16_0.a = col0.a >> 7;
            c16_1.r = col1.r >> 3;
            c16_1.g = col1.g >> 3;
            c16_1.b = col1.b >> 3;
            c16_1.a = col1.a >> 7;
            c16_2.r = col2.r >> 3;
            c16_2.g = col2.g >> 3;
            c16_2.b = col2.b >> 3;
            c16_2.a = col2.a >> 7;
            c16_3.r = col3.r >> 3;
            c16_3.g = col3.g >> 3;
            c16_3.b = col3.b >> 3;
            c16_3.a = col3.a >> 7;
            DecodeColorBlock16(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned short*>(&c16_0),
                               reinterpret_cast<unsigned short*>(&c16_1), reinterpret_cast<unsigned short*>(&c16_2),
                               reinterpret_cast<unsigned short*>(&c16_3));
        }
    }

    return pBuf;
}

// Decodes DXT1 in the Nvidia DDS-loader shape into a 16-bit RGB565 destination: GetColorBlockColors
// in the three-colour form, each Color8888 narrowed into a Color565 bitfield struct, then
// DecodeColorBlock16. The fourth parameter is never read: the block row is pSrc + y * nBlocksW * 8.
unsigned char* DecompressDXT1_565(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                  unsigned int nSrcStride)
{
    K2_UNUSED(nSrcStride);
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 2];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;
    Color565 c16_0, c16_1, c16_2, c16_3;
    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + y * nBlocksW * 8);
        for (unsigned int x = 0; x < nBlocksW; x++, pBlock++)
        {
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, true);
            unsigned short* pDst = reinterpret_cast<unsigned short*>(reinterpret_cast<uintptr_t>(pBuf) + x * 8 + y * 4 * nWidth * 2);
            c16_0.r = col0.r >> 3;
            c16_0.g = col0.g >> 2;
            c16_0.b = col0.b >> 3;
            c16_1.r = col1.r >> 3;
            c16_1.g = col1.g >> 2;
            c16_1.b = col1.b >> 3;
            c16_2.r = col2.r >> 3;
            c16_2.g = col2.g >> 2;
            c16_2.b = col2.b >> 3;
            c16_3.r = col3.r >> 3;
            c16_3.g = col3.g >> 2;
            c16_3.b = col3.b >> 3;
            DecodeColorBlock16(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned short*>(&c16_0),
                               reinterpret_cast<unsigned short*>(&c16_1), reinterpret_cast<unsigned short*>(&c16_2),
                               reinterpret_cast<unsigned short*>(&c16_3));
        }
    }

    return pBuf;
}

// Decodes DXT3 in the Nvidia DDS-loader shape: per 16-byte block, the explicit alpha block at +0
// and the colour block at +8; GetColorBlockColors in the four-colour form, DecodeColorBlock, then
// DecodeAlphaExplicit with Nvidia's alpha-zero keep-mask (alpha cleared, colour kept). All three
// helpers live in this file.
unsigned char* DecompressDXT3_8888(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 4];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 16 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;

    // The alpha decoder's keep-mask is a Color8888 with the alpha byte cleared.
    col0.a = 0;
    col0.r = col0.g = col0.b = 0xff;
    unsigned int nAlphaZero = *reinterpret_cast<unsigned int*>(&col0);

    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++)
        {
            DXTAlphaBlockExplicit* pAlphaBlock = reinterpret_cast<DXTAlphaBlockExplicit*>(pBlock);
            pBlock++;
            unsigned int* pDst = reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(pBuf) + x * 16 + y * 4 * nWidth * 4);
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, false);
            DecodeColorBlock(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned int*>(&col0),
                             reinterpret_cast<unsigned int*>(&col1), reinterpret_cast<unsigned int*>(&col2), reinterpret_cast<unsigned int*>(&col3));
            DecodeAlphaExplicit(pDst, pAlphaBlock, nWidth, nHeight, nAlphaZero);
            pBlock++;
        }
    }

    return pBuf;
}
// DecompressDXT3_8888 plus UnPremultiplyAlpha_8888 over the whole buffer. The alpha pass carries the
// pData + 1 rescan above.
unsigned char* DecompressDXT2_8888(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 4];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 16 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;

    // The alpha decoder's keep-mask is a Color8888 with the alpha byte cleared.
    col0.a = 0;
    col0.r = col0.g = col0.b = 0xff;
    unsigned int nAlphaZero = *reinterpret_cast<unsigned int*>(&col0);

    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++)
        {
            DXTAlphaBlockExplicit* pAlphaBlock = reinterpret_cast<DXTAlphaBlockExplicit*>(pBlock);
            pBlock++;
            unsigned int* pDst = reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(pBuf) + x * 16 + y * 4 * nWidth * 4);
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, false);
            DecodeColorBlock(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned int*>(&col0),
                             reinterpret_cast<unsigned int*>(&col1), reinterpret_cast<unsigned int*>(&col2), reinterpret_cast<unsigned int*>(&col3));
            DecodeAlphaExplicit(pDst, pAlphaBlock, nWidth, nHeight, nAlphaZero);
            pBlock++;
        }
    }

    UnPremultiplyAlpha_8888(pBuf, nWidth, nHeight);

    return pBuf;
}

// DecompressDXT3_8888 with the alpha pass replaced by a call to DecodeAlpha3BitLinear. It runs even
// when nHeight < 1, where DecodeColorBlock returns early. The keep-mask is the same alpha-zero one.
unsigned char* DecompressDXT5_8888(unsigned char* pSrc, unsigned int nWidth, unsigned int nHeight,
                                   unsigned int nSrcStride)
{
    unsigned char* pBuf = new unsigned char[nWidth * nHeight * 4];

    unsigned int nBlocksW = nWidth >> 2;
    if (nBlocksW == 0)
        nBlocksW = 1;
    unsigned int nStride = (nSrcStride == 0) ? nBlocksW * 16 : nSrcStride;

    unsigned int nBlocksH = nHeight >> 2;
    if (nBlocksH == 0)
        nBlocksH = 1;

    Color8888 col0, col1, col2, col3;

    // The alpha decoder's keep-mask is a Color8888 with the alpha byte cleared.
    col0.a = 0;
    col0.r = col0.g = col0.b = 0xff;
    unsigned int nAlphaZero = *reinterpret_cast<unsigned int*>(&col0);

    for (unsigned int y = 0; y < nBlocksH; y++)
    {
        DXTColBlock* pBlock = reinterpret_cast<DXTColBlock*>(reinterpret_cast<uintptr_t>(pSrc) + nStride * y);
        for (unsigned int x = 0; x < nBlocksW; x++)
        {
            DXTAlphaBlock3BitLinear* pAlphaBlock = reinterpret_cast<DXTAlphaBlock3BitLinear*>(pBlock);
            pBlock++;
            unsigned int* pDst = reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(pBuf) + x * 16 + y * 4 * nWidth * 4);
            GetColorBlockColors(pBlock, &col0, &col1, &col2, &col3, false);
            DecodeColorBlock(pDst, pBlock, nWidth, nHeight, reinterpret_cast<unsigned int*>(&col0),
                             reinterpret_cast<unsigned int*>(&col1), reinterpret_cast<unsigned int*>(&col2), reinterpret_cast<unsigned int*>(&col3));
            DecodeAlpha3BitLinear(pDst, pAlphaBlock, nWidth, nHeight, nAlphaZero);
            pBlock++;
        }
    }

    return pBuf;
}
