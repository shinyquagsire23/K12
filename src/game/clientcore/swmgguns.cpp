#include "k2stdint.h"
#include <stddef.h>
#include "appmanager.h"
#include "clientexoapp.h"
#include "AURORA/aurmodel.h"
#include "include/aurbehavior.h"
#include "AURORA/aurbehavior.h"
#include "CExoArrayListImpl.h"
#include "Vector.h"
#include "Quaternion.h"
#include "exoresman.h"
#include "swmgbehavior.h"
#include "swmgguns.h"
#include "swminigame.h"
#include "swmgtrackfollower.h"
#include "nwscript.h"

#include "port/libcbase.h"
#include "port/libcstring.h"

#include "port/crtapi.h"

// SetBulletTargetType's validity mask, written as 0/1 hex digits: it rejects a set bit 4, 8, 12, ...
// 28 only. The real target types are bits (player 1, enemy 2, see CSWTrackFollower::TargetType),
// so 1-3 pass, but so does any other value that misses those bits (4, 6, 0x20, ...).
enum {
    BULLET_TARGET_TYPE_INVALID_DIGITS = 0x11111110,
};

extern int quaternionidentityoptimization;

#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pParam3, SDL_RWops* pParam4);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pParam3, FILE* pParam4);
#endif

// printf-style formatter through the fortified __vsprintf_chk, sized by the caller's buffer.
static void GunBankMessage(char* szBuf, int nSize, const char* szFormat, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, szFormat);
    __vsprintf_chk(szBuf, 0, nSize, szFormat, ap);
    __builtin_va_end(ap);
}

// Rotates a vector by a quaternion, skipping the work when the rotation is the identity.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * w, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

#if !KOTOR1
static inline Vector RotateByQuaternionInverse(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == -1.0f || w == 1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float nw = -w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * nw, wy = y * nw, wz = z * nw;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}
#else
// K1's spelling of the inverse rotate: the forward rotate by `-q`, quaternion operand first.
static inline Vector RotateByQuaternionK1(const Quaternion& q, const Vector& v)
{
    return RotateByQuaternion(v, q);
}
#endif

