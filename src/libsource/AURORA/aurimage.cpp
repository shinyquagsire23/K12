// The image module, with CPU-side pixel conversion and filtering, the TGA, S3TC and PLT container
// readers and writers, and the Mandelbrot/Julia fractals.

#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library uses the larger List header
#ifdef K2_HOST_BUILD
#include <string>
#include <algorithm>
#endif
// K1's resource loaders below include <string>, which brings bionic's <string.h>.
#include <string.h>
#endif
#include "aurimage.h"
#include "Vector.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"

int g_nImagesDownSampled;


struct TiffHeader {
    unsigned char  m_nIDLength;
    unsigned char  m_nColorMapType;
    unsigned char  m_nImageType;
    unsigned char  m_nCMapStart1;
    unsigned char  m_nCMapStart2;
    unsigned char  m_nCMapLength1;
    unsigned char  m_nCMapLength2;
    unsigned short m_nXOffset;
    unsigned short m_nYOffset;
    unsigned short m_nWidth;
    unsigned short m_nHeight;
    unsigned char  m_nPixelDepth;
    unsigned char  m_nImageDescriptor;
};

void print(TiffHeader& header)
{
    printf(" %4d IDLength \n", static_cast<int>(header.m_nIDLength));
    printf(" %4d ColorMapType \n", static_cast<int>(header.m_nColorMapType));
    printf(" %4d ImageType \n", static_cast<int>(header.m_nImageType));
    printf(" %4d CMapStart1 \n", static_cast<int>(header.m_nCMapStart1));
    printf(" %4d CMapStart2 \n", static_cast<int>(header.m_nCMapStart2));
    printf(" %4d CMapLength1 \n", static_cast<int>(header.m_nCMapLength1));
    printf(" %4d CMapLength2 \n", static_cast<int>(header.m_nCMapLength2));
    printf(" %4d XOffset \n", static_cast<int>(header.m_nXOffset));
    printf(" %4d YOffset \n", static_cast<int>(header.m_nYOffset));
    printf(" %4d Width \n", static_cast<int>(header.m_nWidth));
    printf(" %4d Height \n", static_cast<int>(header.m_nHeight));
    printf(" %4d PixelDepth \n", static_cast<int>(header.m_nPixelDepth));
    printf(" %4d ImageDescriptor \n", static_cast<int>(header.m_nImageDescriptor));
    putchar(10);
}

void ImageFlipY(unsigned char* pData, int nWidth, int nHeight, int nBpp)
{
    int nSize = nWidth * nHeight * nBpp;
    // The sizes are image and window dimensions, never negative.
    K2_ASSUME(nSize == 0 || (nWidth > 0 && nHeight > 0 && nBpp > 0));
    unsigned char* pTemp = new unsigned char[nSize];
    unsigned char* pSrc = pData;
    int nDstRow = nWidth * (nHeight - 1);
    int64_t nRowStride = static_cast<int64_t>(nWidth) * nBpp;
    for (int64_t y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            memcpy(pTemp + (nDstRow + x) * nBpp, pSrc + x * nBpp, nBpp);  // idiom-exempt: runtime length
        }
        pSrc += nRowStride;
        nDstRow -= nWidth;
    }
    memcpy(pData, pTemp, nSize);  // idiom-exempt: runtime length
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 releases the new[] buffer with scalar delete (mismatched, as in ImageGetSubImage)
    delete pTemp;
#else
    delete[] pTemp;
#endif
}

__attribute__((no_stack_protector))
void ImageFlipRB(unsigned char* pData, int nWidth, int nHeight, int nBpp)
{
    if (nBpp < 3)
        return;

    int nTotal = nWidth * nHeight * nBpp;
    for (int i = 0; i < nTotal; i += nBpp)
    {
        unsigned char ch = pData[i];
        pData[i] = pData[i + 2];
        pData[i + 2] = ch;
    }
}

void ImageCopy(unsigned char* pDst, const unsigned char* pSrc, int nWidth, int nHeight, int nBpp)
{
    memcpy(pDst, pSrc, nWidth * nHeight * nBpp);  // idiom-exempt: runtime length
}

void ImageFree(void* pData, int nWidth, int nHeight, int nBpp)
{
    K2_UNUSED(nWidth); K2_UNUSED(nHeight); K2_UNUSED(nBpp);
    delete[] static_cast<unsigned char*>(pData);
}

int ImageGetS3TCSize(int nWidth, int nHeight, int nFormat)
{
    int nBlockSize = (nFormat == 4) ? 16 : 8;
    int nSize = 0;
    while (nWidth | nHeight)
    {
        nWidth = nWidth ? nWidth : 1;
        nHeight = nHeight ? nHeight : 1;
        nSize += ((nWidth + 3) / 4) * ((nHeight + 3) / 4) * nBlockSize;
        nWidth >>= 1;
        nHeight >>= 1;
    }
    return nSize;
}

unsigned char* ImageDup(unsigned char* pData, int nWidth, int nHeight, int nFormat, int bCompressed)
{
    int nSize = bCompressed ? ImageGetS3TCSize(nWidth, nHeight, nFormat)
                            : nWidth * nHeight * nFormat;
    unsigned char* pNew = new unsigned char[nSize];
    return static_cast<unsigned char*>(memcpy(pNew, pData, nSize));  // idiom-exempt: runtime length
}

#include "port/crtapi.h"


float ImageGetAlphaMean(unsigned char* pData, int nWidth, int nHeight, int nBpp)
{
    if (nBpp < 4)
        return 1.0f;

    int nCount = nWidth * nHeight;
    float fSum = 0.0f;
    if (nCount > 0)
    {
        for (int i = 0; i < nCount; i++)
        {
            fSum += pData[i * nBpp + 3] / 255.0f;
        }
    }
    return fSum / static_cast<float>(nCount);
}

