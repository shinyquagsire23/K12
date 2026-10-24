#include "nwcanimbasetrigger.h"
#include "exostring.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include <stdio.h>

#include "port/libcbase.h"
class CAurObject;
#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBaseTrigger::CSWCAnimBaseTrigger()
{
    m_nAnimBaseType = ANIM_BASE_TYPE_TRIGGER;
}

CSWCAnimBaseTrigger::~CSWCAnimBaseTrigger()
{
    UnloadModel();
}

int CSWCAnimBaseTrigger::LoadModel(const CResRef& resRef, unsigned char which)
{
    K2_UNUSED(which);
    char szName[20];
    resRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("trigger"), 0, 0);
    if (m_pModel == 0)
        return 0;
    m_resModelTexture = resRef;
    m_bModelTextureDataCached = 0;
    return 1;
}

// The warning message names CSWCAnimBasePlaceable, as the door and placeable siblings do.
unsigned short CSWCAnimBaseTrigger::ServerToClientAnimation(unsigned short nAnimation)
{
    unsigned short nResult;
    switch (nAnimation)
    {
    case SERVER_ANIMATION_MINE_HIDDEN: nResult = CLIENT_ANIMATION_TRIGGER_DEFAULT; break;
    case SERVER_ANIMATION_MINE_VISIBLE: nResult = CLIENT_ANIMATION_TRIGGER_DETECT; break;
    default:
        nResult = nAnimation;
        if (nAnimation >= SERVER_ANIMATION_PAUSE)  // the first server id
        {
            char szMsg[256];
            sprintf(szMsg, "CSWCAnimBasePlaceable::ServerToClientAnimation(): Failed to map "
                           "server anim %i to client anim.", static_cast<int>(nAnimation));
        }
        break;
    }
    return nResult;
}

CExoString CSWCAnimBaseTrigger::GetAnimationName(unsigned short nAnimation)
{
    unsigned short nAnimId = ServerToClientAnimation(nAnimation);
    CExoString sResult;
    if (!g_pRules->m_p2DArrays->m_pAnimations->GetCExoStringEntry(
            nAnimId, CTwoDimArrays::COL2DA_ANIMATIONS_NAME, &sResult))
        sResult = "";
    return sResult;
}