void FireGunCallback(CAurObject* pObject, char* pszEvent, void* pData)
{
    CSWMGGunBank* pGunBank = static_cast<CSWMGGunBank*>(pData);

    if (_strnicmp(pszEvent, "fire", 4) != 0)
    {
        // The compare count is 64 bits wide, so the literal length has to be computed as one.
        if (_strnicmp(pszEvent, "start", 5) == 0
            && _strnicmp(pszEvent + 5, "fire", static_cast<StrnicmpCount>(__strlen_chk("fire", sizeof("fire")))) == 0)
        {
            pGunBank->OnFire();
        }
        return;
    }

    Vector vHook;
    Quaternion qHook;
    int nGunIndex;       // deliberately uninitialized: sscanf is the only writer

    static_cast<void>(sscanf(pszEvent, "fire%d", &nGunIndex));

    char szHook[17];
    GunBankMessage(szHook, sizeof(szHook), "bullethook%d", nGunIndex);

    if (!pObject->GetPartPosition(szHook, &vHook, &qHook))
    {
        char szMessage[256];
        GunBankMessage(szMessage, sizeof(szMessage),
                       "FireGunCallback: no part called bullethook%d", nGunIndex);
        return;
    }

    CResRef resBullet = pGunBank->m_resBullet;
    char szBullet[17];
    resBullet.CopyToString(szBullet);

    CAurObject* pBullet = NewCAurObject(szBullet, const_cast<char*>(""), 0, 0);
    if (pBullet == NULL)
    {
        char szMessage[256];
        GunBankMessage(szMessage, sizeof(szMessage),
                       "FireGunCallback: could not create bullet %s", szBullet);
        return;
    }

    if (pGunBank->IsTargetting())
    {
        CSWMGBehaviorTrackAndFire* pTrack =
            static_cast<CSWMGBehaviorTrackAndFire*>(pObject->GetBehavior(BEHAVIOR_TYPE_MGTRACKANDFIRE));
        if (pTrack == NULL)
        {
            delete pBullet;
            return;
        }

        float fInaccuracy = pTrack->m_targetting.m_fInaccuracy;

#if KOTOR1  // K1: the inverse is the out-of-line `-q`, formed before the position is read
        Quaternion qTarget = pTrack->m_pTarget.m_pObject->GetOrientation();
        Vector vTargetPos = RotateByQuaternionK1(
            -qTarget,
            RotateByQuaternion(pTrack->m_pTarget.m_pObject->GetPosition(), qTarget) +
                pTrack->m_vTargetOffset);
#else
        Quaternion qTarget = pTrack->m_pTarget.m_pObject->GetOrientation();
        Vector     vTarget = pTrack->m_pTarget.m_pObject->GetPosition();

        // The local offset is carried into world space by the target's orientation, then the sum is
        // rotated back by the inverse of that orientation.
        Vector vTargetPos = RotateByQuaternionInverse(
            RotateByQuaternion(vTarget, qTarget) + pTrack->m_vTargetOffset, qTarget);
#endif
        Vector vAim = vTargetPos;

        if (fInaccuracy > 0.0f)
        {
            // A random unit vector, scaled by the spread radius and then made perpendicular to the
            // firing line (Gram-Schmidt), so the scatter never shortens or lengthens the shot.
            // The draw spans [0, 32767], so subtracting 16383 centres it.
            float fSpreadX = static_cast<float>(rand_wincompatible() - 16383);
            float fSpreadY = static_cast<float>(rand_wincompatible() - 16383);
            float fSpreadZ = static_cast<float>(rand_wincompatible() - 16383);
            Vector vSpread = { fSpreadX, fSpreadY, fSpreadZ };
            vSpread.Normalize();
            vSpread = vSpread * (fInaccuracy * pTrack->m_fSpreadRadius);

            Vector vDirection = normalize(vTargetPos - vHook);
            vAim = vTargetPos + vSpread
                   - vDirection * (vDirection.x * vSpread.x + vDirection.y * vSpread.y
                                   + vDirection.z * vSpread.z);
        }

        vAim = vAim - vHook;
        qHook = pointto(vAim) * Quaternion(Vector(1.0f, 0.0f, 0.0f), 1.5707964f);
    }

    pBullet->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

#if !KOTOR1  // K1: no player-speed fold-in and no player-orientation override
    CSWTrackFollower* pPlayer =
        static_cast<CSWMiniGame*>(g_pAppManager->m_pClientExoApp->GetMiniGame())->m_pPlayer;

    // A bank riding the same track as the player fires with the player's own speed folded in, so a
    // shot fired forward from a moving swoop outruns it; the addition is undone after the spawn.
    if (pGunBank->m_pTrackFollower->m_pTrack == pPlayer->m_pTrack)
    {
        float fSpeed = pGunBank->m_bullet.m_fSpeed + pPlayer->m_fSpeed;
        if (fSpeed >= 0.0f)
            pGunBank->m_bullet.m_fSpeed = fSpeed;

        qHook = pPlayer->GetModel(0)->GetOrientation();
    }
#endif

    pBullet->AttachToScene(pObject->GetScene());
    pBullet->SetPosition(vHook);
    pBullet->SetOrientation(qHook);
    pBullet->SetBehavior(new CSWMGBehaviorBullet(pBullet, pGunBank->m_pTrackFollower,
                                                 pGunBank->m_bullet));
    pGunBank->m_pTrackFollower->m_pMiniGame->AddBullet(pBullet);

#if !KOTOR1
    if (pGunBank->m_pTrackFollower->m_pTrack == pPlayer->m_pTrack)
    {
        float fSpeed = pGunBank->m_bullet.m_fSpeed - pPlayer->m_fSpeed;
        if (fSpeed >= 0.0f)
            pGunBank->m_bullet.m_fSpeed = fSpeed;
    }
#endif
}

void CSWMGGunBank::OnFire()
{
    CResRef resScript = m_pTrackFollower->GetScriptName(7);

    m_pTrackFollower->m_lastFiredBullet = m_bullet;

    if (resScript.IsValid())
    {
        CExoString sScript;
        resScript.CopyToString(sScript);
        g_pVirtualMachine->RunScript(&sScript, m_pTrackFollower->m_nObjectId, 1);
    }
}

float CSWMGGunBank::SetBulletSpeed(float speed)
{
    if (speed >= 0.0f)
        m_bullet.m_fSpeed = speed;
    return m_bullet.m_fSpeed;
}

