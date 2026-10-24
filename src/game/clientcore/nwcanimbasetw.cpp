#include "nwcanimbasetw.h"
#include "AURORA/aurmodel.h"
#include "exostring.h"
#include <string.h>

#include "port/libcbase.h"

#include "port/crtapi.h"
#include "port/libcstring.h"

#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif


CSWCAnimBaseTW::~CSWCAnimBaseTW()
{
    if (m_pTail != 0) {
        m_pTail->AttachToObject(0, 0, 0);
        delete m_pTail;
        m_pTail = 0;
    }
    if (m_pWings != 0) {
        m_pWings->AttachToObject(0, 0, 0);
        delete m_pWings;
        m_pWings = 0;
    }
    CSWCAnimBase::UnloadModel();
}

CSWCAnimBaseTW::CSWCAnimBaseTW()
{
    // 11 marks this as the two-weapon anim base.
    m_nAnimBaseType = ANIM_BASE_TYPE_TAIL_WINGS;

    m_pTail = 0;
    m_pWings = 0;
    m_resTailTexture = const_cast<char*>("");
    m_bTailTexturePartial = 0;
    m_tailTextureCache.m_nCount = 0;
    m_tailTextureCache.m_nOffset = 0;
    memset(m_tailTextureCache.m_anData, 0, sizeof(m_tailTextureCache.m_anData));
    m_resWingsTexture = const_cast<char*>("");
    m_bWingsTexturePartial = 0;
    m_wingsTextureCache.m_nCount = 0;
    m_wingsTextureCache.m_nOffset = 0;
    memset(m_wingsTextureCache.m_anData, 0, sizeof(m_wingsTextureCache.m_anData));
    m_resTailEnvMap = const_cast<char*>("");
    m_bTailEnvMap = 0;
    m_resWingsEnvMap = const_cast<char*>("");
    m_bWingsEnvMap = 0;
}

// Returns the "tail" sub-object, the "wings" one, or for any other selector value the model itself.
Gob* CSWCAnimBaseTW::GetModel(unsigned char which)
{
    if (which == ANIM_MODEL_TAIL)
        return static_cast<Gob*>(m_pTail);
    if (which == ANIM_MODEL_WINGS)
        return static_cast<Gob*>(m_pWings);
    return CSWCAnimBase::GetModel(which);
}

// This is the only place the two sub-objects are created; one arm builds "tail", the other "wings",
// each remembering the resref it was built from. The third arm runs the base load and then
// re-attaches the already-built sub-objects; a failed base load aborts the whole call.
int CSWCAnimBaseTW::LoadModel(CResRef const& cResRef, unsigned char which)
{
    if (which == ANIM_MODEL_BODY)
    {
        if (!CSWCAnimBase::LoadModel(cResRef, ANIM_MODEL_BODY))
            return 0;
        if (m_pTail)
            m_pTail->AttachToObject(m_pModel, const_cast<char*>("tail"), 0);
        if (m_pWings)
            m_pWings->AttachToObject(m_pModel, const_cast<char*>("wings"), 0);
        return 1;
    }
    if (which == ANIM_MODEL_TAIL)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pTail = NewCAurObject(szName, const_cast<char*>("tail"), 0, 0);
        if (!m_pTail)
            return 0;
        m_resTailTexture = cResRef;
        m_bTailTexturePartial = 0;
        m_pTail->AttachToObject(m_pModel, const_cast<char*>("tail"), 0);
        return 1;
    }
    if (which == ANIM_MODEL_WINGS)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pWings = NewCAurObject(szName, const_cast<char*>("wings"), 0, 0);
        if (!m_pWings)
            return 0;
        m_resWingsTexture = cResRef;
        m_bWingsTexturePartial = 0;
        m_pWings->AttachToObject(m_pModel, const_cast<char*>("wings"), 0);
        return 1;
    }
    return 0;
}

// In the texture family every body applies the same operation to the two sub-objects and then to
// the model itself.

