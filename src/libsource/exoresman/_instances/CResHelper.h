#ifndef CRESHELPER_H
#define CRESHELPER_H

#include "exoresman.h"

extern CExoResMan* g_pExoResMan;

// The per-resource info block shared by all the CAurora* wrappers.
//
// Each wrapper's Load() stamps a back-reference into the slack ahead of the serviced resource's
// data buffer, and GetPtrToResHelperInfo() hands the block's start back out, which is how the
// renderer walks from a texture or model blob to the handle that owns it. The block is
// CResHelperInfo (exoresman.h) and holds the template's nResType followed by the wrapper's this
// pointer.
//
// Its size is per wrapper, not a family constant: wrappers that cache nothing need ten bytes,
// CAurora16BitTexture twenty-six for its four ReadTextureHeader values and
// CAuroraCompressedTexture thirty for its five. Each TU therefore declares its own
// `enum { RESHELPERINFO_SIZE = ... };`.
//
// CAuroraLayeredTexture places its block after the pixel payload instead of before the data, at
// `data + nWidth * nHeight * 2 + RESHELPERINFO_TRAILING_GAP`; the internal layout is the same. The
// .tga and processed (.tpc) wrappers do the same past their payloads.
enum { RESHELPERINFO_TRAILING_GAP = 4 };

// A templated resource handle fronting a lazily loaded CRes subclass (CResTPC, CResGFF, ...). It is
// a standalone class, not a CRes subclass. SetResRef(resRef, bRequest) rebinds the handle: it
// releases the old object through the global CExoResMan, then looks up or creates and registers the
// T for the new resref, optionally Request()ing it.
template <class T, unsigned short nResType>
class CResHelper {
public:
    CResHelper() {
        m_pRes = 0;
        m_resRef = const_cast<char*>("");
        m_bRequested = 0;
    }

    virtual ~CResHelper();

    void SetResRef(const CResRef& resRef, int bRequest);

    // Forward to the bound resource; 0 while nothing is bound.
    int Request()
    {
        if (m_pRes)
            return m_pRes->Request();
        return 0;
    }
    int CancelRequest()
    {
        if (m_pRes)
            return m_pRes->CancelRequest();
        return 0;
    }

    int     m_bRequested;
    T*      m_pRes;
    CResRef m_resRef;
};

template <class T, unsigned short nResType>
void CResHelper<T, nResType>::SetResRef(const CResRef& resRef, int bRequest) {
    if (resRef == m_resRef)
        return;

    if (m_pRes != 0 && m_resRef != const_cast<char*>("")) {
        if (m_bRequested)
            m_pRes->CancelRequest();
        if (!g_pExoResMan->ReleaseResObject(m_pRes)) {
            delete m_pRes;
            m_pRes = 0;
        }
    }

    if (resRef == const_cast<char*>("")) {
        m_pRes = 0;
        m_resRef = const_cast<char*>("");
        return;
    }

    T* pRes = static_cast<T*>(g_pExoResMan->GetResObject(resRef, nResType));
    if (pRes != 0) {
        m_pRes = pRes;
    } else {
        pRes = new T;
        g_pExoResMan->SetResObject(resRef, nResType, pRes);
        m_pRes = pRes;
    }
    if (bRequest) {
        m_bRequested = 1;
        pRes->Request();
    }
    m_resRef = resRef;
}

template <class T, unsigned short nResType>
CResHelper<T, nResType>::~CResHelper() {
    if (m_pRes != 0 && m_resRef != const_cast<char*>("")) {
        if (m_bRequested) {
            m_pRes->CancelRequest();
            m_bRequested = 0;
        }
        if (!g_pExoResMan->ReleaseResObject(m_pRes)) {
            delete m_pRes;
            m_pRes = 0;
        }
    }
}

#endif