void GunDie(CAurObject* pObject, char* pszEvent, void* pData)
{
    K2_UNUSED(pszEvent);
    CSWMGGunBank* pGunBank = static_cast<CSWMGGunBank*>(pData);

    if (pGunBank == NULL)
    {
        delete pObject;
        return;
    }

    pGunBank->RemoveGun(pObject);
    delete pObject;

    if (pGunBank->m_aGuns.m_count == 0)
    {
        if (pGunBank->m_pTrackFollower != NULL)
            pGunBank->m_pTrackFollower->RemoveGunBank(pGunBank);
        delete pGunBank;
    }
}

static int FindGunIndex(CAurObject** paGuns, int64_t nCount, CAurObject* pGun)
{
    for (int i = 0; i < nCount; i++)
        if (paGuns[i] == pGun)
            return i;
    return -1;
}

CAurObject* CSWMGGunBank::RemoveGun(CAurObject* pGun)
{
    int nIndex = FindGunIndex(m_aGuns.m_pArray, m_aGuns.m_count, pGun);

    if (nIndex == -1)
        return NULL;

    pGun->AttachToObject(NULL, NULL, 0);

    // inlined CExoArrayList<CAurObject*>::DelIndex(nIndex)
    m_aGuns.m_count--;
    for (int i = nIndex; i < m_aGuns.m_count; i++)
        m_aGuns.m_pArray[i] = m_aGuns.m_pArray[i + 1];

    return pGun;
}

int CSWMGGunBank::ClearGuns()
{
    int nOldCount = m_aGuns.m_count;

    for (int i = m_aGuns.m_count - 1; i >= 0; i--)
    {
        m_aGuns.m_pArray[i]->AttachToObject(NULL, NULL, 0);
        delete m_aGuns.m_pArray[i];
    }

    if (m_aGuns.m_pArray != NULL)
    {
        delete[] m_aGuns.m_pArray;
        m_aGuns.m_pArray = NULL;
        m_aGuns.m_capacity = 0;
    }

    m_aGuns.m_count = 0;
    return nOldCount;
}

CSWMGGunBank::~CSWMGGunBank()
{
    ClearGuns();
}

float CSWMGGunBank::SetBulletLifeSpan(float lifespan)
{
    if (lifespan > 0.0f)
        m_bullet.m_fLifeSpan = lifespan;
    return m_bullet.m_fLifeSpan;
}

// Fires every gun whose muzzle behavior is attached; a model with none, such as a decorative
// barrel, has nothing to fire.
void CSWMGGunBank::Fire()
{
    int nCount = m_aGuns.m_count;

    for (int i = 0; i < nCount; i++)
    {
        CSWMGBehaviorFire* pFire =
            static_cast<CSWMGBehaviorFire*>(m_aGuns.m_pArray[i]->GetBehavior(BEHAVIOR_TYPE_MGFIRE));
        if (pFire != NULL)
            pFire->Fire();
    }
}

// Blows the bank up. A gun with a death animation is left alive to play it and calls GunDie back on
// its "donedie" event; a gun with none is dropped immediately.
void CSWMGGunBank::Die()
{
    for (int i = m_aGuns.m_count - 1; i >= 0; i--)
    {
        CAurObject* pGun = m_aGuns.m_pArray[i];

        pGun->PlayAnimation(NULL, 1.0f, 0, 0.0f);

        if (pGun->PlayAnimation(const_cast<char*>("die"), 1.0f, 1, 0.0f))
        {
            pGun->RegisterCallBack(GunDie, const_cast<char*>("donedie"), this, 9999.0f, false);
        }
        else
        {
            RemoveGun(pGun);
            delete pGun;
        }
    }
}

// Runs the base blow-up for the targetting bank, but only for a gun whose track-and-fire behavior
// is still attached. That behavior is swapped for a plain CAurBehaviorAttach with the same parent
// and part before the death animation starts, so the dying model stops tracking its aim.
void CSWMGTargettingGunBank::Die()
{
    for (int i = m_aGuns.m_count - 1; i >= 0; i--)
    {
        CAurObject* pGun = m_aGuns.m_pArray[i];

        CAurBehavior* pBehavior = pGun->GetBehavior(BEHAVIOR_TYPE_MGTRACKANDFIRE);
        if (pBehavior == NULL)
        {
            RemoveGun(pGun);
            delete pGun;
            continue;
        }

        CAurBehaviorAttach* pAttach = static_cast<CAurBehaviorAttach*>(pBehavior);
        if (pAttach->m_pAttachedTo.m_pObject == NULL || pAttach->m_pAttachedPart == NULL)
        {
            RemoveGun(pGun);
            delete pGun;
            continue;
        }

        pGun->SetBehavior(new CAurBehaviorAttach(pGun, pAttach->m_pAttachedTo.m_pObject,
                                                 pAttach->m_pAttachedPart));
        pGun->PlayAnimation(NULL, 1.0f, 0, 0.0f);

        if (pGun->PlayAnimation(const_cast<char*>("die"), 1.0f, 1, 0.0f))
        {
            pGun->RegisterCallBack(GunDie, const_cast<char*>("donedie"), this, 9999.0f, false);
        }
        else
        {
            RemoveGun(pGun);
            delete pGun;
        }
    }
}

