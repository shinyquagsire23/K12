#include "k2stdint.h"
#include "aurlayeredtexture.h"
#include "restype.h"

enum { PLT_BYTES_PER_PIXEL = 2 };

CAuroraLayeredTexture::CAuroraLayeredTexture(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraLayeredTexture::~CAuroraLayeredTexture() {
}

int CAuroraLayeredTexture::GetTextureAttrib(unsigned int* pWidth, unsigned int* pHeight,
                                           unsigned int* pNumLayers) {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetPLTAttrib(pWidth, pHeight, pNumLayers);
    return 0;
}

void* CAuroraLayeredTexture::GetTextureDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetPLTDataPtr();
    return 0;
}

int CAuroraLayeredTexture::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    unsigned int nWidth, nHeight, nNumLayers;
    m_pRes->GetPLTAttrib(&nWidth, &nHeight, &nNumLayers);

    char* pData = static_cast<char*>(m_pRes->GetPLTDataPtr());
    if (pData == 0) {
        m_pRes->Release();
        return 0;
    }

    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData + nWidth * nHeight * PLT_BYTES_PER_PIXEL + RESHELPERINFO_TRAILING_GAP);
    pInfo->m_nResType = RESTYPE_PLT;
    pInfo->m_pResHelper = this;
    return 1;
}

void CAuroraLayeredTexture::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

void* CAuroraLayeredTexture::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0) {
        unsigned int nWidth, nHeight, nNumLayers;
        char* pData = static_cast<char*>(GetTextureDataPtr());
        GetTextureAttrib(&nWidth, &nHeight, &nNumLayers);
        int64_t nSize = nWidth * nHeight * PLT_BYTES_PER_PIXEL;
        pData = reinterpret_cast<char*>(reinterpret_cast<intptr_t>(pData) + nSize);
        return pData + RESHELPERINFO_TRAILING_GAP;
    }
    return 0;
}
