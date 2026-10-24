#include "serverexoappinternal.h"
#include "swmgobject.h"
#include "swmgtrackfollower.h"
#include "swminigame.h"
#include "swmgguns.h"
#include "aurmodel.h"

// The nwscript.nss command ids (the routine's index in nwscript.nss) of the routines that share
// an executor here.
enum {
    CMD_SWMG_GetLastBulletHitDamage = 587,
    CMD_SWMG_GetLastBulletHitTarget = 588,
    CMD_SWMG_AdjustFollowerHitPoints = 590,
    CMD_SWMG_OnBulletHit = 591,
    CMD_SWMG_OnObstacleHit = 592,
    CMD_SWMG_GetLastFollowerHit = 593,
    CMD_SWMG_GetLastObstacleHit = 594,
    CMD_SWMG_GetLastBulletFiredDamage = 595,
    CMD_SWMG_GetLastBulletFiredTarget = 596,
    CMD_SWMG_OnDeath = 598,
    CMD_SWMG_IsFollower = 599,
    CMD_SWMG_IsPlayer = 600,
    CMD_SWMG_IsEnemy = 601,
    CMD_SWMG_IsTrigger = 602,
    CMD_SWMG_IsObstacle = 603,
    CMD_SWMG_SetFollowerHitPoints = 604,
    CMD_SWMG_OnDamage = 605,
    CMD_SWMG_GetCameraNearClip = 608,
    CMD_SWMG_GetCameraFarClip = 609,
    CMD_SWMG_GetEnemyCount = 612,
    CMD_SWMG_GetEnemy = 613,
    CMD_SWMG_GetObstacleCount = 614,
    CMD_SWMG_GetObstacle = 615,
    CMD_SWMG_GetHitPoints = 616,
    CMD_SWMG_GetMaxHitPoints = 617,
    CMD_SWMG_SetMaxHitPoints = 618,
    CMD_SWMG_GetSphereRadius = 619,
    CMD_SWMG_SetSphereRadius = 620,
    CMD_SWMG_GetNumLoops = 621,
    CMD_SWMG_SetNumLoops = 622,
    CMD_SWMG_GetGunBankCount = 624,
    CMD_SWMG_GetGunBankBulletModel = 625,
    CMD_SWMG_GetGunBankGunModel = 626,
    CMD_SWMG_GetGunBankDamage = 627,
    CMD_SWMG_GetGunBankTimeBetweenShots = 628,
    CMD_SWMG_GetGunBankLifespan = 629,
    CMD_SWMG_GetGunBankSpeed = 630,
    CMD_SWMG_GetGunBankTarget = 631,
    CMD_SWMG_SetGunBankBulletModel = 632,
    CMD_SWMG_SetGunBankGunModel = 633,
    CMD_SWMG_SetGunBankDamage = 634,
    CMD_SWMG_SetGunBankTimeBetweenShots = 635,
    CMD_SWMG_SetGunBankLifespan = 636,
    CMD_SWMG_SetGunBankSpeed = 637,
    CMD_SWMG_SetGunBankTarget = 638,
    CMD_SWMG_IsGunBankTargetting = 640,
    CMD_SWMG_GetPlayerOffset = 641,
    CMD_SWMG_GetPlayerInvincibility = 642,
    CMD_SWMG_GetPlayerSpeed = 643,
    CMD_SWMG_GetPlayerMinSpeed = 644,
    CMD_SWMG_GetPlayerAccelerationPerSecond = 645,
    CMD_SWMG_GetPlayerTunnelPos = 646,
    CMD_SWMG_SetPlayerOffset = 647,
    CMD_SWMG_SetPlayerInvincibility = 648,
    CMD_SWMG_SetPlayerSpeed = 649,
    CMD_SWMG_SetPlayerMinSpeed = 650,
    CMD_SWMG_SetPlayerAccelerationPerSecond = 651,
    CMD_SWMG_SetPlayerTunnelPos = 652,
    CMD_SWMG_GetPlayerTunnelNeg = 653,
    CMD_SWMG_SetPlayerTunnelNeg = 654,
    CMD_SWMG_GetPlayerOrigin = 655,
    CMD_SWMG_SetPlayerOrigin = 656,
    CMD_SWMG_SetGunBankHorizontalSpread = 661,
    CMD_SWMG_SetGunBankVerticalSpread = 662,
    CMD_SWMG_SetGunBankSensingRadius = 663,
    CMD_SWMG_SetGunBankInaccuracy = 664,
    CMD_SWMG_GetIsInvulnerable = 665,
    CMD_SWMG_GetPlayerMaxSpeed = 667,
    CMD_SWMG_SetPlayerMaxSpeed = 668,
    CMD_SWMG_GetSoundFrequency = 683,
    CMD_SWMG_SetSoundFrequency = 684,
    CMD_SWMG_GetSoundFrequencyIsRandom = 685,
    CMD_SWMG_SetSoundFrequencyIsRandom = 686,
    CMD_SWMG_GetSoundVolume = 687,
    CMD_SWMG_SetSoundVolume = 688,
    CMD_SWMG_GetPlayerTunnelInfinite = 717,
    CMD_SWMG_SetPlayerTunnelInfinite = 718,
};