unsigned char* ImageBuildPerlin(int nSize, float fScale)
{
    int nCount = nSize * nSize;
    unsigned char* pBuf = new unsigned char[nCount];
    for (int i = 0; i < nCount; i++)
    {
        pBuf[i] = static_cast<unsigned char>(static_cast<float>(rand_wincompatible() % 128 - 64) * fScale + 127.0f);
    }
    return pBuf;
}

unsigned char* ImageBuildParameterPerlin(int nSize, float fScale, int nMod, int nBase)
{
    int nCount = nSize * nSize;
    unsigned char* pBuf = new unsigned char[nCount];
    if (nCount == 0)
        return pBuf;

    for (int i = 0; i < nCount; i++)
        pBuf[i] = nMod ? static_cast<unsigned char>(static_cast<float>(rand_wincompatible() % nMod) * fScale + static_cast<float>(nBase))
                       : 0;
    return pBuf;
}

unsigned char ImageInterpolate(unsigned char* pData, float fX, float fY, int nWidth, int nHeight)
{
    int nX = static_cast<int>(floorf(fX));
    int nY = static_cast<int>(floorf(fY));
    int nX1 = (nX + 1) % nWidth;
    int nY1 = (nY + 1) % nHeight;
    float fFracX = fX - static_cast<float>(nX);
    float fFracY = fY - static_cast<float>(nY);
    float fRow0 = (1.0f - fFracX) * pData[nY * nWidth + nX] + fFracX * pData[nY * nWidth + nX1];
    float fRow1 = (1.0f - fFracX) * pData[nY1 * nWidth + nX] + fFracX * pData[nY1 * nWidth + nX1];
    return static_cast<unsigned char>(static_cast<int>((1.0f - fFracY) * fRow0 + fFracY * fRow1));
}

int ImageGetPerlinPixel(unsigned char** paNoise, int nX, int nY, int nSize, int nOctaves,
                        int nScale)
{
    if (nOctaves < 1)
        return 127;

    int nResult = 0;
    float fU = static_cast<float>(nX) / static_cast<float>(nScale);
    float fV = static_cast<float>(nY) / static_cast<float>(nScale);
    // Counts down so the octave count is read once.
    for (uint64_t i = static_cast<unsigned>(nOctaves); i != 0; i--)
    {
        nResult += ImageInterpolate(*paNoise++, fU * static_cast<float>(nSize), fV * static_cast<float>(nSize), nSize, nSize);
        nResult -= 127;
        nSize <<= 1;
    }
    return nResult + 127;
}

unsigned char* ImageDownSampleToRequiredSize(int nWidth, int nHeight, unsigned char* pData, int nBox)
{
    int nOutWidth = nWidth / nBox;
    int nOutHeight = nHeight / nBox;
    g_nImagesDownSampled++;

    unsigned char* pNew = new unsigned char[nOutHeight * nOutWidth * 4];
    for (int y = 0; y < nOutHeight; y++)
    {
        for (int x = 0; x < nOutWidth; x++)
        {
            int nSum0 = 0;
            int nSum1 = 0;
            int nSum2 = 0;
            int nSum3 = 0;
            for (int i = 0; i < nBox; i++)
            {
                for (int j = 0; j < nBox; j++)
                {
                    unsigned char* pPixel = pData + ((y * nBox + i) * nWidth + x * nBox + j) * 4;
                    nSum0 += pPixel[0];
                    nSum1 += pPixel[1];
                    nSum2 += pPixel[2];
                    nSum3 += pPixel[3];
                }
            }
            unsigned char* pOut = pNew + (y * nOutWidth + x) * 4;
            pOut[0] = static_cast<unsigned char>(nSum0 / (nBox * nBox));
            pOut[1] = static_cast<unsigned char>(nSum1 / (nBox * nBox));
            pOut[2] = static_cast<unsigned char>(nSum2 / (nBox * nBox));
            pOut[3] = static_cast<unsigned char>(nSum3 / (nBox * nBox));
        }
    }
    return pNew;
}

unsigned char* ImageDownSample(int nWidth, int nHeight, int nBpp, unsigned char* pData)
{
    g_nImagesDownSampled++;
    int nHalfWidth = nWidth / 2;
    int nHalfHeight = nHeight / 2;

    unsigned char* pNew = new unsigned char[nHalfHeight * nHalfWidth * nBpp];

    if (nBpp == 2)
    {
        if (nHeight < 2)
            return pNew;
        for (int y = 0; y < nHalfHeight; y++)
        {
            for (int x = 0; x < nHalfWidth; x++)
            {
                for (int k = 0; k < nBpp; k++)
                {
                    int nSumHi = 0;
                    int nSumLo = 0;
                    for (int i = 0; i < 2; i++)
                        for (int j = 0; j < 2; j++)
                        {
                            unsigned char nByte =
                                pData[(y * 2 + i) * nWidth * nBpp + (x * 2 + j) * nBpp + k];
                            nSumHi += nByte & 0xF0;
                            nSumLo += nByte & 0x0F;
                        }
                    pNew[(y * nHalfWidth + x) * nBpp + k] =
                        static_cast<unsigned char>((nSumHi / 4 & 0xF0) | (nSumLo / 4 & 0x0F));
                }
            }
        }
        return pNew;
    }
    if (nBpp == 3 || nBpp == 4 || nBpp == 1)
    {
        if (nHeight < 2)
            return pNew;
        for (int y = 0; y < nHalfHeight; y++)
        {
            for (int x = 0; x < nHalfWidth; x++)
            {
                for (int k = 0; k < nBpp; k++)
                {
                    int nSum = 0;
                    for (int i = 0; i < 2; i++)
                        for (int j = 0; j < 2; j++)
                            nSum += pData[((y * 2 + i) * nWidth + x * 2 + j) * nBpp + k];
                    pNew[(y * nHalfWidth + x) * nBpp + k] = static_cast<unsigned char>(nSum / 4);
                }
            }
        }
        return pNew;
    }
    return pNew;
}

