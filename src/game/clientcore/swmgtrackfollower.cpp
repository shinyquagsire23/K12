#include "k2stdint.h"
#include "swmgtrackfollower.h"

#include "AURORA/aurmodel.h"
#include "exosound.h"
#include "exostring.h"
#include "swmgguns.h"
#include "swminigame.h"
#include "nwscript.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "swmgobject.h"
#include "swmgbehavior.h"
#include "aurscene.h"
#include "Quaternion.h"
#if KOTOR1
#include "AURORA/auranim.h"
#endif

#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "List.h"
#include "include/aurbehavior.h"
#include "AURORA/aurbehavior.h"

float fInertiaScale = 1.0f;
int bDoAcceleration = 1;
float fSpeed = 100.0f;
float fLastSpeedSet = 100.0f;

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"

// A bounded sprintf, the fortified form the engine's string formatting resolves to.
static void snprintff(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
}

extern CExoSound* g_pExoSound;
extern unsigned char g_adjustWake;
#if !KOTOR1
unsigned char g_adjustWake;
#endif

int  EnumGunAttachPoints(CAurPart*);
void OnAnimKeyEvent(CAurObject*, char*, void*);
void SetInitialHook(CAurObject*, char*, void*);
void OnReportKey(CAurObject*, char*, void*);

void TrackFollowerPartDie(CAurObject*, char*, void*);

float CSWTrackFollower::SetSphereRadius(float radius) {
    if (radius >= 0.0f) {
        m_sphereRadius = radius;
        return radius;
    }
    return m_sphereRadius;
}

float CSWTrackFollower::SetInvincibility(float invincibility) {
    if (invincibility >= 0.0f) {
        m_invincibility = invincibility;
        return invincibility;
    }
    return m_invincibility;
}

int CSWTrackFollower::SetHitPoints(int hitPoints) {
    if (hitPoints <= 0)
        return m_hitPoints;
    m_hitPoints = hitPoints;
    return hitPoints;
}

int CSWTrackFollower::SetMaxHitPoints(int maxHitPoints) {
    if (maxHitPoints <= 0)
        return m_maxHitPoints;
    m_maxHitPoints = maxHitPoints;
    return maxHitPoints;
}

void CSWTrackFollower::SetSoundFrequency(int index, int frequency) {
    if (index >= 0 && index <= 1)
        m_sounds[index].frequency = frequency;
}

void CSWTrackFollower::SetSoundFrequencyIsRandom(int index, int isRandom) {
    if (index >= 0 && index <= 1)
        m_sounds[index].frequencyIsRandom = isRandom;
}

int CSWTrackFollower::GetSoundFrequency(int index) {
    if (index >= 0 && index <= 1)
        return m_sounds[index].frequency;
    return 0;
}

int CSWTrackFollower::GetSoundFrequencyIsRandom(int index) {
    if (index >= 0 && index <= 1)
        return m_sounds[index].frequencyIsRandom;
    return 0;
}

int CSWTrackFollower::GetSoundVolume(int index) {
    if (index >= 0 && index <= 1)
        return m_sounds[index].volume;
    return 0;
}

void CSWTrackFollower::AdjustHitPoints(int amount) {
#if KOTOR1  // K1: damage to the player's own vehicle is halved
    if (AsPlayer() != 0 && amount < 0)
        amount /= 2;
#endif
    m_hitPointAdjust = amount;
    return OnDamage(0);
}

// The obstacle this follower last collided with; a stale id is reset to the "none" sentinel.
CSWMiniGameObject* CSWTrackFollower::GetLastObstacleHit() {
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(m_nLastObstacleHitId, CSWMiniGameObjectArray::PROBE_OBSTACLE);
    if (pObject == 0)
        m_nLastObstacleHitId = CSWMiniGameObjectArray::OBJECT_ID_NONE;
    return pObject;
}

void CSWTrackFollower::OnDamage(int bApply) {
    // A script can replace the damage amount before it is applied.
    if (bApply == 0) {
        CResRef script = GetScriptName(SCRIPT_ON_DAMAGE);
        if (script.IsValid()) {
            CExoString scriptName;
            script.CopyToString(scriptName);
            g_pVirtualMachine->RunScript(&scriptName, m_nObjectId, 1);
            return;
        }
    }
    m_hitPoints += m_hitPointAdjust;
    if (m_hitPointAdjust >= 0) {
        m_hitPoints = m_hitPoints < m_maxHitPoints ? m_hitPoints : m_maxHitPoints;
    } else if (m_hitPoints <= 0) {
        if ((m_flags & FLAG_DEAD) == 0) {
            m_flags |= FLAG_DEAD;
            OnDeath(0);
        }
    } else {
        PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
        PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
    }
}

CAurObject* CSWTrackFollower::GetModel(int index) {
    if (index < m_models.m_count)
        return m_models.m_pArray[index].pModel;
    return 0;
}

// Removes a model from the list, compacting the survivors down one slot, and detaches it from
// whatever it hangs off. Returns the model, or null when it was not in the list.
CAurObject* CSWTrackFollower::RemoveModel(CAurObject* pModel) {
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++) {
        if (m_models.m_pArray[i].pModel == pModel) {
            // inlined CExoArrayList<SWMGFollowerModel>::DelIndex(i)
            m_models.m_count--;
            for (int j = i; j < m_models.m_count; j++)
                m_models.m_pArray[j] = m_models.m_pArray[j + 1];
            pModel->AttachToObject(0, 0, 0);
            return pModel;
        }
    }
    return 0;
}

// Handles the "donedie" callback, sent when a model finished its death animation, by removing and
// destroying the model. With no follower context the callback only destroys the model; when it was
// the last model, the vehicle hands itself back to the minigame.
void TrackFollowerPartDie(CAurObject* pModel, char*, void* pData) {
    CSWTrackFollower* pThis = static_cast<CSWTrackFollower*>(pData);
    if (pThis == 0) {
        delete pModel;
        return;
    }
    pThis->RemoveModel(pModel);
    delete pModel;
    if (pThis->m_models.m_count != 0)
        return;
    pThis->UpdateGunBanks();
    if (pThis->m_pMiniGame->m_pPlayer == pThis)
        pThis->m_pMiniGame->SetPlayer(0);
    else {
        pThis->m_pMiniGame->RemoveEnemy(static_cast<CSWMiniEnemy*>(pThis));  // idiom-exempt: same C-cast
                                                                //   self-handoff as OnDeath's
        delete pThis;
    }
}

// Destroys the vehicle: stops the track animation and its sound, blows up every model and gun bank,
// then hands back to the minigame. A script can override the whole sequence first.
void CSWTrackFollower::OnDeath(int a) {
    if (a == 0) {
        CResRef resScript = GetScriptName(SCRIPT_ON_DEATH);
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
            return;
        }
    }

    if (m_pTrack != 0 && (m_flags & FLAG_TRACK_PLAYING) != 0) {
        m_pTrack->PlayAnimation(const_cast<char*>("track"), 0.0f,
                                m_numLoops == 1 ? AnimRun::kAnimOnce | AnimRun::kAnimSyncTime
                                                : AnimRun::kAnimLoopEvents | AnimRun::kAnimSyncTime,
                                0.0f);
        m_flags &= ~FLAG_TRACK_PLAYING;
        if (m_pSounds[0] != 0)
            m_pSounds[0]->Stop();
    }

    CResRef resSound = m_soundNames[0];
    if (resSound.IsValid()) {
        unsigned char nChannel = AsPlayer() == 0 ? 24 : 23;
        unsigned char nVolume = resSound == const_cast<char*>("mgs_accelpad") ? CExoSound::CMAX_VOLUME : 100;
        g_pExoSound->PlayOneShotSound(resSound, nChannel, 0, nVolume, 0.0f, 0.0f);
    }

    int i = m_models.m_count;
    while (i > 0) {
        i--;
        CAurObject* pModel = m_models.m_pArray[i].pModel;

        pModel->PlayAnimation(0, 1.0f, 0, 0.0f);

        if (pModel->PlayAnimation(const_cast<char*>("die"), 1.0f, 1, 0.0f)) {
            pModel->RegisterCallBack(TrackFollowerPartDie, const_cast<char*>("donedie"), this, 9999.0f, false);
        } else {
            RemoveModel(pModel);
            delete pModel;
        }
    }

    int nBanks = m_gunBanks.m_count;
    for (int nBank = 0; nBank < nBanks; nBank++)
        m_gunBanks.m_pArray[nBank]->Die();

    if (m_models.m_count == 0) {
        if (m_pMiniGame->m_pPlayer == this)
            m_pMiniGame->SetPlayer(0);
        else
            m_bPendingDelete = 1;
    }
    m_pMiniGame->RemoveEnemy(static_cast<CSWMiniEnemy*>(this));  // idiom-exempt: the minigame's enemy list is
                                                    //   typed on the derived class; the follower
                                                    //   hands itself down with a C-cast, and the
                                                    //   call is a plain tail call
}

CResRef CSWTrackFollower::GetScriptName(int index) {
#if KOTOR1  // K1: ten script slots
    if (static_cast<unsigned int>(index) <= 9)
#else
    if (static_cast<unsigned int>(index) <= 12)
#endif
        return m_scripts[index];
    return CResRef();
}

void CSWTrackFollower::SetScriptName(int index, CResRef const& resScript) {
#if KOTOR1  // K1: ten script slots
    if (static_cast<unsigned int>(index) <= 9)
#else
    if (static_cast<unsigned int>(index) <= 12)
#endif
        m_scripts[index] = resScript;
}

// Channel 1 is the death sound; channel 0 owns the live streaming source (see GetSound).
CResRef CSWTrackFollower::GetSoundName(int index) {
    if (index == 1)
        return m_soundNames[0];
    return CResRef();
}

