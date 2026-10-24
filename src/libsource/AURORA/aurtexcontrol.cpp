#include "k2stdint.h"
#include "aurres.h"
#if KOTOR1
// K1's own library; List.h's List grows from 8.
#define K1_AURORA_LIST
#endif
#include "aurtexcontrol.h"
#include "aurtexman.h"

int lowqualityblur;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/crtapi.h"

// A hand-expanded fortify strlen, since <string.h> would redeclare memcpy without its exception
// spec.
#define strlen(s) __strlen_chk((s), sizeof(s))

char* firstword(char* pszLine);
char* skipspace(char* pszLine);
unsigned char* AurResGetData(int nBytes);
void Parse(char* pszLine, char* pszValue);
void Parse(char* pszLine, float& fValue);
void Parse(char* pszLine, int& nValue);
// Defined below, with its explicit instantiation.
template <class T> void Parse(char* pszLine, List<T>& list);

float cosineLUT(float fAngle);
float sineLUT(float fAngle);

TextureController::TextureController(CAurTexture* pTexture)
    : m_pTexture(pTexture)
{
    m_pSourceImage = 0;
    m_nLastFrame = 0;
    m_nDistort = 0;
    m_fDistortionAmplitude = 5.0f;
    m_fSpeed = 1.0f;
    m_nDistortAngle = 1;
}