// Flashes every gun through the damage animation when the bank takes a hit, and queues the return
// to idle.
void CSWMGGunBank::Hit()
{
    int nCount = m_aGuns.m_count;

    for (int i = 0; i < nCount; i++)
    {
        CAurObject* pGun = m_aGuns.m_pArray[i];
        pGun->PlayAnimation(const_cast<char*>("damage"), 1.0f, 1, 0.0f);
        pGun->PlayAnimation(const_cast<char*>("ready"), 1.0f, AnimRun::kAnimQueue, 0.0f);
    }
}

// The two behaviors that own a rate of fire share no base for it, so the setter dispatches on the
// behavior type; GetBehavior(-1) means whatever behavior the model has.
float CSWMGGunBank::SetRateOfFire(float rate)
{
    if (rate >= 0.0f)
    {
        int nCount = m_aGuns.m_count;
        m_bullet.m_fRateOfFire = rate;

        for (int i = 0; i < nCount; i++)
        {
            CAurBehavior* pBehavior = m_aGuns.m_pArray[i]->GetBehavior(-1);
            int nType = pBehavior->GetType();

            if (nType == BEHAVIOR_TYPE_MGTRACKANDFIRE)
                static_cast<CSWMGBehaviorTrackAndFire*>(pBehavior)->SetRateOfFire(rate);
            else if (nType == BEHAVIOR_TYPE_MGFIRE)
                static_cast<CSWMGBehaviorFire*>(pBehavior)->SetRateOfFire(rate);
        }
    }

    return m_bullet.m_fRateOfFire;
}

// Accepts a target type built out of 0/1 digits, and reports any value with other bits set.
int CSWMGGunBank::SetBulletTargetType(int nType)
{
    char szBuf[256];

    if (nType & BULLET_TARGET_TYPE_INVALID_DIGITS)
        GunBankMessage(szBuf, sizeof(szBuf),
                       "CSWMGGunBank: Bullet Target Type (%d) is invalid", nType);
    else
        m_bullet.m_nTargetType = nType;

    return m_bullet.m_nTargetType;
}

CAurObject* CSWMGGunBank::GetGun(int index)
{
    if (index < m_aGuns.m_count)
        return m_aGuns.m_pArray[index];
    return NULL;
}

// Distinguishes the plain bank from the auto-aiming one.
int CSWMGGunBank::IsTargetting()
{
    return 0;
}

int CSWMGTargettingGunBank::IsTargetting()
{
    return 1;
}

CSWMGBullet::CSWMGBullet()
{
    m_nDamage = 0;
    m_fSpeed = 0.0f;
    m_fRateOfFire = 0.0f;
    m_fLifeSpan = 0.0f;
    m_nTargetType = 0;
}

CSWMGTargettingParameters::CSWMGTargettingParameters()
{
    m_fHorizontalSpread = 0.0f;
    m_fSensingRadius = 0.0f;
    m_fVerticalSpread = 0.0f;
    m_fInaccuracy = 0.0f;
}

// Builds one gun model and hangs it in the bank; every early-out returns null. There is deliberately
// no AttachToPart here: the model arrives already attached, and pObject and pPart only seed the fire
// behavior's attachment record, so SetGunModel can rebuild the gun from them later.
CAurObject* CSWMGGunBank::AddGun(CAurObject* pObject, CAurPart* pPart)
{
    if (!m_resGunModel.IsValid() || pObject == NULL || pPart == NULL)
        return NULL;

    char szModel[17];
    m_resGunModel.CopyToString(szModel);
    CAurObject* pNewGun = NewCAurObject(szModel, const_cast<char*>(""), 0, 0);
    if (pNewGun == NULL)
        return NULL;

    CSWMGBehaviorFire* pBehavior = new CSWMGBehaviorFire(pNewGun, pObject, pPart);
    pBehavior->SetRateOfFire(m_bullet.m_fRateOfFire);
    pBehavior->m_resFireSound = m_resFireSound;

    pNewGun->SetBehavior(pBehavior);
    pNewGun->RegisterCallBack(FireGunCallback, const_cast<char*>("*"), this, 9999.0f, false);
    pNewGun->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

    m_aGuns.Add(pNewGun);
    return pNewGun;
}

