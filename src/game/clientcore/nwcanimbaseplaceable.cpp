#include "nwcanimbaseplaceable.h"

#include "AURORA/aurmodel.h"
#include "exostring.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include <stdio.h>

#include "port/libcstring.h"

#include "port/libcbase.h"
class CAurObject;
#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCAnimBasePlaceable::CSWCAnimBasePlaceable()
{
    m_nAnimBaseType = ANIM_BASE_TYPE_PLACEABLE;
}

CSWCAnimBasePlaceable::~CSWCAnimBasePlaceable()
{
    UnloadModel();
}

void CSWCAnimBasePlaceable::EnableAlwaysRender() {
    if (m_pModel)
        m_pModel->EnableAlwaysRender();
}

// Loads the placeable's model, built from the "placeable" supermodel.
int CSWCAnimBasePlaceable::LoadModel(const CResRef& resRef, unsigned char which)
{
    K2_UNUSED(which);
    char szName[20];
    resRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("placeable"), 0, 0);
    if (m_pModel == 0)
        return 0;
    m_resModelTexture = resRef;
    m_bModelTextureDataCached = 0;
    return 1;
}

// Resolves an animation's name. Rows the animations table names are used as they are; the synthetic
// range 1000 to 1727 becomes "cut<number>" plus a per-bank suffix, and anything outside it is
// "default".
CExoString CSWCAnimBasePlaceable::GetAnimationName(unsigned short nAnimation)
{
    unsigned short nAnimId = ServerToClientAnimation(nAnimation);
    CExoString sResult;
    if (!g_pRules->m_p2DArrays->m_pAnimations->GetCExoStringEntry(
            nAnimId, CTwoDimArrays::COL2DA_ANIMATIONS_NAME, &sResult))
        sResult = "";

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sResult.GetLength() > 0)
#else
    if (sResult.m_sString != 0 && static_cast<int>(strlen(sResult.m_sString)) > 0)
#endif
        return sResult;

    if (static_cast<unsigned short>(nAnimation - 1000) <= 1727 - 1000)  // the cutscene banks
    {
        CExoString sSuffix;
        int nBase = 0;
        if (nAnimation < 1128)
            nBase = -1000;
        else if (static_cast<unsigned short>(nAnimation - 1200) <= 127)
        {
            nBase = -1200;
            sSuffix = "w";
        }
        else if (static_cast<unsigned short>(nAnimation - 1400) <= 127)
        {
            nBase = -1400;
            sSuffix = "l";
        }
        else if (nAnimation >= 1600)
        {
            nBase = -1600;
            sSuffix = "wl";
        }

        if (nBase != 0 && static_cast<unsigned>(nBase + nAnimation + 1) != 0)
        {
            char szNumber[4];
            sprintf(szNumber, "%03d", nBase + nAnimation + 1);
            CExoString sNumber(szNumber);
            sResult = CExoString("cut") + sNumber + sSuffix;
        }
    }
    else
        sResult = "default";
    return sResult;
}

// Plays an animation on the model, choosing the playback flags per animation and reporting whether
// the animation was accepted.
int CSWCAnimBasePlaceable::SetAnimation(unsigned short nAnimation, float fSpeed, int nFlags,
                                        float fStart)
{
    K2_UNUSED(nFlags);
    if (!m_pModel)
        return 0;
    CExoString sAnim = GetAnimationName(nAnimation);
    if (fSpeed == 0.0f)
        fSpeed = 1.0f;
    switch (nAnimation)
    {
    case CLIENT_ANIMATION_PLACEABLE_DIE: case CLIENT_ANIMATION_PLACEABLE_CLOSE_TO_OPEN:
    case CLIENT_ANIMATION_PLACEABLE_OPEN_TO_CLOSE: case CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF:
    case CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON:
    case SERVER_ANIMATION_DAMAGE: case SERVER_ANIMATION_DAMAGE_2: case SERVER_ANIMATION_DAMAGE_3:
    case SERVER_ANIMATION_DAMAGE_4: case SERVER_ANIMATION_DAMAGE_5: case SERVER_ANIMATION_DAMAGE_6:
        m_pModel->PlayAnimation(sAnim.CStr(), fSpeed, AnimRun::kAnimOnce, fStart);
        break;
    case SERVER_ANIMATION_PAUSE: case SERVER_ANIMATION_DESTROYED:
    case SERVER_ANIMATION_PLACEABLE_ACTIVATE: case SERVER_ANIMATION_PLACEABLE_DEACTIVATE:
    case SERVER_ANIMATION_PLACEABLE_OPEN: case SERVER_ANIMATION_PLACEABLE_CLOSE:
        m_pModel->PlayAnimation(sAnim.CStr(), fSpeed, 0, fStart);
        return 0;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP01: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP02:
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP03: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP04:
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP05: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP06:
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP07: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP08:
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP09: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP10:
        m_pModel->PlayAnimation(sAnim.CStr(), fSpeed, 0, fStart);
        break;
    default:
        if (static_cast<unsigned short>(nAnimation - 1000) <= 1327 - 1000)
            m_pModel->PlayAnimation(sAnim.CStr(), fSpeed, AnimRun::kAnimOnce, fStart);
        else if (static_cast<unsigned short>(nAnimation - 1400) <= 1727 - 1400)
            m_pModel->PlayAnimation(sAnim.CStr(), fSpeed, 0, fStart);
        else
            m_pModel->PlayAnimation(const_cast<char*>(""), 1.0f, 0, fStart);
        break;
    }
    return 1;
}

// Maps a placeable animation id onto its client-side id. Only seventeen ids are live; everything
// else is reported as unknown, including 10109.
unsigned short CSWCAnimBasePlaceable::ServerToClientAnimation(unsigned short nAnimation)
{
    switch (nAnimation)
    {
    case SERVER_ANIMATION_PAUSE: return CLIENT_ANIMATION_PLACEABLE_DEFAULT;
    case SERVER_ANIMATION_DAMAGE: case SERVER_ANIMATION_DAMAGE_2: case SERVER_ANIMATION_DAMAGE_3:
    case SERVER_ANIMATION_DAMAGE_4: case SERVER_ANIMATION_DAMAGE_5: case SERVER_ANIMATION_DAMAGE_6:
        return CLIENT_ANIMATION_PLACEABLE_DAMAGE;
    case SERVER_ANIMATION_DESTROYED: return CLIENT_ANIMATION_PLACEABLE_DEAD;
    case SERVER_ANIMATION_PLACEABLE_ACTIVATE: return CLIENT_ANIMATION_PLACEABLE_ON;
    case SERVER_ANIMATION_PLACEABLE_DEACTIVATE: return CLIENT_ANIMATION_PLACEABLE_OFF;
    case SERVER_ANIMATION_PLACEABLE_OPEN: return CLIENT_ANIMATION_PLACEABLE_OPEN;
    case SERVER_ANIMATION_PLACEABLE_CLOSE: return CLIENT_ANIMATION_PLACEABLE_CLOSE;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP01: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP01;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP02: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP02;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP03: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP03;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP04: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP04;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP05: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP05;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP06: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP06;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP07: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP07;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP08: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP08;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP09: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP09;
    case SERVER_ANIMATION_PLACEABLE_ANIMLOOP10: return CLIENT_ANIMATION_PLACEABLE_ANIMLOOP10;
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