unsigned char* ImageDownSampleCompressed(int nWidth, int nHeight, int nBpp, unsigned char* pData,
                                         int nLevel)
{
    int nBlockBytes = (nBpp == 4) ? 16 : 8;
    unsigned char* pSrc = 0;
    while ((nWidth | nHeight) != 0)
    {
        if (nLevel == 0)
            pSrc = pData;
        nWidth = nWidth ? nWidth : 1;
        nHeight = nHeight ? nHeight : 1;
        pData += ((nWidth + 3) / 4) * ((nHeight + 3) / 4) * nBlockBytes;
        nWidth >>= 1;
        nHeight >>= 1;
        nLevel--;
    }

#ifdef K2_FIX_ENGINE_BUGS
    if (pSrc == 0)
        return 0;
#endif
    unsigned char* pNew = new unsigned char[pData - pSrc];
    memcpy(pNew, pSrc, pData - pSrc);  // idiom-exempt: runtime length
    return pNew;
}


void ImageGammaCorrect(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fGamma)
{
    if (fGamma == 1.0f)
        return;
    if (nWidth * nHeight * nBpp < 1)
        return;

    int nSize = nWidth * nHeight * nBpp;
    float fInv = 1.0f / fGamma;
    for (int i = 0; i < nSize; i++)
        pData[i] = static_cast<unsigned char>(powf(pData[i] / 255.0f, fInv) * 255.0f);
}

int g_nImagesDecoded;
int shortcircuitdecode;

unsigned char* ImageDecode(unsigned char* pSrc, int nWidth, int nHeight, unsigned char* pPalette,
                           int nUnused, int* pnPalettePage, int nPageCount)
{
    K2_UNUSED(nUnused);
    g_nImagesDecoded++;
    if (shortcircuitdecode)
        return 0;

    int nSize = nHeight * nWidth;
    unsigned char* pBuffer = new unsigned char[nSize * 4];

    unsigned int aTable[15360 / 4];
    unsigned char* pTableBytes = reinterpret_cast<unsigned char*>(aTable);
#ifdef K2_FIX_ENGINE_BUGS
    if (nPageCount > static_cast<int>(sizeof(aTable) / 1024))
        nPageCount = static_cast<int>(sizeof(aTable) / 1024);
    memset(aTable, 0, sizeof(aTable));
#endif
    for (int i = 0; i < nPageCount; i++)
        memcpy(pTableBytes + i * 1024, pPalette + static_cast<uint64_t>(pnPalettePage[i]) * 1024,
               sizeof(aTable[0]) * 256);

#ifdef K2_FIX_ENGINE_BUGS
    unsigned int* pOut = reinterpret_cast<unsigned int*>(pBuffer);
#else
    unsigned long long* pOut = reinterpret_cast<unsigned long long*>(pBuffer);
#endif
    if (nHeight < 1)
        return pBuffer;
    for (unsigned int y = 0; y < static_cast<unsigned int>(nHeight); y++)
    {
        int nRowOffset = y * nWidth * 2;
        unsigned char* pRow = pSrc + nRowOffset;
        for (int x = 0; x < nWidth; x++)
        {
            unsigned int nIndex = pRow[x * 2] | (pRow[x * 2 + 1] << 8);
#ifdef K2_FIX_ENGINE_BUGS
            *pOut++ = nIndex < sizeof(aTable) / sizeof(aTable[0]) ? aTable[nIndex] : 0;
#else
            *pOut++ = *reinterpret_cast<unsigned long long*>(&aTable[nIndex]);
#endif
        }
    }
    return pBuffer;
}

void ImageLowpassLowQualityBlur2(unsigned char* pData, int nWidth, int nHeight)
{
    static unsigned char* newimage = new unsigned char[nWidth * nHeight];
    static int oldheight = nHeight;
    static int oldwidth = nWidth;

    if (oldheight != nHeight || oldwidth != nWidth)
    {
        delete[] newimage;
        newimage = new unsigned char[nWidth * nHeight];
        oldheight = nHeight;
        oldwidth = nWidth;
    }

    memset(newimage, 0, nWidth * nHeight);  // idiom-exempt: runtime length

    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            newimage[y * nWidth + x] +=
                pData[((nHeight - 1 + y) % nHeight) * nWidth + (nWidth + x) % nWidth] >> 3;
            newimage[y * nWidth + x] +=
                pData[((y + nHeight) % nHeight) * nWidth + (nWidth - 1 + x) % nWidth] >> 3;
            newimage[y * nWidth + x] +=
                pData[((y + nHeight) % nHeight) * nWidth + (nWidth + x) % nWidth] >> 1;
            newimage[y * nWidth + x] +=
                pData[((y + nHeight) % nHeight) * nWidth + (nWidth + x + 1) % nWidth] >> 3;
            newimage[y * nWidth + x] +=
                pData[((nHeight - 1 + y + 2) % nHeight) * nWidth + (nWidth + x) % nWidth] >> 3;
        }
    }

    memcpy(pData, newimage, nWidth * nHeight);  // idiom-exempt: runtime length
}

void ImageLowpassLowQualityBlur(unsigned char* pDst, unsigned char* pSrc, int nWidth, int nHeight,
                                int nPart)
{
    for (int y = (nPart * nHeight) / 3; y < ((nPart + 1) * nHeight) / 3; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            pDst[y * nWidth + x] +=
                static_cast<unsigned char>(static_cast<int>(static_cast<float>(pSrc[((nHeight - 1 + y) % nHeight) * nWidth + (nWidth + x) % nWidth]) *
                      0.1f));
            pDst[y * nWidth + x] +=
                static_cast<unsigned char>(static_cast<int>(static_cast<float>(pSrc[((y + nHeight) % nHeight) * nWidth + (nWidth - 1 + x) % nWidth]) *
                      0.1f));
            pDst[y * nWidth + x] +=
                static_cast<unsigned char>(static_cast<int>(static_cast<float>(pSrc[((y + nHeight) % nHeight) * nWidth + (nWidth + x) % nWidth]) *
                      0.5f));
            pDst[y * nWidth + x] +=
                static_cast<unsigned char>(static_cast<int>(static_cast<float>(pSrc[((y + nHeight) % nHeight) * nWidth + (nWidth + x + 1) % nWidth]) *
                      0.1f));
            pDst[y * nWidth + x] +=
                static_cast<unsigned char>(static_cast<int>(static_cast<float>(pSrc[((nHeight - 1 + y + 2) % nHeight) * nWidth + (nWidth + x) % nWidth]) *
                      0.1f));
        }
    }
}