int CSWCAnimBaseTW::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, int bCache)
{
    if (m_pTail)
    {
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szNew);
        _strlwr(szOld);
        m_pTail->ReplaceTexture(szOld, szNew);
        if (bCache)
        {
            m_resTailTexture = sNew;
            m_bTailTexturePartial = 0;
        }
    }
    if (m_pWings)
    {
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szNew);
        _strlwr(szOld);
        m_pWings->ReplaceTexture(szOld, szNew);
        if (bCache)
        {
            m_resWingsTexture = sNew;
            m_bWingsTexturePartial = 0;
        }
    }
    return CSWCAnimBase::ReplaceTexture(sOld, sNew, bCache);
}

int CSWCAnimBaseTW::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                   int bCache)
{
    if (which == ANIM_MODEL_TAIL)
    {
        if (!m_pTail)
            return 0;
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pTail->ReplaceTexture(szOld, szNew);
        if (bCache)
        {
            m_resTailTexture = sNew;
            m_bTailTexturePartial = 0;
        }
        return 1;
    }
    if (which == ANIM_MODEL_WINGS)
    {
        if (!m_pWings)
            return 0;
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pWings->ReplaceTexture(szOld, szNew);
        if (bCache)
        {
            m_resWingsTexture = sNew;
            m_bWingsTexturePartial = 0;
        }
        return 1;
    }
    return CSWCAnimBase::ReplaceTexture(ANIM_MODEL_BODY, sOld, sNew, bCache);
}

int CSWCAnimBaseTW::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                   unsigned int nCount, unsigned short* paData, int bCache,
                                   unsigned int nOffset)
{
    int nResult = 0;
    if (which == ANIM_MODEL_TAIL)
    {
        if (m_pTail)
        {
            char szOld[20];
            char szNew[20];
            sOld.CopyToString(szOld);
            sNew.CopyToString(szNew);
            _strlwr(szOld);
            _strlwr(szNew);
            m_pTail->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
            nResult = 1;
            if (bCache)
            {
                m_resTailTexture = sNew;
                m_bTailTexturePartial = nResult;
                m_tailTextureCache.m_nCount = static_cast<unsigned char>(nCount);
                m_tailTextureCache.m_nOffset = static_cast<unsigned char>(nOffset);
                memcpy(m_tailTextureCache.m_anData, paData + nOffset, nCount * sizeof(unsigned short));
            }
        }
    }
    else if (which == ANIM_MODEL_WINGS)
    {
        if (m_pWings)
        {
            char szOld[20];
            char szNew[20];
            sOld.CopyToString(szOld);
            sNew.CopyToString(szNew);
            _strlwr(szOld);
            _strlwr(szNew);
            m_pWings->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
            nResult = 1;
            if (bCache)
            {
                m_resWingsTexture = sNew;
                m_bWingsTexturePartial = nResult;
                m_wingsTextureCache.m_nCount = static_cast<unsigned char>(nCount);
                m_wingsTextureCache.m_nOffset = static_cast<unsigned char>(nOffset);
                memcpy(m_wingsTextureCache.m_anData, paData + nOffset, nCount * sizeof(unsigned short));
            }
        }
    }
    else
        nResult = CSWCAnimBase::ReplaceTexture(ANIM_MODEL_BODY, sOld, sNew, nCount, paData, bCache, nOffset);
    return nResult;
}

int CSWCAnimBaseTW::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, unsigned int nCount,
                                   unsigned short* paData, int bCache, unsigned int nOffset)
{
    if (m_pTail)
    {
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pTail->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
        if (bCache)
        {
            m_resTailTexture = sNew;
            m_bTailTexturePartial = 1;
            m_tailTextureCache.m_nCount = static_cast<unsigned char>(nCount);
            m_tailTextureCache.m_nOffset = static_cast<unsigned char>(nOffset);
            memcpy(m_tailTextureCache.m_anData, paData + nOffset, nCount * sizeof(unsigned short));
        }
    }
    if (m_pWings)
    {
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pWings->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
        if (bCache)
        {
            m_resWingsTexture = sNew;
            m_bWingsTexturePartial = 1;
            m_wingsTextureCache.m_nCount = static_cast<unsigned char>(nCount);
            m_wingsTextureCache.m_nOffset = static_cast<unsigned char>(nOffset);
            memcpy(m_wingsTextureCache.m_anData, paData + nOffset, nCount * sizeof(unsigned short));
        }
    }
    return CSWCAnimBase::ReplaceTexture(sOld, sNew, nCount, paData, bCache, nOffset);
}

