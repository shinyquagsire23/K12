// Free-function Aurora image and texture-codec API, with flips, crops and scales on raw pixel
// buffers, the TGA / S3TC / 16-bit PLT loaders and writers, the Perlin and fractal generators, and
// the blur and fill post passes. Nothing here owns state; every entry point takes its buffer and
// dimensions as arguments.

#ifndef AURIMAGE_H
#define AURIMAGE_H

#include "port/libcbase.h"

struct Vector;

void ImageFlipY(unsigned char* pData, int nWidth, int nHeight, int nBpp);
void ImageFlipRB(unsigned char* pData, int nWidth, int nHeight, int nBpp);
unsigned char* ImageScale(unsigned char* pData, int nSrcWidth, int nSrcHeight, int nBpp,
                         int nDestWidth, int nDestHeight);
unsigned char* ImageDup(unsigned char* pData, int nWidth, int nHeight, int nFormat, int bCompressed);
void ImageCopy(unsigned char* pDst, const unsigned char* pSrc, int nWidth, int nHeight, int nBpp);
void ImageFree(void* pData, int nWidth, int nHeight, int nBpp);

int ImageGetS3TCSize(int nWidth, int nHeight, int nFormat);
unsigned char* ImageConvert32to16bit(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                     int* pFormatOut);

float ImageGetAlphaMean(unsigned char* pData, int nWidth, int nHeight, int nBpp);
unsigned char* ImageBuildPerlin(int nSize, float fScale);
unsigned char* ImageBuildParameterPerlin(int nSize, float fScale, int nMod, int nBase);
unsigned char ImageInterpolate(unsigned char* pData, float fX, float fY, int nWidth, int nHeight);
int ImageGetPerlinPixel(unsigned char** paNoise, int nX, int nY, int nSize, int nOctaves,
                        int nScale);

unsigned char* ImageDownSampleToRequiredSize(int nWidth, int nHeight, unsigned char* pData,
                                             int nBox);
unsigned char* ImageDownSample(int nWidth, int nHeight, int nBpp, unsigned char* pData);
unsigned char* ImageDownSampleCompressed(int nWidth, int nHeight, int nBpp, unsigned char* pData,
                                         int nLevel);

void ImageGammaCorrect(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fGamma);
void ImageLowpassBlur(unsigned char* pData, int nWidth, int nHeight, int nBpp);
void ImageLowpassLowQualityBlur2(unsigned char* pData, int nWidth, int nHeight);
void ImageLowpassLowQualityBlur(unsigned char* pDst, unsigned char* pSrc, int nWidth, int nHeight,
                                int nPart);

void ImageReadTGAHeader(FILE* pFile, int* pWidth, int* pHeight, int* pBpp);
unsigned char* ImageReadTGAImage(FILE* pFile, int nWidth, int nHeight, int nBpp);
int ImageRead16bitHeader(char* pszName, int* pA, int* pB, float* pF, int* pC);
void ImageWrite16bit(FILE* pFile, unsigned char* pData, int nWidth, int nHeight, float fScale,
                     int nFormat);
void ImageWrite16bit(char* pszName, unsigned char* pData, int nWidth, int nHeight, float fScale,
                     int nFormat);
#if KOTOR1
void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData);
#else
void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData,
                   bool bFlipY);
#endif
int ImageReadS3TCHeader(FILE* pFile, int* pWidth, int* pHeight, int* pFormat, int* pUnknown,
                        float* pF);
void ImageWriteS3TC(FILE* pFile, int nWidth, int nHeight, int nFormat, float fScale, int nSize,
                    unsigned char* pData);
int ImageReadS3TCHeader(char* pszName, int* pWidth, int* pHeight, int* pFormat, int* pUnknown,
                        float* pF);
unsigned char* ImageReadS3TC(FILE* pFile, int* pWidth, int* pHeight, int* pFormat,
                             int* pUnknown, float* pF);
unsigned char* ImageReadS3TC(char* pszName, int* pWidth, int* pHeight, int* pFormat,
                             int* pUnknown, float* pF);
unsigned char* ImageRead16bit(char* pszName, int* pWidth, int* pHeight, float* pF,
                              int* pUnknown);
unsigned char* ImageReadTGA(FILE* pFile, int* pWidth, int* pHeight, int* pBpp);
unsigned char* ImageReadTGA(char* pszName, int* pWidth, int* pHeight, int* pBpp);
int ImageReadHeader(char* pszName, int* pWidth, int* pHeight, int* pBpp);
unsigned char* ImageRead(char* pszName, int* pWidth, int* pHeight, int* pBpp);
unsigned char* ImageRead(FILE* pFile, int* pWidth, int* pHeight, int* pBpp);
unsigned char* ImageReadPLT(char* pszName, int* pWidth, int* pHeight, int* pUnknown);
unsigned char* ImageReadProcessed(char* pszName, int* pWidth, int* pHeight, char* pBpp,
                                  int* pFormat, int* pA, int* pB, float* pF, char* pC);
int ImageReadProcessedHeader(char* pszName, int* pWidth, int* pHeight, char* pBpp, int* pFormat,
                             int* pA, float* pF, char* pC);

struct Complex
{
    float m_fRe;
    float m_fIm;
};

Complex operator*(const Complex& a, const Complex& b);
Complex operator*(const Complex& a, float f);
Complex operator+(const Complex& a, const Complex& b);
float magnitudesqr(const Complex& c);
int Mandelval(Complex c);
int Juliaval(Complex c, Complex z);

void ImageMandel(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fXCenter,
                 float fYCenter, float fScale);
void ImageJulia(int nWidth, int nHeight, int nBpp, unsigned char* pData, float fRe, float fIm);

void ImageAddColorModulateAlpha(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                unsigned char nR, unsigned char nG, unsigned char nB);

unsigned char* ImageGetSubImage(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                                int nX, int nY, int nSubWidth, int nSubHeight);
unsigned char* ImageCrop(unsigned char* pData, int nWidth, int nHeight, int nBpp,
                         int nX, int nY, int nCropWidth, int nCropHeight);

#endif