// The object id a mini-game command pushes when it has no object to report.
enum {
    MINIGAME_OBJECT_NONE = 0xff,
};

void CSWVirtualMachineCommands::InitializeSWMGCommands() {
    m_paCommands[582] = &CSWVirtualMachineCommands::ExecuteCommandAurPostString;
    m_paCommands[583] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastEvent;
    m_paCommands[584] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastEventModelName;
    m_paCommands[585] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetObjectByName;
    m_paCommands[586] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_PlayAnimation;
    m_paCommands[587] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletInfo;
    m_paCommands[588] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletInfo;
    m_paCommands[589] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletShooter;
    m_paCommands[590] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_AdjustFollowerHitPoints;
    m_paCommands[591] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_DefaultImplementations;
    m_paCommands[592] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_DefaultImplementations;
    m_paCommands[593] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastObjectHit;
    m_paCommands[594] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastObjectHit;
    m_paCommands[595] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletInfo;
    m_paCommands[596] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletInfo;
    m_paCommands[597] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetObjectName;
    m_paCommands[598] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_DefaultImplementations;
    m_paCommands[599] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething;
    m_paCommands[600] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething;
    m_paCommands[601] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething;
    m_paCommands[602] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething;
    m_paCommands[603] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething;
    m_paCommands[604] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_AdjustFollowerHitPoints;
    m_paCommands[605] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_DefaultImplementations;
    m_paCommands[606] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastHPChange;
    m_paCommands[607] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_RemoveAnimation;
    m_paCommands[608] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetCameraClipping;
    m_paCommands[609] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetCameraClipping;
    m_paCommands[610] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetCameraClipping;
    m_paCommands[611] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayer;
    m_paCommands[612] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameCounts;
    m_paCommands[613] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameObjects;
    m_paCommands[614] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameCounts;
    m_paCommands[615] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameObjects;
    m_paCommands[616] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo;
    m_paCommands[617] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo;
    m_paCommands[618] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerIntInfo;
    m_paCommands[619] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerFloatInfo;
    m_paCommands[620] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerFloatInfo;
    m_paCommands[621] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo;
    m_paCommands[622] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerIntInfo;
    m_paCommands[623] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerPosition;
    m_paCommands[624] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo;
    m_paCommands[625] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankStringInfo;
    m_paCommands[626] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankStringInfo;
    m_paCommands[627] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankIntInfo;
    m_paCommands[628] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankFloatInfo;
    m_paCommands[629] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankFloatInfo;
    m_paCommands[630] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankFloatInfo;
    m_paCommands[631] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankIntInfo;
    m_paCommands[632] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankStringInfo;
    m_paCommands[633] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankStringInfo;
    m_paCommands[634] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankIntInfo;
    m_paCommands[635] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankFloatInfo;
    m_paCommands[636] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankFloatInfo;
    m_paCommands[637] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankFloatInfo;
    m_paCommands[638] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankIntInfo;
    m_paCommands[639] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletHitPart;
    m_paCommands[640] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankIntInfo;
    m_paCommands[641] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo;
    m_paCommands[642] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerFloatInfo;
    m_paCommands[643] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerFloatInfo;
    m_paCommands[644] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerFloatInfo;
    m_paCommands[667] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerFloatInfo;
    m_paCommands[645] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerFloatInfo;
    m_paCommands[646] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo;
    m_paCommands[647] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo;
    m_paCommands[648] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerFloatInfo;
    m_paCommands[649] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerFloatInfo;
    m_paCommands[650] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerFloatInfo;
    m_paCommands[668] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerFloatInfo;
    m_paCommands[651] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerFloatInfo;
    m_paCommands[652] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo;
    m_paCommands[653] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo;
    m_paCommands[654] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo;
    m_paCommands[655] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo;
    m_paCommands[656] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo;
    m_paCommands[657] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo;
    m_paCommands[658] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo;
    m_paCommands[659] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo;
    m_paCommands[660] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo;
    m_paCommands[661] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetTargettingGunBankFloatInfo;
    m_paCommands[662] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetTargettingGunBankFloatInfo;
    m_paCommands[663] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetTargettingGunBankFloatInfo;
    m_paCommands[664] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetTargettingGunBankFloatInfo;
    m_paCommands[665] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo;
    m_paCommands[666] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_StartInvulnerability;
    m_paCommands[683] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSoundProperty;
    m_paCommands[684] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetSoundProperty;
    m_paCommands[685] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSoundProperty;
    m_paCommands[686] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetSoundProperty;
    m_paCommands[687] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSoundProperty;
    m_paCommands[688] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetSoundProperty;
    m_paCommands[718] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo;
    m_paCommands[717] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo;
    m_paCommands[521] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLateralAccelPerSec;
    m_paCommands[520] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetLateralAccelPerSec;
#if !KOTOR1
    m_paCommands[782] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSwoopUpgrade;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[789] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTrackPosition;
    m_paCommands[790] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerPosition;
    m_paCommands[792] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_DestroyMiniGameObject;
#endif
#if !KOTOR1
    m_paCommands[804] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_SetJumpSpeed;
    m_paCommands[825] = &CSWVirtualMachineCommands::ExecuteCommandSWMG_PlayerApplyForce;
#endif
}

