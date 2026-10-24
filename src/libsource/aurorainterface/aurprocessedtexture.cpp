#include "k2stdint.h"
#include "aurprocessedtexture.h"
#include "restype.h"

// K1's TPC format bit for a packed format whose data size covers the whole mip chain.
enum {
    TPC_FORMAT_PACKED_CHAIN = 0x10,
};

CAuroraProcessedTexture::CAuroraProcessedTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraProcessedTexture::~CAuroraProcessedTexture() {
}

int CAuroraProcessedTexture::GetProcessedTextureAttrib(int* pWidth, int* pHeight, char* pFormat,
                                                      int* pnCompressedSize, int* pnUncompressedSize, int* pTXISize,
                                                      float* pAlphaTest, char* pMipCount) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTPCAttrib(pWidth, pHeight, pFormat, pnCompressedSize, pnUncompressedSize, pTXISize,
                                    pAlphaTest, pMipCount);
    return 0;
}

void* CAuroraProcessedTexture::GetProcessedTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTPCDataPtr();
    return 0;
}

int CAuroraProcessedTexture::GetProcessedTextureDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTPCSize();
    return 0;
}

void* CAuroraProcessedTexture::GetProcessedTextureTXIPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTPCTXIPtr();
    return 0;
}

int CAuroraProcessedTexture::GetProcessedTextureTXISize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTPCTXISize();
    return 0;
}

// Loads the .tpc and installs its res-helper info block past the pixels and the TXI block. The
// resource reports only the compressed byte count, so the DXT mip chain is totalled here; a zero
// count means the payload is stored uncompressed.
int CAuroraProcessedTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    int nWidth, nHeight, nSize44, nSize40, nTXISize;
    char cFormat, cMipCount;
    float fAlphaTest;
    m_pRes->GetTPCAttrib(&nWidth, &nHeight, &cFormat, &nSize44, &nSize40, &nTXISize, &fAlphaTest,
                         &cMipCount);
#if KOTOR1  // K1 folds formats 5 and 6 onto 3 and 4
    if (cFormat == 5 || cFormat == 6)
        cFormat -= 2;
#endif

    // GetTPCAttrib decodes only formats 1, 3 and 4.
    if (cFormat != 1 && cFormat != 3 && cFormat != 4) {
        m_pRes->Release();
        return 0;
    }

    char* pData = static_cast<char*>(m_pRes->GetTPCDataPtr());
    if (pData == 0) {
        m_pRes->Release();
        return 0;
    }

    int nTextureSize;
    if (nSize44 != 0) {
        // A cube map is six square faces stacked vertically, so the ratio is both the cube-map test
        // and the face count.
        int nRatio = nHeight / nWidth;
        int nMipWidth = nWidth;
        int nMipHeight = (nRatio == 6) ? nHeight / 6 : nHeight;
        int nBlockBytes = (cFormat == 4) ? 16 : 8;
        int nTotal = nSize44;
        for (int i = 0; i < cMipCount - 1; i++) {
            nMipWidth >>= 1;
            nMipHeight >>= 1;
            nTotal += (nMipWidth + 3) / 4 * ((nMipHeight + 3) / 4) * nBlockBytes;
        }
        nTextureSize = nTotal * ((nRatio == 6) ? 6 : 1);
#if KOTOR1  // K1: a header format bit means the size field already covers the mip chain
        if (static_cast<TPCHeader*>(m_pRes->m_pTPCHeader)->m_formatBits & TPC_FORMAT_PACKED_CHAIN)
            nTextureSize = nSize44;
#endif
    } else {
        nTextureSize = nSize40;
    }

    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData + m_pRes->GetTPCTXISize() + nTextureSize + RESHELPERINFO_TRAILING_GAP);
    pInfo->m_nResType = RESTYPE_TPC;
    pInfo->m_pResHelper = this;
    return 1;
}

void CAuroraProcessedTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

// Peeks the header straight off the file without demanding the resource. The mip total it reports
// is the uncompressed size: a level costs width * height * bytes-per-pixel, and the format code
// doubles as the bytes-per-pixel.
int CAuroraProcessedTexture::ReadProcessedTextureHeader(int* pWidth, int* pHeight, char* pFormat,
                                                        int* pnCompressedSize, int* pnUncompressedSize,
                                                        float* pAlphaTest, char* pMipCount) {
    if (m_pRes == 0)
        return 0;

    TPCHeader header;
    if (!m_pRes->ReadRaw(sizeof(header), reinterpret_cast<char*>(&header)))
        return 0;

    *pWidth = header.m_width;
    *pHeight = header.m_height;
    *reinterpret_cast<unsigned int*>(pAlphaTest) = *reinterpret_cast<unsigned int*>(&header.m_fAlphaTest);
    unsigned char nMipCount = header.m_nMipCount;
    *pMipCount = static_cast<char>(nMipCount);

    unsigned char fmt = header.m_formatBits;
    if (fmt & 1)
        *pFormat = 1;
    else if (fmt & 2)
        *pFormat = 3;
    else if (fmt & 4)
        *pFormat = 4;

    unsigned int nDataSize = header.m_nDataSize;
    *pnUncompressedSize = 0;
    if (nDataSize != 0) {
        *pnCompressedSize = nDataSize;
    } else {
        *pnCompressedSize = 0;
        int nMipWidth = header.m_width;
        int nMipHeight = header.m_height;
        for (unsigned int i = 0; i < nMipCount; i++) {
            *pnUncompressedSize += nMipHeight * nMipWidth * (*pFormat);
            nMipWidth >>= 1;
            nMipHeight >>= 1;
        }
    }
    return 1;
}

void* CAuroraProcessedTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0) {
        char* pData = static_cast<char*>(GetProcessedTextureDataPtr());
        unsigned int nSize = GetTextureSize() + GetProcessedTextureTXISize();
        pData = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(pData) + nSize);
        return pData + RESHELPERINFO_TRAILING_GAP;
    }
    return 0;
}

// The same mip-chain total as Load().
int CAuroraProcessedTexture::GetTextureSize() {
    float fAlphaTest;
    int nWidth, nHeight, nSize44, nSize40, nTXISize;
#ifdef K2_FIX_ENGINE_BUGS
    nSize40 = 0;
#endif
    char cFormat, cMipCount;

    if (m_pRes != 0 && m_pRes->GetDemands() > 0) {
        m_pRes->GetTPCAttrib(&nWidth, &nHeight, &cFormat, &nSize44, &nSize40, &nTXISize,
                             &fAlphaTest, &cMipCount);
        if (nSize44 != 0) {
            int nRatio = nHeight / nWidth;
            int nMipWidth = nWidth;
            int nMipHeight = (nRatio == 6) ? nHeight / 6 : nHeight;
            int nBlockBytes = (cFormat == 4) ? 16 : 8;
            int nTotal = nSize44;
            for (int i = 0; i < cMipCount - 1; i++) {
                nMipWidth >>= 1;
                nMipHeight >>= 1;
                nTotal += (nMipWidth + 3) / 4 * ((nMipHeight + 3) / 4) * nBlockBytes;
            }
            return nTotal * ((nRatio == 6) ? 6 : 1);
        }
    }
    return nSize40;
}
