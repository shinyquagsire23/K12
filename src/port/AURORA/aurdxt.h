#ifndef AURDXT_H
#define AURDXT_H

struct DXTColBlock
{
    unsigned short m_rgb[2];
    unsigned char m_row[4];
};

struct Color8888
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

// The second parameter is a byte count, not a pixel count.
void SwapA1R5G5B5toRGBA(void* pData, unsigned int nBytes);
void SwapA4R4G4B4toRGBA(void* pData, unsigned int nBytes);
// Empty: this format's destination layout already matches.
void SwapR8G8B8toRGB(void* pData, unsigned int nBytes);
void SwapA8R8G8B8toRGBA(void* pData, unsigned int nBytes);

// Four 16-bit rows of four 4-bit alphas.
struct DXTAlphaBlockExplicit
{
    unsigned short m_row[4];
};

// DXT5 alpha block: two endpoint bytes, then six bytes of 3-bit interpolation indices.
struct DXTAlphaBlock3BitLinear
{
    unsigned char m_data[8];
};

void DecodeColorBlock(unsigned int* pDst, DXTColBlock* pBlock, int nX, int nY,
                      unsigned int* pColor0, unsigned int* pColor1,
                      unsigned int* pColor2, unsigned int* pColor3);
void DecodeColorBlock16(unsigned short* pDst, DXTColBlock* pBlock, int nX, int nY,
                        unsigned short* pColor0, unsigned short* pColor1,
                        unsigned short* pColor2, unsigned short* pColor3);
void DecodeAlphaExplicit(unsigned int* pDst, DXTAlphaBlockExplicit* pBlock, int nX, int nY,
                         unsigned int nMask);
void DecodeAlphaExplicit16(unsigned short* pDst, DXTAlphaBlockExplicit* pBlock, int nX, int nY,
                           unsigned short nMask);
void DecodeAlpha3BitLinear(unsigned int* pDst, DXTAlphaBlock3BitLinear* pBlock, int nX, int nY,
                           unsigned int nMask);
void UnPremultiplyAlpha_8888(unsigned char* pData, int nWidth, int nHeight);

// Expands one colour block's two RGB565 endpoints into four colors. bDXT1 selects punch-through:
// when the endpoints are ordered c0 <= c1, index 2 is the 50/50 average with a = 255 and index 3
// is the (40, 96, 40, 0) sentinel; otherwise index 2 is (2*C0 + C1)/3 and index 3 is
// (C0 + 2*C1)/3, both with a = 255.
void GetColorBlockColors(DXTColBlock* pBlock, Color8888* pColor0, Color8888* pColor1,
                         Color8888* pColor2, Color8888* pColor3, bool bDXT1);

// nFormat is a 'DXTn' four-character-code tag.
unsigned char* DecompressDXTnTo8888(unsigned int nFormat, unsigned char* pData, unsigned int nA,
                                    unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT1_8888(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT2_8888(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT3_8888(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT5_8888(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT1_1555(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
// The fourth parameter is declared and never read.
unsigned char* DecompressDXT1_565(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);
unsigned char* DecompressDXT3_4444(unsigned char* pData, unsigned int nA, unsigned int nB, unsigned int nC);

#endif // AURDXT_H
