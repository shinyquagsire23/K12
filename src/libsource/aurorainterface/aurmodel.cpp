#include "aurorainterface/aurmodel.h"
#include "restype.h"

// Info block this wrapper caches into, a 2-byte resource type then the helper pointer.
enum {
    RESHELPERINFO_SIZE = 10,
};

CAuroraModel::CAuroraModel(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraModel::~CAuroraModel() {
}

int CAuroraModel::GetModelDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetMDLSize();
    return 0;
}

void* CAuroraModel::GetModelDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetMDLDataPtr();
    return 0;
}

int CAuroraModel::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->GetMDLDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
#if KOTOR1
    SetResHelperInfo(pInfo, RESTYPE_MDL, this);
#else
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_MDL;
#endif
    return 1;
}

void CAuroraModel::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

void* CAuroraModel::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetModelDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
