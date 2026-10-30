
#include "k2stdint.h"
#include "nwcanimbasehead.h"
#include "AURORA/aurmodel.h"
#include "nwcitem.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "port/libcbase.h"

#include "port/crtapi.h"
#include "port/libcstring.h"

#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBaseHead::CSWCAnimBaseHead()
{
    m_pHeadModel = 0;
    m_nAnimBaseType = ANIM_BASE_TYPE_HEAD;
    m_resTexture = const_cast<char*>("");
    m_bTextureDataCached = 0;
    m_nTextureCount = 0;
    m_nTextureOffset = 0;
    for (int i = 0; i < 10; i++)
        m_anTextureData[i] = 0;
    m_resEnvMap = const_cast<char*>("");
    m_bEnvMapEnabled = 0;
    m_headItemId = OBJECT_INVALID;
}

void CSWCAnimBaseHead::UnloadModel()
{
    if (m_pHeadModel != 0) {
        m_pHeadModel->AttachToObject(0, 0, 0);
        delete m_pHeadModel;
        m_pHeadModel = 0;
    }
    CSWCAnimBaseTW::UnloadModel();
}

void CSWCAnimBaseHead::UnloadModel(unsigned char which)
{
    if (which == ANIM_MODEL_HEAD && m_pHeadModel != 0) {
        m_pHeadModel->AttachToObject(0, 0, 0);
        delete m_pHeadModel;
        m_pHeadModel = 0;
        return;
    }
    CSWCAnimBaseTW::UnloadModel(which);
}

CSWCAnimBaseHead::~CSWCAnimBaseHead()
{
    UnloadModel();
}

void CSWCAnimBaseHead::SetHeadItem(unsigned long long itemId)
{
    m_headItemId = itemId;
}

// For the full model, forwards to the shared model on the virtual base; the head selector answers
// with the head's own model, and anything else has none.
Gob* CSWCAnimBaseHead::GetModel(unsigned char which)
{
    if (which == ANIM_MODEL_BODY)
        return CSWCAnimBaseTW::GetModel(ANIM_MODEL_BODY);
    if (which == ANIM_MODEL_HEAD)
        return static_cast<Gob*>(m_pHeadModel);
    return 0;
}

// EFFECTIVE: the head-less exit gets its own copy of the `return 1` block (tail duplication, 8 B).
int CSWCAnimBaseHead::LoadModel(CResRef const& cResRef, unsigned char which)
{
    if (which == ANIM_MODEL_BODY)
    {
        if (!CSWCAnimBaseTW::LoadModel(cResRef, ANIM_MODEL_BODY))
            return 0;
        if (m_pHeadModel == 0)
            return 1;
        m_pHeadModel->AttachToObject(m_pModel, const_cast<char*>("HEADHOOK"), 0);
    }
    else if (which == ANIM_MODEL_HEAD)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pHeadModel = NewCAurObject(szName, const_cast<char*>("head"), 0, 0);
        if (!m_pHeadModel)
            return 0;
        m_resTexture = cResRef;
        m_bTextureDataCached = 0;
        m_pHeadModel->AttachToObject(m_pModel, const_cast<char*>("HEADHOOK"), 0);
    }
    else if (which == ANIM_MODEL_WINGS || which == ANIM_MODEL_TAIL)
        return CSWCAnimBaseTW::LoadModel(cResRef, which) != 0;
    else
        return 0;
    return 1;
}

// Lower-cases the replacement resrefs into scratch buffers before they reach the model; bCache
// records the pair so RestoreTexture can undo it.
int CSWCAnimBaseHead::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, int bCache)
{
    char szOld[20];
    char szNew[20];
    sOld.CopyToString(szOld);
    sNew.CopyToString(szNew);
    _strlwr(szNew);
    _strlwr(szOld);
    if (m_pHeadModel)
        m_pHeadModel->ReplaceTexture(szOld, szNew);
    if (bCache)
    {
        m_resTexture = sNew;
        m_bTextureDataCached = 0;
    }
    return CSWCAnimBaseTW::ReplaceTexture(sOld, sNew, bCache);
}

