#include "aurfonttexture.h"
#include "restype.h"

// The FNT glyph table is a 24-byte header whose leading int is the glyph count, then 16-byte glyph
// records. The res-helper info block follows the array.
enum {
    FNT_HEADER_SIZE = 24,
    FNT_GLYPH_SIZE = 16,
};

CAuroraFontTexture::CAuroraFontTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraFontTexture::~CAuroraFontTexture() {
}

int CAuroraFontTexture::GetFontTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                                            unsigned char* pBitsPerPixel) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGAAttrib(pWidth, pHeight, pBitsPerPixel);
    return 0;
}

int CAuroraFontTexture::CopyFontData(unsigned char* pDst) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->CopyFNTData(pDst);
    return 0;
}

int CAuroraFontTexture::CopyFontTextureData(unsigned char* pDst) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->CopyTGAData(pDst);
    return 0;
}

void* CAuroraFontTexture::GetFontDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetFNTHeaderPtr();
    return 0;
}

void* CAuroraFontTexture::GetFontTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGADataPtr();
    return 0;
}

int CAuroraFontTexture::GetFontDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetFNTSize();
    return 0;
}

int CAuroraFontTexture::GetFontTextureDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTGASize();
    return 0;
}

int CAuroraFontTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    unsigned int nWidth, nHeight;
    unsigned char nBitsPerPixel;
    m_pRes->GetTGAAttrib(&nWidth, &nHeight, &nBitsPerPixel);

    // Only 24- and 32-bit font bitmaps.
    if (nBitsPerPixel != 24 && nBitsPerPixel != 32) {
        m_pRes->Release();
        return 0;
    }

    if (m_pRes->GetTGADataPtr() == 0) {
        m_pRes->Release();
        return 0;
    }

    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(m_pRes->GetFNTHeaderPtr())
                                              + *static_cast<int*>(m_pRes->GetFNTHeaderPtr()) * FNT_GLYPH_SIZE
                                              + FNT_HEADER_SIZE);
    pInfo->m_nResType = RESTYPE_FNT;
    pInfo->m_pResHelper = this;
    return 1;
}

void CAuroraFontTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

// The base pointer goes through the guarded GetFontDataPtr; the glyph count is read directly from
// the header.
void* CAuroraFontTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetFontDataPtr())
               + *static_cast<int*>(m_pRes->GetFNTHeaderPtr()) * FNT_GLYPH_SIZE + FNT_HEADER_SIZE;
    return 0;
}