// SWMG_AurPostString (K2 only) consumes the (text, x, y, lifetime) argument list and drops it, so
// only the pop failures are observable.
int CSWVirtualMachineCommands::ExecuteCommandAurPostString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sText;
    int nX;
    int nY;
    float fLifetime;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nX))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nY))
        return -2001;
    if (!g_pVirtualMachine->StackPopFloat(&fLifetime))
        return -2001;
#if KOTOR1  // K1 still reads the text (its on-screen print compiled out)
    const char* pszText = sText.CStr();
    (void)pszText;
#endif
    return 0;
}

// Answers with the object's own name; no event record is consulted.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastEvent(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    CExoString sEvent;
    if (pObject != 0)
        sEvent = pObject->m_sName;
    if (!g_pVirtualMachine->StackPushString(CExoString(sEvent.CStr())))
        return -2000;
    return 0;
}

// The model name of the Gob the object is drawn with, not the object's own name.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastEventModelName(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    CExoString sName;
    if (pObject != 0 && pObject->m_spObject.m_pObject != 0)
        sName = static_cast<const char*>(pObject->m_spObject.m_pObject->GetModelName());
    if (!g_pVirtualMachine->StackPushString(sName.CStr()))
        return -2000;
    return 0;
}

// Name lookup in the mini-game object table; a miss pushes the "no object" sentinel.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetObjectByName(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    unsigned long long oidObject = MINIGAME_OBJECT_NONE;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObjectByName(sName);
    if (pObject != 0)
        oidObject = pObject->m_nObjectId;
    if (!g_pVirtualMachine->StackPushObject(oidObject))
        return -2000;
    return 0;
}

// Drives one mini-game object's model animation. The flags fold into one mask: bit 0 is "do not
// loop", bit 3 the overlay flag and bit 5 the queue flag. An empty animation name is the stop
// form and ignores them all.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_PlayAnimation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sAnimation;
    unsigned long long oidObject;
    int bLooping;
    int bOverlay;
    int bQueue;
    CSWMiniGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopString(&sAnimation))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&bLooping))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&bQueue))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&bOverlay))
        return -2001;
    pObject = g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0);
    if (pObject == 0)
        return 0;
    int nFlags = 0;
    if (bLooping == 0)
        nFlags = AnimRun::kAnimOnce;
    if (bOverlay != 0)
        nFlags |= AnimRun::kAnimOverlay;
    if (bQueue != 0)
        nFlags |= AnimRun::kAnimQueue;
#if KOTOR1  // K1: no empty-name reset arm
    pObject->PlayAnimation(static_cast<char*>(sAnimation.CStr()), 1.0f, nFlags);
#else
    if (sAnimation == "")
        pObject->PlayAnimation(0, 0.0f, 2);
    else
        pObject->PlayAnimation(static_cast<char*>(sAnimation.CStr()), 1.0f, nFlags);
#endif
    return 0;
}

// Four ids share one handler, answering the object's own incoming bullet, or the last bullet it
// fired, which only a follower has. Anything else answers -1.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    int nValue = -1;
    if (pObject != 0) {
        switch (nCommand) {
        case CMD_SWMG_GetLastBulletHitDamage:
            nValue = pObject->m_bullet.m_nDamage;
            break;
        case CMD_SWMG_GetLastBulletHitTarget:
            nValue = pObject->m_bullet.m_nTargetType;
            break;
        case CMD_SWMG_GetLastBulletFiredDamage:
            if (pObject->AsFollower() != 0)
                nValue = static_cast<CSWTrackFollower*>(pObject->AsFollower())->m_lastFiredBullet.m_nDamage;
            break;
        case CMD_SWMG_GetLastBulletFiredTarget:
            if (pObject->AsFollower() != 0)
                nValue =
                    static_cast<CSWTrackFollower*>(pObject->AsFollower())->m_lastFiredBullet.m_nTargetType;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return -2000;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletShooter(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject = g_pAppManager->m_pClientExoApp->GetMGOArray()
                                     ->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    unsigned int nShooter = MINIGAME_OBJECT_NONE;
    if (pObject) {
        CSWTrackFollower* pShooter = pObject->GetLastBulletHitShooter();
        if (pShooter)
            nShooter = pShooter->m_nObjectId;
    }
    if (!g_pVirtualMachine->StackPushObject(nShooter))
        return -2000;
    return 0;
}

// 590 queues a hit-point delta, 604 writes an absolute value.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_AdjustFollowerHitPoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidFollower;
    int nValue;
    if (!g_pVirtualMachine->StackPopObject(&oidFollower))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return -2001;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(oidFollower), 4));
    if (pFollower != 0) {
        if (nCommand == CMD_SWMG_AdjustFollowerHitPoints)
            pFollower->AdjustHitPoints(nValue);
        else if (nCommand == CMD_SWMG_SetFollowerHitPoints)
            pFollower->SetHitPoints(nValue);
    }
    return 0;
}