void ImageMandel(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fXCenter,
                 float fYCenter, float fScale)
{
    unsigned char chPixel[4] = {0, 0, 0, 0xFF};
    if (nHeight < 1)
        return;

    float fReStart = fXCenter - fScale;
    float fImStart = fYCenter - fScale;
    float fStep = 2.0f * fScale;
    float fWidth = static_cast<float>(nWidth);
    float fHeight = static_cast<float>(nHeight);

    for (int y = 0; y < nHeight; y++)
    {
        float fIm = fImStart + fStep * (static_cast<float>(y) / fHeight);
        for (int x = 0; x < nWidth; x++)
        {
            float fRe = fReStart + fStep * (static_cast<float>(x) / fWidth);
            int nIter = Mandelval(Complex{fRe, fIm});
            chPixel[0] = static_cast<unsigned char>(nIter << 5);
            chPixel[1] = (nIter << 2) & 0xE0;
            chPixel[2] = (nIter >> 1) & 0xE0;
            memcpy(pData + (y * nWidth + x) * nBpp, chPixel, nBpp);  // idiom-exempt: runtime length
        }
    }
}

void ImageJulia(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fRe, float fIm)
{
    unsigned char chPixel[4] = {0, 0, 0, 0xFF};
    if (nHeight < 1)
        return;

    float fWidth = static_cast<float>(nWidth);
    float fHeight = static_cast<float>(nHeight);

    for (int y = 0; y < nHeight; y++)
    {
        float fZim = 4.0f * (static_cast<float>(y) / fHeight) - 2.0f;
        for (int x = 0; x < nWidth; x++)
        {
            float fZre = 4.0f * (static_cast<float>(x) / fWidth) - 2.0f;
            int nIter = Juliaval(Complex{fRe, fIm}, Complex{fZre, fZim});
            chPixel[0] = static_cast<unsigned char>(nIter << 5);
            chPixel[1] = (nIter << 2) & 0xE0;
            chPixel[2] = (nIter >> 1) & 0xE0;
            memcpy(pData + (y * nWidth + x) * nBpp, chPixel, nBpp);  // idiom-exempt: runtime length
        }
    }
}

static const float g_aLowpassKernel[3][3] = {{0.025f, 0.1f, 0.025f},
                                             {0.1f, 0.5f, 0.1f},
                                             {0.025f, 0.1f, 0.025f}};

void ImageLowpassBlur(unsigned char* pData, int nWidth, int nHeight, int nBpp)
{
    int nSize = nHeight * nWidth * nBpp;
    unsigned char* pNew = new unsigned char[nSize];
    memset(pNew, 0, nSize);  // idiom-exempt: a runtime byte count, not a sizeof

    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    int nSrcY = (y + i + nHeight - 1) % nHeight;
                    int nSrcX = (x + j + nWidth - 1) % nWidth;
                    float fWeight = g_aLowpassKernel[i][j];
                    for (int k = 0; k < nBpp; k++) {
                        K2_ASSUME((y * nWidth + x) * nBpp + k < nSize);   // y < nHeight, x < nWidth
                        pNew[(y * nWidth + x) * nBpp + k] +=
                            static_cast<unsigned char>(fWeight *
                                            pData[(nSrcY * nWidth + nSrcX) * nBpp + k]);
                    }
                }
            }
        }
    }

    memcpy(pData, pNew, nSize);  // idiom-exempt: runtime length
    delete[] pNew;
}

void ImageAddColorModulateAlpha(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                unsigned char nR, unsigned char nG, unsigned char nB)
{
    if (nBpp < 4)
        return;

    float fR = nR;
    float fG = nG;
    float fB = nB;

    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            if (nBpp == 1)
            {
                unsigned char* pPixel = pData + static_cast<int64_t>(y * nWidth + x) * nBpp;
                unsigned char nA = pPixel[3];
                float fA = nA / 255.0f;
                pPixel[0] = static_cast<unsigned char>(fA * pPixel[0] + (1.0f - fA) * fR);
                pPixel[1] = static_cast<unsigned char>(fA * pPixel[1] + (1.0f - fA) * fG);
                pPixel[2] = static_cast<unsigned char>(fA * pData[y * nWidth + x + 2] + (1.0f - fA) * fB);
                pPixel[3] = 255;
            }
            else
            {
                unsigned char* pPixel = pData + static_cast<int64_t>(y * nWidth + x) * nBpp;
                unsigned char nA = pPixel[3];
                float fA = nA / 255.0f;
                pPixel[0] = static_cast<unsigned char>(fA * pPixel[0] + (1.0f - fA) * fR);
                pPixel[1] = static_cast<unsigned char>(fA * pPixel[1] + (1.0f - fA) * fG);
                pPixel[2] = static_cast<unsigned char>(fA * pPixel[2] + (1.0f - fA) * fB);
                pPixel[3] = 255;
            }
        }
    }
}

unsigned char* ImageGetSubImage(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                int nX, int nY, int nSubWidth, int nSubHeight)
{
    K2_UNUSED(nHeight);
    unsigned char* pNew = new unsigned char[nSubWidth * nBpp * nSubHeight];
    for (int y = 0; y < nSubHeight; y++)
    {
        for (int x = 0; x < nSubWidth; x++)
        {
            memcpy(pNew + (x + y * nSubWidth) * nBpp, pData + static_cast<int>(static_cast<unsigned int>(nX + x + (nY + y) * nWidth) * nBpp), nBpp);  // idiom-exempt: runtime length
        }
    }

    return pNew;
}