// Points a sound channel at a new resref and returns that channel's live source. Channel 1 only
// carries a name (the one-shot death sound); channel 0 owns the streaming source, built on first
// use and torn down when the name goes invalid.
// EFFECTIVE: one null test is a branch in the orig and a cinc select here.
CExoSoundSource* CSWTrackFollower::SetSoundName(int index, CResRef const& resSound) {
    if (index >= 0) {
        if (index == 0) {
            if (m_pSounds[index] != 0) {
                int bWasPlaying = m_pSounds[index]->IsHardwarePlayingSoundPleaseDontUseThis();
                if (bWasPlaying)
                    m_pSounds[index]->Stop();
                if (resSound.IsValid()) {
                    m_pSounds[index]->SetResRef(resSound, 1);
                    m_pSounds[index]->Set3D(1);
                    int nModels = m_models.m_count;
                    CExoSoundSource* pSound = m_pSounds[index];
                    Vector vPosition = nModels != 0
                                           ? m_models.m_pArray[0].pModel->GetPosition()
                                           : Vector();
                    pSound->SetPosition(vPosition, 0.0f);
                    m_pSounds[index]->SetVolume(static_cast<unsigned char>(m_sounds[index].volume));
                    int nPriority = AsPlayer() != 0 ? 23 : 24;
                    m_pSounds[index]->SetPriorityGroup(static_cast<unsigned char>(nPriority));
                    if (bWasPlaying)
                        m_pSounds[index]->Play();
                } else {
                    delete m_pSounds[index];
                    m_pSounds[index] = 0;
                }
            } else if (resSound.IsValid()) {
                m_pSounds[index] = new CExoSoundSource(resSound);
                m_pSounds[index]->Set3D(1);
                int nModels = m_models.m_count;
                CExoSoundSource* pSound = m_pSounds[index];
                Vector vPosition = nModels != 0
                                       ? m_models.m_pArray[0].pModel->GetPosition()
                                       : Vector();
                pSound->SetPosition(vPosition, 0.0f);
                m_pSounds[index]->SetVolume(static_cast<unsigned char>(m_sounds[index].volume));
                int nPriority = AsPlayer() != 0 ? 23 : 24;
                m_pSounds[index]->SetPriorityGroup(static_cast<unsigned char>(nPriority));
            }
            return m_pSounds[index];
        }
        if (index <= 1)
            m_soundNames[index - 1] = resSound;
    }
    return 0;
}

// Loads the eight script hooks this class owns; the base owns slots 0 to 4.
void CSWTrackFollower::LoadScripts(CResGFF* pGff, CResStruct* pStruct, CResStruct& gffStruct) {
    CSWMiniGameObject::LoadScripts(pGff, pStruct, gffStruct);
    int bSuccess;
    SetScriptName(SCRIPT_ON_DAMAGE, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnDamage"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_DEATH, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnDeath"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_FIRE, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnFire"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_HIT_OBSTACLE, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnHitObstacle"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_TRACK_LOOP, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnTrackLoop"), bSuccess, CResRef("")));
#if !KOTOR1  // K1: no OnAccelerate / OnBrake / OnHitWorld hooks
    SetScriptName(SCRIPT_ON_ACCELERATE, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnAccelerate"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_BRAKE, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnBrake"), bSuccess, CResRef("")));
    SetScriptName(SCRIPT_ON_HIT_WORLD, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("OnHitWorld"), bSuccess, CResRef("")));
#endif
}

// Reads the vehicle description out of its GFF struct: the hit-point and geometry scalars (each
// only committed when the file supplies a sane value), the gun banks, then the Scripts and Sounds
// sub-structs. Always reports success.
int CSWTrackFollower::Load(CResGFF* pGff, CResStruct* pStruct) {
    CResStruct subStruct;
    CResList gunBankList;
    int bSuccess;
    int nHitPoints = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("Hit_Points"), bSuccess, 0);
    if (nHitPoints >= 1)
        m_hitPoints = nHitPoints;
    int nMaxHitPoints = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("Max_HPs"), bSuccess, 0);
    if (nMaxHitPoints >= 1)
        m_maxHitPoints = nMaxHitPoints;
    float fSphereRadius = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Sphere_Radius"), bSuccess, -1.0f);
    if (fSphereRadius >= 0.0f)
        m_sphereRadius = fSphereRadius;
    SetLooping(pGff->ReadFieldINT(pStruct, const_cast<char*>("Num_Loops"), bSuccess, -10));
    m_nBumpDamage = pGff->ReadFieldINT(pStruct, const_cast<char*>("Bump_Damage"), bSuccess, 0);
    float fInvincibility = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Invince_Period"), bSuccess, 0.0f);
    if (fInvincibility >= 0.0f)
        m_invincibility = fInvincibility;

    bSuccess = pGff->GetList(&gunBankList, pStruct, const_cast<char*>("Gun_Banks"));
    if (bSuccess) {
        int nGunBanks = pGff->GetListCount(&gunBankList);
        for (int i = 0; i < nGunBanks; i++) {
            CResStruct gunBankStruct;
            bSuccess = pGff->GetListElement(&gunBankStruct, &gunBankList, i);
            if (bSuccess)
                LoadGun(pGff, &gunBankStruct);
        }
        if (nGunBanks != 0)
            AttachGuns();
    }

    if (pGff->GetStructFromStruct(&subStruct, pStruct, const_cast<char*>("Scripts")))
        LoadScripts(pGff, pStruct, subStruct);
    if (pGff->GetStructFromStruct(&subStruct, pStruct, const_cast<char*>("Sounds")))
        LoadSounds(pGff, pStruct, subStruct);
    OnCreate();
    return 1;
}

// Channel 0 is the looping engine sound, channel 1 the one-shot death sound.
void CSWTrackFollower::LoadSounds(CResGFF* pGff, CResStruct* pStruct, CResStruct& gffStruct) {
    K2_UNUSED(pStruct);
    int bSuccess;
    CExoSoundSource* pSound =
        SetSoundName(0, pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("Engine"), bSuccess, CResRef("")));
    if (pSound != 0)
        pSound->SetLooping(1);
    m_soundNames[0] = pGff->ReadFieldCResRef(&gffStruct, const_cast<char*>("Death"), bSuccess, CResRef(""));
}

CExoSoundSource* CSWTrackFollower::GetSound(int index) {
    if (index != 0)
        return 0;
    return m_pSounds[index];
}

void CSWTrackFollower::SetSoundVolume(int index, int volume) {
    if (static_cast<unsigned int>(index) <= 1) {
        m_sounds[index].volume = volume;
        if (index == 0)
            m_pSounds[index]->SetVolume(static_cast<unsigned char>(volume));
    }
}

CSWMGGunBank* CSWTrackFollower::GetGunBank(int index) {
    if (index >= 0 && index < m_gunBanks.m_count)
        return m_gunBanks.m_pArray[index];
    return 0;
}

// Turns every gun bank loose from the vehicle once it is wrecked.
void CSWTrackFollower::UpdateGunBanks() {
    if ((m_flags & FLAG_DEAD) == 0)
        return;
    int nBanks = m_gunBanks.m_count;
    for (int i = 0; i < nBanks; i++)
        m_gunBanks.m_pArray[i]->m_pTrackFollower = 0;
}

// Drops one bank from the array; a wrecked vehicle skips it, its gun banks are already detached.
void CSWTrackFollower::RemoveGunBank(CSWMGGunBank* pGunBank) {
    if ((m_flags & FLAG_DEAD) == 0)
        return;
    m_gunBanks.Remove(pGunBank);
}

// A part predicate for EnumParts that keeps the parts named "gunbank<N>-...".
int EnumGunAttachPoints(CAurPart* pPart) {
    return _strnicmp(pPart->Name(), "gunbank", static_cast<StrnicmpCount>(__strlen_chk("gunbank", sizeof("gunbank")))) == 0;
}

// Attaches one gun to every "gunbank<N>-" part of pModel and reports how many landed. The
// diagnostics are formatted and dropped.
int CSWTrackFollower::AttachGuns(CAurObject* pModel) {
    List<CAurPart*> parts;
    pModel->EnumParts(EnumGunAttachPoints, &parts);
    int nAttached = 0;
    for (int i = 0; i < parts.m_count; i++) {
        int nBankId;
        if (sscanf(parts.m_pData[i]->Name(), "gunbank%d-", &nBankId) == 1) {
            if (nBankId < 0) {
                char szMsg[128];
                snprintff(szMsg, sizeof(szMsg), "CSWTrackFollower: Bank ID %d < 0.", nBankId);
            } else if (nBankId >= m_gunBanks.m_count) {
                char szMsg[128];
                snprintff(szMsg, sizeof(szMsg),
                          "CSWTrackFollower: Trying to attach gun to a non-existent bank ID: %d",
                          nBankId);
            } else {
                m_gunBanks.m_pArray[nBankId]->AddGun(pModel, parts.m_pData[i]);
                nAttached++;
            }
        } else {
            char szMsg[128];
            snprintff(szMsg, sizeof(szMsg),
                      "CSWTrackFollower: Could not find bank ID in attach part name %s.",
                      parts.m_pData[i]->Name());
        }
    }
    return nAttached;
}

void CSWTrackFollower::AttachGuns() {
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
        AttachGuns(m_models.m_pArray[i].pModel);
}

// Detaches every model from whatever it hangs off, frees the array and reports how many there were.
int CSWTrackFollower::ClearModels() {
    int nModels = m_models.m_count;
    SWMGFollowerModel* pModels = m_models.m_pArray;
    for (int i = 0; i < nModels; i++) {
        pModels[i].pModel->AttachToObject(0, 0, 0);
        pModels = m_models.m_pArray;
    }
    if (pModels != 0) {
        delete[] pModels;
        m_models.m_pArray = 0;
        m_models.m_capacity = 0;
    }
    m_models.m_count = 0;
    return nModels;
}

// The follower's world position is its first model's; with no models it sits at the origin.
Vector CSWTrackFollower::GetPosition() {
    if (m_models.m_count == 0)
        return Vector(0.0f, 0.0f, 0.0f);
    return m_models.m_pArray[0].pModel->GetPosition();
}