// The escape hatch a mini-game script calls to run the engine's built-in behaviour. Only the
// bullet hook works on any mini-game object; the other three are vehicle-only.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_DefaultImplementations(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    if (pObject != 0) {
        switch (nCommand) {
        case CMD_SWMG_OnBulletHit:
            pObject->OnHitBullet(1);
            break;
        case CMD_SWMG_OnObstacleHit:
            if (pObject->AsFollower() != 0)
                static_cast<CSWTrackFollower*>(pObject->AsFollower())->OnHitObstacle(1);
            break;
        case CMD_SWMG_OnDeath:
            if (pObject->AsFollower() != 0)
                static_cast<CSWTrackFollower*>(pObject->AsFollower())->OnDeath(1);
            break;
        case CMD_SWMG_OnDamage:
            if (pObject->AsFollower() != 0)
                static_cast<CSWTrackFollower*>(pObject->AsFollower())->OnDamage(1);
            break;
        }
    }
    return 0;
}

// The two share one body, which resolves OBJECT_SELF through the mini-game object table and pushes
// the hit object's id, or an invalid id when there was none.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastObjectHit(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    unsigned int nObjectId = MINIGAME_OBJECT_NONE;
    if (pObject != 0) {
        if (nCommand == CMD_SWMG_GetLastFollowerHit) {
            CSWTrackFollower* pFollower = pObject->GetLastFollowerHit();
            if (pFollower != 0)
                nObjectId = pFollower->m_nObjectId;
        } else if (nCommand == CMD_SWMG_GetLastObstacleHit) {
            CSWTrackFollower* pSelf = static_cast<CSWTrackFollower*>(pObject->AsFollower());
            if (pSelf != 0) {
                CSWMiniGameObject* pObstacle = pSelf->GetLastObstacleHit();
                if (pObstacle != 0)
                    nObjectId = pObstacle->m_nObjectId;
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(nObjectId))
        return -2000;
    return 0;
}

// The display name, through the virtual getter; a missing object pushes the empty string.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetObjectName(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    unsigned long long oidObject;
    CSWMiniGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    pObject = g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0);
    if (pObject != 0)
        sName = pObject->GetName();
    if (!g_pVirtualMachine->StackPushString(sName))
        return -2000;
    return 0;
}

// The dynamic-type probes, behind one byte-indexed jump table. A trigger also counts as an
// obstacle, which is why those two ids share a tail; an unknown id answers 0.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_IsSomething(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0);
    int bResult = 0;
    if (pObject != 0) {
        switch (nCommand) {
        case CMD_SWMG_IsFollower:
            bResult = (pObject->AsFollower() != 0);
            break;
        case CMD_SWMG_IsEnemy:
            bResult = (pObject->AsEnemy() != 0);
            break;
        case CMD_SWMG_IsObstacle:
            bResult = (pObject->AsObstacle() != 0);
            // fall through -- a trigger counts as an obstacle too
        case CMD_SWMG_IsTrigger:
            if (pObject->AsEnemy() != 0
                && static_cast<CSWMiniEnemy*>(pObject->AsEnemy())->m_bTrigger != 0)
                bResult = 1;
            break;
        case CMD_SWMG_IsPlayer:
            bResult = (pObject->AsPlayer() != 0);
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return -2000;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastHPChange(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 4));
    int nHPChange = 0;
    if (pFollower)
        nHPChange = pFollower->m_hitPointAdjust;
    if (!g_pVirtualMachine->StackPushInteger(nHPChange))
        return -2000;
    return 0;
}

// Plays the named animation with the stop bit set, at unit speed.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_RemoveAnimation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sAnimation;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopString(&sAnimation))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0);
    if (pObject != 0)
        pObject->PlayAnimation(static_cast<char*>(sAnimation.CStr()), 1.0f, AnimRun::kAnimRemove);
    return 0;
}

// One CSWMiniGame::GetClipDist answers both clip getters, the command id choosing which out-param
// is passed. The result starts at 0, so a missing mini-game pushes 0.0.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetCameraClipping(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    float fClip = 0.0f;
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pMiniGame != 0) {
        if (nCommand == CMD_SWMG_GetCameraNearClip)
            pMiniGame->GetClipDist(&fClip, 0);
        else if (nCommand == CMD_SWMG_GetCameraFarClip)
            pMiniGame->GetClipDist(0, &fClip);
    }
    if (!g_pVirtualMachine->StackPushFloat(fClip))
        return -2000;
    return 0;
}

// The mini-game is fetched before the two pops, so a null one still consumes both arguments.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetCameraClipping(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    float fNear;
    float fFar;
    if (!g_pVirtualMachine->StackPopFloat(&fNear))
        return -2001;
    if (!g_pVirtualMachine->StackPopFloat(&fFar))
        return -2001;
    if (pMiniGame != 0)
        pMiniGame->SetClipDist(fNear, fFar);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayer(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    CSWTrackFollower* pPlayer = pMiniGame ? pMiniGame->m_pPlayer : 0;
    unsigned int nPlayer = MINIGAME_OBJECT_NONE;
    if (pPlayer)
        nPlayer = pPlayer->m_nObjectId;
    if (!g_pVirtualMachine->StackPushObject(nPlayer))
        return -2000;
    return 0;
}