// With no selector, each live sub-object is restored and the base call still runs; the selector
// arm returns early for a sub-object that was never built.
int CSWCAnimBaseTW::RestoreTexture()
{
    if (m_pTail && m_bTailTexturePartial)
    {
        CResRef cEmpty = "";
        ReplaceTexture(ANIM_MODEL_TAIL, cEmpty, m_resTailTexture, m_tailTextureCache.m_nCount, m_tailTextureCache.m_anData, 0,
                       m_tailTextureCache.m_nOffset);
    }
    if (m_pWings && m_bWingsTexturePartial)
    {
        CResRef cEmpty = "";
        ReplaceTexture(ANIM_MODEL_WINGS, cEmpty, m_resWingsTexture, m_wingsTextureCache.m_nCount, m_wingsTextureCache.m_anData, 0,
                       m_wingsTextureCache.m_nOffset);
    }
    return CSWCAnimBase::RestoreTexture();
}

int CSWCAnimBaseTW::RestoreTexture(unsigned char which)
{
    if (which == ANIM_MODEL_TAIL)
    {
        if (!m_pTail)
            return 0;
        if (m_bTailTexturePartial)
        {
            CResRef cEmpty = "";
            ReplaceTexture(ANIM_MODEL_TAIL, cEmpty, m_resTailTexture, m_tailTextureCache.m_nCount, m_tailTextureCache.m_anData, 0,
                           m_tailTextureCache.m_nOffset);
        }
        else
            m_pTail->RestoreTexture();
        return 1;
    }
    if (which == ANIM_MODEL_WINGS)
    {
        if (!m_pWings)
            return 0;
        if (m_bWingsTexturePartial)
        {
            CResRef cEmpty = "";
            ReplaceTexture(ANIM_MODEL_WINGS, cEmpty, m_resWingsTexture, m_wingsTextureCache.m_nCount, m_wingsTextureCache.m_anData, 0,
                           m_wingsTextureCache.m_nOffset);
        }
        else
            m_pWings->RestoreTexture();
        return 1;
    }
    return CSWCAnimBase::RestoreTexture(which);
}

// Both sub-objects get the new environment map. The no-selector overload caches it into both, the
// selector one only into the sub-object it named.
int CSWCAnimBaseTW::SetEnvMap(int nIndex, int bCache, CResRef const& cResRef)
{
    if (nIndex)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        if (m_pTail)
            m_pTail->SetEnvironmentMap(szName);
        if (m_pWings)
            m_pWings->SetEnvironmentMap(szName);
    }
    else
    {
        if (m_pTail)
            m_pTail->SetEnvironmentMap(const_cast<char*>(""));
        if (m_pWings)
            m_pWings->SetEnvironmentMap(const_cast<char*>(""));
    }
    if (bCache)
    {
        m_resTailEnvMap = cResRef;
        m_bTailEnvMap = nIndex;
        m_resWingsEnvMap = cResRef;
        m_bWingsEnvMap = nIndex;
    }
    return CSWCAnimBase::SetEnvMap(nIndex, bCache, cResRef);
}

int CSWCAnimBaseTW::SetEnvMap(unsigned char which, int nIndex, int bCache, CResRef const& cResRef)
{
    char szName[17];
    if (which == ANIM_MODEL_TAIL)
    {
        if (m_pTail)
        {
            if (nIndex)
            {
                cResRef.CopyToString(szName);
                m_pTail->SetEnvironmentMap(szName);
            }
            else
                m_pTail->SetEnvironmentMap(const_cast<char*>(""));
            if (bCache)
            {
                m_resTailEnvMap = cResRef;
                m_bTailEnvMap = nIndex;
            }
        }
        return 1;
    }
    if (which == ANIM_MODEL_WINGS)
    {
        if (m_pWings)
        {
            if (nIndex)
            {
                cResRef.CopyToString(szName);
                m_pWings->SetEnvironmentMap(szName);
            }
            else
                m_pWings->SetEnvironmentMap(const_cast<char*>(""));
            if (bCache)
            {
                m_resWingsEnvMap = cResRef;
                m_bWingsEnvMap = nIndex;
            }
        }
        return 1;
    }
    return CSWCAnimBase::SetEnvMap(which, nIndex, bCache, cResRef);
}