// Each per-channel key lazily sizes the scale/translate list to four slots filled with 1.0f,
// and uses -1.0f as the "unset" sentinel.
void TextureController::ParseField(char* pszLine)
{
    if (!_stricmp("channelscale", firstword(pszLine)))
        Parse(pszLine + strlen("channelscale"), m_lstChannelScale);
    if (!_stricmp("channeltranslate", firstword(pszLine)))
        Parse(pszLine + strlen("channeltranslate"), m_lstChannelTranslate);
    if (!_stricmp("distort", firstword(pszLine)))
        Parse(pszLine + strlen("distort"), m_nDistort);
    if (!_stricmp("distortangle", firstword(pszLine)))
        Parse(pszLine + strlen("distortangle"), m_nDistortAngle);
    if (!_stricmp("distortionamplitude", firstword(pszLine)))
        Parse(pszLine + strlen("distortionamplitude"), m_fDistortionAmplitude);
    if (!_stricmp("speed", firstword(pszLine)))
        Parse(pszLine + strlen("speed"), m_fSpeed);

    float fValue;
    fValue = -1.0f;
    if (!_stricmp("channelscale0", firstword(pszLine)))
        Parse(pszLine + strlen("channelscale0"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelScale.m_count == 0) {
            m_lstChannelScale.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelScale.m_pData[i] = 1.0f;
        }
        m_lstChannelScale.m_pData[0] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channelscale1", firstword(pszLine)))
        Parse(pszLine + strlen("channelscale1"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelScale.m_count == 0) {
            m_lstChannelScale.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelScale.m_pData[i] = 1.0f;
        }
        m_lstChannelScale.m_pData[1] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channelscale2", firstword(pszLine)))
        Parse(pszLine + strlen("channelscale2"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelScale.m_count == 0) {
            m_lstChannelScale.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelScale.m_pData[i] = 1.0f;
        }
        m_lstChannelScale.m_pData[2] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channelscale3", firstword(pszLine)))
        Parse(pszLine + strlen("channelscale3"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelScale.m_count == 0) {
            m_lstChannelScale.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelScale.m_pData[i] = 1.0f;
        }
        m_lstChannelScale.m_pData[3] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channeltranslate0", firstword(pszLine)))
        Parse(pszLine + strlen("channeltranslate0"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelTranslate.m_count == 0) {
            m_lstChannelTranslate.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelTranslate.m_pData[i] = 1.0f;
        }
        m_lstChannelTranslate.m_pData[0] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channeltranslate1", firstword(pszLine)))
        Parse(pszLine + strlen("channeltranslate1"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelTranslate.m_count == 0) {
            m_lstChannelTranslate.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelTranslate.m_pData[i] = 1.0f;
        }
        m_lstChannelTranslate.m_pData[1] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channeltranslate2", firstword(pszLine)))
        Parse(pszLine + strlen("channeltranslate2"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelTranslate.m_count == 0) {
            m_lstChannelTranslate.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelTranslate.m_pData[i] = 1.0f;
        }
        m_lstChannelTranslate.m_pData[2] = fValue;
    }
    fValue = -1.0f;
    if (!_stricmp("channeltranslate3", firstword(pszLine)))
        Parse(pszLine + strlen("channeltranslate3"), fValue);
    if (fValue >= 0.0f) {
        if (m_lstChannelTranslate.m_count == 0) {
            m_lstChannelTranslate.SetSize(4);
            for (int i = 0; i < 4; i++)
                m_lstChannelTranslate.m_pData[i] = 1.0f;
        }
        m_lstChannelTranslate.m_pData[3] = fValue;
    }
}

WaterTextureController::WaterTextureController(CAurTexture* pTexture)
    : TextureController(pTexture)
{
    m_nWaterWidth = 31;
    m_nWaterHeight = 31;
    m_pfHeights = 0;
    m_pfNextHeights = 0;
    m_pfVelocities = 0;
    m_fForceCycleSpeed = 57.295776f;   // 180/pi, so the cycle speed is in radians per second
    m_fAngleCycleSpeed = 2.8647888f;
    m_pImage = 0;
    m_pBlurImage = 0;
    m_nCycle = 2;
}

WaveTextureController::WaveTextureController(CAurTexture* pTexture)
    : TextureController(pTexture)
{
    m_pfHeights = 0;
    m_pfPrevHeights = 0;
}

ArturoTextureController::ArturoTextureController(CAurTexture* pTexture)
    : TextureController(pTexture)
{
    m_nLastFrame = -1;
    m_nArturoWidth = 15;
    m_nArturoHeight = 15;
    m_pfNextHeights = 0;
    m_pfHeights = 0;
    m_pfPrevHeights = 0;
    m_pImage = 0;
}

CycleTIDTextureController::CycleTIDTextureController(CAurTexture* pTexture)
    : TextureController(pTexture)
{
    m_fFPS = 1.0f;
    m_nTIDIndex = 0;
    m_fFrameTime = 0;
}

TextureController::~TextureController()
{
    delete[] m_pSourceImage;
}

void TextureController::SetImage()
{
    int nSize = m_pTexture->GetWidth() * m_pTexture->GetHeight() * m_pTexture->GetBPP();
    m_pSourceImage = new unsigned char[nSize];
    memcpy(m_pSourceImage, m_pTexture->GetImage(), nSize);  // idiom-exempt: runtime length
}

unsigned char TextureController::newimage[256 * 256 * 4];

// Per-pixel barrel/lens displacement, written into the shared scratch image which is also the
// return value. m_nDistortAngle picks the mode: nonzero derives a polar angle from the pixel
// value (cosine displaces the row, sine the column); zero applies one radial magnitude to both
// axes. The - 0.5f bias is inside the truncation, and the wrap masks are power-of-two row and
// column masks, not a modulo.
unsigned char* TextureController::Distort(unsigned char* pImage)
{
    int nBPP = m_pTexture->GetBPP();
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();

    if (m_nDistortAngle != 0) {
        if (nHeight >= 1) {
            for (int y = 0; y < nHeight; y++) {
                if (nWidth >= 1) {
                    for (int x = 0; x < nWidth; x++) {
                        float fAngle = static_cast<float>(pImage[x]) * 0.024543693f;
                        int nOffX = static_cast<int>(m_fDistortionAmplitude * cosineLUT(fAngle) - 0.5f);
                        int nOffY = static_cast<int>(m_fDistortionAmplitude * sineLUT(fAngle) - 0.5f);
                        int nRow = (nHeight + y + nOffX) & (nHeight - 1);
                        int nCol = (nWidth + x + nOffY) & (nWidth - 1);
                        memcpy(&newimage[(y * nWidth + x) * nBPP],
                               &m_pSourceImage[(nRow * nWidth + nCol) * nBPP],
                               nBPP);  // idiom-exempt: runtime length
                    }
                }
                pImage += nWidth;
            }
        }
    } else {
        if (nHeight >= 1) {
            for (int y = 0; y < nHeight; y++) {
                if (nWidth >= 1) {
                    for (int x = 0; x < nWidth; x++) {
                        int nOff = static_cast<int>(m_fDistortionAmplitude
                                       * (static_cast<float>(pImage[x] - 127) * 0.0078125f) - 0.5f);
                        int nRow = (y + nOff) & (nHeight - 1);
                        int nCol = (x + nOff) & (nWidth - 1);
                        memcpy(&newimage[(y * nWidth + x) * nBPP],
                               &m_pSourceImage[(nRow * nWidth + nCol) * nBPP],
                               nBPP);  // idiom-exempt: runtime length
                    }
                }
                pImage += nWidth;
            }
        }
    }
    return newimage;
}

// The parser itself lives in aurparse/; this TU spells the List<float> instantiation, so the
// body must stay identical to the copy there.
#include "List.h"

template <class T>
void Parse(char* pszLine, List<T>& list)
{
    int nCount = 0;
    char cType;
    int nScan = sscanf(pszLine, "%d %c", &nCount, &cType);
    if (nScan <= 0) {
        // `endlist`-terminated form: one element per line until the terminator (or EOF).
        pszLine = AurResGetNextLine();
        while (pszLine) {
            if (!strncmp(skipspace(pszLine), "endlist", strlen("endlist")))
                break;
            if (list.m_count != 0) {
                if (list.m_count == nCount)
                    list.Add(list.m_pData[list.m_count - 1]);
            } else {
                list.allocate(1);
                list.m_count = 1;
            }
            int n = nCount;
            Parse(pszLine, list.m_pData[n]);
            nCount++;
            pszLine = AurResGetNextLine();
        }
        skipspace(skipspace(pszLine) + strlen("endlist"));
        return;
    }

    list.SetSize(nCount);
    if (nScan == 1) {
        // In the count-only form there is one element per following line.
        for (int64_t i = 0; i < list.m_count; i++) {
            char* pszNext = AurResGetNextLine();
            if (!pszNext)
                break;
            Parse(pszNext, list.m_pData[i]);
        }
    } else {
        // In the packed-blob form the elements are the resource's raw bytes.
        int nBytes = nCount * sizeof(T);
        memcpy(list.m_pData, AurResGetData(nBytes), nBytes);  // idiom-exempt: runtime length
        pszLine[0] = 0;
    }
}

#if KOTOR1  // K1 spells this instantiation in its own parser TU
extern template void Parse<float>(char*, List<float>&);
#else
template void Parse<float>(char*, List<float>&);
#endif

// The channel packer the upload tail calls. pSrc is the height field, one byte per pixel, and the
// destination is written per channel pair with the two List<float> tuning arrays applied as an
// affine map.
//
// The nBPP == 4 path writes four bytes per pixel and scales the translate term by 255.0f; every
// other nBPP scales it by 15.0f after shifting the source byte right by 4, treating the same byte
// as a 0..15 intensity. Each channel's scale and translate are guarded loads that default to
// 1.0f and 0.0f, and each term is gated independently: a zero or negative coefficient contributes
// zero, not a zero product.
// A null pDst is allocated here and returned.
unsigned char* TextureController::Height2Channel(unsigned char* pSrc, unsigned char* pDst)
{
    int nHeight = m_pTexture->GetHeight();
    int nWidth = m_pTexture->GetWidth();
    int nBPP = m_pTexture->GetBPP();
    if (pDst == 0)
        pDst = new unsigned char[nWidth * nHeight * nBPP];
    if (nHeight < 1)
        return pDst;

    for (int y = 0; y < nHeight; y++) {
        for (int x = 0; x < nWidth; x++) {
            unsigned char uSrc = pSrc[y * nWidth + x];
            if (nBPP == 4) {
                for (int n = 0; n < 4; n++) {
                    float fScale = 1.0f;
                    if (m_lstChannelScale.m_count > n)
                        fScale = m_lstChannelScale.m_pData[n];
                    float fTranslate = 0.0f;
                    if (m_lstChannelTranslate.m_count > n)
                        fTranslate = m_lstChannelTranslate.m_pData[n];
                    int nValue = 0;
                    if (fScale > 0.0f)
                        nValue = static_cast<int>(static_cast<float>(uSrc) * fScale);
                    if (fTranslate > 0.0f)
                        nValue = nValue + static_cast<int>(fTranslate * 255.0f);
                    pDst[(y * nWidth + x) * 4 + n] = static_cast<unsigned char>(nValue);
                }
            } else if (nBPP >= 1) {
                unsigned char* pDstRow = pDst + y * nWidth * nBPP;
                float fValue = static_cast<float>(uSrc >> 4);
                for (int n = 0; n < nBPP; n += 2) {
                    float fScaleLo = 1.0f;
                    if (m_lstChannelScale.m_count > n)
                        fScaleLo = m_lstChannelScale.m_pData[n];
                    float fTranslateLo = 0.0f;
                    if (m_lstChannelTranslate.m_count > n)
                        fTranslateLo = m_lstChannelTranslate.m_pData[n];
                    unsigned char uValue = 0;
                    if (fScaleLo > 0.0f)
                        uValue = static_cast<unsigned char>(static_cast<int>(fValue * fScaleLo)) & 0x0F;
                    if (fTranslateLo > 0.0f)
                        uValue = (static_cast<unsigned char>(uValue + static_cast<int>(fTranslateLo * 15.0f))) & 0x0F;
                    pDstRow[x * nBPP + n / 2] = uValue;

                    float fScaleHi = 1.0f;
                    if (m_lstChannelScale.m_count > n + 1)
                        fScaleHi = m_lstChannelScale.m_pData[n + 1];
                    float fTranslateHi = 0.0f;
                    if (m_lstChannelTranslate.m_count > n + 1)
                        fTranslateHi = m_lstChannelTranslate.m_pData[n + 1];
                    if (fScaleHi > 0.0f) {
                        uValue = static_cast<unsigned char>(static_cast<int>(fValue * fScaleHi) << 4);
                        pDstRow[x * nBPP + n / 2] = uValue;
                    }
                    if (fTranslateHi > 0.0f) {
                        uValue = (static_cast<unsigned char>(uValue + (static_cast<int>(fTranslateHi * 15.0f) << 4))) & 0xF0;
                        pDstRow[x * nBPP + n / 2] = uValue;
                    }
                }
            }
        }
    }
    return pDst;
}

// Engine-scope globals, declared at the point of use because no header in src/ declares them.
// texcurrentframe is the renderer's frame counter, DeltaT the last frame's duration, and the rest
// are the water and ring-distort tuning knobs.
extern int   texcurrentframe;
extern float DeltaT;
extern int   lowqualityblur;
int lowqualityblur2 = 1;
int enablerandomimpulses = 1;
int filterwater = 1;
float amplitude = 0.0299999993f;
float frequency1 = 60.0f;
float frequency2 = 25.0f;
float distortionspeed = 1.0f;
#if KOTOR1  // K1 clamps with the libm pair and converts through a call
int FloatToIntSSE(float f);
#endif
// The clamps below are if-statements, not fminf/fmaxf.
void ImageLowpassBlur(unsigned char*, int, int, int);
void ImageLowpassLowQualityBlur(unsigned char*, unsigned char*, int, int, int);
void ImageLowpassLowQualityBlur2(unsigned char*, int, int);
unsigned char* ImageBuildPerlin(int nSize, float fScale);
// The 4th argument is a mode flag, constant 0 at its only call site.
unsigned char* ImageBuildParameterPerlin(int nSize, float fScale, int nMaxChannel, int nBase);
int ImageGetPerlinPixel(unsigned char** ppImages, int nX, int nY, int nBase, int nCount,
                        int nDim);

// g_nArturoPass is the interlace pass (0 and 1 each redraw half the rows, 2 uploads) and
// g_fArturoTime the renderer-time accumulator. The pass starts at -1, so the first Control() call
// runs pass 0.
static int   g_nArturoPass = -1;
static float g_fArturoTime = 0.0f;

void WaterTextureController::ParseField(char* pszLine)
{
    TextureController::ParseField(pszLine);
    if (!_stricmp("forcecyclespeed", firstword(pszLine)))
        Parse(pszLine + strlen("forcecyclespeed"), m_fForceCycleSpeed);
    if (!_stricmp("anglecyclespeed", firstword(pszLine)))
        Parse(pszLine + strlen("anglecyclespeed"), m_fAngleCycleSpeed);
    if (!_stricmp("waterwidth", firstword(pszLine)))
        Parse(pszLine + strlen("waterwidth"), m_nWaterWidth);
    if (!_stricmp("waterheight", firstword(pszLine)))
        Parse(pszLine + strlen("waterheight"), m_nWaterHeight);
}

void WaterTextureController::Init()
{
    int nWidth = m_nWaterWidth;
    int nHeight = m_nWaterHeight;
    int nSize = nHeight * nWidth;
    m_nFrameCounter = 0;
    if (m_nDistort != 0)
        SetImage();
    m_pfVelocities = new float[nSize];
    m_pfHeights = new float[nSize];
    m_pfNextHeights = new float[nSize];
    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                K2_ASSUME(nWidth * y + x < nSize);   // y < nHeight, x < nWidth
                m_pfVelocities[nWidth * y + x] = 0.0f;
                m_pfHeights[nWidth * y + x] = 127.0f;
            }
        }
    }
    int nTexWidth = m_pTexture->GetWidth();
    int nTexHeight = m_pTexture->GetHeight();
    int nTexSize = nTexHeight * nTexWidth;
    m_pImage = new unsigned char[nTexSize];
    m_pBlurImage = new unsigned char[nTexSize];
    m_fForcePhase = 0.0f;
    m_fAnglePhase = 0.0f;
}

void WaveTextureController::Init()
{
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nSize = nHeight * nWidth;
    if (m_nDistort != 0)
        SetImage();
    m_pfHeights = new float[nSize];
    m_pfPrevHeights = new float[nSize];
    unsigned char* pImage = static_cast<unsigned char*>(m_pTexture->GetImage());
    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                int n = nWidth * y + x;
                K2_ASSUME(n < nSize);   // y < nHeight, x < nWidth
                m_pfPrevHeights[n] = 127.0f;
                m_pfHeights[n] = pImage ? static_cast<float>(pImage[m_pTexture->GetBPP() * n]) : 127.0f;
                m_pfHeights[n] = static_cast<float>(rand_wincompatible() % 256);
            }
        }
    }
}

