#include "aurorainterface.h"

#include "aurorainterface/aurmodel.h"
#include "aurmodelx.h"
#include "aurtexture.h"
#include "aur16bittexture.h"
#include "aurfonttexture.h"
#include "aurlayeredtexture.h"
#include "aurcompressedtexture.h"
#include "aurprocessedtexture.h"
#include "aurtxi.h"
#include "aurvisibility.h"
#include "restype.h"

CExoLinkedList<CAuroraModel> CAuroraInterface::m_lstRequestedModel;
CExoString CAuroraInterface::m_pCurrentTexturePack[4];

CAuroraTexture* CAuroraInterface::GetTextureInternal(const CExoString& sName) {
    CAuroraTexture* pTexture = new CAuroraTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TGA) {
            CAuroraTexture* pServiced =
                static_cast<CAuroraTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAurora16BitTexture* CAuroraInterface::Get16BitTextureInternal(const CExoString& sName) {
    CAurora16BitTexture* pTexture = new CAurora16BitTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_4PC) {
            CAurora16BitTexture* pServiced =
                static_cast<CAurora16BitTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAuroraFontTexture* CAuroraInterface::GetFontTextureInternal(const CExoString& sName) {
    CAuroraFontTexture* pTexture = new CAuroraFontTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_FNT) {
            CAuroraFontTexture* pServiced =
                static_cast<CAuroraFontTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAuroraLayeredTexture* CAuroraInterface::GetLayeredTextureInternal(const CExoString& sName) {
    CAuroraLayeredTexture* pTexture = new CAuroraLayeredTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_PLT) {
            CAuroraLayeredTexture* pServiced =
                static_cast<CAuroraLayeredTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAuroraCompressedTexture* CAuroraInterface::GetCompressedTextureInternal(const CExoString& sName) {
    CAuroraCompressedTexture* pTexture = new CAuroraCompressedTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_DDS) {
            CAuroraCompressedTexture* pServiced =
                static_cast<CAuroraCompressedTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAuroraProcessedTexture* CAuroraInterface::GetProcessedTextureInternal(const CExoString& sName) {
    CAuroraProcessedTexture* pTexture = new CAuroraProcessedTexture(CResRef(sName), 0);

    if (pTexture->m_pRes != 0 && pTexture->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TPC) {
            CAuroraProcessedTexture* pServiced =
                static_cast<CAuroraProcessedTexture*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTexture;
                pTexture = pServiced;
            }
        }
    }

    if (!pTexture->Load()) {
        if (pTexture->m_pRes == 0 || pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
        pTexture = 0;
    }
    return pTexture;
}

CAuroraTXI* CAuroraInterface::GetTXIInternal(const CExoString& sName) {
    CAuroraTXI* pTXI = new CAuroraTXI(CResRef(sName), 0);

    if (pTXI->m_pRes != 0 && pTXI->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTXI->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TXI) {
            CAuroraTXI* pServiced = static_cast<CAuroraTXI*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pTXI;
                pTXI = pServiced;
            }
        }
    }

    if (!pTXI->Load()) {
        if (pTXI->m_pRes == 0 || pTXI->m_pRes->GetDemands() <= 0)
            delete pTXI;
        pTXI = 0;
    }
    return pTXI;
}

CAuroraVisibility* CAuroraInterface::GetVisibilityInternal(const CExoString& sName) {
    CAuroraVisibility* pVisibility = new CAuroraVisibility(CResRef(sName), 0);

    if (pVisibility->m_pRes != 0 && pVisibility->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pVisibility->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_VIS) {
            CAuroraVisibility* pServiced =
                static_cast<CAuroraVisibility*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pVisibility;
                pVisibility = pServiced;
            }
        }
    }

    if (!pVisibility->Load()) {
        if (pVisibility->m_pRes == 0 || pVisibility->m_pRes->GetDemands() <= 0)
            delete pVisibility;
        pVisibility = 0;
    }
    return pVisibility;
}

CAuroraModelX* CAuroraInterface::GetModelXInternal(const CExoString& sName) {
    CAuroraModelX* pModel = new CAuroraModelX(CResRef(sName), 0);

    if (pModel->m_pRes != 0 && pModel->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pModel->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_MDX) {
            CAuroraModelX* pServiced = static_cast<CAuroraModelX*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                delete pModel;
                pModel = pServiced;
            }
        }
    }

    if (!pModel->Load()) {
        if (pModel->m_pRes == 0 || pModel->m_pRes->GetDemands() <= 0)
            delete pModel;
        pModel = 0;
    }
    return pModel;
}