int CSWCAnimBaseTW::RestoreEnvMap()
{
    if (m_pTail)
    {
        if (m_bTailEnvMap)
        {
            char szName[17];
            m_resTailEnvMap.CopyToString(szName);
            m_pTail->SetEnvironmentMap(szName);
        }
        else
            m_pTail->SetEnvironmentMap(const_cast<char*>(""));
    }
    if (m_pWings)
    {
        if (m_bWingsEnvMap)
        {
            char szName[17];
            m_resWingsEnvMap.CopyToString(szName);
            m_pWings->SetEnvironmentMap(szName);
        }
        else
            m_pWings->SetEnvironmentMap(const_cast<char*>(""));
    }
    return CSWCAnimBase::RestoreEnvMap();
}

void CSWCAnimBaseTW::UnloadModel()
{
    if (m_pTail)
    {
        m_pTail->AttachToObject(0, 0, 0);
        delete m_pTail;
        m_pTail = 0;
    }
    if (m_pWings)
    {
        m_pWings->AttachToObject(0, 0, 0);
        delete m_pWings;
        m_pWings = 0;
    }
    CSWCAnimBase::UnloadModel();
}

void CSWCAnimBaseTW::UnloadModel(unsigned char which)
{
    if (which == ANIM_MODEL_TAIL)
    {
        if (m_pTail)
        {
            m_pTail->AttachToObject(0, 0, 0);
            delete m_pTail;
            m_pTail = 0;
            return;
        }
    }
    else if (which == ANIM_MODEL_WINGS)
    {
        if (m_pWings)
        {
            m_pWings->AttachToObject(0, 0, 0);
            delete m_pWings;
            m_pWings = 0;
            return;
        }
    }
    CSWCAnimBase::UnloadModel(which);
}

// Both queued-animation bodies fetch the name once through GetAnimationName and replay it on every
// live sub-object, falling back to the literal "default" when a sub-object reports no such animation.

int CSWCAnimBaseTW::SetAnimationInternal(CAurObject* pObject, unsigned short nAnimation, float fSpeed,
                                         int nFlags, float fStart)
{
    if (fSpeed == 0.0f)
        fSpeed = 1.0f;
    if (m_pTail || m_pWings)
    {
        CExoString sName = GetAnimationName(nAnimation);
        if (m_pTail)
        {
            if (!m_pTail->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart))
                m_pTail->PlayAnimation(const_cast<char*>("default"), fSpeed, nFlags, fStart);
        }
        if (m_pWings)
        {
            if (!m_pWings->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart))
                m_pWings->PlayAnimation(const_cast<char*>("default"), fSpeed, nFlags, fStart);
        }
    }
    return CSWCAnimBase::SetAnimationInternal(pObject, nAnimation, fSpeed, nFlags, fStart);
}

// The "model itself" arm is the only one that reaches the base's animation body. An arm whose
// sub-object was never built reports failure without trying the "default" fallback, and the
// fallback's own result is discarded.
int CSWCAnimBaseTW::SetAnimationOnSubObjectInternal(CAurObject* pObject, unsigned char nSubObject,
                                                    unsigned short nAnimation, float fSpeed,
                                                    int nFlags, float fStart)
{
    int nResult = 0;
    CExoString sName = GetAnimationName(nAnimation);
    if (nSubObject == ANIM_MODEL_BODY)
        nResult = CSWCAnimBase::SetAnimationInternal(pObject, nAnimation, fSpeed, nFlags, fStart);
    else if (nSubObject == ANIM_MODEL_TAIL)
    {
        if (m_pTail)
        {
            nResult = m_pTail->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
            if (!nResult)
                m_pTail->PlayAnimation(const_cast<char*>("default"), fSpeed, nFlags, fStart);
        }
    }
    else if (nSubObject == ANIM_MODEL_WINGS)
    {
        if (m_pWings)
        {
            nResult = m_pWings->PlayAnimation(sName.CStr(), fSpeed, nFlags, fStart);
            if (!nResult)
                m_pWings->PlayAnimation(const_cast<char*>("default"), fSpeed, nFlags, fStart);
        }
    }
    return nResult;
}