void WaveTextureController::ParseField(char* pszLine)
{
    TextureController::ParseField(pszLine);
}

// Runs the finite-difference wave solver, a 5-point Laplacian over m_pfHeights written into
// m_pfPrevHeights minus its old value, then a buffer swap, a random impulse, a clamp to [40, 215], and
// the conversion to a byte image. The impulse lands on the buffer the next frame reads.
void WaveTextureController::Control()
{
    if (m_nLastFrame == texcurrentframe)
        return;
    m_nLastFrame = texcurrentframe;

    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();

    if (nHeight >= 1) {
        float fDelta = (DeltaT < 0.04f) ? DeltaT : 0.04f;
        float fDeltaSq = fDelta * fDelta;
        float afKernel[9] = {
            0.0f, fDeltaSq, 0.0f,
            fDeltaSq, 2.0f - 4.0f * fDeltaSq, fDeltaSq,
            0.0f, fDeltaSq, 0.0f,
        };
        for (int y = 0; y < nHeight; y++) {
            if (nWidth >= 1) {
                for (int x = 0; x < nWidth; x++) {
                    int nIndex = nWidth * y + x;
                    float fSum = 0.0f;
                    for (int nY = 0; nY < 3; nY++) {
                        for (int nX = 0; nX < 3; nX++) {
                            int nNeighbor = ((y + nY + nWidth - 1) % nHeight) * nWidth
                                          + ((x + nX + nHeight - 1) % nWidth);
                            fSum = fSum + m_pfHeights[nNeighbor] * afKernel[nY * 3 + nX];
                        }
                    }
                    m_pfPrevHeights[nIndex] = fSum - m_pfPrevHeights[nIndex];
                }
            }
        }
    }

    // Ping-pong the two scratch buffers.
    float* pfTemp = m_pfHeights;
    m_pfHeights = m_pfPrevHeights;
    m_pfPrevHeights = pfTemp;

    if (enablerandomimpulses != 0) {
        int nRow = rand_wincompatible() % nHeight;
        int nCol = rand_wincompatible() % nWidth;
        m_pfPrevHeights[nRow * nWidth + nCol] += static_cast<float>(rand_wincompatible() % 32);
    }

    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                float fValue = m_pfHeights[nWidth * y + x];
                if (fValue < 40.0f)
                    fValue = 40.0f;
                if (fValue > 215.0f)
                    fValue = 215.0f;
                m_pfHeights[nWidth * y + x] = fValue;
            }
        }
    }

    unsigned char* pImage = new unsigned char[nHeight * nWidth];
    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                pImage[nWidth * y + x] = static_cast<unsigned char>(static_cast<int>(m_pfHeights[nWidth * y + x]));
            }
        }
    }

    bool bKeep = m_pTexture->IsBumpMap();
    if (m_nDistort != 0) {
        pImage = Distort(pImage);
        bKeep = true;
    } else {
        pImage = Height2Channel(pImage, 0);
        bKeep = false;
    }
    m_pTexture->glImage(true, pImage);
    if (!bKeep)
        delete[] pImage;
}