int CSWCAnimBaseHead::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                     int bCache)
{
    if (which == ANIM_MODEL_HEAD)
    {
        if (!m_pHeadModel)
            return 0;
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pHeadModel->ReplaceTexture(szOld, szNew);
        if (bCache)
        {
            m_resTexture = sNew;
            m_bTextureDataCached = 0;
        }
        return 1;
    }
    return CSWCAnimBaseTW::ReplaceTexture(which, sOld, sNew, bCache);
}

int CSWCAnimBaseHead::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, unsigned int nCount,
                                     unsigned short* paData, int bCache, unsigned int nOffset)
{
    char szOld[20];
    char szNew[20];
    sOld.CopyToString(szOld);
    sNew.CopyToString(szNew);
    _strlwr(szOld);
    _strlwr(szNew);
    if (m_pHeadModel == 0)
        __builtin_trap();
    if (bCache)
    {
        m_resTexture = sNew;
        m_bTextureDataCached = 1;
        m_nTextureCount = static_cast<unsigned char>(nCount);
        m_nTextureOffset = static_cast<unsigned char>(nOffset);
        memcpy(m_anTextureData, paData + nOffset, nCount * sizeof(unsigned short));
    }
    return CSWCAnimBaseTW::ReplaceTexture(sOld, sNew, nCount, paData, bCache, nOffset);
}

int CSWCAnimBaseHead::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                     unsigned int nCount, unsigned short* paData, int bCache,
                                     unsigned int nOffset)
{
    int nResult = 0;
    if (which == ANIM_MODEL_HEAD)
    {
        if (m_pHeadModel)
        {
            char szOld[20];
            char szNew[20];
            sOld.CopyToString(szOld);
            sNew.CopyToString(szNew);
            _strlwr(szOld);
            _strlwr(szNew);
            m_pHeadModel->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
            nResult = 1;
            if (bCache)
            {
                m_resTexture = sNew;
                m_bTextureDataCached = nResult;
                m_nTextureCount = static_cast<unsigned char>(nCount);
                m_nTextureOffset = static_cast<unsigned char>(nOffset);
                memcpy(m_anTextureData, paData + nOffset, nCount * sizeof(unsigned short));
            }
        }
    }
    else
        nResult = CSWCAnimBaseTW::ReplaceTexture(which, sOld, sNew, nCount, paData, bCache, nOffset);
    return nResult;
}

// Undoes a cached replacement texture, else re-dispatches the replace with an empty new name.
int CSWCAnimBaseHead::RestoreTexture()
{
    if (m_pHeadModel)
    {
        if (m_bTextureDataCached)
        {
            CResRef cEmpty = "";
            ReplaceTexture(ANIM_MODEL_HEAD, cEmpty, m_resTexture, m_nTextureCount, m_anTextureData, 0,
                           m_nTextureOffset);
        }
        else
            m_pHeadModel->RestoreTexture();
    }
    return CSWCAnimBaseTW::RestoreTexture();
}

int CSWCAnimBaseHead::RestoreTexture(unsigned char which)
{
    if (which != ANIM_MODEL_HEAD)
        return CSWCAnimBaseTW::RestoreTexture(which);
    if (!m_pHeadModel)
        return 0;
    if (m_bTextureDataCached)
    {
        CResRef cEmpty = "";
        ReplaceTexture(ANIM_MODEL_HEAD, cEmpty, m_resTexture, m_nTextureCount, m_anTextureData, 0, m_nTextureOffset);
        return 1;
    }
    m_pHeadModel->RestoreTexture();
    return 1;
}

