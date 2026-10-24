// The flying-projectile animation base.

#include "nwcanimbaseprojectile.h"

#include "AURORA/aurmodel.h"
#include "Quaternion.h"

#include "port/libcbase.h"
class CAurObject;
#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

// Stamps this leaf's animation-base kind byte, which RequestModel dispatches on;
// the rest is the base constructor.
CSWCAnimBaseProjectile::CSWCAnimBaseProjectile()
{
    m_nAnimBaseType = ANIM_BASE_TYPE_PROJECTILE;
}

// The base call is qualified: an unqualified UnloadModel would stay virtual even inside this
// destructor, and the engine calls it directly.
CSWCAnimBaseProjectile::~CSWCAnimBaseProjectile()
{
    CSWCAnimBase::UnloadModel();
}

int CSWCAnimBaseProjectile::LoadModel(const CResRef& resRef, unsigned char which)
{
    K2_UNUSED(which);
    char szName[20];
    resRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("projectile"), 0, 0);
    if (m_pModel == 0)
        return 0;
    m_resModelTexture = resRef;
    m_bModelTextureDataCached = 0;
    return 1;
}

// Selects one of five animation names from the family's jump table. This leaf has no null-model
// guard, unlike CSWCAnimBasePlaceable's SetAnimation: every arm loads the model directly, so do not
// add one.
int CSWCAnimBaseProjectile::SetAnimation(unsigned short nAnimation, float fSpeed, int nFlags,
                                         float fStart)
{
    if (fSpeed == 0.0f)
        fSpeed = 1.0f;

    switch (nAnimation)
    {
    case PROJECTILE_ANIMATION_DEFAULT:
        m_pModel->PlayAnimation(const_cast<char*>("default"), fSpeed, nFlags, fStart);
        break;
    case PROJECTILE_ANIMATION_TRAVEL:
        m_pModel->PlayAnimation(const_cast<char*>("travel01"), fSpeed, nFlags, fStart);
        break;
    case PROJECTILE_ANIMATION_IMPACT:
        m_pModel->PlayAnimation(const_cast<char*>("impact01"), fSpeed, nFlags | 1, fStart);
        break;
    case PROJECTILE_ANIMATION_THROW_OUT:
        m_pModel->PlayAnimation(const_cast<char*>("throwout"), fSpeed, nFlags, fStart);
        break;
    case PROJECTILE_ANIMATION_THROW_BACK:
        m_pModel->PlayAnimation(const_cast<char*>("throwback"), fSpeed, nFlags, fStart);
        break;
    default:
        m_pModel->PlayAnimation(const_cast<char*>(""), 1.0f, 0, fStart);
        break;
    }
    return 1;
}

// Aims the model along a direction. The normalized vector is held in a named local because both the
// yaw and the pitch need its address; the `<=` compare and the 1e-5f threshold match the sibling
// CSWSArea orientation list.
int CSWCAnimBaseProjectile::SetOrientation(const Vector& v)
{
    if (!m_pModel)
        return 0;

    if (v.x * v.x + v.y * v.y + v.z * v.z <= 1e-5f)
        return 0;

    Vector vNorm = normalize(v);
    m_pModel->SetOrientation(YawPitchRoll(Yaw(vNorm), Pitch(vNorm), 0.0f));
    return 1;
}