// The enemy and obstacle counts share one body.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameCounts(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    int nCount = 0;
    if (pMiniGame) {
        if (nCommand == CMD_SWMG_GetEnemyCount)
            nCount = pMiniGame->m_enemyIds.m_count;
        else if (nCommand == CMD_SWMG_GetObstacleCount)
            nCount = pMiniGame->m_obstacleIds.m_count;
    }
    if (!g_pVirtualMachine->StackPushInteger(nCount))
        return -2000;
    return 0;
}

// One handler, two accessors, answering the object's own small id (an invalid id when anything
// fails).
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameObjects(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    int nIndex;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return -2001;
    unsigned long long oidObject = MINIGAME_OBJECT_NONE;
    if (pMiniGame != 0) {
        CSWMiniGameObject* pObject K2_UNSET;
        if (nCommand == CMD_SWMG_GetEnemy)
            pObject = static_cast<CSWMiniGameObject*>(pMiniGame->GetEnemy(nIndex));
        else if (nCommand == CMD_SWMG_GetObstacle)
            pObject = pMiniGame->GetObstacle(nIndex);
        if (pObject != 0)
            oidObject = pObject->m_nObjectId;
    }
    if (!g_pVirtualMachine->StackPushObject(oidObject))
        return -2000;
    return 0;
}

// Five int getters off any mini-game vehicle.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerIntInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    int nValue = 0;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        switch (nCommand) {
        case CMD_SWMG_GetHitPoints:
            nValue = static_cast<CSWTrackFollower*>(pObject)->m_hitPoints;
            break;
        case CMD_SWMG_GetMaxHitPoints:
            nValue = static_cast<CSWTrackFollower*>(pObject)->m_maxHitPoints;
            break;
        case CMD_SWMG_GetNumLoops:
            nValue = static_cast<CSWTrackFollower*>(pObject)->m_numLoops;
            break;
        case CMD_SWMG_GetGunBankCount:
            nValue = static_cast<CSWTrackFollower*>(pObject)->m_gunBanks.m_count;
            break;
        case CMD_SWMG_GetIsInvulnerable:
            nValue = (static_cast<CSWTrackFollower*>(pObject)->m_invincibilityTimer > 0.0f);
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return -2000;
    return 0;
}

// 618 sets the max hit points, 622 the speed looping through the virtual slot.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerIntInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidFollower;
    int nValue;
    if (!g_pVirtualMachine->StackPopObject(&oidFollower))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return -2001;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(oidFollower), 4));
    if (pFollower != 0) {
        if (nCommand == CMD_SWMG_SetMaxHitPoints)
            pFollower->SetMaxHitPoints(nValue);
        else if (nCommand == CMD_SWMG_SetNumLoops)
            pFollower->SetLooping(nValue);
    }
    return 0;
}

// The follower-keyed float getter pair; a missing follower pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidFollower;
    if (!g_pVirtualMachine->StackPopObject(&oidFollower))
        return -2001;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(oidFollower), 4));
    float fValue = 0.0f;
    if (pFollower != 0) {
        if (nCommand == CMD_SWMG_GetSphereRadius)
            fValue = pFollower->m_sphereRadius;
        else if (nCommand == CMD_SWMG_GetPlayerInvincibility)
            fValue = pFollower->m_invincibility;
    }
    if (!g_pVirtualMachine->StackPushFloat(fValue))
        return -2000;
    return 0;
}

// 620 sets the sphere radius, 648 the invincibility.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidFollower;
    float fValue;
    if (!g_pVirtualMachine->StackPopObject(&oidFollower))
        return -2001;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return -2001;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(oidFollower), 4));
    if (pFollower != 0) {
        if (nCommand == CMD_SWMG_SetSphereRadius)
            pFollower->SetSphereRadius(fValue);
        else if (nCommand == CMD_SWMG_SetPlayerInvincibility)
            pFollower->SetInvincibility(fValue);
    }
    return 0;
}

// The id is looked up as a follower first and then as an obstacle, whose position comes off its
// renderer Gob; neither hit means the zero vector.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerPosition(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    Vector vPosition;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
            ->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4));
    if (pFollower != 0) {
        vPosition = pFollower->GetPosition();
#if KOTOR1  // K1: followers only (no obstacle fallback)
    }
#else
    } else {
        CSWMGObstacle* pObstacle = static_cast<CSWMGObstacle*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                ->GetMiniGameObject(static_cast<unsigned int>(oidObject), 3));
        if (pObstacle != 0)
            vPosition = pObstacle->m_pGob.m_pObject->GetPosition();
    }
#endif
    if (!g_pVirtualMachine->StackPushVector(vPosition))
        return -2000;
    return 0;
}

