#include "aurmodelx.h"
#include "restype.h"

// This wrapper caches nothing in the resource-helper block, so it is just <nResType:2><pResHelper:8>.
enum {
    RESHELPERINFO_SIZE = sizeof(CResHelperInfo),
};

CAuroraModelX::CAuroraModelX(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraModelX::~CAuroraModelX() {
}

int CAuroraModelX::GetModelDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetMDXSize();
    return 0;
}

void* CAuroraModelX::GetModelDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetMDXDataPtr();
    return 0;
}

int CAuroraModelX::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->GetMDXDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
#if KOTOR1
    SetResHelperInfo(pInfo, RESTYPE_MDX, this);
#else
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_MDX;
#endif
    return 1;
}

void CAuroraModelX::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

void* CAuroraModelX::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetModelDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