void PerlinTextureController::ParseField(char* pszLine)
{
    TextureController::ParseField(pszLine);
}

void ArturoTextureController::ParseField(char* pszLine)
{
    TextureController::ParseField(pszLine);
    if (!_stricmp("arturowidth", firstword(pszLine)))
        Parse(pszLine + strlen("arturowidth"), m_nArturoWidth);
    if (!_stricmp("arturoheight", firstword(pszLine)))
        Parse(pszLine + strlen("arturoheight"), m_nArturoHeight);
}

void CycleTIDTextureController::ParseField(char* pszLine)
{
    if (!_stricmp("fps", firstword(pszLine)))
        Parse(pszLine + strlen("fps"), m_fFPS);
    TextureController::ParseField(pszLine);
}

void HeatDistortTextureController::Init()
{
}

void HeatDistortTextureController::Control()
{
}

// Advances the accumulated time and, when it passes one frame at m_fFPS, steps the texture-ID
// index on.
void CycleTIDTextureController::Control()
{
    if (m_nLastFrame != texcurrentframe) {
        m_nLastFrame = texcurrentframe;
        m_fFrameTime = DeltaT + m_fFrameTime;
        if (m_fFrameTime > 1.0f / m_fFPS) {
            m_fFrameTime = 0.0f;
            int nFrame = m_nTIDIndex + 1;
            int nCount = m_pTexture->GetNumX() * m_pTexture->GetNumY();
            m_nTIDIndex = nFrame - (nFrame / nCount) * nCount;
            m_pTexture->IncrementTIDIndex(m_nTIDIndex);
        }
    }
}

void RandomTextureController::Init()
{
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nBPP = m_pTexture->GetBPP();
    if (m_nDistort != 0)
        SetImage();
    int nSize = nWidth * nHeight;
    m_pNoise = new unsigned char[nSize];
    m_pImage = new unsigned char[nSize * nBPP];
}

void RingDistortTextureController::Init()
{
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nBPP = m_pTexture->GetBPP();
    int nSize = nHeight * nWidth * nBPP;
    m_fPhase = 0.0f;
    m_pImage = new unsigned char[nSize];
}

