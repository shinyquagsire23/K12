#include "aurcompressedtexture.h"
#include "restype.h"

// Declared locally rather than through CResGFFFormat.h, whose inclusion would drag in that header's
// CExoFile / Vector / Quaternion set.
void ByteSwap(int* p);
void ByteSwap(float* p);

// The res-helper info block, the two common fields plus this wrapper's attribute cache.
enum {
    RESHELPERINFO_SIZE = 30,
};

CAuroraCompressedTexture::CAuroraCompressedTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraCompressedTexture::~CAuroraCompressedTexture() {
}

int CAuroraCompressedTexture::GetCompressedTextureAttrib(unsigned int* pWidth,
                                                        unsigned int* pHeight,
                                                        unsigned char* pBytesPerPixel,
                                                        unsigned int* pDataSize, float* pAlphaTest) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetDDSAttrib(pWidth, pHeight, pBytesPerPixel, pDataSize, pAlphaTest);
    return 0;
}

int CAuroraCompressedTexture::GetCompressedTextureDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetDDSSize();
    return 0;
}

void* CAuroraCompressedTexture::GetCompressedTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetDDSDataPtr();
    return 0;
}

int CAuroraCompressedTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->GetDDSDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_DDS;
    return 1;
}

void CAuroraCompressedTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

// Reads the 20-byte .dds header without demanding the resource: ReadRaw goes straight at the file,
// so the guard is only `m_pRes != 0`, with no GetDemands() test.
int CAuroraCompressedTexture::ReadTextureHeader(int* pnWidth, int* pnHeight, int* pnBytesPerPixel,
                                               int* pnDataSize, float* pfAlphaTest) {
    if (m_pRes == 0)
        return 0;

    DDSHeader header;
    if (!m_pRes->ReadRaw(sizeof(header), reinterpret_cast<char*>(&header)))
        return 0;

    ByteSwap(reinterpret_cast<int*>(&header.m_width));
    ByteSwap(reinterpret_cast<int*>(&header.m_height));
    ByteSwap(reinterpret_cast<int*>(&header.m_nBytesPerPixel));
    ByteSwap(reinterpret_cast<int*>(&header.m_nDataSize));
    ByteSwap(reinterpret_cast<float*>(&header.m_fAlphaTest));
    *pnWidth = header.m_width;
    *pnHeight = header.m_height;
    *pnBytesPerPixel = header.m_nBytesPerPixel;
    *pnDataSize = header.m_nDataSize;
    // Raw-copies the bits: m_fAlphaTest is an unsigned int, so assigning the value would convert it to
    // float instead of reinterpreting it.
    *reinterpret_cast<unsigned int*>(pfAlphaTest) = header.m_fAlphaTest;
    return 1;
}

void* CAuroraCompressedTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetCompressedTextureDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