CExoString CSWTrackFollower::GetName() {
    if (m_pTrack == 0)
        return CExoString("");
    return CExoString(static_cast<const char*>(m_pTrack->GetModelName()));
}

CAurObject* CSWTrackFollower::SetTrack(CAurObject* pTrack, int nAttachFlag) {
    if (m_pTrack == pTrack)
        return pTrack;
    return ForceSetTrack(pTrack, nAttachFlag);
}

void CSWTrackFollower::Stop() {
    if (m_pTrack == 0 || (m_flags & FLAG_TRACK_PLAYING) == 0)
        return;
    m_pTrack->PlayAnimation(const_cast<char*>("track"), 0.0f,
                                m_numLoops == 1 ? AnimRun::kAnimOnce | AnimRun::kAnimSyncTime
                                                : AnimRun::kAnimLoopEvents | AnimRun::kAnimSyncTime,
                                0.0f);
    m_flags &= ~FLAG_TRACK_PLAYING;
    if (m_pSounds[0] != 0)
        m_pSounds[0]->Stop();
}

void CSWTrackFollower::Resume() {
    if ((m_flags & FLAG_TRACK_PLAYING) != 0)
        return;
    m_flags |= FLAG_TRACK_PLAYING;
    if (m_pTrack == 0)
        return;
    float fTrackSpeed = m_fSpeed;
    int nAnim = m_numLoops == 1 ? AnimRun::kAnimOnce | AnimRun::kAnimSyncTime
                                 : AnimRun::kAnimLoopEvents | AnimRun::kAnimSyncTime;
    m_pTrack->PlayAnimation(const_cast<char*>("track"), fTrackSpeed / 100.0f, nAnim, 0.0f);
}

void CSWTrackFollower::Die() {
    if ((m_flags & FLAG_DEAD) != 0)
        return;
    m_flags |= FLAG_DEAD;
    OnDeath(0);
}

// The visible "I just took a hit" reaction.
void CSWTrackFollower::Hit() {
    PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
    PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
    int nBanks = m_gunBanks.m_count;
    for (int i = 0; i < nBanks; i++)
        m_gunBanks.m_pArray[i]->Hit();
}

// Tests a bullet against every model. A bullet this vehicle fired itself is ignored; the bullet's
// behavior remembers its shooter.
int CSWTrackFollower::DoBulletHitCheck(CAurObject* pBullet) {
    CSWMGBehaviorBullet* pBehavior = static_cast<CSWMGBehaviorBullet*>(pBullet->GetBehavior(BEHAVIOR_TYPE_MGBULLET));
    if (pBehavior == 0 || pBehavior->m_pTrackFollower != this) {
        int nModels = m_models.m_count;
        for (int i = 0; i < nModels; i++) {
            if (m_pMiniGame->HitCheckBullet(pBullet, m_models.m_pArray[i].pModel,
                                            static_cast<CSWMiniGameObject*>(this)))  // idiom-exempt: unmodelled base
                return 1;
        }
    }
    return 0;
}

void CSWTrackFollower::Update(float fSeconds) {
    CSWMiniGameObject::Update(fSeconds);
    if (m_invincibilityTimer > 0.0f) {
        float fLeft = m_invincibilityTimer - fSeconds;
        m_invincibilityTimer = fLeft > 0.0f ? fLeft : 0.0f;
    }
    if (m_pSounds[0] != 0)
        m_pSounds[0]->SetPosition(GetPosition(), 0.0f);
}

// Plays the animation on every model and on the player's spare camerahook model.
void CSWTrackFollower::PlayAnimation(char* pszAnim, float fAnimSpeed, int nFlags) {
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
        m_models.m_pArray[i].pModel->PlayAnimation(pszAnim, fAnimSpeed, nFlags, 0.0f);

    if (AsPlayer() != 0 && static_cast<CSWMiniPlayer*>(AsPlayer())->m_pCameraHook != 0)  // idiom-exempt: downcast off the As* probe
        static_cast<CSWMiniPlayer*>(AsPlayer())->m_pCameraHook->PlayAnimation(pszAnim, fAnimSpeed, nFlags, 0.0f);  // idiom-exempt: downcast off the As* probe
}

// Responds to an obstacle collision. The obstacle's script gets to react first, otherwise the
// vehicle drops its behaviors and wrecks itself.
void CSWTrackFollower::OnHitObstacle(int bApply) {
    if (bApply == 0) {
        CResRef resScript = GetScriptName(SCRIPT_ON_HIT_OBSTACLE);
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
            return;
        }
    }

#if !KOTOR1  // K1: a swoop-race vehicle is wrecked by an obstacle too
    if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_SWOOP)
        return;
#endif

    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
        m_models.m_pArray[i].pModel->SetBehavior(0);

    if ((m_flags & FLAG_DEAD) == 0) {
        m_flags |= FLAG_DEAD;
        OnDeath(0);
    }
}

// Sweeps the vehicle's lead model against the obstacle's "hitbump" part. On a hit both objects
// record each other and the side that carries a script gets to react.
int CSWTrackFollower::DoObstacleHitCheck(CSWMGObstacle* pObstacle) {
    CAurObject* pGob = pObstacle->m_pGob;  // the obstacle's watched model
    if (pGob == 0)
        return 0;
    Part* pPart = pGob->GetPart(const_cast<char*>("hitbump"));
    if (pPart == 0)
        return 0;
    if (m_models.m_count == 0)
        return 0;

    CAurObject* pModel = m_models.m_pArray[0].pModel;
    Vector vPrevious = pModel->GetPreviousPosition();
    Vector vCurrent = pModel->GetPosition();
    if (pGob->HitCheckPart(pPart, vPrevious, vCurrent, 0, &DefaultHitInfo) == 0)
        return 0;

#if KOTOR1  // K1: both sides get their hit handler (no obstacle-script choice)
    m_nLastObstacleHitId = pObstacle->m_nObjectId;
    OnHitObstacle(0);
    pObstacle->m_nLastFollowerHit = m_nObjectId;
    pObstacle->OnHitFollower(0);
#else
    m_nLastObstacleHitId = pObstacle->m_nObjectId;
    pObstacle->m_nLastFollowerHit = m_nObjectId;
    if (pObstacle->GetScriptName(SCRIPT_ON_HIT_FOLLOWER).IsValid())
        pObstacle->OnHitFollower(0);
    else
        OnHitObstacle(0);
#endif
    return 1;
}

// Appends a model, ignoring nulls and duplicates; the array starts at 16 entries and doubles. A
// new model is attached to the track's "modelhook" straight away when a track exists.
CAurObject* CSWTrackFollower::AddModel(CAurObject* pModel, unsigned char nRotatingModel) {
    if (pModel == 0)
        return pModel;

    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++) {
        if (m_models.m_pArray[i].pModel == pModel)
            return pModel;
    }

    SWMGFollowerModel model;
    model.pModel = pModel;
    model.nRotatingModel = nRotatingModel;
    m_models.Add(model);

    if (m_pTrack != 0)
        pModel->AttachToObject(m_pTrack, const_cast<char*>("modelhook"), 0);
    return pModel;
}

CSWTrackFollower::~CSWTrackFollower() {
#if !KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1: no wake latch
    g_adjustWake = 0;
#endif
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
        delete m_models.m_pArray[i].pModel;
    m_models.Clear();
    // The gun banks are freed only when the death path did not already do it (see OnDeath).
    if ((m_flags & FLAG_DEAD) == 0) {
        for (int i = m_gunBanks.m_count - 1; i >= 0; i--) {
            CSWMGGunBank* pBank = m_gunBanks.m_pArray[i];
            if (pBank != 0) {
                pBank->CSWMGGunBank::~CSWMGGunBank();
                operator delete(pBank);
            }
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    else {
        // A bank still finishing a gun's death outlives the follower, so its GunDie callback must
        // not reach back to it.
        for (int i = m_gunBanks.m_count - 1; i >= 0; i--)
            m_gunBanks.m_pArray[i]->m_pTrackFollower = 0;
    }
#endif
    if (m_pSounds[0] != 0) {
        m_pSounds[0]->Stop();
        delete m_pSounds[0];
    }
    delete m_pTrack;
}

// The "*" animation callback that Go hangs on every model and on the track. It handles the
// "custom_<tag>" and "loop" event families and ignores everything else. Its userdata is the
// minigame object rather than the follower.
void OnAnimKeyEvent(CAurObject* pObject, char* pszEvent, void* pData) {
    CSWMiniGameObject* pMGO = static_cast<CSWMiniGameObject*>(pData);
    uint64_t nPrefix = __strlen_chk("custom_", sizeof("custom_"));
    if (_strnicmp(pszEvent, "custom_", static_cast<StrnicmpCount>(nPrefix)) == 0) {
        CResRef resScript = pMGO->GetScriptName(CSWMiniGameObject::SCRIPT_ON_ANIM_EVENT);
        CExoString sTag(pszEvent + nPrefix);
        pMGO->m_spObject = pObject;
        pMGO->m_sName = sTag;
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, pMGO->m_nObjectId, 1);
        }
        return;
    }
    if (_strnicmp(pszEvent, "loop", static_cast<StrnicmpCount>(__strlen_chk("loop", sizeof("loop")))) != 0)
        return;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(pMGO->AsFollower());
    if (pFollower == 0)
        return;
    int nLoops = pFollower->m_numLoops;
    if (nLoops != -1) {
        pFollower->SetLooping(nLoops - 1);
        CAurObject* pTrack = pFollower->m_pTrack;
        if (pTrack != 0 && (pFollower->m_flags & CSWTrackFollower::FLAG_TRACK_PLAYING) != 0) {
            int nAnim = pFollower->m_numLoops == 1 ? 17 : 20;
            pTrack->PlayAnimation(const_cast<char*>("track"), pFollower->m_fSpeed / 100.0f, nAnim, 0.0f);
        }
    }
    CResRef resScript = pFollower->GetScriptName(CSWMiniGameObject::SCRIPT_ON_TRACK_LOOP);
    if (resScript.IsValid()) {
        CExoString sScript;
        resScript.CopyToString(sScript);
        g_pVirtualMachine->RunScript(&sScript, pFollower->m_nObjectId, 1);
    }
}

