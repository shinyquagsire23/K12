#include "nwcanimbasedoor.h"
#include "exostring.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "AURORA/aurmodel.h"
#include <stdio.h>

#include "port/libcstring.h"

#include "port/libcbase.h"
#include "exostringinline.h"
class CAurObject;
#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBaseDoor::CSWCAnimBaseDoor()
{
    m_nAnimBaseType = ANIM_BASE_TYPE_DOOR;
}

CSWCAnimBaseDoor::~CSWCAnimBaseDoor()
{
    UnloadModel();
}

// Only the open-door style may pass; the other siblings ignore the byte entirely.
int CSWCAnimBaseDoor::LoadModel(const CResRef& resRef, unsigned char nStyle)
{
    if (nStyle != ANIM_MODEL_BODY)
        return 0;

    char szName[20];
    resRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("door"), 0, 0);
    if (m_pModel == 0)
    {
        char szMsg[128];
        sprintf(szMsg, "Cannot load door model '%s'.", szName);
        return 0;
    }
    m_resModelTexture = resRef;
    m_bModelTextureDataCached = 0;
    return 1;
}

CExoString CSWCAnimBaseDoor::GetAnimationName(unsigned short nAnimation)
{
    unsigned short nAnimId = ServerToClientAnimation(nAnimation);
    CExoString sResult;
    if (!g_pRules->m_p2DArrays->m_pAnimations->GetCExoStringEntry(
            nAnimId, CTwoDimArrays::COL2DA_ANIMATIONS_NAME, &sResult))
        sResult = "";

    if (sResult.GetLength() <= 0)
    {
        CExoString sMsg;
        sMsg.Format(const_cast<char*>("CSWCAnimBaseDoor::GetAnimationName(): No name for server animation %d"),
                    nAnimation);
    }
    return sResult;
}

// The message text names CSWCAnimBasePlaceable, which is the exported string, not a typo.
// EFFECTIVE: one redundant argument truncation; every spelling of it measures the same length.
unsigned short CSWCAnimBaseDoor::ServerToClientAnimation(unsigned short nAnimation)
{
    switch (nAnimation)
    {
    case SERVER_ANIMATION_DAMAGE: return CLIENT_ANIMATION_DOOR_DAMAGE;
    case SERVER_ANIMATION_CLOSED: return CLIENT_ANIMATION_DOOR_CLOSED;
    case SERVER_ANIMATION_DOOR_OPEN1: return CLIENT_ANIMATION_DOOR_OPENED_1;
    case SERVER_ANIMATION_DOOR_OPEN2: return CLIENT_ANIMATION_DOOR_OPENED_2;
    case SERVER_ANIMATION_DESTROYED:
        return m_nDestroyedAnimVariant == 0 ? CLIENT_ANIMATION_DOOR_DEFAULT : CLIENT_ANIMATION_DOOR_DEAD;
    case SERVER_ANIMATION_DAMAGE_2: return CLIENT_ANIMATION_DOOR_DAMAGE;
    case SERVER_ANIMATION_DOOR_BUSTED: return CLIENT_ANIMATION_DOOR_BUSTED;
    case SERVER_ANIMATION_DOOR_TRANSITION: return CLIENT_ANIMATION_DOOR_TRANSITION;
    case SERVER_ANIMATION_DAMAGE_3:
    case SERVER_ANIMATION_DAMAGE_4:
    case SERVER_ANIMATION_DAMAGE_5:
    case SERVER_ANIMATION_DAMAGE_6:
        return CLIENT_ANIMATION_DOOR_DAMAGE;
    default:
        break;
    }

    if (nAnimation >= SERVER_ANIMATION_PAUSE)  // the first server id
    {
        char szMsg[256];
        sprintf(szMsg, "CSWCAnimBasePlaceable::ServerToClientAnimation(): Failed to map server anim "
                       "%i to client anim.", static_cast<int>(nAnimation));
    }
    return nAnimation;
}

int CSWCAnimBaseDoor::SetAnimationInternal(CAurObject* pObject, unsigned short nAnim,
                                           float fParam, int nParam, float fParam2)
{
    K2_UNUSED(nParam);
    if (pObject == 0)
        return 0;

    CExoString sName = GetAnimationName(nAnim);
    fParam = (fParam == 0.0f) ? 1.0f : fParam;

    switch (nAnim)
    {
    // The looping door animations that play at the default speed.
    case SERVER_ANIMATION_CLOSED: case SERVER_ANIMATION_DOOR_OPEN1: case SERVER_ANIMATION_DOOR_OPEN2:
    case SERVER_ANIMATION_DESTROYED: case SERVER_ANIMATION_DOOR_BUSTED:
        pObject->PlayAnimation(sName.CStr(), fParam, AnimRun::kAnimNoBlendIn, fParam2);
        break;
    // The ones that force the alternate playback rate.
    case CLIENT_ANIMATION_DOOR_DIE: case CLIENT_ANIMATION_DOOR_OPENING_1:
    case CLIENT_ANIMATION_DOOR_OPENING_2: case CLIENT_ANIMATION_DOOR_CLOSING_1:
    case CLIENT_ANIMATION_DOOR_CLOSING_2:
    case SERVER_ANIMATION_DAMAGE: case SERVER_ANIMATION_DAMAGE_2: case SERVER_ANIMATION_DAMAGE_3:
    case SERVER_ANIMATION_DAMAGE_4: case SERVER_ANIMATION_DAMAGE_5: case SERVER_ANIMATION_DAMAGE_6:
        pObject->PlayAnimation(sName.CStr(), fParam, AnimRun::kAnimOnce | AnimRun::kAnimNoBlendIn, fParam2);
        break;
    // With no door animation at all, clear the animation instead of starting one.
    default:
        pObject->PlayAnimation(const_cast<char*>(""), 1.0f, 0, fParam2);
        break;
    }
    return 1;
}