CAuroraModel* CAuroraInterface::GetModelResHelper(const CExoString& sName, int bNoLoad,
                                                 int* pnCreated,
                                                 CExoLinkedListNode** ppNode) {
    CExoString sNameLower;
    CExoString sModelName;

    sNameLower = sName.LowerCase();

    // The queue is sorted by lowercased resref, so the walk stops at the first entry whose name is
    // >= the query: equal means already queued (reuse it), greater is where a new entry belongs.
    // The node pointer doubles as the list cursor.
    CExoLinkedListNode* pNode = m_lstRequestedModel.GetHeadPos();
    CAuroraModel* pModel = m_lstRequestedModel.GetHead();
    if (ppNode != 0)
        *ppNode = 0;

    while (pNode != 0) {
        if (pModel != 0) {
            pModel->m_resRef.CopyToString(sModelName);
            sModelName = sModelName.LowerCase();

            if (sModelName == sNameLower) {
                if (pnCreated != 0)
                    *pnCreated = 0;
                if (ppNode != 0)
                    *ppNode = 0;
                // A queue-only lookup returns the queued entry without touching the queue.
                if (bNoLoad != 0)
                    return pModel;
                m_lstRequestedModel.Remove(pNode);
                goto done;
            }
            if (sModelName > sNameLower) {
                if (pnCreated != 0)
                    *pnCreated = 1;
                if (ppNode != 0)
                    *ppNode = pNode;
                goto create;
            }
        }
        pModel = m_lstRequestedModel.GetNext(pNode);
    }
    if (pModel == 0)
        goto create;
    goto done;

create:
    pModel = new CAuroraModel(CResRef(sName), bNoLoad);

done:
    // A new wrapper can still turn out to duplicate a resource another holder has serviced; adopt
    // that object the same way the Get*Internal helpers do.
    if (pModel->m_pRes != 0 && pModel->m_pRes->GetDemands() >= 1) {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pModel->GetPtrToResHelperInfo());
        if (pInfo != 0 && pInfo->m_nResType == RESTYPE_MDL) {
            CAuroraModel* pServiced = static_cast<CAuroraModel*>(pInfo->m_pResHelper);
            if (pServiced != 0) {
                if (pnCreated != 0)
                    *pnCreated = 0;
                delete pModel;
                pModel = pServiced;
            }
        }
    }
    return pModel;
}

CAuroraModel* CAuroraInterface::GetModelInternal(const CExoString& sName, int bNoLoad,
                                                int* pnCreated,
                                                CExoLinkedListNode** ppNode) {
    CAuroraModel* pModel = GetModelResHelper(sName, bNoLoad, pnCreated, ppNode);
    if (pModel == 0)
        return 0;
    if (bNoLoad == 0) {
        if (!pModel->Load()) {
            if (pModel->m_pRes == 0 || pModel->m_pRes->GetDemands() <= 0)
                delete pModel;
            pModel = 0;
        }
    }
    return pModel;
}
void CAuroraInterface::ReleaseTextureInternal(CAuroraTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TGA &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::Release16BitTextureInternal(CAurora16BitTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_4PC &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::ReleaseFontTextureInternal(CAuroraFontTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_FNT &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::ReleaseLayeredTextureInternal(CAuroraLayeredTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_PLT &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::ReleaseCompressedTextureInternal(CAuroraCompressedTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_DDS &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::ReleaseProcessedTextureInternal(CAuroraProcessedTexture* pTexture) {
    if (pTexture->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTexture->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TPC &&
        pInfo->m_pResHelper == pTexture) {
        pTexture->Unload();
        if (pTexture->m_pRes->GetDemands() <= 0)
            delete pTexture;
    }
}

void CAuroraInterface::ReleaseTXIInternal(CAuroraTXI* pTXI) {
    if (pTXI->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pTXI->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_TXI &&
        pInfo->m_pResHelper == pTXI) {
        pTXI->Unload();
        if (pTXI->m_pRes->GetDemands() <= 0)
            delete pTXI;
    }
}

void CAuroraInterface::ReleaseVisibilityInternal(CAuroraVisibility* pVisibility) {
    if (pVisibility->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pVisibility->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_VIS &&
        pInfo->m_pResHelper == pVisibility) {
        pVisibility->Unload();
        if (pVisibility->m_pRes->GetDemands() <= 0)
            delete pVisibility;
    }
}

void CAuroraInterface::ReleaseModelInternal(CAuroraModel* pModel) {
    if (pModel->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pModel->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_MDL &&
        pInfo->m_pResHelper == pModel) {
        pModel->Unload();
        if (pModel->m_pRes->GetDemands() <= 0)
            delete pModel;
    }
}

void CAuroraInterface::ReleaseModelXInternal(CAuroraModelX* pModel) {
    if (pModel->m_pRes->GetDemands() < 1)
        return;

    CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pModel->GetPtrToResHelperInfo());
    if (pInfo != 0 && pInfo->m_nResType == RESTYPE_MDX &&
        pInfo->m_pResHelper == pModel) {
        pModel->Unload();
        if (pModel->m_pRes->GetDemands() <= 0)
            delete pModel;
    }
}
