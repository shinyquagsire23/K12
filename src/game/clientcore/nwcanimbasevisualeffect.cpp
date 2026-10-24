#include "nwcanimbasevisualeffect.h"
#include "exostring.h"
#include "AURORA/aurmodel.h"

#include "port/libcbase.h"
class CAurObject;
#if KOTOR1  // K1 streams models through SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBaseVisualEffect::CSWCAnimBaseVisualEffect()
{
    m_nAnimBaseType = 8;
}

int CSWCAnimBaseVisualEffect::LoadModel(const CResRef& resRef, unsigned char which)
{
    K2_UNUSED(which);
    char szName[20];
    resRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("visualeffect"), 0, 0);
    if (m_pModel == 0)
        return 0;
    m_resModelTexture = resRef;
    m_bModelTextureDataCached = 0;
    m_pModel->DisableFog();
    return 1;
}

CSWCAnimBaseVisualEffect::~CSWCAnimBaseVisualEffect()
{
    UnloadModel();
}

CExoString CSWCAnimBaseVisualEffect::GetAnimationName(unsigned short nAnim)
{
    static const char* const s_animNames[] = {"default", "impact", "duration", "cessation"};
    if (nAnim < 4)
    {
        short nIndex = static_cast<short>(nAnim);
        return CExoString(s_animNames[nIndex]);
    }
    return CExoString("");
}

int CSWCAnimBaseVisualEffect::SetAnimationInternal(CAurObject* pObject, unsigned short nAnim,
                                                   float fParam, int nParam, float fParam2)
{
    CExoString sName;
    int bResult;
    if (pObject != 0)
    {
        sName = GetAnimationName(nAnim);
        bResult = pObject->PlayAnimation(sName.CStr(), fParam, nParam, fParam2) != 0;
    }
    else
    {
        bResult = 0;
    }
    return bResult;
}