// The two resref properties of a gun bank. The answer is built up front, so every miss pushes the
// empty string.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankStringInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sValue;
    unsigned long long oidObject;
    int nBank;
    CSWMiniGameObject* pObject;
    CSWMGGunBank* pGunBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    pObject = g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_GetGunBankBulletModel)
                pGunBank->m_resBullet.CopyToString(sValue);
            else if (nCommand == CMD_SWMG_GetGunBankGunModel)
                pGunBank->m_resGunModel.CopyToString(sValue);
        }
    }
    if (!g_pVirtualMachine->StackPushString(sValue))
        return -2000;
    return 0;
}

// 627 answers the bullet damage and 631 its target type, which goes through the bank's own
// IsTargetting() virtual.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankIntInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    int nValue = 0;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_GetGunBankDamage)
                nValue = pGunBank->m_bullet.m_nDamage;
            else if (nCommand == CMD_SWMG_GetGunBankTarget)
                nValue = pGunBank->m_bullet.m_nTargetType;
            else if (nCommand == CMD_SWMG_IsGunBankTargetting)
                nValue = pGunBank->IsTargetting();
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return -2000;
    return 0;
}

// Answers straight off the bullet description, with 628 the rate of fire, 629 the life span and 630
// the speed.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetGunBankFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    float fValue = 0.0f;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_GetGunBankTimeBetweenShots)
                fValue = pGunBank->m_bullet.m_fRateOfFire;
            else if (nCommand == CMD_SWMG_GetGunBankLifespan)
                fValue = pGunBank->m_bullet.m_fLifeSpan;
            else if (nCommand == CMD_SWMG_GetGunBankSpeed)
                fValue = pGunBank->m_bullet.m_fSpeed;
        }
    }
    if (!g_pVirtualMachine->StackPushFloat(fValue))
        return -2000;
    return 0;
}

// 632 sets the bullet model, 633 the gun model, which has to swap every live gun object over.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankStringInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sValue;
    unsigned long long oidObject;
    int nBank;
    CSWMiniGameObject* pObject;
    CSWMGGunBank* pGunBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    if (!g_pVirtualMachine->StackPopString(&sValue))
        return -2001;
    pObject = g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_SetGunBankBulletModel)
                pGunBank->m_resBullet = CResRef(sValue);
            else if (nCommand == CMD_SWMG_SetGunBankGunModel)
                pGunBank->SetGunModel(CResRef(sValue));
        }
    }
    return 0;
}

// 638 validates through SetBulletTargetType; 634 stores straight into the bullet description.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankIntInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nValue;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_SetGunBankDamage)
                pGunBank->m_bullet.m_nDamage = nValue;
            else if (nCommand == CMD_SWMG_SetGunBankTarget)
                pGunBank->SetBulletTargetType(nValue);
        }
    }
    return 0;
}

// All three arms go through a validating setter.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetGunBankFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    float fValue;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0) {
            if (nCommand == CMD_SWMG_SetGunBankTimeBetweenShots)
                pGunBank->SetRateOfFire(fValue);
            else if (nCommand == CMD_SWMG_SetGunBankLifespan)
                pGunBank->SetBulletLifeSpan(fValue);
            else if (nCommand == CMD_SWMG_SetGunBankSpeed)
                pGunBank->SetBulletSpeed(fValue);
        }
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastBulletHitPart(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sHitPart;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(m_oidSelf), 0);
    if (pObject != 0)
        sHitPart = pObject->m_sLastBulletHitPart;
    if (!g_pVirtualMachine->StackPushString(sHitPart))
        return -2000;
    return 0;
}

// Five vector getters on the player vehicle, with the far-out id 717 falling out of the jump table
// into its own compare.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerVectorInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWTrackFollower* pPlayer = g_pAppManager->m_pClientExoApp->GetMiniGame()->m_pPlayer;
    Vector vInfo(0.0f, 0.0f, 0.0f);
    if (pPlayer != 0) {
        switch (nCommand) {
        case CMD_SWMG_GetPlayerOffset:
            vInfo = static_cast<CSWMiniPlayer*>(pPlayer)->m_vOffset;
            break;
        case CMD_SWMG_GetPlayerTunnelPos:
            vInfo = static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelPos;
            break;
        case CMD_SWMG_GetPlayerTunnelNeg:
            vInfo = static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelNeg;
            break;
        case CMD_SWMG_GetPlayerOrigin:
            vInfo = static_cast<CSWMiniPlayer*>(pPlayer)->m_vOrigin;
            break;
        case CMD_SWMG_GetPlayerTunnelInfinite:
            vInfo = static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelInfinite;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushVector(vInfo))
        return -2000;
    return 0;
}

// Four floats off the player vehicle, the ids dense enough for a byte-indexed jump table.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetPlayerFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWTrackFollower* pPlayer = g_pAppManager->m_pClientExoApp->GetMiniGame()->m_pPlayer;
    float fValue = 0.0f;
    if (pPlayer != 0) {
        switch (nCommand) {
        case CMD_SWMG_GetPlayerSpeed:
            fValue = pPlayer->m_fSpeed;
            break;
        case CMD_SWMG_GetPlayerMinSpeed:
            fValue = static_cast<CSWMiniPlayer*>(pPlayer)->m_minSpeed;
            break;
        case CMD_SWMG_GetPlayerAccelerationPerSecond:
            fValue = static_cast<CSWMiniPlayer*>(pPlayer)->m_aps;
            break;
        case CMD_SWMG_GetPlayerMaxSpeed:
            fValue = static_cast<CSWMiniPlayer*>(pPlayer)->m_maxSpeed;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushFloat(fValue))
        return -2000;
    return 0;
}

