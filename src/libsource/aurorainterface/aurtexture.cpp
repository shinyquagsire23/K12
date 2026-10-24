#include "k2stdint.h"
#include "aurtexture.h"
#include "restype.h"

CAuroraTexture::CAuroraTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraTexture::~CAuroraTexture() {
}

int CAuroraTexture::GetTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                                    unsigned char* pBitsPerPixel) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGAAttrib(pWidth, pHeight, pBitsPerPixel);
    return 0;
}

void* CAuroraTexture::GetTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGADataPtr();
    return 0;
}

int CAuroraTexture::GetTextureDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGASize();
    return 0;
}

int CAuroraTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    unsigned int nWidth, nHeight;
    unsigned char nBitsPerPixel;
    m_pRes->GetTGAAttrib(&nWidth, &nHeight, &nBitsPerPixel);

    // Only palettised, 24-bit and 32-bit Targas are usable.
    if (nBitsPerPixel != 8 && nBitsPerPixel != 24 && nBitsPerPixel != 32) {
        m_pRes->Release();
        return 0;
    }

    unsigned char* pData = static_cast<unsigned char*>(m_pRes->GetTGADataPtr());
    if (pData == 0) {
        m_pRes->Release();
        return 0;
    }

    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(reinterpret_cast<char*>(pData) + nWidth * nHeight * (nBitsPerPixel / 8)
                                              + RESHELPERINFO_TRAILING_GAP);
    pInfo->m_nResType = RESTYPE_TGA;
    pInfo->m_pResHelper = this;
    return 1;
}

// Swaps the red and blue bytes of every pixel in place, then releases. The pixel loop is skipped
// rather than null-checked when the payload is absent, the count being zero in that case.
void CAuroraTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0) {
        unsigned int nWidth, nHeight;
        unsigned char nBitsPerPixel;
        m_pRes->GetTGAAttrib(&nWidth, &nHeight, &nBitsPerPixel);

        unsigned int nPixels = nWidth * nHeight;
        unsigned int nBytesPerPixel = nBitsPerPixel / 8;
        unsigned char* p = static_cast<unsigned char*>(m_pRes->GetTGADataPtr());
        if (nPixels != 0) {
            do {
                unsigned char nSwap = p[0];
                p[0] = p[2];
                p[2] = nSwap;
                p += nBytesPerPixel;
            } while (--nPixels != 0);
        }

        m_pRes->Release();
    }
}

int CAuroraTexture::ReadTextureHeader(int* pnWidth, int* pnHeight, int* pnBytesPerPixel) {
    if (m_pRes == 0)
        return 0;

    TGAHeader header;
    if (!m_pRes->ReadRaw(sizeof(header), reinterpret_cast<char*>(&header)))
        return 0;

    *pnWidth = header.m_width;
    *pnHeight = header.m_height;
    *pnBytesPerPixel = header.m_bitsPerPixel / 8;
    return 1;
}

void* CAuroraTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0) {
        unsigned int nWidth, nHeight;
        unsigned char nBitsPerPixel;
        char* pData = static_cast<char*>(GetTextureDataPtr());
        GetTextureAttrib(&nWidth, &nHeight, &nBitsPerPixel);
        int64_t nSize = nWidth * nHeight * (nBitsPerPixel / 8);
        pData = reinterpret_cast<char*>(reinterpret_cast<intptr_t>(pData) + nSize);
        return pData + RESHELPERINFO_TRAILING_GAP;
    }
    return 0;
}