void ImageFillAlpha(unsigned char* pData, int nWidth, int nHeight, int nBpp, const Vector& vColor)
{
    if (nBpp != 4)
        return;
    if (nHeight < 1)
        return;

    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            unsigned char* pPixel = pData + static_cast<int64_t>(y * nWidth + x) * nBpp;
            unsigned char nA = pPixel[3];
            float fA = nA / 255.0f;
            float fR = (static_cast<float>(pPixel[0]) / 255.0f * fA + vColor.x * (1.0f - fA)) * 255.0f;
            float fG = (static_cast<float>(pPixel[1]) / 255.0f * fA + vColor.y * (1.0f - fA)) * 255.0f;
            float fB = (static_cast<float>(pPixel[2]) / 255.0f * fA + vColor.z * (1.0f - fA)) * 255.0f;
            pPixel[0] = static_cast<unsigned char>(fR);
            pPixel[1] = static_cast<unsigned char>(fG);
            pPixel[2] = static_cast<unsigned char>(fB);
            pPixel[3] = 255;
        }
    }
}

unsigned char* ImageCrop(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                         int nX, int nY, int nCropWidth, int nCropHeight)
{
    K2_UNUSED(nHeight);
    unsigned char* pNew = new unsigned char[nCropWidth * nBpp * nCropHeight];
    unsigned int nSrcRow = (nY * nWidth + nX) * nBpp;
    unsigned int nDstRow = 0;
    for (int y = 0; y < nCropHeight; y++)
    {
        for (int x = 0; x < nCropWidth; x++)
        {
            if (nBpp > 0)
                memcpy(pNew + static_cast<int>(nDstRow + x * nBpp), pData + static_cast<int>(nSrcRow + x * nBpp), static_cast<unsigned int>(nBpp));  // idiom-exempt: runtime length
        }
        nSrcRow += nWidth * nBpp;
        nDstRow += nCropWidth * nBpp;
    }
    return pNew;
}

unsigned char* ImageScale(unsigned char* pData, int nSrcWidth, int nSrcHeight, int nBpp,
                          int nDestWidth, int nDestHeight)
{
    int nDestRowBytes = nDestWidth * nBpp;
    if (nDestRowBytes * nDestHeight == 0)
        return 0;

    unsigned char* pOut = new unsigned char[nDestRowBytes * nDestHeight];
    unsigned int* pAccum = new unsigned int[nDestRowBytes];
    memset(pAccum, 0, nDestRowBytes * sizeof(unsigned int));

    int nSrcRow = 0;
    int nRowWeight = nDestHeight;
    int nSrcWeight = nSrcHeight;
    int y = 0;
    while (y < nDestHeight)
    {
        int nYWeight = (nRowWeight >= nSrcWeight) ? nSrcWeight : nRowWeight;
        int nDestLeft = nDestWidth;
        int nSrcLeft = nSrcWidth;
        int nCol = 0;
        for (int x = 0; x < nDestWidth;)
        {
            int bAdvanceCol;
            if (nDestLeft >= nSrcLeft)
            {
                int nWeight = nSrcLeft * nYWeight;
                for (int c = 0; c < nBpp; c++) {
                    K2_ASSUME(x * nBpp + c < nDestRowBytes);   // x < nDestWidth
                    pAccum[x * nBpp + c] +=
                        pData[nBpp * (nSrcRow * nSrcWidth + nCol) + c] * nWeight;
                }
                nDestLeft -= nSrcLeft;
                if (nDestLeft == 0)
                {
                    nDestLeft = nDestWidth;
                    bAdvanceCol = 1;
                }
                else
                {
                    bAdvanceCol = 0;
                }
                x++;
                nSrcLeft = nSrcWidth;
            }
            else
            {
                int nWeight = nDestLeft * nYWeight;
                for (int c = 0; c < nBpp; c++) {
                    K2_ASSUME(x * nBpp + c < nDestRowBytes);   // x < nDestWidth
                    pAccum[x * nBpp + c] +=
                        pData[nBpp * (nSrcRow * nSrcWidth + nCol) + c] * nWeight;
                }
                nSrcLeft -= nDestLeft;
                nDestLeft = nDestWidth;
                bAdvanceCol = 1;
            }
            nCol += bAdvanceCol;
        }

        int nRemain = nRowWeight - nSrcWeight;
        if (nRowWeight < nSrcWeight)
        {
            nSrcWeight -= nRowWeight;
            nRowWeight = nDestHeight;
            nSrcRow++;
            continue;
        }

        int nDenom = nSrcWidth * nSrcHeight;
        int nRound = nDenom / 2;
        unsigned char* pOutRow = pOut + static_cast<int64_t>(y * nDestWidth) * nBpp;
        for (int x = 0; x < nDestWidth; x++)
        {
            for (int c = 0; c < nBpp; c++)
            {
                K2_ASSUME((y * nDestWidth + x) * nBpp + c < nDestRowBytes * nDestHeight);   // y < nDestHeight
                pOutRow[x * nBpp + c] = static_cast<unsigned char>((pAccum[x * nBpp + c] + nRound) / nDenom);
            }
        }
        if (nRemain == 0)
        {
            nRowWeight = nDestHeight;
            nSrcRow++;
        }
        else
        {
            nRowWeight = nRemain;
        }
        y++;
        memset(pAccum, 0, nDestRowBytes * sizeof(unsigned int));
        // The destination budget carries its remainder into the next row; the source budget
        // restarts every row.
        nSrcWeight = nSrcHeight;
    }

    delete[] pAccum;
    return pOut;
}

unsigned char* ImageConvert32to16bit(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                     int* pFormatOut)
{
    unsigned char* pNew = new unsigned char[nWidth * nHeight * 2];
    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            unsigned char* pSrc = pData + (y * nWidth + x) * nBpp;
            unsigned char b0 = pSrc[0];
            unsigned char b1 = pSrc[1];
            unsigned char b2 = pSrc[2];
            unsigned char b3 = (nBpp == 3) ? 0xFF : pSrc[3];
            pNew[(y * nWidth + x) * 2] = (b2 & 0xF0) | (b3 >> 4);
            pNew[(y * nWidth + x) * 2 + 1] = (b0 & 0xF0) | (b1 >> 4);
        }
    }
    *pFormatOut = 9;
    return pNew;
}



