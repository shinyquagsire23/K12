#include "aurtxi.h"
#include "restype.h"

// The helper info block holds a 2-byte resource type followed by the owning helper pointer.
enum {
    RESHELPERINFO_SIZE = 10,
};

CAuroraTXI::CAuroraTXI(CResRef resRef, int bRequest) {
    SetResRef(resRef, bRequest);
}

CAuroraTXI::~CAuroraTXI() {
}

int CAuroraTXI::GetTXIDataSize() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTXISize();
    return 0;
}

void* CAuroraTXI::GetTXIDataPtr() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return m_pRes->GetTXIDataPtr();
    return 0;
}

int CAuroraTXI::Load() {
    if (m_pRes->Demand() == 0)
        return 0;

    char* pData = static_cast<char*>(m_pRes->GetTXIDataPtr());
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pData - RESHELPERINFO_SIZE);
    pInfo->m_pResHelper = this;
    pInfo->m_nResType = RESTYPE_TXI;
    return 1;
}

void CAuroraTXI::Unload() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        m_pRes->Release();
}

void* CAuroraTXI::GetPtrToResHelperInfo() {
    if (m_pRes != 0 && m_pRes->GetDemands() > 0)
        return static_cast<char*>(GetTXIDataPtr()) - RESHELPERINFO_SIZE;
    return 0;
}