// Records where the track's modelhook sat at the start (the player-only "starttrack" hook).
void SetInitialHook(CAurObject* pTrack, char* pszEvent, void* pData) {
    K2_UNUSED(pszEvent);
    Vector vHook(0.0f, 0.0f, 0.0f);
    Quaternion qHook(1.0f, 0.0f, 0.0f, 0.0f);
    pTrack->GetPart(const_cast<char*>("modelhook"))->GetWorld(vHook, qHook);
    static_cast<CSWMiniPlayer*>(pData)->m_vStartHookPosition = vHook;
}

// The track's "ReportKey" event handler; K1 records the reported key for the vehicle's inertia.
void OnReportKey(CAurObject* pObject, char* pszEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pszEvent);
#if !KOTOR1
    K2_UNUSED(pData);
#endif
#if KOTOR1  // K1: records the reported key on the player's two key tables
    CSWMiniGame* pGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pGame->m_bUseInertia == 0)
        return;
    NewControllerKeyReport* pReport = static_cast<NewControllerKeyReport*>(pData);
    CSWMiniPlayer* pPlayer = static_cast<CSWMiniPlayer*>(pGame->m_pPlayer);
    SWMGKeyInfo info;
    info.m_fAlpha = pReport->m_fAlpha;
    info.m_bPosition = pReport->m_pVector != 0;
    info.m_bOrientation = pReport->m_pQuaternion != 0;
    if (pReport->m_pQuaternion != 0)
        info.m_qOrientation = *pReport->m_pQuaternion;
    if (pReport->m_pVector != 0)
        info.m_vPosition = *pReport->m_pVector;
    if (pReport->m_bQuaternion)
        pPlayer->m_keyInfo.Add(info);
    else
        pPlayer->m_keyInfo2.Add(info);
#endif
}

// Replaces the track: drops the old one, re-hangs every model off the new track's "modelhook" at
// that hook's world transform, and gives the player its start-of-track callback.
CAurObject* CSWTrackFollower::ForceSetTrack(CAurObject* pTrack, int nAttachFlag) {
    if (m_pTrack != 0 && m_pTrack != pTrack)
        delete m_pTrack;

    m_pTrack = pTrack;
    m_flags &= ~FLAG_TRACK_PLAYING;
    if (pTrack == 0)
        return 0;

    Part* pPart = pTrack->GetPart(const_cast<char*>("modelhook"));
    Vector vHook(0.0f, 0.0f, 0.0f);
    Quaternion qHook(1.0f, 0.0f, 0.0f, 0.0f);
    pPart->GetWorld(vHook, qHook);

    if (AsPlayer() != 0)
        m_pTrack->RegisterCallBack(SetInitialHook, const_cast<char*>("starttrack"), this, 9999.0f, false);

    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++) {
        CAurObject* pModel = m_models.m_pArray[i].pModel;
        pModel->AttachToObject(m_pTrack, const_cast<char*>("modelhook"),
                               m_models.m_pArray[i].bRotatingModel == 0 ? 0 : nAttachFlag);
        pModel->SetPosition(vHook);
    }

    if (AsPlayer() != 0 && static_cast<CSWMiniPlayer*>(this)->m_pCameraHook != 0)  // idiom-exempt: downcast of self after the As* probe
        static_cast<CSWMiniPlayer*>(this)->m_pCameraHook->AttachToObject(m_pTrack, const_cast<char*>("modelhook"), nAttachFlag);  // idiom-exempt: downcast of self after the As* probe

    return m_pTrack;
}

// Starts the vehicle: arms the animation-key callbacks, puts the models into their ready pose,
// starts the track animation at the current speed and starts the engine sound.
void CSWTrackFollower::Go() {
    if (m_pTrack == 0)
        return;
    if ((m_flags & FLAG_TRACK_PLAYING) != 0)
        return;

    int nAnim = m_numLoops == 1 ? AnimRun::kAnimOnce | AnimRun::kAnimLoopEvents : AnimRun::kAnimLoopEvents;
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++) {
        CAurObject* pModel = m_models.m_pArray[i].pModel;
        pModel->RegisterCallBack(OnAnimKeyEvent, const_cast<char*>("*"), this, 9999.0f, false);
        pModel->PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, 0, 0.0f);
    }

    if (AsPlayer() != 0) {
        m_pTrack->RegisterCallBack(OnReportKey, const_cast<char*>("ReportKey"), 0, 9999.0f, false);
        nAnim |= AnimRun::kAnimTrace;
    }

    m_pTrack->RegisterCallBack(OnAnimKeyEvent, const_cast<char*>("*"), this, 9999.0f, false);
    m_pTrack->PlayAnimation(const_cast<char*>("track"), m_fSpeed / 100.0f, nAnim, 0.0f);
    m_flags |= FLAG_TRACK_PLAYING;

    CExoSoundSource* pSound = m_pSounds[0];
    if (pSound == 0)
        return;
    if (pSound->IsHardwarePlayingSoundPleaseDontUseThis())
        return;
    pSound->SetPosition(GetPosition(), 0.0f);
    pSound->Play();
}

CSWTrackFollower::CSWTrackFollower(CSWMiniGame* pMiniGame, unsigned int nObjectId)
    : CSWMiniGameObject(nObjectId)
{
    m_pMiniGame = pMiniGame;
    m_pTrack = 0;
    m_numLoops = 1;
    m_sphereRadius = 0.0f;
    m_flags &= ~(FLAG_TRACK_PLAYING | FLAG_DEAD);
    m_hitPoints = 100;
    m_nBumpDamage = 0;
    m_fSpeed = 100.0f;
    m_invincibilityTimer = 0.0f;
    m_invincibility = 0.0f;
    m_pSounds[0] = 0;
    m_sounds[0].volume = 0;
    m_sounds[0].frequency = 0;
    m_sounds[0].frequencyIsRandom = 0;
    m_sounds[1].volume = 0;
    m_sounds[1].frequency = 0;
    m_sounds[1].frequencyIsRandom = 0;
    m_nLastObstacleHitId = CSWMiniGameObjectArray::OBJECT_ID_NONE;
    m_hitPointAdjust = 0;
#if !KOTOR1  // K1: no wake latch
    g_adjustWake = 1;
#endif
}

// Stores the new speed and replays the track animation at it. Returns the stored speed.
float CSWTrackFollower::SetSpeed(float speed) {
    m_fSpeed = speed;
    if (m_pTrack == 0)
        return m_fSpeed;
    if ((m_flags & FLAG_TRACK_PLAYING) == 0)
        return m_fSpeed;
    int nAnim = m_numLoops == 1 ? AnimRun::kAnimOnce | AnimRun::kAnimSyncTime
                                 : AnimRun::kAnimLoopEvents | AnimRun::kAnimSyncTime;
    m_pTrack->PlayAnimation(const_cast<char*>("track"), m_fSpeed / 100.0f, nAnim, 0.0f);
    return m_fSpeed;
}

void CSWTrackFollower::OnHitWorld(int nParam) {
    // The script gets the chance to react before the built-in response.
    if (nParam == 0) {
        CResRef resScript = GetScriptName(SCRIPT_ON_HIT_WORLD);
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
            return;
        }
    }
    // Dropping the models' behaviors is what stops a vehicle that hit the world from driving on.
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
        m_models.m_pArray[i].pModel->SetBehavior(0);
}

void CSWTrackFollower::OnAccelerate(int a) {
    // The input event sends 0; the script does the speeding up.
    if (a == 0) {
        CResRef resScript = GetScriptName(SCRIPT_ON_ACCELERATE);
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
        }
    }
}

void CSWTrackFollower::OnBrake(int a) {
    if (a == 0) {
        CResRef resScript = GetScriptName(SCRIPT_ON_BRAKE);
        if (resScript.IsValid()) {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
        }
    }
}

// The player's vehicle in a minigame, with speed and looping setters, the per-frame physics tick
// and the destructor.

















extern int   bDoAcceleration;
extern float fSpeed;
extern float fLastSpeedSet;
extern int   quaternionidentityoptimization;
extern float fInertiaScale;

#if !KOTOR1
// Rotates v by the conjugate of q, with a fast path for the identity.
static inline Vector RotateByQuaternionInverse(const Vector& v, const Quaternion& q)
{
    float w = q.w, x = q.x, y = q.y, z = q.z;
    float nw = -w;
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f))
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * nw;
        float yz = y * z, wy = y * nw;
        float wz = z * nw;
        Vector r;
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}
#else
// K1 rotates a key position forward by the track's orientation.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w, x = q.x, y = q.y, z = q.z;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f))
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = w * x;
        float yz = y * z, wy = w * y;
        float wz = w * z;
        Vector r;
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((wy + xz) + (wy + xz));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((wx + yz) + (wx + yz)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}
#endif

static void snprintff(char* buf, uint64_t n, const char* fmt, ...);

float ClosestDistanceBetweenSegments(const Vector& a1, const Vector& a2, const Vector& b1,
                                     const Vector& b2);

// Constructs the player. Object id 1 is the player's own slot (an enemy passes 2). The key tables
// are reserved but left empty.
CSWMiniPlayer::CSWMiniPlayer(CSWMiniGame* pMiniGame)
    : CSWTrackFollower(pMiniGame, 1)
    , m_minSpeed(100.0f)
    , m_maxSpeed(100.0f)
#if !KOTOR1
    , m_fStartHookOrientationW(1.0f)