int CSWCAnimBaseHead::SetEnvMap(int nIndex, int bCache, CResRef const& cResRef)
{
    if (!m_pHeadModel)
        return 0;
    if (nIndex)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pHeadModel->SetEnvironmentMap(szName);
    }
    else
        m_pHeadModel->SetEnvironmentMap(const_cast<char*>(""));
    if (bCache)
    {
        m_resEnvMap = cResRef;
        m_bEnvMapEnabled = nIndex;
    }
    return CSWCAnimBaseTW::SetEnvMap(nIndex, bCache, cResRef);
}

// EFFECTIVE: the orig keeps a second `return 1` block for the bCache arm (tail duplication, 8 B).
int CSWCAnimBaseHead::SetEnvMap(unsigned char which, int nIndex, int bCache, CResRef const& cResRef)
{
    if (which != ANIM_MODEL_HEAD)
        return CSWCAnimBaseTW::SetEnvMap(which, nIndex, bCache, cResRef);
    if (!m_pHeadModel)
        return 0;
    if (nIndex)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pHeadModel->SetEnvironmentMap(szName);
    }
    else
        m_pHeadModel->SetEnvironmentMap(const_cast<char*>(""));
    if (bCache)
    {
        m_resEnvMap = cResRef;
        m_bEnvMapEnabled = nIndex;
    }
    return 1;
}

int CSWCAnimBaseHead::RestoreEnvMap()
{
    if (!m_pHeadModel)
        return 0;
    if (m_bEnvMapEnabled)
    {
        char szName[17];
        m_resEnvMap.CopyToString(szName);
        m_pHeadModel->SetEnvironmentMap(szName);
    }
    else
        m_pHeadModel->SetEnvironmentMap(const_cast<char*>(""));
    return CSWCAnimBaseTW::RestoreEnvMap();
}

void CSWCAnimBaseHead::TurnOnShadows()
{
    if (m_pHeadModel)
        m_pHeadModel->TurnOnShadows();
    CSWCAnimBaseTW::TurnOnShadows();
}

void CSWCAnimBaseHead::TurnOffShadows()
{
    if (m_pHeadModel)
        m_pHeadModel->TurnOffShadows();
    CSWCAnimBaseTW::TurnOffShadows();
}

int CSWCAnimBaseHead::EnableBumpedOutShell(CResRef const& cResRef, float fScale)
{
    if (!m_pHeadModel)
        return 0;
    char szName[17];
    cResRef.CopyToString(szName);
    _strlwr(szName);
    m_pHeadModel->EnableRenderBumpedOut(szName, fScale);
    return CSWCAnimBaseTW::EnableBumpedOutShell(cResRef, fScale);
}

int CSWCAnimBaseHead::DisableBumpedOutShell()
{
    if (!m_pHeadModel)
        return 0;
    m_pHeadModel->DisableRenderBumpedOut();
    return CSWCAnimBaseTW::DisableBumpedOutShell();
}

int CSWCAnimBaseHead::SetMotionBlur(int bMotionBlur)
{
    if (!m_pHeadModel)
        return 0;
    m_pHeadModel->SetMotionBlurred(bMotionBlur != 0);
    return CSWCAnimBaseTW::SetMotionBlur(bMotionBlur);
}

void CSWCAnimBaseHead::EnableDistortion()
{
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_headItemId);
    if (m_pHeadModel)
        m_pHeadModel->EnableDistortion();
    if (pItem != 0)
    {
        if (pItem->GetModel(ANIM_MODEL_BODY, 1) != 0)
            pItem->GetModel(ANIM_MODEL_BODY, 1)->EnableDistortion();
    }
    CSWCAnimBaseTW::EnableDistortion();
}

void CSWCAnimBaseHead::DisableDistortion()
{
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_headItemId);
    if (m_pHeadModel)
        m_pHeadModel->DisableDistortion();
    if (pItem != 0)
    {
        if (pItem->GetModel(ANIM_MODEL_BODY, 1) != 0)
            pItem->GetModel(ANIM_MODEL_BODY, 1)->DisableDistortion();
    }
    CSWCAnimBaseTW::DisableDistortion();
}