CAurObject* CSWMGTargettingGunBank::AddGun(CAurObject* pObject, CAurPart* pPart)
{
    if (!m_resGunModel.IsValid() || pObject == NULL || pPart == NULL)
        return NULL;

    char szModel[17];
    m_resGunModel.CopyToString(szModel);
    CAurObject* pNewGun = NewCAurObject(szModel, const_cast<char*>(""), 0, 0);
    if (pNewGun == NULL)
        return NULL;

    // The tracked object seeds two things, its model (the behavior's m_pTarget) and its collision
    // sphere (the firing spread's world-space radius).
    CSWMGBehaviorTrackAndFire* pBehavior = new CSWMGBehaviorTrackAndFire(
        pNewGun, pObject, pPart, m_pTarget->GetModel(0));
    pBehavior->SetRateOfFire(m_bullet.m_fRateOfFire);
    pBehavior->m_targetting = m_targetting;
    pBehavior->m_fSpreadRadius = m_pTarget->m_sphereRadius;
    pBehavior->m_resFireSound = m_resFireSound;

    if (m_pTarget->AsPlayer() != NULL)
        pBehavior->m_vTargetOffset = static_cast<CSWMiniPlayer*>(m_pTarget->AsPlayer())->m_vTargetOffset;

    pNewGun->SetBehavior(pBehavior);
    pNewGun->RegisterCallBack(FireGunCallback, const_cast<char*>("*"), this, 9999.0f, false);
    pNewGun->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

    m_aGuns.Add(pNewGun);
    return pNewGun;
}

// Swaps every gun in the bank over to a new model. A CAurObject cannot be re-skinned in place, so
// each gun is rebuilt: read the front gun's behavior, build a fresh model from the new resref, clone
// the behavior onto it with the same tunables, re-attach it to the same parent part, then destroy the
// old gun and append the new one. Each pass removes the front element and appends its replacement, so
// the array rotates through itself once.
// sic: pNewGun is uninitialized on every early-out path, so a gun with no behavior, with another
// behavior type, or with an unattached behavior gets the previous iteration's model.
void CSWMGGunBank::SetGunModel(const CResRef& res)
{
    m_resGunModel = res;

    int nCount = m_aGuns.m_count;

    for (int i = 0; i < nCount; i++)
    {
        CAurObject* pOldGun = m_aGuns.m_pArray[0];
        CAurObject* pNewGun;  // sic: uninitialized on the early-out paths
#ifdef K2_FIX_ENGINE_BUGS
        pNewGun = NULL;
#endif

        CAurBehavior* pBehavior = pOldGun->GetBehavior(-1);
        if (pBehavior != NULL)
        {
            int nType = pBehavior->GetType();

            if (nType == BEHAVIOR_TYPE_MGFIRE || nType == BEHAVIOR_TYPE_MGTRACKANDFIRE)
            {
                CAurObject* pTarget = NULL;
                if (nType == BEHAVIOR_TYPE_MGTRACKANDFIRE)
                    pTarget = static_cast<CSWMGBehaviorTrackAndFire*>(pBehavior)->m_pTarget.m_pObject;

                CAurBehaviorAttach* pAttach = static_cast<CAurBehaviorAttach*>(pBehavior);
                CAurObject* pParent = pAttach->m_pAttachedTo.m_pObject;
                CAurPart*   pPart;

                if (pParent != NULL && (pPart = pAttach->m_pAttachedPart) != NULL)
                {
                    char szModel[17];
                    m_resGunModel.CopyToString(szModel);
                    pNewGun = NewCAurObject(szModel, const_cast<char*>(""), 0, 0);

                    int nType2 = pBehavior->GetType();

                    if (nType2 == BEHAVIOR_TYPE_MGFIRE)
                    {
                        CSWMGBehaviorFire* pOld = static_cast<CSWMGBehaviorFire*>(pBehavior);
                        CSWMGBehaviorFire* pNew =
                            new CSWMGBehaviorFire(pNewGun, pParent, pPart);

                        pNew->SetRateOfFire(pOld->m_fRateOfFire);
                        pNew->SetFireSound(pOld->m_resFireSound);

                        pNewGun->SetBehavior(pNew);
                        pNewGun->RegisterCallBack(FireGunCallback, const_cast<char*>("*"), this, 9999.0f,
                                                  false);
                    }
                    else if (nType2 == BEHAVIOR_TYPE_MGTRACKANDFIRE)                    
                    {
                        CSWMGBehaviorTrackAndFire* pOld = static_cast<CSWMGBehaviorTrackAndFire*>(pBehavior);
                        CSWMGBehaviorTrackAndFire* pNew =
                            new CSWMGBehaviorTrackAndFire(pNewGun, pParent, pPart, pTarget);

                        pNew->m_targetting = pOld->m_targetting;
                        pNew->m_fSpreadRadius = pOld->m_fSpreadRadius;
                        pNew->m_vTargetOffset = pOld->m_vTargetOffset;
                        pNew->SetRateOfFire(pOld->m_fRateOfFire);
                        pNew->SetFireSound(pOld->m_resFireSound);

                        pNewGun->SetBehavior(pNew);
                        pNewGun->RegisterCallBack(FireGunCallback, const_cast<char*>("*"), this, 9999.0f,
                                                  false);
                    }
                    pNewGun->AttachToPart(pParent, pPart, 0);
                }
            }
        }

#ifdef K2_FIX_ENGINE_BUGS
        if (pNewGun == NULL)
        {
            // No replacement was built, so keep the old gun and rotate it to the back.
            m_aGuns.DelIndex(0);
            m_aGuns.Add(pOldGun);
            continue;
        }
#endif
        pNewGun->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

        m_aGuns.DelIndex(0);

        delete pOldGun;
        m_aGuns.Add(pNewGun);
    }
}