#endif
{
    m_nBank = 0;
    m_bBumped = 0;
    m_aps = 0.0f;
    m_pCameraHook = 0;
#if !KOTOR1
    m_nStartHookOrientationXY = 0;
    m_nStartHookOrientationZ = 0;
#endif
#if KOTOR1  // K1: half the reserved capacities
    m_keyInfo.SetSize(4);
    m_keyInfo2.SetSize(4);
    m_paKeyInfo.SetSize(8);
#else
    m_keyInfo.SetSize(8);
    m_keyInfo2.SetSize(8);
    m_paKeyInfo.SetSize(16);
#endif
    m_keyInfo.m_count = 0;
    m_keyInfo2.m_count = 0;
}


int CSWMiniPlayer::Load(CResGFF* pGff, CResStruct* pStruct)
{
    CSWTrackFollower::Load(pGff, pStruct);

    int bSuccess;
    float fMinSpeed = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Minimum_Speed"), bSuccess, -1.0f);
#if KOTOR1  // K1: speeds and acceleration go through the guarded setters
    SetMinSpeed(fMinSpeed);
#else
    m_minSpeed = fMinSpeed;
#endif
    float fMaxSpeed = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Maximum_Speed"), bSuccess, 100.0f);
    if (fMaxSpeed >= 0.0f)
        m_maxSpeed = fMaxSpeed;

    float fAps = -1.0f;
    float fAccelSecs = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Accel_Secs"), bSuccess, -1.0f);
    if (fAccelSecs == 0.0f)
        fAps = fMaxSpeed - fMinSpeed;
    else if (fAccelSecs > 0.0f)
        fAps = (fMaxSpeed - fMinSpeed) / fAccelSecs;
#if KOTOR1
    SetAPS(fAps);
#else
    m_aps = fAps;
#endif

    float fX = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelXPos"), bSuccess, 0.0f);
    float fY = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelYPos"), bSuccess, 0.0f);
    float fZ = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelZPos"), bSuccess, 0.0f);
    m_vTunnelPos.x = fX;
    m_vTunnelPos.y = fY;
    m_vTunnelPos.z = fZ;

    fX = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelXNeg"), bSuccess, 0.0f);
    fY = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelYNeg"), bSuccess, 0.0f);
    fZ = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TunnelZNeg"), bSuccess, 0.0f);
    m_vTunnelNeg.x = fX;
    m_vTunnelNeg.y = fY;
    m_vTunnelNeg.z = fZ;

    m_vTunnelInfinite = pGff->ReadFieldVector(pStruct, const_cast<char*>("TunnelInfinite"), bSuccess, Vector());

    fX = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Start_Offset_X"), bSuccess, 0.0f);
    fY = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Start_Offset_Y"), bSuccess, 0.0f);
    fZ = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Start_Offset_Z"), bSuccess, 0.0f);
    m_vOrigin.x = fX;
    m_vOrigin.y = fY;
    m_vOrigin.z = fZ;
    if ((m_flags & FLAG_TRACK_PLAYING) == 0)
        m_vOffset = m_vOrigin;

    fX = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_Offset_X"), bSuccess, 0.0f);
    fY = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_Offset_Y"), bSuccess, 0.0f);
    fZ = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_Offset_Z"), bSuccess, 0.0f);
    m_vTargetOffset.x = fX;
    m_vTargetOffset.y = fY;
    m_vTargetOffset.z = fZ;
    return 1;
}

// Loads the player's gun as the enemy's LoadGun does, but without the targetting block, since a
// player vehicle aims by hand, and with a gun bank of its own kind.
int CSWMiniPlayer::LoadGun(CResGFF* pGff, CResStruct* pStruct)
{
    CResRef resBulletModel;
    CResRef resGunModel;
    CResRef resFireSound;
    CSWMGBullet bullet;
    CResStruct structBullet;
    int bSuccess;
    int nBank = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("BankID"), bSuccess, static_cast<unsigned int>(-1));
    if (nBank == -1)
        return 0;

    resGunModel = pGff->ReadFieldCResRef(pStruct, const_cast<char*>("Gun_Model"), bSuccess, CResRef(""));
    if (!resGunModel.IsValid())
        return 0;

    bSuccess = pGff->GetStructFromStruct(&structBullet, pStruct, const_cast<char*>("Bullet"));
    if (bSuccess == 0)
        return bSuccess;

    resBulletModel = pGff->ReadFieldCResRef(&structBullet, const_cast<char*>("Bullet_Model"), bSuccess, CResRef(""));
    if (!resBulletModel.IsValid())
        return 0;

    bullet.m_nDamage = pGff->ReadFieldDWORD(&structBullet, const_cast<char*>("Damage"), bSuccess, 0);
    if (bSuccess == 0)
        return 0;
    bullet.m_fLifeSpan = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Lifespan"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_fRateOfFire = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Rate_Of_Fire"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_fSpeed = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Speed"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_nTargetType = pGff->ReadFieldDWORD(&structBullet, const_cast<char*>("Target_Type"), bSuccess, 0);
    if (bSuccess == 0)
        return 0;

    bullet.m_resModel = pGff->ReadFieldCResRef(&structBullet, const_cast<char*>("Collision_Sound"), bSuccess, CResRef(""));
    resFireSound = pGff->ReadFieldCResRef(pStruct, const_cast<char*>("Fire_Sound"), bSuccess, CResRef(""));

    CreateGunBank(nBank, resBulletModel, resGunModel, bullet, resFireSound);
    return 1;
}

#if KOTOR1  // K1: a negative speed is ignored
float CSWMiniPlayer::SetMinSpeed(float speed)
{
    if (speed >= 0.0f)
    {
        m_minSpeed = speed;
        return speed;
    }
    return m_minSpeed;
}
#else
void CSWMiniPlayer::SetMinSpeed(float speed)
{
    m_minSpeed = speed;
}
#endif

#if KOTOR1  // K1: a negative acceleration is ignored
float CSWMiniPlayer::SetAPS(float aps)
{
    if (aps >= 0.0f)
    {
        m_aps = aps;
        return aps;
    }
    return m_aps;
}
#else
void CSWMiniPlayer::SetAPS(float aps)
{
    m_aps = aps;
}
#endif

float CSWMiniPlayer::SetMaxSpeed(float speed)
{
    if (speed >= 0.0f)
    {
        m_maxSpeed = speed;
        return speed;
    }
    return m_maxSpeed;
}

// Accepts -1 to loop forever or any positive count; 0 is rejected. An enemy's SetLooping also
// requires a unit magnitude.
void CSWMiniPlayer::SetLooping(int loop)
{
    if (loop >= -1 && loop != 0)
        m_numLoops = loop;
}

