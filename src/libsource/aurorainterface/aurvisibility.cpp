#include "aurvisibility.h"
#include "restype.h"

// This wrapper caches nothing in the block, so it is just <nResType:2><pResHelper:8>.
enum {
    RESHELPERINFO_SIZE = 10,
};

CAuroraVisibility::CAuroraVisibility(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraVisibility::~CAuroraVisibility() {
}

int CAuroraVisibility::GetVisibilityDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetVISSize();
    return 0;
}

void* CAuroraVisibility::GetVisibilityDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetVISDataPtr();
    return 0;
}

int CAuroraVisibility::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->GetVISDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_VIS;
    return 1;
}

void CAuroraVisibility::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

void* CAuroraVisibility::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetVisibilityDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