static int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nResult = __vsprintf_chk(pszBuf, 0, nSize, pszFmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

int ImageReadS3TCHeader(FILE* pFile, int* pWidth, int* pHeight, int* pFormat, int* pUnknown,
                        float* pF)
{
    if (!pFile)
        return 0;

    *pUnknown = 0;
    *pWidth = 0;
    *pHeight = 0;
    *pFormat = 0;
    *pF = -1.0f;
    fread(pWidth, 4, 1, pFile);
    fread(pHeight, 4, 1, pFile);
    fread(pFormat, 4, 1, pFile);
    fread(pUnknown, 4, 1, pFile);
    fread(pF, 4, 1, pFile);
    return 1;
}

int ImageReadS3TCHeader(char* pszName, int* pWidth, int* pHeight, int* pFormat, int* pUnknown,
                        float* pF)
{
    char szPath[256];
    if (!pszName)
        return 0;
    if (!*pszName)
        return 0;

    *pUnknown = 0;
    *pWidth = 0;
    *pHeight = 0;
    *pFormat = 0;
    FormatName(szPath, 256, "%s", pszName);
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        FormatName(szPath, 256, "%s.dds", pszName);
        pFile = fopen(szPath, "rb");
        if (!pFile)
            return 0;
    }

    *pUnknown = 0;
    *pWidth = 0;
    *pHeight = 0;
    *pFormat = 0;
    *pF = -1.0f;
    fread(pWidth, 4, 1, pFile);
    fread(pHeight, 4, 1, pFile);
    fread(pFormat, 4, 1, pFile);
    fread(pUnknown, 4, 1, pFile);
    fread(pF, 4, 1, pFile);
    fclose(pFile);
    return 1;
}

struct TGATail
{
    unsigned short m_awReserved[6];
    unsigned short m_nWidth;
    unsigned short m_nHeight;
    unsigned char m_nBpp;
    unsigned char m_nDescriptor;
};

__attribute__((no_stack_protector))
void ImageReadTGAHeader(FILE* pFile, int* pWidth, int* pHeight, int* pBpp)
{
    TGATail header;
    fread(&header, 18, 1, pFile);
    *pWidth = header.m_nWidth;
    *pHeight = header.m_nHeight;
    *pBpp = header.m_nBpp >> 3;
}

unsigned char* ImageReadS3TC(FILE* pFile, int* pWidth, int* pHeight, int* pFormat,
                             int* pUnknown, float* pF)
{
    if (!pFile)
        return 0;

    fread(pWidth, 4, 1, pFile);
    fread(pHeight, 4, 1, pFile);
    fread(pFormat, 4, 1, pFile);
    fread(pUnknown, 4, 1, pFile);
    fread(pF, 4, 1, pFile);

    int nBlockSize = (*pFormat == 4) ? 16 : 8;
    int nAlloc = ImageGetS3TCSize(*pWidth, *pHeight, *pFormat);
    unsigned char* pNew = new unsigned char[nAlloc];
#ifdef K2_FIX_ENGINE_BUGS
    int nTopSize = ((*pWidth + 3) / 4) * ((*pHeight + 3) / 4) * nBlockSize;
    if (*pUnknown < 0 || *pUnknown > nTopSize)
        *pUnknown = nTopSize;
    K2_ASSUME(*pUnknown <= nAlloc);   // the top level is the first of the chain
#endif
    fread(pNew, 1, *pUnknown, pFile);

    unsigned char* pDst = pNew;
    int nSize = *pUnknown;
    int nWidth = *pWidth >> 1;
    int nHeight = *pHeight >> 1;
    while (nWidth | nHeight)
    {
        int nMipWidth = nWidth ? nWidth : 1;
        int nMipHeight = nHeight ? nHeight : 1;
        pDst += nSize;
        nSize = ((nMipWidth + 3) / 4) * (((nMipHeight + 3) / 4) * nBlockSize);
        fread(pDst, 1, nSize, pFile);
        nWidth = nMipWidth >> 1;
        nHeight = nMipHeight >> 1;
    }
    return pNew;
}

unsigned char* ImageReadS3TC(char* pszName, int* pWidth, int* pHeight, int* pFormat,
                             int* pUnknown, float* pF)
{
    char szPath[256];
    if (!pszName)
        return 0;
    if (!*pszName)
        return 0;

    FormatName(szPath, 256, "%s", pszName);
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        FormatName(szPath, 256, "%s.dds", pszName);
        pFile = fopen(szPath, "rb");
        if (!pFile)
            return 0;
    }

    unsigned char* pNew = ImageReadS3TC(pFile, pWidth, pHeight, pFormat, pUnknown, pF);
    fclose(pFile);
    return pNew;
}

__attribute__((no_stack_protector))
unsigned char* ImageReadTGA(FILE* pFile, int* pWidth, int* pHeight, int* pBpp)
{
    if (!pFile)
        return 0;

    TGATail header;
    fread(&header, 18, 1, pFile);
    *pWidth = header.m_nWidth;
    *pHeight = header.m_nHeight;
    *pBpp = header.m_nBpp >> 3;

    int nSize = *pWidth * *pHeight * *pBpp;
    unsigned char* pNew = new unsigned char[nSize];
    fread(pNew, 1, nSize, pFile);
    ImageFlipRB(pNew, *pWidth, *pHeight, *pBpp);
    return pNew;
}