// Runs the per-frame physics tick: accelerates the speed, applies the position delta with its jump
// boost and tunnel-wall clamp, publishes the speed the scripts read and re-aims every attached
// model.
void CSWMiniPlayer::Control(float fDelta, CSWMGPhysicsState& cur, CSWMGPhysicsState& prev)
{
#if KOTOR1
    Vector vCurPos = cur.m_vPosition;
    Vector vPrevPos = prev.m_vPosition;
#endif
    if (m_bBumped == 0 && bDoAcceleration != 0 && m_fSpeed != m_maxSpeed)
    {
        float fStep = m_aps * fDelta;
        if (m_fSpeed < m_maxSpeed)
        {
            float fNew = m_fSpeed + fStep;
            SetSpeed(fNew < m_maxSpeed ? fNew : m_maxSpeed);
        }
        else
        {
            float fNew = m_fSpeed - fStep;
            SetSpeed(fNew > m_maxSpeed ? fNew : m_maxSpeed);
        }
    }

#if KOTOR1  // K1: the reported track keys drive the vehicle's inertia; no jump, no tunnel-wall stop
    m_bBumped = 0;

    if (fSpeed != fLastSpeedSet)
    {
        SetSpeed(fSpeed);
        fLastSpeedSet = fSpeed;
    }

    Vector vDelta = vCurPos - vPrevPos;
    if (m_pMiniGame->m_bUseInertia != 0)
    {
        Vector vPrevHook = m_vStartHookPosition;
        if ((m_flags & FLAG_TRACK_PLAYING) != 0)
        {
            int nKeys1 = m_keyInfo.m_count;
            int nKeys2 = m_keyInfo2.m_count;
            int nLast1;
            int nLast2;
            if (nKeys1 != 0 || nKeys2 != 0)
            {
                Vector vTrackPos = m_pTrack->GetPosition();
                Quaternion qTrack = m_pTrack->GetOrientation();
                nLast2 = nKeys2 - 1;
                nLast1 = nKeys1 - 1;
                int nOut = 0;
                float fLast = 0.0f;
                int i = 0;
                int j = 0;
                float fBase1 = 0.0f;
                float fBase2 = 0.0f;
                while (j < nLast2 && i < nLast1)
                {
                    SWMGKeyInfo* pKey1 = &m_keyInfo[i];
                    SWMGKeyInfo* pKey2 = &m_keyInfo2[j];
                    float a1 = fBase1 + pKey1->m_fAlpha;
                    float a2 = fBase2 + pKey2->m_fAlpha;
                    if (a1 <= a2)
                    {
                        if (nOut != 0 && a1 == fLast)
                        {
                            fBase1 = a1;
                            m_paKeyInfo[nOut - 1] = pKey1;
                        }
                        else
                        {
                            pKey1->m_fAlpha = a1 - fLast;
                            m_paKeyInfo[nOut] = pKey1;
                            fLast = a1;
                            nOut++;
                        }
                        i++;
                    }
                    else
                    {
                        if (nOut != 0 && a2 == fLast)
                        {
                            fBase2 = a2;
                            m_paKeyInfo[nOut - 1] = pKey2;
                        }
                        else
                        {
                            pKey2->m_fAlpha = a2 - fLast;
                            m_paKeyInfo[nOut] = pKey2;
                            fLast = a2;
                            nOut++;
                        }
                        j++;
                    }
                }
                for (; i < nLast1; i++)
                {
                    SWMGKeyInfo* pKey1 = &m_keyInfo[i];
                    float a1 = fBase1 + pKey1->m_fAlpha;
                    if (nOut != 0 && a1 == fLast)
                    {
                        fBase1 = a1;
                        m_paKeyInfo[nOut - 1] = pKey1;
                    }
                    else
                    {
                        pKey1->m_fAlpha = a1 - fLast;
                        m_paKeyInfo[nOut] = pKey1;
                        fLast = a1;
                        nOut++;
                    }
                }
                for (; j < nLast2; j++)
                {
                    SWMGKeyInfo* pKey2 = &m_keyInfo2[j];
                    float a2 = fBase2 + pKey2->m_fAlpha;
                    if (nOut != 0 && a2 == fLast)
                    {
                        fBase2 = a2;
                        m_paKeyInfo[nOut - 1] = pKey2;
                    }
                    else
                    {
                        pKey2->m_fAlpha = a2 - fLast;
                        m_paKeyInfo[nOut] = pKey2;
                        fLast = a2;
                        nOut++;
                    }
                }
                for (int k = 0; k < nOut; k++)
                {
                    SWMGKeyInfo* pKey = m_paKeyInfo[k];
                    if (pKey->m_bOrientation)
                        m_qStartHookOrientation = pKey->m_qOrientation * qTrack;
                    if (pKey->m_bPosition)
                        m_vStartHookPosition = vTrackPos + RotateByQuaternion(pKey->m_vPosition, qTrack);
                    if (pKey->m_fAlpha != 0.0f)
                        cur.m_vVelocity = DoInertia(vPrevHook, m_vStartHookPosition,
                                                    m_qStartHookOrientation, pKey->m_fAlpha) +
                                          cur.m_vVelocity;
                    vPrevHook = m_vStartHookPosition;
                }
            }
            else
            {
                nLast2 = 0;
                nLast1 = 0;
            }
            m_pTrack->GetPart(const_cast<char*>("modelhook"))->GetWorld(m_vStartHookPosition, m_qStartHookOrientation);
            float fTime = fDelta;
            if (nLast1 != 0 || nLast2 != 0)
            {
                float fAlpha1 = m_keyInfo[nLast1].m_fAlpha;
                float fAlpha2 = m_keyInfo2[nLast2].m_fAlpha;
                fTime = fAlpha1 < fAlpha2 ? fAlpha1 : fAlpha2;
            }
            cur.m_vVelocity = DoInertia(vPrevHook, m_vStartHookPosition, m_qStartHookOrientation, fTime) +
                              cur.m_vVelocity;
        }
        m_keyInfo.m_count = 0;
        m_keyInfo2.m_count = 0;
    }

    m_vOffset = vDelta + m_vOffset;
    KeepInTunnel(m_vOffset);
#else
    Vector vDelta = cur.m_vPosition - prev.m_vPosition;
    if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_SWOOP)
    {
        vDelta.z = vDelta.z + m_pMiniGame->m_jumpSpeed * fDelta;
        m_pMiniGame->m_jumpSpeed = m_pMiniGame->m_jumpSpeed + fDelta * -45.8f;
    }

    m_bBumped = 0;

    if (fSpeed != fLastSpeedSet)
    {
        SetSpeed(fSpeed);
        fLastSpeedSet = fSpeed;
    }

    m_vOffset = vDelta + m_vOffset;
    Vector vPreTunnel = m_vOffset;
    KeepInTunnel(m_vOffset);

    if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_SWOOP && __builtin_fabsf(vPreTunnel.x - m_vOffset.x) > 0.01f)
    {
        // The previous state's velocity x and position are saved, the velocity's x cleared and the
        // pair stored back; zeroing the member's x alone is not the same state.
        m_pMiniGame->m_curPhysicsState.m_vVelocity.x = 0.0f;
        Vector vVel = m_pMiniGame->m_prevPhysicsState.m_vVelocity;
        Vector vPos = m_pMiniGame->m_prevPhysicsState.m_vPosition;
        vVel.x = 0.0f;
        m_pMiniGame->m_prevPhysicsState.m_vVelocity = vVel;
        m_pMiniGame->m_prevPhysicsState.m_vPosition = vPos;
    }
#endif

    // Re-aim every attached model's behavior at the vehicle's current offset.
    int nModels = m_models.m_count;
    for (int i = 0; i < nModels; i++)
    {
        if (m_models.m_pArray[i].bRotatingModel == 0)
            continue;
        CAurBehavior* pBehavior = m_models.m_pArray[i].pModel->GetBehavior(-1);
        if (pBehavior == 0)
            continue;
        int nType = pBehavior->GetType();
        if (nType == kBehaviorAttachRotate)
            static_cast<CAurBehaviorAttachRotate*>(pBehavior)->m_vRotation = m_vOffset;
        else if (nType == kBehaviorAttachTranslate)
            static_cast<CAurBehaviorAttachTranslate*>(pBehavior)->m_vTranslation = m_vOffset;
    }

    // The spare camerahook model gets the same treatment, but only the rotate variant is accepted.
    CAurObject* pCameraHook = m_pCameraHook;
    if (pCameraHook != 0)
    {
        CAurBehavior* pBehavior = pCameraHook->GetBehavior(-1);
        if (pBehavior != 0 && pBehavior->GetType() == kBehaviorAttachRotate)
            static_cast<CAurBehaviorAttachRotate*>(pBehavior)->m_vRotation = m_vOffset;
    }
}

// Stamps the origin, which also becomes the offset while the model is not yet attached to a track.
Vector CSWMiniPlayer::SetOrigin(const Vector& v)
{
    m_vOrigin = v;
    if ((m_flags & FLAG_TRACK_PLAYING) == 0)
        m_vOffset = m_vOrigin;
    return m_vOrigin;
}

// Stores the offset and clamps it against the tunnel, returning the clamped value.
Vector CSWMiniPlayer::SetOffset(const Vector& v)
{
    m_vOffset = v;
    KeepInTunnel(m_vOffset);
    return m_vOffset;
}

// Keeps the vehicle inside the tunnel. An axis with m_vTunnelInfinite set wraps into [0, 360)
// rather than clamping; every other axis clamps into [m_vOrigin + m_vTunnelNeg, m_vOrigin +
// m_vTunnelPos], since the two tunnel vectors are deltas from the origin.
void CSWMiniPlayer::KeepInTunnel(Vector& vOffset)
{
    if (m_vTunnelInfinite.x != 0.0f)
    {
        if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_TURRET)
        {
            if (vOffset.x < 0.0f)
                vOffset.x += 360.0f;
            else if (vOffset.x > 359.0f)
                vOffset.x -= 360.0f;
        }
    }
    else if (vOffset.x > m_vOrigin.x + m_vTunnelPos.x)
    {
        vOffset.x = m_vOrigin.x + m_vTunnelPos.x;
    }
    else if (vOffset.x < m_vOrigin.x + m_vTunnelNeg.x)
    {
        vOffset.x = m_vOrigin.x + m_vTunnelNeg.x;
    }

    if (m_vTunnelInfinite.y != 0.0f)
    {
        if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_TURRET)
        {
            if (vOffset.y < 0.0f)
                vOffset.y += 360.0f;
            else if (vOffset.y > 359.0f)
                vOffset.y -= 360.0f;
        }
    }
    else if (vOffset.y > m_vOrigin.y + m_vTunnelPos.y)
    {
        vOffset.y = m_vOrigin.y + m_vTunnelPos.y;
    }
    else if (vOffset.y < m_vOrigin.y + m_vTunnelNeg.y)
    {
        vOffset.y = m_vOrigin.y + m_vTunnelNeg.y;
    }

    if (m_vTunnelInfinite.z != 0.0f)
    {
        if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_TURRET)
        {
            if (vOffset.z < 0.0f)
                vOffset.z += 360.0f;
            else if (vOffset.z > 359.0f)
                vOffset.z -= 360.0f;
        }
    }
    else if (vOffset.z > m_vOrigin.z + m_vTunnelPos.z)
    {
        vOffset.z = m_vOrigin.z + m_vTunnelPos.z;
    }
    else if (vOffset.z < m_vOrigin.z + m_vTunnelNeg.z)
    {
        vOffset.z = m_vOrigin.z + m_vTunnelNeg.z;
    }
}

// Computes the inertia force, the frame's velocity change with the bump plane's normal component
// projected out and the remainder rotated and scaled.
Vector CSWMiniPlayer::DoInertia(const Vector& vFrom, const Vector& vTo, const Quaternion& qRot,
                                float fDelta)
{
    Vector vNormal = m_pMiniGame->BumpPlane(qRot);

    Vector vVel = vTo - vFrom;
    if (fDelta != 0.0f)
    {
        float fInv = 1.0f / fDelta;
        vVel = vVel * fInv;
    }

    Vector vDiff;
    vDiff.x = m_vLastVelocity.x - vVel.x;
    vDiff.y = m_vLastVelocity.y - vVel.y;
    vDiff.z = m_vLastVelocity.z - vVel.z;
    m_vLastVelocity = vVel;

    float fDot = vNormal.x * vDiff.x + vNormal.y * vDiff.y + vNormal.z * vDiff.z;
    vDiff.x -= vNormal.x * fDot;
    vDiff.y -= vNormal.y * fDot;
    vDiff.z -= vNormal.z * fDot;

#if KOTOR1  // K1: the negated-w inverse, then the forward rotation
    Quaternion qInverse = -qRot;
    return RotateByQuaternion(vDiff, qInverse) * fInertiaScale;
#else
    return RotateByQuaternionInverse(vDiff, qRot) * fInertiaScale;
#endif
}