void RandomTextureController::Control()
{
    if (m_nLastFrame != texcurrentframe) {
        m_nLastFrame = texcurrentframe;
        if (m_pImage != 0) {
            int nWidth = m_pTexture->GetWidth();
            int nHeight = m_pTexture->GetHeight();
            for (int64_t y = 0; y < nHeight; y++) {
                for (int x = 0; x < nWidth; x++) {
                    m_pNoise[nWidth * y + x] = static_cast<unsigned char>(rand_wincompatible());
                }
            }
            unsigned char* pImage;
            if (m_nDistort != 0) {
                pImage = Distort(m_pNoise);
            } else {
                pImage = Height2Channel(m_pNoise, m_pImage);
                m_pImage = pImage;
            }
            m_pTexture->glImage(true, pImage);
        }
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void DirtyTextureController::Control()
{
    if (m_nLastFrame != texcurrentframe && m_pImage != 0)
        m_nLastFrame = texcurrentframe;
}
#endif // !KOTOR1

void PerlinTextureController::Init()
{
    if (m_nDistort != 0)
        SetImage();
    m_fBlend = 0.0f;
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nSize = nWidth * nHeight;
    m_pPrevNoise = new unsigned char[nSize];
    m_pNextNoise = new unsigned char[nSize];
    BuildPerlinImage(m_pPrevNoise, m_pTexture->GetWidth());
    BuildPerlinImage(m_pNextNoise, m_pTexture->GetWidth());
}

// Builds the octave pyramid for a square nDim image and evaluates it with ImageGetPerlinPixel.
void PerlinTextureController::BuildPerlinImage(unsigned char* pDst, int nDim)
{
    int64_t nCount = 0;
    uint64_t nBytes = 0;
    do {
        nCount++;
        nBytes += sizeof(unsigned char*);
    } while (4 << nCount <= nDim);
    unsigned char** ppImages = reinterpret_cast<unsigned char**>(new char[nBytes]);

    int nRes = 4;
    float fScale = 1.0f;
    for (int64_t i = 0; i < nCount; i++) {
        ppImages[i] = ImageBuildPerlin(nRes, fScale);
        nRes <<= 1;
        fScale *= 0.5f;
    }

    if (nDim >= 1) {
        for (int64_t nY = 0; nY < nDim; nY++) {
            for (int64_t nX = 0; nX < nDim; nX++)
                pDst[nX] = static_cast<unsigned char>(ImageGetPerlinPixel(ppImages, static_cast<int>(nY), static_cast<int>(nX), 4, static_cast<int>(nCount), nDim));
            pDst += nDim;
        }
    }

    unsigned char** pp = ppImages;
    for (int i = static_cast<int>(nCount); i != 0; i--) {
        if (*pp != 0)
            delete[] *pp;
        pp++;
    }
    delete[] ppImages;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
// Generates the dirty-glass image. nTextureSize is the cube dimension, the three temporary byte
// layers are the noise octaves MakePerlinTexture builds when nFunction == 1, and each texel byte
// is nValue % channel, where nValue is the sum of the three layers (or a raw random byte).
//
// Texel (A, B, C) reads L0[A*nHeight + B] + L1[A*nHeight + C] + L2[B*nWidth + C]. The three
// layers are indexed asymmetrically, and nWidth/nHeight are the texture's own dimensions read
// back after the three setters, not nTextureSize. The destination is a flat byte counter that
// runs across the whole image.
void DirtyTextureController::Init(int nColor, int nTextureSize, int nFormula, int nFunction)
{
    int nBPP = m_pTexture->GetBPP();
    m_nReserved1 = 0;
    m_nFormula = static_cast<char>(nFormula);
    int nLayerSize = nTextureSize * nTextureSize;
    unsigned char* pLayer0 = new unsigned char[nLayerSize];
    unsigned char* pLayer1 = new unsigned char[nLayerSize];
    unsigned char* pLayer2 = new unsigned char[nLayerSize];
#ifdef K2_FIX_ENGINE_BUGS
    // The texel loop below writes four bytes per texel whatever the base texture's depth.
    int nTexelBytes = nBPP < 4 ? 4 : nBPP;
    m_pImage = new unsigned char[nLayerSize * nTextureSize * nTexelBytes];
#else
    m_pImage = new unsigned char[nLayerSize * nTextureSize * nBPP];
#endif
    m_pTexture->SetDepth(nTextureSize);
    m_pTexture->SetHeight(nTextureSize);
    m_pTexture->SetWidth(nTextureSize);
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nRed = (nColor >> 16) & 0xFF;
    int nGreen = (nColor >> 8) & 0xFF;
    int nBlue = nColor & 0xFF;
    m_pTexture->SetDepth(m_pTexture->GetHeight());
    int nMaxChannel = nRed > nGreen ? nRed : nGreen;
    if (nBlue > nMaxChannel)
        nMaxChannel = nBlue;
    if (nFunction == 1) {
        MakePerlinTexture(pLayer0, nMaxChannel);
        MakePerlinTexture(pLayer1, nMaxChannel);
        MakePerlinTexture(pLayer2, nMaxChannel);
    }
    int nAlpha = nColor >> 24;
    if (nHeight >= 1) {
        int nDest = 0;
        unsigned char* pLayer1Row = pLayer1;
        for (int64_t nA = 0; nA < nHeight; nA++) {
            if (nWidth >= 1) {
                unsigned char* pLayer2Row = pLayer2;
                for (int64_t nB = 0; nB < nWidth; nB++) {
                    int64_t nLayer0Index = nA * nHeight + nB;
                    unsigned char* p1 = pLayer1Row;
                    unsigned char* p2 = pLayer2Row;
                    int64_t nC = 0;
                    do {
#ifdef K2_FIX_ENGINE_BUGS
                        // the setters above made the texture nTextureSize on each side (or left
                        // it 0 x 0), so the layers hold nTextureSize^2 bytes and the image
                        // nTextureSize^3 texels of four bytes
                        K2_ASSUME(nLayer0Index < nLayerSize);
                        K2_ASSUME(nDest + nC * 4 + 3 < nLayerSize * nTextureSize * nTexelBytes);
#endif
                        int nValue = nFunction == 1
                            ? pLayer0[nLayer0Index] + *p1 + *p2
                            : (rand_wincompatible() & 0xFF);
                        m_pImage[nDest + nC * 4 + 3] = static_cast<unsigned char>(nAlpha != 0 ? (nValue % nAlpha) : 0);
                        m_pImage[nDest + nC * 4] = static_cast<unsigned char>(nRed != 0 ? (nValue % nRed) : 0);
                        m_pImage[nDest + nC * 4 + 1] = static_cast<unsigned char>(nGreen != 0 ? (nValue % nGreen) : 0);
                        m_pImage[nDest + nC * 4 + 2] = static_cast<unsigned char>(nBlue != 0 ? (nValue % nBlue) : 0);
                        p1++;
                        p2++;
                        nC++;
                    } while (nC < nWidth);
                    nDest += nWidth * 4;
                    pLayer2Row += nWidth;
                }
            }
            pLayer1Row += nHeight;
        }
    }
    delete[] pLayer0;
    delete[] pLayer1;
    delete[] pLayer2;
    m_pTexture->glImage(false, m_pImage);
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
// The parameterised twin of PerlinTextureController::BuildPerlinImage: same octave pyramid and
// pixel pass, but the dimension comes from GetHeight() and the image buffer is a parameter.
void DirtyTextureController::MakePerlinTexture(unsigned char* pImage, int nMaxChannel)
{
    m_pTexture->GetWidth();  // result discarded
    int nHeight = m_pTexture->GetHeight();

    int64_t nCount = 0;
    uint64_t nBytes = 0;
    do {
        nCount++;
        nBytes += sizeof(unsigned char*);
    } while (4 << nCount <= nHeight);
    unsigned char** ppImages = reinterpret_cast<unsigned char**>(new char[nBytes]);

    int nRes = 4;
    float fScale = 1.0f;
    for (int64_t i = 0; i < nCount; i++) {
        ppImages[i] = ImageBuildParameterPerlin(nRes, fScale, nMaxChannel, 0);
        nRes <<= 1;
        fScale *= 0.5f;
    }

    if (nHeight >= 1) {
        for (int64_t nY = 0; nY < nHeight; nY++) {
            for (int64_t nX = 0; nX < nHeight; nX++)
                pImage[nX] = static_cast<unsigned char>(ImageGetPerlinPixel(ppImages, static_cast<int>(nY), static_cast<int>(nX), 4, static_cast<int>(nCount), nHeight));
            pImage += nHeight;
        }
    }

    unsigned char** pp = ppImages;
    for (int i = static_cast<int>(nCount); i != 0; i--) {
        if (*pp != 0)
            delete[] *pp;
        pp++;
    }
    delete[] ppImages;
}
#endif // !KOTOR1

void LifeTextureController::Init()
{
    char aColor[4] = {0, 0, 0, static_cast<char>(0xff)};
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nBPP = m_pTexture->GetBPP();
    int nSize = nHeight * nWidth * nBPP;
    unsigned char* pImage = static_cast<unsigned char*>(m_pTexture->GetImage());
    m_pCells = new unsigned char[nSize];
    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                aColor[0] = 0;
                aColor[1] = 0;
                aColor[2] = 0;
                aColor[3] = static_cast<char>(0xff);
                if (rand_wincompatible() % 100 >= 61)
                    aColor[0] = static_cast<char>(0xff);
                memcpy(pImage + (y * nWidth + x) * nBPP, aColor, nBPP);  // idiom-exempt: runtime length
                memcpy(m_pCells + (y * nWidth + x) * nBPP, aColor, nBPP);  // idiom-exempt: runtime length
            }
        }
    }
}

// A radial sine ripple written straight into the colour image, then one phase advance. Both
// channels use the same phase argument. The clamps are written min-then-max as if-statements.
void RingDistortTextureController::Control()
{
    if (m_nLastFrame != texcurrentframe && m_pImage != 0) {
        m_nLastFrame = texcurrentframe;
        int nWidth = m_pTexture->GetWidth();
        int nHeight = m_pTexture->GetHeight();
        m_pTexture->GetBPP();
        if (nHeight >= 1) {
            float fInvWidth = 1.0f / static_cast<float>(nWidth);
            int nOffset = 0;
            for (int y = 0; y < nHeight; y++) {
                float fY = (static_cast<float>(y) + 1.0f) * fInvWidth - 0.5f;
                float fYSq = fY * fY;
                for (int x = 0; x < nWidth; x++) {
                    float fX = (static_cast<float>(x) + 1.0f) * fInvWidth - 0.5f;
                    float fR = sqrtf(fX * fX + fYSq);
                    float fRVal = amplitude * cosineLUT(fR * frequency1 + m_fPhase * frequency2) + 0.5f;
                    float fGVal = amplitude * cosineLUT(fR * frequency1 + m_fPhase * frequency2) + 0.5f;
#if KOTOR1
                    // K1 clamps with fminf/fmaxf and converts through FloatToIntSSE.
                    float fR0 = fmaxf(fminf(fRVal * 255.0f, 255.0f), 0.0f);
                    float fG0 = fmaxf(fminf(fGVal * 255.0f, 255.0f), 0.0f);
                    unsigned char nR = static_cast<unsigned char>(FloatToIntSSE(fR0));
                    unsigned char nG = static_cast<unsigned char>(FloatToIntSSE(fG0));
                    m_pImage[nOffset + 2] = 0xFF;
                    m_pImage[nOffset + 1] = nG;
                    m_pImage[nOffset + 0] = nR;
                    m_pImage[nOffset + 3] = 0xFF;
#else
                    float fR0 = fRVal * 255.0f;
                    if (fR0 > 255.0f)
                        fR0 = 255.0f;
                    if (fR0 < 0.0f)
                        fR0 = 0.0f;
                    m_pImage[nOffset + 0] = static_cast<unsigned char>(fR0);
                    float fG0 = fGVal * 255.0f;
                    if (fG0 > 255.0f)
                        fG0 = 255.0f;
                    if (fG0 < 0.0f)
                        fG0 = 0.0f;
                    m_pImage[nOffset + 1] = static_cast<unsigned char>(fG0);
                    m_pImage[nOffset + 2] = 0xFF;
                    m_pImage[nOffset + 3] = 0xFF;
#endif
                    nOffset += 4;
                }
            }
        }
        m_pTexture->glImage(true, m_pImage);
        m_fPhase += distortionspeed * DeltaT;
        while (m_fPhase > 6.2831855f)
            m_fPhase += -6.2831855f;
    }
}

// A 5-point Laplacian relaxation over m_pfHeights, written to m_pfNextHeights with a random impulse, then
// the double-buffer rotation. The nHeight guard covers the loops only, so the rotation still runs
// without them.
void ArturoTextureController::BuildArturoImage()
{
    int nWidth = m_nArturoWidth;
    int nHeight = m_nArturoHeight;
    int nSize = nHeight * nWidth;
    int nRand0 = rand_wincompatible();
    int nRand1 = rand_wincompatible();
    if (nHeight >= 1) {
        int nXStart = nRand0 % nWidth;
        int nYStart = nRand1 % nHeight;
        for (int y = 0; y < nHeight; y++) {
            if (nWidth > 0) {
                for (int x = 0; x < nWidth; x++) {
                    float fSum = m_pfHeights[y * nWidth + (x - 1 + nWidth) % nWidth] + 0.0f
                               + m_pfHeights[y * nWidth + (x + 1) % nWidth]
                               + m_pfHeights[((y + nHeight - 1) % nHeight) * nWidth + x]
                               + m_pfHeights[((y + 1) % nHeight) * nWidth + x];
                    float fValue = fSum * 0.5f - m_pfPrevHeights[y * nWidth + x]
                                 + m_pfHeights[y * nWidth + x] * 0.0f;
                    if (x == nXStart && y == nYStart && enablerandomimpulses)
                        fValue += static_cast<float>(rand_wincompatible() % 256 - 128);
                    float fV = fValue + 127.0f;
#if KOTOR1  // K1 clamps with the libm pair, so a NaN takes the bound
                    fV = fmaxf(fminf(fV, 255.0f), 0.0f);
#else
                    if (fV > 255.0f)
                        fV = 255.0f;
                    if (fV < 0.0f)
                        fV = 0.0f;
#endif
                    m_pfNextHeights[y * nWidth + x] = fV - 127.0f;
                }
            }
        }
    }
    memcpy(m_pfPrevHeights, m_pfHeights, nSize * sizeof(float));
    memcpy(m_pfHeights, m_pfNextHeights, nSize * sizeof(float));
}

// Conway's Game of Life over the byte image, one generation per call. The previous image is read
// from a copy so the write-back can be in place, and the frame gate is a five-frame throttle.
//
// Each pixel's byte 2 carries the cell's age, raised by 4 when alive and lowered by 4 when dead.
void LifeTextureController::Control()
{
    if (m_nLastFrame > texcurrentframe - 5)
        return;
    m_nLastFrame = texcurrentframe;

    unsigned char aPixel[4] = { 0, 0, 0, 0xFF };

    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nBPP = m_pTexture->GetBPP();
    int nSize = nWidth * nHeight * nBPP;
    unsigned char* pNew = new unsigned char[nSize];
    memcpy(pNew, m_pCells, nSize);  // idiom-exempt: runtime length

    if (nHeight >= 1) {
        for (int y = 0; y < nHeight; y++) {
            if (nWidth >= 1) {
                for (int x = 0; x < nWidth; x++) {
                    int nCount = 0;
                    unsigned char nCentre = 0;
                    for (int nDY = -1; nDY <= 1; nDY++) {
                        for (int nDX = -1; nDX <= 1; nDX++) {
                            unsigned char nValue = pNew[(((y + nHeight + nDY) % nHeight) * nWidth
                                                       + ((x + nWidth + nDX) % nWidth)) * nBPP];
                            if (nDY == 0 && nDX == 0)
                                nCentre = nValue;
                            else if (nValue != 0)
                                nCount++;
                        }
                    }

                    int nIndex = (y * nWidth + x) * nBPP;
                    if (nCentre != 0)
                        nCount = nCount | 1;
                    bool bAlive = (nCount == 3);

                    if (bAlive) {
                        aPixel[0] = 0xFF;
                        int nHot = 0;
                        if (nBPP >= 3) {
                            nHot = m_pCells[nIndex + 2] + 4;
                            if (nHot > 255)
                                nHot = 255;
                        }
                        aPixel[1] = static_cast<unsigned char>(nHot);
                        aPixel[2] = static_cast<unsigned char>(nHot);
                    } else {
                        aPixel[0] = 0;
                        aPixel[1] = 0;
                        if (m_pCells[nIndex] != 0) {
                            aPixel[2] = 128;
                        } else if (nBPP >= 3) {
                            int nCold = m_pCells[nIndex + 2] - 4;
                            if (nCold < 0)
                                nCold = 0;
                            aPixel[2] = static_cast<unsigned char>(nCold);
                        }
                    }
                    memcpy(m_pCells + nIndex, aPixel, nBPP);  // idiom-exempt: runtime length
                }
            }
        }
    }

    m_pTexture->GetImage();
#if KOTOR1
    // K1 leaks the copy too, through a plain operator call.
    unsigned char* pTemp = static_cast<unsigned char*>(operator new[](nSize));
#else
    unsigned char* pTemp = new unsigned char[nSize];  // idiom-exempt: deliberately leaked
#endif
    memcpy(pTemp, m_pCells, nSize);                   // idiom-exempt: runtime length
    m_pTexture->glImage(true, 0);
    m_pTexture->FreeImage();
    delete[] pNew;
}

// A two-phase blend. m_fBlend advances by DeltaT * 4 and, whenever it passes 1, the previous
// octave is rotated into m_pPrevNoise and the next one regenerated through BuildPerlinImage; either
// way the frame ends by blending the two octaves into a fresh byte image.
void PerlinTextureController::Control()
{
    if (m_nLastFrame == texcurrentframe)
        return;
    m_nLastFrame = texcurrentframe;

    int nWidth = m_pTexture->GetWidth();
    int nSize = m_pTexture->GetWidth() * nWidth;

    m_fBlend = m_fBlend + DeltaT * 4.0f;
    if (m_fBlend > 1.0f) {
        memcpy(m_pPrevNoise, m_pNextNoise, nSize);  // idiom-exempt: runtime length
        BuildPerlinImage(m_pNextNoise, m_pTexture->GetWidth());
        while (m_fBlend >= 1.0f)
            m_fBlend = m_fBlend - 1.0f;
    }

    unsigned char* pNew = new unsigned char[nSize];
    for (int i = 0; i < nSize; i++)
        pNew[i] = static_cast<unsigned char>((1.0f - m_fBlend) * m_pPrevNoise[i] + m_fBlend * m_pNextNoise[i]);

    m_pTexture->GetImage();
    unsigned char* pImage;
    bool bKeep;
    if (m_nDistort != 0) {
        pImage = Distort(pNew);
        bKeep = true;
    } else {
        pImage = Height2Channel(pNew, 0);
        bKeep = false;
    }
    delete[] pNew;
    m_pTexture->glImage(true, pImage);
    if (!bKeep)
        delete[] pImage;
}

// Runs the water solver, a 3x3 stencil over the height field into m_pfNextHeights and m_pfVelocities, then a
// bilinear resample of the level into the texture image, the water filter chain, and the upload
// tail.
//
// The solver runs on a square m_nWaterWidth field, so m_nWaterHeight appears only in the
// resample. Both out-of-range clamp arms zero m_pfVelocities as well, and the tail's first arm tests
// GetBPP() == 1 where the Arturo tail tests IsBumpMap.
void WaterTextureController::Control()
{
    if (m_nLastFrame == texcurrentframe)
        return;
    m_nLastFrame = texcurrentframe;

    if (lowqualityblur == 0) {
        if (m_nFrameCounter & 1) {
            m_nFrameCounter = m_nFrameCounter + 1;
            return;
        }
        m_nFrameCounter = 0;
    }
    m_nCycle = (m_nCycle + 1) % 3;

    if (lowqualityblur == 0 || m_nCycle == 0) {
        float fDelta = (DeltaT < 0.04f ? DeltaT : 0.04f) * m_fSpeed;
        int nWidth = m_nWaterWidth;

        m_fForcePhase = m_fForcePhase + m_fForceCycleSpeed * fDelta / 57.295776f;
        if (m_fForcePhase > 6.2831855f)
            m_fForcePhase = m_fForcePhase - 6.2831855f;
        m_fAnglePhase = m_fAnglePhase + m_fAngleCycleSpeed * fDelta / 57.295776f;
        if (m_fAnglePhase > 6.2831855f)
            m_fAnglePhase = m_fAnglePhase - 6.2831855f;

        // The wave is driven by four phase-shifted cosine taps around a quarter-cycle: [0] is the
        // amplitude, [8] a quarter-cycle of it, and the four kernel corners are the ramps. All
        // four subtract their offset; with + they would land on the mirrored corners.
        float fAmplitude = cosineLUT(m_fForcePhase) * 0.25f + 0.2f;
        float fPhase = cosineLUT(m_fAnglePhase) + 0.0f;
        float afKernel[9] = { 0.5f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.5f };
        afKernel[0] = fAmplitude * cosineLUT(fPhase - 0.7853982f) + 0.5f;
        afKernel[2] = fAmplitude * cosineLUT(fPhase - 2.3561945f) + 0.5f;
        afKernel[8] = fAmplitude * cosineLUT(fPhase - 3.926991f) + 0.5f;
        afKernel[6] = fAmplitude * cosineLUT(fPhase - 5.4977875f) + 0.5f;

        int nImageWidth = m_pTexture->GetWidth();
        float fWaterHeight = static_cast<float>(m_nWaterHeight);
        int nTexHeight = m_pTexture->GetHeight();
        float fWaterWidth = static_cast<float>(m_nWaterWidth);
        int nTexWidth = m_pTexture->GetWidth();

        // The wave kernel is a 3x3 stencil over the height field, damped by m_pfVelocities.
        if (nWidth >= 1) {
            for (int y = 0; y < nWidth; y++) {
                for (int x = 0; x < nWidth; x++) {
                    int nIndex = y * nWidth + x;
                    float fLevel = m_pfVelocities[nIndex];
                    float fCenter = m_pfHeights[nIndex] - 127.0f;
                    float fSum = 0.0f;
                    for (int nY = 0; nY < 3; nY++) {
                        for (int nX = 0; nX < 3; nX++) {
                            if (nY == 1 && nX == 1) {
                                fSum = fSum - fCenter * afKernel[nY * 3 + nX];
                            } else {
                                int nNeighbor = ((y + nY + nWidth - 1) % nWidth) * nWidth
                                              + ((x + nX + nWidth - 1) % nWidth);
                                fSum = fSum + ((m_pfHeights[nNeighbor] - 127.0f) - fCenter)
                                            * afKernel[nY * 3 + nX];
                            }
                        }
                    }
                    fSum = fSum + fLevel * -0.025f;
                    m_pfVelocities[nIndex] = fLevel + fDelta * fSum;
                    m_pfNextHeights[nIndex] = (fDelta * fLevel + fCenter)
                                      + fSum * 0.5f * fDelta * fDelta;
                }
            }
        }

        // One random drop per frame.
        if (enablerandomimpulses != 0) {
            int nRow = rand_wincompatible() % nWidth;
            int nCol = rand_wincompatible() % nWidth;
            m_pfVelocities[nRow * nWidth + nCol] += static_cast<float>(rand_wincompatible() % 127);
        }

        // Centre the level on 127 and hold it inside [40, 215], zeroing m_pfVelocities wherever it clips.
        if (nWidth >= 1) {
            for (int y = 0; y < nWidth; y++) {
                for (int x = 0; x < nWidth; x++) {
                    int nIndex = y * nWidth + x;
                    float fValue = m_pfNextHeights[nIndex] + 127.0f;
                    if (fValue < 40.0f) {
                        m_pfVelocities[nIndex] = 0;
                        fValue = 40.0f;
                    } else if (fValue > 215.0f) {
                        m_pfVelocities[nIndex] = 0;
                        fValue = 215.0f;
                    }
                    m_pfHeights[nIndex] = fValue;
                }
            }
        }

        // Bilinear resample of the level into the texture image.
        {
            float fStepY = fWaterHeight / static_cast<float>(nTexHeight);
            float fStepX = fWaterWidth / static_cast<float>(nTexWidth);
            int nOffset = 0;
            for (int y = 0; y < m_pTexture->GetHeight(); y++) {
                float fFY = fStepY * static_cast<float>(y);
                if (nImageWidth >= 1) {
                    int nY0 = static_cast<int>(fFY - 0.5f);
                    float fWY1 = fFY - static_cast<float>(nY0);
                    float fWY0 = 1.0f - fWY1;
                    for (int x = 0; x < nImageWidth; x++) {
                        float fFX = fStepX * static_cast<float>(x);
                        int nX0 = static_cast<int>(fFX - 0.5f);
                        int nX1 = (nX0 + 1) % m_nWaterWidth;
                        int nY1 = (nY0 + 1) % m_nWaterHeight;
                        float fWX1 = fFX - static_cast<float>(nX0);
                        float fWX0 = 1.0f - fWX1;
                        int nRow0 = m_nWaterWidth * nY0;
                        int nRow1 = m_nWaterWidth * nY1;
                        float fValue = fWY0 * (fWX0 * m_pfHeights[nRow0 + nX0]
                                             + fWX1 * m_pfHeights[nRow0 + nX1])
                                     + fWY1 * (fWX0 * m_pfHeights[nRow1 + nX0]
                                             + fWX1 * m_pfHeights[nRow1 + nX1]);
                        unsigned char* pDest = (lowqualityblur == 0) ? m_pImage : m_pBlurImage;
                        pDest[nOffset + x] = static_cast<unsigned char>(static_cast<int>(fValue));
                    }
                }
                nOffset += nImageWidth;
            }
        }

        if (lowqualityblur != 0)
            memset(m_pImage, 0,
                   m_pTexture->GetHeight() * nImageWidth);  // idiom-exempt: runtime length
    }

    // The water filter chain.
    if (filterwater != 0) {
        if (lowqualityblur != 0) {
            ImageLowpassLowQualityBlur(m_pImage, m_pBlurImage, m_pTexture->GetWidth(),
                                       m_pTexture->GetHeight(), m_nCycle);
        } else if (lowqualityblur2 != 0) {
            ImageLowpassLowQualityBlur2(m_pImage, m_pTexture->GetWidth(), m_pTexture->GetHeight());
        } else {
            ImageLowpassBlur(m_pImage, m_pTexture->GetWidth(), m_pTexture->GetHeight(), 1);
        }
    }

    if (m_nCycle == 2 || lowqualityblur == 0) {
        unsigned char* pImage = m_pImage;
        bool bKeep = true;
        // This arm tests GetBPP() == 1; Arturo's tail tests IsBumpMap and Wave's discards it.
        if (m_pTexture->GetBPP() == 1) {
            pImage = m_pImage;
        } else if (m_nDistort != 0) {
            pImage = Distort(m_pImage);
        } else {
            pImage = Height2Channel(m_pImage, 0);
            bKeep = false;
        }
        m_pTexture->glImage(true, pImage);
        if (!bKeep)
            delete[] pImage;
        m_nFrameCounter = m_nFrameCounter + 1;
    }
}

// Two nested frame gates. g_fArturoTime advances by DeltaT and, when it passes 0.03f, wraps by
// that amount; the second gate is the frame counter. g_nArturoPass then selects the behaviour:
// pass 0 advances m_fBlend and rebuilds the image, passes 0 and 1 each redraw half the rows, and
// pass 2 uploads.
//
// The redraw is a bilinear resample of the two float buffers into the byte image over the
// m_nArturoWidth x m_nArturoHeight octave grid, not the texture's own dimensions; the +1 wrap applies to both
// axes while the base index cannot run off the edge.
void ArturoTextureController::Control()
{
    g_fArturoTime = g_fArturoTime + DeltaT;
    if (g_fArturoTime > 0.03f) {
    g_fArturoTime = g_fArturoTime - floorf(g_fArturoTime / 0.03f) * 0.03f;

    if (m_nLastFrame == texcurrentframe)
        return;
    m_nLastFrame = texcurrentframe;
    g_nArturoPass = (g_nArturoPass + 1) % 3;

    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    if (g_nArturoPass == 0) {
        m_fBlend = m_fBlend + DeltaT * m_fSpeed * 1.5f;
        if (m_fBlend > 1.0f) {
            BuildArturoImage();
            while (m_fBlend > 1.0f)
                m_fBlend = m_fBlend - 1.0f;
        }
    }

    if (m_nDistort != 0 && g_nArturoPass <= 1) {
        int nHalf = nHeight / 2;
        int nYStart = g_nArturoPass * nHalf;
        int nYEnd = (g_nArturoPass == 1) ? nHeight : nHalf * (g_nArturoPass + 1);
        if (nYStart < nYEnd) {
            float fStepX = static_cast<float>(m_nArturoWidth) / static_cast<float>(nWidth);
            float fStepY = static_cast<float>(m_nArturoHeight) / static_cast<float>(nHeight);
            int64_t nIndex = static_cast<int64_t>(nWidth) * nYStart;
            float fFY = fStepY * static_cast<float>(nYStart);
            for (int64_t nY = nYStart; nY < nYEnd; nY++) {
                if (nWidth >= 1) {
                    int nY0 = static_cast<int>(fFY - 0.5f);
                    float fWY1 = fFY - static_cast<float>(nY0);
                    float fWY0 = 1.0f - fWY1;
                    float fFX = 0.0f;
                    for (int x = 0; x < nWidth; x++) {
                        int nX0 = static_cast<int>(fFX - 0.5f);
                        int nX1 = (nX0 + 1) % m_nArturoWidth;
                        int nY1 = (nY0 + 1) % m_nArturoHeight;
                        float fWX1 = fFX - static_cast<float>(nX0);
                        float fWX0 = 1.0f - fWX1;
                        int nRow0 = m_nArturoWidth * nY0;
                        int nRow1 = m_nArturoWidth * nY1;
                        float f68 = fWY0 * (fWX0 * m_pfPrevHeights[nRow0 + nX0] + fWX1 * m_pfPrevHeights[nRow0 + nX1])
                                  + fWY1 * (fWX0 * m_pfPrevHeights[nRow1 + nX0] + fWX1 * m_pfPrevHeights[nRow1 + nX1]);
                        float f60 = fWY0 * (fWX0 * m_pfHeights[nRow0 + nX0] + fWX1 * m_pfHeights[nRow0 + nX1])
                                  + fWY1 * (fWX0 * m_pfHeights[nRow1 + nX0] + fWX1 * m_pfHeights[nRow1 + nX1]);
                        m_pImage[nIndex + x] = static_cast<unsigned char>(static_cast<int>(
                            (1.0f - m_fBlend) * f68 + m_fBlend * f60 + 127.0f));
                        fFX += fStepX;
                    }
                }
                fFY += fStepY;
                nIndex += nWidth;
            }
        }
    }

    if (g_nArturoPass == 2) {
        unsigned char* pImage = m_pImage;
        bool bKeep = true;
        if (m_pTexture->IsBumpMap()) {
            pImage = m_pImage;
        } else if (m_nDistort != 0) {
            pImage = Distort(m_pImage);
        } else {
            pImage = Height2Channel(m_pImage, 0);
            bKeep = false;
        }
        m_pTexture->glImage(true, pImage);
        if (!bKeep)
            delete[] pImage;
    }
    }
}

// Allocates the three float buffers and the byte image, writes a radial sine ramp into m_pfHeights
// while m_pfPrevHeights is zeroed, then builds the image twice.
void ArturoTextureController::Init()
{
    int nWidth = m_pTexture->GetWidth();
    int nHeight = m_pTexture->GetHeight();
    int nSize = nHeight * nWidth;
    int nCount = m_nArturoWidth * m_nArturoHeight;
    m_fBlend = 0.0f;
    if (m_nDistort != 0)
        SetImage();
    m_pfHeights = new float[nCount];
    m_pfPrevHeights = new float[nCount];
    m_pfNextHeights = new float[nCount];
    m_pImage = new unsigned char[nSize];

    int nDimY = m_nArturoHeight;
    if (nDimY >= 1) {
        for (int y = 0; y < nDimY; y++) {
            float fY = static_cast<float>(y) / static_cast<float>(nDimY) * 2.0f - 1.0f;
            float fYSq = fY * fY;
            for (int x = 0; x < m_nArturoWidth; x++) {
                float fX = static_cast<float>(x) / static_cast<float>(m_nArturoWidth) * 2.0f - 1.0f;
                float fR = sqrtf(fYSq + fX * fX);
                K2_ASSUME(y * m_nArturoWidth + x < nCount);   // y < m_nArturoHeight, x < m_nArturoWidth
                m_pfPrevHeights[y * m_nArturoWidth + x] = 0.0f;
                m_pfHeights[y * m_nArturoWidth + x] = sinf((1.0f - fR) * 3.14159274f) * 127.0f;
            }
        }
    }
    BuildArturoImage();
    BuildArturoImage();
}