unsigned char* ImageReadTGA(char* pszName, int* pWidth, int* pHeight, int* pBpp)
{
    char szPath[256];
    TGATail header;
    if (!pszName)
        return 0;
    if (!*pszName)
        return 0;

    FormatName(szPath, 256, "%s", pszName);
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        FormatName(szPath, 256, "%s.tga", pszName);
        pFile = fopen(szPath, "rb");
        if (!pFile)
            return 0;
    }

    fread(&header, 18, 1, pFile);
    *pWidth = header.m_nWidth;
    *pHeight = header.m_nHeight;
    *pBpp = header.m_nBpp >> 3;

    int nSize = *pWidth * *pHeight * *pBpp;
    unsigned char* pNew = new unsigned char[nSize];
    fread(pNew, 1, nSize, pFile);
    fclose(pFile);
    ImageFlipRB(pNew, *pWidth, *pHeight, *pBpp);
    return pNew;
}

unsigned char* ImageReadTGAImage(FILE* pFile, int nWidth, int nHeight, int nBpp)
{
    int nSize = nWidth * nHeight * nBpp;
    unsigned char* pBuf = new unsigned char[nSize];
    fread(pBuf, 1, nSize, pFile);
    return pBuf;
}

unsigned char* ImageRead16bit(char* pszName, int* pWidth, int* pHeight, float* pF, int* pUnknown)
{
    char szPath[256];
    __builtin___strcpy_chk(szPath, pszName, sizeof(szPath));
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        __builtin___strcat_chk(szPath, ".4pc", sizeof(szPath));
        pFile = fopen(szPath, "rb");
        if (!pFile)
            return 0;
    }

    fread(pWidth, 4, 1, pFile);
    fread(pHeight, 4, 1, pFile);
    fread(pF, 4, 1, pFile);
    fread(pUnknown, 4, 1, pFile);

    int nSize = *pWidth * *pHeight;
    unsigned char* pNew = new unsigned char[nSize * 2];
    fread(pNew, 1, nSize * 2, pFile);
    fclose(pFile);
    return pNew;
}

int ImageRead16bitHeader(char* pszName, int* pA, int* pB, float* pF, int* pC)
{
    char szPath[256];
    __builtin___strcpy_chk(szPath, pszName, sizeof(szPath));
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        __builtin___strcat_chk(szPath, ".4pc", sizeof(szPath));
        pFile = fopen(szPath, "rb");
    }
    int nResult = 0;
    if (pFile)
    {
        nResult = 1;
        fread(pA, 4, 1, pFile);
        fread(pB, 4, 1, pFile);
        fread(pF, 4, 1, pFile);
        fread(pC, 4, 1, pFile);
        fclose(pFile);
    }
    return nResult;
}

unsigned char* ImageRead(char* pszName, int* pWidth, int* pHeight, int* pBpp)
{
    return ImageReadTGA(pszName, pWidth, pHeight, pBpp);
}

unsigned char* ImageRead(FILE* pFile, int* pWidth, int* pHeight, int* pBpp)
{
    return ImageReadTGA(pFile, pWidth, pHeight, pBpp);
}

int ImageReadHeader(char* pszName, int* pWidth, int* pHeight, int* pBpp)
{
    char szPath[256];
    TGATail header;
    if (!pszName)
        return 0;
    if (!*pszName)
        return 0;

    *pWidth = 0;
    *pHeight = 0;
    *pBpp = 0;
    FormatName(szPath, 256, "%s", pszName);
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
    {
        FormatName(szPath, 256, "%s.tga", pszName);
        pFile = fopen(szPath, "rb");
        if (!pFile)
            return 0;
    }

    fread(&header, 18, 1, pFile);
    *pWidth = header.m_nWidth;
    *pHeight = header.m_nHeight;
    *pBpp = header.m_nBpp >> 3;
    fclose(pFile);
    return 1;
}

// The 48-byte header as this disk reader takes it, with the dimensions read as 64-bit values at
// 0x20/0x28 (CResPLT's PLTHeader is the resource format's own six dwords).
struct ImagePLTHeader
{
    char m_acReserved[32];
    uint64_t m_nWidth;
    uint64_t m_nHeight;
};

unsigned char* ImageReadPLT(char* pszName, int* pWidth, int* pHeight, int* pUnknown)
{
    K2_UNUSED(pUnknown);
    char szPath[32];
    FormatName(szPath, 32, "%s.plt", pszName);
    FILE* pFile = fopen(szPath, "rb");
    if (!pFile)
        return 0;

    ImagePLTHeader header;
    fread(&header, 48, 1, pFile);
    *pWidth = static_cast<int>(header.m_nWidth);
    *pHeight = static_cast<int>(header.m_nHeight);
    uint64_t nSize = header.m_nWidth * header.m_nHeight * 2;
    unsigned char* pNew = new unsigned char[nSize];
    fread(pNew, 1, nSize, pFile);
    fclose(pFile);
    return pNew;
}

unsigned char* ImageReadProcessed(char* pszName, int* pWidth, int* pHeight, char* pBpp,
                                  int* pFormat, int* pA, int* pB, float* pF, char* pC)
{
    K2_UNUSED(pszName); K2_UNUSED(pWidth); K2_UNUSED(pHeight); K2_UNUSED(pBpp); K2_UNUSED(pFormat); K2_UNUSED(pA); K2_UNUSED(pB); K2_UNUSED(pF); K2_UNUSED(pC);
    return 0;
}

int ImageReadProcessedHeader(char* pszName, int* pWidth, int* pHeight, char* pBpp, int* pFormat,
                             int* pA, float* pF, char* pC)
{
    K2_UNUSED(pszName); K2_UNUSED(pWidth); K2_UNUSED(pHeight); K2_UNUSED(pBpp); K2_UNUSED(pFormat); K2_UNUSED(pA); K2_UNUSED(pF); K2_UNUSED(pC);
    return 0;
}