// Creates the player's gun bank as the enemy's CreateGunBank does, but without the targetting
// parameters and building a plain CSWMGGunBank. A negative bank id is refused.
CSWMGGunBank* CSWMiniPlayer::CreateGunBank(int nBankId, const CResRef& resBulletModel,
                                           const CResRef& resGunModel, const CSWMGBullet& bullet,
                                           const CResRef& resFireSound)
{
    if (nBankId < 0)
        return 0;

    CSWMGGunBank* pBank;
    if (m_gunBanks.m_count <= nBankId) {
        for (int i = m_gunBanks.m_count; i < nBankId; i++)
            m_gunBanks.Add(0);
        pBank = new CSWMGGunBank(this, resBulletModel, resGunModel, bullet);
        pBank->SetFireSound(resFireSound);
        m_gunBanks.Add(pBank);
    } else {
        if (m_gunBanks.m_pArray[nBankId] != 0) {
            char szMsg[64];
            snprintff(szMsg, sizeof(szMsg), "CSWMiniPlayer: Recreating gun bank %d", nBankId);
            return 0;
        }
        pBank = new CSWMGGunBank(this, resBulletModel, resGunModel, bullet);
        pBank->SetFireSound(resFireSound);
        m_gunBanks.Insert(pBank, nBankId);
        m_gunBanks.DelIndex(nBankId + 1);
    }
    return pBank;
}

// Runs the vehicle collision pass. Tests the ids in the list, drops the ones whose object has gone
// and sums the hits; bApply gates every mutation. Only the first model can be skipped when the
// averaged hit point is folded back into m_vOffset.
#if KOTOR1
Vector planeproject(const Vector& vNormal, float fDist, const Vector& vPoint);
int SphereIntersect(const Vector& vStart, const Vector& vEnd, const Vector& vCenter, float fRadius);

// K1 runs a relative-motion sphere test instead, projects the push onto the bump plane, moves
// every model, and drops the vehicle to its minimum speed on a self-bump.
int CSWMiniPlayer::DoFollowerHitCheck(CExoArrayList<unsigned int>& list, int bApply)
{
    int nModels = m_models.m_count;
    if (nModels == 0)
        return 0;

    CAurObject* pFollowModel = m_models.m_pArray[0].pModel;
    CSWMiniGameObjectArray* pMgoArray = g_pAppManager->m_pClientExoApp->GetMGOArray();

    Vector vFollowPos = pFollowModel->GetPosition();
    Vector vFollowPrevPos = pFollowModel->GetPreviousPosition();
    Vector vBumpNormal = m_pMiniGame->BumpPlane(pFollowModel->GetOrientation());

    int nHits = 0;
    Vector vSum;
    Vector vCandPos;
    Vector vRelPos;
    Vector vRelPrevPos;
    Vector vCenter;
    Vector vTunnel;

    if (list.m_count < 1)
        return 0;

    int nMaxBump = 0;
    float fPlaneDist =
        -(vFollowPos.x * vBumpNormal.x + vFollowPos.y * vBumpNormal.y + vFollowPos.z * vBumpNormal.z);
    for (int i = list.m_count - 1; i >= 0; i--)
    {
        CSWMiniEnemy* pEnemy = static_cast<CSWMiniEnemy*>(pMgoArray->GetMiniGameObject(list.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy == 0)
        {
            list.DelIndex(i);
            continue;
        }

        if (pEnemy->m_bTrigger == 0 && m_pMiniGame->m_bDoBumping == 0)
            continue;

        if (pEnemy->m_models.m_count < 1)
            break;
        CAurObject* pCandFollowModel = pEnemy->m_models.m_pArray[0].pModel;
        if (pCandFollowModel == 0)
            break;

        vCandPos = pCandFollowModel->GetPosition();
        Vector vCandPrevPos = pCandFollowModel->GetPreviousPosition();
        float fEnemyRadius = pEnemy->m_sphereRadius;
        vRelPrevPos = vFollowPrevPos - vCandPrevPos;
        vRelPos = vFollowPos - vCandPos;
        if (!SphereIntersect(vRelPrevPos, vRelPos, vCenter, fEnemyRadius + m_sphereRadius))
            continue;

        if (pEnemy->m_bTrigger != 0)
        {
            if (bApply == 0)
                continue;
            m_nLastFollowerHit = pEnemy->m_nObjectId;
            OnHitFollower(0);
            pEnemy->m_nLastFollowerHit = m_nObjectId;
            pEnemy->OnHitFollower(0);
            continue;
        }

        vCandPos = planeproject(vBumpNormal, fPlaneDist, vCandPos);
        Vector vDir = normalize(vFollowPos - vCandPos);
        Vector vPush = vCandPos + vDir * (fEnemyRadius + m_sphereRadius + 0.1f) - vFollowPos;

        if (bApply != 0)
        {
            if (!(pEnemy->m_invincibilityTimer > 0.0f))
            {
                pEnemy->PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
                pEnemy->PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
                int nBanks = pEnemy->m_gunBanks.m_count;
                for (int j = 0; j < nBanks; j++)
                    pEnemy->m_gunBanks.m_pArray[j]->Hit();
                pEnemy->AdjustHitPoints(-m_nBumpDamage);
                pEnemy->m_invincibilityTimer = pEnemy->m_invincibility;
            }
            if (!(nMaxBump > pEnemy->m_nBumpDamage))
                nMaxBump = pEnemy->m_nBumpDamage;
            m_nLastFollowerHit = pEnemy->m_nObjectId;
            OnHitFollower(0);
            pEnemy->m_nLastFollowerHit = m_nObjectId;
            pEnemy->OnHitFollower(0);
        }

        vSum = vSum + vPush;
        nHits++;
    }

    if (nHits != 0)
    {
        float fInv = 1.0f / static_cast<float>(nHits);
        vTunnel = vSum * fInv + m_vOffset;
        KeepInTunnel(vTunnel);
        Vector vMove = vFollowPos + vTunnel - m_vOffset;
        m_vOffset = vTunnel;
        for (int i = 0; i < nModels; i++)
            m_models.m_pArray[i].pModel->SetPosition(vMove);

        if (m_invincibilityTimer == 0.0f)
        {
            SetSpeed(m_minSpeed);
            m_invincibilityTimer = m_invincibility;
            if (bApply != 0)
            {
                PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
                PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
                int nBanks = m_gunBanks.m_count;
                for (int i = 0; i < nBanks; i++)
                    m_gunBanks.m_pArray[i]->Hit();
                AdjustHitPoints(-nMaxBump);
                m_bBumped = 1;
            }
        }
    }
    return nHits;
}
#else
int CSWMiniPlayer::DoFollowerHitCheck(CExoArrayList<unsigned int>& list, int bApply)
{
    int nModels = m_models.m_count;
    if (nModels == 0)
        return 0;

    CAurObject* pFollowModel = m_models.m_pArray[0].pModel;
    CSWMiniGameObjectArray* pMgoArray = g_pAppManager->m_pClientExoApp->GetMGOArray();

    Vector vFollowPos = pFollowModel->GetPosition();
    Vector vFollowPrevPos = pFollowModel->GetPreviousPosition();
    Quaternion qFollow = pFollowModel->GetOrientation();

    m_pMiniGame->BumpPlane(qFollow);
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);

    int nHits = 0;
    int nMaxBump = 0;
    Vector vSum;
    Vector vCandPrevPos;
    Vector vCandPos;
    Vector vDir;
    Vector vTunnel;

    for (int i = list.m_count - 1; i >= 0; i--)
    {
        CSWMiniEnemy* pEnemy = static_cast<CSWMiniEnemy*>(pMgoArray->GetMiniGameObject(list.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy == 0)
        {
            list.DelIndex(i);
            continue;
        }

        if (pEnemy->m_bTrigger == 0 && m_pMiniGame->m_bDoBumping == 0)
            continue;

        if (pEnemy->m_models.m_count < 1)
            break;
        CAurObject* pCandFollowModel = pEnemy->m_models.m_pArray[0].pModel;
        if (pCandFollowModel == 0)
            break;

        vCandPos = pCandFollowModel->GetPosition();
        vCandPrevPos = pCandFollowModel->GetPreviousPosition();

        float fEnemyRadius = pEnemy->m_sphereRadius;
        float fDistance =
            ClosestDistanceBetweenSegments(vFollowPrevPos, vFollowPos, vCandPrevPos, vCandPos);
        if (!(fDistance < fEnemyRadius + m_sphereRadius))
            continue;

        if (pEnemy->m_bTrigger != 0)
        {
            if (bApply == 0)
                continue;
            m_nLastFollowerHit = pEnemy->m_nObjectId;
            OnHitFollower(0);
            pEnemy->m_nLastFollowerHit = m_nObjectId;
            pEnemy->OnHitFollower(0);
            continue;
        }

        vDir = normalize(vFollowPos - vCandPos);
        // Both radii are captured before the hit scripts run (OnDamage / OnHitFollower can call
        // SWMG_SetSphereRadius); the push uses these, not the post-script values.
        float fMyRadius = m_sphereRadius;

        if (bApply != 0)
        {
            if (!(pEnemy->m_invincibilityTimer > 0.0f))
            {
                pEnemy->PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
                pEnemy->PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
                int nBanks = pEnemy->m_gunBanks.m_count;
                for (int j = 0; j < nBanks; j++)
                    pEnemy->m_gunBanks.m_pArray[j]->Hit();
                pEnemy->m_hitPointAdjust = -m_nBumpDamage;
                pEnemy->OnDamage(0);
                pEnemy->m_invincibilityTimer = pEnemy->m_invincibility;
            }
            m_nLastFollowerHit = pEnemy->m_nObjectId;
            int nBumpDamage = pEnemy->m_nBumpDamage;
            OnHitFollower(0);
            if (nBumpDamage > nMaxBump)
                nMaxBump = nBumpDamage;
            pEnemy->m_nLastFollowerHit = m_nObjectId;
            pEnemy->OnHitFollower(0);
        }

        vSum = vSum + (vCandPos + vDir * (fEnemyRadius + fMyRadius + 1.1f) - vFollowPos);
        nHits++;
    }

    if (nHits != 0)
    {
        float fInv = 1.0f / static_cast<float>(nHits);
        vTunnel = m_vOffset + vSum * fInv;
        KeepInTunnel(vTunnel);
        Vector vMove = vFollowPos + vTunnel - m_vOffset;
        m_vOffset = vTunnel;
        for (int i = 0; i < nModels - 1; i++)
            m_models.m_pArray[i].pModel->SetPosition(vMove);

        // A vehicle that hit nothing takes no bump damage and arms no invincibility.
        if (m_invincibilityTimer <= 0.0f)
        {
            m_invincibilityTimer = m_invincibility * 2.0f;
            if (bApply != 0)
            {
                PlayAnimation(const_cast<char*>("damage"), 1.0f, 1);
                PlayAnimation(const_cast<char*>("Ready_01"), 1.0f, AnimRun::kAnimQueue);
                int nBanks = m_gunBanks.m_count;
                for (int i = 0; i < nBanks; i++)
                    m_gunBanks.m_pArray[i]->Hit();
                m_hitPointAdjust = -nMaxBump;
                OnDamage(0);
                m_bBumped = 1;
            }
        }
    }
    return nHits;
}
#endif

// Fires every gun bank. The count is read once because Fire() is virtual.
void CSWMiniPlayer::Fire()
{
    int nCount = m_gunBanks.m_count;
    for (int i = 0; i < nCount; i++)
        m_gunBanks.m_pArray[i]->Fire();
}

// Hooks up or clears the spare camerahook model; the attach type comes from the minigame when the
// hook flag is set.
void CSWMiniPlayer::SetCameraHook(CAurObject* pModel, unsigned char bHook)
{
    int nAttachType = 0;
    if (bHook != 0)
        nAttachType = m_pMiniGame->GetAttachType();
    delete m_pCameraHook;
    m_pCameraHook = pModel;
    m_bCameraHook = bHook;
    if (pModel != 0 && m_pTrack != 0)
        pModel->AttachToObject(m_pTrack, const_cast<char*>("modelhook"), nAttachType);
}

// Rolls the vehicle: sets the bank to nAmount (or adds to it) clamped to ten either way, then plays
// the matching bank animation.
void CSWMiniPlayer::AdjustPosition(int nAmount, int bAdd)
{
    char szBank[10];
#if !KOTOR1  // K1: no early return for a swoop race without lateral acceleration
    if (m_pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_SWOOP && m_pMiniGame->m_lateralAccelPerSec == 0.0f)
        return;
#endif

    if (bAdd != 0)
        nAmount += m_nBank;
    else if (m_nBank == nAmount)
        return;

    m_nBank = nAmount;
    if (m_nBank < -10)
        m_nBank = -10;
    else if (m_nBank > 10)
        m_nBank = 10;

    if (m_nBank == 0)
        snprintff(szBank, sizeof(szBank), "Ready_01");
    else
        snprintff(szBank, sizeof(szBank), "Bank%c_%02d", m_nBank < 0 ? 'L' : 'R',
                  m_nBank < 0 ? -m_nBank : m_nBank);
    PlayAnimation(szBank, 1.0f, AnimRun::kAnimSyncTime);
}

CSWMiniPlayer::~CSWMiniPlayer()
{
    delete m_pCameraHook;
}

extern template class SafePointer<CAurObject>;



CSWMGObstacle::CSWMGObstacle(CSWMiniGame* pGame, CAurObject* pGob) : CSWMiniGameObject(3)
{
    K2_UNUSED(pGame);
    m_pGob = pGob;
}

CSWMGObstacle::~CSWMGObstacle()
{
    delete m_pGob;
}

int CSWMGObstacle::Load(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct sScripts;
    if (pGff->GetStructFromStruct(&sScripts, pStruct, const_cast<char*>("Scripts")))
        LoadScripts(pGff, pStruct, sScripts);
    OnCreate();
    return 1;
}

CExoString CSWMGObstacle::GetName()
{
    // An obstacle is named after the model it draws through; an unbound one gets the empty string.
    return CExoString(m_pGob ? static_cast<char*>(m_pGob.m_pObject->GetModelName()) : "");
}

CResRef CSWMGObstacle::GetScriptName(int nIndex)
{
    // An out-of-range index gives an empty resref.
    if (static_cast<unsigned int>(nIndex) <= 4)
        return m_aScripts[nIndex];
    return CResRef();
}

void CSWMGObstacle::SetScriptName(int nIndex, const CResRef& r)
{
    if (static_cast<unsigned int>(nIndex) <= 4)
        m_aScripts[nIndex] = r;
}

void CSWMGObstacle::LoadScripts(CResGFF* pGff, CResStruct* pStruct, CResStruct& sScripts)
{
    CSWMiniGameObject::LoadScripts(pGff, pStruct, sScripts);
}

static void snprintff(char* buf, uint64_t n, const char* fmt, ...);

// Loads the enemy's gun through a guard chain over the GFF's gun block that fills a
// CSWMGTargettingParameters, the nested "Bullet" struct's CSWMGBullet and three resrefs, then hands
// them all to CreateGunBank. Returns 1 on success and 0 on every failure. The GFF's own names are
// misleading: the model resref sits under "Bullet_Model", the bullet's model under
// "Collision_Sound", and "Fire_Sound" belongs to the gun bank rather than the bullet.
int CSWMiniEnemy::LoadGun(CResGFF* pGff, CResStruct* pStruct)
{
    CResRef resBulletModel;
    CResRef resGunModel;
    CResRef resFireSound;
    CSWMGBullet bullet;
    CResStruct structBullet;
    int bSuccess;
    CSWMGTargettingParameters targetting;
    int nBank = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("BankID"), bSuccess, static_cast<unsigned int>(-1));
    if (nBank == -1)
        return 0;

    resGunModel = pGff->ReadFieldCResRef(pStruct, const_cast<char*>("Gun_Model"), bSuccess, CResRef(""));
    if (!resGunModel.IsValid())
        return 0;

    targetting.m_fSensingRadius = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Sensing_Radius"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    targetting.m_fHorizontalSpread = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Horiz_Spread"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    targetting.m_fVerticalSpread = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Vert_Spread"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    targetting.m_fInaccuracy = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Inaccuracy"), bSuccess, 0.0f);

    bSuccess = pGff->GetStructFromStruct(&structBullet, pStruct, const_cast<char*>("Bullet"));
    if (bSuccess == 0)
        return bSuccess;

    resBulletModel = pGff->ReadFieldCResRef(&structBullet, const_cast<char*>("Bullet_Model"), bSuccess, CResRef(""));
    if (!resBulletModel.IsValid())
        return 0;

    bullet.m_nDamage = pGff->ReadFieldDWORD(&structBullet, const_cast<char*>("Damage"), bSuccess, 0);
    if (bSuccess == 0)
        return 0;
    bullet.m_fLifeSpan = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Lifespan"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_fRateOfFire = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Rate_Of_Fire"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_fSpeed = pGff->ReadFieldFLOAT(&structBullet, const_cast<char*>("Speed"), bSuccess, 0.0f);
    if (bSuccess == 0)
        return 0;
    bullet.m_nTargetType = pGff->ReadFieldDWORD(&structBullet, const_cast<char*>("Target_Type"), bSuccess, 0);
    if (bSuccess == 0)
        return 0;

    bullet.m_resModel = pGff->ReadFieldCResRef(&structBullet, const_cast<char*>("Collision_Sound"), bSuccess, CResRef(""));
    resFireSound = pGff->ReadFieldCResRef(pStruct, const_cast<char*>("Fire_Sound"), bSuccess, CResRef(""));

    CreateGunBank(nBank, resBulletModel, resGunModel, bullet, targetting, resFireSound);
    return 1;
}