// Builds a gun bank for one vehicle. The bullet's fields arrive through the class's own validating
// setters, so an out-of-range speed, lifespan or target type is silently dropped instead of copied;
// m_nDamage has no setter and is assigned directly.
CSWMGGunBank::CSWMGGunBank(CSWTrackFollower* pTrackFollower, const CResRef& resBullet,
                           const CResRef& resGun, const CSWMGBullet& bullet)
{
    SetBulletModel(resBullet);
    SetGunModel(resGun);

    m_bullet.m_nDamage = bullet.m_nDamage;
    SetBulletSpeed(bullet.m_fSpeed);
    SetRateOfFire(bullet.m_fRateOfFire);
    SetBulletLifeSpan(bullet.m_fLifeSpan);
    SetBulletTargetType(bullet.m_nTargetType);
    m_bullet.m_resModel = bullet.m_resModel;

    m_pTrackFollower = pTrackFollower;
}

// Builds the auto-aiming bank by building the plain bank, then filling the targetting parameters
// through this class's own validating setters, so an out-of-range spread is dropped rather than
// stored.
CSWMGTargettingGunBank::CSWMGTargettingGunBank(CSWTrackFollower* pTrackFollower,
                                               const CResRef& resBullet, const CResRef& resGun,
                                               const CSWMGBullet& bullet,
                                               const CSWMGTargettingParameters& params,
                                               CSWTrackFollower* pTarget)
    : CSWMGGunBank(pTrackFollower, resBullet, resGun, bullet)
{
    SetSensingRadius(params.m_fSensingRadius);
    SetHorizontalSpread(params.m_fHorizontalSpread);
    SetVerticalSpread(params.m_fVerticalSpread);
    SetInaccuracy(params.m_fInaccuracy);
    m_pTarget = pTarget;
}

float CSWMGTargettingGunBank::SetSensingRadius(float radius)
{
    if (radius >= 0.0f)
        m_targetting.m_fSensingRadius = radius;
    return m_targetting.m_fSensingRadius;
}

float CSWMGTargettingGunBank::SetHorizontalSpread(float spread)
{
    if (spread >= 0.0f || spread == -1.0f)
        m_targetting.m_fHorizontalSpread = spread;
    return m_targetting.m_fHorizontalSpread;
}

float CSWMGTargettingGunBank::SetVerticalSpread(float spread)
{
    if (spread >= 0.0f || spread == -1.0f)
        m_targetting.m_fVerticalSpread = spread;
    return m_targetting.m_fVerticalSpread;
}

float CSWMGTargettingGunBank::SetInaccuracy(float inaccuracy)
{
    if (inaccuracy >= 0.0f)
        m_targetting.m_fInaccuracy = inaccuracy;
    return m_targetting.m_fInaccuracy;
}
