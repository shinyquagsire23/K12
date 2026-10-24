#include "aur16bittexture.h"
#include "restype.h"

// Declared locally rather than through CResGFFFormat.h, whose inclusion would drag in that header's
// CExoFile / Vector / Quaternion set.
void ByteSwap(int* p);
void ByteSwap(float* p);

// The res-helper info block holds the two common fields plus 16 bytes of attribute cache for this
// wrapper.
enum {
    RESHELPERINFO_SIZE = 26,
};

CAurora16BitTexture::CAurora16BitTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAurora16BitTexture::~CAurora16BitTexture() {
}

int CAurora16BitTexture::GetTextureAttrib(unsigned int* pnWidth, unsigned int* pnHeight, float* pfAlphaTest,
                                         int* pnFormat) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->Get4PCAttrib(pnWidth, pnHeight, pfAlphaTest, pnFormat);
    return 0;
}

void* CAurora16BitTexture::GetTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->Get4PCDataPtr();
    return 0;
}

int CAurora16BitTexture::GetTextureDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->Get4PCSize();
    return 0;
}

int CAurora16BitTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->Get4PCDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_4PC;
    return 1;
}

void CAurora16BitTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

// Reads the 16-byte .4pc header without demanding the resource.
int CAurora16BitTexture::ReadTextureHeader(int* pnWidth, int* pnHeight, float* pfAlphaTest, int* pnFormat) {
    if (m_pRes == 0)
        return 0;

    Header4PC header;
    if (!m_pRes->ReadRaw(sizeof(header), reinterpret_cast<char*>(&header)))
        return 0;

    ByteSwap(reinterpret_cast<int*>(&header.m_nWidth));
    ByteSwap(reinterpret_cast<int*>(&header.m_nHeight));
    ByteSwap(&header.m_fAlphaTest);
    ByteSwap(reinterpret_cast<int*>(&header.m_nFormat));
    *pnWidth = header.m_nWidth;
    *pnHeight = header.m_nHeight;
    *pfAlphaTest = header.m_fAlphaTest;
    *pnFormat = header.m_nFormat;
    return 1;
}

void* CAurora16BitTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetTextureDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