// Builds the targetting gun bank for this gun configuration and files it in the base's m_gunBanks
// list under the GFF's "BankID", padding the list with nulls when the id skips ahead. A bank id
// that is already taken is refused. The new bank targets the minigame's player.
CSWMGTargettingGunBank* CSWMiniEnemy::CreateGunBank(int nBankId, const CResRef& resBulletModel,
        const CResRef& resGunModel, const CSWMGBullet& bullet,
        const CSWMGTargettingParameters& targetting, const CResRef& resFireSound)
{
    if (nBankId < 0)
        return 0;

    CSWMGTargettingGunBank* pBank;
    if (m_gunBanks.m_count <= nBankId) {
        for (int i = m_gunBanks.m_count; i < nBankId; i++)
            m_gunBanks.Add(0);
        pBank = new CSWMGTargettingGunBank(this, resBulletModel, resGunModel, bullet, targetting,
                                           m_pMiniGame->m_pPlayer);
        pBank->SetFireSound(resFireSound);
        m_gunBanks.Add(pBank);
    } else {
        if (m_gunBanks.m_pArray[nBankId] != 0) {
            char szMsg[64];
            snprintff(szMsg, sizeof(szMsg), "CSWMiniEnemy: Recreating gun bank %d", nBankId);
            return 0;
        }
        // A bank that fills a padded-out placeholder keeps the constructor's empty fire sound.
        pBank = new CSWMGTargettingGunBank(this, resBulletModel, resGunModel, bullet, targetting,
                                           m_pMiniGame->m_pPlayer);
        m_gunBanks.Insert(pBank, nBankId);
        m_gunBanks.DelIndex(nBankId + 1);
    }
    return pBank;
}

// Loads the base's fields, then the enemy's own "Trigger" field. Always reports success.
int CSWMiniEnemy::Load(CResGFF* pGff, CResStruct* pStruct)
{
    CSWTrackFollower::Load(pGff, pStruct);
    int bSuccess;
    m_bTrigger = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("Trigger"), bSuccess, 0);
    return 1;
}

// An enemy only loops in a unit direction, so any other magnitude leaves the previous value.
void CSWMiniEnemy::SetLooping(int direction)
{
    int magnitude = direction < 0 ? -direction : direction;
    if (magnitude == 1)
        m_numLoops = direction;
}

// K1 needs these template members out of line from this TU.