void ImageWriteS3TC(FILE* pFile, int nWidth, int nHeight, int nFormat, float fScale, int nSize,
                    unsigned char* pData)
{
    if (pFile == 0)
        return;

    int nBlockBytes = (nFormat == 4) ? 16 : 8;
    fwrite(&nWidth, 4, 1, pFile);
    fwrite(&nHeight, 4, 1, pFile);
    fwrite(&nFormat, 4, 1, pFile);
    fwrite(&nSize, 4, 1, pFile);
    fwrite(&fScale, 4, 1, pFile);
    fwrite(pData, 1, nSize, pFile);

    int nW = nWidth >> 1;
    int nH = nHeight >> 1;
    while ((nW | nH) != 0)
    {
        nW = nW ? nW : 1;
        nH = nH ? nH : 1;
        pData += nSize;
        nSize = ((nW + 3) / 4) * ((nH + 3) / 4) * nBlockBytes;
        fwrite(pData, 1, nSize, pFile);
        nW >>= 1;
        nH >>= 1;
    }
}

void ImageWrite16bit(FILE* pFile, unsigned char* pData, int nWidth, int nHeight, float fScale,
                     int nFormat)
{
    fwrite(&nWidth, 4, 1, pFile);
    fwrite(&nHeight, 4, 1, pFile);
    fwrite(&fScale, 4, 1, pFile);
    fwrite(&nFormat, 4, 1, pFile);
    fwrite(pData, 1, nWidth * nHeight * 2, pFile);
}

void ImageWrite16bit(char* pszName, unsigned char* pData, int nWidth, int nHeight, float fScale,
                     int nFormat)
{
    FILE* pFile = fopen(pszName, "wb");
    ImageWrite16bit(pFile, pData, nWidth, nHeight, fScale, nFormat);
    fclose(pFile);
}

#include "restga.h"

#if KOTOR1
#include <string>
#include <algorithm>
#include "sdlrwops.h"

void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData)
{
    TGAHeader header;
    header.m_idLength = 0;
    header.m_colorMapType = 0;
    header.m_imageType = 2;
    for (int i = 0; i < 5; i++)
        header.m_abColorMapSpec[i] = 0;
    header.m_xOrigin = 0;
    header.m_yOrigin = 0;
    header.m_width = static_cast<unsigned short>(nWidth);
    header.m_height = static_cast<unsigned short>(nHeight);
    header.m_bitsPerPixel = static_cast<unsigned char>(nBpp << 3);
    header.m_descriptor = 0;

    SDL_RWops* pFile;
    int nSize;
    unsigned char* pNew;
    std::string sPath(pszName);
    std::replace(sPath.begin(), sPath.end(), '\\', '/');
    pFile = SDL_RWFromFile(sPath.c_str(), "wb");
    if (pFile) {
        SDL_RWwrite(pFile, &header, sizeof(header), 1);
        nSize = nWidth * nHeight * nBpp;
        pNew = new unsigned char[nSize];
        memcpy(pNew, pData, nSize);  // idiom-exempt: runtime length
        ImageFlipY(pNew, nWidth, nHeight, nBpp);
        ImageFlipRB(pNew, nWidth, nHeight, nBpp);
        SDL_RWwrite(pFile, pNew, 1, nSize);
#ifdef K2_FIX_ENGINE_BUGS
        delete[] pNew;
#else
        delete pNew;
#endif
        SDL_RWclose(pFile);
    }
}
#else
void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData,
                   bool bFlipY)
{
    char szPath[1024];
    __strcpy_chk(szPath, pszName, sizeof(szPath));

    FILE* pFile = fopen(szPath, "wb");
    if (!pFile)
        return;

    TGAHeader header;
    memset(&header, 0, sizeof(header));
    header.m_imageType = 2;
    header.m_width = static_cast<unsigned short>(nWidth);
    header.m_height = static_cast<unsigned short>(nHeight);
    header.m_bitsPerPixel = static_cast<unsigned char>(nBpp << 3);
    fwrite(&header, sizeof(header), 1, pFile);

    int nSize = nWidth * nHeight * nBpp;
    unsigned char* pNew = new unsigned char[nSize];
    memcpy(pNew, pData, nSize);  // idiom-exempt: runtime length
    if (bFlipY)
        ImageFlipY(pNew, nWidth, nHeight, nBpp);
    ImageFlipRB(pNew, nWidth, nHeight, nBpp);

    fwrite(pNew, 1, nSize, pFile);
#ifdef K2_FIX_ENGINE_BUGS
    delete[] pNew;
#else
    delete pNew;
#endif
    fclose(pFile);
}
#endif

Complex operator*(const Complex& a, const Complex& b)
{
    return Complex{a.m_fRe * b.m_fRe - a.m_fIm * b.m_fIm,
                   a.m_fRe * b.m_fIm + a.m_fIm * b.m_fRe};
}

Complex operator*(const Complex& a, float f)
{
    return Complex{a.m_fRe * f, a.m_fIm * f};
}

Complex operator+(const Complex& a, const Complex& b)
{
    return Complex{a.m_fRe + b.m_fRe, a.m_fIm + b.m_fIm};
}

float magnitudesqr(const Complex& c)
{
    return c.m_fRe * c.m_fRe + c.m_fIm * c.m_fIm;
}

int Mandelval(Complex c)
{
    int nIter = 0;
    float fZi = 0.0f;
    float fZr = 0.0f;
    while (nIter < 255 && fZr * fZr + fZi * fZi < 4.0f)
    {
        float fZr2 = fZr * fZr - fZi * fZi;
        float fZi2 = fZr * fZi * 2.0f;
        fZr = c.m_fRe + fZr2;
        fZi = c.m_fIm + fZi2;
        nIter++;
    }
    return nIter;
}

int Juliaval(Complex c, Complex z)
{
    int nIter = 0;
    while (nIter < 255 && z.m_fRe * z.m_fRe + z.m_fIm * z.m_fIm < 4.0f)
    {
        float fZr2 = z.m_fRe * z.m_fRe - z.m_fIm * z.m_fIm;
        float fZi2 = z.m_fRe * z.m_fIm * 2.0f;
        z = Complex{c.m_fRe + fZr2, c.m_fIm + fZi2};
        nIter++;
    }
    return nIter;
}