// The setter half of the vector pair puts five setters on one handler, each id one above its
// getter, with 718 out of the jump table. Id 656 falls through, so setting the origin also clobbers
// the infinite-tunnel vector.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerVectorInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    Vector vInfo(0.0f, 0.0f, 0.0f);
    if (!g_pVirtualMachine->StackPopVector(&vInfo))
        return -2001;
    CSWTrackFollower* pPlayer = g_pAppManager->m_pClientExoApp->GetMiniGame()->m_pPlayer;
    if (pPlayer == 0)
        return 0;
    switch (nCommand) {
    case CMD_SWMG_SetPlayerOffset:
        static_cast<CSWMiniPlayer*>(pPlayer)->SetOffset(vInfo);
        break;
    case CMD_SWMG_SetPlayerTunnelPos:
        static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelPos = vInfo;
        break;
    case CMD_SWMG_SetPlayerTunnelNeg:
        static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelNeg = vInfo;
        break;
    case CMD_SWMG_SetPlayerOrigin:
        static_cast<CSWMiniPlayer*>(pPlayer)->SetOrigin(vInfo);
        // fall through: id 718's store runs for 656 as well
    case CMD_SWMG_SetPlayerTunnelInfinite:
        static_cast<CSWMiniPlayer*>(pPlayer)->m_vTunnelInfinite = vInfo;
        break;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetPlayerFloatInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    float fValue;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return -2001;
    CSWTrackFollower* pPlayer = g_pAppManager->m_pClientExoApp->GetMiniGame()->m_pPlayer;
    if (pPlayer != 0) {
        switch (nCommand) {
        case CMD_SWMG_SetPlayerSpeed:
            pPlayer->SetSpeed(fValue);
            break;
        case CMD_SWMG_SetPlayerMinSpeed:
            static_cast<CSWMiniPlayer*>(pPlayer)->SetMinSpeed(fValue);
            break;
        case CMD_SWMG_SetPlayerAccelerationPerSecond:
            static_cast<CSWMiniPlayer*>(pPlayer)->SetAPS(fValue);
            break;
        case CMD_SWMG_SetPlayerMaxSpeed:
            static_cast<CSWMiniPlayer*>(pPlayer)->SetMaxSpeed(fValue);
            break;
        }
    }
    return 0;
}

// Registered for commands 657..660 but the switch tests 661..664, the setters' ids, so no script id
// reaches an arm and all four getters answer 0.0.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo(int nCommand,
                                                                               int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    float fValue = 0.0f;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
        if (pGunBank != 0 && pGunBank->IsTargetting() != 0) {
            CSWMGTargettingGunBank* pTargetting = static_cast<CSWMGTargettingGunBank*>(pGunBank);
            switch (nCommand) {
            case CMD_SWMG_SetGunBankHorizontalSpread:
                fValue = pTargetting->m_targetting.m_fHorizontalSpread;
                break;
            case CMD_SWMG_SetGunBankVerticalSpread:
                fValue = pTargetting->m_targetting.m_fVerticalSpread;
                break;
            case CMD_SWMG_SetGunBankSensingRadius:
                fValue = pTargetting->m_targetting.m_fSensingRadius;
                break;
            case CMD_SWMG_SetGunBankInaccuracy:
                fValue = pTargetting->m_targetting.m_fInaccuracy;
                break;
            }
        }
    }
    if (!g_pVirtualMachine->StackPushFloat(fValue))
        return -2000;
    return 0;
}

// The write side of the getter above, whose switch base is the one the registration uses.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetTargettingGunBankFloatInfo(int nCommand,
                                                                               int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    float fValue;
    int nBank;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nBank))
        return -2001;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject == 0)
        return 0;
    CSWMGGunBank* pGunBank = static_cast<CSWTrackFollower*>(pObject)->GetGunBank(nBank);
    if (pGunBank == 0 || pGunBank->IsTargetting() == 0)
        return 0;
    CSWMGTargettingGunBank* pTargetting = static_cast<CSWMGTargettingGunBank*>(pGunBank);
    switch (nCommand) {
    case CMD_SWMG_SetGunBankHorizontalSpread:
        pTargetting->SetHorizontalSpread(fValue);
        break;
    case CMD_SWMG_SetGunBankVerticalSpread:
        pTargetting->SetVerticalSpread(fValue);
        break;
    case CMD_SWMG_SetGunBankSensingRadius:
        pTargetting->SetSensingRadius(fValue);
        break;
    case CMD_SWMG_SetGunBankInaccuracy:
        pTargetting->SetInaccuracy(fValue);
        break;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_StartInvulnerability(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidFollower;
    if (!g_pVirtualMachine->StackPopObject(&oidFollower))
        return -2001;
    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                                      ->GetMiniGameObject(static_cast<unsigned int>(oidFollower), 4));
    if (pFollower)
        pFollower->m_invincibilityTimer = pFollower->m_invincibility;
    return 0;
}

// The read side of the trio below, on the three ids one lower.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSoundProperty(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nSound;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nSound))
        return -2001;
    int nValue = 0;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        if (nCommand == CMD_SWMG_GetSoundFrequency)
            nValue = static_cast<CSWTrackFollower*>(pObject)->GetSoundFrequency(nSound);
        else if (nCommand == CMD_SWMG_GetSoundFrequencyIsRandom)
            nValue = static_cast<CSWTrackFollower*>(pObject)->GetSoundFrequencyIsRandom(nSound);
        else if (nCommand == CMD_SWMG_GetSoundVolume)
            nValue = static_cast<CSWTrackFollower*>(pObject)->GetSoundVolume(nSound);
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return -2000;
    return 0;
}

// The three sound setters share this handler; the follower lookup makes them CSWTrackFollower's.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetSoundProperty(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nSound;
    int nValue;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nSound))
        return -2001;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return -2001;
    CSWMiniGameObject* pObject =
        g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObject(static_cast<unsigned int>(oidObject), 4);
    if (pObject != 0) {
        switch (nCommand) {
        case CMD_SWMG_SetSoundFrequency:
            static_cast<CSWTrackFollower*>(pObject)->SetSoundFrequency(nSound, nValue);
            break;
        case CMD_SWMG_SetSoundFrequencyIsRandom:
            static_cast<CSWTrackFollower*>(pObject)->SetSoundFrequencyIsRandom(nSound, nValue);
            break;
        case CMD_SWMG_SetSoundVolume:
            static_cast<CSWTrackFollower*>(pObject)->SetSoundVolume(nSound, nValue);
            break;
        }
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLateralAccelPerSec(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fLateral = g_pAppManager->m_pClientExoApp->GetMiniGame()->m_lateralAccelPerSec;
    if (!g_pVirtualMachine->StackPushFloat(fLateral))
        return -2000;
    return 0;
}

// The swoop-track lateral acceleration, scaled up by the player's dexterity modifier. The player
// creature is dereferenced with no null check.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetLateralAccelPerSec(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1  // K1: the scripted value as is (no player-DEX scaling)
    float fAccel;
    if (!g_pVirtualMachine->StackPopFloat(&fAccel))
        return -2001;
#else
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());
    char nDexMod = static_cast<char>(pPlayer->m_pStats->GetDEXMod(0));
    float fAccel;
    if (!g_pVirtualMachine->StackPopFloat(&fAccel))
        return -2001;
    fAccel = fAccel + fAccel * nDexMod * 0.3f;
#endif
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    pMiniGame->m_lateralAccelPerSec = fAccel;
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetSwoopUpgrade(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nUpgrade;
    int nSlot;
    if (g_pVirtualMachine->StackPopInteger(&nSlot)) {
        CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        nUpgrade = pMiniGame->m_swoopUpgrades[nSlot];
    } else {
        nUpgrade = 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(nUpgrade))
        return -2000;
    return 0;
}
#endif

// A failed pop still pushes the zero vector, courtesy of Vector's zeroing default ctor.
int CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTrackPosition(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    Vector vPosition;
    if (g_pVirtualMachine->StackPopObject(&oidObject))
        vPosition = static_cast<CSWTrackFollower*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                ->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0)->AsFollower())->m_pTrack->GetPosition();
    if (!g_pVirtualMachine->StackPushVector(vPosition))
        return -2000;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetFollowerPosition(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    // Same silently-tolerated pop as SWMG_SetJumpSpeed: always reports success.
    Vector vPosition;
    if (g_pVirtualMachine->StackPopVector(&vPosition)) {
        CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        pMiniGame->SetCurPos(vPosition);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSWMG_DestroyMiniGameObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (g_pVirtualMachine->StackPopObject(&oidObject)) {
        CSWMGObstacle* pObstacle = static_cast<CSWMGObstacle*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
                ->GetMiniGameObject(static_cast<unsigned int>(oidObject), 0)->AsObstacle());
        if (pObstacle != 0)
            g_pAppManager->m_pClientExoApp->GetMiniGame()->RemoveObstacle(pObstacle);
    }
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSWMG_SetJumpSpeed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    // N.B. a failed pop is silently tolerated here -- the command always reports success.
    float fJumpSpeed;
    if (g_pVirtualMachine->StackPopFloat(&fJumpSpeed)) {
        CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        pMiniGame->m_jumpSpeed = fJumpSpeed;
    }
    return 0;
}
#endif

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSWMG_PlayerApplyForce(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vForce;
    if (g_pVirtualMachine->StackPopVector(&vForce)) {
        CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        pMiniGame->m_jumpSpeed = vForce.z;
        pMiniGame->m_curPhysicsState.m_vVelocity.x = vForce.x;
        pMiniGame->m_lateralAccelPerSec = vForce.x;
        pMiniGame->m_prevPhysicsState.m_vVelocity.x = vForce.x;
    }
    return 0;
}
#endif
