// The NWScript command handlers, one ExecuteCommand<Name>(int, int) member per script command,
// dispatched through the command table built by InitializeCommands.
#include "k2stdint.h"
#include "serverexoappinternal.h"
#include "appmanager.h"
#include "serveraimaster.h"
#include "swmgtrackfollower.h"
#include "swmgobject.h"
#include "swminigame.h"
#include "swmgguns.h"
#include "exolocstringinternal.h"
#include "aurorainterface.h"
#include "clientoptions.h"
#include "nwspell.h"
#include "nwmessagetypes.h"
#include "nwsrules.h"
#include "nwrace.h"
#include "nwmessage.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "nwsplayerluo.h"
#include "nwsspellimpact.h"
#include "swguiexamine.h"
#include "nwsarea.h"
#include "nwsparty.h"
#include "nwssoundobject.h"
#include "nwsstore.h"
#include "nwsambientsound.h"
#include "nwsplaceable.h"
#include "nwsitem.h"
#include "swsareamap.h"
#include "nwsdoor.h"
#include "Log_Base2.h"
#include "nwsobject.h"
#include "nwsencounter.h"
#include "nwsjournal.h"
#include "nwcanimbase.h"
#include "nwsexpression.h"
#include "nwstrigger.h"
#include "CExoArrayListImpl.h"
#include "exobase.h"
#include "nwnamegen.h"
#include "nwarea.h"
#include "exoinput.h"
#include "exoiniinternal.h"
#include "nwccreature.h"
#include "nwcitem.h"
#include "AURORA/aurmodel.h"
#include "port/crtapi.h"
#include "nwscriptconst.h"
#include "nwcprojectile.h"

#include <vector>
#include "serveranimation.h"
#include "nwsaiaction.h"
#include "nwseffect.h"
#include "nwsscriptevent.h"
#include "restype.h"
#include "enginevfx.h"
#include "strref.h"
#include "exostringinline.h"

// What a command handler returns when it fails; the virtual machine aborts the script on any
// negative result. A handler reports a failed stack push as -2000 and a failed pop as -2001, though
// a few commands answer a pop failure with -2000.
enum {
    VM_ERROR_PUSH_FAILED = -2000,
    VM_ERROR_POP_FAILED = -2001,
    VM_ERROR_NO_SUCH_COMMAND = -2002,
};

// The number of script commands, the length of the command table.
enum {
#if KOTOR1
    VM_NUM_COMMANDS = 772,
#else
    VM_NUM_COMMANDS = 877,
#endif
};

// The engine structure types a script value can carry, nwscript.nss's ENGINE_STRUCTURE_0..3.
enum {
    ENGINE_STRUCTURE_EFFECT = 0,
    ENGINE_STRUCTURE_EVENT = 1,
    ENGINE_STRUCTURE_LOCATION = 2,
    ENGINE_STRUCTURE_TALENT = 3,
};

#if KOTOR1
// KOTOR 2's script id for the dive roll, which KOTOR 1's nwscript.nss lacks.
enum {
    ANIMATION_FIREFORGET_DIVE_ROLL = 123,
};
#endif

#if KOTOR1
// The tutorial window rows KOTOR 1's script ids open, numbered as KOTOR 2's TUTORIAL_WINDOW_*.
enum {
    TUTORIAL_ROW_START_SWOOP_RACE = 9,
    TUTORIAL_ROW_RETURN_TO_BASE = 40,
    TUTORIAL_ROW_TEMP1 = 42,
};
#endif

// Reputation bounds: below 11 is hostile, above 89 friendly, and 11 to 89 neutral.
enum {
    REPUTATION_HOSTILE_BELOW = 11,
    REPUTATION_FRIENDLY_ABOVE = 89,
};

// NWN's Knock spell id, which the door and placeable knock actions still cast. Row 93 of KOTOR's
// spells.2da is the cryoban grenade item ability.
enum {
    SPELL_KNOCK = 93,
};

// CSWSObject::m_nCastClassIndex outside the class indexes: a cast with no class behind it, or a
// spell-like ability.
enum {
    CAST_CLASS_SPELL_LIKE_ABILITY = 0xfe,
    CAST_CLASS_NONE = 0xff,
};

// The combat-pause mode a message box pauses the game with.
enum {
    PAUSE_MODE_MESSAGE_BOX = 12,
};

// The item local boolean set once the item's text has been shown in the datapad box.
enum {
    LOCAL_BOOLEAN_DATAPAD_SHOWN = 94,
};

// The UnlockedPlanetSongs ini mask with all eleven planet songs set.
enum {
    UNLOCKED_PLANET_SONGS_ALL = 0x7ff,
};

// The skill id a skill increase or decrease uses for every skill at once.
enum {
    SKILL_ALL = 0xff,
};

// Script command ids: a command's index in nwscript.nss's function list, the number the
// compiled script passes to ExecuteCommand. The ids from 772 up exist only in KOTOR 2.
enum {
    CMD_SetFacing = 10,
    CMD_ActionMoveToLocation = 21,
    CMD_GetNearestCreature = 38,
    CMD_ActionCastSpellAtObject = 48,
    CMD_GetCurrentHitPoints = 49,
    CMD_EffectAssuredHit = 51,
    CMD_GetLastItemEquipped = 52,
    CMD_fabs = 67,
    CMD_cos = 68,
    CMD_sin = 69,
    CMD_tan = 70,
    CMD_acos = 71,
    CMD_asin = 72,
    CMD_atan = 73,
    CMD_log = 74,
    CMD_pow = 75,
    CMD_sqrt = 76,
    CMD_abs = 77,
    CMD_GetFirstEffect = 85,
    CMD_GetFirstObjectInArea = 93,
    CMD_d3 = 96,
    CMD_d4 = 97,
    CMD_d6 = 98,
    CMD_d8 = 99,
    CMD_d10 = 100,
    CMD_d12 = 101,
    CMD_d20 = 102,
    CMD_d100 = 103,
    CMD_ReflexSave = 109,
    CMD_WillSave = 110,
    CMD_MagicalEffect = 112,
    CMD_SupernaturalEffect = 113,
    CMD_ExtraordinaryEffect = 114,
    CMD_RoundsToSeconds = 121,
    CMD_HoursToSeconds = 122,
    CMD_TurnsToSeconds = 123,
    CMD_GetGoodEvilValue = 125,
    CMD_GetAlignmentGoodEvil = 127,
    CMD_GetFirstObjectInShape = 128,
    CMD_EffectEntangle = 130,
    CMD_EffectDeath = 133,
    CMD_EffectKnockdown = 134,
    CMD_ActionGiveItem = 135,
    CMD_SetFacingPoint = 143,
    CMD_AngleToVector = 144,
    CMD_VectorToAngle = 145,
    CMD_TouchAttackMelee = 146,
    CMD_TouchAttackRanged = 147,
    CMD_EffectParalyze = 148,
    CMD_EffectSleep = 154,
    CMD_EffectConfused = 157,
    CMD_EffectFrightened = 158,
    CMD_EffectStunned = 161,
    CMD_EffectRegenerate = 164,
    CMD_ChangeFaction = 173,
    CMD_ActionJumpToObject = 196,
    CMD_GetModuleFileName = 210,
    CMD_ActionJumpToLocation = 214,
    CMD_FeetToMeters = 218,
    CMD_YardsToMeters = 219,
    CMD_SpeakString = 221,
    CMD_GetPositionFromLocation = 223,
    CMD_GetFacingFromLocation = 225,
    CMD_GetNearestCreatureToLocation = 226,
    CMD_GetNearestObject = 227,
    CMD_GetNearestObjectToLocation = 228,
    CMD_GetNearestObjectByTag = 229,
    CMD_StringToInt = 232,
    CMD_ActionCastSpellAtLocation = 234,
    CMD_GetIsEnemy = 235,
    CMD_GetIsFriend = 236,
    CMD_GetIsNeutral = 237,
    CMD_GetLastSpellCaster = 245,
    CMD_GetLastSpell = 246,
    CMD_EffectAssuredDeflection = 252,
    CMD_GetLastPerceptionHeard = 257,
    CMD_GetLastPerceptionInaudible = 258,
    CMD_GetLastPerceptionSeen = 259,
    CMD_GetLastPerceptionVanished = 261,
    CMD_GetFirstInPersistentObject = 262,
    CMD_EffectHaste = 270,
    CMD_GetEncounterActive = 276,
    CMD_SetEncounterActive = 277,
    CMD_GetEncounterSpawnsMax = 278,
    CMD_SetEncounterSpawnsMax = 279,
    CMD_GetEncounterSpawnsCurrent = 280,
    CMD_SetEncounterSpawnsCurrent = 281,
    CMD_GetModuleItemAcquired = 282,
    CMD_GetModuleItemAcquiredFrom = 283,
    CMD_GetObjectSeen = 289,
    CMD_GetObjectHeard = 290,
    CMD_GetLastPlayerDied = 291,
    CMD_GetModuleItemLost = 292,
    CMD_GetModuleItemLostBy = 293,
    CMD_SetEncounterDifficulty = 296,
    CMD_GetEncounterDifficulty = 297,
    CMD_PlayAnimation = 300,
    CMD_TalentSpell = 301,
    CMD_GetCreatureTalentRandom = 307,
    CMD_GetCreatureTalentBest = 308,
    CMD_ActionUseTalentOnObject = 309,
    CMD_ActionUseTalentAtLocation = 310,
    CMD_GetIsDoorActionPossible = 337,
    CMD_DoDoorAction = 338,
    CMD_GetFirstItemInInventory = 339,
    CMD_GetClassByPosition = 341,
    CMD_GetLevelByPosition = 342,
    CMD_GetLevelByClass = 343,
    CMD_VersusAlignmentEffect = 355,
    CMD_VersusRacialTypeEffect = 356,
    CMD_GetGender = 358,
    CMD_GetTypeFromTalent = 362,
    CMD_GetIdFromTalent = 363,
    CMD_ActionSurrenderToEnemies = 379,
    CMD_GetFirstFactionMember = 380,
    CMD_GetNextFactionMember = 381,
    CMD_ActionForceMoveToLocation = 382,
    CMD_ActionForceMoveToObject = 383,
    CMD_PopUpGUIPanel = 388,
    CMD_EffectForceResisted = 402,
    CMD_GetLastPlayerDying = 410,
    CMD_GetStartingLocation = 411,
    CMD_ChangeToStandardFaction = 412,
    CMD_GetLastRespawnButtonPresser = 419,
    CMD_EffectForceFizzle = 420,
    CMD_GetLastSpellHarmful = 423,
    CMD_MusicBackgroundPlay = 425,
    CMD_MusicBackgroundStop = 426,
    CMD_MusicBackgroundSetDelay = 427,
    CMD_MusicBackgroundChangeDay = 428,
    CMD_MusicBackgroundChangeNight = 429,
    CMD_MusicBattlePlay = 430,
    CMD_MusicBattleStop = 431,
    CMD_MusicBattleChange = 432,
    CMD_AmbientSoundPlay = 433,
    CMD_AmbientSoundStop = 434,
    CMD_AmbientSoundChangeDay = 435,
    CMD_AmbientSoundChangeNight = 436,
    CMD_GetItemActivated = 439,
    CMD_GetItemActivator = 440,
    CMD_GetItemActivatedTargetLocation = 441,
    CMD_GetItemActivatedTarget = 442,
    CMD_EffectDispelMagicAll = 460,
    CMD_EffectDisguise = 463,
    CMD_EffectTimeStop = 467,
    CMD_EffectDispelMagicBest = 473,
    CMD_SurrenderToEnemies = 476,
    CMD_ActionUnlockObject = 483,
    CMD_ActionLockObject = 484,
    CMD_ActionCastFakeSpellAtObject = 501,
    CMD_ActionCastFakeSpellAtLocation = 502,
    CMD_FloatingTextStrRefOnCreature = 525,
    CMD_FloatingTextStringOnCreature = 526,
    CMD_GetTrapDisarmable = 527,
    CMD_GetTrapDetectable = 528,
    CMD_GetTrapDetectedBy = 529,
    CMD_GetTrapFlagged = 530,
    CMD_GetTrapBaseType = 531,
    CMD_GetTrapOneShot = 532,
    CMD_GetTrapCreator = 533,
    CMD_GetTrapKeyTag = 534,
    CMD_GetTrapDisarmDC = 535,
    CMD_GetTrapDetectDC = 536,
    CMD_GetLockKeyRequired = 537,
    CMD_GetLockKeyTag = 538,
    CMD_GetLockLockable = 539,
    CMD_GetLockUnlockDC = 540,
    CMD_GetLockLockDC = 541,
    CMD_GetFirstPC = 548,
    CMD_GetNextPC = 549,
    CMD_SetTrapDetectedBy = 550,
    CMD_PopUpDeathGUIPanel = 554,
    CMD_SetTrapDisabled = 555,
    CMD_GetModuleName = 561,
    CMD_AmbientSoundSetDayVolume = 567,
    CMD_AmbientSoundSetNightVolume = 568,
    CMD_GetGlobalBoolean = 578,
    CMD_SetGlobalBoolean = 579,
    CMD_GetGlobalNumber = 580,
    CMD_SetGlobalNumber = 581,
    CMD_GetLocalBoolean = 679,
    CMD_SetLocalBoolean = 680,
    CMD_GetLocalNumber = 681,
    CMD_SetLocalNumber = 682,
    CMD_IsAvailableCreature = 696,
    CMD_GetNPCSelectability = 709,
    CMD_GetFirstAttacker = 727,
    CMD_GetNextAttacker = 728,
    CMD_GetCategoryFromTalent = 735,
    CMD_SurrenderByFaction = 736,
    CMD_EffectCutSceneHorrified = 754,
    CMD_EffectCutSceneParalyze = 755,
    CMD_EffectCutSceneStunned = 756,
    CMD_SurrenderRetainBuffs = 762,
    CMD_GetFeatAcquired = 783,
    CMD_GetSpellAcquired = 784,
    CMD_GrantFeat = 786,
    CMD_GrantSpell = 787,
    CMD_GetInfluence = 795,
    CMD_SetInfluence = 796,
    CMD_ModifyInfluence = 797,
    CMD_IncrementGlobalNumber = 799,
    CMD_DecrementGlobalNumber = 800,
    CMD_EffectDroidConfused = 809,
    CMD_ModifyReflexSavingThrowBase = 828,
    CMD_ModifyFortitudeSavingThrowBase = 829,
    CMD_ModifyWillSavingThrowBase = 830,
    CMD_EffectMindTrick = 848,
    CMD_EffectDroidScramble = 852,
    CMD_GetSkillRankBase = 870,
};

#if !KOTOR1  // K1 deletes engine structures at once

struct GarbageCollectGameDefinedStructureEntry {
    int   m_nType;
    void* m_pStructure;

    GarbageCollectGameDefinedStructureEntry(int nType, void* pStructure)
        : m_nType(nType), m_pStructure(pStructure) {}
    void Collect();
};

std::vector<GarbageCollectGameDefinedStructureEntry> s_gameDefinedStructureGarbageCollectEntries;
#endif  // !KOTOR1


CSWVirtualMachineCommands::~CSWVirtualMachineCommands()
{
    if (m_paCommands)
    {
        delete[] m_paCommands;
        m_paCommands = 0;
    }
}

int CSWVirtualMachineCommands::ExecuteCommand(int nCommand, int nArg)
{
    if (nCommand >= VM_NUM_COMMANDS)
        return VM_ERROR_NO_SUCH_COMMAND;
    CommandFn fn = m_paCommands[nCommand];
    if (fn != 0)
        return (this->*fn)(nCommand, nArg);
    return VM_ERROR_NO_SUCH_COMMAND;
}

void CSWVirtualMachineCommands::RunScriptCallback(CExoString& sName)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (sName != "")
        pAIMaster->AppendToScriptsRun(sName);
}

void CSWVirtualMachineCommands::ReportError(CExoString& sName, int nArg)
{
    K2_UNUSED(sName); K2_UNUSED(nArg);
}

void* CSWVirtualMachineCommands::CopyGameDefinedStructure(int nType, void* pSource)
{
    switch (nType)
    {
    case ENGINE_STRUCTURE_EFFECT:
    {
        CGameEffect* pEffect = new CGameEffect(0);
        pEffect->CopyEffect(static_cast<CGameEffect*>(pSource), 0);
        return pEffect;
    }
    case ENGINE_STRUCTURE_EVENT:
    {
        CScriptEvent* pEvent = new CScriptEvent;
        pEvent->CopyScriptEvent(static_cast<CScriptEvent*>(pSource));
        return pEvent;
    }
    case ENGINE_STRUCTURE_LOCATION:
    {
        CScriptLocation* pLocation = new CScriptLocation;
        pLocation->CopyScriptLocation(static_cast<CScriptLocation*>(pSource));
        return pLocation;
    }
    case ENGINE_STRUCTURE_TALENT:
    {
        CScriptTalent* pTalent = new CScriptTalent;
        pTalent->CopyScriptTalent(static_cast<CScriptTalent*>(pSource));
        return pTalent;
    }
    }
    return 0;
}

int CSWVirtualMachineCommands::SaveGameDefinedStructure(int nType, void* pStructure, CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct structData;
    pGff->AddStructToStruct(&structData, pStruct, const_cast<char*>("GameDefinedStrct"), nType);
    switch (nType)
    {
    case ENGINE_STRUCTURE_EFFECT: static_cast<CGameEffect*>(pStructure)->SaveGameEffect(pGff, &structData); return 1;
    case ENGINE_STRUCTURE_EVENT: return static_cast<CScriptEvent*>(pStructure)->SaveEvent(pGff, &structData);
    case ENGINE_STRUCTURE_LOCATION: return static_cast<CScriptLocation*>(pStructure)->SaveLocation(pGff, &structData);
    case ENGINE_STRUCTURE_TALENT: return static_cast<CScriptTalent*>(pStructure)->SaveTalent(pGff, &structData);
    }
    return 0;
}

int CSWVirtualMachineCommands::LoadGameDefinedStructure(int nType, void** ppStructure, CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct structData;
    pGff->GetStructFromStruct(&structData, pStruct, const_cast<char*>("GameDefinedStrct"));
    switch (nType)
    {
    case ENGINE_STRUCTURE_EFFECT:
    {
        CGameEffect* pEffect = new CGameEffect(1);
        *ppStructure = pEffect;
        pEffect->LoadGameEffect(pGff, &structData);
        return 1;
    }
    case ENGINE_STRUCTURE_EVENT:
    {
        CScriptEvent* pEvent = new CScriptEvent;
        *ppStructure = pEvent;
        return pEvent->LoadEvent(pGff, &structData);
    }
    case ENGINE_STRUCTURE_LOCATION:
    {
        CScriptLocation* pLocation = new CScriptLocation;
        *ppStructure = pLocation;
        return pLocation->LoadLocation(pGff, &structData);
    }
    case ENGINE_STRUCTURE_TALENT:
    {
        CScriptTalent* pTalent = new CScriptTalent;
        *ppStructure = pTalent;
        return pTalent->LoadTalent(pGff, &structData);
    }
    }
    return 0;
}

int CSWVirtualMachineCommands::GetEqualGameDefinedStructure(int nType, void* pA, void* pB)
{
    switch (nType)
    {
    case ENGINE_STRUCTURE_EFFECT: return static_cast<CGameEffect*>(pA)->m_effectId == static_cast<CGameEffect*>(pB)->m_effectId;
    case ENGINE_STRUCTURE_EVENT: return *static_cast<CScriptEvent*>(pA) == *static_cast<CScriptEvent*>(pB);
    case ENGINE_STRUCTURE_LOCATION:
        return equal(static_cast<CScriptLocation*>(pA)->m_vPosition, static_cast<CScriptLocation*>(pB)->m_vPosition) &&
               equal(static_cast<CScriptLocation*>(pA)->m_vOrientation, static_cast<CScriptLocation*>(pB)->m_vOrientation);
    case ENGINE_STRUCTURE_TALENT: return static_cast<CScriptTalent*>(pA)->Equals(static_cast<CScriptTalent*>(pB));
    }
    return 0;
}

#if !KOTOR1  // K1 deletes at once
void GarbageCollectGameDefinedStructureAll()
{
    while (s_gameDefinedStructureGarbageCollectEntries.size() != 0)
    {
        s_gameDefinedStructureGarbageCollectEntries.back().Collect();
        s_gameDefinedStructureGarbageCollectEntries.pop_back();
    }
}

void GarbageCollectGameDefinedStructureEntry::Collect()
{
    if (m_nType == ENGINE_STRUCTURE_EFFECT) { if (m_pStructure) delete static_cast<CGameEffect*>(m_pStructure); }
    if (m_nType == ENGINE_STRUCTURE_EVENT) { if (m_pStructure) delete static_cast<CScriptEvent*>(m_pStructure); }
    if (m_nType == ENGINE_STRUCTURE_LOCATION) { if (m_pStructure) delete static_cast<CScriptLocation*>(m_pStructure); }
    if (m_nType == ENGINE_STRUCTURE_TALENT) { if (m_pStructure) delete static_cast<CScriptTalent*>(m_pStructure); }
}
#endif  // !KOTOR1

void CSWVirtualMachineCommands::DestroyGameDefinedStructure(int nType, void* pStructure)
{
#if KOTOR1  // K1 deletes immediately
    switch (nType) {
    case ENGINE_STRUCTURE_EFFECT:
        if (pStructure) delete static_cast<CGameEffect*>(pStructure);
        break;
    case ENGINE_STRUCTURE_EVENT:
        if (pStructure) delete static_cast<CScriptEvent*>(pStructure);
        break;
    case ENGINE_STRUCTURE_LOCATION:
        if (pStructure) delete static_cast<CScriptLocation*>(pStructure);
        break;
    case ENGINE_STRUCTURE_TALENT:
        if (pStructure) delete static_cast<CScriptTalent*>(pStructure);
        break;
    }
#else
    s_gameDefinedStructureGarbageCollectEntries.push_back(
        GarbageCollectGameDefinedStructureEntry(nType, pStructure));
#endif
}

void* CSWVirtualMachineCommands::CreateGameDefinedStructure(int nType)
{
    switch (nType)
    {
    case ENGINE_STRUCTURE_EFFECT: return new CGameEffect(0);
    case ENGINE_STRUCTURE_EVENT: return new CScriptEvent;
    case ENGINE_STRUCTURE_LOCATION: return new CScriptLocation;
    case ENGINE_STRUCTURE_TALENT: return new CScriptTalent;
    }
    return 0;
}


int bShipBuild;

int g_nRunScriptVar;

int CSWVirtualMachineCommands::ExecuteCommandExploreAreaForPlayer(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastAssociateCommand(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionNthNearestMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetDislike(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandDeleteMe(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetAssociateListenPatterns(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandActionEquipMostEffectiveArmor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature)
        pCreature->EquipMostEffectiveArmor();
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandCancelPostDialogCharacterSwitch(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_oidPostDialogCharacterSwitch = OBJECT_INVALID;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandHoldWorldFadeInForDialog(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 1;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIntToHexString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sHex;
    int nValue;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    sHex.Format(const_cast<char*>("0x%08x"), nValue);
    if (!g_pVirtualMachine->StackPushString(sHex))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIntToString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sNumber;
    int nValue;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    sNumber = CExoString(nValue);
    if (!g_pVirtualMachine->StackPushString(sNumber))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandInsertString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sInserted;
    CExoString sString;
    int nPos;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sInserted))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nPos))
        return VM_ERROR_POP_FAILED;
    sString.Insert(sInserted, nPos);
    if (!g_pVirtualMachine->StackPushString(sString))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetFadeUntilScript(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bFadeUntilScript = 1;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandResetDialogState(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandShowSwoopUpgradeScreen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSwoopUpgradeScreen(OBJECT_INVALID);
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandShowUpgradeScreen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
#if KOTOR1  // K1 takes only the item id
    K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowUpgradeScreen(oidItem);
    return 0;
#else
    unsigned long long oidItem;
    unsigned long long oidCreature = OBJECT_INVALID;
    int nA = 0;
    int nB = 0;
    CExoString sDesc;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
        if (nArgCount >= 3) {
            if (!g_pVirtualMachine->StackPopInteger(&nA))
                return VM_ERROR_POP_FAILED;
            if (!g_pVirtualMachine->StackPopInteger(&nB))
                return VM_ERROR_POP_FAILED;
            if (nArgCount >= 5) {
                if (!g_pVirtualMachine->StackPopString(&sDesc))
                    return VM_ERROR_POP_FAILED;
            }
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowUpgradeScreen(oidItem, oidCreature,
                                                                     nA, nB, sDesc);
    return 0;
#endif
}

int CSWVirtualMachineCommands::ExecuteCommandDoSinglePlayerAutoSave(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pServerExoApp->SetAutoSavePending(1);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandDoDoorAction(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    int nAction;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAction))
        return VM_ERROR_POP_FAILED;
    CGameObject* pGameObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (!pGameObject || !pGameObject->AsSWSObject() || !pCreature)
        return 0;
    CSWSObject* pSWSObject = pGameObject->AsSWSObject();
    switch (nAction) {
    case DOOR_ACTION_OPEN:
        if (nCommand == CMD_DoDoorAction)
            pCreature->AddActionToFront(AIACTION_OPEN_DOOR, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidObject,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        else
            pCreature->AddActionToFront(AIACTION_USE_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidObject,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        break;
    case DOOR_ACTION_UNLOCK:
        pCreature->AddActionToFront(nCommand == CMD_DoDoorAction ? AIACTION_OPEN_DOOR : AIACTION_USE_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidObject,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        pCreature->AddActionToFront(AIACTION_UNLOCK_OBJECT, ACTION_GROUP_PREVIOUS, ACTION_PARAM_OBJECT, &oidObject,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        break;
    case DOOR_ACTION_BASH:
        pCreature->AddAttackActions(oidObject, 0, 0, 0, 0, 1, 0, 1, SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0, 4, 0);
        break;
    case DOOR_ACTION_IGNORE:
        if (nCommand == CMD_DoDoorAction) {
            CPathfindInformation* pPath = pCreature->m_pPathfindInfo;
            pPath->m_oidIgnoreDoor = oidObject;
        }
        break;
    case DOOR_ACTION_KNOCK:
        // Casts spell 93, the cryoban grenade ability in KOTOR (SPELL_KNOCK is NWN's numbering). Left
        // as the original has it: the shipped scripts never request a knock.
        if (pSWSObject->m_bPlot)
            break;
        for (unsigned char nClass = 0; nClass < pCreature->m_pStats->m_numClasses; ++nClass) {
            if (pCreature->m_pStats->GetSpellUsesLeft(SPELL_KNOCK, nClass, 0, 0)) {
                pCreature->AddCastSpellActions(SPELL_KNOCK, nClass, 0, 0, 0, pSWSObject->m_position,
                    oidObject, 0, 0, 1, 0, 0, 0, -1, 0xff, 0);
                break;
            }
        }
        break;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandResistSpell(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    unsigned long long oidCaster;
    if (!g_pVirtualMachine->StackPopObject(&oidCaster))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    int nRes = 0;
    CGameObject* pCasterGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidCaster);
    CGameObject* pTargetGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pCasterGO && pTargetGO) {
        int nSpellId = pCasterGO->AsSWSObject()->m_castSpellId;
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell) {
            CGameObject* pSelfGO = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
            int nCasterLevel;
            if (pSelfGO && pSelfGO->AsSWSAreaOfEffectObject()) {
                nCasterLevel = pSelfGO->AsSWSAreaOfEffectObject()->m_nSpellLevel;
            } else if (pCasterGO->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
                CSWSCreature* pCasterCreature = pCasterGO->AsSWSCreature();
                if (pCasterCreature->m_bItemCastSpell == 1) {
                    nCasterLevel = pCasterCreature->m_nItemCastSpellLevel;
                } else if (pCasterCreature->m_nCastClassIndex >= CAST_CLASS_SPELL_LIKE_ABILITY) {
                    nCasterLevel = 2 * pSpell->m_nSpellLevelOther - 1;
                } else {
                    nCasterLevel = pCasterCreature->m_pStats->GetLevel(0);
                }
            } else {
                nCasterLevel = 2 * pSpell->m_nSpellLevelOther - 1;
            }
            const int nTargetType = pTargetGO->m_objectType;
            if (nTargetType == CGameObject::OBJECT_TYPE_CREATURE) {
                CSWSCreature* pTargetCreature = pTargetGO->AsSWSCreature();
                nRes = pTargetCreature->DoSpellLevelAbsorption(pCasterGO->AsSWSObject());
                int nResistance = 0;
#ifdef K2_FIX_ENGINE_BUGS
                int nRoll = 0;
#else
                int nRoll;   // unset on the absorption and immunity arms
#endif
                if (nRes == -1) {
                    nRes = pTargetCreature->DoSpellImmunity(pCasterGO->AsSWSObject());
                    if (nRes == -1) {
                        if (pSpell->m_nUserType == 1) {
                            const char nForceRes =
                                pTargetCreature->m_pStats->GetForceResistance();
                            if (nForceRes > 0) {
                                nRoll = g_pRules->RollDice(1, 20);
                                if (nCasterLevel + nRoll < nForceRes) {
                                    nRes = 1;
                                    nResistance = nForceRes;
                                } else {
                                    nRes = 0;
                                }
                            } else {
                                nRes = 0;
                            }
                        } else {
                            nRes = pSpell->m_nUserType == 2 ? 0 : -1;
                        }
                    }
                }
                if (nRes == 1) {
                    CSWCCMessageData* pMsg = new CSWCCMessageData;
                    pMsg->SetObjectID(0, oidTarget);
                    pMsg->SetInteger(0, 1);
                    CSWSPlayer* pPlayer =
                        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidCaster);
                    if (pPlayer)
                        g_pAppManager->m_pServerExoApp->GetSWSMessage()
                            ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_FORCE_RESIST, pMsg, 0);
                    pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidTarget);
                    if (pPlayer)
                        g_pAppManager->m_pServerExoApp->GetSWSMessage()
                            ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_FORCE_RESIST, pMsg, 0);
                    delete pMsg;
                    pMsg = new CSWCCMessageData;
                    pMsg->SetObjectID(0, oidTarget);
                    pMsg->SetObjectID(1, oidCaster);
                    pMsg->SetInteger(0, nRoll);
                    pMsg->SetInteger(1, nCasterLevel);
                    pMsg->SetInteger(2, nResistance);
                    pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidCaster);
                    if (pPlayer)
                        g_pAppManager->m_pServerExoApp->GetSWSMessage()
                            ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_FORCE_RESISTANCE_ROLL, pMsg, 0);
                    pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidTarget);
                    if (pPlayer)
                        g_pAppManager->m_pServerExoApp->GetSWSMessage()
                            ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_FORCE_RESISTANCE_ROLL, pMsg, 0);
                    delete pMsg;
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nRes))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandExportAllCharacters(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    g_pAppManager->m_pServerExoApp->SetExportAllCharactersPending(1);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandDisableVideoEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if !KOTOR1  // K1 has no video-effect override flag
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bScriptVideoEffect = 0;
#endif
    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsInCombat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidObject;
    int nValue = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 pops only the object
    K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature != 0)
        nValue = pCreature->m_bCombatState;
#else
    int nArg = 0;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nArg))
            return VM_ERROR_POP_FAILED;
    }
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature != 0) {
        nValue = pCreature->m_bCombatState;
        if (nValue != 0 && nArg != 0 && pCreature->m_nCombatType != 1)
            nValue = 0;
    }
#endif
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsXBox(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    if (!g_pVirtualMachine->StackPushInteger(0))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetTimeHour(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned int nHour = g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTimeHour();
    if (!g_pVirtualMachine->StackPushInteger(nHour))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetTimeMinute(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned int nMinute = g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTimeMinute();
    if (!g_pVirtualMachine->StackPushInteger(nMinute))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetTimeSecond(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned int nSecond = g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTimeSecond();
    if (!g_pVirtualMachine->StackPushInteger(nSecond))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetTimeMillisecond(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned int nMilli = g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTimeMillisecond();
    if (!g_pVirtualMachine->StackPushInteger(nMilli))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetButtonMashCheck(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nButtonMash = g_pAppManager->m_pServerExoApp->GetButtonMashCheck();
    if (!g_pVirtualMachine->StackPushInteger(nButtonMash))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLoadFromSaveGame(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nLoadFromSave = g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame();
    if (!g_pVirtualMachine->StackPushInteger(nLoadFromSave))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsMoviePlaying(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bMovie = g_pAppManager->m_pClientExoApp->IsMoviePlaying();
    if (!g_pVirtualMachine->StackPushInteger(bMovie))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandShowDemoScreen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sDemo;
    int nA;
    int nB;
    int nC;
    int nD;
    if (!g_pVirtualMachine->StackPopString(&sDemo))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nA))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nB))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nD))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDemoScreen(sDemo, nA, nB, nC, nD);
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandShowLevelUpGUI(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int bShown = pGui->ShowLevelUpGUI(1);
    if (!g_pVirtualMachine->StackPushInteger(bShown))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetRunScriptVar(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    if (!g_pVirtualMachine->StackPushInteger(g_nRunScriptVar))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandShipBuild(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    bShipBuild = 1;
    if (!g_pVirtualMachine->StackPushInteger(1))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsConversationActive(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    if (!g_pVirtualMachine->StackPushInteger(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsCreditSequenceInProgress(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1  // K1 reads the flag off CClientExoApp
    int bCredits = g_pAppManager->m_pClientExoApp->GetCreditSequenceInProgress();
#else
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int bCredits = pGui->m_creditSequenceInProgress;
#endif
    if (!g_pVirtualMachine->StackPushInteger(bCredits))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetGameDifficulty(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1  // K1 keeps the difficulty in the server info
    int nDifficulty = g_pAppManager->m_pServerExoApp->GetServerInfo()->m_difficultyLevel;
#else
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    int nDifficulty = pOptions->m_difficultyLevel;
#endif
    if (!g_pVirtualMachine->StackPushInteger(nDifficulty))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPartyAIStyle(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nAIState = pPartyTable->m_aiState;
    if (!g_pVirtualMachine->StackPushInteger(nAIState))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapSelectedPlanet(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nPlanet = pPartyTable->m_selectedPlanet;
    if (!g_pVirtualMachine->StackPushInteger(nPlanet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapPlanetAvailable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPlanet;
    if (!g_pVirtualMachine->StackPopInteger(&nPlanet))
        return VM_ERROR_POP_FAILED;
    int nAvailable = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPlanetAvailable(nPlanet);
    if (!g_pVirtualMachine->StackPushInteger(nAvailable))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapPlanetSelectable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPlanet;
    if (!g_pVirtualMachine->StackPopInteger(&nPlanet))
        return VM_ERROR_POP_FAILED;
    int nSelectable = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPlanetSelectable(nPlanet);
    if (!g_pVirtualMachine->StackPushInteger(nSelectable))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetReputation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidArg;
    unsigned long long oidSelf;
    CGameObject* pObject;
    int nReputation;
    if (!g_pVirtualMachine->StackPopObject(&oidArg))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidSelf))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray->GetGameObject(oidSelf, &pObject) == CGameObjectArray::SUCCESS)
        nReputation = pObject->AsSWSObject()->GetReputation(oidArg, 0);
    else
        nReputation = -1;
    if (!g_pVirtualMachine->StackPushInteger(nReputation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetCombatActionPending(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int nPending = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_actionList.m_pInternal->m_pHead != 0)
            nPending = pCreature->m_actionList.m_pInternal->m_pHead->m_pData != 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(nPending))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetSkillRank(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nSkill;
    unsigned long long oidObject;
    CGameObject* pObject;
    int nRank;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray->GetGameObject(oidObject, &pObject) == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        nRank = pObject->AsSWSCreature()->m_pStats->GetSkillRank(static_cast<unsigned char>(nSkill), static_cast<CSWSObject*>(0),
                                                                 nCommand == CMD_GetSkillRankBase);
    } else {
        nRank = -1;
    }
    if (!g_pVirtualMachine->StackPushInteger(nRank))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetSpellFormMask(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSpell;
    if (!g_pVirtualMachine->StackPopInteger(&nSpell))
        return VM_ERROR_POP_FAILED;
    CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
    int nMask = pSpell ? static_cast<int>(pSpell->m_nSpellFormMask) : 0;
    if (!g_pVirtualMachine->StackPushInteger(nMask))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandGetSpellBaseForcePointCost(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSpell;
    if (!g_pVirtualMachine->StackPopInteger(&nSpell))
        return VM_ERROR_POP_FAILED;
    CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
    int nCost = pSpell ? pSpell->m_nBaseForcePoints : 0;
    if (!g_pVirtualMachine->StackPushInteger(nCost))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetSoloMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nSolo = pPartyTable->m_soloMode;
    if (!g_pVirtualMachine->StackPushInteger(nSolo))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandGetLastForfeitViolation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nViolation = 0;
    if (pPartyTable)
        nViolation = pPartyTable->m_forfeitViolations;
    if (!g_pVirtualMachine->StackPushInteger(nViolation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

int CSWVirtualMachineCommands::ExecuteCommandGetPartyMemberCount(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    int nCount = pParty->m_nCharacterCount;
    if (!g_pVirtualMachine->StackPushInteger(nCount))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsNight(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    int bResult = 0;
    if (pModule)
        bResult = pModule->m_nTimeOfDay == 2;
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetObjectValid(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (!g_pVirtualMachine->StackPushInteger(pObject != 0))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetAttackTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    unsigned long long oidAttackTarget =
        pCreature ? pCreature->m_oidAttackTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidAttackTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetBlockingCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    // Dereferences the path info unchecked.
    unsigned long long oidBlocking =
        pCreature ? pCreature->m_pPathfindInfo->m_oidPathTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidBlocking))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetBaseItemType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    int nBaseItem = pItem ? pItem->m_baseItem : BASE_ITEM_INVALID;
    if (!g_pVirtualMachine->StackPushInteger(nBaseItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetItemComponent(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    int nComponent;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidItem);
    if (pCreature) {
        nComponent = pCreature->GetComponent();
    } else {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidItem);
        CSWSItem* pItemComponent = 0;
        if (pPlaceable && pPlaceable->m_pItemRepository) {
#ifdef K2_FIX_ENGINE_BUGS
            unsigned long long oidComponent =
                pPlaceable->m_pItemRepository->FindItemWithBaseItemType(ITEM_TYPE_COMPONENTS, 0);
#else
            unsigned long long oidComponent =
                pPlaceable->m_pItemRepository->FindItemWithBaseItemType(ITEM_TYPE_CREDITS, 0);
#endif
            pItemComponent = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidComponent);
        }
        nComponent = pItemComponent ? pItemComponent->m_stackCount : 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(nComponent))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandGetItemComponentPieceValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidItem;
    int nValue = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidItem, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSItem() != 0)
            nValue = pObject->AsSWSItem()->GetCost() / 20;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetIsPuppet(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nIsPuppet = pCreature ? pCreature->m_isPuppet : 0;
    if (!g_pVirtualMachine->StackPushInteger(nIsPuppet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandGetAreaUnescapable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    // The declared return type is the flat CSWArea base.
    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
    int nUnescapable = pArea->m_nUnescapable;
    if (!g_pVirtualMachine->StackPushInteger(nUnescapable))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMaxStealthXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nMaxStealthXP = 0;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            nMaxStealthXP = pArea->m_maxStealthXP;
    }
    if (!g_pVirtualMachine->StackPushInteger(nMaxStealthXP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetAttemptedAttackTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    unsigned long long oidTarget =
        pCreature ? pCreature->m_attemptedAttackTargetId : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetAttemptedSpellTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    unsigned long long oidTarget = pCreature ? pCreature->m_oidAttemptedSpellTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetAttemptedMovementTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    unsigned long long oidTarget = pCreature ? pCreature->m_oidAttemptedMovementTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetBlockingDoor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    unsigned long long oidDoor = pCreature ? pCreature->m_oidBlockingObject : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidDoor))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsDawn(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    int bResult = 0;
    if (pModule)
        bResult = pModule->m_nTimeOfDay == 3;
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsDay(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    int bResult = 0;
    if (pModule)
        bResult = pModule->m_nTimeOfDay == 1;
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsDusk(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    int bResult = 0;
    if (pModule)
        bResult = pModule->m_nTimeOfDay == 4;
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMetaMagicFeat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    int nMetaMagic = CSWSObject::CAST_METAMAGIC_NONE;
#if KOTOR1  // K1 has no null test here
    pObject->AsSWSObject();
    {
#else
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
#endif
        CSWSObject* pSwsObject = pObject->AsSWSObject();
        if (pSwsObject != 0)
            nMetaMagic = pSwsObject->m_nCastMetaMagic;
    }
    if (!g_pVirtualMachine->StackPushInteger(nMetaMagic))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetModule(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = 0;
    if (g_pAppManager && g_pAppManager->m_pServerExoApp)
        pModule = g_pAppManager->m_pServerExoApp->GetModule();
    unsigned long long oidModule = pModule ? pModule->m_objectId : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidModule))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPCLevellingUp(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    unsigned long long oidPC = pModule ? pModule->m_oidLevelUpPlayer : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidPC))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetSubScreenID(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int nSubScreen = 0;
    if (pGui && pGui->m_bInGameMenuShown)
        nSubScreen = pGui->m_lastGuiPanel + 1;
    if (!g_pVirtualMachine->StackPushInteger(nSubScreen))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetTag(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sTag;
    CGameObject* pObject;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE)
            sTag = pObject->AsSWSModule()->m_sTag;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
            sTag = pObject->AsSWSArea()->m_sDebugName;
        else
            sTag = static_cast<CSWSObject*>(pObject)->m_sTag;
    }
    if (!g_pVirtualMachine->StackPushString(sTag))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsListening(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int bListening = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        bListening = static_cast<CSWSObject*>(pObject)->m_bListening;
    if (!g_pVirtualMachine->StackPushInteger(bListening))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandRevealMap(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vPos;
    int nFlag;
    if (!g_pVirtualMachine->StackPopVector(&vPos))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nFlag))
        return VM_ERROR_POP_FAILED;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule && pModule->m_pAreaMap) {
        CSWSAreaMap* pMap = pModule->m_pAreaMap;
        if (nFlag == -1)
            pMap->SetEntireMapExplored();
        else
            pMap->SetWorldPointExplored(vPos, nFlag);
    }
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSaveNPCByObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nIndex = -1;
    unsigned long long oidMember = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidMember))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SaveMemberByObject(nIndex, oidMember);
    return 0;
}
#endif

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSavePUPByObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nIndex = -1;
    unsigned long long oidMember = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidMember))
        return VM_ERROR_POP_FAILED;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    pPartyTable->SavePuppetByObject(nIndex, oidMember);
    return 0;
}
#endif

int CSWVirtualMachineCommands::ExecuteCommandGetIsDebilitated(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bDebilitated = 0;
    if (pCreature != 0)
        bDebilitated = pCreature->IsDebilitated(0);
    if (!g_pVirtualMachine->StackPushInteger(bDebilitated))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPlaceableIllumination(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidPlaceable;
    if (!g_pVirtualMachine->StackPopObject(&oidPlaceable))
        return VM_ERROR_POP_FAILED;
    CSWSPlaceable* pPlaceable = g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidPlaceable);
    int nLightIsOn = 0;
    if (pPlaceable != 0)
        nLightIsOn = pPlaceable->GetLightIsOn();
    if (!g_pVirtualMachine->StackPushInteger(nLightIsOn))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMaxForcePoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nMaxFP = 0;
    if (pCreature != 0)
        nMaxFP = pCreature->GetMaxForcePoints();
    if (!g_pVirtualMachine->StackPushInteger(nMaxFP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// 'Movment' is the upstream spelling of this name.
int CSWVirtualMachineCommands::ExecuteCommandGetCreatureMovmentType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nMovementRate = 1;
    if (pCreature != 0)
        nMovementRate = pCreature->m_pStats->m_nMovementRate;
    if (!g_pVirtualMachine->StackPushInteger(nMovementRate))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMatchedStringsCount(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nCount = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        nCount = static_cast<CSWSObject*>(pObject)->m_matchedExpressionStrings.m_count;
    if (!g_pVirtualMachine->StackPushInteger(nCount))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMatchedStringsNum(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nNum = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        nNum = static_cast<CSWSObject*>(pObject)->m_nListenPatternNumber;
    if (!g_pVirtualMachine->StackPushInteger(nNum))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsEncounterCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bEncounter = 0;
    if (pCreature != 0)
        bEncounter = pCreature->m_oidEncounter != OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushInteger(bEncounter))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPartyMemberByIndex(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nIndex;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    CSWCCreature* pCharacter = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nIndex);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCharacter != 0)
        oidMember = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pCharacter->m_objectId);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetMinOneHP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 leaves this unset for an object that is not a CSWSObject
    int nMinOneHP;
#else
    int nMinOneHP = 0;
#endif
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        nMinOneHP = pObject->AsSWSObject()->m_bMin1HP;
    if (!g_pVirtualMachine->StackPushInteger(nMinOneHP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPlotFlag(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int bPlot = 0;
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        bPlot = pObject->AsSWSObject()->m_bPlot != 0;
    if (!g_pVirtualMachine->StackPushInteger(bPlot))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetRacialType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int nRace = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
            nRace = pObject->AsSWSCreature()->m_pStats->m_nRace;
    }
    if (!g_pVirtualMachine->StackPushInteger(nRace))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetRacialSubType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int nSubType;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
            nSubType = pObject->AsSWSCreature()->m_pStats->m_nSubraceIndex;
        else
            nSubType = -1;
    } else {
        nSubType = 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(nSubType))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPosition(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    Vector vPosition;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA)
        vPosition = static_cast<CSWSObject*>(pObject)->m_position;
    if (!g_pVirtualMachine->StackPushVector(vPosition))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsAIState(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 1;
    if (pObject != 0) {
        if (pObject->AsSWSCreature() != 0) {
            if (pObject->AsSWSCreature()->GetDead() == 0) {
#if KOTOR1  // K1 uses GetIsPCDying() and GetDeadTemp()
                if (pObject->AsSWSCreature()->GetIsPCDying() == 0)
                    nValue = pObject->AsSWSCreature()->GetDeadTemp() != 0;
#else
                // The discarded cast is deliberate, since the test below reads the local.
                pObject->AsSWSCreature();
                CSWSCreature* pC = pObject->AsSWSCreature();
                nValue = pC->GetDeadTemp();
#endif
            }
        } else
            nValue = static_cast<CSWSObject*>(pObject)->GetDead();
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsEffectValid(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    int nValue;
    if (pEffect != 0
            && pEffect->m_subType != CSWSEffectListHandler::EFFECT_INVALIDEFFECT
            && pEffect->m_bIsExposed != 0)
        nValue = 1;
    else
        nValue = 0;
    // Runs for a null structure too.
#ifdef K2_FIX_ENGINE_BUGS
    unsigned short nDuration = pEffect != 0 ? pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_MASK : 0xffff;
#else
    unsigned short nDuration = pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_MASK;
#endif
    if (nDuration == CGameEffect::DURATION_TYPE_INSTANTANEOUS) {
    } else if (nDuration == CGameEffect::DURATION_TYPE_TEMPORARY) {
    } else if (nDuration == CGameEffect::DURATION_TYPE_PERMANENT) {
    } else if (nDuration == CGameEffect::DURATION_TYPE_EQUIPPED) {
    } else if (nDuration == CGameEffect::DURATION_TYPE_INNATE) {
    } else
        nValue = 0;
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Restricts an effect to a subset of targets. Command 355 stamps the alignment pair and
// command 356 the racial type into an integer slot that differs per effect family.
// A 1-argument call never pops the alignment value, which is still read below.
int CSWVirtualMachineCommands::ExecuteCommandVersusEffect(int nCommand, int nArgCount)
{
    CGameEffect* pEffect;
#ifdef K2_FIX_ENGINE_BUGS
    int nLawChaos = 0;
    int nGoodEvil = 0;
#else
    int nLawChaos;
    int nGoodEvil;
#endif
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_PUSH_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nLawChaos))
            return VM_ERROR_PUSH_FAILED;
        if (nArgCount >= 3 && !g_pVirtualMachine->StackPopInteger(&nGoodEvil))
            return VM_ERROR_PUSH_FAILED;
    }
    int bValidRange = (nCommand != CMD_VersusAlignmentEffect || (nLawChaos >= 0 && nLawChaos <= 3));
    if (nCommand == CMD_VersusRacialTypeEffect && (nLawChaos < 0 || nLawChaos > CSWRace::CRACE_ALL_RACES)) {
    } else if (bValidRange && pEffect != 0) {
        unsigned short nSubType = pEffect->m_subType;
        if (nSubType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE
                || nSubType == CSWSEffectListHandler::EFFECT_ATTACK_DECREASE
                || nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE
                || nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(3, nLawChaos);
                pEffect->SetInteger(4, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(2, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_SANCTUARY
                || nSubType == CSWSEffectListHandler::EFFECT_INVISIBILITY) {
            if (nCommand == CMD_VersusAlignmentEffect)
                pEffect->SetInteger(3, nGoodEvil);
            else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(1, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_CONCEALMENT) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(2, nLawChaos);
                pEffect->SetInteger(3, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(1, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_IMMUNITY) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(2, nLawChaos);
                pEffect->SetInteger(3, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(1, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_AC_INCREASE
                || nSubType == CSWSEffectListHandler::EFFECT_AC_DECREASE) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(3, nLawChaos);
                pEffect->SetInteger(4, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(2, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_DECREASE) {
        } else if (nSubType == CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE
                || nSubType == CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(4, nLawChaos);
                pEffect->SetInteger(5, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(3, nLawChaos);
        } else if (nSubType == CSWSEffectListHandler::EFFECT_SKILL_INCREASE
                || nSubType == CSWSEffectListHandler::EFFECT_SKILL_DECREASE) {
            if (nCommand == CMD_VersusAlignmentEffect) {
                pEffect->SetInteger(3, nLawChaos);
                pEffect->SetInteger(4, nGoodEvil);
            } else if (nCommand == CMD_VersusRacialTypeEffect)
                pEffect->SetInteger(2, nLawChaos);
        }
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPCSpeaker(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1  // K1 reads the player client's game object and does not null-test it
    unsigned long long oidPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    unsigned long long oidSpeaker =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidPlayer)->m_gameObjectId;
#else
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    unsigned long long oidSpeaker = OBJECT_INVALID;
    CSWCCreature* pPC = pParty->GetCharacter(0);
    if (pPC)
        oidSpeaker = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pPC->m_objectId);
#endif
    if (!g_pVirtualMachine->StackPushObject(oidSpeaker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIntToFloat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nValue;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    float fValue = static_cast<float>(nValue);
    if (!g_pVirtualMachine->StackPushFloat(fValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetScriptParameter(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nIndex;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    int nParam = 0;
    if (nIndex >= 1 && nIndex <= 5)
        nParam = g_pVirtualMachine->m_nScriptParam[nIndex - 1];
    if (!g_pVirtualMachine->StackPushInteger(nParam))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// The three float pops write straight into the vector components.
int CSWVirtualMachineCommands::ExecuteCommandSetAreaFogColor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vColor;
    unsigned long long oidArea;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&vColor.x))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&vColor.y))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&vColor.z))
        return VM_ERROR_POP_FAILED;
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea) {
        pArea->SetFogColor(vColor);
        CSWCArea* pCArea = pArea->GetClientArea();
        if (pCArea)
            pCArea->SetFogColor(vColor);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetAreaTransitionBMP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sBMP;
    int nTransition;
    CSWSPlayer* pPlayer;
    if (!g_pVirtualMachine->StackPopInteger(&nTransition))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sBMP))
        return VM_ERROR_POP_FAILED;
    pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_oidSelf);
    if (pPlayer == 0)
        return 0;
    pPlayer->SetAreaTransitionBMP(nTransition, sBMP);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetAreaUnescapable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nUnescapable;
    if (!g_pVirtualMachine->StackPopInteger(&nUnescapable))
        return VM_ERROR_POP_FAILED;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pModule->GetArea());
        if (pArea)
            pArea->m_nUnescapable = nUnescapable;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandShowGalaxyMapGui(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPlanet;
    if (!g_pVirtualMachine->StackPopInteger(&nPlanet))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowGalaxyMapGui(nPlanet);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetSpellCasterItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidCasterItem = OBJECT_INVALID;
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
        oidCasterItem = pObject->AsSWSCreature()->m_lastSpellItemId;
    if (!g_pVirtualMachine->StackPushObject(oidCasterItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSaveNPCState(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNpc;
    if (!g_pVirtualMachine->StackPopInteger(&nNpc))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SaveMember(nNpc, 0);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandShowTutorialWindow(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nWindow;
    if (!g_pVirtualMachine->StackPopInteger(&nWindow))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1: script windows 0/1/2 are tutorial entries 9/40/42; anything else shows nothing
    if (nWindow == TUTORIAL_WINDOW_START_SWOOP_RACE)
        nWindow = TUTORIAL_ROW_START_SWOOP_RACE;
    else if (nWindow == TUTORIAL_WINDOW_RETURN_TO_BASE)
        nWindow = TUTORIAL_ROW_RETURN_TO_BASE;
    else if (nWindow == 2)
        nWindow = TUTORIAL_ROW_TEMP1;
    else
        return 0;
#endif
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
        static_cast<unsigned char>(nWindow), OBJECT_INVALID, OBJECT_INVALID, 0);
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandShowChemicalUpgradeScreen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidActivator = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidActivator))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowChemicalUpgradeScreen(oidActivator);
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandDisplayFeedBackText(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    oidObject = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidObject);
    int nStrRef;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(oidObject);
    int nEntry = 0;
    if (pGameObject == 0 || pGameObject->AsSWCObject() == 0)
        return 0;
    g_pRules->m_p2DArrays->m_pFeedBackText->GetINTEntry(nStrRef, CExoString("StrRef"), &nEntry);
    if (nEntry != 0) {
        CExoString sText = g_pTlkTable->GetSimpleString(nEntry);
        pGameObject->AsSWCObject()->SetFeedbackInfo(sText);
    } else {
        pGameObject->AsSWCObject()->SetFeedbackInfo(CExoString("BAD STRREF"));
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandDisplayMessageBox(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nStrRef = -1;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    CExoString sIcon;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopString(&sIcon))
            return VM_ERROR_POP_FAILED;
    }
    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    if (pBox != 0) {
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        CResRef resIcon(sIcon);
        pBox->SetIcon(resIcon);
        pBox->SetMessage(nStrRef);
#if !KOTOR1  // K1 has no DisplayMessageBox command
        pBox->m_bPausedCombat = 1;
#endif
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, PAUSE_MODE_MESSAGE_BOX, 0);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 0);
    }
    return 0;
}


// Sets the datapad entry's read bit, then shows its identified description in the shared
// popup box and pauses the game while it is up.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandDisplayDatapad(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidObject));
    if (pItem == 0)
        return 0;
    CSWGuiMessageBox* pBox =
        static_cast<CSWGuiMessageBox*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pDatapadMessageBox);
    pItem->m_varTable.SetLocalBoolean(LOCAL_BOOLEAN_DATAPAD_SHOWN, 1);
    CExoString sText;
    pItem->m_descriptionIdentified.GetString(0, &sText, 0);
    pBox->SetAllowCancel(0);
    CSWGuiTextParams& params = pBox->m_lblMessage.m_text.m_params;
    params.SetAlignment(9);
    params.SetFont(CResRef("fnt_d16x16"));
    pBox->SetMessage(sText);
    pBox->SetCallback(0, 0, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
    g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, PAUSE_MODE_MESSAGE_BOX, 0);
    return 0;
}
#endif // !KOTOR1

// Handles the three emotion checks, comparing the first-popped object's reputation against
// OBJECT_SELF, or against a second popped object.
int CSWVirtualMachineCommands::ExecuteCommandGetEmotions(int nCommand, int nArgCount)
{
    unsigned long long oidAgainst = OBJECT_INVALID;
    unsigned long long oidObject = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1)
        oidAgainst = m_oidSelf;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopObject(&oidAgainst))
        return VM_ERROR_POP_FAILED;
    int nRet = 0;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0) {
            if (nCommand == CMD_GetIsEnemy) {
                if (pObject->AsSWSPlaceable() == 0
                        || pObject->AsSWSPlaceable()->m_hostileState != 0)
                    nRet = pSWSObject->GetReputation(oidAgainst, 0) < REPUTATION_HOSTILE_BELOW;
            } else if (nCommand == CMD_GetIsFriend)
                nRet = pSWSObject->GetReputation(oidAgainst, 0) > REPUTATION_FRIENDLY_ABOVE;
            else if (nCommand == CMD_GetIsNeutral)
                nRet = static_cast<unsigned>(pSWSObject->GetReputation(oidAgainst, 0) - REPUTATION_HOSTILE_BELOW)
                       < REPUTATION_FRIENDLY_ABOVE - REPUTATION_HOSTILE_BELOW + 1;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nRet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetSpellForcePointCost(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nCost = 0;
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                == CGameObjectArray::SUCCESS) {
            CSWSObject* pSObject = pObject->AsSWSObject();
            if (pSObject != 0
                && (pSObject->AsSWSCreature() != 0 || pSObject->AsSWSPlaceable() != 0))
                nCost = pSObject->m_nLastSpellForcePointCost;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nCost))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandGrantAbility(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nValue;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature == 0)
        return 0;
    if (nCommand == CMD_GrantSpell)
        pCreature->m_pStats->AddKnownSpell(
            static_cast<unsigned char>(pCreature->m_pStats->m_numClasses - 1), nValue);
    else if (nCommand == CMD_GrantFeat)
        pCreature->m_pStats->AddFeat(static_cast<unsigned short>(nValue));
    return 0;
}

// Adjusts damage by a reflex save. On a passed save the damage is halved, or zeroed by Improved
// Evasion, and a flash effect is applied to the source.
int CSWVirtualMachineCommands::ExecuteCommandGetReflexAdjustedDamage(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature;
    unsigned long long oidSource;
    int nDamage;
    int nSaveVersus;
#if KOTOR1  // K1 leaves the DC unset for the 4-argument form
#ifdef K2_FIX_ENGINE_BUGS
    int nDC = 0;
#else
    int nDC;
#endif
    if (nArgCount == 4) {
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nDamage))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nSaveVersus))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidSource))
            return VM_ERROR_POP_FAILED;
    } else {
        if (!g_pVirtualMachine->StackPopInteger(&nDamage))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nDC))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nSaveVersus))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidSource))
            return VM_ERROR_POP_FAILED;
    }
#else
    int nDC = 0;
    if (nArgCount == 4) {
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nDamage))
            return VM_ERROR_POP_FAILED;
    } else {
        if (!g_pVirtualMachine->StackPopInteger(&nDamage))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nDC))
            return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopInteger(&nSaveVersus))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidSource))
        return VM_ERROR_POP_FAILED;
#endif
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0
            && pCreature->SavingThrowRoll(SAVING_THROW_REFLEX, static_cast<unsigned short>(nDC), static_cast<unsigned char>(nSaveVersus), oidSource, 1, 0, 0) != 0) {
#if KOTOR1  // K1 always halves the damage (no Evasion check)
        nDamage = nDamage / 2;
#else
        if (pCreature->m_pStats->HasFeat(FEAT_EVASION))
            nDamage = 0;
        else
            nDamage = nDamage / 2;
#endif
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS
            | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pEffect->SetCreator(oidSource);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
        pEffect->SetInteger(0, VFX_REFLEX_SAVE);
        pCreature->ApplyEffect(pEffect, 0, 0);
    }
    if (!g_pVirtualMachine->StackPushInteger(nDamage))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsImmune(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nImmunityType;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nImmunityType))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidAgainst = OBJECT_INVALID;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopObject(&oidAgainst))
            return VM_ERROR_POP_FAILED;
    }
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAgainst);
    CGameObject* pObject;
    int nRet = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSCreature() != 0) {
            if (nImmunityType >= 1 && nImmunityType < CSWSEffectListHandler::IMMUNITY_TYPE_MAX)
                nRet = pObject->AsSWSCreature()->m_pStats
                           ->GetEffectImmunity(static_cast<unsigned char>(nImmunityType), pCreature);
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nRet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsLinkImmune(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_PUSH_FAILED;
    int nValue = 0;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
            && pObject->AsSWSCreature() != 0) {
        nValue = pObject->AsSWSCreature()->m_pStats->GetEffectLinkImmunity(pEffect);
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    if (pEffect != 0)
        delete pEffect;
    pEffect = 0;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSendMessageToPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sMessage;
    unsigned long long oidPC;
    CSWCCMessageData* pMsg;
    if (nArgCount != 2)
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sMessage))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidPC);
    pMsg = 0;
    if (pPC != 0) {
        pMsg = new CSWCCMessageData;
        pMsg->SetString(0, sMessage);
        pPC->SendFeedbackMessage(FEEDBACK_SCRIPT_MESSAGE, pMsg);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsOpen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0) {
        if (pObject->AsSWSDoor() != 0)
            nValue = pObject->AsSWSDoor()->m_curOpenState != 0;
        else if (pObject->AsSWSPlaceable() != 0)
            nValue = pObject->AsSWSPlaceable()->m_nAnimation == SERVER_ANIMATION_PLACEABLE_OPEN;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsInConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        unsigned long long oidOwner = pObject->AsSWSObject()->m_dialogOwnerId;
        CGameObject* pOwnerGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidOwner);
        if (pOwnerGO != 0 && pOwnerGO->AsSWSObject() != 0)
            nValue = pOwnerGO->AsSWSObject()->m_dialogOwnerId == oidOwner;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandShowPartySelection(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
#if KOTOR1  // K1 pops the two forced NPCs only for exactly 3 arguments
    CExoString sText;
#ifdef K2_FIX_ENGINE_BUGS
    int nForceNpc1 = -1;
    int nForceNpc2 = -1;
#else
    int nForceNpc1;
    int nForceNpc2;
#endif
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    if (nArgCount == 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nForceNpc1))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nForceNpc2))
            return VM_ERROR_POP_FAILED;
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowPartySelection(sText, 1, nForceNpc1,
                                                                      nForceNpc2);
    return 0;
#else
    CExoString sText;
    int nForceNpc1 = -1;
    int nForceNpc2 = -1;
    int nD = 0;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nForceNpc1))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nForceNpc2))
            return VM_ERROR_POP_FAILED;
    }
    if (nArgCount >= 4) {
        if (!g_pVirtualMachine->StackPopInteger(&nD))
            return VM_ERROR_POP_FAILED;
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowPartySelection(sText, 1, nForceNpc1,
                                                                      nForceNpc2, nD);
    return 0;
#endif
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsLiveContentAvailable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sAlias;
    int nIndex;
    int nValue;
    const CExoString* pRet;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 treats content 1 as always available
    nValue = nIndex == 1;
    if (nIndex >= 2 && nIndex <= 6) {
        sAlias.Format(const_cast<char*>("LIVE%d"), nIndex);
        pRet = &g_pExoBase->m_pAliasList->GetAliasPath(sAlias, 0);
        nValue = pRet->GetLength() != 0;
    }
#else
    nValue = 0;
    if (nIndex > 0 && nIndex < 7) {
        sAlias.Format(const_cast<char*>("LIVE%d"), nIndex);
        pRet = &g_pExoBase->m_pAliasList->GetAliasPath(sAlias, 0);
        if (pRet->m_sString != 0)
            nValue = static_cast<int>(strlen(pRet->m_sString)) != 0;
    }
#endif
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetRestrictMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    int nValue = 0;
    if (pCreature != 0 && pCreature->m_bInParty != 0) {
        if (g_pAppManager->m_pServerExoApp->GetModule() != 0
                && g_pAppManager->m_pServerExoApp->GetModule()->GetArea() != 0)
            nValue = g_pAppManager->m_pServerExoApp->GetModule()->GetArea()
                         ->m_nPlayerRestrictMode;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetIsPoisoned(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0) {
        if (pObject->AsSWSObject() != 0) {
            CSWSObject* pSObject = pObject->AsSWSObject();
            if (pSObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                nValue = pSObject->AsSWSCreature()->m_bPoisoned;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Gates the five locked-object interactions. The door-action command picks the door twin of every
// check, anything else the placeable twin.
int CSWVirtualMachineCommands::ExecuteCommandGetIsDoorActionPossible(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    int nRet = 0;
    int nAction;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAction))
        return VM_ERROR_POP_FAILED;
    CSWSDoor* pDoor = (nCommand == CMD_GetIsDoorActionPossible)
        ? g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidTarget) : 0;
    CSWSPlaceable* pPlaceable = (nCommand == CMD_GetIsDoorActionPossible)
        ? 0 : g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidTarget);
    if (pCreature != 0 && ((pDoor != 0 && pDoor->m_bStatic == 0)
            || (pPlaceable != 0 && pPlaceable->m_bStatic == 0))) {
        switch (nAction) {
        case DOOR_ACTION_OPEN:
            if (nCommand == CMD_GetIsDoorActionPossible) {
                if (pDoor != 0 && pDoor->m_nAnimation == SERVER_ANIMATION_CLOSED)
                    nRet = pDoor->m_bLocked == 0;
            } else {
                if (pPlaceable != 0 && pPlaceable->m_nAnimation == SERVER_ANIMATION_PLACEABLE_CLOSE)
                    nRet = pPlaceable->m_bLocked == 0;
            }
            break;
        case DOOR_ACTION_UNLOCK:
        {
            signed char nSkill = pCreature->m_pStats->GetSkillRank(SKILL_SECURITY, 0, 0);
            if (nCommand == CMD_GetIsDoorActionPossible) {
                if (pDoor != 0 && pDoor->m_bLocked != 0 && pDoor->m_bKeyRequired == 0
                        && nSkill > 0)
                    nRet = nSkill + 20 >= static_cast<unsigned char>(pDoor->GetOpenLockDC());
            } else {
                if (pPlaceable != 0 && pPlaceable->m_bLocked != 0
                        && pPlaceable->m_bKeyRequired == 0 && nSkill > 0)
                    nRet = nSkill + 20 >= static_cast<unsigned char>(pPlaceable->GetOpenLockDC());
            }
        }
            break;
        case DOOR_ACTION_BASH:
            if ((pDoor != 0 && pDoor->m_bPlot == 0)
                    || (pPlaceable != 0 && pPlaceable->m_bPlot == 0))
                nRet = 1;
            break;
        case DOOR_ACTION_IGNORE:
            nRet = 1;
            break;
        case DOOR_ACTION_KNOCK:
            if ((pDoor != 0 && pDoor->m_bPlot == 0 && pDoor->m_bKeyRequired == 0)
                    || (pPlaceable != 0 && pPlaceable->m_bPlot == 0
                        && pPlaceable->m_bKeyRequired == 0)) {
                for (unsigned char nClass = 0; nClass <= pCreature->m_pStats->m_numClasses; nClass++) {
                    if (pCreature->m_pStats->GetSpellUsesLeft(SPELL_KNOCK, nClass, 0, 0) != 0) {
                        nRet = 1;
                        break;
                    }
                }
            }
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nRet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Deep-copies the matched-string list before indexing it.
int CSWVirtualMachineCommands::ExecuteCommandGetMatchedString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sMatched;
    CGameObject* pObject;
    int nWhich;
    if (!g_pVirtualMachine->StackPopInteger(&nWhich))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS
            && pObject != 0) {
        CSWSObject* pSWSObject = static_cast<CSWSObject*>(pObject);
        CExoArrayList<CExoString*> aCopied(pSWSObject->m_matchedExpressionStrings);
        if (nWhich >= 0 && nWhich < aCopied.m_count)
            sMatched = *aCopied.m_pArray[nWhich];
    }
    if (!g_pVirtualMachine->StackPushString(sMatched))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Answers the five door/placeable lock-data probes: key required, key name, lockable, open-lock DC
// and close-lock DC.
int CSWVirtualMachineCommands::ExecuteCommandGetLockInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidLockable;
    if (!g_pVirtualMachine->StackPopObject(&oidLockable))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidLockable);
#if KOTOR1
    int bPushed;
    switch (nCommand) {
    case CMD_GetLockKeyRequired:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSDoor()->m_bKeyRequired);
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSPlaceable()->m_bKeyRequired);
        else
            bPushed = g_pVirtualMachine->StackPushInteger(0);
        break;
    case CMD_GetLockKeyTag:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bPushed = g_pVirtualMachine->StackPushString(pObject->AsSWSDoor()->m_keyName);
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bPushed = g_pVirtualMachine->StackPushString(pObject->AsSWSPlaceable()->m_keyName);
        else
            bPushed = g_pVirtualMachine->StackPushString(CExoString(""));
        break;
    case CMD_GetLockLockable:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSDoor()->m_bLockable);
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSPlaceable()->m_bLockable);
        else
            bPushed = g_pVirtualMachine->StackPushInteger(0);
        break;
    case CMD_GetLockUnlockDC:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSDoor()->GetOpenLockDC());
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSPlaceable()->GetOpenLockDC());
        else
            bPushed = g_pVirtualMachine->StackPushInteger(0);
        break;
    case CMD_GetLockLockDC:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSDoor()->m_closeLockDC);
        else if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bPushed = g_pVirtualMachine->StackPushInteger(pObject->AsSWSPlaceable()->m_closeLockDC);
        else
            bPushed = g_pVirtualMachine->StackPushInteger(0);
        break;
    default:
        return 0;
    }
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    return 0;
#else
    int nRet = 0;
    switch (nCommand) {
    case CMD_GetLockKeyRequired:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nRet = pObject->AsSWSDoor()->m_bKeyRequired;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nRet = pObject->AsSWSPlaceable()->m_bKeyRequired;
        }
        break;
    case CMD_GetLockKeyTag:
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
            if (!g_pVirtualMachine->StackPushString(pObject->AsSWSDoor()->m_keyName))
                return VM_ERROR_PUSH_FAILED;
            return 0;
        } else if (pObject != 0
                && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            if (!g_pVirtualMachine->StackPushString(pObject->AsSWSPlaceable()->m_keyName))
                return VM_ERROR_PUSH_FAILED;
            return 0;
        } else {
            if (!g_pVirtualMachine->StackPushString(CExoString("")))
                return VM_ERROR_PUSH_FAILED;
            return 0;
        }
    case CMD_GetLockLockable:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nRet = pObject->AsSWSDoor()->m_bLockable;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nRet = pObject->AsSWSPlaceable()->m_bLockable;
        }
        break;
    case CMD_GetLockUnlockDC:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nRet = pObject->AsSWSDoor()->GetOpenLockDC();
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nRet = pObject->AsSWSPlaceable()->GetOpenLockDC();
        }
        if (!g_pVirtualMachine->StackPushInteger(nRet & 0xff))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    case CMD_GetLockLockDC:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nRet = pObject->AsSWSDoor()->m_closeLockDC;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nRet = pObject->AsSWSPlaceable()->m_closeLockDC;
        }
        break;
    default:
        return 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(nRet))
        return VM_ERROR_PUSH_FAILED;
    return 0;
#endif
}

int CSWVirtualMachineCommands::ExecuteCommandGetLocalValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nLocalIndex;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nLocalIndex))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0) {
        CSWVarTable* pVars;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
            pVars = &pObject->AsSWSArea()->m_varTable;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE)
            pVars = &pObject->AsSWSModule()->m_varTable;
        else
            pVars = &static_cast<CSWSObject*>(pObject)->m_varTable;
        if (nCommand == CMD_GetLocalBoolean)
            nValue = pVars->GetLocalBoolean(nLocalIndex);
        else if (nCommand == CMD_GetLocalNumber)
            nValue = pVars->GetLocalNumber(nLocalIndex) & 0xff;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptLocation sLocation;
    unsigned long long oidObject;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray->GetGameObject(oidObject, &pObject) == CGameObjectArray::SUCCESS
        && pObject != 0
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        sLocation = static_cast<CSWSObject*>(pObject)->GetScriptLocation();
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, &sLocation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLocked(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int nLocked = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pArray->GetGameObject(oidObject, &pObject) == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSDoor() != 0)
            nLocked = pObject->AsSWSDoor()->m_bLocked;
        else if (pObject->AsSWSPlaceable() != 0)
            nLocked = pObject->AsSWSPlaceable()->m_bLocked;
    }
    if (!g_pVirtualMachine->StackPushInteger(nLocked))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}


int CSWVirtualMachineCommands::ExecuteCommandGetReflexSavingThrow(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature != 0 && pCreature->m_pStats != 0)
                nValue = pCreature->m_pStats->GetReflexSavingThrow(0);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            if (pDoor != 0)
                nValue = pDoor->m_refSave;
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            if (pPlaceable != 0)
                nValue = pPlaceable->m_refSave;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"
#include "port/crtapi.h"

void runscript(char* pszText)
{
    CExoString sScript;
    char szName[20];
    static_cast<void>(sscanf(pszText, "%s %d", szName, &g_nRunScriptVar));
    sScript = szName;
    g_pVirtualMachine->RunScript(&sScript, 0, 0);
}

// Looks for a walkable clear line from the creature to a random point within nRange: a random
// offset, a direct-line test, and on a blocked line a shrinking probe re-fired from the creature,
// up to 21 tries. Falls back to the creature's position.
int CSWVirtualMachineCommands::ExecuteCommandGetRandomDestination(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nRange = 0;
    Vector vPos;
    Vector vDir;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nRange))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature == 0) {
        if (!g_pVirtualMachine->StackPushVector(vPos))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
    if (pArea == 0) {
        if (!g_pVirtualMachine->StackPushVector(vPos))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    vPos = pCreature->m_position;
    Vector vCand;
    float fX = vPos.x, fY = vPos.y;
    vCand.x = fX + static_cast<float>(rand_wincompatible() % (nRange * 2) - nRange);
    vCand.y = fY + static_cast<float>(rand_wincompatible() % (nRange * 2) - nRange);
    vCand.z = pArea->ComputeHeight(vCand, 1, 0, 0);
    vDir = vCand - vPos;
    float fDist = magnitude(vDir);
    vDir = normalize(vDir);
    pArea->m_pActivePathfindInfo = pCreature->m_pPathfindInfo;
    if (pArea->TestDirectLine(vPos, vCand, static_cast<float>(pCreature->m_pPathfindInfo->m_fPersonalSpace),
                pCreature->m_pPathfindInfo->m_fHeightAlias, 0, 0) == 1) {
        if (!g_pVirtualMachine->StackPushVector(vCand))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    float fStep = fDist;
    for (int nTry = 20;;) {
        fStep *= 0.75f;
        if (fStep < 2.5f) {
            switch (rand_wincompatible() % 3) {
            case 0:
                vDir = -vDir;
                break;
            case 1: {
                Vector v = vDir;
                vDir.x = v.z * 0.0f - v.y;
                vDir.y = v.x - v.z * 0.0f;
                vDir.z = v.y * 0.0f - v.x * 0.0f;
                break;
            }
            case 2: {
                Vector v = vDir;
                vDir.x = -(v.z * 0.0f - v.y);
                vDir.y = -(v.x - v.z * 0.0f);
                vDir.z = -(v.y * 0.0f - v.x * 0.0f);
                break;
            }
            }
            fStep = fDist;
        }
        vCand = vPos + vDir * fStep;
        if (fStep < 1.0f) {
            vCand = vPos;
            break;
        }
        if (nTry-- < 0) {
            vCand = vPos;
            break;
        }
        if (pArea->TestDirectLine(vPos, vCand, static_cast<float>(pCreature->m_pPathfindInfo->m_fPersonalSpace),
                    pCreature->m_pPathfindInfo->m_fHeightAlias, 0, 0) == 1)
            break;
    }
    if (!g_pVirtualMachine->StackPushVector(vCand))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Plays the scripted animation on a dialogue participant: snaps it back to idle first, translates the
// script constant into an engine animation id and either plays it instantly (negative duration) or
// queues it as an action. Outside a local-client object the target is left uninitialised and read
// anyway.
int CSWVirtualMachineCommands::ExecuteCommandPlayAnimation(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameObject* pObject;
    float fSpeed;
    float fDurationSeconds;
    int nAnimation;
    int nOne;
    if (!g_pVirtualMachine->StackPopInteger(&nAnimation))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fSpeed))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fDurationSeconds))
        return VM_ERROR_POP_FAILED;

    if (m_nScriptRunArg == 1) {
        g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject);
    }
#ifdef K2_FIX_ENGINE_BUGS
    else
        pObject = 0;
    if (pObject == 0)
        return 0;
#endif

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_oidSelf);
        unsigned long long oidA =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogParticipantIdA;
        unsigned long long oidB =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogParticipantIdB;
        if (oidClient == oidA) {
            CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(oidClient);
            if (pClientObject != 0 && pClientObject->AsSWCObject() != 0) {
                pClientObject->AsSWCObject()->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 0);
            }
        }
        if (oidClient == oidB) {
            CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(oidClient);
            if (pClientObject != 0 && pClientObject->AsSWCObject() != 0) {
                pClientObject->AsSWCObject()->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 0);
            }
        }
    }

    int nScriptAnimation = nAnimation;
#if KOTOR1  // K1 has no 10001+ pass-through and 19 falls into 20
    if (pObject->AsSWSCreature() != 0) {
        switch (nAnimation) {
        case ANIMATION_LOOPING_PAUSE: nAnimation = SERVER_ANIMATION_PAUSE; break;
        case ANIMATION_LOOPING_PAUSE2: nAnimation = SERVER_ANIMATION_PAUSE2; break;
        case ANIMATION_LOOPING_LISTEN: nAnimation = SERVER_ANIMATION_LISTEN; break;
        case ANIMATION_LOOPING_MEDITATE: nAnimation = SERVER_ANIMATION_MEDITATE; break;
        case ANIMATION_LOOPING_WORSHIP: nAnimation = SERVER_ANIMATION_WORSHIP; break;
        case ANIMATION_LOOPING_TALK_NORMAL: nAnimation = SERVER_ANIMATION_TALK_NORMAL; break;
        case ANIMATION_LOOPING_TALK_PLEADING: nAnimation = SERVER_ANIMATION_TALK_PLEADING; break;
        case ANIMATION_LOOPING_TALK_FORCEFUL: nAnimation = SERVER_ANIMATION_TALK_FORCEFUL; break;
        case ANIMATION_LOOPING_TALK_LAUGHING: nAnimation = SERVER_ANIMATION_TALK_LAUGHING; break;
        case ANIMATION_LOOPING_TALK_SAD: nAnimation = SERVER_ANIMATION_TALK_SAD; break;
        case ANIMATION_LOOPING_GET_LOW: nAnimation = SERVER_ANIMATION_GET_LOW; break;
        case ANIMATION_LOOPING_GET_MID: nAnimation = SERVER_ANIMATION_GET_MID; break;
        case ANIMATION_LOOPING_PAUSE_TIRED: nAnimation = SERVER_ANIMATION_PAUSE_TIRED; break;
        case ANIMATION_LOOPING_PAUSE_DRUNK: nAnimation = SERVER_ANIMATION_PAUSE_DRUNK; break;
        case ANIMATION_LOOPING_FLIRT: nAnimation = SERVER_ANIMATION_FLIRT; break;
        case ANIMATION_LOOPING_USE_COMPUTER: nAnimation = SERVER_ANIMATION_USE_COMPUTER_LOOP; break;
        case ANIMATION_LOOPING_DANCE: nAnimation = SERVER_ANIMATION_DANCE; break;
        case ANIMATION_LOOPING_DANCE1: nAnimation = SERVER_ANIMATION_DANCE1; break;
        case ANIMATION_LOOPING_HORROR: nAnimation = SERVER_ANIMATION_HORROR; break;
        case ANIMATION_LOOPING_READY: nAnimation = SERVER_ANIMATION_READY;  // falls through
        case ANIMATION_LOOPING_DEACTIVATE: nAnimation = SERVER_ANIMATION_DEACTIVATE; break;
        case ANIMATION_LOOPING_SPASM: nAnimation = SERVER_ANIMATION_SPASM; break;
        case ANIMATION_LOOPING_SLEEP: nAnimation = SERVER_ANIMATION_SLEEP; break;
        case ANIMATION_LOOPING_PRONE: nAnimation = SERVER_ANIMATION_PRONE; break;
        case ANIMATION_LOOPING_PAUSE3: nAnimation = SERVER_ANIMATION_PAUSE3; break;
        case ANIMATION_LOOPING_WELD: nAnimation = SERVER_ANIMATION_WELD; break;
        case ANIMATION_LOOPING_DEAD: nAnimation = SERVER_ANIMATION_DEAD; break;
        case ANIMATION_LOOPING_TALK_INJURED: nAnimation = SERVER_ANIMATION_TALK_INJURED; break;
        case ANIMATION_LOOPING_LISTEN_INJURED: nAnimation = SERVER_ANIMATION_LISTEN_INJURED; break;
        case ANIMATION_LOOPING_TREAT_INJURED: nAnimation = SERVER_ANIMATION_TREAT_INJURED_LOOP; break;
        case ANIMATION_LOOPING_DEAD_PRONE: nAnimation = SERVER_ANIMATION_DEAD_PRONE; break;
        case ANIMATION_LOOPING_KNEEL_TALK_ANGRY: nAnimation = SERVER_ANIMATION_KNEEL_TALK_ANGRY; break;
        case ANIMATION_LOOPING_KNEEL_TALK_SAD: nAnimation = SERVER_ANIMATION_KNEEL_TALK_SAD; break;
        case ANIMATION_FIREFORGET_HEAD_TURN_LEFT: nAnimation = SERVER_ANIMATION_HEAD_TURN_LEFT; break;
        case ANIMATION_FIREFORGET_HEAD_TURN_RIGHT: nAnimation = SERVER_ANIMATION_HEAD_TURN_RIGHT; break;
        case ANIMATION_FIREFORGET_PAUSE_SCRATCH_HEAD: nAnimation = SERVER_ANIMATION_PAUSE_SCRATCH_HEAD; break;
        case ANIMATION_FIREFORGET_PAUSE_BORED: nAnimation = SERVER_ANIMATION_PAUSE_BORED; break;
        case ANIMATION_FIREFORGET_SALUTE: nAnimation = SERVER_ANIMATION_SALUTE; break;
        case ANIMATION_FIREFORGET_BOW: nAnimation = SERVER_ANIMATION_BOW; break;
        case ANIMATION_FIREFORGET_GREETING: nAnimation = SERVER_ANIMATION_GREETING; break;
        case ANIMATION_FIREFORGET_TAUNT: nAnimation = SERVER_ANIMATION_TAUNT; break;
        case ANIMATION_FIREFORGET_VICTORY1:
        case ANIMATION_FIREFORGET_VICTORY2:
        case ANIMATION_FIREFORGET_VICTORY3: nAnimation = SERVER_ANIMATION_VICTORY; break;
        case ANIMATION_FIREFORGET_INJECT: {
            CExoString sModelType;
            g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                    pObject->AsSWSCreature()->m_appearance.m_headId,
                    CExoString("MODELTYPE"), &sModelType);
            if (sModelType.CompareNoCase("B") || sModelType.CompareNoCase("F")) {
                nAnimation = SERVER_ANIMATION_INJECT;
            } else {
                nAnimation = SERVER_ANIMATION_PAUSE;
            }
            break;
        }
        case ANIMATION_FIREFORGET_USE_COMPUTER: nAnimation = SERVER_ANIMATION_USE_COMPUTER; break;
        case ANIMATION_FIREFORGET_PERSUADE: nAnimation = SERVER_ANIMATION_PERSUADE; break;
        case ANIMATION_FIREFORGET_ACTIVATE: nAnimation = SERVER_ANIMATION_ACTIVATE; break;
        case ANIMATION_FIREFORGET_CHOKE: nAnimation = SERVER_ANIMATION_CHOKE; break;
        case ANIMATION_FIREFORGET_THROW_HIGH: nAnimation = SERVER_ANIMATION_THROW_HIGH; break;
        case ANIMATION_FIREFORGET_THROW_LOW: nAnimation = SERVER_ANIMATION_THROW_LOW; break;
        case ANIMATION_FIREFORGET_CUSTOM01: nAnimation = SERVER_ANIMATION_CUSTOM01; break;
        case ANIMATION_FIREFORGET_TREAT_INJURED: nAnimation = SERVER_ANIMATION_TREAT_INJURED; break;
        default: nAnimation = SERVER_ANIMATION_PAUSE; break;
        }
    } else if (pObject->AsSWSPlaceable() != 0) {
#else
    if (pObject->AsSWSCreature() != 0) {
        // A script id above 10000 names animations.2da row (id - 10000) directly:
        // ServerToClientAnimation passes ids it does not map through unchanged.
        if (nAnimation >= SERVER_ANIMATION_BASE + 1) {
            nAnimation -= SERVER_ANIMATION_BASE;
        } else {
            switch (nAnimation) {
            case ANIMATION_LOOPING_PAUSE: nAnimation = SERVER_ANIMATION_PAUSE; break;
            case ANIMATION_LOOPING_PAUSE2: nAnimation = SERVER_ANIMATION_PAUSE2; break;
            case ANIMATION_LOOPING_LISTEN: nAnimation = SERVER_ANIMATION_LISTEN; break;
            case ANIMATION_LOOPING_MEDITATE: nAnimation = SERVER_ANIMATION_MEDITATE; break;
            case ANIMATION_LOOPING_WORSHIP: nAnimation = SERVER_ANIMATION_WORSHIP; break;
            case ANIMATION_LOOPING_TALK_NORMAL: nAnimation = SERVER_ANIMATION_TALK_NORMAL; break;
            case ANIMATION_LOOPING_TALK_PLEADING: nAnimation = SERVER_ANIMATION_TALK_PLEADING; break;
            case ANIMATION_LOOPING_TALK_FORCEFUL: nAnimation = SERVER_ANIMATION_TALK_FORCEFUL; break;
            case ANIMATION_LOOPING_TALK_LAUGHING: nAnimation = SERVER_ANIMATION_TALK_LAUGHING; break;
            case ANIMATION_LOOPING_TALK_SAD: nAnimation = SERVER_ANIMATION_TALK_SAD; break;
            case ANIMATION_LOOPING_GET_LOW: nAnimation = SERVER_ANIMATION_GET_LOW; break;
            case ANIMATION_LOOPING_GET_MID: nAnimation = SERVER_ANIMATION_GET_MID; break;
            case ANIMATION_LOOPING_PAUSE_TIRED: nAnimation = SERVER_ANIMATION_PAUSE_TIRED; break;
            case ANIMATION_LOOPING_PAUSE_DRUNK: nAnimation = SERVER_ANIMATION_PAUSE_DRUNK; break;
            case ANIMATION_LOOPING_FLIRT: nAnimation = SERVER_ANIMATION_FLIRT; break;
            case ANIMATION_LOOPING_USE_COMPUTER: nAnimation = SERVER_ANIMATION_USE_COMPUTER_LOOP; break;
            case ANIMATION_LOOPING_DANCE: nAnimation = SERVER_ANIMATION_DANCE; break;
            case ANIMATION_LOOPING_DANCE1: nAnimation = SERVER_ANIMATION_DANCE1; break;
            case ANIMATION_LOOPING_HORROR: nAnimation = SERVER_ANIMATION_HORROR; break;
            case ANIMATION_LOOPING_READY: nAnimation = SERVER_ANIMATION_READY; break;
            case ANIMATION_LOOPING_DEACTIVATE: nAnimation = SERVER_ANIMATION_DEACTIVATE; break;
            case ANIMATION_LOOPING_SPASM: nAnimation = SERVER_ANIMATION_SPASM; break;
            case ANIMATION_LOOPING_SLEEP: nAnimation = SERVER_ANIMATION_SLEEP; break;
            case ANIMATION_LOOPING_PRONE: nAnimation = SERVER_ANIMATION_PRONE; break;
            case ANIMATION_LOOPING_PAUSE3: nAnimation = SERVER_ANIMATION_PAUSE3; break;
            case ANIMATION_LOOPING_WELD: nAnimation = SERVER_ANIMATION_WELD; break;
            case ANIMATION_LOOPING_DEAD: nAnimation = SERVER_ANIMATION_DEAD; break;
            case ANIMATION_LOOPING_TALK_INJURED: nAnimation = SERVER_ANIMATION_TALK_INJURED; break;
            case ANIMATION_LOOPING_LISTEN_INJURED: nAnimation = SERVER_ANIMATION_LISTEN_INJURED; break;
            case ANIMATION_LOOPING_TREAT_INJURED: nAnimation = SERVER_ANIMATION_TREAT_INJURED_LOOP; break;
            case ANIMATION_LOOPING_DEAD_PRONE: nAnimation = SERVER_ANIMATION_DEAD_PRONE; break;
            case ANIMATION_LOOPING_KNEEL_TALK_ANGRY: nAnimation = SERVER_ANIMATION_KNEEL_TALK_ANGRY; break;
            case ANIMATION_LOOPING_KNEEL_TALK_SAD: nAnimation = SERVER_ANIMATION_KNEEL_TALK_SAD; break;
            case ANIMATION_LOOPING_CHECK_BODY: nAnimation = SERVER_ANIMATION_CHECK_BODY; break;
            case ANIMATION_LOOPING_UNLOCK_DOOR: nAnimation = SERVER_ANIMATION_UNLOCK_DOOR; break;
            case ANIMATION_LOOPING_SIT_AND_MEDITATE: nAnimation = SERVER_ANIMATION_SIT_AND_MEDITATE; break;
            case ANIMATION_LOOPING_SIT_CHAIR: nAnimation = SERVER_ANIMATION_SIT_CHAIR; break;
            case ANIMATION_LOOPING_SIT_CHAIR_DRINK: nAnimation = SERVER_ANIMATION_SIT_CHAIR_DRINK; break;
            case ANIMATION_LOOPING_SIT_CHAIR_PAZAK: nAnimation = SERVER_ANIMATION_SIT_CHAIR_PAZAK; break;
            case ANIMATION_LOOPING_SIT_CHAIR_COMP1: nAnimation = SERVER_ANIMATION_SIT_CHAIR_COMP1; break;
            case ANIMATION_LOOPING_SIT_CHAIR_COMP2: nAnimation = SERVER_ANIMATION_SIT_CHAIR_COMP2; break;
            case ANIMATION_LOOPING_RAGE: nAnimation = SERVER_ANIMATION_FORCE_RAGE; break;
            case 42: break;
            case ANIMATION_LOOPING_CLOSED: nAnimation = SERVER_ANIMATION_CLOSED; break;
            case ANIMATION_LOOPING_STEALTH: nAnimation = SERVER_ANIMATION_STEALTH; break;
            case ANIMATION_LOOPING_CHOKE_WORKING: nAnimation = SERVER_ANIMATION_CHOKE; break;
            case ANIMATION_LOOPING_MEDITATE_STAND: nAnimation = SERVER_ANIMATION_MEDITATE_STAND; break;
            case ANIMATION_FIREFORGET_HEAD_TURN_LEFT: nAnimation = SERVER_ANIMATION_HEAD_TURN_LEFT; break;
            case ANIMATION_FIREFORGET_HEAD_TURN_RIGHT: nAnimation = SERVER_ANIMATION_HEAD_TURN_RIGHT; break;
            case ANIMATION_FIREFORGET_PAUSE_SCRATCH_HEAD: nAnimation = SERVER_ANIMATION_PAUSE_SCRATCH_HEAD; break;
            case ANIMATION_FIREFORGET_PAUSE_BORED: nAnimation = SERVER_ANIMATION_PAUSE_BORED; break;
            case ANIMATION_FIREFORGET_SALUTE: nAnimation = SERVER_ANIMATION_SALUTE; break;
            case ANIMATION_FIREFORGET_BOW: nAnimation = SERVER_ANIMATION_BOW; break;
            case ANIMATION_FIREFORGET_GREETING: nAnimation = SERVER_ANIMATION_GREETING; break;
            case ANIMATION_FIREFORGET_TAUNT: nAnimation = SERVER_ANIMATION_TAUNT; break;
            case ANIMATION_FIREFORGET_VICTORY1:
            case ANIMATION_FIREFORGET_VICTORY2:
            case ANIMATION_FIREFORGET_VICTORY3: nAnimation = SERVER_ANIMATION_VICTORY; break;
            case ANIMATION_FIREFORGET_INJECT: {
                CExoString sModelType;
                g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                        pObject->AsSWSCreature()->m_appearance.m_headId,
                        CExoString("MODELTYPE"), &sModelType);
                if (sModelType.CompareNoCase("B") || sModelType.CompareNoCase("F")) {
                    nAnimation = SERVER_ANIMATION_INJECT;
                } else {
                    nAnimation = SERVER_ANIMATION_PAUSE;
                }
                break;
            }
            case ANIMATION_FIREFORGET_USE_COMPUTER: nAnimation = SERVER_ANIMATION_USE_COMPUTER; break;
            case ANIMATION_FIREFORGET_PERSUADE: nAnimation = SERVER_ANIMATION_PERSUADE; break;
            case ANIMATION_FIREFORGET_ACTIVATE: nAnimation = SERVER_ANIMATION_ACTIVATE; break;
            case ANIMATION_FIREFORGET_THROW_HIGH: nAnimation = SERVER_ANIMATION_THROW_HIGH; break;
            case ANIMATION_FIREFORGET_THROW_LOW: nAnimation = SERVER_ANIMATION_THROW_LOW; break;
            case ANIMATION_FIREFORGET_CUSTOM01: nAnimation = SERVER_ANIMATION_CUSTOM01; break;
            case ANIMATION_FIREFORGET_TREAT_INJURED: nAnimation = SERVER_ANIMATION_TREAT_INJURED; break;
            case ANIMATION_FIREFORGET_FORCE_CAST: nAnimation = SERVER_ANIMATION_CAST_1; break;
            case ANIMATION_FIREFORGET_OPEN: nAnimation = SERVER_ANIMATION_OPEN; break;
            case ANIMATION_FIREFORGET_SCREAM: nAnimation = SERVER_ANIMATION_FORCE_RAGE; break;
            default: nAnimation = SERVER_ANIMATION_PAUSE; break;
            }
        }
    } else if (pObject->AsSWSPlaceable() != 0) {
#endif
        switch (nAnimation) {
        case ANIMATION_PLACEABLE_ACTIVATE: nAnimation = SERVER_ANIMATION_PLACEABLE_ACTIVATE; break;
        case ANIMATION_PLACEABLE_DEACTIVATE: nAnimation = SERVER_ANIMATION_PLACEABLE_DEACTIVATE; break;
        case ANIMATION_PLACEABLE_OPEN: nAnimation = SERVER_ANIMATION_PLACEABLE_OPEN; break;
        case ANIMATION_PLACEABLE_CLOSE: nAnimation = SERVER_ANIMATION_PLACEABLE_CLOSE; break;
        case ANIMATION_PLACEABLE_ANIMLOOP01: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP01; break;
        case ANIMATION_PLACEABLE_ANIMLOOP02: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP02; break;
        case ANIMATION_PLACEABLE_ANIMLOOP03: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP03; break;
        case ANIMATION_PLACEABLE_ANIMLOOP04: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP04; break;
        case ANIMATION_PLACEABLE_ANIMLOOP05: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP05; break;
        case ANIMATION_PLACEABLE_ANIMLOOP06: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP06; break;
        case ANIMATION_PLACEABLE_ANIMLOOP07: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP07; break;
        case ANIMATION_PLACEABLE_ANIMLOOP08: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP08; break;
        case ANIMATION_PLACEABLE_ANIMLOOP09: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP09; break;
        case ANIMATION_PLACEABLE_ANIMLOOP10: nAnimation = SERVER_ANIMATION_PLACEABLE_ANIMLOOP10; break;
        default: nAnimation = SERVER_ANIMATION_PAUSE; break;
        }
    }

    if (fDurationSeconds < 0.0f) {
        // Fire-and-forget animation ids only.
#if KOTOR1  // K1 allows only ids below 33
        if (static_cast<unsigned int>(nScriptAnimation) >= 33)
            return 0;
#else
        if (static_cast<unsigned int>(nScriptAnimation) >= 47 && nScriptAnimation < SERVER_ANIMATION_BASE + 1)
            return 0;
#endif
        CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
        if (pServerObject == 0 || pServerObject->AsSWSObject() == 0)
            return 0;
        pServerObject->AsSWSObject()->SetAnimation(nAnimation);
        return 0;
    }

    if (pObject->AsSWSObject()->m_bCommandable == 0)
        return 0;
    nOne = 1;
    // Command 300 wipes the action queue and plays the animation at the front of it.
    if (nCommand == CMD_PlayAnimation) {
        pObject->AsSWSObject()->ClearAllActions(1);
        pObject->AsSWSObject()->AddActionToFront(AIACTION_PLAY_ANIMATION, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nAnimation, ACTION_PARAM_FLOAT, &fSpeed,
                ACTION_PARAM_FLOAT, &fDurationSeconds, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0);
    } else {
        pObject->AsSWSObject()->AddAction(AIACTION_PLAY_ANIMATION, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nAnimation, ACTION_PARAM_FLOAT, &fSpeed,
                ACTION_PARAM_FLOAT, &fDurationSeconds, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0);
    }
    return 0;
}

// nwscript GetFirstInPersistentObject / GetNextInPersistentObject: walks the persistent
// object's bracket of the area's X-sorted object index, keeping candidates that clear the
// object-type mask and the persistent object's containment test. Encounter, area-of-effect and
// trigger each supply their own bracket, containment test and resume cursor; GetNext resumes
// from the cursor the previous hit stored. A mask or zone argument that was never pushed is
// read uninitialised.
int CSWVirtualMachineCommands::ExecuteCommandGetInPersistentObject(int nCommand, int nArgCount)
{
    unsigned long long oidPersistent = OBJECT_INVALID;
#ifdef K2_FIX_ENGINE_BUGS
    int nResidentObjectType = OBJECT_TYPE_CREATURE;
    int nPersistentZone = 0;
    int nIndex = 0;
#else
    int nResidentObjectType;
    int nPersistentZone;
    int nIndex;
#endif
    CGameObject* pPersistent;

    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopObject(&oidPersistent))
            return VM_ERROR_POP_FAILED;
        if (nArgCount >= 2) {
            if (!g_pVirtualMachine->StackPopInteger(&nResidentObjectType))
                return VM_ERROR_POP_FAILED;
            if (nArgCount >= 3) {
                if (!g_pVirtualMachine->StackPopInteger(&nPersistentZone))
                    return VM_ERROR_POP_FAILED;
            }
        }
    }

    unsigned long long oidResult;
    unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
            oidPersistent, &pPersistent);
    if (nStatus == CGameObjectArray::SUCCESS) {
        if (pPersistent->AsSWSEncounter() != 0) {
            CSWSEncounter* pEncounter = pPersistent->AsSWSEncounter();
            CSWSArea* pArea = static_cast<CSWSArea*>(pPersistent->AsSWSEncounter()->GetArea());
            if (pArea != 0) {
#ifdef K2_FIX_ENGINE_BUGS
                float fScanStartX = 0.0f;
                float fScanEndX = 0.0f;
                if (nPersistentZone != 0)
                    goto pushInvalid;
#else
                float fScanStartX;
                float fScanEndX;
#endif
                if (nPersistentZone == 0) {
                    fScanStartX = pEncounter->m_fBBoxMinX;
                    fScanEndX = pEncounter->m_fBBoxMaxX;
                }
                if (nCommand == CMD_GetFirstInPersistentObject) {
                    if (!pArea->GetFirstObjectIndiceByX(&nIndex, fScanStartX))
                        goto pushInvalid;
                } else {
                    if (nPersistentZone == 0)
                        nIndex = pEncounter->m_nScanIndex + 1;
                    if (nIndex == pArea->m_aObjectsByX.m_count)
                        goto pushInvalid;
                }
                CSWSObject* pCandidate;
                Vector vPos;
                while (nIndex < pArea->m_aObjectsByX.m_count) {
                    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                            pArea->m_aObjectsByX.m_pArray[nIndex], reinterpret_cast<CGameObject**>(&pCandidate)) == nStatus &&
                        pCandidate != 0) {
                        if ((pCandidate->m_objectType == GAME_OBJECT_TYPE_CREATURE && nResidentObjectType == OBJECT_TYPE_CREATURE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ITEM && nResidentObjectType == OBJECT_TYPE_ITEM) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_TRIGGER && nResidentObjectType == OBJECT_TYPE_TRIGGER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_DOOR && nResidentObjectType == OBJECT_TYPE_DOOR) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT && nResidentObjectType == OBJECT_TYPE_AREA_OF_EFFECT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_WAYPOINT && nResidentObjectType == OBJECT_TYPE_WAYPOINT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_PLACEABLE && nResidentObjectType == OBJECT_TYPE_PLACEABLE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_STORE && nResidentObjectType == OBJECT_TYPE_STORE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER && nResidentObjectType == OBJECT_TYPE_ENCOUNTER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_SOUND && nResidentObjectType == OBJECT_TYPE_SOUND) ||
                            nResidentObjectType == OBJECT_TYPE_ALL) {
                            vPos = pCandidate->m_position;
                            if (vPos.x > fScanEndX)
                                goto pushInvalid;
                            if (nPersistentZone != 0)
                                goto pushNotFound;
                            if (pEncounter->GetInActivateArea(vPos) == 0) {
                                nIndex++;
                                goto pushNotFound;
                            }
                            oidResult = pArea->m_aObjectsByX.m_pArray[nIndex];
                            pEncounter->m_nScanIndex = static_cast<unsigned short>(nIndex + 1);
                            goto pushFound;
                        }
                        nIndex++;
                    }
                }
            }
        } else if (pPersistent->AsSWSAreaOfEffectObject() != 0) {
            if (nPersistentZone != 0)
                goto pushInvalid;
            CSWSAreaOfEffectObject* pAreaOfEffect = pPersistent->AsSWSAreaOfEffectObject();
            CSWSArea* pArea = static_cast<CSWSArea*>(pPersistent->AsSWSAreaOfEffectObject()->GetArea());
            if (pArea != 0) {
                float fScanStartX = pAreaOfEffect->GetPosition().x - pAreaOfEffect->m_fRadius;
                float fScanEndX = pAreaOfEffect->GetPosition().x + pAreaOfEffect->m_fRadius;
                if (nCommand == CMD_GetFirstInPersistentObject) {
                    if (!pArea->GetFirstObjectIndiceByX(&nIndex, fScanStartX))
                        goto pushInvalid;
                } else {
                    nIndex = pAreaOfEffect->m_nScanIndex;
                    if (pArea->m_aObjectsByX.m_count == nIndex)
                        goto pushInvalid;
                }
                CSWSObject* pCandidate;
                while (nIndex < pArea->m_aObjectsByX.m_count) {
                    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                            pArea->m_aObjectsByX.m_pArray[nIndex], reinterpret_cast<CGameObject**>(&pCandidate)) == nStatus &&
                        pCandidate != 0) {
                        if ((pCandidate->m_objectType == GAME_OBJECT_TYPE_CREATURE && nResidentObjectType == OBJECT_TYPE_CREATURE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ITEM && nResidentObjectType == OBJECT_TYPE_ITEM) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_TRIGGER && nResidentObjectType == OBJECT_TYPE_TRIGGER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_DOOR && nResidentObjectType == OBJECT_TYPE_DOOR) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT && nResidentObjectType == OBJECT_TYPE_AREA_OF_EFFECT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_WAYPOINT && nResidentObjectType == OBJECT_TYPE_WAYPOINT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_PLACEABLE && nResidentObjectType == OBJECT_TYPE_PLACEABLE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_STORE && nResidentObjectType == OBJECT_TYPE_STORE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER && nResidentObjectType == OBJECT_TYPE_ENCOUNTER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_SOUND && nResidentObjectType == OBJECT_TYPE_SOUND) ||
                            nResidentObjectType == OBJECT_TYPE_ALL) {
                            Vector vPos = pCandidate->m_position;
                            if (vPos.x > fScanEndX) {
                                pAreaOfEffect->m_nScanIndex = 0;
                                goto pushInvalid;
                            }
                            // The calls below are kept for their side effects only.
                            if (pCandidate->AsSWSCreature() != 0)
                                pCandidate->AsSWSCreature()->GetDeadTemp();
                            if (pAreaOfEffect->InAreaOfEffect(vPos) != 0 &&
                                pCandidate->AsSWSObject()->GetDead() == 0) {
                                CSWSCreature* pCreature = pCandidate->AsSWSCreature();
                                if (pCreature->GetDeadTemp() == 0) {
                                    oidResult = pArea->m_aObjectsByX.m_pArray[nIndex];
                                    pAreaOfEffect->m_nScanIndex = static_cast<unsigned short>(nIndex + 1);
                                    goto pushFound;
                                }
                            }
                        }
                        nIndex++;
                    }
                }
            }
        } else if (pPersistent->AsSWSTrigger() != 0) {
            if (nPersistentZone != 0)
                goto pushInvalid;
            CSWSTrigger* pTrigger = pPersistent->AsSWSTrigger();
            CSWSArea* pArea = static_cast<CSWSArea*>(pTrigger->GetArea());
            if (pArea != 0) {
                float fScanStartX = pTrigger->m_pGeomVerts[0].x;
                float fScanEndX = fScanStartX;
                for (int i = 1; i < pTrigger->m_nGeomVertCount; i++) {
                    if (pTrigger->m_pGeomVerts[i].x < fScanStartX)
                        fScanStartX = pTrigger->m_pGeomVerts[i].x;
                    if (pTrigger->m_pGeomVerts[i].x > fScanEndX)
                        fScanEndX = pTrigger->m_pGeomVerts[i].x;
                }
                if (nCommand == CMD_GetFirstInPersistentObject) {
                    if (!pArea->GetFirstObjectIndiceByX(&nIndex, fScanStartX))
                        goto pushInvalid;
                } else {
                    nIndex = pTrigger->m_nScanIndex;
                    if (pArea->m_aObjectsByX.m_count == nIndex)
                        goto pushInvalid;
                }
                CSWSObject* pCandidate;
                Vector vPos;
                while (nIndex < pArea->m_aObjectsByX.m_count) {
                    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                            pArea->m_aObjectsByX.m_pArray[nIndex], reinterpret_cast<CGameObject**>(&pCandidate)) == nStatus &&
                        pCandidate != 0) {
                        if ((pCandidate->m_objectType == GAME_OBJECT_TYPE_CREATURE && nResidentObjectType == OBJECT_TYPE_CREATURE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ITEM && nResidentObjectType == OBJECT_TYPE_ITEM) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_TRIGGER && nResidentObjectType == OBJECT_TYPE_TRIGGER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_DOOR && nResidentObjectType == OBJECT_TYPE_DOOR) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT && nResidentObjectType == OBJECT_TYPE_AREA_OF_EFFECT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_WAYPOINT && nResidentObjectType == OBJECT_TYPE_WAYPOINT) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_PLACEABLE && nResidentObjectType == OBJECT_TYPE_PLACEABLE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_STORE && nResidentObjectType == OBJECT_TYPE_STORE) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER && nResidentObjectType == OBJECT_TYPE_ENCOUNTER) ||
                            (pCandidate->m_objectType == GAME_OBJECT_TYPE_SOUND && nResidentObjectType == OBJECT_TYPE_SOUND) ||
                            nResidentObjectType == OBJECT_TYPE_ALL) {
                            vPos = pCandidate->m_position;
                            if (vPos.x > fScanEndX) {
                                pTrigger->m_nScanIndex = 0;
                                goto pushInvalid;
                            }
                            if (pTrigger->InTrigger(vPos) != 0 &&
                                pCandidate->AsSWSObject()->GetDead() == 0) {
                                CSWSCreature* pCreature = pCandidate->AsSWSCreature();
                                if (pCreature->GetDeadTemp() == 0) {
                                    oidResult = pArea->m_aObjectsByX.m_pArray[nIndex];
                                    pTrigger->m_nScanIndex = static_cast<unsigned short>(nIndex + 1);
                                    goto pushFound;
                                }
                            }
                        }
                        nIndex++;
                    }
                }
            }
        }
    }
    goto pushNotFound;

pushNotFound:
    if (!g_pVirtualMachine->StackPushObject(OBJECT_INVALID))
        return VM_ERROR_PUSH_FAILED;
    return 0;

pushFound:
    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;

pushInvalid:
    if (!g_pVirtualMachine->StackPushObject(CGameObjectArray::INVALIDOBJID))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Walks the area's X-sorted spatial index and returns the first object of the requested type inside
// a shape anchored at a script location, resuming from the area's cursor on the next-object call.
// The nShape codes are NWScript's SHAPE_* constants, but the shapes do not line up with those names,
// so the case labels below stay numeric.
int CSWVirtualMachineCommands::ExecuteCommandGetObjectInShape(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nShape;
    int nObjectFilter;
    int bLineOfSight;
    int nIndex;
    float fSize;
    Vector vTarget;
    Vector vOrigin;
    Vector vPos;
    Vector vProj;
    CScriptLocation* pLocation = 0;
    CSWSObject* pObject = 0;

    unsigned long long oidResult;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (!g_pVirtualMachine->StackPopInteger(&nShape) ||
        !g_pVirtualMachine->StackPopFloat(&fSize) ||
        !g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)) ||
        !g_pVirtualMachine->StackPopInteger(&bLineOfSight) ||
        !g_pVirtualMachine->StackPopInteger(&nObjectFilter) ||
        !g_pVirtualMachine->StackPopVector(&vOrigin)) {
        if (pLocation != 0) {
            delete pLocation;
            pLocation = 0;
        }
        return VM_ERROR_POP_FAILED;
    }

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServer->GetPlayerCreature();
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
    if (pPlayer != 0 && pArea != 0) {
        vTarget = pLocation->m_vPosition;
        if (nShape == 3 || nShape == 0) {
            if (pObjectArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS)
                vOrigin = pObject->m_position;
        }
        if (pLocation != 0) {
            delete pLocation;
            pLocation = 0;
        }

        float fMinX;
        float fMaxX;
        if ((nShape | 2) == 3) {
            fMinX = (vTarget.x < vOrigin.x) ? vTarget.x : vOrigin.x;
            fMaxX = (vTarget.x > vOrigin.x) ? vTarget.x : vOrigin.x;
            fMinX = fMinX - fSize;
            fMaxX = fSize + fMaxX;
        } else if (nShape == 0) {
            Vector vDir = normalize(vTarget - vOrigin);
            if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z <= 0.0001f)
                vDir = Vector(0.0f, 1.0f, 0.0f);
            Vector vPerp = Vector(-vDir.y, vDir.x, vDir.z) * 1.5f;
            vTarget = vOrigin + vDir * fSize;
            Vector vCorners[4];
            vCorners[0] = vOrigin + vPerp;
            vCorners[1] = vOrigin - vPerp;
            vCorners[2] = vTarget + vPerp;
            vCorners[3] = vTarget - vPerp;
            fMinX = vCorners[0].x;
            fMaxX = vCorners[0].x;
            for (int i = 1; i < 4; i++) {
                if (vCorners[i].x < fMinX)
                    fMinX = vCorners[i].x;
                if (vCorners[i].x > fMaxX)
                    fMaxX = vCorners[i].x;
            }
        } else {
            fMinX = vTarget.x - fSize;
            fMaxX = vTarget.x + fSize;
        }

        if (nCommand == CMD_GetFirstObjectInShape) {
            if (!pArea->GetFirstObjectIndiceByX(&nIndex, fMinX))
                goto pushNotFound;
        } else {
            nIndex = pArea->m_nObjectsByXIndex + 1;
            if (nIndex == pArea->m_aObjectsByX.m_count)
                goto pushNotFound;
        }

        float fSizeSq = fSize * fSize;
        while (nIndex < pArea->m_aObjectsByX.m_count) {
            if (pObjectArray->GetGameObject(pArea->m_aObjectsByX.m_pArray[nIndex], reinterpret_cast<CGameObject**>(&pObject))
                        == CGameObjectArray::SUCCESS &&
                pObject != 0 && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
                vPos = pObject->m_position;
            }
            // The index is X-sorted, so a candidate past the bracket ends the walk.
            if (vPos.x > fMaxX)
                goto pushNotFound;
            if (((nObjectFilter & OBJECT_TYPE_CREATURE) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) ||
                ((nObjectFilter & OBJECT_TYPE_ITEM) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_ITEM) ||
                ((nObjectFilter & OBJECT_TYPE_TRIGGER) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_TRIGGER) ||
                ((nObjectFilter & OBJECT_TYPE_DOOR) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_DOOR) ||
                ((nObjectFilter & OBJECT_TYPE_AREA_OF_EFFECT) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT) ||
                ((nObjectFilter & OBJECT_TYPE_WAYPOINT) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_WAYPOINT) ||
                ((nObjectFilter & OBJECT_TYPE_PLACEABLE) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE) ||
                ((nObjectFilter & OBJECT_TYPE_STORE) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_STORE) ||
                ((nObjectFilter & OBJECT_TYPE_ENCOUNTER) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER) ||
                ((nObjectFilter & OBJECT_TYPE_SOUND) != 0 && pObject->m_objectType == GAME_OBJECT_TYPE_SOUND) ||
                (nObjectFilter & OBJECT_TYPE_ALL) == OBJECT_TYPE_ALL) {
                switch (nShape) {
                case 0: {
                    Vector vFromOrigin = vPos - vOrigin;
                    if (vFromOrigin.x * vFromOrigin.x + vFromOrigin.y * vFromOrigin.y +
                            vFromOrigin.z * vFromOrigin.z <= fSizeSq) {
                        Vector vSpan = vTarget - vOrigin;
                        Vector vOffset = vPos - vOrigin;
                        Vector vAxis = normalize(vSpan);
                        Vector vToPos = normalize(vOffset);
                        if (vAxis.x * vToPos.x + vAxis.y * vToPos.y + vAxis.z * vToPos.z >= 0.0f) {
                            vProj = lineproject(vOrigin, vTarget, vPos);
                            Vector vOff = vPos - vProj;
                            if (vOff.x * vOff.x + vOff.y * vOff.y + vOff.z * vOff.z <= 2.25f)
                                goto pushFound;
                        }
                    }
                    break;
                }
                case 1: {
                    vProj = lineproject(vOrigin, vTarget, vPos);
                    Vector vSpan = vTarget - vOrigin;
                    Vector vOffset = vPos - vOrigin;
                    Vector vAxis = normalize(vSpan);
                    Vector vToPos = normalize(vOffset);
                    if (vAxis.x * vToPos.x + vAxis.y * vToPos.y + vAxis.z * vToPos.z > 0.0f) {
                        Vector vOff = vPos - vProj;
                        float fDistSq = vOff.x * vOff.x + vOff.y * vOff.y + vOff.z * vOff.z;
                        Vector vSpanAgain = vTarget - vOrigin;
                        float fRadius = fSize / magnitude(vSpanAgain) * magnitude(vProj);
                        if (fDistSq <= fRadius * fRadius)
                            goto pushFound;
                    }
                    break;
                }
                case 2:
                    if (vPos.x <= vTarget.x + fSize && vPos.x >= vTarget.x - fSize &&
                        vPos.y <= fSize + vTarget.y && vPos.y >= vTarget.y - fSize &&
                        vPos.z <= fSize + vTarget.z && vPos.z >= vTarget.z - fSize)
                        goto pushFound;
                    break;
                case 3: {
                    Vector vHit = lineproject(vOrigin, vTarget, vPos);
                    Vector vSpan = vTarget - vOrigin;
                    Vector vOffset = vPos - vOrigin;
                    Vector vAxis = normalize(vSpan);
                    Vector vToPos = normalize(vOffset);
                    if (vAxis.x * vToPos.x + vAxis.y * vToPos.y + vAxis.z * vToPos.z >= 0.866f) {
                        Vector vAlong = vHit - vOrigin;
                        if (vAlong.x * vAlong.x + vAlong.y * vAlong.y + vAlong.z * vAlong.z
                                <= fSizeSq)
                            goto pushFound;
                    }
                    break;
                }
                default: {
                    Vector vOff = vPos - vTarget;
                    if (vOff.x * vOff.x + vOff.y * vOff.y + vOff.z * vOff.z <= fSizeSq)
                        goto pushFound;
                }
                }
            }
            nIndex++;
        }
    } else {
        if (pLocation != 0) {
            delete pLocation;
            pLocation = 0;
        }
    }
    goto pushNotFound;

pushFound:
    pArea->m_nObjectsByXIndex = nIndex;
    oidResult = pObject->m_objectId;
    goto push;

pushNotFound:
    oidResult = CGameObjectArray::INVALIDOBJID;

push:
    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
#if KOTOR1
    unsigned long long oidPC = CGameObjectArray::INVALIDOBJID;
    if (nCommand == CMD_GetFirstPC)
        oidPC = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    else if (nCommand == CMD_GetNextPC)
        oidPC = g_pAppManager->m_pServerExoApp->GetNextPCObject();
    if (!g_pVirtualMachine->StackPushObject(oidPC))
        return VM_ERROR_PUSH_FAILED;
    return 0;
#else
    unsigned long long oidPC = OBJECT_INVALID;
    if (nCommand == CMD_GetFirstPC) {
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nCurrentPCIndex = 0;
        g_pAppManager->m_pClientExoApp->GetSWParty();
        oidPC = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    } else if (nCommand == CMD_GetNextPC) {
        oidPC = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNextPC();
    }
    if (!g_pVirtualMachine->StackPushObject(oidPC))
        return VM_ERROR_PUSH_FAILED;
    return 0;
#endif
}

int CSWVirtualMachineCommands::ExecuteCommandGetFortitudeSavingThrow(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nValue = 0;
    if (pObject != 0) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature != 0 && pCreature->m_pStats != 0)
                nValue = pCreature->m_pStats->GetFortSavingThrow(0);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            if (pDoor != 0)
                nValue = pDoor->m_fortSave;
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            if (pPlaceable != 0)
                nValue = pPlaceable->m_fortSave;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFoundEnemyCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidEnemy = OBJECT_INVALID;
    if (pCreature != 0)
        oidEnemy = pCreature->m_pPathfindInfo->m_oidFoundEnemy;
    if (!g_pVirtualMachine->StackPushObject(oidEnemy))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Reads CSWSObject::m_lastHostileActorId after re-validating it: the remembered id must still
// resolve to a living creature (an area-of-effect object resolves to its creator) and, for a
// party member, must still have hit points. A plot object never keeps a hostile actor. Anything
// else clears the slot and pushes OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetLastHostileActor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidActor = OBJECT_INVALID;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    CSWSObject* pSelf = 0;
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        pSelf = pObject->AsSWSObject();
    if (pSelf != 0) {
        oidActor = pSelf->m_lastHostileActorId;
        int bStillValid = 0;
        if (pSelf->m_bPlot == 1) {
            if (oidActor == OBJECT_INVALID)
                bStillValid = 1;
        } else {
            CGameObject* pActor = g_pAppManager->m_pServerExoApp->GetGameObject(oidActor);
            if (pActor != 0) {
                unsigned char nType = pActor->m_objectType;
                if (nType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT)
                    oidActor = pActor->AsSWSAreaOfEffectObject()->m_oidCreator;
                if (nType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT
                        || nType == CGameObject::OBJECT_TYPE_CREATURE) {
                    CSWSCreature* pActorCreature =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidActor);
#if KOTOR1  // K1 uses GetIsPCDying() and GetDeadTemp()
                    if (pActorCreature != 0 && pActorCreature->GetDead() != 1 &&
                        pActorCreature->GetIsPCDying() != 1 && pActorCreature->GetDeadTemp() == 0)
                        bStillValid = 1;
#else
                    if (pActorCreature != 0 && pActorCreature->GetDead() != 1 &&
                        pActorCreature->GetDeadTemp() == 0)
                        bStillValid = 1;
#endif
                }
            }
        }
        if (!bStillValid) {
            oidActor = OBJECT_INVALID;
            pSelf->SetLastHostileActor(OBJECT_INVALID, 1);
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidActor))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Reads the pending-cast triple CSWSObject keeps, one command each for the cast class, the spell
// and the caster.
int CSWVirtualMachineCommands::ExecuteCommandGetSpellCast(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nResult = g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf,
                                                                                 &pObject);
    unsigned char nSuccess = CGameObjectArray::SUCCESS;
    switch (nCommand) {
    case CMD_GetLastSpellCaster: {
        unsigned long long oidCaster = OBJECT_INVALID;
        if (nResult == nSuccess) {
            if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
                oidCaster = pObject->AsSWSCreature()->m_oidLastSpellCaster;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                oidCaster = pObject->AsSWSPlaceable()->m_oidLastSpellCaster;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                oidCaster = pObject->AsSWSDoor()->m_oidLastSpellCaster;
        }
        if (!g_pVirtualMachine->StackPushObject(oidCaster))
            return VM_ERROR_PUSH_FAILED;
        break;
    }
    case CMD_GetLastSpell: {
        int nSpellId = -1;
        if (nResult == nSuccess) {
            if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
                nSpellId = pObject->AsSWSCreature()->m_nLastSpellId;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                nSpellId = pObject->AsSWSPlaceable()->m_nLastSpellId;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                nSpellId = pObject->AsSWSDoor()->m_nLastSpellId;
        }
        if (!g_pVirtualMachine->StackPushInteger(nSpellId))
            return VM_ERROR_PUSH_FAILED;
        break;
    }
    case CMD_GetLastSpellHarmful: {
        int nCastClass = 0;
        if (nResult == nSuccess) {
            if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
                nCastClass = pObject->AsSWSCreature()->m_bLastSpellHarmful;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                nCastClass = pObject->AsSWSPlaceable()->m_bLastSpellHarmful;
            else if (pObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                nCastClass = pObject->AsSWSDoor()->m_bLastSpellHarmful;
        }
        if (!g_pVirtualMachine->StackPushInteger(nCastClass))
            return VM_ERROR_PUSH_FAILED;
        break;
    }
    }
    return 0;
}

// Handles the NWScript trap and mine query family, all twelve commands in one handler. The trap
// object is popped first, then dispatched on its engine type (trigger, placeable or door), each of
// which keeps the same GFF fields. A non-trap object answers 0, except GetTrapBaseType (-1),
// GetTrapCreator (OBJECT_INVALID) and GetTrapKeyTag ("").
int CSWVirtualMachineCommands::ExecuteCommandGetMineInfo(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidTrap;
    if (!g_pVirtualMachine->StackPopObject(&oidTrap))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTrap);
    int nValue = 0;
    unsigned long long oidDetectedBy;
    switch (nCommand) {
    case CMD_GetTrapDisarmable:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_bTrapDisarmable;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->m_bTrapDisarmable;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->m_bTrapDisarmable;
        }
        break;

    case CMD_GetTrapDetectable:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_bTrapDetectable;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->m_bTrapDetectable;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->m_bTrapDetectable;
        }
        break;

    case CMD_GetTrapDetectedBy: {  // Contains() returns the match count, which is the answer
        if (!g_pVirtualMachine->StackPopObject(&oidDetectedBy))
            return VM_ERROR_POP_FAILED;
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_objectsInTrigger.Contains(oidDetectedBy);
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->m_objectsOnPlaceable.Contains(oidDetectedBy);
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->m_objectsAtDoor.Contains(oidDetectedBy);
        }
        break;
    }

    case CMD_GetTrapFlagged:  // the trigger keeps the flag as an int, the other two as a byte
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_bMineFlagged;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = static_cast<unsigned char>(pObject->AsSWSPlaceable()->m_bMineFlagged);
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = static_cast<unsigned char>(pObject->AsSWSDoor()->m_bMineFlagged);
        }
        break;

    case CMD_GetTrapBaseType:  // the only command whose "not a trap" answer is -1
        nValue = -1;
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_trapType;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->m_trapType;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->m_trapType;
        }
        break;

    case CMD_GetTrapOneShot:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->m_bTrapOneShot;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->m_bTrapOneShot;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->m_bTrapOneShot;
        }
        break;

    case CMD_GetTrapCreator: {  // only the trigger records one
        unsigned long long oidCreator = OBJECT_INVALID;
        if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
            oidCreator = pObject->AsSWSTrigger()->m_creatorId;
        if (!g_pVirtualMachine->StackPushObject(oidCreator))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }

    case CMD_GetTrapKeyTag:
        if (pObject != 0) {
            const CExoString* psKeyTag;
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                psKeyTag = &pObject->AsSWSTrigger()->m_keyName;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                psKeyTag = &pObject->AsSWSPlaceable()->m_keyName;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                psKeyTag = &pObject->AsSWSDoor()->m_keyName;
            else
                goto pushNoKeyTag;
            if (!g_pVirtualMachine->StackPushString(*psKeyTag))
                return VM_ERROR_PUSH_FAILED;
            return 0;
        }
    pushNoKeyTag:
        {
            CExoString sNoKeyTag("");
            if (!g_pVirtualMachine->StackPushString(sNoKeyTag))
                return VM_ERROR_PUSH_FAILED;
        }
        return 0;

    case CMD_GetTrapDisarmDC:  // the trigger's accessor returns int, the other two a byte
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->GetDisarmDC();
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->GetDisarmDC();
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->GetDisarmDC();
        }
        break;

    case CMD_GetTrapDetectDC:
        if (pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
                nValue = pObject->AsSWSTrigger()->GetDetectDC();
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nValue = pObject->AsSWSPlaceable()->GetDetectDC();
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nValue = pObject->AsSWSDoor()->GetDetectDC();
        }
        break;

    case CMD_SetTrapDetectedBy: {  // returns whether the detector was recorded
        unsigned long long oidDetector;
        if (!g_pVirtualMachine->StackPopObject(&oidDetector))
            return VM_ERROR_POP_FAILED;
        CSWSCreature* pDetector =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidDetector);
        if (pObject != 0 && pDetector != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
                pObject->AsSWSTrigger()->m_objectsInTrigger.AddUnique(oidDetector);
                nValue = 1;
            } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                CExoArrayList<unsigned long long>* pList =
                    &pObject->AsSWSPlaceable()->m_objectsOnPlaceable;
                pList->AddUnique(oidDetector);
                nValue = 1;
            } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
                pObject->AsSWSDoor()->m_objectsAtDoor.AddUnique(oidDetector);
                nValue = 1;
            }
        }
        break;
    }

    default:
        // Spring the trap by signalling the OnDisarm/OnTrapTriggered event. Only the trigger also
        // queues its own destruction.
        if (nCommand == CMD_SetTrapDisabled && pObject != 0) {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
                if (pObject->AsSWSTrigger()->m_bTrap == 1) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 0, m_oidSelf, pObject->m_objectId,
                            CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 1, m_oidSelf, pObject->m_objectId,
                            CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
                }
            } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                if (pObject->AsSWSPlaceable()->m_bTrapFlag == 1) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 0, m_oidSelf, pObject->m_objectId,
                            CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                }
            } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
                if (pObject->AsSWSDoor()->m_bTrapFlag == 1) {
                    CScriptEvent* pEvent = new CScriptEvent;
                    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 0, m_oidSelf, pObject->m_objectId,
                            CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                }
            }
        }
        return 0;
    }

    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The nearest-creature and nearest-object queries behind five NWScript entry points. All five seed
// themselves on the area's X-sorted index at the source position and sweep outward in both
// directions, insertion-sorting every candidate that passes the per-command filter into a
// distance/object-id pair of lists, then answer the nNth entry. A side is finished once its dx alone
// exceeds the nNth-best distance.

int CSWVirtualMachineCommands::ExecuteCommandGetNearestObject(int nCommand, int nArgCount)
{
    CExoString sTagRaw;
    CSWSObject* pCandidate;
    int nFirstCriteriaType K2_UNSET;
    int nFirstCriteriaValue K2_UNSET;
    int nNth;
    int nObjectFilter K2_UNSET;
    int nSecondCriteriaType = -1;
    int nSecondCriteriaValue = -1;
    int nThirdCriteriaType = -1;
    int nThirdCriteriaValue = -1;
    unsigned long long oidTarget K2_UNSET;
#ifdef K2_FIX_ENGINE_BUGS
    CSWSObject* pTarget = 0;
#else
    CSWSObject* pTarget;
#endif
    int nIndex;

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CScriptLocation* pLocation = 0;
    CExoString sTag;

    switch (nCommand) {
    case CMD_GetNearestCreature:
    case CMD_GetNearestCreatureToLocation:
        if (!g_pVirtualMachine->StackPopInteger(&nFirstCriteriaType) ||
            !g_pVirtualMachine->StackPopInteger(&nFirstCriteriaValue))
            return VM_ERROR_POP_FAILED;
        break;
    case CMD_GetNearestObject:
    case CMD_GetNearestObjectToLocation:
        if (!g_pVirtualMachine->StackPopInteger(&nObjectFilter))
            return VM_ERROR_POP_FAILED;
        break;
    case CMD_GetNearestObjectByTag:
        if (!g_pVirtualMachine->StackPopString(&sTagRaw))
            return VM_ERROR_POP_FAILED;
        sTag = sTagRaw.LowerCase();
        break;
    }

    switch (nCommand) {
    case CMD_GetNearestCreature:
    case CMD_GetNearestObject:
    case CMD_GetNearestObjectByTag:
        if (!g_pVirtualMachine->StackPopObject(&oidTarget))
            return VM_ERROR_POP_FAILED;
        break;
    case CMD_GetNearestCreatureToLocation:
    case CMD_GetNearestObjectToLocation:
        if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
            return VM_ERROR_POP_FAILED;
        break;
    }

    if (!g_pVirtualMachine->StackPopInteger(&nNth))
        return VM_ERROR_POP_FAILED;

    if (nArgCount >= 5 && (nCommand == CMD_GetNearestCreature || nCommand == CMD_GetNearestCreatureToLocation)) {
        if (!g_pVirtualMachine->StackPopInteger(&nSecondCriteriaType) ||
            !g_pVirtualMachine->StackPopInteger(&nSecondCriteriaValue) ||
            !g_pVirtualMachine->StackPopInteger(&nThirdCriteriaType) ||
            !g_pVirtualMachine->StackPopInteger(&nThirdCriteriaValue))
            return VM_ERROR_POP_FAILED;
    }

    unsigned long long oidResult = OBJECT_INVALID;
    Vector vSource;
    CSWSArea* pArea;

    if (nCommand == CMD_GetNearestCreature || nCommand == CMD_GetNearestObjectByTag || nCommand == CMD_GetNearestObject) {
        if (pObjectArray->GetGameObject(oidTarget, reinterpret_cast<CGameObject**>(&pTarget))
                != CGameObjectArray::SUCCESS)
            goto push;
        if (pTarget->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
            goto push;
        vSource = pTarget->m_position;
        pArea = static_cast<CSWSArea*>(pTarget->GetArea());
        if (pArea == 0)
            goto push;
        if (!pArea->GetFirstObjectIndiceByX(&nIndex, vSource.x))
            goto push;
        while (nIndex < pArea->m_aObjectsByX.m_count && pArea->m_aObjectsByX.m_pArray[nIndex] != oidTarget)
            nIndex++;
        if (nIndex >= pArea->m_aObjectsByX.m_count || pArea->m_aObjectsByX.m_pArray[nIndex] != oidTarget)
            goto push;
    } else {
        vSource = pLocation->m_vPosition;
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pPlayer = pServer->GetPlayerCreature();
        if (pPlayer == 0)
            goto push;
        pArea = static_cast<CSWSArea*>(pPlayer->GetArea());
        if (pArea == 0)
            goto push;
        if (!pArea->GetFirstObjectIndiceByX(&nIndex, vSource.x))
            goto push;
    }

    {
        CExoArrayList<float> alDistances;
        CExoArrayList<unsigned long long> alObjects;
        int nOffset = 1;
        int bDonePositive = 0;
        int bDoneNegative = 0;

        while (!bDoneNegative || !bDonePositive) {
            int bMatch;
            int nScan = nIndex + nOffset;
            if (nScan < 0) {
                nOffset = abs(nOffset);
                bDoneNegative = 1;
                continue;
            }
            if (nScan >= pArea->m_aObjectsByX.m_count) {
                nOffset = -abs(nOffset);
                bDonePositive = 1;
                continue;
            }
            {
                unsigned long long oidCandidate = pArea->m_aObjectsByX.m_pArray[nScan];
                // Skip already-collected ids, since the two directions can meet, and the index can
                // duplicate an id while it is re-sorted.
                if (alObjects.Contains(oidCandidate))
                    goto advance;
                if (pObjectArray->GetGameObject(oidCandidate, reinterpret_cast<CGameObject**>(&pCandidate))
                        != CGameObjectArray::SUCCESS)
                    goto advance;

                switch (nCommand) {
                case CMD_GetNearestObject:
                case CMD_GetNearestObjectToLocation: {
                    bMatch = (pCandidate->m_objectType == GAME_OBJECT_TYPE_CREATURE && (nObjectFilter & OBJECT_TYPE_CREATURE) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_ITEM && (nObjectFilter & OBJECT_TYPE_ITEM) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_TRIGGER && (nObjectFilter & OBJECT_TYPE_TRIGGER) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_DOOR && (nObjectFilter & OBJECT_TYPE_DOOR) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT && (nObjectFilter & OBJECT_TYPE_AREA_OF_EFFECT) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_WAYPOINT && (nObjectFilter & OBJECT_TYPE_WAYPOINT) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_PLACEABLE && (nObjectFilter & OBJECT_TYPE_PLACEABLE) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_STORE && (nObjectFilter & OBJECT_TYPE_STORE) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER && (nObjectFilter & OBJECT_TYPE_ENCOUNTER) != 0) ||
                             (pCandidate->m_objectType == GAME_OBJECT_TYPE_SOUND && (nObjectFilter & OBJECT_TYPE_SOUND) != 0) ||
                             (nObjectFilter & OBJECT_TYPE_ALL) == OBJECT_TYPE_ALL;
                    break;
                }
                case CMD_GetNearestCreature:
                case CMD_GetNearestCreatureToLocation: {
                    CSWSCreature* pCreature;
                    if (pCandidate->AsSWSCreature() == 0)
                        goto advance;
                    pCreature = pCandidate->AsSWSCreature();
                    if (pCreature == 0)
                        goto advance;

                    bMatch = 0;
                    int nCriteriaType = nFirstCriteriaType;
                    int nCriteriaValue = nFirstCriteriaValue;
                    for (int nCriteria = 0; nCriteria < 3; nCriteria++) {
                        if (nCriteria == 1) {
                            nCriteriaType = nSecondCriteriaType;
                            nCriteriaValue = nSecondCriteriaValue;
                        } else if (nCriteria == 2) {
                            nCriteriaType = nThirdCriteriaType;
                            nCriteriaValue = nThirdCriteriaValue;
                        }
                        if (nCriteriaType == -1 || nCriteriaValue == -1)
                            continue;
                        switch (nCriteriaType) {
                        case CREATURE_TYPE_RACIAL_TYPE:
                            // RACIAL_TYPE_ALL is the row count and RACIAL_TYPE_INVALID is one
                            // past it. Both sentinels test the first criteria value rather than the
                            // loop's current one.
                            if (nFirstCriteriaValue == g_pRules->m_nNumRaces)
                                bMatch = 1;
                            else if (nFirstCriteriaValue == g_pRules->m_nNumRaces + 1)
                                bMatch = 0;
                            else
                                bMatch = (nCriteriaValue == pCreature->m_pStats->m_nRace);
                            break;
                        case CREATURE_TYPE_PLAYER_CHAR:
                            bMatch = (pCreature->m_pStats->m_bIsPC == nCriteriaValue);
                            break;
                        case CREATURE_TYPE_CLASS:
                            // For the class criterion, the first class goes through GetClass(),
                            // which answers its no-class sentinel for an empty slot; the second and
                            // third are read directly and simply fail when the creature has fewer
                            // classes.
#if KOTOR1  // K1 reads the second and third classes through GetClass too
                            bMatch = (nCriteriaValue == pCreature->m_pStats->GetClass(0)) ||
                                     (pCreature->m_pStats->m_numClasses >= 2 &&
                                      nCriteriaValue == pCreature->m_pStats->GetClass(1)) ||
                                     (pCreature->m_pStats->m_numClasses >= 3 &&
                                      nCriteriaValue == pCreature->m_pStats->GetClass(2));
#else
                            bMatch = (nCriteriaValue == pCreature->m_pStats->GetClass(0)) ||
                                     (pCreature->m_pStats->m_numClasses >= 2 &&
                                      nCriteriaValue == pCreature->m_pStats->m_classInfo[1].m_class)
#ifndef K2_FIX_ENGINE_BUGS
                                     || (pCreature->m_pStats->m_numClasses >= 3 &&
                                      nCriteriaValue == pCreature->m_pStats->m_classInfo[2].m_class)
#endif
                                     ;
#endif
                            break;
                        case CREATURE_TYPE_REPUTATION: {
                            // The reputation criterion is friend, enemy or neutral.
                            int nReputation;
#ifdef K2_FIX_ENGINE_BUGS
                            if (!pTarget)
                                break;
#endif
                            if (nCriteriaValue == REPUTATION_TYPE_FRIEND)
                                bMatch = (pCreature->AsSWSObject()->GetReputation(
                                              pTarget->m_objectId, 0) > REPUTATION_FRIENDLY_ABOVE);
                            else if (nCriteriaValue == REPUTATION_TYPE_ENEMY)
                                bMatch = (pCreature->AsSWSObject()->GetReputation(
                                              pTarget->m_objectId, 0) < REPUTATION_HOSTILE_BELOW);
                            else if (nCriteriaValue == REPUTATION_TYPE_NEUTRAL) {
                                nReputation = pCreature->AsSWSObject()->GetReputation(pTarget->m_objectId, 0);
                                bMatch = (nReputation >= REPUTATION_HOSTILE_BELOW
                                          && nReputation <= REPUTATION_FRIENDLY_ABOVE);
                            }
                            break;
                        }
                        case CREATURE_TYPE_IS_ALIVE:
                            // No test of its own, the GetDead() and hit-point gate below already
                            // covers it.
                            break;
                        case CREATURE_TYPE_HAS_SPELL_EFFECT:
                            bMatch = pCreature->HasSpellEffectApplied(nCriteriaValue);
                            break;
                        case CREATURE_TYPE_DOES_NOT_HAVE_SPELL_EFFECT:
                            bMatch = (pCreature->HasSpellEffectApplied(nCriteriaValue) == 0);
                            break;
                        case CREATURE_TYPE_PERCEPTION:
                            // Each PERCEPTION_* constant is a (want-seen, want-heard) pair where 2
                            // means "don't care"; the candidate's state is bit 0 (seen) and bit 1
                            // (heard) of the target's visibility entry.
                            if (pTarget->AsSWSCreature() != 0) {
                                int nWantSeen;
                                int nWantHeard;
                                switch (nCriteriaValue) {
                                case PERCEPTION_SEEN_AND_HEARD: nWantSeen = 1; nWantHeard = 1; break;
                                case PERCEPTION_NOT_SEEN_AND_NOT_HEARD: nWantSeen = 0; nWantHeard = 0; break;
                                case PERCEPTION_HEARD_AND_NOT_SEEN: nWantSeen = 0; nWantHeard = 1; break;
                                case PERCEPTION_SEEN_AND_NOT_HEARD: nWantSeen = 1; nWantHeard = 0; break;
                                case PERCEPTION_NOT_HEARD: nWantSeen = 2; nWantHeard = 0; break;
                                case PERCEPTION_HEARD: nWantSeen = 2; nWantHeard = 1; break;
                                case PERCEPTION_NOT_SEEN: nWantSeen = 0; nWantHeard = 2; break;
                                case PERCEPTION_SEEN: nWantSeen = 1; nWantHeard = 2; break;
                                default: nWantSeen = 2; nWantHeard = 2; break;
                                }
                                CSWVisibilityNode* pNode =
                                    pTarget->AsSWSCreature()->GetVisibleListElement(
                                        pCreature->m_objectId);
                                int nSeen = 0;
                                int nHeard = 0;
                                if (pNode != 0) {
                                    nSeen = pNode->m_flags & 1;
                                    nHeard = (pNode->m_flags >> 1) & 1;
                                }
                                bMatch = (nWantSeen == 2 || nSeen == nWantSeen) &&
                                         (nWantHeard == 2 || nHeard == nWantHeard);
                            }
                            break;
                        default:
                            bMatch = 0;
                            break;
                        }
                        if (!bMatch)
                            break;
                    }

                    if (pCreature->GetDead() == 1)
                        goto advance;
#if KOTOR1  // K1 uses GetIsPCDying() and GetDeadTemp()
                    if (pCreature->GetIsPCDying() == 1 || pCreature->GetDeadTemp() == 1)
#else
                    if (pCreature->GetDeadTemp() == 1)
#endif
                        goto advance;
                    break;
                }
                case CMD_GetNearestObjectByTag:
                    bMatch = pCandidate->m_sTag == sTag;
                    break;
                default:
                    goto advance;
                }

                if (!bMatch)
                    goto advance;

                {
                    float fdx = pCandidate->m_position.x - vSource.x;
                    float fdy = pCandidate->m_position.y - vSource.y;
                    float fdz = pCandidate->m_position.z - vSource.z;
                    // The index is X-sorted, so once dx**2 alone beats the worst kept distance,
                    // this side of the seed is finished.
                    if (alDistances.m_count >= nNth &&
                        fdx * fdx > alDistances.m_pArray[nNth - 1]) {
                        if (nOffset > 0)
                            bDonePositive = 1;
                        else
                            bDoneNegative = 1;
                        nOffset = -nOffset;
                    }
                    float fDistSq = fdx * fdx + fdy * fdy + fdz * fdz;
                    int nInsert = 0;
                    while (nInsert < alDistances.m_count &&
                           fDistSq >= alDistances.m_pArray[nInsert])
                        nInsert++;
                    alDistances.Insert(fDistSq, nInsert);
                    alObjects.Insert(oidCandidate, nInsert);
                }
            }
        advance:
            // Alternate sides while both directions are still live.
            if (!bDonePositive && !bDoneNegative && abs(nOffset) % 2 == (nOffset > 0))
                nOffset = -nOffset;
            else
                nOffset += nOffset / abs(nOffset);
        }

        if (nNth <= alObjects.m_count)
            oidResult = alObjects.m_pArray[nNth - 1];
    }

push:
    // Kept for its side effects only; the probe's result is discarded.
    CSWSCreature* pNearest = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidResult);
    if (nCommand != CMD_GetNearestObjectByTag && pNearest != 0 && pNearest->GetDead() == 0)
        pNearest->GetDeadTemp();

    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    if (pLocation != 0) {
        delete pLocation;
    }
    return 0;
}

// The reference object plus a "must already be detected by it" flag, then a linear scan of the
// area's mine list for the closest hit.
int CSWVirtualMachineCommands::ExecuteCommandGetNearestMine(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    int bTrapDetected;
    float fBest;

    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bTrapDetected))
        return VM_ERROR_POP_FAILED;

    unsigned long long oidResult = OBJECT_INVALID;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSelf = pObject->AsSWSObject();
        if (pSelf->GetArea() != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSObject()->GetArea());
            if (pArea->m_mineObjects.m_count > 0) {
                fBest = 100000000.0f;
                for (int64_t i = 0; i < pArea->m_mineObjects.m_count; i++) {
                    CGameObject* pMine =
                        g_pAppManager->m_pServerExoApp->GetGameObject(pArea->m_mineObjects.m_pArray[i]);
                    if (pMine == 0)
                        continue;
                    if (bTrapDetected == 1) {
                        // Contains() returns the match count; zero means this object has not
                        // spotted the trap.
                        if (pMine->AsSWSTrigger() != 0) {
                            if (pMine->AsSWSTrigger()->m_objectsInTrigger.Contains(oidTarget) == 0)
                                continue;
                        } else if (pMine->AsSWSDoor() != 0) {
                            if (pMine->AsSWSDoor()->m_objectsAtDoor.Contains(oidTarget) == 0)
                                continue;
                        } else if (pMine->AsSWSPlaceable() != 0) {
                            if (pMine->AsSWSPlaceable()->m_objectsOnPlaceable.Contains(oidTarget) == 0)
                                continue;
                        } else {
                            continue;
                        }
                    }
                    CSWSObject* pMineObject = pMine->AsSWSObject();
                    float fdx = pSelf->m_position.x - pMineObject->m_position.x;
                    float fdy = pSelf->m_position.y - pMineObject->m_position.y;
                    float fdz = pSelf->m_position.z - pMineObject->m_position.z;
                    float fDistSq = fdx * fdx + fdy * fdy + fdz * fdz;
                    if (fDistSq < fBest) {
                        oidResult = pArea->m_mineObjects.m_pArray[i];
                        fBest = fDistSq;
                    }
                }
            }
        }
    }

    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The four module item-event reads (acquired, acquired-from, lost, lost-by):
// four one-field reads off the module's last-item-event record.
int CSWVirtualMachineCommands::ExecuteCommandGetModuleItemStuff(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidResult = OBJECT_INVALID;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        switch (nCommand) {
        case CMD_GetModuleItemAcquired:
            oidResult = pModule->m_oidAcquiredItem;
            break;
        case CMD_GetModuleItemAcquiredFrom:
            oidResult = pModule->m_oidItemAcquirer;
            break;
        case CMD_GetModuleItemLost:
            oidResult = pModule->m_oidUnAcquiredItem;
            break;
        case CMD_GetModuleItemLostBy:
            oidResult = pModule->m_oidItemUnAcquirer;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetMusicDayTrack (NWScript): the area's ambient-sound record carries the two music-track rows.
int CSWVirtualMachineCommands::ExecuteCommandGetDayTrack(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidArea;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;

    int nTrack = 0;
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea != 0)
        nTrack = pArea->m_pAmbientSound->m_nMusicDayTrack;

    if (!g_pVirtualMachine->StackPushInteger(nTrack))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetMusicNightTrack (NWScript): the night twin of GetDayTrack, one field over.
int CSWVirtualMachineCommands::ExecuteCommandGetNightTrack(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidArea;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;

    int nTrack = 0;
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea != 0)
        nTrack = pArea->m_pAmbientSound->m_nMusicNightTrack;

    if (!g_pVirtualMachine->StackPushInteger(nTrack))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetCreatureMovementRate (NWScript): the creaturespeed.2da row off the creature's stats block.
int CSWVirtualMachineCommands::ExecuteCommandGetMovementRate(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;

    int nRate = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0 && pCreature->m_pStats != 0)
        nRate = pCreature->m_pStats->m_nMovementRate;

    if (!g_pVirtualMachine->StackPushInteger(nRate))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Sums OBJECT_SELF's last-damage tally across all fifteen damage flags. A missing object
// pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandGetTotalDamageDealt(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nTotal = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (pObject != 0) {
        if (pObject->AsSWSObject() != 0) {
            CSWSObject* pSelf = pObject->AsSWSObject();
            if (pSelf != 0) {
                for (int i = 0; i < 15; i++) {
                    if (pSelf->GetLastDamageAmountByFlags(1 << i) >= 1)
                        nTotal += pSelf->GetLastDamageAmountByFlags(1 << i);
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nTotal))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The object's display name, assembled server-side from its first and last CExoLocStrings for
// the no-particular-player language.
int CSWVirtualMachineCommands::ExecuteCommandGetName(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    unsigned long long oidObject;
    CSWSObject* pObject;

    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray->GetGameObject(oidObject, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS &&
        pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        CSWSMessage::GetLocStringServer(MSG_PLAYERID_ALL_CLIENTS, pObject->AsSWSObject()->GetFirstName(),
                                        pObject->AsSWSObject()->GetLastName(), sName, 0);
    }

    if (!g_pVirtualMachine->StackPushString(sName))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Handles the two module-name commands. One pushes the running module's name, the other the
// save-file tag, which is the part of m_sModuleResourceName after its colon.
int CSWVirtualMachineCommands::ExecuteCommandGetModuleName(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sName;

#if KOTOR1  // K1 reports the module's localized name
    switch (nCommand) {
    case CMD_GetModuleName: {
        CExoLocString lsName(g_pAppManager->m_pServerExoApp->GetModule()->m_sName);
        lsName.GetStringLoc(g_pAppManager->m_pServerExoApp->GetModuleLanguage(), &sName, 0);
        break;
    }
    case CMD_GetModuleFileName: {
        sName = g_pAppManager->m_pServerExoApp->GetModule()->m_sModuleResourceName;
        int nColon = sName.Find(':', 0);
        if (nColon != -1)
            sName = sName.Right(sName.GetLength() - nColon - 1);
        break;
    }
    }
#else
    if (nCommand == CMD_GetModuleName) {
        sName = g_pAppManager->m_pServerExoApp->GetModuleName();
    } else if (nCommand == CMD_GetModuleFileName) {
        sName = g_pAppManager->m_pServerExoApp->GetModule()->m_sModuleResourceName;
        int nColon = sName.Find(':', 0);
        if (nColon != -1)
            sName = sName.Right((sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0) - nColon - 1);
    }
#endif

    if (!g_pVirtualMachine->StackPushString(sName))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetTransitionTarget: the object a transition trigger or door leads to. A trigger resolves its
// "LinkedTo" tag through the module -- as a waypoint when LinkedToFlags == 2, else as a plain tag
// lookup -- while a door hands back its own cached linked object.
int CSWVirtualMachineCommands::ExecuteCommandGetWaypointByTrigger(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTrigger;
    CSWSObject* pObject;

    if (!g_pVirtualMachine->StackPopObject(&oidTrigger))
        return VM_ERROR_POP_FAILED;

    unsigned long long oidResult = OBJECT_INVALID;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray->GetGameObject(oidTrigger, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSTrigger() != 0) {
            CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
            unsigned char nLinkedToFlags = pObject->AsSWSTrigger()->m_linkedToFlags;
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            if (nLinkedToFlags == 2)
                oidResult = pModule->GetWaypoint(pTrigger->m_linkedTo);
            else
                oidResult = pModule->FindObjectByTagOrdinal(pTrigger->m_linkedTo, 0);
        } else if (pObject->AsSWSDoor() != 0) {
            CSWSObject* pLinked = pObject->AsSWSDoor()->GetLinkedObject();
            if (pLinked != 0)
                oidResult = pLinked->m_objectId;
        }
    }

    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetNumStackedItems (NWScript): the item's stack size, or -1 when the id is not an item.
int CSWVirtualMachineCommands::ExecuteCommandGetNumStackedItems(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;

    int nCount = -1;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0)
        nCount = pItem->m_stackCount;

    if (!g_pVirtualMachine->StackPushInteger(nCount))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetNPCAIStyle (NWScript): the creature's current AI-state id off its stats block.
int CSWVirtualMachineCommands::ExecuteCommandGetNPCAIStyle(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    CSWSObject* pObject;

    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;

    int nAIStyle = 0;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray->GetGameObject(oidCreature, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS &&
        pObject->AsSWSCreature() != 0)
        nAIStyle = pObject->AsSWSCreature()->m_pStats->m_aiState;

    if (!g_pVirtualMachine->StackPushInteger(nAIStyle))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Two party-table queries over the same popped NPC index, whether the creature is available and the
// party-member selectability check. The second maps the "no such slot" sentinel to a signed -1.
int CSWVirtualMachineCommands::ExecuteCommandIsAvailableNPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nNPC;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;

    int nValue = 0;
    if (nCommand == CMD_IsAvailableCreature) {
        nValue = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(nNPC);
    } else if (nCommand == CMD_GetNPCSelectability) {
        unsigned char nSelectability =
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCSelectability(nNPC);
#if KOTOR1  // K1 pushes the raw selectability byte
        nValue = nSelectability;
#else
        nValue = (nSelectability == 0xff) ? -1 : nSelectability;
#endif
    }

    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Maps the engine's object-type byte onto the script-visible OBJECT_TYPE_* bitmask. Types with no
// script equivalent (including 8 and 0xf) answer OBJECT_TYPE_ALL.
int CSWVirtualMachineCommands::ExecuteCommandGetObjectType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CSWSObject* pObject;

    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;

    int nType = 0;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray->GetGameObject(oidObject, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS &&
        pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE:         nType = OBJECT_TYPE_CREATURE;         break;
        case GAME_OBJECT_TYPE_ITEM:             nType = OBJECT_TYPE_ITEM;             break;
        case GAME_OBJECT_TYPE_TRIGGER:          nType = OBJECT_TYPE_TRIGGER;          break;
        case GAME_OBJECT_TYPE_PLACEABLE:        nType = OBJECT_TYPE_PLACEABLE;        break;
        case GAME_OBJECT_TYPE_DOOR:             nType = OBJECT_TYPE_DOOR;             break;
        case GAME_OBJECT_TYPE_AREA_OF_EFFECT:   nType = OBJECT_TYPE_AREA_OF_EFFECT;   break;
        case GAME_OBJECT_TYPE_WAYPOINT:         nType = OBJECT_TYPE_WAYPOINT;         break;
        case GAME_OBJECT_TYPE_ENCOUNTER:        nType = OBJECT_TYPE_ENCOUNTER;        break;
        case GAME_OBJECT_TYPE_STORE:            nType = OBJECT_TYPE_STORE;            break;
        case GAME_OBJECT_TYPE_SOUND:            nType = OBJECT_TYPE_SOUND;            break;
        default:                                nType = OBJECT_TYPE_ALL;              break;
        }
    }

    if (!g_pVirtualMachine->StackPushInteger(nType))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The nth object in the module carrying the given tag. The tag is lower-cased first because the
// module indexes tags in lower case.
int CSWVirtualMachineCommands::ExecuteCommandGetObjectByTag(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sTag;
    int nOrdinal;
    unsigned long long oidResult;

    if (!g_pVirtualMachine->StackPopString(&sTag))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nOrdinal))
        return VM_ERROR_POP_FAILED;

    oidResult = g_pAppManager->m_pServerExoApp->GetModule()->FindObjectByTagOrdinal(
        sTag.LowerCase(), nOrdinal);

    if (!g_pVirtualMachine->StackPushObject(oidResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Answers the seen and heard queries. Both read the same visibility-list entry the creature keeps
// for the target, answering with bit 0 (seen) or bit 1 (heard).
int CSWVirtualMachineCommands::ExecuteCommandGetObjectVisibility(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    unsigned long long oidSource;
    CSWSObject* pSource;

    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidSource))
        return VM_ERROR_POP_FAILED;

    int nSeen = 0;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (pObjectArray->GetGameObject(oidSource, reinterpret_cast<CGameObject**>(&pSource)) == CGameObjectArray::SUCCESS &&
        pSource->AsSWSCreature() != 0) {
        if (nCommand == CMD_GetObjectSeen) {
            CSWVisibilityNode* pNode =
                pSource->AsSWSCreature()->GetVisibleListElement(oidTarget);
            if (pNode != 0)
                nSeen = pNode->m_flags & 1;
        } else if (nCommand == CMD_GetObjectHeard) {
            CSWVisibilityNode* pNode =
                pSource->AsSWSCreature()->GetVisibleListElement(oidTarget);
            if (pNode != 0)
                nSeen = (pNode->m_flags >> 1) & 1;
        }
    }

    if (!g_pVirtualMachine->StackPushInteger(nSeen))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The demolitions skill of whoever set the trap, off whichever of the three trap carriers the
// object turns out to be (door / placeable / trigger).
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetOwnerDemolitionsSkill(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTrap;
    if (!g_pVirtualMachine->StackPopObject(&oidTrap))
        return VM_ERROR_POP_FAILED;

    int nSkill = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTrap);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSelf = pObject->AsSWSObject();
        if (pSelf != 0) {
#if !KOTOR1  // K2-only command; K1 doors/placeables have no OwnerDemolitionsSkill
            if (pSelf->AsSWSDoor() != 0)
                nSkill = pSelf->AsSWSDoor()->m_nOwnerDemolitionsSkill;
            else if (pSelf->AsSWSPlaceable() != 0)
                nSkill = pSelf->AsSWSPlaceable()->m_nOwnerDemolitionsSkill;
            else
#endif
            if (pSelf->AsSWSTrigger() != 0)
                nSkill = pSelf->AsSWSTrigger()->m_nOwnerDemolitionsSkill;
        }
    }

    if (!g_pVirtualMachine->StackPushInteger(nSkill))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// The influence trio (get, set and modify) works on the party-table influence score for one NPC
// slot. The writers clamp to [0,100] and report the direction to the HUD only when the value moved.
// ModifyInfluence seeds an unset slot (-1) to 50 first.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandInfluence(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nNPC;
    int nAmount = 0;

    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0) {
        if (nCommand == CMD_GetInfluence) {
            if (!g_pVirtualMachine->StackPushInteger(nAmount))
                return VM_ERROR_PUSH_FAILED;
        }
        return 0;
    }

    unsigned long long oidNPC = pPartyTable->GetNPCObject(nNPC, 0, 1);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidNPC);

    if (nCommand == CMD_GetInfluence) {
        nAmount = pPartyTable->GetInfluence(nNPC);
        if (!g_pVirtualMachine->StackPushInteger(nAmount))
            return VM_ERROR_PUSH_FAILED;
    } else if (nCommand == CMD_SetInfluence) {
        int nOld = pPartyTable->GetInfluence(nNPC);
        if (!g_pVirtualMachine->StackPopInteger(&nAmount))
            return VM_ERROR_POP_FAILED;
        if (nAmount < 0)
            nAmount = 0;
        else if (nAmount > 100)
            nAmount = 100;
        pPartyTable->SetInfluence(nNPC, nAmount);
        if (nOld != nAmount) {
            unsigned char nStatus = (nOld < nAmount) ? CGuiInGame::GUI_STATUS_INFLUENCE_GAINED
                                                     : CGuiInGame::GUI_STATUS_INFLUENCE_LOST;
            g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(nStatus, nNPC);
            if (pCreature != 0)
                pCreature->HandleAlignmentInfluence();
        }
    } else if (nCommand == CMD_ModifyInfluence) {
        int nOld = pPartyTable->GetInfluence(nNPC);
        if (!g_pVirtualMachine->StackPopInteger(&nAmount))
            return VM_ERROR_POP_FAILED;
        if (nOld == -1) {
            pPartyTable->SetInfluence(nNPC, 50);
            nOld = 50;
        }
        int nNew = nOld + nAmount;
        if (nNew >= 100)
            nNew = 100;
        if (nNew < 0)
            nNew = 0;
        pPartyTable->SetInfluence(nNPC, nNew);
        if (nOld != nNew) {
            unsigned char nStatus = (nAmount > 0) ? CGuiInGame::GUI_STATUS_INFLUENCE_GAINED
                                                   : CGuiInGame::GUI_STATUS_INFLUENCE_LOST;
            g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(nStatus, nNPC);
            if (pCreature != 0)
                pCreature->HandleAlignmentInfluence();
        }
    }
    return 0;
}
#endif

// The type code of the inventory event currently being reported to the script. CSWSObject
// stashes the (type, item) pair on the owner while the OnInventoryDisturbed handler runs.
int CSWVirtualMachineCommands::ExecuteCommandGetInventoryDisturbType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    int nType = 0;
    if (pObject != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0)
            nType = pSWSObject->m_nLastDisturbType;
    }
    if (!g_pVirtualMachine->StackPushInteger(nType))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetInventoryDisturbItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem = CGameObjectArray::INVALIDOBJID;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (pObject != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0)
            oidItem = pSWSObject->m_oidLastDisturbItem;
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The stealth-XP trio shares GetMaxStealthXP's body above, one area field apart.
int CSWVirtualMachineCommands::ExecuteCommandGetCurrentStealthXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nCurrentStealthXP = 0;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            nCurrentStealthXP = pArea->m_currentStealthXP;
    }
    if (!g_pVirtualMachine->StackPushInteger(nCurrentStealthXP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetStealthXPDecrement(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nDecrement = 0;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            nDecrement = pArea->m_stealthXPDecrement;
    }
    if (!g_pVirtualMachine->StackPushInteger(nDecrement))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetStealthXPEnabled(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nEnabled = 0;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            nEnabled = pArea->m_bStealthXPEnabled;
    }
    if (!g_pVirtualMachine->StackPushInteger(nEnabled))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetPartyLeader: party slot 0 on the client party roster, translated back to a
// server object id; an empty roster pushes OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetPartyLeader(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    unsigned long long oidLeader = OBJECT_INVALID;
    if (pLeader)
        oidLeader = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId);
    if (!g_pVirtualMachine->StackPushObject(oidLeader))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandFloatToInt(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fValue;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPushInteger(static_cast<int>(fValue)))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPlayMovieQueue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bAllowSkips;
    if (!g_pVirtualMachine->StackPopInteger(&bAllowSkips))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->PlayMovieQueue(bAllowSkips);
    return 0;
}

// EventConversation (NWScript): a bare event structure of type 7, handed straight to the stack.
int CSWVirtualMachineCommands::ExecuteCommandEventConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptEvent* pEvent = new CScriptEvent;
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE;
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EVENT, pEvent) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEvent;
    return 0;
}

// GetCheatCode (NWScript): the shipping build answers 0 for every code -- the argument is popped
// and dropped. Note the pop failure reports -2000, not the usual -2001.
int CSWVirtualMachineCommands::ExecuteCommandGetCheatCode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nCheatCode;
    if (!g_pVirtualMachine->StackPopInteger(&nCheatCode))
        return VM_ERROR_PUSH_FAILED;
#if KOTOR1  // K1: code 0 reports the "dancedancemalak" console flag
    int bActive = 0;
    if (nCheatCode == 0)
        bActive = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bDanceDanceMalak;
    if (!g_pVirtualMachine->StackPushInteger(bActive))
        return VM_ERROR_PUSH_FAILED;
#else
    if (!g_pVirtualMachine->StackPushInteger(0))
        return VM_ERROR_PUSH_FAILED;
#endif
    return 0;
}

// GetLastPazaakResult (NWScript): the client keeps the (result, opponent) pair from the last
// pazaak hand; only the first half reaches the script.
int CSWVirtualMachineCommands::ExecuteCommandGetLastPazaakResult(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nResult = 0;
    int nOpponent = 0;
    g_pAppManager->m_pClientExoApp->GetPazaakResults(&nResult, &nOpponent);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// N.B. the pop failure reports -2000 here, not the family's usual -2001.
int CSWVirtualMachineCommands::ExecuteCommandSuppressStatusSummaryEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nEntries;
    if (!g_pVirtualMachine->StackPopInteger(&nEntries))
        return VM_ERROR_PUSH_FAILED;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SuppressStatusSummary(nEntries);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetButtonMashCheck(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bCheck = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bCheck))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->SetButtonMashCheck(bCheck);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPauseGame(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bPause;
    if (!g_pVirtualMachine->StackPopInteger(&bPause))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->SetPausedByCombat(bPause, 0, 0);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectPlay(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        pSound->Play();
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectStop(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        pSound->Stop();
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetSoloMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bSolo;
    if (!g_pVirtualMachine->StackPopInteger(&bSolo))
        return VM_ERROR_PUSH_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetSoloMode(bSolo, 1);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetPartyLeader(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWParty* pParty = pClient->GetSWParty();
    int nNPC;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    int bResult;
    if (nNPC == -1) {
        // Hand the lead back to the player character, reviving it first if the switch would
        // otherwise leave a corpse in charge.
        for (int i = 0; i < pParty->m_nCharacterCount; i++) {
            CSWCCreature* pMember =
                g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(pParty->m_members[i].id);
            if (pMember && pMember->GetServerCreature()) {
                CSWSCreature* pCreature = pMember->GetServerCreature();
                if (pCreature->m_bInParty == 1 && pCreature->m_bPlayerCharacter == 1) {
                    if (pCreature->GetDead() == 1 || pCreature->GetDeadTemp() == 1) {
                        CGameEffect* pEffect = new CGameEffect(1);
                        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
                        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK)
                                                      | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                        pCreature->ApplyEffect(pEffect, 0, 0);
                        pCreature->m_bRaiseable = 1;
                    }
                }
            }
        }
        pParty->SetLeader(-3);
        bResult = 1;
    } else {
        unsigned long long oidNPC = pServer->GetPartyTable()->GetNPCObject(nNPC, 0, 1);
        unsigned long long oidMember = pClient->ServerToClientObjectId(oidNPC);
        bResult = 0;
        for (int i = 0; i < pParty->m_nCharacterCount; i++) {
            if (pParty->m_members[i].id == oidMember) {
                pParty->SetLeader(i);
                bResult = 1;
                break;
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(bResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandVectorMagnitude(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vValue;
    if (!g_pVirtualMachine->StackPopVector(&vValue))
        return VM_ERROR_POP_FAILED;
    float fMagnitude = magnitude(vValue);
    if (!g_pVirtualMachine->StackPushFloat(fMagnitude))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandDisableMap(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bDisable = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bDisable))
        return VM_ERROR_POP_FAILED;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    pPartyTable->m_disableMap = bDisable;
    return 0;
}
#endif

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandDisableHealthRegen(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bDisable = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bDisable))
        return VM_ERROR_POP_FAILED;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    pPartyTable->m_disableRegen = bDisable;
    return 0;
}
#endif

int CSWVirtualMachineCommands::ExecuteCommandSetPartyAIStyle(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStyle = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nStyle))
        return VM_ERROR_POP_FAILED;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    pPartyTable->m_aiState = nStyle;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetKeepStealthInDialog(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bKeep = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bKeep))
        return VM_ERROR_POP_FAILED;
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGui->m_bKeepStealthInDialog = bKeep;
    return 0;
}
#endif // !KOTOR1

// GetLastDamager / GetLastKiller: the same guarded double cast as GetCombatActionPending,
// one object-id field apart.
int CSWVirtualMachineCommands::ExecuteCommandGetLastDamager(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidDamager = OBJECT_INVALID;
#if KOTOR1  // K1 has no null test here
    pObject->AsSWSObject();
    {
#else
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
#endif
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0)
            oidDamager = pSWSObject->m_lastDamagerId;
    }
    if (!g_pVirtualMachine->StackPushObject(oidDamager))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastKiller(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidKiller = OBJECT_INVALID;
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0)
            oidKiller = pSWSObject->m_killerId;
    }
    if (!g_pVirtualMachine->StackPushObject(oidKiller))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetInputClass(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nClass = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nClass))
        return VM_ERROR_POP_FAILED;
    if (nClass >= 0 && nClass <= 5)
        g_pAppManager->m_pClientExoApp->SetInputClass(nClass, 1);
    return 0;
}

// PrintObject / PrintInteger: the shipping build pops the arguments and drops them; only the
// optional destination argument's arity handling is left.
int CSWVirtualMachineCommands::ExecuteCommandPrintObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidObject;
    int nDestination;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nDestination))
            return VM_ERROR_POP_FAILED;
    } else {
        nDestination = 0;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPrintInteger(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nValue;
    int nDestination;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nDestination))
            return VM_ERROR_POP_FAILED;
    } else {
        nDestination = 0;
    }
    return 0;
}

#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSetForfeitConditions(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nConditions = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nConditions))
        return VM_ERROR_POP_FAILED;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable)
        pPartyTable->m_forfeitConditions = nConditions;
    return 0;
}
#endif

int CSWVirtualMachineCommands::ExecuteCommandYavinHackCloseDoor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidDoor;
    if (!g_pVirtualMachine->StackPopObject(&oidDoor))
        return VM_ERROR_POP_FAILED;
    CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidDoor);
    if (pDoor)
        pDoor->SetOpenState(0, 0);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEnableVideoEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nEffect;
    if (!g_pVirtualMachine->StackPopInteger(&nEffect))
        return VM_ERROR_POP_FAILED;
#if !KOTOR1  // K1: no video-effect override flag
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGui->m_bScriptVideoEffect = 1;
#endif
    g_pAppManager->m_pClientExoApp->EnableVideoEffect(nEffect);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetNPCSelectability(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    int bSelectable;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bSelectable))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetNPCSelectability(nNPC, static_cast<unsigned char>(bSelectable));
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetMusicVolume(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fVolume;
    if (!g_pVirtualMachine->StackPopFloat(&fVolume))
        return VM_ERROR_PUSH_FAILED;
    fVolume = (fVolume < 0.0f) ? 0.0f : fVolume;
    fVolume = (fVolume < 1.0f) ? fVolume : 1.0f;
    g_pExoSound->SetMusicVolume(fVolume, 1);
    return 0;
}

// GetScriptStringParameter (NWScript): the string half of the SetScriptParams payload.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetScriptStringParameter(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sParam = g_pVirtualMachine->m_sScriptParam[0];
    if (!g_pVirtualMachine->StackPushString(sParam))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandSetGalaxyMapPlanetAvailable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPlanet;
    int bAvailable;
    if (!g_pVirtualMachine->StackPopInteger(&nPlanet))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bAvailable))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetPlanetAvailable(nPlanet, bAvailable);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetGalaxyMapPlanetSelectable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPlanet;
    int bSelectable;
    if (!g_pVirtualMachine->StackPopInteger(&nPlanet))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bSelectable))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetPlanetSelectable(nPlanet, bSelectable);
    return 0;
}

// Both gate on m_nScriptRunArg: the actions form runs for any nonzero value, the effects form
// only for exactly 1.
int CSWVirtualMachineCommands::ExecuteCommandClearAllActions(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSObject* pObject = 0;
    if (m_nScriptRunArg != 0) {
        CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
        if (pArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS)
            pObject->ClearAllActions(1);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandClearAllEffects(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSObject* pObject;
    if (m_nScriptRunArg == 1) {
        CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
        if (pArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS)
            pObject->ClearAllEffects(0);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetAvailableNPCID(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    unsigned long long oidNPC;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidNPC))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetNPCObject(nNPC, oidNPC);
    return 0;
}

// Same popped-and-dropped shape as PrintObject / PrintInteger.
int CSWVirtualMachineCommands::ExecuteCommandPrintVector(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    Vector vValue;
    int nDestination;
    if (!g_pVirtualMachine->StackPopVector(&vValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nDestination))
            return VM_ERROR_POP_FAILED;
    } else {
        nDestination = 0;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandAwardStealthXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            pArea->AwardStealthXP(pCreature->m_objectId);
    }
    return 0;
}

// Play/StopRumblePattern (NWScript): both forward the pattern id and push the app's own answer.
int CSWVirtualMachineCommands::ExecuteCommandPlayRumblePattern(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPattern;
    if (!g_pVirtualMachine->StackPopInteger(&nPattern))
        return VM_ERROR_POP_FAILED;
    int nResult = g_pAppManager->m_pClientExoApp->PlayRumblePattern(nPattern);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandStopRumblePattern(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPattern;
    if (!g_pVirtualMachine->StackPopInteger(&nPattern))
        return VM_ERROR_POP_FAILED;
    int nResult = g_pAppManager->m_pClientExoApp->StopRumblePattern(nPattern);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetDisableTransit(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bDisable = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bDisable))
        return VM_ERROR_POP_FAILED;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pModule->GetArea());
        if (pArea)
            pArea->m_disableTransit = bDisable;
    }
    return 0;
}
#endif

int CSWVirtualMachineCommands::ExecuteCommandSetPlaceableIllumination(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidPlaceable;
    int bLightOn;
    if (!g_pVirtualMachine->StackPopObject(&oidPlaceable))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bLightOn))
        return VM_ERROR_POP_FAILED;
    CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidPlaceable);
    if (pPlaceable)
        pPlaceable->SetLightIsOn(bLightOn);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectCrush(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
    pEffect->SetInteger(0, 15);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetVolume(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    int nVolume;  // uninitialised when the sound object is not found  // idiom-exempt: really uninitialised in the orig -- the missed-lookup arm
#ifdef K2_FIX_ENGINE_BUGS
    nVolume = 0;
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        nVolume = pSound->m_volume;
    if (!g_pVirtualMachine->StackPushInteger(nVolume))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetVolume(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    int nVolume;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nVolume))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        pSound->ChangeVolume(nVolume);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetFixedVariance(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    float fVariance;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fVariance))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        pSound->ChangeFixedVariance(fVariance);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetPitchVariation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    float fVariation;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fVariation))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        pSound->ChangePitchVariation(fVariation);
    return 0;
}

// The shipping build answers "is this a live item", not the item's own identified flag.
int CSWVirtualMachineCommands::ExecuteCommandGetIdentified(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (!g_pVirtualMachine->StackPushInteger(pItem != 0))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandRemoveAvailableNPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    int bRemoved = g_pAppManager->m_pServerExoApp->GetPartyTable()->RemoveNPC(nNPC);
    if (!g_pVirtualMachine->StackPushInteger(bRemoved))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSwitchPlayerCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSlot;
    if (!g_pVirtualMachine->StackPopInteger(&nSlot))
        return VM_ERROR_POP_FAILED;
    int bSwitched = g_pAppManager->m_pServerExoApp->GetPartyTable()->SwitchPlayerCharacter(nSlot);
    if (!g_pVirtualMachine->StackPushInteger(bSwitched))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetFixedVariance(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    float fVariance;  // uninitialised when the sound object is not found  // idiom-exempt: uninitialised in the orig, same as SoundObjectGetVolume
#ifdef K2_FIX_ENGINE_BUGS
    fVariance = 0.0f;
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        fVariance = pSound->m_fixedVariance;
    if (!g_pVirtualMachine->StackPushFloat(fVariance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetPitchVariation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    float fVariation;  // uninitialised when the sound object is not found  // idiom-exempt: uninitialised in the orig, same as SoundObjectGetVolume
#ifdef K2_FIX_ENGINE_BUGS
    fVariation = 0.0f;
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    CSWSSoundObject* pSound = g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound)
        fVariation = pSound->m_pitchVariation;
    if (!g_pVirtualMachine->StackPushFloat(fVariation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandRemoveFromParty(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0 && pCreature->m_bPlayerCharacter != 0) {
        CSWSFaction* pFaction = pCreature->GetFaction();
        if (pFaction)
            pFaction->RemoveMember(oidCreature);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetIdentified(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    int bIdentified;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bIdentified))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem)
        pItem->SetIdentified(bIdentified);
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetHealTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidTarget = pCreature ? static_cast<unsigned long long>(pCreature->m_oidHealTarget)
                                             : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetHealTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature)
        pCreature->m_oidHealTarget = static_cast<int>(oidTarget);
    return 0;
}
#endif // !KOTOR1

// rand() % nMax, with a zero bound short-circuited to 0. The result follows the divisor's
// sign, not the dividend's.
int CSWVirtualMachineCommands::ExecuteCommandRandom(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMax;
    if (!g_pVirtualMachine->StackPopInteger(&nMax))
        return VM_ERROR_POP_FAILED;
    int nValue = 0;
    if (nMax != 0)
    {
        nValue = rand_wincompatible() % nMax;
        if (nMax < 0)
            nValue = -nValue;
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetItemStackSize(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    int nStackSize = pItem ? pItem->m_stackCount : 0;
    if (!g_pVirtualMachine->StackPushInteger(nStackSize))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetTutorialWindowsEnabled(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bEnabled;
    if (!g_pVirtualMachine->StackPopInteger(&bEnabled))
        return VM_ERROR_POP_FAILED;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_bTutorialWindows = static_cast<unsigned char>(bEnabled);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetSpellTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidTarget = pCreature ? pCreature->m_oidSpellTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastMineDetected(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMine = pCreature ? pCreature->m_oidLastMineDetected : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidMine))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetCreatureSize(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nSize = pCreature ? pCreature->m_nCreatureSize : 0;
    if (!g_pVirtualMachine->StackPushInteger(nSize))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

void CSWVirtualMachineCommands::InitializeCommands() {
    m_paCommands = new CommandFn[VM_NUM_COMMANDS];
    for (int i = 0; i < VM_NUM_COMMANDS; i++)
        m_paCommands[i] = 0;
    m_paCommands[77] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[71] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[37] = &CSWVirtualMachineCommands::ExecuteCommandAttack;
    m_paCommands[700] = &CSWVirtualMachineCommands::ExecuteCommandActionBarkString;
    m_paCommands[502] = &CSWVirtualMachineCommands::ExecuteCommandCastSpell;
    m_paCommands[501] = &CSWVirtualMachineCommands::ExecuteCommandCastSpell;
    m_paCommands[48] = &CSWVirtualMachineCommands::ExecuteCommandCastSpell;
    m_paCommands[44] = &CSWVirtualMachineCommands::ExecuteCommandCloseDoor;
    m_paCommands[294] = &CSWVirtualMachineCommands::ExecuteCommandActionDoCommand;
    m_paCommands[32] = &CSWVirtualMachineCommands::ExecuteCommandEquipItem;
    m_paCommands[399] = &CSWVirtualMachineCommands::ExecuteCommandActionEquipMostDamagingMelee;
    m_paCommands[400] = &CSWVirtualMachineCommands::ExecuteCommandActionEquipMostDamagingRanged;
    m_paCommands[404] = &CSWVirtualMachineCommands::ExecuteCommandActionEquipMostEffectiveArmor;
    m_paCommands[730] = &CSWVirtualMachineCommands::ExecuteCommandActionFollowLeader;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[843] = &CSWVirtualMachineCommands::ExecuteCommandActionFollowOwner;
#endif
    m_paCommands[167] = &CSWVirtualMachineCommands::ExecuteCommandActionFollowObject;
    m_paCommands[382] = &CSWVirtualMachineCommands::ExecuteCommandMoveToPoint;
    m_paCommands[383] = &CSWVirtualMachineCommands::ExecuteCommandMoveToObject;
    m_paCommands[135] = &CSWVirtualMachineCommands::ExecuteCommandActionExchangeItem;
    m_paCommands[329] = &CSWVirtualMachineCommands::ExecuteCommandActionInteractObject;
    m_paCommands[214] = &CSWVirtualMachineCommands::ExecuteCommandActionJumpToPoint;
    m_paCommands[196] = &CSWVirtualMachineCommands::ExecuteCommandActionJumpToObject;
    m_paCommands[484] = &CSWVirtualMachineCommands::ExecuteCommandActionLockActions;
    m_paCommands[360] = &CSWVirtualMachineCommands::ExecuteCommandActionMoveAwayFromLocation;
    m_paCommands[23] = &CSWVirtualMachineCommands::ExecuteCommandMoveAwayFromObject;
    m_paCommands[22] = &CSWVirtualMachineCommands::ExecuteCommandMoveToObject;
    m_paCommands[21] = &CSWVirtualMachineCommands::ExecuteCommandMoveToPoint;
    m_paCommands[43] = &CSWVirtualMachineCommands::ExecuteCommandOpenDoor;
    m_paCommands[205] = &CSWVirtualMachineCommands::ExecuteCommandActionPauseConversation;
    m_paCommands[34] = &CSWVirtualMachineCommands::ExecuteCommandPickUpItem;
    m_paCommands[40] = &CSWVirtualMachineCommands::ExecuteCommandPlayAnimation;
    m_paCommands[35] = &CSWVirtualMachineCommands::ExecuteCommandPutDownItem;
    m_paCommands[20] = &CSWVirtualMachineCommands::ExecuteCommandActionRandomWalk;
    m_paCommands[206] = &CSWVirtualMachineCommands::ExecuteCommandActionResumeConversation;
    m_paCommands[39] = &CSWVirtualMachineCommands::ExecuteCommandSpeakString;
    m_paCommands[240] = &CSWVirtualMachineCommands::ExecuteCommandSpeakStrRef;
    m_paCommands[204] = &CSWVirtualMachineCommands::ExecuteCommandActionStartConversation;
    m_paCommands[379] = &CSWVirtualMachineCommands::ExecuteCommandSurrenderToEnemies;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[853] = &CSWVirtualMachineCommands::ExecuteCommandActionSwitchWeapons;
#endif
    m_paCommands[136] = &CSWVirtualMachineCommands::ExecuteCommandActionExchangeItem;
    m_paCommands[33] = &CSWVirtualMachineCommands::ExecuteCommandUnequipItem;
    m_paCommands[483] = &CSWVirtualMachineCommands::ExecuteCommandActionLockActions;
    m_paCommands[287] = &CSWVirtualMachineCommands::ExecuteCommandActionUseFeat;
    m_paCommands[288] = &CSWVirtualMachineCommands::ExecuteCommandActionUseSkill;
    m_paCommands[310] = &CSWVirtualMachineCommands::ExecuteCommandActionUseTalent;
    m_paCommands[309] = &CSWVirtualMachineCommands::ExecuteCommandActionUseTalent;
    m_paCommands[694] = &CSWVirtualMachineCommands::ExecuteCommandAddAvailableNPCByObject;
    m_paCommands[697] = &CSWVirtualMachineCommands::ExecuteCommandAddAvailableNPCByTemplate;
#if !KOTOR1
    m_paCommands[836] = &CSWVirtualMachineCommands::ExecuteCommandAddAvailablePUPByTemplate;
#endif
#if !KOTOR1
    m_paCommands[837] = &CSWVirtualMachineCommands::ExecuteCommandAddAvailablePUPByObject;
#endif
    m_paCommands[367] = &CSWVirtualMachineCommands::ExecuteCommandAddJournalQuestEntry;
    m_paCommands[669] = &CSWVirtualMachineCommands::ExecuteCommandAddJournalWorldEntry;
    m_paCommands[670] = &CSWVirtualMachineCommands::ExecuteCommandAddJournalWorldEntryStrref;
    m_paCommands[389] = &CSWVirtualMachineCommands::ExecuteCommandAddMultiClass;
    m_paCommands[574] = &CSWVirtualMachineCommands::ExecuteCommandAddPartyMember;
#if !KOTOR1
    m_paCommands[840] = &CSWVirtualMachineCommands::ExecuteCommandAddPartyPuppet;
#endif
    m_paCommands[572] = &CSWVirtualMachineCommands::ExecuteCommandAddToParty;
    m_paCommands[201] = &CSWVirtualMachineCommands::ExecuteCommandAdjustAlignment;
    m_paCommands[209] = &CSWVirtualMachineCommands::ExecuteCommandAdjustReputation;
    m_paCommands[435] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[436] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[433] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[567] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[568] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[434] = &CSWVirtualMachineCommands::ExecuteCommandAmbientSound;
    m_paCommands[144] = &CSWVirtualMachineCommands::ExecuteCommandVectorConversions;
    m_paCommands[216] = &CSWVirtualMachineCommands::ExecuteCommandApplyEffectAtPoint;
    m_paCommands[220] = &CSWVirtualMachineCommands::ExecuteCommandApplyEffectOnObject;
    m_paCommands[72] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[6] = &CSWVirtualMachineCommands::ExecuteCommandAssignCommand;
#if !KOTOR1
    m_paCommands[838] = &CSWVirtualMachineCommands::ExecuteCommandAssignPUP;
#endif
    m_paCommands[51] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[252] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[73] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[480] = &CSWVirtualMachineCommands::ExecuteCommandAwardStealthXP;
    m_paCommands[671] = &CSWVirtualMachineCommands::ExecuteCommandBarkString;
    m_paCommands[255] = &CSWVirtualMachineCommands::ExecuteCommandBeginConversation;
    m_paCommands[54] = &CSWVirtualMachineCommands::ExecuteCommandCancelCombat;
    m_paCommands[757] = &CSWVirtualMachineCommands::ExecuteCommandCancelPostDialogCharacterSwitch;
    m_paCommands[234] = &CSWVirtualMachineCommands::ExecuteCommandCastSpell;
    m_paCommands[173] = &CSWVirtualMachineCommands::ExecuteCommandChangeFaction;
    m_paCommands[737] = &CSWVirtualMachineCommands::ExecuteCommandChangeByFaction;
    m_paCommands[747] = &CSWVirtualMachineCommands::ExecuteCommandChangeItemCost;
    m_paCommands[412] = &CSWVirtualMachineCommands::ExecuteCommandChangeFaction;
    m_paCommands[9] = &CSWVirtualMachineCommands::ExecuteCommandClearAllActions;
    m_paCommands[710] = &CSWVirtualMachineCommands::ExecuteCommandClearAllEffects;
    m_paCommands[68] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[766] = &CSWVirtualMachineCommands::ExecuteCommandCreateItemOnFloor;
    m_paCommands[31] = &CSWVirtualMachineCommands::ExecuteCommandCreateItemOnObject;
    m_paCommands[243] = &CSWVirtualMachineCommands::ExecuteCommandCreateObject;
    m_paCommands[503] = &CSWVirtualMachineCommands::ExecuteCommandCutsceneAttack;
    m_paCommands[507] = &CSWVirtualMachineCommands::ExecuteCommandCutsceneMove;
    m_paCommands[100] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[103] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[101] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[95] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[102] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[96] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[97] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[98] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[99] = &CSWVirtualMachineCommands::ExecuteCommandRollDice;
    m_paCommands[7] = &CSWVirtualMachineCommands::ExecuteCommandDelayCommand;
    m_paCommands[672] = &CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldAllEntries;
    m_paCommands[673] = &CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldEntry;
    m_paCommands[674] = &CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldEntryStrref;
    m_paCommands[241] = &CSWVirtualMachineCommands::ExecuteCommandDestroyObject;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[857] = &CSWVirtualMachineCommands::ExecuteCommandDetonateMine;
#endif
#if !KOTOR1
    m_paCommands[856] = &CSWVirtualMachineCommands::ExecuteCommandDisableMap;
#endif
#if !KOTOR1
    m_paCommands[858] = &CSWVirtualMachineCommands::ExecuteCommandDisableHealthRegen;
#endif
    m_paCommands[510] = &CSWVirtualMachineCommands::ExecuteCommandDisableVideoEffect;
    m_paCommands[366] = &CSWVirtualMachineCommands::ExecuteCommandDisplayFeedBackText;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[864] = &CSWVirtualMachineCommands::ExecuteCommandDisplayMessageBox;
    m_paCommands[865] = &CSWVirtualMachineCommands::ExecuteCommandDisplayDatapad;
#endif
    m_paCommands[338] = &CSWVirtualMachineCommands::ExecuteCommandDoDoorAction;
    m_paCommands[547] = &CSWVirtualMachineCommands::ExecuteCommandDoDoorAction;
    m_paCommands[512] = &CSWVirtualMachineCommands::ExecuteCommandDoSinglePlayerAutoSave;
    m_paCommands[500] = &CSWVirtualMachineCommands::ExecuteCommandDuplicateHeadAppearance;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[800] = &CSWVirtualMachineCommands::ExecuteCommandModifyGlobal;
#endif
    m_paCommands[754] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[755] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[756] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[391] = &CSWVirtualMachineCommands::ExecuteCommandEffectDroidStun;
    m_paCommands[153] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceJump;
    m_paCommands[392] = &CSWVirtualMachineCommands::ExecuteCommandEffectForcePushed;
    m_paCommands[269] = &CSWVirtualMachineCommands::ExecuteCommandEffectForcePushTargeted;
    m_paCommands[703] = &CSWVirtualMachineCommands::ExecuteCommandEffectWhirlwind;
    m_paCommands[469] = &CSWVirtualMachineCommands::ExecuteCommandEffectBlasterDeflectionIncrease;
    m_paCommands[470] = &CSWVirtualMachineCommands::ExecuteCommandEffectBlasterDeflectionDecrease;
    m_paCommands[471] = &CSWVirtualMachineCommands::ExecuteCommandEffectHorrified;
    m_paCommands[372] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageForcePoints;
    m_paCommands[373] = &CSWVirtualMachineCommands::ExecuteCommandEffectHealForcePoints;
    m_paCommands[156] = &CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryForcePoints;
    m_paCommands[159] = &CSWVirtualMachineCommands::ExecuteCommandEffectChoke;
    m_paCommands[224] = &CSWVirtualMachineCommands::ExecuteCommandEffectBodyFuel;
    m_paCommands[676] = &CSWVirtualMachineCommands::ExecuteCommandEffectPsychicStatic;
    m_paCommands[702] = &CSWVirtualMachineCommands::ExecuteCommandEffectLightsaberThrow;
    m_paCommands[446] = &CSWVirtualMachineCommands::ExecuteCommandEffectAbilityDecrease;
    m_paCommands[80] = &CSWVirtualMachineCommands::ExecuteCommandEffectAbilityIncrease;
    m_paCommands[450] = &CSWVirtualMachineCommands::ExecuteCommandEffectACDecrease;
    m_paCommands[115] = &CSWVirtualMachineCommands::ExecuteCommandEffectACIncrease;
    m_paCommands[171] = &CSWVirtualMachineCommands::ExecuteCommandEffectAreaEffect;
    m_paCommands[447] = &CSWVirtualMachineCommands::ExecuteCommandEffectAttackDecrease;
    m_paCommands[118] = &CSWVirtualMachineCommands::ExecuteCommandEffectAttackIncrease;
    m_paCommands[207] = &CSWVirtualMachineCommands::ExecuteCommandEffectBeam;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[778] = &CSWVirtualMachineCommands::ExecuteCommandEffectBlind;
    m_paCommands[781] = &CSWVirtualMachineCommands::ExecuteCommandEffectCrush;
#endif
    m_paCommands[458] = &CSWVirtualMachineCommands::ExecuteCommandEffectConcealment;
    m_paCommands[157] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[79] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamage;
    m_paCommands[448] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageDecrease;
    m_paCommands[449] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageImmunityDecrease;
    m_paCommands[275] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageImmunityIncrease;
    m_paCommands[120] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageIncrease;
    m_paCommands[119] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageReduction;
    m_paCommands[81] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageResistance;
    m_paCommands[487] = &CSWVirtualMachineCommands::ExecuteCommandEffectDamageShield;
    m_paCommands[133] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[463] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[460] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[473] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[809] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
#endif
    m_paCommands[130] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[849] = &CSWVirtualMachineCommands::ExecuteCommandEffectFactionModifier;
#endif
#if !KOTOR1  // K1 slots 769-771 are the movie queue / Yavin commands
    m_paCommands[770] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceBody;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[823] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceSight;
#endif
    m_paCommands[675] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceDrain;
    m_paCommands[420] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[454] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceResistanceDecrease;
    m_paCommands[212] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceResistanceIncrease;
    m_paCommands[402] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[459] = &CSWVirtualMachineCommands::ExecuteCommandEffectForceShield;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[779] = &CSWVirtualMachineCommands::ExecuteCommandEffectFPRegenModifier;
#endif
    m_paCommands[158] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[777] = &CSWVirtualMachineCommands::ExecuteCommandEffectFury;
#endif
    m_paCommands[270] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[78] = &CSWVirtualMachineCommands::ExecuteCommandEffectHeal;
    m_paCommands[387] = &CSWVirtualMachineCommands::ExecuteCommandEffectHitPointChangeWhenDying;
    m_paCommands[273] = &CSWVirtualMachineCommands::ExecuteCommandEffectImmunity;
    m_paCommands[457] = &CSWVirtualMachineCommands::ExecuteCommandEffectInvisibility;
    m_paCommands[134] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[199] = &CSWVirtualMachineCommands::ExecuteCommandEffectLinkEffects;
    m_paCommands[477] = &CSWVirtualMachineCommands::ExecuteCommandEffectMissChance;
    m_paCommands[485] = &CSWVirtualMachineCommands::ExecuteCommandEffectModifyAttacks;
    m_paCommands[451] = &CSWVirtualMachineCommands::ExecuteCommandEffectMovementSpeedDecrease;
    m_paCommands[165] = &CSWVirtualMachineCommands::ExecuteCommandEffectMovementSpeedIncrease;
    m_paCommands[148] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[250] = &CSWVirtualMachineCommands::ExecuteCommandEffectPoison;
    m_paCommands[164] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[82] = &CSWVirtualMachineCommands::ExecuteCommandEffectResurrection;
    m_paCommands[452] = &CSWVirtualMachineCommands::ExecuteCommandEffectSavingThrowDecrease;
    m_paCommands[117] = &CSWVirtualMachineCommands::ExecuteCommandEffectSavingThrowIncrease;
    m_paCommands[466] = &CSWVirtualMachineCommands::ExecuteCommandEffectSeeInvisible;
    m_paCommands[453] = &CSWVirtualMachineCommands::ExecuteCommandEffectSkillDecrease;
    m_paCommands[351] = &CSWVirtualMachineCommands::ExecuteCommandEffectSkillIncrease;
    m_paCommands[154] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[149] = &CSWVirtualMachineCommands::ExecuteCommandEffectSpellImmunity;
    m_paCommands[472] = &CSWVirtualMachineCommands::ExecuteCommandEffectSpellLevelAbsorption;
    m_paCommands[161] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[156] = &CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryFP;
    m_paCommands[314] = &CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryHP;
    m_paCommands[467] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[465] = &CSWVirtualMachineCommands::ExecuteCommandEffectTrueSeeing;
    m_paCommands[180] = &CSWVirtualMachineCommands::ExecuteCommandEffectVisualEffect;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[780] = &CSWVirtualMachineCommands::ExecuteCommandEffectVPRegenModifier;
    m_paCommands[848] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[852] = &CSWVirtualMachineCommands::ExecuteCommandEffectComplex;
    m_paCommands[871] = &CSWVirtualMachineCommands::ExecuteCommandEnableRendering;
#endif
    m_paCommands[508] = &CSWVirtualMachineCommands::ExecuteCommandEnableVideoEffect;
    m_paCommands[564] = &CSWVirtualMachineCommands::ExecuteCommandEndGame;
    m_paCommands[424] = &CSWVirtualMachineCommands::ExecuteCommandEventActivateItem;
    m_paCommands[295] = &CSWVirtualMachineCommands::ExecuteCommandEventConversation;
    m_paCommands[244] = &CSWVirtualMachineCommands::ExecuteCommandEventSpellCastAt;
    m_paCommands[132] = &CSWVirtualMachineCommands::ExecuteCommandEventUserDefined;
    m_paCommands[8] = &CSWVirtualMachineCommands::ExecuteCommandExecuteScript;
    m_paCommands[403] = &CSWVirtualMachineCommands::ExecuteCommandExploreAreaForPlayer;
    m_paCommands[557] = &CSWVirtualMachineCommands::ExecuteCommandExportAllCharacters;
    m_paCommands[114] = &CSWVirtualMachineCommands::ExecuteCommandSetSubType;
    m_paCommands[67] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[553] = &CSWVirtualMachineCommands::ExecuteCommandFaceObjectAwayFromObject;
    m_paCommands[218] = &CSWVirtualMachineCommands::ExecuteCommandDistanceConversions;
    m_paCommands[66] = &CSWVirtualMachineCommands::ExecuteCommandFindSubString;
    m_paCommands[526] = &CSWVirtualMachineCommands::ExecuteCommandFloatingText;
    m_paCommands[525] = &CSWVirtualMachineCommands::ExecuteCommandFloatingText;
    m_paCommands[231] = &CSWVirtualMachineCommands::ExecuteCommandFloatToInt;
    m_paCommands[3] = &CSWVirtualMachineCommands::ExecuteCommandFloatToString;
    m_paCommands[108] = &CSWVirtualMachineCommands::ExecuteCommandSavingThrow;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[822] = &CSWVirtualMachineCommands::ExecuteCommandForceHeartbeat;
    m_paCommands[862] = &CSWVirtualMachineCommands::ExecuteCommandForceAlwaysUpdate;
#endif
    m_paCommands[331] = &CSWVirtualMachineCommands::ExecuteCommandGetAbilityModifier;
    m_paCommands[139] = &CSWVirtualMachineCommands::ExecuteCommandGetAbilityScore;
    m_paCommands[116] = &CSWVirtualMachineCommands::ExecuteCommandGetAC;
    m_paCommands[127] = &CSWVirtualMachineCommands::ExecuteCommandGetAlignment;
    m_paCommands[524] = &CSWVirtualMachineCommands::ExecuteCommandGetAppearanceType;
    m_paCommands[24] = &CSWVirtualMachineCommands::ExecuteCommandGetArea;
    m_paCommands[264] = &CSWVirtualMachineCommands::ExecuteCommandGetAOEObjectCreator;
    m_paCommands[15] = &CSWVirtualMachineCommands::ExecuteCommandGetAreaUnescapable;
    m_paCommands[316] = &CSWVirtualMachineCommands::ExecuteCommandGetAttackTarget;
    m_paCommands[361] = &CSWVirtualMachineCommands::ExecuteCommandGetAttemptedAttackTarget;
    m_paCommands[375] = &CSWVirtualMachineCommands::ExecuteCommandGetAttemptedSpellTarget;
    m_paCommands[489] = &CSWVirtualMachineCommands::ExecuteCommandGetAttemptedMovementTarget;
    m_paCommands[490] = &CSWVirtualMachineCommands::ExecuteCommandGetBlockingCreature;
    m_paCommands[397] = &CSWVirtualMachineCommands::ExecuteCommandGetBaseItemType;
    m_paCommands[336] = &CSWVirtualMachineCommands::ExecuteCommandGetBlockingDoor;
    m_paCommands[267] = &CSWVirtualMachineCommands::ExecuteCommandGetButtonMashCheck;
    m_paCommands[84] = &CSWVirtualMachineCommands::ExecuteCommandGetCasterLevel;
    m_paCommands[735] = &CSWVirtualMachineCommands::ExecuteCommandTalentAccess;
    m_paCommands[494] = &CSWVirtualMachineCommands::ExecuteCommandGetChallengeRating;
    m_paCommands[764] = &CSWVirtualMachineCommands::ExecuteCommandGetCheatCode;
    m_paCommands[341] = &CSWVirtualMachineCommands::ExecuteCommandGetClassInformation;
    m_paCommands[326] = &CSWVirtualMachineCommands::ExecuteCommandGetEnteringObject;
    m_paCommands[163] = &CSWVirtualMachineCommands::ExecuteCommandGetCommandable;
    m_paCommands[306] = &CSWVirtualMachineCommands::ExecuteCommandGetCreatureHasTalent;
    m_paCommands[479] = &CSWVirtualMachineCommands::ExecuteCommandGetCreatureSize;
    m_paCommands[308] = &CSWVirtualMachineCommands::ExecuteCommandGetCreatureTalent;
    m_paCommands[307] = &CSWVirtualMachineCommands::ExecuteCommandGetCreatureTalent;
    m_paCommands[522] = &CSWVirtualMachineCommands::ExecuteCommandGetCurrentAction;
    m_paCommands[55] = &CSWVirtualMachineCommands::ExecuteCommandGetCurrentForcePoints;
    m_paCommands[49] = &CSWVirtualMachineCommands::ExecuteCommandGetHitpoints;
    m_paCommands[474] = &CSWVirtualMachineCommands::ExecuteCommandGetCurrentStealthXP;
    m_paCommands[344] = &CSWVirtualMachineCommands::ExecuteCommandGetDamageDealtByType;
    m_paCommands[523] = &CSWVirtualMachineCommands::ExecuteCommandGetDifficultyMultiplier;
    m_paCommands[151] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetween;
    m_paCommands[319] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetween2D;
    m_paCommands[298] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetweenLocations;
    m_paCommands[334] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetweenLocations2D;
    m_paCommands[41] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceToObject;
    m_paCommands[335] = &CSWVirtualMachineCommands::ExecuteCommandGetDistanceToObject2D;
    m_paCommands[91] = &CSWVirtualMachineCommands::ExecuteCommandGetEffectCreator;
    m_paCommands[89] = &CSWVirtualMachineCommands::ExecuteCommandGetEffectDurationType;
    m_paCommands[305] = &CSWVirtualMachineCommands::ExecuteCommandGetEffectSpellId;
    m_paCommands[90] = &CSWVirtualMachineCommands::ExecuteCommandGetEffectSubType;
    m_paCommands[170] = &CSWVirtualMachineCommands::ExecuteCommandGetEffectType;
    m_paCommands[276] = &CSWVirtualMachineCommands::ExecuteCommandGetEncounterData;
    m_paCommands[297] = &CSWVirtualMachineCommands::ExecuteCommandGetEncounterData;
    m_paCommands[280] = &CSWVirtualMachineCommands::ExecuteCommandGetEncounterData;
    m_paCommands[278] = &CSWVirtualMachineCommands::ExecuteCommandGetEncounterData;
    m_paCommands[25] = &CSWVirtualMachineCommands::ExecuteCommandGetEnteringObject;
    m_paCommands[26] = &CSWVirtualMachineCommands::ExecuteCommandGetLeavingObject;
    m_paCommands[28] = &CSWVirtualMachineCommands::ExecuteCommandGetFacing;
    m_paCommands[225] = &CSWVirtualMachineCommands::ExecuteCommandLocationAccess;
    m_paCommands[187] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageGoodEvilAlignment;
    m_paCommands[189] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageLevel;
    m_paCommands[186] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageReputation;
    m_paCommands[190] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageXP;
    m_paCommands[193] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionBestAC;
    m_paCommands[172] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionEqual;
    m_paCommands[185] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionGold;
    m_paCommands[562] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionLeader;
    m_paCommands[184] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionLeastDamagedMember;
    m_paCommands[183] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionMostDamagedMember;
    m_paCommands[191] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionMostFrequentClass;
    m_paCommands[182] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionStrongestMember;
    m_paCommands[181] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionWeakestMember;
    m_paCommands[192] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionWorstAC;
    m_paCommands[727] = &CSWVirtualMachineCommands::ExecuteCommandGetAttacker;
    m_paCommands[85] = &CSWVirtualMachineCommands::ExecuteCommandGetEffect;
    m_paCommands[380] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionMember;
    m_paCommands[262] = &CSWVirtualMachineCommands::ExecuteCommandGetInPersistentObject;
    m_paCommands[339] = &CSWVirtualMachineCommands::ExecuteCommandGetItemInInventory;
    m_paCommands[93] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectInArea;
    m_paCommands[128] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectInShape;
    m_paCommands[548] = &CSWVirtualMachineCommands::ExecuteCommandGetPC;
    m_paCommands[491] = &CSWVirtualMachineCommands::ExecuteCommandGetFortitudeSavingThrow;
    m_paCommands[495] = &CSWVirtualMachineCommands::ExecuteCommandGetFoundEnemyCreature;
    m_paCommands[513] = &CSWVirtualMachineCommands::ExecuteCommandGetGameDifficulty;
    m_paCommands[358] = &CSWVirtualMachineCommands::ExecuteCommandGetStat;
    m_paCommands[578] = &CSWVirtualMachineCommands::ExecuteCommandGetGlobalNumber;
    m_paCommands[692] = &CSWVirtualMachineCommands::ExecuteCommandGetGlobalLocation;
    m_paCommands[580] = &CSWVirtualMachineCommands::ExecuteCommandGetGlobalNumber;
    m_paCommands[194] = &CSWVirtualMachineCommands::ExecuteCommandGetGlobalString;
    m_paCommands[211] = &CSWVirtualMachineCommands::ExecuteCommandGetGoingToBeAttackedBy;
    m_paCommands[418] = &CSWVirtualMachineCommands::ExecuteCommandGetGold;
    m_paCommands[311] = &CSWVirtualMachineCommands::ExecuteCommandGetGoldPieceValue;
#if !KOTOR1  // K1 slots 769-771 are the movie queue / Yavin commands
    m_paCommands[771] = &CSWVirtualMachineCommands::ExecuteCommandGetGold;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[772] = &CSWVirtualMachineCommands::ExecuteCommandGetGoldPieceValue;
    m_paCommands[774] = &CSWVirtualMachineCommands::ExecuteCommandGetChemical;
    m_paCommands[775] = &CSWVirtualMachineCommands::ExecuteCommandGetChemicalPieceValue;
#endif
    m_paCommands[125] = &CSWVirtualMachineCommands::ExecuteCommandGetAlignment;
    m_paCommands[285] = &CSWVirtualMachineCommands::ExecuteCommandGetHasFeat;
    m_paCommands[543] = &CSWVirtualMachineCommands::ExecuteCommandGetHasFeatEffect;
    m_paCommands[570] = &CSWVirtualMachineCommands::ExecuteCommandGetHasInventory;
    m_paCommands[286] = &CSWVirtualMachineCommands::ExecuteCommandGetHasSkill;
    m_paCommands[377] = &CSWVirtualMachineCommands::ExecuteCommandGetHasSpell;
    m_paCommands[304] = &CSWVirtualMachineCommands::ExecuteCommandGetHasSpellEffect;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[814] = &CSWVirtualMachineCommands::ExecuteCommandGetHealTarget;
#endif
    m_paCommands[166] = &CSWVirtualMachineCommands::ExecuteCommandGetHitDice;
    m_paCommands[332] = &CSWVirtualMachineCommands::ExecuteCommandGetIdentified;
    m_paCommands[363] = &CSWVirtualMachineCommands::ExecuteCommandTalentAccess;
    m_paCommands[353] = &CSWVirtualMachineCommands::ExecuteCommandGetInventoryDisturbItem;
    m_paCommands[352] = &CSWVirtualMachineCommands::ExecuteCommandGetInventoryDisturbType;
    m_paCommands[701] = &CSWVirtualMachineCommands::ExecuteCommandGetIsConversationActive;
    m_paCommands[566] = &CSWVirtualMachineCommands::ExecuteCommandGetCreatureMovmentType;
    m_paCommands[407] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDawn;
    m_paCommands[405] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDay;
    m_paCommands[732] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDebilitated;
    m_paCommands[337] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDoorActionPossible;
    m_paCommands[408] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDusk;
    m_paCommands[409] = &CSWVirtualMachineCommands::ExecuteCommandGetIsEncounterCreature;
    m_paCommands[235] = &CSWVirtualMachineCommands::ExecuteCommandGetEmotions;
    m_paCommands[236] = &CSWVirtualMachineCommands::ExecuteCommandGetEmotions;
    m_paCommands[274] = &CSWVirtualMachineCommands::ExecuteCommandGetIsImmune;
    m_paCommands[320] = &CSWVirtualMachineCommands::ExecuteCommandGetIsInCombat;
    m_paCommands[445] = &CSWVirtualMachineCommands::ExecuteCommandGetIsInConversation;
    m_paCommands[390] = &CSWVirtualMachineCommands::ExecuteCommandGetIsLinkImmune;
    m_paCommands[174] = &CSWVirtualMachineCommands::ExecuteCommandIsListening;
    m_paCommands[748] = &CSWVirtualMachineCommands::ExecuteCommandGetIsLiveContentAvailable;
    m_paCommands[237] = &CSWVirtualMachineCommands::ExecuteCommandGetEmotions;
    m_paCommands[406] = &CSWVirtualMachineCommands::ExecuteCommandGetIsNight;
    m_paCommands[42] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectValid;
    m_paCommands[443] = &CSWVirtualMachineCommands::ExecuteCommandGetIsOpen;
    m_paCommands[546] = &CSWVirtualMachineCommands::ExecuteCommandGetIsDoorActionPossible;
    m_paCommands[751] = &CSWVirtualMachineCommands::ExecuteCommandGetIsPoisoned;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[842] = &CSWVirtualMachineCommands::ExecuteCommandGetIsPuppet;
    m_paCommands[851] = &CSWVirtualMachineCommands::ExecuteCommandGetIsXBox;
#endif
    m_paCommands[312] = &CSWVirtualMachineCommands::ExecuteCommandGetIsRacialTypePlayable;
    m_paCommands[359] = &CSWVirtualMachineCommands::ExecuteCommandGetIsTalentValid;
    m_paCommands[551] = &CSWVirtualMachineCommands::ExecuteCommandGetIsMined;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[844] = &CSWVirtualMachineCommands::ExecuteCommandGetIsPartyLeader;
    m_paCommands[845] = &CSWVirtualMachineCommands::ExecuteCommandGetPartyLeader;
#endif
    m_paCommands[422] = &CSWVirtualMachineCommands::ExecuteCommandGetIsWeaponEffective;
    m_paCommands[439] = &CSWVirtualMachineCommands::ExecuteCommandItemActivated;
    m_paCommands[441] = &CSWVirtualMachineCommands::ExecuteCommandItemActivated;
    m_paCommands[442] = &CSWVirtualMachineCommands::ExecuteCommandItemActivated;
    m_paCommands[440] = &CSWVirtualMachineCommands::ExecuteCommandItemActivated;
    m_paCommands[401] = &CSWVirtualMachineCommands::ExecuteCommandGetItemACValue;
    m_paCommands[398] = &CSWVirtualMachineCommands::ExecuteCommandGetItemHasItemProperty;
    m_paCommands[155] = &CSWVirtualMachineCommands::ExecuteCommandGetItemInSlot;
    m_paCommands[30] = &CSWVirtualMachineCommands::ExecuteCommandGetItemPossessedBy;
    m_paCommands[29] = &CSWVirtualMachineCommands::ExecuteCommandGetItemPossessor;
    m_paCommands[138] = &CSWVirtualMachineCommands::ExecuteCommandGetItemStackSize;
    m_paCommands[369] = &CSWVirtualMachineCommands::ExecuteCommandGetJournalEntry;
    m_paCommands[384] = &CSWVirtualMachineCommands::ExecuteCommandGetJournalQuestExperience;
    m_paCommands[321] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAssociateCommand;
    m_paCommands[36] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAttacker;
    m_paCommands[318] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAttackMode;
    m_paCommands[722] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAttackAction;
    m_paCommands[725] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAttackResult;
    m_paCommands[721] = &CSWVirtualMachineCommands::ExecuteCommandGetLastHostileTarget;
    m_paCommands[317] = &CSWVirtualMachineCommands::ExecuteCommandGetLastAttackType;
    m_paCommands[260] = &CSWVirtualMachineCommands::ExecuteCommandGetLastClosedBy;
    m_paCommands[724] = &CSWVirtualMachineCommands::ExecuteCommandGetLastCombatFeatUsed;
    m_paCommands[711] = &CSWVirtualMachineCommands::ExecuteCommandGetLastConversation;
    m_paCommands[346] = &CSWVirtualMachineCommands::ExecuteCommandGetLastDamager;
    m_paCommands[347] = &CSWVirtualMachineCommands::ExecuteCommandGetLastDisarmed;
    m_paCommands[348] = &CSWVirtualMachineCommands::ExecuteCommandGetLastDisturbed;
    m_paCommands[723] = &CSWVirtualMachineCommands::ExecuteCommandGetLastForcePowerUsed;
    m_paCommands[556] = &CSWVirtualMachineCommands::ExecuteCommandGetLastHostileActor;
    m_paCommands[437] = &CSWVirtualMachineCommands::ExecuteCommandGetLastKiller;
    m_paCommands[349] = &CSWVirtualMachineCommands::ExecuteCommandGetLastLocked;
    m_paCommands[376] = &CSWVirtualMachineCommands::ExecuteCommandGetLastOpenedBy;
    m_paCommands[365] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPazaakResult;
    m_paCommands[256] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPerceived;
    m_paCommands[257] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPerception;
    m_paCommands[258] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPerception;
    m_paCommands[259] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPerception;
    m_paCommands[261] = &CSWVirtualMachineCommands::ExecuteCommandGetLastPerception;
    m_paCommands[291] = &CSWVirtualMachineCommands::ExecuteCommandModuleAccess;
    m_paCommands[410] = &CSWVirtualMachineCommands::ExecuteCommandModuleAccess;
    m_paCommands[419] = &CSWVirtualMachineCommands::ExecuteCommandModuleAccess;
    m_paCommands[52] = &CSWVirtualMachineCommands::ExecuteCommandModuleAccess;
    m_paCommands[254] = &CSWVirtualMachineCommands::ExecuteCommandGetLastSpeaker;
    m_paCommands[246] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellCast;
    m_paCommands[245] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellCast;
    m_paCommands[423] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellCast;
    m_paCommands[486] = &CSWVirtualMachineCommands::ExecuteCommandGetLastMineDetected;
    m_paCommands[350] = &CSWVirtualMachineCommands::ExecuteCommandGetLastUnlocked;
    m_paCommands[330] = &CSWVirtualMachineCommands::ExecuteCommandGetLastUsedBy;
    m_paCommands[328] = &CSWVirtualMachineCommands::ExecuteCommandGetLastWeaponUsed;
    m_paCommands[343] = &CSWVirtualMachineCommands::ExecuteCommandGetClassInformation;
    m_paCommands[342] = &CSWVirtualMachineCommands::ExecuteCommandGetClassInformation;
    m_paCommands[251] = &CSWVirtualMachineCommands::ExecuteCommandGetLoadFromSaveGame;
    m_paCommands[679] = &CSWVirtualMachineCommands::ExecuteCommandGetLocalValue;
    m_paCommands[681] = &CSWVirtualMachineCommands::ExecuteCommandGetLocalValue;
    m_paCommands[213] = &CSWVirtualMachineCommands::ExecuteCommandGetLocation;
    m_paCommands[325] = &CSWVirtualMachineCommands::ExecuteCommandGetLocked;
    m_paCommands[537] = &CSWVirtualMachineCommands::ExecuteCommandGetLockInfo;
    m_paCommands[538] = &CSWVirtualMachineCommands::ExecuteCommandGetLockInfo;
    m_paCommands[539] = &CSWVirtualMachineCommands::ExecuteCommandGetLockInfo;
    m_paCommands[541] = &CSWVirtualMachineCommands::ExecuteCommandGetLockInfo;
    m_paCommands[540] = &CSWVirtualMachineCommands::ExecuteCommandGetLockInfo;
    m_paCommands[195] = &CSWVirtualMachineCommands::ExecuteCommandGetMatchedStringsNum;
    m_paCommands[178] = &CSWVirtualMachineCommands::ExecuteCommandGetMatchedString;
    m_paCommands[179] = &CSWVirtualMachineCommands::ExecuteCommandGetMatchedStringsCount;
    m_paCommands[56] = &CSWVirtualMachineCommands::ExecuteCommandGetMaxForcePoints;
    m_paCommands[50] = &CSWVirtualMachineCommands::ExecuteCommandGetHitpoints;
    m_paCommands[464] = &CSWVirtualMachineCommands::ExecuteCommandGetMaxStealthXP;
    m_paCommands[105] = &CSWVirtualMachineCommands::ExecuteCommandGetMetaMagicFeat;
    m_paCommands[715] = &CSWVirtualMachineCommands::ExecuteCommandGetMinOneHP;
    m_paCommands[242] = &CSWVirtualMachineCommands::ExecuteCommandGetModule;
    m_paCommands[210] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleName;
    m_paCommands[282] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleItemStuff;
    m_paCommands[283] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleItemStuff;
    m_paCommands[292] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleItemStuff;
    m_paCommands[293] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleItemStuff;
    m_paCommands[561] = &CSWVirtualMachineCommands::ExecuteCommandGetModuleName;
    m_paCommands[496] = &CSWVirtualMachineCommands::ExecuteCommandGetMovementRate;
    m_paCommands[558] = &CSWVirtualMachineCommands::ExecuteCommandGetDayTrack;
    m_paCommands[559] = &CSWVirtualMachineCommands::ExecuteCommandGetNightTrack;
    m_paCommands[253] = &CSWVirtualMachineCommands::ExecuteCommandGetName;
    m_paCommands[226] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestObject;
    m_paCommands[38] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestObject;
    m_paCommands[227] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestObject;
    m_paCommands[229] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestObject;
    m_paCommands[228] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestObject;
    m_paCommands[488] = &CSWVirtualMachineCommands::ExecuteCommandGetNearestMine;
    m_paCommands[728] = &CSWVirtualMachineCommands::ExecuteCommandGetAttacker;
    m_paCommands[86] = &CSWVirtualMachineCommands::ExecuteCommandGetEffect;
    m_paCommands[381] = &CSWVirtualMachineCommands::ExecuteCommandGetFactionMember;
    m_paCommands[263] = &CSWVirtualMachineCommands::ExecuteCommandGetInPersistentObject;
    m_paCommands[340] = &CSWVirtualMachineCommands::ExecuteCommandGetItemInInventory;
    m_paCommands[94] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectInArea;
    m_paCommands[709] = &CSWVirtualMachineCommands::ExecuteCommandIsAvailableNPC;
    m_paCommands[129] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectInShape;
    m_paCommands[549] = &CSWVirtualMachineCommands::ExecuteCommandGetPC;
    m_paCommands[705] = &CSWVirtualMachineCommands::ExecuteCommandGetNPCAIStyle;
    m_paCommands[709] = &CSWVirtualMachineCommands::ExecuteCommandIsAvailableNPC;
    m_paCommands[475] = &CSWVirtualMachineCommands::ExecuteCommandGetNumStackedItems;
    m_paCommands[200] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectByTag;
    m_paCommands[290] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectVisibility;
    m_paCommands[289] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectVisibility;
    m_paCommands[106] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectType;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[793] = &CSWVirtualMachineCommands::ExecuteCommandGetOwnerDemolitionsSkill;
#endif
#if !KOTOR1
    m_paCommands[795] = &CSWVirtualMachineCommands::ExecuteCommandInfluence;
#endif
#if !KOTOR1
    m_paCommands[796] = &CSWVirtualMachineCommands::ExecuteCommandInfluence;
#endif
#if !KOTOR1
    m_paCommands[797] = &CSWVirtualMachineCommands::ExecuteCommandInfluence;
#endif
    m_paCommands[704] = &CSWVirtualMachineCommands::ExecuteCommandGetPartyAIStyle;
    m_paCommands[577] = &CSWVirtualMachineCommands::ExecuteCommandGetPartyMemberByIndex;
    m_paCommands[126] = &CSWVirtualMachineCommands::ExecuteCommandGetPartyMemberCount;
    m_paCommands[743] = &CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapPlanetAvailable;
    m_paCommands[741] = &CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapPlanetSelectable;
    m_paCommands[565] = &CSWVirtualMachineCommands::ExecuteCommandGetRunScriptVar;
    m_paCommands[542] = &CSWVirtualMachineCommands::ExecuteCommandGetPCLevellingUp;
    m_paCommands[238] = &CSWVirtualMachineCommands::ExecuteCommandGetPCSpeaker;
    m_paCommands[545] = &CSWVirtualMachineCommands::ExecuteCommandGetPlaceableIllumination;
    m_paCommands[455] = &CSWVirtualMachineCommands::ExecuteCommandGetPlotFlag;
    m_paCommands[27] = &CSWVirtualMachineCommands::ExecuteCommandGetPosition;
    m_paCommands[223] = &CSWVirtualMachineCommands::ExecuteCommandLocationAccess;
    m_paCommands[107] = &CSWVirtualMachineCommands::ExecuteCommandGetRacialType;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[798] = &CSWVirtualMachineCommands::ExecuteCommandGetRacialSubType;
    m_paCommands[815] = &CSWVirtualMachineCommands::ExecuteCommandGetRandomDestination;
#endif
    m_paCommands[299] = &CSWVirtualMachineCommands::ExecuteCommandGetReflexAdjustedDamage;
    m_paCommands[493] = &CSWVirtualMachineCommands::ExecuteCommandGetReflexSavingThrow;
    m_paCommands[208] = &CSWVirtualMachineCommands::ExecuteCommandGetReputation;
    m_paCommands[83] = &CSWVirtualMachineCommands::ExecuteCommandGetRestrictMode;
#if !KOTOR1
    m_paCommands[768] = &CSWVirtualMachineCommands::ExecuteCommandGetScriptParameter;
#endif
#if !KOTOR1  // K1 slots 769-771 are the movie queue / Yavin commands
    m_paCommands[769] = &CSWVirtualMachineCommands::ExecuteCommandSetFadeUntilScript;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[872] = &CSWVirtualMachineCommands::ExecuteCommandGetCombatActionPending;
#endif
    m_paCommands[744] = &CSWVirtualMachineCommands::ExecuteCommandGetGalaxyMapSelectedPlanet;
    m_paCommands[315] = &CSWVirtualMachineCommands::ExecuteCommandGetSkillRank;
    m_paCommands[462] = &CSWVirtualMachineCommands::ExecuteCommandGetSoloMode;
    m_paCommands[438] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellCasterItem;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[776] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellForcePointCost;
    m_paCommands[783] = &CSWVirtualMachineCommands::ExecuteCommandGetHasFeat;
    m_paCommands[784] = &CSWVirtualMachineCommands::ExecuteCommandGetHasSpell;
    m_paCommands[786] = &CSWVirtualMachineCommands::ExecuteCommandGrantAbility;
    m_paCommands[787] = &CSWVirtualMachineCommands::ExecuteCommandGrantAbility;
    m_paCommands[817] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellFormMask;
    m_paCommands[818] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellBaseForcePointCost;
#endif
#if !KOTOR1
    m_paCommands[827] = &CSWVirtualMachineCommands::ExecuteCommandGetLastForfeitViolation;
#endif
#if !KOTOR1
    m_paCommands[831] = &CSWVirtualMachineCommands::ExecuteCommandGetScriptStringParameter;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[870] = &CSWVirtualMachineCommands::ExecuteCommandGetSkillRank;
    m_paCommands[828] = &CSWVirtualMachineCommands::ExecuteCommandModifyBaseSavingThrow;
    m_paCommands[829] = &CSWVirtualMachineCommands::ExecuteCommandModifyBaseSavingThrow;
    m_paCommands[830] = &CSWVirtualMachineCommands::ExecuteCommandModifyBaseSavingThrow;
    m_paCommands[832] = &CSWVirtualMachineCommands::ExecuteCommandGetObjectPersonalSpace;
    m_paCommands[833] = &CSWVirtualMachineCommands::ExecuteCommandAdjustCreatureAttributes;
    m_paCommands[834] = &CSWVirtualMachineCommands::ExecuteCommandSetCreatureAILevel;
    m_paCommands[835] = &CSWVirtualMachineCommands::ExecuteCommandResetCreatureAILevel;
    m_paCommands[850] = &CSWVirtualMachineCommands::ExecuteCommandChangeObjectAppearance;
    m_paCommands[869] = &CSWVirtualMachineCommands::ExecuteCommandAdjustCreatureSkills;
#endif
#if !KOTOR1
    m_paCommands[847] = &CSWVirtualMachineCommands::ExecuteCommandCreatureFlourishWeapon;
#endif
    m_paCommands[248] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellId;
    m_paCommands[111] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellSaveDC;
    m_paCommands[222] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellTargetLoc;
    m_paCommands[47] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellTargetObject;
    m_paCommands[752] = &CSWVirtualMachineCommands::ExecuteCommandGetSpellTarget;
    m_paCommands[713] = &CSWVirtualMachineCommands::ExecuteCommandGetStandardFaction;
    m_paCommands[411] = &CSWVirtualMachineCommands::ExecuteCommandModuleAccess;
    m_paCommands[498] = &CSWVirtualMachineCommands::ExecuteCommandGetStealthXPDecrement;
    m_paCommands[481] = &CSWVirtualMachineCommands::ExecuteCommandGetStealthXPEnabled;
    m_paCommands[239] = &CSWVirtualMachineCommands::ExecuteCommandGetStrByStrRef;
    m_paCommands[63] = &CSWVirtualMachineCommands::ExecuteCommandGetStringLeft;
    m_paCommands[59] = &CSWVirtualMachineCommands::ExecuteCommandGetStringLength;
    m_paCommands[61] = &CSWVirtualMachineCommands::ExecuteCommandGetStringLowerCase;
    m_paCommands[62] = &CSWVirtualMachineCommands::ExecuteCommandGetStringRight;
    m_paCommands[60] = &CSWVirtualMachineCommands::ExecuteCommandGetStringUpperCase;
    m_paCommands[571] = &CSWVirtualMachineCommands::ExecuteCommandGetStrRefSoundDuration;
    m_paCommands[497] = &CSWVirtualMachineCommands::ExecuteCommandGetSubRace;
    m_paCommands[65] = &CSWVirtualMachineCommands::ExecuteCommandGetSubString;
    m_paCommands[53] = &CSWVirtualMachineCommands::ExecuteCommandGetSubScreenID;
    m_paCommands[168] = &CSWVirtualMachineCommands::ExecuteCommandGetTag;
    m_paCommands[16] = &CSWVirtualMachineCommands::ExecuteCommandGetTimeHour;
    m_paCommands[19] = &CSWVirtualMachineCommands::ExecuteCommandGetTimeMillisecond;
    m_paCommands[17] = &CSWVirtualMachineCommands::ExecuteCommandGetTimeMinute;
    m_paCommands[18] = &CSWVirtualMachineCommands::ExecuteCommandGetTimeSecond;
    m_paCommands[345] = &CSWVirtualMachineCommands::ExecuteCommandGetTotalDamageDealt;
    m_paCommands[198] = &CSWVirtualMachineCommands::ExecuteCommandGetWaypointByTrigger;
    m_paCommands[531] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[533] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[528] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[536] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[529] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[527] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[535] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[530] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[534] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[532] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
#if !KOTOR1
    m_paCommands[841] = &CSWVirtualMachineCommands::ExecuteCommandGetPUPOwner;
#endif
    m_paCommands[362] = &CSWVirtualMachineCommands::ExecuteCommandTalentAccess;
    m_paCommands[514] = &CSWVirtualMachineCommands::ExecuteCommandGetUserActionsPending;
    m_paCommands[247] = &CSWVirtualMachineCommands::ExecuteCommandGetUserDefinedEventNumber;
    m_paCommands[726] = &CSWVirtualMachineCommands::ExecuteCommandGetWasForcePowerSuccessful;
    m_paCommands[197] = &CSWVirtualMachineCommands::ExecuteCommandGetWaypointByTag;
    m_paCommands[511] = &CSWVirtualMachineCommands::ExecuteCommandGetWeaponRanged;
    m_paCommands[492] = &CSWVirtualMachineCommands::ExecuteCommandGetWillSavingThrow;
    m_paCommands[395] = &CSWVirtualMachineCommands::ExecuteCommandGetXP;
    m_paCommands[322] = &CSWVirtualMachineCommands::ExecuteCommandGiveGoldToCreature;
    m_paCommands[271] = &CSWVirtualMachineCommands::ExecuteCommandGiveItem;
    m_paCommands[714] = &CSWVirtualMachineCommands::ExecuteCommandGivePlotXP;
    m_paCommands[393] = &CSWVirtualMachineCommands::ExecuteCommandGiveXPToCreature;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[820] = &CSWVirtualMachineCommands::ExecuteCommandHasLineOfSight;
#endif
    m_paCommands[122] = &CSWVirtualMachineCommands::ExecuteCommandTimeConversions;
    m_paCommands[760] = &CSWVirtualMachineCommands::ExecuteCommandHoldWorldFadeInForDialog;
    m_paCommands[64] = &CSWVirtualMachineCommands::ExecuteCommandInsertString;
    m_paCommands[230] = &CSWVirtualMachineCommands::ExecuteCommandIntToFloat;
    m_paCommands[396] = &CSWVirtualMachineCommands::ExecuteCommandIntToHexString;
    m_paCommands[92] = &CSWVirtualMachineCommands::ExecuteCommandIntToString;
    m_paCommands[696] = &CSWVirtualMachineCommands::ExecuteCommandIsAvailableNPC;
    m_paCommands[519] = &CSWVirtualMachineCommands::ExecuteCommandIsCreditSequenceInProgress;
    m_paCommands[140] = &CSWVirtualMachineCommands::ExecuteCommandIsAIState;
    m_paCommands[88] = &CSWVirtualMachineCommands::ExecuteCommandIsEffectValid;
#if KOTOR1
    m_paCommands[768] = &CSWVirtualMachineCommands::ExecuteCommandIsMoviePlaying;
#else
    m_paCommands[805] = &CSWVirtualMachineCommands::ExecuteCommandIsMoviePlaying;
#endif
    m_paCommands[699] = &CSWVirtualMachineCommands::ExecuteCommandIsNPCPartyMember;
    m_paCommands[576] = &CSWVirtualMachineCommands::ExecuteCommandIsObjectPartyMember;
    m_paCommands[217] = &CSWVirtualMachineCommands::ExecuteCommandIsPC;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[824] = &CSWVirtualMachineCommands::ExecuteCommandIsRunning;
    m_paCommands[799] = &CSWVirtualMachineCommands::ExecuteCommandModifyGlobal;
    m_paCommands[810] = &CSWVirtualMachineCommands::ExecuteCommandIsStealthed;
    m_paCommands[811] = &CSWVirtualMachineCommands::ExecuteCommandIsMeditating;
    m_paCommands[812] = &CSWVirtualMachineCommands::ExecuteCommandIsInTotalDefense;
    m_paCommands[816] = &CSWVirtualMachineCommands::ExecuteCommandIsFormActive;
#endif
    m_paCommands[313] = &CSWVirtualMachineCommands::ExecuteCommandActionJumpToPoint;
    m_paCommands[385] = &CSWVirtualMachineCommands::ExecuteCommandActionJumpToObject;
    m_paCommands[215] = &CSWVirtualMachineCommands::ExecuteCommandLocation;
    m_paCommands[74] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[112] = &CSWVirtualMachineCommands::ExecuteCommandSetSubType;
    m_paCommands[428] = &CSWVirtualMachineCommands::ExecuteCommandMusicBackground;
    m_paCommands[429] = &CSWVirtualMachineCommands::ExecuteCommandMusicBackground;
    m_paCommands[569] = &CSWVirtualMachineCommands::ExecuteCommandGetBattleTrack;
    m_paCommands[425] = &CSWVirtualMachineCommands::ExecuteCommandMusicBackground;
    m_paCommands[427] = &CSWVirtualMachineCommands::ExecuteCommandMusicBackground;
    m_paCommands[426] = &CSWVirtualMachineCommands::ExecuteCommandMusicBackground;
    m_paCommands[432] = &CSWVirtualMachineCommands::ExecuteCommandMusicBattle;
    m_paCommands[430] = &CSWVirtualMachineCommands::ExecuteCommandMusicBattle;
    m_paCommands[431] = &CSWVirtualMachineCommands::ExecuteCommandMusicBattle;
    m_paCommands[759] = &CSWVirtualMachineCommands::ExecuteCommandNoClicksFor;
    m_paCommands[272] = &CSWVirtualMachineCommands::ExecuteCommandObjectToString;
    m_paCommands[378] = &CSWVirtualMachineCommands::ExecuteCommandOpenStore;
    m_paCommands[57] = &CSWVirtualMachineCommands::ExecuteCommandPauseGame;
    m_paCommands[300] = &CSWVirtualMachineCommands::ExecuteCommandPlayAnimation;
    m_paCommands[733] = &CSWVirtualMachineCommands::ExecuteCommandPlayMovie;
#if KOTOR1
    m_paCommands[770] = &CSWVirtualMachineCommands::ExecuteCommandPlayMovieQueue;
#else
    m_paCommands[807] = &CSWVirtualMachineCommands::ExecuteCommandPlayMovieQueue;
#endif
    m_paCommands[364] = &CSWVirtualMachineCommands::ExecuteCommandPlayPazaak;
    m_paCommands[738] = &CSWVirtualMachineCommands::ExecuteCommandPlayRoomAnimation;
    m_paCommands[370] = &CSWVirtualMachineCommands::ExecuteCommandPlayRumblePattern;
    m_paCommands[46] = &CSWVirtualMachineCommands::ExecuteCommandPlaySound;
    m_paCommands[677] = &CSWVirtualMachineCommands::ExecuteCommandPlayVisualAreaEffect;
    m_paCommands[554] = &CSWVirtualMachineCommands::ExecuteCommandPopUpGUIPanel;
    m_paCommands[388] = &CSWVirtualMachineCommands::ExecuteCommandPopUpGUIPanel;
    m_paCommands[75] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[2] = &CSWVirtualMachineCommands::ExecuteCommandPrintFloat;
    m_paCommands[4] = &CSWVirtualMachineCommands::ExecuteCommandPrintInteger;
    m_paCommands[560] = &CSWVirtualMachineCommands::ExecuteCommandPrintLogEntry;
    m_paCommands[5] = &CSWVirtualMachineCommands::ExecuteCommandPrintObject;
    m_paCommands[1] = &CSWVirtualMachineCommands::ExecuteCommandPrintString;
    m_paCommands[141] = &CSWVirtualMachineCommands::ExecuteCommandPrintVector;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[854] = &CSWVirtualMachineCommands::ExecuteCommandPlayOverlayAnimation;
#endif
#if KOTOR1
    m_paCommands[769] = &CSWVirtualMachineCommands::ExecuteCommandQueueMovie;
#else
    m_paCommands[806] = &CSWVirtualMachineCommands::ExecuteCommandQueueMovie;
#endif
    m_paCommands[0] = &CSWVirtualMachineCommands::ExecuteCommandRandom;
    m_paCommands[249] = &CSWVirtualMachineCommands::ExecuteCommandRandomName;
#if !KOTOR1
    m_paCommands[876] = &CSWVirtualMachineCommands::ExecuteCommandRebuildPartyTable;
#endif
    m_paCommands[109] = &CSWVirtualMachineCommands::ExecuteCommandSavingThrow;
    m_paCommands[695] = &CSWVirtualMachineCommands::ExecuteCommandRemoveAvailableNPC;
    m_paCommands[87] = &CSWVirtualMachineCommands::ExecuteCommandRemoveEffect;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[867] = &CSWVirtualMachineCommands::ExecuteCommandRemoveEffectByID;
    m_paCommands[868] = &CSWVirtualMachineCommands::ExecuteCommandRemoveEffectByExactMatch;
#endif
    m_paCommands[573] = &CSWVirtualMachineCommands::ExecuteCommandRemoveFromParty;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[866] = &CSWVirtualMachineCommands::ExecuteCommandRemoveHeartbeat;
#endif
    m_paCommands[368] = &CSWVirtualMachineCommands::ExecuteCommandRemoveJournalQuestEntry;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[846] = &CSWVirtualMachineCommands::ExecuteCommandRemoveNPCFromPartyToBase;
#endif
    m_paCommands[575] = &CSWVirtualMachineCommands::ExecuteCommandRemovePartyMember;
    m_paCommands[749] = &CSWVirtualMachineCommands::ExecuteCommandResetDialogState;
    m_paCommands[169] = &CSWVirtualMachineCommands::ExecuteCommandResistSpell;
    m_paCommands[515] = &CSWVirtualMachineCommands::ExecuteCommandRevealMap;
    m_paCommands[121] = &CSWVirtualMachineCommands::ExecuteCommandTimeConversions;
    m_paCommands[734] = &CSWVirtualMachineCommands::ExecuteCommandSaveNPCState;
#if !KOTOR1
    m_paCommands[873] = &CSWVirtualMachineCommands::ExecuteCommandSaveNPCByObject;
#endif
#if !KOTOR1
    m_paCommands[874] = &CSWVirtualMachineCommands::ExecuteCommandSavePUPByObject;
#endif
    m_paCommands[374] = &CSWVirtualMachineCommands::ExecuteCommandSendMessageToPC;
    m_paCommands[746] = &CSWVirtualMachineCommands::ExecuteCommandSetAreaFogColor;
    m_paCommands[203] = &CSWVirtualMachineCommands::ExecuteCommandSetAreaTransitionBMP;
    m_paCommands[14] = &CSWVirtualMachineCommands::ExecuteCommandSetAreaUnescapable;
    m_paCommands[327] = &CSWVirtualMachineCommands::ExecuteCommandSetAssociateListenPatterns;
    m_paCommands[767] = &CSWVirtualMachineCommands::ExecuteCommandSetAvailableNPCID;
    m_paCommands[268] = &CSWVirtualMachineCommands::ExecuteCommandSetButtonMashCheck;
    m_paCommands[45] = &CSWVirtualMachineCommands::ExecuteCommandSetCameraLocation;
    m_paCommands[504] = &CSWVirtualMachineCommands::ExecuteCommandSetCameraMode;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[791] = &CSWVirtualMachineCommands::ExecuteCommandSetFakeCombatState;
#endif
    m_paCommands[162] = &CSWVirtualMachineCommands::ExecuteCommandSetCommandable;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[859] = &CSWVirtualMachineCommands::ExecuteCommandSetCurrentForm;
#endif
    m_paCommands[478] = &CSWVirtualMachineCommands::ExecuteCommandSetCurrentStealthXP;
    m_paCommands[284] = &CSWVirtualMachineCommands::ExecuteCommandSetCustomToken;
    m_paCommands[461] = &CSWVirtualMachineCommands::ExecuteCommandSetDialogPlaceableCamera;
    m_paCommands[552] = &CSWVirtualMachineCommands::ExecuteCommandSetEffectIcon;
    m_paCommands[277] = &CSWVirtualMachineCommands::ExecuteCommandSetEncounterData;
    m_paCommands[296] = &CSWVirtualMachineCommands::ExecuteCommandSetEncounterData;
    m_paCommands[281] = &CSWVirtualMachineCommands::ExecuteCommandSetEncounterData;
    m_paCommands[279] = &CSWVirtualMachineCommands::ExecuteCommandSetEncounterData;
    m_paCommands[10] = &CSWVirtualMachineCommands::ExecuteCommandSetFacing;
    m_paCommands[143] = &CSWVirtualMachineCommands::ExecuteCommandSetFacing;
    m_paCommands[719] = &CSWVirtualMachineCommands::ExecuteCommandSetFadeIn;
    m_paCommands[720] = &CSWVirtualMachineCommands::ExecuteCommandSetFadeOut;
    m_paCommands[731] = &CSWVirtualMachineCommands::ExecuteCommandSetForcePowerUnsuccessful;
    m_paCommands[729] = &CSWVirtualMachineCommands::ExecuteCommandSetFormation;
    m_paCommands[579] = &CSWVirtualMachineCommands::ExecuteCommandSetGlobalNumber;
    m_paCommands[693] = &CSWVirtualMachineCommands::ExecuteCommandSetGlobalLocation;
    m_paCommands[581] = &CSWVirtualMachineCommands::ExecuteCommandSetGlobalNumber;
    m_paCommands[160] = &CSWVirtualMachineCommands::ExecuteCommandSetGlobalString;
    m_paCommands[750] = &CSWVirtualMachineCommands::ExecuteCommandSetGoodEvilValue;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[813] = &CSWVirtualMachineCommands::ExecuteCommandSetHealTarget;
#endif
    m_paCommands[333] = &CSWVirtualMachineCommands::ExecuteCommandSetIdentified;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[861] = &CSWVirtualMachineCommands::ExecuteCommandSetInputClass;
#endif
    m_paCommands[323] = &CSWVirtualMachineCommands::ExecuteCommandSetIsDestroyable;
    m_paCommands[266] = &CSWVirtualMachineCommands::ExecuteCommandSetItemNonEquippable;
    m_paCommands[150] = &CSWVirtualMachineCommands::ExecuteCommandSetItemStackSize;
    m_paCommands[678] = &CSWVirtualMachineCommands::ExecuteCommandSetJournalQuestEntryPicture;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[819] = &CSWVirtualMachineCommands::ExecuteCommandSetKeepStealthInDialog;
#endif
    m_paCommands[421] = &CSWVirtualMachineCommands::ExecuteCommandSetLightsaberPowered;
    m_paCommands[175] = &CSWVirtualMachineCommands::ExecuteCommandSetListening;
    m_paCommands[176] = &CSWVirtualMachineCommands::ExecuteCommandSetListenString;
    m_paCommands[680] = &CSWVirtualMachineCommands::ExecuteCommandSetLocalValue;
    m_paCommands[682] = &CSWVirtualMachineCommands::ExecuteCommandSetLocalValue;
    m_paCommands[324] = &CSWVirtualMachineCommands::ExecuteCommandSetLocked;
    m_paCommands[505] = &CSWVirtualMachineCommands::ExecuteCommandSetLockOrientationInDialog;
    m_paCommands[506] = &CSWVirtualMachineCommands::ExecuteCommandSetLockHeadFollowInDialog;
    m_paCommands[708] = &CSWVirtualMachineCommands::ExecuteCommandSetNPCSelectability;
    m_paCommands[386] = &CSWVirtualMachineCommands::ExecuteCommandSetMapPinEnabled;
    m_paCommands[758] = &CSWVirtualMachineCommands::ExecuteCommandSetMaxHitPoints;
    m_paCommands[468] = &CSWVirtualMachineCommands::ExecuteCommandSetMaxStealthXP;
    m_paCommands[550] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[555] = &CSWVirtualMachineCommands::ExecuteCommandGetMineInfo;
    m_paCommands[716] = &CSWVirtualMachineCommands::ExecuteCommandSetMinOneHP;
    m_paCommands[765] = &CSWVirtualMachineCommands::ExecuteCommandSetMusicVolume;
    m_paCommands[707] = &CSWVirtualMachineCommands::ExecuteCommandSetNPCAIStyle;
    m_paCommands[706] = &CSWVirtualMachineCommands::ExecuteCommandSetPartyAIStyle;
    m_paCommands[13] = &CSWVirtualMachineCommands::ExecuteCommandSetPartyLeader;
    m_paCommands[544] = &CSWVirtualMachineCommands::ExecuteCommandSetPlaceableIllumination;
    m_paCommands[742] = &CSWVirtualMachineCommands::ExecuteCommandSetGalaxyMapPlanetAvailable;
    m_paCommands[740] = &CSWVirtualMachineCommands::ExecuteCommandSetGalaxyMapPlanetSelectable;
    m_paCommands[456] = &CSWVirtualMachineCommands::ExecuteCommandSetPlotFlag;
    m_paCommands[58] = &CSWVirtualMachineCommands::ExecuteCommandSetRestrictMode;
    m_paCommands[152] = &CSWVirtualMachineCommands::ExecuteCommandSetReturnStrref;
    m_paCommands[753] = &CSWVirtualMachineCommands::ExecuteCommandSetSoloMode;
    m_paCommands[563] = &CSWVirtualMachineCommands::ExecuteCommandSetSpeedBlurEffect;
    m_paCommands[12] = &CSWVirtualMachineCommands::ExecuteCommandSetTime;
    m_paCommands[516] = &CSWVirtualMachineCommands::ExecuteCommandSetTutorialWindowsEnabled;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[863] = &CSWVirtualMachineCommands::ExecuteCommandEnableRain;
#endif
    m_paCommands[394] = &CSWVirtualMachineCommands::ExecuteCommandSetXP;
    m_paCommands[761] = &CSWVirtualMachineCommands::ExecuteCommandShipBuild;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[821] = &CSWVirtualMachineCommands::ExecuteCommandShowDemoScreen;
#endif
    m_paCommands[739] = &CSWVirtualMachineCommands::ExecuteCommandShowGalaxyMapGui;
    m_paCommands[265] = &CSWVirtualMachineCommands::ExecuteCommandShowLevelUpGUI;
    m_paCommands[712] = &CSWVirtualMachineCommands::ExecuteCommandShowPartySelection;
    m_paCommands[517] = &CSWVirtualMachineCommands::ExecuteCommandShowTutorialWindow;
    m_paCommands[354] = &CSWVirtualMachineCommands::ExecuteCommandShowUpgradeScreen;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[773] = &CSWVirtualMachineCommands::ExecuteCommandShowChemicalUpgradeScreen;
    m_paCommands[785] = &CSWVirtualMachineCommands::ExecuteCommandShowSwoopUpgradeScreen;
#endif
    m_paCommands[131] = &CSWVirtualMachineCommands::ExecuteCommandSignalEvent;
    m_paCommands[69] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[188] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetFixedVariance;
    m_paCommands[689] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetPitchVariation;
    m_paCommands[691] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectGetVolume;
    m_paCommands[413] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectPlay;
    m_paCommands[124] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetFixedVariance;
    m_paCommands[690] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetPitchVariation;
    m_paCommands[416] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetPosition;
    m_paCommands[415] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetVolume;
    m_paCommands[414] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectStop;
    m_paCommands[745] = &CSWVirtualMachineCommands::ExecuteCommandSoundObjectFadeAndStop;
    m_paCommands[698] = &CSWVirtualMachineCommands::ExecuteCommandSpawnAvailableNPC;
#if !KOTOR1
    m_paCommands[839] = &CSWVirtualMachineCommands::ExecuteCommandSpawnAvailablePUP;
#endif
    m_paCommands[417] = &CSWVirtualMachineCommands::ExecuteCommandSpeakOneLinerConversation;
    m_paCommands[221] = &CSWVirtualMachineCommands::ExecuteCommandSpeakString;
    m_paCommands[76] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[509] = &CSWVirtualMachineCommands::ExecuteCommandStartNewModule;
    m_paCommands[499] = &CSWVirtualMachineCommands::ExecuteCommandSetStealthXPDecrement;
    m_paCommands[482] = &CSWVirtualMachineCommands::ExecuteCommandSetStealthXPEnabled;
    m_paCommands[518] = &CSWVirtualMachineCommands::ExecuteCommandStartCreditSequence;
    m_paCommands[371] = &CSWVirtualMachineCommands::ExecuteCommandStopRumblePattern;
    m_paCommands[233] = &CSWVirtualMachineCommands::ExecuteCommandStringConversions;
    m_paCommands[232] = &CSWVirtualMachineCommands::ExecuteCommandStringConversions;
    m_paCommands[113] = &CSWVirtualMachineCommands::ExecuteCommandSetSubType;
    m_paCommands[763] = &CSWVirtualMachineCommands::ExecuteCommandSuppressStatusSummaryEntry;
    m_paCommands[736] = &CSWVirtualMachineCommands::ExecuteCommandChangeByFaction;
    m_paCommands[762] = &CSWVirtualMachineCommands::ExecuteCommandSurrenderToEnemies;
    m_paCommands[476] = &CSWVirtualMachineCommands::ExecuteCommandSurrenderToEnemies;
    m_paCommands[11] = &CSWVirtualMachineCommands::ExecuteCommandSwitchPlayerCreature;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[788] = &CSWVirtualMachineCommands::ExecuteCommandSpawnMine;
#endif
#if !KOTOR1
    m_paCommands[826] = &CSWVirtualMachineCommands::ExecuteCommandSetForfeitConditions;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[794] = &CSWVirtualMachineCommands::ExecuteCommandSetOrientOnClick;
#endif
#if !KOTOR1
    m_paCommands[860] = &CSWVirtualMachineCommands::ExecuteCommandSetDisableTransit;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[801] = &CSWVirtualMachineCommands::ExecuteCommandSetBonusForcePoints;
    m_paCommands[802] = &CSWVirtualMachineCommands::ExecuteCommandAddBonusForcePoints;
    m_paCommands[803] = &CSWVirtualMachineCommands::ExecuteCommandGetBonusForcePoints;
#endif
    m_paCommands[444] = &CSWVirtualMachineCommands::ExecuteCommandTakeGoldFromCreature;
    m_paCommands[302] = &CSWVirtualMachineCommands::ExecuteCommandTalent;
    m_paCommands[303] = &CSWVirtualMachineCommands::ExecuteCommandTalent;
    m_paCommands[301] = &CSWVirtualMachineCommands::ExecuteCommandTalent;
    m_paCommands[70] = &CSWVirtualMachineCommands::ExecuteCommandMath;
    m_paCommands[177] = &CSWVirtualMachineCommands::ExecuteCommandTestString;
    m_paCommands[146] = &CSWVirtualMachineCommands::ExecuteCommandDoTouchAttack;
    m_paCommands[147] = &CSWVirtualMachineCommands::ExecuteCommandDoTouchAttack;
    m_paCommands[123] = &CSWVirtualMachineCommands::ExecuteCommandTimeConversions;
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[855] = &CSWVirtualMachineCommands::ExecuteCommandUnlockAllMusic;
#endif
    m_paCommands[142] = &CSWVirtualMachineCommands::ExecuteCommandVector;
    m_paCommands[104] = &CSWVirtualMachineCommands::ExecuteCommandVectorMagnitude;
    m_paCommands[137] = &CSWVirtualMachineCommands::ExecuteCommandVectorNormalize;
    m_paCommands[145] = &CSWVirtualMachineCommands::ExecuteCommandVectorConversions;
    m_paCommands[355] = &CSWVirtualMachineCommands::ExecuteCommandVersusEffect;
    m_paCommands[356] = &CSWVirtualMachineCommands::ExecuteCommandVersusEffect;
    m_paCommands[357] = &CSWVirtualMachineCommands::ExecuteCommandVersusEffect;
    m_paCommands[202] = &CSWVirtualMachineCommands::ExecuteCommandWait;
    m_paCommands[110] = &CSWVirtualMachineCommands::ExecuteCommandSavingThrow;
    m_paCommands[219] = &CSWVirtualMachineCommands::ExecuteCommandDistanceConversions;
#if KOTOR1
    m_paCommands[771] = &CSWVirtualMachineCommands::ExecuteCommandYavinHackCloseDoor;
#else
    m_paCommands[808] = &CSWVirtualMachineCommands::ExecuteCommandYavinHackCloseDoor;
#endif
#if !KOTOR1  // K1 has 772 commands
    m_paCommands[875] = &CSWVirtualMachineCommands::ExecuteCommandGetIsPlayerMadeCharacter;
#endif
    InitializeSWMGCommands();
}

// The script-side effect constructors build the engine effect the script asked for, push it on the
// VM value stack (success frees our reference; the push-failure return leaves it for the stack's
// own failure path), and return the engine's created-OK/NO_EFFECT status codes. Every member of the
// family shares the skeleton; only the middle differs.

int CSWVirtualMachineCommands::ExecuteCommandEffectDroidStun(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
    pEffect->SetInteger(0, 3);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// EffectVPRegenModifier: the vitality-regeneration multiplier effect. Its creator is stamped only
// when OBJECT_SELF still resolves, so an effect built from a dead script owner is creator-less.
int CSWVirtualMachineCommands::ExecuteCommandEffectVPRegenModifier(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VPREGENMODIFIER);
    pEffect->SetInteger(0, nAmount);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// EffectForceJump: the target and the "straight to melee" flag ride in the effect itself. The
// flag operand is optional and defaults to 0; the target defaults to OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandEffectForceJump(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    unsigned long long oidTarget = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    int bStraightToMelee = 0;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bStraightToMelee))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEJUMP);
    pEffect->SetObjectID(0, oidTarget);
    pEffect->SetInteger(0, bStraightToMelee);
    pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectForcePushed(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
    pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The aimed cousin of EffectForcePushed above. It has the same subtype, but integer slot 0 carries
// the "targeted" flag and the three float slots carry the shove's centre, copied out of the popped
// location.
int CSWVirtualMachineCommands::ExecuteCommandEffectForcePushTargeted(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptLocation* pLocation = 0;
    int bIgnoreTestDirectLine = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bIgnoreTestDirectLine))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
    pEffect->SetCreator(m_oidSelf);
    pEffect->SetInteger(0, 1);
    pEffect->SetInteger(1, bIgnoreTestDirectLine);
    pEffect->SetFloat(0, pLocation->m_vPosition.x);
    pEffect->SetFloat(1, pLocation->m_vPosition.y);
    pEffect->SetFloat(2, pLocation->m_vPosition.z);
    if (pLocation != 0)
        delete pLocation;
    pLocation = 0;  // the store stays outside the guard: the null case reaches it too  // idiom-exempt: the orig's `str xzr,[sp,#0x10]` is on the MERGED path -- the
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectWhirlwind(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
    pEffect->SetInteger(0, EFFECT_STATE_WHIRLWIND);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The four amount-carrying members of the family pop a script integer first (so a failed pop is
// -2001, ahead of the push's -2000) and park it in the effect's integer slot 0.
int CSWVirtualMachineCommands::ExecuteCommandEffectBlasterDeflectionIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nChange;
    if (g_pVirtualMachine->StackPopInteger(&nChange) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE);
    pEffect->SetInteger(0, nChange);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectBlasterDeflectionDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nChange;
    if (g_pVirtualMachine->StackPopInteger(&nChange) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE);
    pEffect->SetInteger(0, nChange);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectHorrified(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
    pEffect->SetInteger(0, 8);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectDamageForcePoints(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamage;
    if (g_pVirtualMachine->StackPopInteger(&nDamage) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGEFORCEPOINTS);
    pEffect->SetInteger(0, nDamage);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectHealForcePoints(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nHeal;
    if (g_pVirtualMachine->StackPopInteger(&nHeal) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HEALFORCEPOINTS);
    pEffect->SetInteger(0, nHeal);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryForcePoints(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (g_pVirtualMachine->StackPopInteger(&nAmount) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TEMPORARY_FORCE);
    pEffect->SetInteger(0, nAmount);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectChoke(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
    pEffect->SetInteger(0, 7);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectBodyFuel(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BODYFUEL);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectPsychicStatic(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_PSYCHICSTATIC);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// In the operand-less effect constructors, the effect carries only its subtype, and OBJECT_SELF's
// own id is stamped as the creator after one last resolve of the object array.
int CSWVirtualMachineCommands::ExecuteCommandEffectForceSight(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCE_SIGHT);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectFury(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FURY);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// In the one-operand "amount" effects, the creator is stamped unconditionally, because the amount
// was just read off the stack.
int CSWVirtualMachineCommands::ExecuteCommandEffectForceResistanceDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_DECREASE);
    pEffect->SetCreator(m_oidSelf);
    pEffect->SetInteger(0, nAmount);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectForceResistanceIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE);
    pEffect->SetCreator(m_oidSelf);
    pEffect->SetInteger(0, nAmount);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectFactionModifier(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FACTION_MODIFIER);
    pEffect->SetInteger(0, nAmount);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectForceBody(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCE_BODY);
    pEffect->SetInteger(0, nAmount);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectFPRegenModifier(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FPREGENMODIFIER);
    pEffect->SetInteger(0, nAmount);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectPoison(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_POISON);
    pEffect->SetInteger(0, nAmount);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The script-facing cap is five extra attacks; a larger request produces an EFFECT_INVALIDEFFECT
// placeholder.
int CSWVirtualMachineCommands::ExecuteCommandEffectModifyAttacks(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNumAttacks;
    if (!g_pVirtualMachine->StackPopInteger(&nNumAttacks))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (nNumAttacks >= 6) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_MODIFYNUMATTACKS);
        pEffect->SetCreator(m_oidSelf);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
        pEffect->SetInteger(0, nNumAttacks);
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Rides the concealment subtype with a percentage in slot 0 and the miss-chance flavour in slot 1
// (always the Normal flavour here). An out-of-range percentage takes the same placeholder as
// EffectModifyAttacks.
int CSWVirtualMachineCommands::ExecuteCommandEffectMissChance(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPercent;
    if (!g_pVirtualMachine->StackPopInteger(&nPercent))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (static_cast<unsigned int>(nPercent - 1) >= 100) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
        pEffect->SetCreator(m_oidSelf);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_CONCEALMENT);
        pEffect->SetInteger(0, nPercent);
        pEffect->SetInteger(1, CSWSEffectListHandler::MISS_CHANCE_TYPE_NORMAL);
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Shares EffectMissChance's subtype and range check; slot 1 takes the rules table's race count, so
// the concealment applies to every race.
int CSWVirtualMachineCommands::ExecuteCommandEffectConcealment(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPercent;
    if (!g_pVirtualMachine->StackPopInteger(&nPercent))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (static_cast<unsigned int>(nPercent - 1) >= 100) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->SetCreator(m_oidSelf);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_CONCEALMENT);
        pEffect->SetInteger(0, nPercent);
        pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Only three of the script-visible invisibility types build a real effect (1, 2 and 4); the rest
// take the placeholder. Integer slot 1 is the race count, not a miss-chance flavour.
int CSWVirtualMachineCommands::ExecuteCommandEffectInvisibility(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nType;
    if (!g_pVirtualMachine->StackPopInteger(&nType))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    switch (nType) {
    case INVISIBILITY_TYPE_NORMAL:
    case INVISIBILITY_TYPE_DARKNESS:
    case INVISIBILITY_TYPE_IMPROVED:
        pEffect->SetCreator(m_oidSelf);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVISIBILITY);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
        pEffect->SetInteger(0, nType);
        pEffect->SetInteger(1, g_pRules->m_nNumRaces);
        break;
    default:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
        break;
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The one member of this run that takes a float, and the one that asks for an instantaneous
// duration. A zero delta is rejected outright.
int CSWVirtualMachineCommands::ExecuteCommandEffectHitPointChangeWhenDying(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fDelta;
    if (!g_pVirtualMachine->StackPopFloat(&fDelta))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (fDelta == 0.0f) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->SetCreator(m_oidSelf);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HITPOINTCHANGEWHENDYING);
        pEffect->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pEffect->SetFloat(0, fDelta);
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The only effect constructor here that takes object ids, up to three targets in slots 0..2, plus a
// mode integer that is popped and never read. Operands past the first are gated on nArgCount and
// the ids default to OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandEffectLightsaberThrow(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    unsigned long long oidFirst = OBJECT_INVALID;
    unsigned long long oidSecond = OBJECT_INVALID;
    unsigned long long oidThird = OBJECT_INVALID;
    int nMode = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidFirst))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopObject(&oidSecond))
            return VM_ERROR_POP_FAILED;
    }
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopObject(&oidThird))
            return VM_ERROR_POP_FAILED;
    }
    if (nArgCount >= 4) {
        if (!g_pVirtualMachine->StackPopInteger(&nMode))
            return VM_ERROR_POP_FAILED;
    }
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LIGHTSABERTHROW);
    pEffect->SetObjectID(0, oidFirst);
    pEffect->SetObjectID(1, oidSecond);
    pEffect->SetObjectID(2, oidThird);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Has no script operand at all; slot 0 holds a hardcoded 16.
int CSWVirtualMachineCommands::ExecuteCommandEffectBlind(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BLINDNESS);
    pEffect->SetInteger(0, 16);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Two engine-structure pops (both at index 0); both failures report -2000 rather than -2001. The
// subtype is committed before the pops and the effect is told to re-link itself after the creator
// lands.
int CSWVirtualMachineCommands::ExecuteCommandEffectLinkEffects(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    void* pSecond;
    void* pFirst;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LINK);
    if (g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, &pFirst) == 0)
        return VM_ERROR_PUSH_FAILED;
    if (g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, &pSecond) == 0)
        return VM_ERROR_PUSH_FAILED;
    pEffect->SetLinked(static_cast<CGameEffect*>(pFirst), static_cast<CGameEffect*>(pSecond));
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(m_oidSelf);
    pEffect->UpdateLinked();
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The only two members of the run that gate the whole body on the exact argument count.
int CSWVirtualMachineCommands::ExecuteCommandEffectMovementSpeedDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    if (nArgCount != 1)
        return VM_ERROR_POP_FAILED;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectMovementSpeedIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    if (nArgCount != 1)
        return VM_ERROR_POP_FAILED;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectResurrection(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1  // K1 has no heal-amount argument
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUPERNATURAL_CLEAR_MASK) | SUBTYPE_MAGICAL;
#else
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
#endif
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Two stacked operands (ability index, amount). The ability index is clamped into
// [0, EFFECT_ABILITY_CHARISMA]; an out-of-range request falls back to strength.
int CSWVirtualMachineCommands::ExecuteCommandEffectAbilityDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAbility;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAbility))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (nAbility < 0 || nAbility > CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA)
        nAbility = CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAbility);
    pEffect->SetInteger(1, nAmount);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectAbilityIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAbility;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAbility))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (nAbility < 0 || nAbility > CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA)
        nAbility = CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAbility);
    pEffect->SetInteger(1, nAmount);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Same clamp-and-default shape as the ability pair, but the second operand (the AC class, up to
// AC_DEFLECTION, defaulting to AC_DODGE) is the one clamped, and a third operand gated on
// nArgCount >= 3 carries the damage-type flag. Fills integer slots 0, 1, 2 and 5.
int CSWVirtualMachineCommands::ExecuteCommandEffectACDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    int nValue;
    int nACType;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nACType))
        return VM_ERROR_POP_FAILED;
    int nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
            return VM_ERROR_POP_FAILED;
    }
    if (nACType < 0 || nACType > CSWSEffectListHandler::AC_DEFLECTION)
        nACType = 0;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nACType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    pEffect->SetInteger(5, nDamageType);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectACIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    int nValue;
    int nACType;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nACType))
        return VM_ERROR_POP_FAILED;
    int nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
            return VM_ERROR_POP_FAILED;
    }
    if (nACType < 0 || nACType > CSWSEffectListHandler::AC_DEFLECTION)
        nACType = 0;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nACType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    pEffect->SetInteger(5, nDamageType);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// There is no nArgCount gate, just exactly two operands, the second (the bonus class) clamped to
// [0, ATTACK_BONUS_UNARMED] with ATTACK_BONUS_MISC as the fallback.
int CSWVirtualMachineCommands::ExecuteCommandEffectAttackDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nValue;
    int nBonusType;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nBonusType))
        return VM_ERROR_POP_FAILED;
    if (nBonusType < 0 || nBonusType > CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
        nBonusType = CSWSEffectListHandler::ATTACK_BONUS_MISC;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, nBonusType);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectAttackIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nValue;
    int nBonusType;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nBonusType))
        return VM_ERROR_POP_FAILED;
    if (nBonusType < 0 || nBonusType > CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
        nBonusType = CSWSEffectListHandler::ATTACK_BONUS_MISC;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, nBonusType);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// In the damage-flag family, three clamp their damage-type operand to 0..DAMAGE_FLAG_POISON,
// falling back to DAMAGE_FLAG_UNIVERSAL, and all stamp the race count into the slots the consumer
// scans per race. The two locals are declared in the reverse of pop order because the first pop
// takes the lower address-taken slot.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamageDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamageType;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (static_cast<unsigned int>(nAmount - 1) >= 10)
        nAmount = 0;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
    pEffect->SetInteger(1, nDamageType);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectDamageIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamageType;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (static_cast<unsigned int>(nAmount - 1) >= 10)
        nAmount = 0;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
    pEffect->SetInteger(1, nDamageType);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Three operands and no amount clamp; the damage type is the first pop and fills slot 0.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamageResistance(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    int nDamageType;
    int nLimit;
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nLimit))
        return VM_ERROR_POP_FAILED;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nDamageType);
    pEffect->SetInteger(1, nAmount);
    pEffect->SetInteger(2, nLimit);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Three operands; the one range-limited is the power (second pop, up to DAMAGE_POWER_ENERGY,
// falling back to DAMAGE_POWER_NORMAL). The amount and the limit are both floored at zero.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamageReduction(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    int nPower;
    int nLimit;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nPower))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nLimit))
        return VM_ERROR_POP_FAILED;
    if (nPower < 0 || nPower > CSWSEffectListHandler::DAMAGE_POWER_ENERGY)
        nPower = CSWSEffectListHandler::DAMAGE_POWER_NORMAL;
    nAmount = nAmount < 0 ? 0 : nAmount;
    nLimit = nLimit < 0 ? 0 : nLimit;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nAmount);
    pEffect->SetInteger(1, nPower);
    pEffect->SetInteger(2, nLimit);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The only member of the family with an unconditional creator stamp and no object-array resolve.
// Its first operand is capped at 10001, the second clamped to 1..10 and the third is the usual
// damage-type fallback.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamageShield(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamage;
    int nAmount;
    int nDamageType;
    if (!g_pVirtualMachine->StackPopInteger(&nDamage))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (static_cast<unsigned int>(nDamage) >= 10001)
        nDamage = 1;
    if (static_cast<unsigned int>(nAmount - 1) >= 10)
        nAmount = 0;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nDamage);
    pEffect->SetInteger(1, nAmount);
    pEffect->SetInteger(2, nDamageType);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// These two build the effect before popping anything. The amount is clamped to a +/-100 window:
// Decrease clamps both ends, Increase only the top.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamageImmunityDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamageType;
    int nAmount;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE);
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    nAmount = nAmount > 100 ? 100 : nAmount;
    nAmount = nAmount < -100 ? -100 : nAmount;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(m_oidSelf);
    pEffect->SetInteger(0, nDamageType);
    pEffect->SetInteger(1, nAmount);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectDamageImmunityIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDamageType;
    int nAmount;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE);
    if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    nAmount = nAmount > 100 ? 100 : nAmount;
    nAmount = nAmount < 0 ? 0 : nAmount;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(m_oidSelf);
    pEffect->SetInteger(0, nDamageType);
    pEffect->SetInteger(1, nAmount);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The save class is the first pop and is rejected outright outside the rules table's four
// categories; the second operand is the bonus and a third (from 3 arguments, initialised to 0)
// goes to integer slot 2. Integer slot 3 is the race count.
int CSWVirtualMachineCommands::ExecuteCommandEffectSavingThrowDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    int nSaveType;
    int nValue;
    int nAmount = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nSaveType))
        return VM_ERROR_POP_FAILED;
    if (static_cast<unsigned int>(nSaveType) >= 4)
        nSaveType = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nAmount))
            return VM_ERROR_POP_FAILED;
    }
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, nSaveType);
    pEffect->SetInteger(2, nAmount);
    pEffect->SetInteger(3, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectSavingThrowIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    int nSaveType;
    int nValue;
    int nAmount = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nSaveType))
        return VM_ERROR_POP_FAILED;
    if (static_cast<unsigned int>(nSaveType) >= 4)
        nSaveType = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nAmount))
            return VM_ERROR_POP_FAILED;
    }
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, nSaveType);
    pEffect->SetInteger(2, nAmount);
    pEffect->SetInteger(3, g_pRules->m_nNumRaces);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The only string-bearing constructor in the run. It takes one int plus three stacked CExoString
// operands, each landing in a SetString slot with three zero floats in the matching SetFloat slots.
int CSWVirtualMachineCommands::ExecuteCommandEffectAreaEffect(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString s0;
    CExoString s1;
    CExoString s2;
    int nType;
    if (!g_pVirtualMachine->StackPopInteger(&nType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&s0))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&s1))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&s2))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AREA_OF_EFFECT);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nType);
    pEffect->SetFloat(0, 0.0f);
    pEffect->SetFloat(1, 0.0f);
    pEffect->SetFloat(2, 0.0f);
    pEffect->SetString(0, s0);
    pEffect->SetString(1, s1);
    pEffect->SetString(2, s2);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Takes four operands, with a real guard on the payload. The visual-effect id must name a
// beam-family effect, otherwise the whole payload is skipped and the placeholder is used. The
// creator is stamped before the integers and the duration bits last.
int CSWVirtualMachineCommands::ExecuteCommandEffectBeam(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nA;
    int nVisualEffectId;
    int nB = 0;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopInteger(&nVisualEffectId))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nA))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nB))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (CLoopingVisualEffect::GetIsBeam(static_cast<unsigned short>(nVisualEffectId))) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BEAM);
        pEffect->SetCreator(m_oidSelf);
        pEffect->SetInteger(0, nVisualEffectId);
        pEffect->SetInteger(1, nA);
        pEffect->SetInteger(2, nB);
        pEffect->SetObjectID(0, oidTarget);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Builds the whole 21-integer damage vector: slots 0..14 start at -1, the total lands in slot 14 and
// in the per-damage-type bucket, one slot holds the fixed 1000 save-format version stamp, and the
// next two repeat the type flag and the power. The type and power operands are optional: with 1
// argument the type defaults to 3, with 2 the power to DAMAGE_POWER_NORMAL.
int CSWVirtualMachineCommands::ExecuteCommandEffectDamage(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    int nDamage;
    int nDamageType;
    int nPower;
    if (!g_pVirtualMachine->StackPopInteger(&nDamage))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nDamageType))
            return VM_ERROR_POP_FAILED;
    } else {
        nDamageType = 3;
    }
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nPower))
            return VM_ERROR_POP_FAILED;
    } else {
        nPower = CSWSEffectListHandler::DAMAGE_POWER_NORMAL;
    }
    if (static_cast<unsigned int>(nDamage) >= 10001)
        nDamage = 1;
    if (nDamageType < 0 || nDamageType > CSWSEffectListHandler::DAMAGE_FLAG_POISON)
        nDamageType = CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL;
    if (nPower < 0 || nPower > CSWSEffectListHandler::DAMAGE_POWER_ENERGY)
        nPower = CSWSEffectListHandler::DAMAGE_POWER_NORMAL;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
    pEffect->SetNumIntegers(21);
    for (int i = 0; i < 15; i++)
        pEffect->SetInteger(i, -1);
    pEffect->SetInteger(Log_Base2(nDamageType), nDamage);
    pEffect->SetInteger(14, nDamage);
    pEffect->SetInteger(16, 1000);
    pEffect->SetInteger(17, nDamageType);
    pEffect->SetInteger(18, nPower);
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// One operand, with the same placeholder arm as EffectHeal, but only one integer slot: a forcedrain
// payload is a single amount.
int CSWVirtualMachineCommands::ExecuteCommandEffectForceDrain(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    if (nAmount >= 1) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEDRAIN);
        pEffect->SetNumIntegers(1);
        pEffect->SetInteger(0, nAmount);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Exactly one argument; a non-positive amount gets the placeholder. The creator stamp is guarded on
// the creature cast rather than a bare null test, so the cast doubles as the null check.
int CSWVirtualMachineCommands::ExecuteCommandEffectHeal(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand);
    if (nArgCount != 1)
        return VM_ERROR_POP_FAILED;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    if (nAmount >= 1) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HEAL);
        pEffect->SetInteger(0, nAmount);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The effect is built and stamped with its creator before the operand is popped. The immunity class
// saturates at IMMUNITY_TYPE_MAX rather than falling back, so the order of the two clamps matters.
// Integer slot 1 is the race count.
int CSWVirtualMachineCommands::ExecuteCommandEffectImmunity(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    int nType;
    if (!g_pVirtualMachine->StackPopInteger(&nType))
        return VM_ERROR_POP_FAILED;
    nType = CSWSEffectListHandler::IMMUNITY_TYPE_MAX < nType
                ? CSWSEffectListHandler::IMMUNITY_TYPE_MAX : nType;
    nType = nType < 0 ? 0 : nType;
    pEffect->SetInteger(0, nType);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The action-queue dispatchers pop the script's operands, resolve the acting object through
// m_oidSelf, and hang the requested action off CSWSObject's AddAction.

int CSWVirtualMachineCommands::ExecuteCommandActionDoCommand(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    void* pScriptSituation = 0;
    if (g_pVirtualMachine->StackPopCommand(&pScriptSituation) == 0)
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS)
        static_cast<CSWSObject*>(pObject)->AddDoCommandAction(pScriptSituation);
    return 0;
}

// Posts a script event to an object's event queue as a zero-delay AI-master event. The event
// structure belongs to the queue; if the target does not resolve it is leaked. The passed-in id
// degrades to OBJECT_INVALID when OBJECT_SELF no longer resolves.
int CSWVirtualMachineCommands::ExecuteCommandSignalEvent(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    void* pEventStruct;
    CGameObject* pSelfObject;
    CGameObject* pTargetObject;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EVENT, &pEventStruct))
        return VM_ERROR_POP_FAILED;
    void* pEvent = pEventStruct;
    unsigned long long oidCaller;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pSelfObject)
            == CGameObjectArray::SUCCESS)
        oidCaller = m_oidSelf;
    else
        oidCaller = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pTargetObject)
            == CGameObjectArray::SUCCESS)
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, oidCaller, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    return 0;
}

// Re-queues the current script's continuation on OBJECT_SELF after a delay in seconds, converted to
// the AI master's millisecond ticks. It refuses outside a real action script, and on that path it
// frees the situation itself.
int CSWVirtualMachineCommands::ExecuteCommandDelayCommand(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fDelay;
    void* pScriptSituation = 0;
    CGameObject* pObject;
    if (g_pVirtualMachine->StackPopFloat(&fDelay) == 0)
        return VM_ERROR_POP_FAILED;
    if (g_pVirtualMachine->StackPopCommand(&pScriptSituation) == 0)
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS) {
        unsigned long long oidCaller =
            m_nScriptRunArg == 1 ? m_oidSelf : CGameObjectArray::INVALIDOBJID;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, static_cast<int>(fDelay * 1000.0f), oidCaller, m_oidSelf,
            CServerAIMaster::EVENT_TIMED_EVENT, pScriptSituation);
    } else if (pScriptSituation != 0) {
        operator delete(pScriptSituation);
        pScriptSituation = 0;
    }
    return 0;
}

// AssignCommand's twin of DelayCommand: the situation is queued as a zero-delay timed event on the
// AI master against another object, so it runs with that object as OBJECT_SELF. The event's caller
// id is OBJECT_SELF only inside a real action script. If the target object cannot be resolved the
// command frees the situation itself, with a bare operator delete and no destructor call.

int CSWVirtualMachineCommands::ExecuteCommandAssignCommand(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    if (g_pVirtualMachine->StackPopObject(&oidTarget) == 0)
        return VM_ERROR_POP_FAILED;
    void* pScriptSituation = 0;
    if (g_pVirtualMachine->StackPopCommand(&pScriptSituation) == 0)
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            == CGameObjectArray::SUCCESS) {
        unsigned long long oidCaller =
            m_nScriptRunArg == 1 ? m_oidSelf : CGameObjectArray::INVALIDOBJID;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, oidCaller, oidTarget, CServerAIMaster::EVENT_TIMED_EVENT, pScriptSituation);
    } else if (pScriptSituation != 0) {
        operator delete(pScriptSituation);
        pScriptSituation = 0;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandActionEquipMostDamagingMelee(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oid;
    int n;
    if (g_pVirtualMachine->StackPopObject(&oid) == 0 || g_pVirtualMachine->StackPopInteger(&n) == 0)
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->EquipMostDamagingMeleeWeapon(oid, n, 0);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandActionEquipMostDamagingRanged(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oid;
    if (g_pVirtualMachine->StackPopObject(&oid) == 0)
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0 && pCreature->EquipMostDamagingRangedWeapon(oid) == 0)
        pCreature->EquipMostDamagingMeleeWeapon(oid, 0, 0);
    return 0;
}

#if KOTOR1
extern int RenderPartyFollow;
#endif
int CSWVirtualMachineCommands::ExecuteCommandActionFollowLeader(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
#if KOTOR1  // K1 reads the party-follow debug toggle
    if (pCreature != 0 && pCreature->m_bInParty != 0 && pCreature->m_bCommandable == 1) {
        if (RenderPartyFollow) {
            const char* pszTag = pCreature->m_sTag.CStr();
            (void)pszTag;
        }
        pCreature->AddAction(AIACTION_PARTY_FOLLOW_LEADER, ACTION_GROUP_NEW);
    }
#else
    if (pCreature != 0 && pCreature->m_bInParty != 0 && pCreature->m_bCommandable == 1)
        pCreature->AddAction(AIACTION_PARTY_FOLLOW_LEADER, ACTION_GROUP_NEW);
#endif
    return 0;
}

// Queues the creature's use of a feat against a target. The acting creature must be in an area,
// whose id rides along as UseFeat's last argument.
int CSWVirtualMachineCommands::ExecuteCommandActionUseFeat(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nFeat;
    unsigned long long oidTarget;
    CGameObject* pObject;
    if (g_pVirtualMachine->StackPopInteger(&nFeat) == 0)
        return VM_ERROR_POP_FAILED;
    if (g_pVirtualMachine->StackPopObject(&oidTarget) == 0)
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0
        && pObject->AsSWSCreature()->GetArea() != 0)
        pObject->AsSWSCreature()->UseFeat(
            static_cast<unsigned short>(nFeat), 0, oidTarget,
            static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea())->m_objectId);
    return 0;
}

// The owner-follow twin of ActionFollowLeader has the same puppet/commandable gate, but the follow
// distance comes off the VM stack and the creature is resolved before the pop.

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandActionFollowOwner(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    float fRange = 0.0f;
    if (g_pVirtualMachine->StackPopFloat(&fRange) == 0)
        return VM_ERROR_POP_FAILED;
    if (pCreature != 0 && pCreature->m_isPuppet != 0 && pCreature->m_bCommandable == 1)
        pCreature->AddAction(AIACTION_FOLLOW_OWNER, ACTION_GROUP_NEW, ACTION_PARAM_FLOAT, &fRange);
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandActionBarkString(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStrRef;
    if (g_pVirtualMachine->StackPopInteger(&nStrRef) == 0)
        return VM_ERROR_POP_FAILED;
    // Barks are suppressed for the whole duration of a conversation; the action is simply dropped.
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
        if (pCreature != 0)
            pCreature->AddAction(AIACTION_BARK_STRING, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nStrRef);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandRandomName(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    // Builds a throw-away generator, pulls one name and tears the table down, so every call
    // re-reads the resource.
    CSWNameGen nameGen;
    CExoString sName;
    nameGen.LoadNameTable(CExoString("names"));
    sName = nameGen.GetRandomName(0);
    nameGen.UnloadNameTable();
    if (g_pVirtualMachine->StackPushString(sName) == 0)
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandActionInteractObject(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oid;
    if (g_pVirtualMachine->StackPopObject(&oid) == 0)
        return VM_ERROR_POP_FAILED;
    CSWSPlaceable* pPlaceable = g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oid);
    if (pPlaceable != 0) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
        if (pCreature != 0)
            pCreature->AddUseObjectAction(pPlaceable->m_objectId);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPickUpItem(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oid;
    if (g_pVirtualMachine->StackPopObject(&oid) == 0)
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->AddPickUpItemActions(oid, OBJECT_INVALID, 0xff);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPutDownItem(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oid;
    if (g_pVirtualMachine->StackPopObject(&oid) == 0)
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->AddDropItemActions(oid, pCreature->m_position, 0);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandActionSwitchWeapons(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->AddAction(AIACTION_SWITCH_WEAPONS, ACTION_GROUP_NEW);
    return 0;
}

// Each GetLast* combat-bookkeeping getter pops a creature id (seeded to OBJECT_INVALID before the
// pop), reads one creature slot, and defaults to OBJECT_INVALID for the object-valued ones and 0
// for the int-valued ones.

int CSWVirtualMachineCommands::ExecuteCommandGetLastAttackAction(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nAction = pCreature ? pCreature->m_nLastAttackAction : 0;
    if (!g_pVirtualMachine->StackPushInteger(nAction))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastAttackResult(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nResult = pCreature ? pCreature->m_nLastAttackResult : 0;
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastHostileTarget(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidTarget =
        pCreature ? pCreature->m_oidLastHostileTarget : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastCombatFeatUsed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nFeat = pCreature ? pCreature->m_nLastCombatFeatUsed : 0;
    if (!g_pVirtualMachine->StackPushInteger(nFeat))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastForcePowerUsed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nPower = pCreature ? pCreature->m_nLastForcePowerUsed : 0;
    if (!g_pVirtualMachine->StackPushInteger(nPower))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetWasForcePowerSuccessful(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bSuccess = pCreature ? pCreature->m_bWasForcePowerSuccessful : 0;
    if (!g_pVirtualMachine->StackPushInteger(bSuccess))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The Is* creature predicates share the pop/lookup/push frame of the GetLast* block, differing
// only in how each derives its int -- a nested field read, a client-object hop, or a helper call.
// Every null arm falls into the same push 0.

int CSWVirtualMachineCommands::ExecuteCommandIsPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 asks whether a player client owns the object (no stats m_bIsPC)
    int bIsPC = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidCreature) != 0;
#else
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bIsPC = (pCreature && pCreature->m_pStats) ? pCreature->m_pStats->m_bIsPC : 0;
#endif
    if (!g_pVirtualMachine->StackPushInteger(bIsPC))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsRunning(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    // The walk/run bit lives on the client-side creature, so this is a two-hop null chain.
    CSWCCreature* pClientCreature = pCreature ? pCreature->GetClientCreature() : 0;
    int bRunning = pClientCreature ? pClientCreature->m_bRunning : 0;
    if (!g_pVirtualMachine->StackPushInteger(bRunning))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsStealthed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bStealthed = pCreature ? pCreature->GetActivity(CSWSCreature::ACTIVITY_STEALTH) : 0;
    if (!g_pVirtualMachine->StackPushInteger(bStealthed))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandIsMeditating(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bMeditating = pCreature ? (pCreature->m_nStancePosture == CSWSCreature::STANCE_POSTURE_MEDITATIVE) : 0;
    if (!g_pVirtualMachine->StackPushInteger(bMeditating))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandIsInTotalDefense(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bTotalDefense = pCreature ? (pCreature->IsInTotalDefense() != 0) : 0;
    if (!g_pVirtualMachine->StackPushInteger(bTotalDefense))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

int CSWVirtualMachineCommands::ExecuteCommandGetLastWeaponUsed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidWeapon =
        pCreature ? pCreature->m_oidLastWeaponUsed : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidWeapon))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandIsObjectPartyMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bInParty = pCreature ? pCreature->m_bInParty : 0;
    if (!g_pVirtualMachine->StackPushInteger(bInParty))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The two error codes are transposed against every sibling command. The stack-pop failure returns
// -2000 and the push failure -2001.
int CSWVirtualMachineCommands::ExecuteCommandGetGoingToBeAttackedBy(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_PUSH_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidAttacker =
        pCreature ? pCreature->m_oidGoingToBeAttackedBy : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidAttacker))
        return VM_ERROR_POP_FAILED;
    return 0;
}

// Takes no argument: the last conversation string is app state, not per-object. The success test is
// inverted against the rest of the file, so the `if` is written straight rather than negated. The
// pushed string is a temporary, so it dies with the full-expression.
int CSWVirtualMachineCommands::ExecuteCommandGetLastConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    if (g_pVirtualMachine->StackPushString(
            g_pAppManager->m_pServerExoApp->GetLastConversation()))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// m_pStats is dereferenced unchecked here; only the creature is null-tested, unlike its
// GetChallengeRating and GetSubRace neighbours.
int CSWVirtualMachineCommands::ExecuteCommandGetAppearanceType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nAppearance = pCreature ? pCreature->m_pStats->m_nAppearanceType : 0;
    if (!g_pVirtualMachine->StackPushInteger(nAppearance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetChallengeRating(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    float fRating = 0.0f;
    if (pCreature && pCreature->m_pStats)
        fRating = pCreature->m_pStats->m_fChallengeRating;
    if (!g_pVirtualMachine->StackPushFloat(fRating))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// One body behind three script commands, dispatched on nCommand rather than split: 341
// GetClassByPosition, 342 GetLevelByPosition and 343 GetLevelByClass. The first two are
// position-keyed (1-based) and share the same num-classes guard; the third sweeps both multiclass
// slots for the popped class id. The three tests are sequential ifs, not an else-if chain, and
// 341's extra < 33 clamp is the classes.2da row bound.
int CSWVirtualMachineCommands::ExecuteCommandGetClassInformation(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nPosition;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopInteger(&nPosition))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nResult = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0) {
        if (nCommand == CMD_GetClassByPosition) {
            nResult = 255;
            if (nPosition > 0 && nPosition <= pCreature->m_pStats->m_numClasses) {
#if KOTOR1  // K1 calls GetClass three times, discarding the first result
                pCreature->m_pStats->GetClass(static_cast<unsigned char>(nPosition - 1));
                if (pCreature->m_pStats->GetClass(static_cast<unsigned char>(nPosition - 1)) < 33)
                    nResult = pCreature->m_pStats->GetClass(static_cast<unsigned char>(nPosition - 1));
#else
                unsigned char nClass = pCreature->m_pStats->GetClass(static_cast<unsigned char>(nPosition - 1));
                if (nClass < 33)
                    nResult = nClass;
#endif
            }
        }
        if (nCommand == CMD_GetLevelByPosition) {
            if (nPosition > 0 && nPosition <= pCreature->m_pStats->m_numClasses)
                nResult = pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(nPosition - 1), 0);
        }
        if (nCommand == CMD_GetLevelByClass) {
            for (unsigned char nSlot = 0; nSlot < 2; nSlot++) {
                if (pCreature->m_pStats->GetClass(nSlot) == nPosition)
                    nResult = pCreature->m_pStats->GetClassLevel(nSlot, 0);
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// nSubrace is left uninitialised on both null arms here, unlike every sibling command.
int CSWVirtualMachineCommands::ExecuteCommandGetSubRace(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nSubrace;  // uninitialised when the creature or its stats are missing  // idiom-exempt: deliberately uninitialized — see the note above
#ifdef K2_FIX_ENGINE_BUGS
    nSubrace = 0;
#endif
    if (pCreature && pCreature->m_pStats)
        nSubrace = pCreature->m_pStats->m_nSubraceIndex;
    if (!g_pVirtualMachine->StackPushInteger(nSubrace))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The battle track is area state, so the argument is an area id and the track lives on the area's
// ambient sound, dereferenced unchecked.
int CSWVirtualMachineCommands::ExecuteCommandGetBattleTrack(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidArea;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    int nTrack = pArea ? pArea->m_pAmbientSound->m_nBattleMusicTrack : 0;
    if (!g_pVirtualMachine->StackPushInteger(nTrack))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Suppresses click handling for a duration in seconds, which the GUI wants in milliseconds. A
// non-positive duration is dropped. The pop failure reports -2000, not the usual -2001.
int CSWVirtualMachineCommands::ExecuteCommandNoClicksFor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fSeconds = 0.0f;
    if (!g_pVirtualMachine->StackPopFloat(&fSeconds))
        return VM_ERROR_PUSH_FAILED;
    if (fSeconds > 0.0f) {
        unsigned int nMilliseconds = static_cast<unsigned int>(fSeconds * 1000.0f);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->MarkNoClickEvent(nMilliseconds);
    }
    return 0;
}

// All five GetEffect* accessors pop an engine structure, read one thing off it and push. A
// successful push consumes the structure; both failure paths return early and leak it.

int CSWVirtualMachineCommands::ExecuteCommandGetEffectDurationType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPushInteger(pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_MASK))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The subtype selector is bits 3-4 of the same field, pushed unshifted.
int CSWVirtualMachineCommands::ExecuteCommandGetEffectSubType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPushInteger(pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_MASK))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetEffectSpellId(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPushInteger(pEffect->m_spellId))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The only one of the five that null-guards the popped pointer; the others dereference it blind.
int CSWVirtualMachineCommands::ExecuteCommandGetEffectCreator(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidCreator = pEffect ? pEffect->m_creatorId : OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidCreator))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetEffectType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    int nEffectType = pEffect->GetEffectTypeScriptConstant();
    if (!g_pVirtualMachine->StackPushInteger(nEffectType))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    pEffect = 0;  // idiom-exempt: the orig's `str xzr,[sp]` — see the note above
    return 0;
}

// The two vision effects use the same constructor skeleton as the EffectDroidStun family plus a
// subtype stamp. Both force the script subtype selector to SUBTYPE_SUPERNATURAL (8) while leaving
// the duration selector in the low bits intact.
int CSWVirtualMachineCommands::ExecuteCommandEffectSeeInvisible(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SEEINVISIBLE);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectTrueSeeing(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TRUESEEING);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The two skill-modifier effects pop the skill id then the amount, stamp SUBTYPE_SUPERNATURAL like
// the vision pair, and validate the skill against skills.2da. The valid window is 1..numSkills-1
// plus the SKILL_ALL_SKILLS sentinel; anything else downgrades the effect to EFFECT_INVALIDEFFECT
// rather than failing the command. Integer slot 2 takes the race count, so the effect applies to
// every race. These two never delete the effect, because the push takes ownership either way.
int CSWVirtualMachineCommands::ExecuteCommandEffectSkillDecrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSkill;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SKILL_DECREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (nSkill < 1 || (nSkill >= g_pRules->m_nNumSkills && nSkill != SKILL_ALL)) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->SetInteger(0, nSkill);
        pEffect->SetInteger(1, nAmount);
        pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    }
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandEffectSkillIncrease(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSkill;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    int nAmount;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SKILL_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (nSkill < 1 || (nSkill >= g_pRules->m_nNumSkills && nSkill != SKILL_ALL)) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->SetInteger(0, nSkill);
        pEffect->SetInteger(1, nAmount);
        pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    }
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The stealth-XP setters mirror the getters above, except that the player creature is resolved
// before the stack pop. A missing creature or area simply drops the value and returns 0.

int CSWVirtualMachineCommands::ExecuteCommandSetCurrentStealthXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nStealthXP;
    if (!g_pVirtualMachine->StackPopInteger(&nStealthXP))
        return VM_ERROR_POP_FAILED;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            pArea->SetCurrentStealthXP(nStealthXP);
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetMaxStealthXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nMaxStealthXP;
    if (!g_pVirtualMachine->StackPopInteger(&nMaxStealthXP))
        return VM_ERROR_POP_FAILED;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            pArea->SetMaxStealthXP(nMaxStealthXP);
    }
    return 0;
}

// These two fields have no setter method; the area field is stored directly.
int CSWVirtualMachineCommands::ExecuteCommandSetStealthXPDecrement(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nDecrement;
    if (!g_pVirtualMachine->StackPopInteger(&nDecrement))
        return VM_ERROR_POP_FAILED;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            pArea->m_stealthXPDecrement = nDecrement;
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSetStealthXPEnabled(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int bEnabled;
    if (!g_pVirtualMachine->StackPopInteger(&bEnabled))
        return VM_ERROR_POP_FAILED;
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea)
            pArea->m_bStealthXPEnabled = bEnabled;
    }
    return 0;
}

// The module/area chain is deliberately re-spelled at each use rather than cached in a local.
int CSWVirtualMachineCommands::ExecuteCommandSetRestrictMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMode;
    if (!g_pVirtualMachine->StackPopInteger(&nMode))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetModule()
            && g_pAppManager->m_pServerExoApp->GetModule()->GetArea())
        static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetModule()->GetArea())
                ->SetPlayerRestrictMode(nMode);
    return 0;
}

// The GetLast*By object-state getters pop nothing, always work on OBJECT_SELF, and get the object
// back through the server object array. They come in two shapes. The "which subclass am I" pairs
// (ClosedBy / OpenedBy / UsedBy) test the placeable cast then the door cast, while the type-tag
// switches (Locked / Unlocked / Disarmed / Disturbed) go through AsSWSObject and branch on
// m_objectType. Doors have no separate "used by" slot, so GetLastUsedBy and GetLastOpenedBy read
// the same door field. GetLastUsedBy and GetLastDisarmed leave their result uninitialised on the
// paths their guards reject.

int CSWVirtualMachineCommands::ExecuteCommandGetLastClosedBy(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidCloser = OBJECT_INVALID;
    if (pObject != 0) {
        if (pObject->AsSWSPlaceable() != 0)
            oidCloser = pObject->AsSWSPlaceable()->m_oidLastClosedBy;
        else if (pObject->AsSWSDoor() != 0)
            oidCloser = pObject->AsSWSDoor()->m_oidLastClosedBy;
    }
    if (!g_pVirtualMachine->StackPushObject(oidCloser))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastOpenedBy(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidOpener = OBJECT_INVALID;
    if (pObject != 0) {
        if (pObject->AsSWSPlaceable() != 0)
            oidOpener = pObject->AsSWSPlaceable()->m_oidLastOpenedBy;
        else if (pObject->AsSWSDoor() != 0)
            oidOpener = pObject->AsSWSDoor()->m_oidLastOpenedBy;
    }
    if (!g_pVirtualMachine->StackPushObject(oidOpener))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastUsedBy(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidUser;
    if (pObject->AsSWSPlaceable() != 0)
        oidUser = pObject->AsSWSPlaceable()->m_oidLastUsedBy;
    else if (pObject->AsSWSDoor() != 0)
        oidUser = pObject->AsSWSDoor()->m_oidLastOpenedBy;
    else
        oidUser = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidUser))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastLocked(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidLocker = OBJECT_INVALID;
#if KOTOR1  // K1 has no null test here
    pObject->AsSWSObject();
    {
#else
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
#endif
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0) {
            if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                oidLocker = pSWSObject->AsSWSDoor()->m_oidLastLockedBy;
            else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                oidLocker = pSWSObject->AsSWSPlaceable()->m_oidLastLockedBy;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidLocker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastUnlocked(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidUnlocker = OBJECT_INVALID;
#if KOTOR1  // K1 has no null test here
    pObject->AsSWSObject();
    {
#else
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
#endif
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0) {
            if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                oidUnlocker = pSWSObject->AsSWSDoor()->m_oidLastUnlockedBy;
            else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                oidUnlocker = pSWSObject->AsSWSPlaceable()->m_oidLastUnlockedBy;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidUnlocker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastDisarmed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidDisarmer;
    // The guard this call came from is gone. Every path it rejected leaves oidDisarmer
    // uninitialised, so the branches are dropped. Only the call is kept.
    pObject->AsSWSObject();  // idiom-exempt: the orig's folded-away guard, see above
    CSWSObject* pSWSObject = pObject->AsSWSObject();
    if (pSWSObject == 0)
        oidDisarmer = OBJECT_INVALID;
    else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
        oidDisarmer = pSWSObject->AsSWSPlaceable()->m_oidLastDisarmedBy;
    else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
        oidDisarmer = pSWSObject->AsSWSDoor()->m_oidLastDisarmedBy;
    else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_TRIGGER)
        oidDisarmer = pSWSObject->AsSWSTrigger()->m_oidDisarmedBy;
    else
        oidDisarmer = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPushObject(oidDisarmer))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastDisturbed(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    unsigned long long oidDisturber = OBJECT_INVALID;
#if KOTOR1  // K1 has no null test here
    pObject->AsSWSObject();
    {
#else
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
#endif
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0) {
            if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
                oidDisturber = pSWSObject->AsSWSCreature()->m_oidLastDisturbedBy;
            else if (pSWSObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                oidDisturber = pSWSObject->AsSWSPlaceable()->m_oidLastDisturbedBy;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidDisturber))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The odd one out, going through the object array's out-param lookup rather than the server's
// GetGameObject, so the object pointer is a real stack local. Its sentinel is the object array's
// invalid-object constant, not the bare literal every sibling uses.
int CSWVirtualMachineCommands::ExecuteCommandGetLastPerceived(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject);
    unsigned long long oidPerceived = CGameObjectArray::INVALIDOBJID;
    if (pObject->AsSWSCreature() != 0)
        oidPerceived = pObject->AsSWSCreature()->m_oidLastPerceived;
    if (!g_pVirtualMachine->StackPushObject(oidPerceived))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The four one-hot perception flags share this handler (257, 258, 259 and 261; 260 is
// GetLastClosedBy).
int CSWVirtualMachineCommands::ExecuteCommandGetLastPerception(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int bPerceived = 0;
    switch (nCommand) {
        case CMD_GetLastPerceptionHeard:
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject->AsSWSCreature() != 0)
                bPerceived = pObject->AsSWSCreature()->m_bLastPerceptionHeard;
            break;
        case CMD_GetLastPerceptionInaudible:
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject->AsSWSCreature() != 0)
                bPerceived = pObject->AsSWSCreature()->m_bLastPerceptionInaudible;
            break;
        case CMD_GetLastPerceptionSeen:
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject->AsSWSCreature() != 0)
                bPerceived = pObject->AsSWSCreature()->m_bLastPerceptionSeen;
            break;
        case CMD_GetLastPerceptionVanished:
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject->AsSWSCreature() != 0)
                bPerceived = pObject->AsSWSCreature()->m_bLastPerceptionVanished;
            break;
    }
    if (!g_pVirtualMachine->StackPushInteger(bPerceived))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The two stragglers of the family. Both take the double-cast route, but GetLastAttacker pops a
// target (seeded to OBJECT_INVALID before the pop) and GetLastSpeaker goes through the object
// array's status-returning lookup.

int CSWVirtualMachineCommands::ExecuteCommandGetLastAttacker(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    unsigned long long oidAttacker = OBJECT_INVALID;
    if (pObject != 0 && pObject->AsSWSObject() != 0)
        oidAttacker = pObject->AsSWSObject()->m_oidLastAttacker;
    if (!g_pVirtualMachine->StackPushObject(oidAttacker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetLastSpeaker(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidSpeaker = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject->AsSWSCreature() != 0)
        oidSpeaker = pObject->AsSWSCreature()->m_oidLastSpeaker;
    if (!g_pVirtualMachine->StackPushObject(oidSpeaker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Neither handler below null-checks the mini-game object the table hands back; the probe is invoked
// straight on it. The obstacle/follower view is checked afterwards, so an id that names the wrong
// kind of object is safe, but an unknown id crashes.

// Starts an overlay (upper-body) animation on a creature. The handler recognises exactly one
// animation id and hard-codes the server animation it maps; every other id is accepted and
// silently dropped. The server-to-client mapping is asked of the anim base itself, so the same
// script id can resolve differently per appearance.
// ActionJumpToPoint (command 214) and its to-front twin (313): pops a location, copies its position
// and orientation into locals (the location is destroyed right after), then queues action 5 with the
// eight (type, &value) parameter pairs the action node wants. Only a commandable creature standing
// in a real area gets the action.
// The area's object id is read before the null check, so a creature with no area dereferences NULL.
int CSWVirtualMachineCommands::ExecuteCommandActionJumpToPoint(int nCommand, int nArgCount)
{
    Vector vPosition(-1.0f, -1.0f, -1.0f);
    unsigned long long oidArea;
    int bWalkStraightLine = 1;
    Vector vOrientation(0.0f, 0.0f, 0.0f);
    CScriptLocation* pLocation;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    vPosition = pLocation->m_vPosition;
    vOrientation = pLocation->m_vOrientation;
    if (pLocation != 0)
        delete pLocation;
    pLocation = 0;
    if (nArgCount > 1) {
        if (!g_pVirtualMachine->StackPopInteger(&bWalkStraightLine))
            return VM_ERROR_POP_FAILED;
    }
    if (m_nScriptRunArg == 1) {
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                    == CGameObjectArray::SUCCESS
                && pObject->AsSWSCreature() != 0
                && pObject->AsSWSCreature()->m_bCommandable != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea());
            oidArea = pArea->m_objectId;
            if (pArea != 0 && pArea->m_objectType == CGameObject::OBJECT_TYPE_AREA) {
                // 20.0f, not 15.0: the value is handed to both arms' range parameter.
                float fRange = 20.0f;
                if (nCommand == CMD_ActionJumpToLocation)
                    static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_JUMP_TO_POINT, ACTION_GROUP_NEW,
                            ACTION_PARAM_FLOAT, &vPosition.x, ACTION_PARAM_FLOAT, &vPosition.y, ACTION_PARAM_FLOAT, &vPosition.z,
                            ACTION_PARAM_OBJECT, &oidArea, ACTION_PARAM_INT, &bWalkStraightLine, ACTION_PARAM_FLOAT, &fRange,
                            ACTION_PARAM_FLOAT, &vOrientation.x, ACTION_PARAM_FLOAT, &vOrientation.y);
                else
                    static_cast<CSWSObject*>(pObject)->AddActionToFront(AIACTION_JUMP_TO_POINT, ACTION_GROUP_NEW,
                            ACTION_PARAM_FLOAT, &vPosition.x, ACTION_PARAM_FLOAT, &vPosition.y, ACTION_PARAM_FLOAT, &vPosition.z,
                            ACTION_PARAM_OBJECT, &oidArea, ACTION_PARAM_INT, &bWalkStraightLine, ACTION_PARAM_FLOAT, &fRange,
                            ACTION_PARAM_FLOAT, &vOrientation.x, ACTION_PARAM_FLOAT, &vOrientation.y,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
        }
    }
    return 0;
}

// Moves gold off oCreature and, unless bDestroy is set, onto OBJECT_SELF -- as gold on a creature, or
// as a stack of g_i_credits001 in a placeable's or a container item's repository. The amount is
// clamped to what the creature carries, and a party-member source also gets the HUD credit counter
// updated. The OBJECT_SELF lookup result is dereferenced with no null check; only the downcast is
// guarded.
int CSWVirtualMachineCommands::ExecuteCommandTakeGoldFromCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAmount;
    int bDestroy;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bDestroy))
        return VM_ERROR_POP_FAILED;
    if (nAmount < 1)
        return 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature == 0)
        return 0;
    if (nAmount > pCreature->GetGold())
        nAmount = pCreature->GetGold();
    if (nAmount < 1)
        return 0;
    CSWSObject* pTaker = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf)->AsSWSObject();
    if (pTaker == 0)
        return 0;
    unsigned char nType = pTaker->m_objectType;
    switch (nType) {
    case GAME_OBJECT_TYPE_CREATURE: {
        pCreature->RemoveGold(nAmount, 1);
        if (bDestroy == 0) {
            CSWSCreature* pTakerCreature = pTaker->AsSWSCreature();
            if (pTakerCreature != 0)
                pTakerCreature->AddGold(nAmount, 1);
        }
        break;
    }
    case GAME_OBJECT_TYPE_PLACEABLE: {
        pCreature->RemoveGold(nAmount, 1);
        if (bDestroy == 0) {
            CSWSPlaceable* pPlaceable = pTaker->AsSWSPlaceable();
            if (pPlaceable != 0 && pPlaceable->m_bHasInventory != 0
                && pPlaceable->m_pItemRepository != 0) {
                CSWSItem* pGold = new CSWSItem(OBJECT_INVALID);
                CResRef resRef("g_i_credits001");
                pGold->LoadFromTemplate(resRef);
                pGold->m_stackCount = nAmount;
                pPlaceable->AcquireItem(&pGold, OBJECT_INVALID, 1);
            }
        }
        break;
    }
    default: {
        pCreature->RemoveGold(nAmount, 1);
        if (nType == GAME_OBJECT_TYPE_ITEM && bDestroy == 0) {  // a container
            CSWSItem* pContainer = pTaker->AsSWSItem();
            if (pContainer != 0) {
                CSWRules* pRules = g_pRules;
                int nBaseItem = pContainer->m_baseItem;
                if (pRules->GetBaseItem(nBaseItem)->m_bContainer != 0) {
                    CItemRepository* pRepository = pContainer->m_pItemRepository;
                    if (pRepository != 0) {
                        CSWSItem* pGold = new CSWSItem(OBJECT_INVALID);
                        CResRef resRef("g_i_credits001");
                        pGold->LoadFromTemplate(resRef);
                        pGold->m_stackCount = nAmount;
                        pRepository->AddItem(&pGold, 1, 0, 1);
                    }
                }
            }
        }
        break;
    }
    }
    if (pCreature->m_bInParty != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_CREDITS, -nAmount);
    return 0;
}

// StartNewModule(sModuleName, sWaypoint, sMovie1..sMovie6): stashes the destination on the server
// and queues up to six transition movies on the client, then kicks off a black global fade if the
// in-game GUI is up. The last four movie operands are popped as one group.
// All six movie strings are constructed even when only four are popped; the spare ones still go
// through AddMoveToModuleMovie, empty.
int CSWVirtualMachineCommands::ExecuteCommandStartNewModule(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sModuleName;
    if (!g_pVirtualMachine->StackPopString(&sModuleName))
        return VM_ERROR_POP_FAILED;
    CExoString sWaypoint;
    if (!g_pVirtualMachine->StackPopString(&sWaypoint))
        return VM_ERROR_POP_FAILED;
    CExoString sMovie1;
    if (!g_pVirtualMachine->StackPopString(&sMovie1))
        return VM_ERROR_POP_FAILED;
    CExoString sMovie2;
    if (!g_pVirtualMachine->StackPopString(&sMovie2))
        return VM_ERROR_POP_FAILED;
    CExoString sMovie3;
    CExoString sMovie4;
    CExoString sMovie5;
    CExoString sMovie6;
    if (nArgCount >= 5) {
        if (!g_pVirtualMachine->StackPopString(&sMovie3))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopString(&sMovie4))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopString(&sMovie5))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopString(&sMovie6))
            return VM_ERROR_POP_FAILED;
    }
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    pServer->SetMoveToModulePending(1);
    pServer->SetMoveToModuleString(sModuleName);
    pServer->SetMoveToModuleStartWaypoint(sWaypoint);
    pClient->AddMoveToModuleMovie(sMovie1);
    pClient->AddMoveToModuleMovie(sMovie2);
    pClient->AddMoveToModuleMovie(sMovie3);
    pClient->AddMoveToModuleMovie(sMovie4);
    pClient->AddMoveToModuleMovie(sMovie5);
    pClient->AddMoveToModuleMovie(sMovie6);
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui != 0 && (pInGameGui->m_nGlobalDialogState != 0 || pInGameGui->m_bRunningGuiScript != 0))
        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
            0, 0.0f, 0.0f, Vector{0.0f, 0.0f, 0.0f});
    return 0;
}

// DestroyObject(oDestroy, fDelay, bNoFade, fDelayUntilFade, nHideFeedback): the four optional
// operands are popped only as far as the argument count says. Destroying the current player
// character is special-cased first: the client walks forward through the party until a member with
// a live server-side client object turns up, that one becomes the player creature and the old one
// is dropped from the roster. The object then gets its fade parameters and is queued for
// EVENT_DESTROY_OBJECT, and its party and puppet table slots are cleared. The event's pointer slot
// carries the inverted nHideFeedback flag as a raw 0/1.
int CSWVirtualMachineCommands::ExecuteCommandDestroyObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidObject;
    float fDelay = 0.0f;
    int bNoFade = 0;
    float fDelayUntilFade = 0.0f;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nHideFeedback = 0;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopFloat(&fDelay))
            return VM_ERROR_POP_FAILED;
        if (nArgCount >= 3) {
            if (!g_pVirtualMachine->StackPopInteger(&bNoFade))
                return VM_ERROR_POP_FAILED;
            if (nArgCount >= 4) {
                if (!g_pVirtualMachine->StackPopFloat(&fDelayUntilFade))
                    return VM_ERROR_POP_FAILED;
#if !KOTOR1
                if (nArgCount >= 5) {
                    if (!g_pVirtualMachine->StackPopInteger(&nHideFeedback))
                        return VM_ERROR_POP_FAILED;
                }
#endif
            }
        }
    }
    CSWCCreature* pPlayerCharacter =
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (pPlayerCharacter != 0
        && g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pPlayerCharacter->m_objectId)
               == oidObject) {
        unsigned long long oidPlayerCharacter = pPlayerCharacter->m_objectId;
        for (unsigned char i = 0;
             i < g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount; i++) {
            g_pAppManager->m_pClientExoApp->ChangeCharacter(-2, 1);
            CSWCCreature* pNewCharacter =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            if (pNewCharacter != 0
                && g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                       g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                           pNewCharacter->m_objectId)) != 0) {
                g_pAppManager->m_pClientExoApp->SetPlayerCreature(pNewCharacter->m_objectId);
                g_pAppManager->m_pClientExoApp->AttachPlayerGameObjectToArea();
                break;
            }
        }
        g_pAppManager->m_pClientExoApp->GetSWParty()->RemoveCharacter(oidPlayerCharacter);
    }
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CGameObject* pObject = pServer->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        unsigned int nFadeOutTime = static_cast<unsigned int>(fDelayUntilFade * 1000.0f);
        pObject->AsSWSObject()->SetDeleteNoFade(bNoFade);
        pObject->AsSWSObject()->SetFadeOutTime(nFadeOutTime);
    }
    pServer->GetPartyTable()->ClearMember(pServer->GetPartyTable()->GetNPCID(oidObject));
#if !KOTOR1
    pServer->GetPartyTable()->ClearPuppet(pServer->GetPartyTable()->GetPUPID(oidObject));
#endif
    int bShowFeedback = (nHideFeedback == 0);
    pServer->GetServerAIMaster()->AddEventDeltaTime(0, static_cast<int>(fDelay * 1000.0f), m_oidSelf,
            oidObject, CServerAIMaster::EVENT_DESTROY_OBJECT, reinterpret_cast<void*>(static_cast<int64_t>(bShowFeedback)));
    return 0;
}

// CreatureFlourishWeapon: the "show off your weapon" bark, debounced by the client creature's
// switch-weapons cooldown. It powers up the equipped weapons and forces the combat stance only when
// the creature is alive; a downed party member skips straight to the animation. With player
// movement input live it drives an overlay animation, otherwise it queues the fire-and-forget
// flourish. Every early exit answers 0.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandCreatureFlourishWeapon(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject == 0)
        return 0;
    if (pObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    CSWCCreature* pClientCreature = pCreature->GetClientCreature();
    if (pClientCreature == 0)
        return 0;
    pClientCreature->ComputeWeaponOverlays(0, 1);
    if (pClientCreature->m_nSwitchWeaponsCooldown > 0)
        return 0;
    if (pCreature->GetDead() == 0
        && pCreature->GetDeadTemp() == 0) {
        unsigned char nAttackType = pClientCreature->GetAnimationBase()->m_nWeaponClass;
        if (nAttackType == 1 || nAttackType == 8)
            return 0;
        unsigned long long oidRightHand = pClientCreature->GetEquippedItemID(CSWInventory::SLOT_RIGHTWEAPON);
        if (oidRightHand != OBJECT_INVALID) {
            CGameObject* pItem = g_pAppManager->m_pClientExoApp->GetGameObject(oidRightHand);
            if (pItem != 0 && pItem->AsSWCItem() != 0)
                pItem->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        unsigned long long oidLeftHand = pClientCreature->GetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON);
        if (oidLeftHand != OBJECT_INVALID) {
            CGameObject* pItem = g_pAppManager->m_pClientExoApp->GetGameObject(oidLeftHand);
            if (pItem != 0 && pItem->AsSWCItem() != 0)
                pItem->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        pCreature->SetCombatState(1, 2, 0);
    }
    if (g_pAppManager->m_pClientExoApp->GetPlayerMovementInput())
        pClientCreature->GetAnimationBase()->SetOverlayAnimation(
            pClientCreature->GetAnimationBase()->ServerToClientAnimation(SERVER_ANIMATION_WIELD_SWAP), 1);
    else
        pClientCreature->InsertFireForgetAnimation(SERVER_ANIMATION_WIELD_DRAW, 1500, 1.0f, 0, 0);
    pClientCreature->m_nSwitchWeaponsCooldown = 1000;
    return 0;
}
#endif

// Flips an object's "will not render" flag and pushes the change down into the client scene graph so
// it takes effect this frame. Only a real transition does any work, and each early exit returns 0.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandEnableRendering(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bEnable;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bEnable))
        return VM_ERROR_POP_FAILED;
    CGameObject* pGameObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pGameObject == 0)
        return 0;
    CSWSObject* pObject = pGameObject->AsSWSObject();
    if (pObject == 0)
        return 0;
    if (bEnable != 0) {
        if (pObject->m_bWillNotRender != 0) {
            pObject->m_bWillNotRender = 0;
            CSWCObject* pClient = static_cast<CSWCObject*>(pObject->GetClientObject());
            if (pClient == 0)
                return 0;
            if (pClient->GetAnimationBase() == 0)
                return 0;
            Gob* pGob = pClient->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            if (pGob != 0)
                pGob->EnableRendering(true);
            pGob = pClient->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
            if (pGob != 0)
                pGob->EnableRendering(true);
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;
            CSWSItem* pRightHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pRightHand != 0) {
                CSWCItem* pClientItem = pRightHand->GetClientItem();
                if (pClientItem != 0
                    && static_cast<CSWCObject*>(pClientItem)->GetAnimationBase() != 0) {
                    pGob = static_cast<CSWCObject*>(pClientItem)
                            ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
                    if (pGob != 0)
                        pGob->EnableRendering(true);
                }
            }
            CSWSItem* pLeftHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if (pLeftHand != 0) {
                CSWCItem* pClientItem = pLeftHand->GetClientItem();
                if (pClientItem != 0
                    && static_cast<CSWCObject*>(pClientItem)->GetAnimationBase() != 0) {
                    pGob = static_cast<CSWCObject*>(pClientItem)
                            ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
                    if (pGob != 0)
                        pGob->EnableRendering(true);
                }
            }
            CSWSItem* pHeadItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HEAD));
            if (pHeadItem == 0)
                return 0;
            CSWCItem* pClientHeadItem = pHeadItem->GetClientItem();
            if (pClientHeadItem == 0)
                return 0;
            if (static_cast<CSWCObject*>(pClientHeadItem)->GetAnimationBase() == 0)
                return 0;
            pGob = static_cast<CSWCObject*>(pClientHeadItem)
                    ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            if (pGob != 0)
                pGob->EnableRendering(true);
        }
    } else {
        if (pObject->m_bWillNotRender == 0) {
            pObject->m_bWillNotRender = 1;
            CSWCObject* pClient = static_cast<CSWCObject*>(pObject->GetClientObject());
            if (pClient == 0)
                return 0;
            if (pClient->GetAnimationBase() == 0)
                return 0;
            Gob* pGob = pClient->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            if (pGob != 0)
                pGob->EnableRendering(false);
            pGob = pClient->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
            if (pGob != 0)
                pGob->EnableRendering(false);
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature == 0)
                return 0;
            CSWSItem* pRightHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            if (pRightHand != 0) {
                CSWCItem* pClientItem = pRightHand->GetClientItem();
                if (pClientItem != 0
                    && static_cast<CSWCObject*>(pClientItem)->GetAnimationBase() != 0) {
                    pGob = static_cast<CSWCObject*>(pClientItem)
                            ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
                    if (pGob != 0)
                        pGob->EnableRendering(false);
                }
            }
            CSWSItem* pLeftHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if (pLeftHand != 0) {
                CSWCItem* pClientItem = pLeftHand->GetClientItem();
                if (pClientItem != 0
                    && static_cast<CSWCObject*>(pClientItem)->GetAnimationBase() != 0) {
                    pGob = static_cast<CSWCObject*>(pClientItem)
                            ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
                    if (pGob != 0)
                        pGob->EnableRendering(false);
                }
            }
            CSWSItem* pHeadItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HEAD));
            if (pHeadItem == 0)
                return 0;
            CSWCItem* pClientHeadItem = pHeadItem->GetClientItem();
            if (pClientHeadItem == 0)
                return 0;
            if (static_cast<CSWCObject*>(pClientHeadItem)->GetAnimationBase() == 0)
                return 0;
            pGob = static_cast<CSWCObject*>(pClientHeadItem)
                    ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            if (pGob != 0)
                pGob->EnableRendering(false);
        }
    }
    return 0;
}
#endif // !KOTOR1

// CreateItemOnObject: builds a CSWSItem from a template ResRef and hands it to the target's own
// acquisition path -- creature, placeable or store, each with its own AcquireItem arity. Anything
// else is a hard failure: the fresh item is deleted and OBJECT_INVALID is pushed. The stack size is
// clamped to the baseitems.2da row's stacking column, and a clamp to zero aborts the creation. The
// creature arm is the only one with a UI side effect: giving an item to a party member raises the
// "item received" toast unless it was suppressed, and the item's localized name is
// snapshotted before AcquireItem because acquisition can merge and destroy the item.
int CSWVirtualMachineCommands::ExecuteCommandCreateItemOnObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sTemplate;
    unsigned long long oidTarget;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    int nStackSize = 1;
#if KOTOR1  // K1 CreateItemOnObject has no bSuppressMessage argument
    if (nArgCount > 2 && !g_pVirtualMachine->StackPopInteger(&nStackSize))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidItem = CGameObjectArray::INVALIDOBJID;
#else
    unsigned long long oidItem = CGameObjectArray::INVALIDOBJID;
    if (nArgCount > 2 && !g_pVirtualMachine->StackPopInteger(&nStackSize))
        return VM_ERROR_POP_FAILED;
    int bSuppressMessage = 0;
    if (nArgCount >= 4 && !g_pVirtualMachine->StackPopInteger(&bSuppressMessage))
        return VM_ERROR_POP_FAILED;
#endif
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            == CGameObjectArray::SUCCESS) {
        CResRef resRef(static_cast<char*>(sTemplate.CStr()));
        CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
        if (pItem->LoadFromTemplate(resRef) != 0) {
            pItem->m_bits.m_bDropable = 1;
            CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
            if (pBaseItem != 0 && nStackSize > pBaseItem->m_nStacking)
                nStackSize = pBaseItem->m_nStacking;
            if (nStackSize >= 1) {
                pItem->m_stackCount = static_cast<unsigned short>(nStackSize);
                if (pObject->AsSWSCreature() != 0) {
#if !KOTOR1  // K1 has no received-item name list
                    CExoLocString sItemName;
                    if (pItem != 0)
                        sItemName = pItem->m_localizedName;
#endif
                    pObject->AsSWSCreature()->AcquireItem(&pItem, OBJECT_INVALID, OBJECT_INVALID, 1);
#if KOTOR1
                    if (pObject->AsSWSCreature()->m_bInParty != 0)
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_RECEIVED, 0);
#else
                    if (pObject->AsSWSCreature()->m_bInParty != 0 && bSuppressMessage == 0) {
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddReceivedItemToList(sItemName);
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_RECEIVED, 0);
                    }
#endif
                } else if (pObject->AsSWSPlaceable() != 0) {
                    pObject->AsSWSPlaceable()->AcquireItem(&pItem, OBJECT_INVALID, 1);
                } else if (pObject->AsSWSStore() != 0) {
#if KOTOR1  // K1: the one-argument store overload
                    pObject->AsSWSStore()->AcquireItem(pItem);
#else
                    pObject->AsSWSStore()->AcquireItem(pItem, 0);
#endif
                } else {
                    delete pItem;
                    pItem = 0;
                }
                if (pItem != 0)
                    oidItem = pItem->m_objectId;
            } else
                delete pItem;
        } else
            delete pItem;
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandPlayOverlayAnimation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nAnimation;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAnimation))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            CSWCCreature* pClientCreature = pCreature->GetClientCreature();
            if (pClientCreature != 0 && nAnimation == ANIMATION_FIREFORGET_DIVE_ROLL)
                pClientCreature->GetAnimationBase()->SetOverlayAnimation(
                    pClientCreature->GetAnimationBase()->ServerToClientAnimation(SERVER_ANIMATION_DIVE_ROLL), 1);
        }
    }
    return 0;
}

// ActionLockActions, two commands on one handler: one locks the target, the other unlocks it with no
// key object and no run-to-it flag.
int CSWVirtualMachineCommands::ExecuteCommandActionLockActions(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        if (nCommand == CMD_ActionUnlockObject)
            pObject->AsSWSObject()->AddUnlockObjectAction(oidTarget, OBJECT_INVALID, 0);
        else if (nCommand == CMD_ActionLockObject)
            pObject->AsSWSObject()->AddLockObjectAction(oidTarget);
    }
    return 0;
}

// The CExoString case converters. The result string is constructed before the pop and assigned from
// the converter's by-value temporary.
int CSWVirtualMachineCommands::ExecuteCommandGetStringLowerCase(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sInput;
    CExoString sResult;
    if (!g_pVirtualMachine->StackPopString(&sInput))
        return VM_ERROR_POP_FAILED;
    sResult = sInput.LowerCase();
    if (!g_pVirtualMachine->StackPushString(sResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetStringUpperCase(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sInput;
    CExoString sResult;
    if (!g_pVirtualMachine->StackPopString(&sInput))
        return VM_ERROR_POP_FAILED;
    sResult = sInput.UpperCase();
    if (!g_pVirtualMachine->StackPushString(sResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// DetonateMine: fades the mine trigger out over one tick and queues its destruction through the AI
// master, crediting the running script's own object as the event source. The lookup result is
// dereferenced with no null check, and the cast is re-run for the fade receiver.
int CSWVirtualMachineCommands::ExecuteCommandDetonateMine(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidMine = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidMine))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidMine);
    if (pObject->AsSWSTrigger() != 0) {
        pObject->AsSWSTrigger()->SetFadeOutTime(1);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_oidSelf, oidMine, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    }
    return 0;
}

// ForceHeartbeat: runs the creature's heartbeat script immediately and restarts its timer. The
// object id handed to RunScript comes off the CGameObject, not the creature view.
int CSWVirtualMachineCommands::ExecuteCommandForceHeartbeat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        g_pVirtualMachine->RunScript(
                &pObject->AsSWSCreature()->m_scripts[CSWSCreature::CREATURE_SCRIPT_HEARTBEAT],
                pObject->m_objectId, 1);
        pObject->AsSWSCreature()->ResetHeartbeatTimer();
    }
    return 0;
}

// The pop-failure code here is -2000, not the file's usual -2001.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetBonusForcePoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_PUSH_FAILED;
    int nBonus;
#ifdef K2_FIX_ENGINE_BUGS
    nBonus = 0;
#endif
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
            nBonus = pCreature->m_nBonusForcePoints;
    }
    if (!g_pVirtualMachine->StackPushInteger(nBonus))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// A missed lookup returns 0 (success). The null guard must stay around the store, and the int has
// to be declared before the object id or the two stack slots swap.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandForceAlwaysUpdate(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bForceAlwaysUpdate = 0;
    unsigned long long oidObject = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bForceAlwaysUpdate))
        return VM_ERROR_POP_FAILED;
    int nResult = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        nResult = 0;
        static_cast<CSWSObject*>(pObject)->m_bForceAlwaysUpdate = bForceAlwaysUpdate;
    }
    return nResult;
}
#endif // !KOTOR1

// The zero vector normalizes to itself rather than to a NaN, because a default-constructed (zeroed)
// vector is compared first and only a non-zero one reaches normalize().
int CSWVirtualMachineCommands::ExecuteCommandVectorNormalize(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vVector;
    if (!g_pVirtualMachine->StackPopVector(&vVector))
        return VM_ERROR_POP_FAILED;
    Vector vZero;
    Vector vResult;
    if (vVector != vZero)
        vResult = normalize(vVector);
    if (!g_pVirtualMachine->StackPushVector(vResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Three floats popped straight into the components of one Vector.
int CSWVirtualMachineCommands::ExecuteCommandVector(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vVector;
    if (!g_pVirtualMachine->StackPopFloat(&vVector.x))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&vVector.y))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&vVector.z))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPushVector(vVector))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// CExoString::GetLength written out by hand: a bare strlen, narrowed to int.
int CSWVirtualMachineCommands::ExecuteCommandGetStringLength(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sString;
    int nLength;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    nLength = sString.GetLength();
    if (!g_pVirtualMachine->StackPushInteger(nLength))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// A missed lookup returns 0, and both pop failures are -2000 rather than the usual -2001. The max
// is stored directly while the current value goes through the clamping setter, so this both raises
// the ceiling and heals to full.
int CSWVirtualMachineCommands::ExecuteCommandSetMaxHitPoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nMaxHitPoints;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nMaxHitPoints))
        return VM_ERROR_PUSH_FAILED;
    int nResult = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject != 0) {
            pSWSObject->m_hitPoints = nMaxHitPoints;
            nResult = 0;
            pSWSObject->SetCurrentHitPoints(nMaxHitPoints);
        }
    }
    return nResult;
}

// The creature probe is a type check only, and the flag is stored back through the CGameObject, not
// through the creature view. A missed lookup returns 0.
int CSWVirtualMachineCommands::ExecuteCommandSetOrientOnClick(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bOrientOnClick;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bOrientOnClick))
        return VM_ERROR_POP_FAILED;
    int nResult = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        nResult = 0;
        static_cast<CSWSObject*>(pObject)->m_bOrientOnClick = bOrientOnClick;
    }
    return nResult;
}

// The flag argument is popped as an object and then truncated into an int field; a missed lookup
// returns 0.
int CSWVirtualMachineCommands::ExecuteCommandSetForcePowerUnsuccessful(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long nUnsuccessful = 0;
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&nUnsuccessful))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nResult = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0) {
        nResult = 0;
        pCreature->m_bWasForcePowerSuccessful = static_cast<int>(nUnsuccessful);
    }
    return nResult;
}

int CSWVirtualMachineCommands::ExecuteCommandSetItemNonEquippable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    int bNonEquippable;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bNonEquippable))
        return VM_ERROR_POP_FAILED;
    int nResult;
#ifdef K2_FIX_ENGINE_BUGS
    nResult = 0;
#endif
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0) {
        nResult = 0;
        pItem->m_bits.m_bNonEquippable = static_cast<unsigned short>(bNonEquippable);
    }
    return nResult;
}

// The ratio is an optional second argument, defaulted to 0.75. The default is dead, because a
// failed pop returns rather than falling through to the setter.
int CSWVirtualMachineCommands::ExecuteCommandSetSpeedBlurEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int bEnable;
    if (!g_pVirtualMachine->StackPopInteger(&bEnable))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        float fRatio = 0.75f;
        if (!g_pVirtualMachine->StackPopFloat(&fRatio))
            return VM_ERROR_POP_FAILED;
        AurSetSpeedBlurRatio(fRatio);
    }
    if (bEnable != 0)
        AurEnableSpeedBlur();
    else
        AurDisableSpeedBlur();
    return 0;
}

// SetEffectIcon: wraps the popped effect in a two-node link -- a fresh EFFECT_EFFECT_ICON clone
// carrying the icon index in integer slot 0, linked against the popped effect under an EFFECT_LINK
// clone -- and pushes the link. Both clones are marked exposed. A failed icon pop frees the popped
// effect.
int CSWVirtualMachineCommands::ExecuteCommandSetEffectIcon(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameEffect* pPopped;
    int nIcon;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pPopped)))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = pPopped;
    if (!g_pVirtualMachine->StackPopInteger(&nIcon)) {
        if (pEffect != 0)
            delete pEffect;
        return VM_ERROR_POP_FAILED;
    }
    CGameEffect* pIcon = new CGameEffect(pEffect);
    pIcon->m_bIsExposed = 1;
    pIcon->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_EFFECT_ICON);
    pIcon->SetCreator(m_oidSelf);
    pIcon->m_bIsExposed = 1;
    pIcon->SetInteger(0, nIcon);
    CGameEffect* pLink = new CGameEffect(pEffect);
    pLink->m_bIsExposed = 1;
    pLink->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LINK);
    pLink->SetCreator(m_oidSelf);
    pLink->SetLinked(pIcon, pEffect);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pLink) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pLink;
    return 0;
}

// The optional second argument is popped and then thrown away, since the same local is overwritten
// with -1 before the call on both paths.
int CSWVirtualMachineCommands::ExecuteCommandSetDialogPlaceableCamera(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nCameraId;
    int nEffectFlag;
    if (!g_pVirtualMachine->StackPopInteger(&nCameraId))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&nEffectFlag))
        return VM_ERROR_POP_FAILED;
    nEffectFlag = -1;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetShotFromScript(nCameraId, nEffectFlag);
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandSoundObjectSetPosition(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    Vector vPosition;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopVector(&vPosition))
        return VM_ERROR_POP_FAILED;
    int nResult;
#ifdef K2_FIX_ENGINE_BUGS
    nResult = 0;
#endif
    CSWSSoundObject* pSound =
        g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound != 0) {
        nResult = 0;
        pSound->ChangePosition(vPosition);
    }
    return nResult;
}

// The script argument is a fade time in seconds, converted to the sound object's milliseconds. The
// popped float is copied into a second local before the lookup.
int CSWVirtualMachineCommands::ExecuteCommandSoundObjectFadeAndStop(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidSound;
    float fFadeSeconds;
    if (!g_pVirtualMachine->StackPopObject(&oidSound))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fFadeSeconds))
        return VM_ERROR_POP_FAILED;
    int nResult;
#ifdef K2_FIX_ENGINE_BUGS
    nResult = 0;
#endif
    float fFade = fFadeSeconds;
    CSWSSoundObject* pSound =
        g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(oidSound);
    if (pSound != 0) {
        nResult = 0;
        pSound->FadeAndStop(static_cast<unsigned int>(fFade * 1000.0f));
    }
    return nResult;
}

// The lookup result is not null-checked before the creature probe.
int CSWVirtualMachineCommands::ExecuteCommandChangeObjectAppearance(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nAppearance;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAppearance))
        return VM_ERROR_POP_FAILED;
    int nResult;
#ifdef K2_FIX_ENGINE_BUGS
    nResult = 0;
#endif
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetGameObject(oidObject)->AsSWSCreature();
    if (pCreature != 0) {
        nResult = 0;
        pCreature->Disguise(nAppearance, 0, 0, 1);
    }
    return nResult;
}

// AssignPUP (K2-only): two integers popped -- the puppet row and the NPC party slot it is bound to
// -- handed straight to the party table; the result is pushed.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandAssignPUP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPUPIndex;
    int nNPCIndex;
    if (!g_pVirtualMachine->StackPopInteger(&nPUPIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nNPCIndex))
        return VM_ERROR_POP_FAILED;
    int nResult = g_pAppManager->m_pServerExoApp->GetPartyTable()->AssignPUP(nPUPIndex, nNPCIndex);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

// The creature's live Force pool is the sum of the stats' two short slots (the base pool and the
// temporary bonus pool). A null creature pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandGetCurrentForcePoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nForcePoints = 0;
    if (pCreature != 0)
        nForcePoints = static_cast<short>(pCreature->m_pStats->m_nCurrentForcePoints + pCreature->m_pStats->m_nTemporaryForcePoints);
    if (!g_pVirtualMachine->StackPushInteger(nForcePoints))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Pops the talent engine structure (type 3), reports whether its type is anything other than
// TALENT_TYPE_INVALID, then destroys it. The type read is unguarded.
int CSWVirtualMachineCommands::ExecuteCommandGetIsTalentValid(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptTalent* pTalent;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_TALENT, reinterpret_cast<void**>(&pTalent)))
        return VM_ERROR_POP_FAILED;
    int bValid = (pTalent->m_nType != -1);
    delete pTalent;
    pTalent = 0;
    if (!g_pVirtualMachine->StackPushInteger(bValid))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The shared handler for the stat getters; only command 358 (GetGender) uses it, and a null creature
// pushes -1.
int CSWVirtualMachineCommands::ExecuteCommandGetStat(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nStat = -1;
    if (pCreature != 0 && nCommand == CMD_GetGender)
        nStat = pCreature->m_pStats->m_nGender;
    if (!g_pVirtualMachine->StackPushInteger(nStat))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// EndGame: the optional integer argument selects the delay -- 60 seconds when set, immediate when
// clear. With no argument at all the flag defaults to 1 and the pop is skipped.
int CSWVirtualMachineCommands::ExecuteCommandEndGame(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nShowEndGameGui = 1;
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopInteger(&nShowEndGameGui))
            return VM_ERROR_POP_FAILED;
    }
#if KOTOR1  // K1 uses a 5-second delay and sets the game-over flag
    g_pAppManager->m_pClientExoApp->SetEndGamePending(1, nShowEndGameGui != 0 ? 5.0f : 0.0f,
                                                      nShowEndGameGui);
    if (nShowEndGameGui != 0)
        g_pAppManager->m_pClientExoApp->SetGameOver(1);
#else
    g_pAppManager->m_pClientExoApp->SetEndGamePending(1, nShowEndGameGui != 0 ? 60.0f : 0.0f,
                                                      nShowEndGameGui);
#endif
    return 0;
}

// YardsToMeters (219) / FeetToMeters (218) share one handler: pop a float, scale it by the
// conversion factor, push it back.
int CSWVirtualMachineCommands::ExecuteCommandDistanceConversions(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    float fValue;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return VM_ERROR_POP_FAILED;
    float fResult K2_UNSET;
    if (nCommand == CMD_FeetToMeters)
        fResult = fValue * 0.3048f;
    else if (nCommand == CMD_YardsToMeters)
        fResult = fValue * 0.9144f;
    if (!g_pVirtualMachine->StackPushFloat(fResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// CancelCombat: the creature id is mandatory, the clear-the-queue flag optional. Both locals carry
// defaults the pop-failure path never reaches.
int CSWVirtualMachineCommands::ExecuteCommandCancelCombat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    int nClearActions = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nClearActions))
            return VM_ERROR_POP_FAILED;
    }
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0)
        pCreature->CancelCombat(nClearActions);
    return 0;
}

// QueueMovie: the movie resref then its skippable flags word, appended to the client's movie queue.
// No push; the string is destroyed on every exit.
int CSWVirtualMachineCommands::ExecuteCommandQueueMovie(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nFlags;
    CExoString sMovie;
    if (!g_pVirtualMachine->StackPopString(&sMovie))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nFlags))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pClientExoApp->AddMovieToMovieQueue(sMovie, nFlags);
    return 0;
}

// SetCameraMode: the player object and the camera mode, forwarded as a client message. Both the
// player lookup and the message channel are optional; either being null is a silent 0.
int CSWVirtualMachineCommands::ExecuteCommandSetCameraMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidPlayer;
    int nMode;
    if (!g_pVirtualMachine->StackPopObject(&oidPlayer))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nMode))
        return VM_ERROR_POP_FAILED;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidPlayer);
    if (pPlayer != 0) {
        CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        if (pMessage != 0)
            pMessage->SendServerToPlayerCamera_SetMode(pPlayer, static_cast<unsigned char>(nMode));
    }
    return 0;
}

// SetGoodEvilValue: the new light/dark value is truncated to a short before the 0..100 clamp, so
// 65536 reads as 0.
int CSWVirtualMachineCommands::ExecuteCommandSetGoodEvilValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nValue;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0) {
#if KOTOR1  // K1 goes through ModifyAlignment by the delta
        pCreature->m_pStats->ModifyAlignment(static_cast<short>(nValue) - pCreature->m_pStats->m_nAlignment);
#else
        int nAlignment = static_cast<short>(nValue);
        if (nAlignment < 0)
            nAlignment = 0;
        if (nAlignment > 100)
            nAlignment = 100;
        pCreature->m_pStats->m_nAlignment = static_cast<short>(nAlignment);
#endif
    }
    return 0;
}

// SetLockOrientationInDialog: the object id is translated to its client id and handed to the
// in-game GUI's conversation camera. OBJECT_INVALID is a no-op rather than an error.
int CSWVirtualMachineCommands::ExecuteCommandSetLockOrientationInDialog(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bLock;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bLock))
        return VM_ERROR_POP_FAILED;
    if (oidObject != OBJECT_INVALID) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidObject);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetLockObjectOrientationInDialog(oidClient,
                                                                                        bLock);
    }
    return 0;
}

// The SetLockOrientationInDialog twin, aimed at the head-tracking lock instead of the body
// orientation.
int CSWVirtualMachineCommands::ExecuteCommandSetLockHeadFollowInDialog(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bLock;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bLock))
        return VM_ERROR_POP_FAILED;
    if (oidObject != OBJECT_INVALID) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidObject);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetLockHeadFollowInDialog(oidClient, bLock);
    }
    return 0;
}

// SpawnAvailableNPC: materialises available-NPC slot nNPC at the script's location, facing the
// location's orientation, and pushes the new object's id. The `1`s are the use-position and
// notify-clients flags. A failed push returns -2000 and leaks the location; the success path frees
// it.
int CSWVirtualMachineCommands::ExecuteCommandSpawnAvailableNPC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    CScriptLocation* pLocation;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidSpawned = g_pAppManager->m_pServerExoApp->GetPartyTable()->SpawnNPC(
        nNPC, 1, pLocation->m_vPosition, pLocation->m_vOrientation, 1);
    if (!g_pVirtualMachine->StackPushObject(oidSpawned))
        return VM_ERROR_PUSH_FAILED;
    if (pLocation != 0)
        delete pLocation;
    pLocation = 0;  // the null case reaches this store too, so it stays outside the guard  // idiom-exempt: the orig's `str xzr,[sp,#8]` is on the MERGED path -- the
    return 0;
}

// SpawnAvailablePUP: materialises available-PUP slot nPUP at the script's location, facing the
// location's orientation, and pushes the new object's id. The `1`s are the use-position and
// notify-clients flags. A failed push returns -2000 and leaks the location; the success path frees
// it.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandSpawnAvailablePUP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPUP;
    CScriptLocation* pLocation;
    if (!g_pVirtualMachine->StackPopInteger(&nPUP))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidSpawned = g_pAppManager->m_pServerExoApp->GetPartyTable()->SpawnPUP(
        nPUP, 1, pLocation->m_vPosition, pLocation->m_vOrientation, 1);
    if (!g_pVirtualMachine->StackPushObject(oidSpawned))
        return VM_ERROR_PUSH_FAILED;
    if (pLocation != 0)
        delete pLocation;
    pLocation = 0;  // the null case reaches this store too, so it stays outside the guard  // idiom-exempt: the orig's `str xzr,[sp,#8]` is on the MERGED path -- the
    return 0;
}
#endif

// GetHasFeatEffect / GetHasSpellEffect: one skeleton, two effect-list probes. The operands come off
// the stack in the reverse of the script's argument order, and a missing or foreign object answers 0
// rather than failing.
int CSWVirtualMachineCommands::ExecuteCommandGetHasFeatEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nFeat;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nFeat))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int bHas = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSObject() != 0)
        bHas = static_cast<CSWSObject*>(pObject)->GetHasFeatEffectApplied(static_cast<unsigned short>(nFeat));
    if (!g_pVirtualMachine->StackPushInteger(bHas))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetHasSpellEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nSpellId;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nSpellId))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int bHas = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSObject() != 0)
        bHas = static_cast<CSWSObject*>(pObject)->HasSpellEffectApplied(nSpellId);
    if (!g_pVirtualMachine->StackPushInteger(bHas))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetStrRefSoundDuration: the recorded length of a talk-table entry's VO line. The record is a stack
// temporary; a negative strref or a failed fetch zeroes its length field rather than skipping the
// push, so the pushed value is always read back out of the record.
int CSWVirtualMachineCommands::ExecuteCommandGetStrRefSoundDuration(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStrRef;
    STR_RES strRes;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    if (nStrRef < 0 || g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE) == 0)
        strRes.fSoundLength = 0.0f;
    if (!g_pVirtualMachine->StackPushFloat(strRes.fSoundLength))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// StringToInt / StringToFloat share one handler: pop the text, run the CRT converter
// over it and push the typed result.

int CSWVirtualMachineCommands::ExecuteCommandStringConversions(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sValue;
    if (!g_pVirtualMachine->StackPopString(&sValue))
        return VM_ERROR_POP_FAILED;
    if (nCommand == CMD_StringToInt) {
        if (!g_pVirtualMachine->StackPushInteger(atoi(sValue.CStr())))
            return VM_ERROR_PUSH_FAILED;
    } else {
        if (!g_pVirtualMachine->StackPushFloat(static_cast<float>(atof(sValue.CStr()))))
            return VM_ERROR_PUSH_FAILED;
    }
    return 0;
}

// SetTime: advances the world clock to the requested wall-clock time, then re-reads the resulting
// day and time-of-day back out and republishes them through the module.
int CSWVirtualMachineCommands::ExecuteCommandSetTime(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nHour;
    int nMinute;
    int nSecond;
    int nMillisecond;
    unsigned int nCalendarDay;
    unsigned int nTimeOfDay;
    if (!g_pVirtualMachine->StackPopInteger(&nHour))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nMinute))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nSecond))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nMillisecond))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->AdvanceToTime(nHour, nMinute, nSecond,
                                                                  nMillisecond);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nCalendarDay, &nTimeOfDay);
    g_pAppManager->m_pServerExoApp->GetModule()->UpdateTime(nCalendarDay, nTimeOfDay, 0);
    return 0;
}

// GetGlobalLocation: the location-typed sibling of GetGlobalString. The location is a stack
// temporary the push copies out of, so there is nothing to free on the failure arm. The pop failure
// returns -2000 rather than -2001, as GetGlobalString does.
int CSWVirtualMachineCommands::ExecuteCommandGetGlobalLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    CScriptLocation sLocation;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueLocation(sName, sLocation);
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, &sLocation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// EventUserDefined: wraps the script's event number in a type-11 event and pushes it as an engine
// structure. The event is deleted on every exit, including the pop failure.
int CSWVirtualMachineCommands::ExecuteCommandEventUserDefined(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptEvent* pEvent = new CScriptEvent;
    int nUserDefinedEventNumber;
    if (!g_pVirtualMachine->StackPopInteger(&nUserDefinedEventNumber)) {
#if !KOTOR1  // K1 frees the event only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
    pEvent->SetInteger(0, nUserDefinedEventNumber);
    int bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EVENT, pEvent);
#if KOTOR1
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    delete pEvent;
#else
    delete pEvent;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
#endif
    return 0;
}

// AddBonusForcePoints: tops up a creature's flat Force-point bonus and, for a creature that came
// out of character creation, tells the in-game GUI to flash the gain (11) or the loss (12). The
// panel value is the raw delta either way. Both pop failures return -2000 rather than -2001.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandAddBonusForcePoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nBonus;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nBonus))
        return VM_ERROR_PUSH_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    int nCurrent = pCreature->m_nBonusForcePoints;
    int nDelta = nBonus;
    pCreature->m_nBonusForcePoints = nDelta + nCurrent;
    if (pCreature->m_bPlayerMadeCharacter != 0) {
        CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        if (nDelta < 0)
            pGui->UpdateStatus(CGuiInGame::GUI_STATUS_MAX_FP_LOST, nBonus);
        else
            pGui->UpdateStatus(CGuiInGame::GUI_STATUS_MAX_FP_GAINED, nBonus);
    }
    return 0;
}
#endif // !KOTOR1

// RollDice: the whole d2/d3/d4/d6/d8/d10/d12/d20/d100 family shares one handler, with the die's
// side count coming from the command id. d2 is the default arm. The dice-count operand is optional
// and clamps up to 1.
int CSWVirtualMachineCommands::ExecuteCommandRollDice(int nCommand, int nArgCount)
{
    int nDice = 1;
    int nSides;
    switch (nCommand) {
    case CMD_d3:  nSides = 3;   break;
    case CMD_d4:  nSides = 4;   break;
    case CMD_d6:  nSides = 6;   break;
    case CMD_d8:  nSides = 8;   break;
    case CMD_d10: nSides = 10;  break;
    case CMD_d12: nSides = 12;  break;
    case CMD_d20: nSides = 20;  break;
    case CMD_d100: nSides = 100; break;
    default:  nSides = 2;   break;
    }
    if (nArgCount == 1) {
        if (!g_pVirtualMachine->StackPopInteger(&nDice))
            return VM_ERROR_POP_FAILED;
    }
    nDice = (nDice > 1) ? nDice : 1;
    int nRoll = g_pRules->RollDice(static_cast<unsigned char>(nDice), static_cast<unsigned char>(nSides));
    if (!g_pVirtualMachine->StackPushInteger(nRoll))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// DeleteJournalWorldAllEntries: clears the module's world journal and tells every connected
// player's client to do the same. The message object is fetched before the module, and its null
// check sits inside the per-player loop rather than guarding the whole walk.
int CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldAllEntries(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0)
        pModule->DeleteWorldJournalAllEntries();
    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 0;
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
            pMessage->SendServerToPlayerJournalDeleteWorldAll(pPlayer);
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return 0;
}

// GetSpellTargetLocation: where the spell or force power currently being run was aimed. The answer
// is the caster's pending-cast block, so outside a spell script the zeroed location built up front is
// pushed unchanged. Placeables can cast too, which is why the creature test falls through to a
// placeable one.
int CSWVirtualMachineCommands::ExecuteCommandGetSpellTargetLoc(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    CScriptLocation location;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (m_nScriptRunArg == 1
        && pArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS) {
        CSWSObject* pCaster = pObject->AsSWSObject();
        if (pCaster != 0
            && (pCaster->AsSWSCreature() != 0 || pCaster->AsSWSPlaceable() != 0)) {
            location.m_vPosition = pCaster->m_castPosition;
            location.m_vOrientation = pCaster->m_orientation;
        }
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, &location))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetEnteringObject: who triggered the OnEnter/OnClientEnter event currently running. The id is
// recorded on OBJECT_SELF at event-dispatch time, so this is a per-type field read dispatched by
// object kind; eight kinds answer, everything else (and every script that is not an action script)
// gets OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetEnteringObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidEntering = CGameObjectArray::INVALIDOBJID;
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
        if (pObject != 0) {
            switch (pObject->m_objectType) {
            case GAME_OBJECT_TYPE_MODULE: {
                CSWSModule* pModule = pObject->AsSWSModule();
                if (pModule != 0)
                    oidEntering = pModule->m_oidEnteringClient;
                break;
            }
            case GAME_OBJECT_TYPE_AREA: {
                CSWSArea* pArea = pObject->AsSWSArea();
                if (pArea != 0)
                    oidEntering = pArea->m_oidEnteringObject;
                break;
            }
            case GAME_OBJECT_TYPE_TRIGGER: {
                CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
                if (pTrigger != 0)
                    oidEntering = pTrigger->m_oidEnteringObject;
                break;
            }
            case GAME_OBJECT_TYPE_PLACEABLE: {
                CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
                if (pPlaceable != 0)
                    oidEntering = pPlaceable->m_oidEnteringObject;
                break;
            }
            case GAME_OBJECT_TYPE_DOOR: {
                CSWSDoor* pDoor = pObject->AsSWSDoor();
                if (pDoor != 0)
                    oidEntering = pDoor->m_oidEnteringObject;
                break;
            }
            case GAME_OBJECT_TYPE_AREA_OF_EFFECT: {
                CSWSAreaOfEffectObject* pAoE = pObject->AsSWSAreaOfEffectObject();
                if (pAoE != 0)
                    oidEntering = pAoE->m_enteredObjectId;
                break;
            }
            case GAME_OBJECT_TYPE_ENCOUNTER: {
                CSWSEncounter* pEncounter = pObject->AsSWSEncounter();
                if (pEncounter != 0)
                    oidEntering = pEncounter->m_enteredObjectId;
                break;
            }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidEntering))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetJournalEntry: the state of a quest in the first player's journal; the walk stops at the first
// player whose game object is a creature. The no-player-list path answers 1 and a quest that is
// absent answers 0.
int CSWVirtualMachineCommands::ExecuteCommandGetJournalEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    CSWSCreature* pCreature;
    int nState;
    CExoString sPlotId;
    if (!g_pVirtualMachine->StackPopString(&sPlotId))
        return VM_ERROR_POP_FAILED;
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 1;
    nState = 0;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
            pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0) {
                nState = pCreature->GetJournal()->GetState(sPlotId);
                break;
            }
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    if (!g_pVirtualMachine->StackPushInteger(nState))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// RemoveJournalQuestEntry: drops the named quest from every player's journal. The no-player-list
// path answers 1 rather than 0, the only success code in the family that is not 0.
int CSWVirtualMachineCommands::ExecuteCommandRemoveJournalQuestEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    CSWSCreature* pCreature;
    CExoString sPlotId;
    if (!g_pVirtualMachine->StackPopString(&sPlotId))
        return VM_ERROR_POP_FAILED;
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 1;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
            pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0)
                pCreature->GetJournal()->Destroy(sPlotId);
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return 0;
}

// CutsceneAttack: queues a scripted, pre-decided attack on OBJECT_SELF -- the animation, the to-hit
// result and the damage all come from the script rather than the combat engine. All four operand
// slots are pre-seeded, so a partially-failed pop still leaves defined values behind.
int CSWVirtualMachineCommands::ExecuteCommandCutsceneAttack(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget = OBJECT_INVALID;
    int nAnimation = 0;
    int nAttackResult = 0;
    int nDamage = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAnimation))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAttackResult))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nDamage))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pSelf = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pSelf != 0)
        pSelf->AddCutsceneAttackActions(oidTarget, nAnimation, nAttackResult, nDamage);
    return 0;
}

// SetXP: overwrites a creature's earned experience. A negative amount is rejected up front and the
// whole lookup is skipped; the second SetExperience argument (1) is the award-level-ups flag.
int CSWVirtualMachineCommands::ExecuteCommandSetXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidCreature;
    int nXP;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nXP))
        return VM_ERROR_POP_FAILED;
    int nExperience = nXP;
    if (nExperience >= 0
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
            pCreature->m_pStats->SetExperience(nExperience, 1);
    }
    return 0;
}

// PlayVisualAreaEffect: fires a one-shot area visual at the script's location. The player
// creature's area is the receiver, so the effect is invisible to anyone elsewhere. A missing
// creature returns 0 without freeing the popped location.
int CSWVirtualMachineCommands::ExecuteCommandPlayVisualAreaEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nVisualEffectId;
    CScriptLocation* pLocation;
    if (!g_pVirtualMachine->StackPopInteger(&nVisualEffectId))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pPlayer == 0)
        return 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(pPlayer->GetArea());
    if (pArea != 0)
        pArea->PlayVisualEffect(static_cast<unsigned short>(nVisualEffectId), pLocation->m_vPosition);
    if (pLocation != 0) {
        delete pLocation;
        pLocation = 0;
    }
    return 0;
}

// The faction-aggregate queries share one skeleton across five bodies. The creature id slot is
// pre-seeded with OBJECT_INVALID; the creature is resolved and the aggregate runs over its faction.
// The faction manager is reached through the server pimpl with both links null-checked, and the
// returned faction record is not checked. Any broken link short-circuits to the -1 answer, which is
// still pushed.

int CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageGoodEvilAlignment(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nAlignment = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nAlignment = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                 ->GetFaction(pCreature->m_pStats->m_factionId)->GetAverageGoodEvilAlignment();
    if (!g_pVirtualMachine->StackPushInteger(nAlignment))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageLevel(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nLevel = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nLevel = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                 ->GetFaction(pCreature->m_pStats->m_factionId)->GetAverageLevel();
    if (!g_pVirtualMachine->StackPushInteger(nLevel))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nXP = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nXP = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                 ->GetFaction(pCreature->m_pStats->m_factionId)->GetAverageXP();
    if (!g_pVirtualMachine->StackPushInteger(nXP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionGold(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nGold = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nGold = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                 ->GetFaction(pCreature->m_pStats->m_factionId)->GetGold();
    if (!g_pVirtualMachine->StackPushInteger(nGold))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionMostFrequentClass(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nClass = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nClass = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                 ->GetFaction(pCreature->m_pStats->m_factionId)->GetMostFrequentClass();
    if (!g_pVirtualMachine->StackPushInteger(nClass))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The six member-picking faction queries share the aggregates' skeleton but answer an object rather
// than an int: the popped creature names the faction, an optional second operand (defaulted on)
// filters the scan, and any broken link short-circuits to OBJECT_INVALID, which is still pushed.

int CSWVirtualMachineCommands::ExecuteCommandGetFactionBestAC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetBestAC(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionLeastDamagedMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetLeastDamagedMember(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionMostDamagedMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetMostDamagedMember(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionStrongestMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetStrongestMember(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionWeakestMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetWeakestMember(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandGetFactionWorstAC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bMustBeVisible = 1;
    if (nArgCount == 2 && !g_pVirtualMachine->StackPopInteger(&bMustBeVisible))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidMember = OBJECT_INVALID;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        oidMember = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                        ->GetFaction(pCreature->m_pStats->m_factionId)->GetWorstAC(oidCreature, bMustBeVisible);
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetFactionAverageReputation: the aggregates' skeleton, but with a second popped operand -- the
// object the reputation is measured towards -- and no pre-seeded OBJECT_INVALID on either slot.
int CSWVirtualMachineCommands::ExecuteCommandGetFactionAverageReputation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int nReputation = -1;
    if (pCreature != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal != 0
        && g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager != 0)
        nReputation = g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager
                          ->GetFaction(pCreature->m_pStats->m_factionId)->GetAverageReputation(oidTarget);
    if (!g_pVirtualMachine->StackPushInteger(nReputation))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetFactionEqual: 1 when both objects resolve to the same faction id. This one goes through
// CServerExoApp::GetFactionOfObject (a status-returning out-param call), and a failed lookup on
// either object answers 0 rather than -1.
int CSWVirtualMachineCommands::ExecuteCommandGetFactionEqual(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidFirst;
    unsigned long long oidSecond;
    int nFirstFaction;
    int nSecondFaction;
    if (!g_pVirtualMachine->StackPopObject(&oidFirst))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidSecond))
        return VM_ERROR_POP_FAILED;
    int bEqual = 0;
    if (g_pAppManager->m_pServerExoApp->GetFactionOfObject(oidFirst, &nFirstFaction) == 1
        && g_pAppManager->m_pServerExoApp->GetFactionOfObject(oidSecond, &nSecondFaction) == 1)
        bEqual = (nFirstFaction == nSecondFaction);
    if (!g_pVirtualMachine->StackPushInteger(bEqual))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetFactionLeader: the creature's faction leader id, or OBJECT_INVALID when either the creature or
// its faction is missing.
int CSWVirtualMachineCommands::ExecuteCommandGetFactionLeader(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    unsigned long long oidLeader = OBJECT_INVALID;
    if (pCreature != 0 && pCreature->GetFaction() != 0)
        oidLeader = pCreature->GetFaction()->GetLeader();
    if (!g_pVirtualMachine->StackPushObject(oidLeader))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetListening: the IsListening writer -- the same guards and the same raw CSWSObject cast onto
// m_bListening.
int CSWVirtualMachineCommands::ExecuteCommandSetListening(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bListening;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bListening))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        static_cast<CSWSObject*>(pObject)->m_bListening = bListening;
    return 0;
}

// SetPlotFlag: sets m_bPlot through the object probe, called twice -- once as the guard, once for
// the store.
int CSWVirtualMachineCommands::ExecuteCommandSetPlotFlag(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bPlot;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bPlot))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        pSWSObject->m_bPlot = bPlot;
    }
    return 0;
}

// GetWeaponRanged: the item's baseitems.2da row's RangedWeapon flag, normalised to 0/1. A missing
// item pushes 0, and the base-item lookup itself is not null-checked.
int CSWVirtualMachineCommands::ExecuteCommandGetWeaponRanged(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    int bRanged = 0;
    if (pItem != 0) {
        int nBaseItem = pItem->m_baseItem;
        bRanged = (g_pRules->GetBaseItem(nBaseItem)->m_bRangedWeapon != 0);
    }
    if (!g_pVirtualMachine->StackPushInteger(bRanged))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// IsNPCPartyMember: the party table's own membership test, widened to also accept the
// currently-selected NPC slot.
int CSWVirtualMachineCommands::ExecuteCommandIsNPCPartyMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    int bMember = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(nNPC);
#if !KOTOR1  // K1 tests party membership only
    if (bMember == 0)
        bMember = (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC == nNPC);
#endif
    if (!g_pVirtualMachine->StackPushInteger(bMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetObjectPersonalSpace: the creature's pathfinding personal-space radius. Anything that is not a
// creature pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandGetObjectPersonalSpace(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    float fPersonalSpace = 0.0f;
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        fPersonalSpace = pObject->AsSWSCreature()->m_pPathfindInfo->m_fCrePersonalSpace;
    if (!g_pVirtualMachine->StackPushFloat(fPersonalSpace))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetStandardFaction: the object's faction id through the faction manager. Both the server pimpl and
// its faction manager are re-tested, and either being null pushes the -1 sentinel the local was
// preset to.
int CSWVirtualMachineCommands::ExecuteCommandGetStandardFaction(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nFaction = -1;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer->m_pInternal != 0 && pServer->m_pInternal->m_pFactionManager != 0)
        pServer->GetFactionOfObject(oidObject, &nFaction);
    if (!g_pVirtualMachine->StackPushInteger(nFaction))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetReturnStrref: the return-to-base / end-of-mission prompt strings kept on the party table. The
// third (query) strref is optional and defaults to 0.
int CSWVirtualMachineCommands::ExecuteCommandSetReturnStrref(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int bShow;
    int nStrRef;
    int nQueryStrRef = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bShow))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nQueryStrRef))
            return VM_ERROR_POP_FAILED;
    }
    g_pAppManager->m_pServerExoApp->GetPartyTable()->SetReturnStrref(bShow, nStrRef, nQueryStrRef);
    return 0;
}

// GetIsPartyLeader: compares the popped id against the client party's slot-0 character, translated
// back into a server object id. No party character at all pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandGetIsPartyLeader(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    int bIsLeader = 0;
    if (pLeader != 0) {
        bIsLeader = (oidCreature == g_pAppManager->m_pServerExoApp
                                        ->ClientToServerObjectId(pLeader->m_objectId));
    }
    if (!g_pVirtualMachine->StackPushInteger(bIsLeader))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetPUPOwner: the party member a puppet is bound to -- the puppet's own id is mapped to a puppet
// row first, then the row to its owning creature. No owner pushes OBJECT_INVALID.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandGetPUPOwner(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidPUP;
    if (!g_pVirtualMachine->StackPopObject(&oidPUP))
        return VM_ERROR_POP_FAILED;
    int nPUPID = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPUPID(oidPUP);
    CSWSCreature* pOwner = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPUPOwner(nPUPID);
    unsigned long long oidOwner = OBJECT_INVALID;
    if (pOwner != 0)
        oidOwner = pOwner->m_objectId;
    if (!g_pVirtualMachine->StackPushObject(oidOwner))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

// SetFormation: (anchor, member, formation, slot). The member is the creature the formation is
// applied to; the anchor is passed through. Both ids preset to OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandSetFormation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidAnchor = OBJECT_INVALID;
    unsigned long long oidCreature = OBJECT_INVALID;
    int nFormation;
    int nSlot;
    if (!g_pVirtualMachine->StackPopObject(&oidAnchor))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nFormation))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nSlot))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0)
        pCreature->SetFormation(oidAnchor, nFormation, nSlot);
    return 0;
}

// CutsceneMove: queues an uninterruptible cutscene walk on OBJECT_SELF. The popped object is the
// move target handed to the action, not the mover, which is always OBJECT_SELF.
int CSWVirtualMachineCommands::ExecuteCommandCutsceneMove(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    Vector vDest;
    int bRun;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopVector(&vDest))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bRun))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->AddCutsceneMoveActions(oidTarget, vDest, bRun);
    return 0;
}

// AddAvailableNPCByObject: binds an already-spawned creature to an availability slot in the party
// table. A null creature is passed through; AddNPC does its own checking.
int CSWVirtualMachineCommands::ExecuteCommandAddAvailableNPCByObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNPC;
    unsigned long long oidCreature;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(oidCreature);
    int nResult = pServer->GetPartyTable()->AddNPC(nNPC, pCreature);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// The AddAvailableNPCByObject twin for puppets.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandAddAvailablePUPByObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nPUP;
    unsigned long long oidCreature;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (!g_pVirtualMachine->StackPopInteger(&nPUP))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(oidCreature);
    int nResult = pServer->GetPartyTable()->AddPUP(nPUP, pCreature);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

// GetCommandable: the object argument is optional and defaults to OBJECT_SELF. The lookup result is
// not null-tested, so a successful lookup yielding a null pointer would fault.
int CSWVirtualMachineCommands::ExecuteCommandGetCommandable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidObject = m_oidSelf;
    CGameObject* pObject;
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopObject(&oidObject))
            return VM_ERROR_POP_FAILED;
    }
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 leaves this uninitialised when the lookup fails
    int bCommandable;
#else
    int bCommandable = 0;
#endif
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS)
        bCommandable = static_cast<CSWSObject*>(pObject)->m_bCommandable;
    if (!g_pVirtualMachine->StackPushInteger(bCommandable))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetIsWeaponEffective: asks OBJECT_SELF whether its main- or off-hand weapon can hurt the popped
// target. Not a creature answers 0.
int CSWVirtualMachineCommands::ExecuteCommandGetIsWeaponEffective(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidVersus;
    int bOffHand;
    if (!g_pVirtualMachine->StackPopObject(&oidVersus))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bOffHand))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    int bEffective = 0;
    if (pCreature != 0)
        bEffective = pCreature->GetIsWeaponEffective(oidVersus, bOffHand);
    if (!g_pVirtualMachine->StackPushInteger(bEffective))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetSpellId: the spell OBJECT_SELF is currently casting, off the shared object cast block. Anything
// that is not a creature pushes -1.
int CSWVirtualMachineCommands::ExecuteCommandGetSpellId(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nSpellId = -1;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        nSpellId = pObject->AsSWSCreature()->m_castSpellId;
    if (!g_pVirtualMachine->StackPushInteger(nSpellId))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetIsPlayerMadeCharacter: the character-creation flag, i.e. "is this the player character".
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandGetIsPlayerMadeCharacter(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    int bPlayerMade = 0;
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
        bPlayerMade = pObject->AsSWSCreature()->m_bPlayerMadeCharacter;
    if (!g_pVirtualMachine->StackPushInteger(bPlayerMade))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// GetUserActionsPending: whether OBJECT_SELF still has player-queued actions waiting.
int CSWVirtualMachineCommands::ExecuteCommandGetUserActionsPending(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nPending = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        nPending = pObject->AsSWSCreature()->GetUserActionsPending();
    if (!g_pVirtualMachine->StackPushInteger(nPending))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GivePlotXP: awards the plot-XP row named by the popped string, scaled by a percentage. The string
// pop failure returns -2000 where every sibling command returns -2001; only the integer pop uses
// -2001.
int CSWVirtualMachineCommands::ExecuteCommandGivePlotXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sPlotName("");
    int nPercent;
    if (!g_pVirtualMachine->StackPopString(&sPlotName))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nPercent))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetPartyTable()->GivePlotXP(sPlotName, nPercent);
    return 0;
}

// RemoveHeartbeat: blanks a placeable's OnHeartbeat script name. The placeable probe is the only
// type filter, so calling it on anything else is a silent no-op.
int CSWVirtualMachineCommands::ExecuteCommandRemoveHeartbeat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        if (pPlaceable != 0)
            pPlaceable->m_scriptsOn[CSWSPlaceable::ON_SCRIPT_HEARTBEAT] = CExoString("");
    }
    return 0;
}

// SetLightsaberPowered: a client-side override -- the server id is translated first, then the
// client creature's blade state is forced.
int CSWVirtualMachineCommands::ExecuteCommandSetLightsaberPowered(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int bOverride;
    int bLeftBlade;
    int bRightBlade;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bOverride))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bLeftBlade))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bRightBlade))
        return VM_ERROR_POP_FAILED;
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CSWCCreature* pCreature =
        pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidCreature));
    if (pCreature != 0)
        pCreature->SetOverrideLightsaberState(bOverride, bLeftBlade, bRightBlade);
    return 0;
}

// ResetCreatureAILevel: re-applies the creature's own stored AI level to the AI master, undoing any
// SetAILevel override.
int CSWVirtualMachineCommands::ExecuteCommandResetCreatureAILevel(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            int nAILevel = pCreature->m_nSavedAILevel;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pCreature, nAILevel);
        }
    }
    return 0;
}

// GetHasSpell: whether the creature knows the given spell or power. Command 784 asks about the raw
// known list; every other id routed here asks the filtered one.
int CSWVirtualMachineCommands::ExecuteCommandGetHasSpell(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
#if KOTOR1
    K2_UNUSED(nCommand);
#endif
    unsigned long long oidCreature;
    int nSpell;
    if (!g_pVirtualMachine->StackPopInteger(&nSpell))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bHasSpell = 0;
    if (pCreature != 0)
#if KOTOR1  // K1 always runs the known-list test (no command 784)
        bHasSpell = pCreature->m_pStats->HasSpell(0, nSpell, 1);
#else
        bHasSpell = pCreature->m_pStats->HasSpell(0, nSpell, nCommand != CMD_GetSpellAcquired);
#endif
    if (!g_pVirtualMachine->StackPushInteger(bHasSpell))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetCurrentForm: switches the creature's combat stance, but only to a form it actually knows. The
// lookup result is discarded; only the out-pointer is tested.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetCurrentForm(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nForm;
    unsigned long long oidCreature;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nForm))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject);
    if (pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_pStats->HasSpell(0, nForm, 0))
            pCreature->m_stanceId = nForm;
    }
    return 0;
}
#endif // !KOTOR1

// SetMinOneHP: the "damage cannot kill this object" flag, normalised to 0/1 through an if/else
// rather than a compare-and-set.
int CSWVirtualMachineCommands::ExecuteCommandSetMinOneHP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int bMinOneHP;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bMinOneHP))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (bMinOneHP != 0)
            pSWSObject->m_bMin1HP = 1;
        else
            pSWSObject->m_bMin1HP = 0;
    }
    return 0;
}

// EnableRain: drives the client area's rain channel at either full or zero intensity.
int CSWVirtualMachineCommands::ExecuteCommandEnableRain(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int bEnable = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bEnable))
        return VM_ERROR_POP_FAILED;
    float fAmount = 0.0f;
    if (bEnable != 0)
        fAmount = 100.0f;
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0)
        g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->SetWeather(CSWCArea::WEATHER_RAIN, fAmount);
    return 0;
}

// CloseDoor: queues the close action on OBJECT_SELF, but only while the implementer is running in
// script-arg mode 1; every other mode drops the request silently after the pop.
int CSWVirtualMachineCommands::ExecuteCommandCloseDoor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidDoor;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidDoor))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1) {
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                == CGameObjectArray::SUCCESS
            && static_cast<CSWSObject*>(pObject)->GetArea() != 0)
            pObject->AsSWSObject()->AddCloseDoorAction(oidDoor, 0);
    }
    return 0;
}


// SpeakStringByStrRef: queues the speak action with the talk-table string ref in parameter slot
// 0 and an animation id in slot 1. The talk volume is translated on the way in:
// TALKVOLUME_WHISPER (1) and TALKVOLUME_SHOUT (2) pick their own animations, everything else falls
// back to the plain talk animation.
int CSWVirtualMachineCommands::ExecuteCommandSpeakStrRef(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nStrRef;
    int nTalkVolume;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nTalkVolume))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg != 1)
        return 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;
    int nAnimation = 8;
    if (nTalkVolume == TALKVOLUME_WHISPER)
        nAnimation = 10;
    else if (nTalkVolume == TALKVOLUME_SHOUT)
        nAnimation = 9;
    if (pObject->AsSWSObject() == 0)
        return 0;
    if (pObject->AsSWSObject()->m_bCommandable == 0)
        return 0;
    pObject->AsSWSObject()->AddAction(AIACTION_SPEAK_STRREF, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nStrRef, ACTION_PARAM_INT, &nAnimation,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 0;
}

// SetCameraFacing: queues the camera-facing action with five parameter slots -- a mode flag, the
// facing itself,
// and three zeroed extras (distance, pitch, transition type) this one-argument entry point never
// fills in. The popped angle is scaled by 180/pi on the way in.
int CSWVirtualMachineCommands::ExecuteCommandSetCameraLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMode;
    float fAngle;
    float fDistance;
    float fPitch;
    int nTransitionType;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopFloat(&fAngle))
        return VM_ERROR_POP_FAILED;
    nMode = 1;
    fAngle = fAngle * (180.0f / 3.1415926535f);
    fDistance = 0.0f;
    fPitch = 0.0f;
    nTransitionType = 0;
    if (m_nScriptRunArg != 1)
        return 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0
        && pObject->AsSWSCreature()->m_bCommandable != 0)
        pObject->AsSWSCreature()->AddAction(AIACTION_ORIENT_CAMERA, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nMode, ACTION_PARAM_FLOAT, &fAngle, ACTION_PARAM_FLOAT, &fDistance,
            ACTION_PARAM_FLOAT, &fPitch, ACTION_PARAM_INT, &nTransitionType,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 0;
}

// ActionPauseConversation: suspends OBJECT_SELF's running dialogue. Runs only for a live script
// frame on a commandable object. A speaker that is dead -- or a party member whose hit points have
// already fallen to zero -- cannot be paused and drops its orientation lock instead. The normal path
// raises the dialog-paused flag and, for creatures, sets bit 2 of the suppressed-activity mask.
int CSWVirtualMachineCommands::ExecuteCommandActionPauseConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    if (m_nScriptRunArg != 1)
        return 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject->AsSWSObject() == 0)
        return 0;
    if (pObject->AsSWSObject()->m_bCommandable == 0)
        return 0;
    int bDown = 0;
#if KOTOR1  // K1 uses GetDeadTemp() and GetIsPCDying()
    if (pObject->AsSWSCreature() != 0)
        bDown = pObject->AsSWSCreature()->GetDeadTemp();
    if (pObject->AsSWSObject()->GetDead() == 1 || pObject->AsSWSObject()->GetIsPCDying() == 1 ||
        bDown == 1) {
#else
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        bDown = pCreature->GetDeadTemp();
    }
    if (pObject->AsSWSObject()->GetDead() == 1) {
        pObject->AsSWSObject()->SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
    } else if (bDown != 0) {
#endif
        pObject->AsSWSObject()->SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
    } else {
        pObject->AsSWSObject()->m_bDialogPaused = 1;
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_nLockedActivityFlags |= 4;
    }
    return 0;
}

// ActionJumpToObject: command 196 queues the jump, command 385 puts it at the front of the action
// queue; everything else about the two arms is identical. The action carries the destination
// object in parameter slot 0 and the walk-straight-line flag (on by default) in slot 1. A
// non-commandable creature silently drops it.
int CSWVirtualMachineCommands::ExecuteCommandActionJumpToObject(int nCommand, int nArgCount)
{
    unsigned long long oidTarget;
    int bWalkStraightLine = 1;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bWalkStraightLine))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature == 0 || pCreature->m_bCommandable == 0)
        return 0;
    if (nCommand == CMD_ActionJumpToObject)
        pCreature->AddAction(AIACTION_JUMP_TO_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &bWalkStraightLine,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    else
        pCreature->AddActionToFront(AIACTION_JUMP_TO_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &bWalkStraightLine,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 0;
}

// OpenStore: opens a merchant screen for one PC. The two optional arguments are the per-visit bonus
// mark-up and mark-down percentages, each clamped to [-100, 100] and parked on the store object
// itself beside its own permanent mark-up and mark-down before the panel is shown. The queued
// signal event names the shopper, which is the popped player's own game object when the id resolves
// to a client, otherwise the local player's creature. The event is handed to the AI master and not
// deleted here; only the unwind path frees it.
int CSWVirtualMachineCommands::ExecuteCommandOpenStore(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidStore;
    unsigned long long oidPC;
    int nBonusMarkUp = 0;
    int nBonusMarkDown = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidStore))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidPC))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nBonusMarkUp))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nBonusMarkDown))
            return VM_ERROR_POP_FAILED;
    }
    if (nBonusMarkUp > 99)
        nBonusMarkUp = 100;
    else if (nBonusMarkUp < -99)
        nBonusMarkUp = -100;
    if (nBonusMarkDown > 99)
        nBonusMarkDown = 100;
    else if (nBonusMarkDown < -99)
        nBonusMarkDown = -100;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidPC);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidStore);
    if (pObject == 0)
        return 0;
    CSWSStore* pStore = pObject->AsSWSStore();
    if (pStore == 0)
        return 0;
    pStore->m_nBonusMarkUp = nBonusMarkUp;
    pStore->m_nBonusMarkDown = nBonusMarkDown;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowStoreGui(oidStore, oidPC);
    CScriptEvent* pEvent = new CScriptEvent;
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN;
    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (pPlayer != 0)
        pAI->AddEventDeltaTime(0, 0, pPlayer->GetGameObject()->m_objectId, oidStore,
                               CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    else
        pAI->AddEventDeltaTime(0, 0, g_pAppManager->m_pServerExoApp->GetPlayerCreatureId(),
                               oidStore, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    return 0;
}

// BarkString: pops a speaker and a talk-table string ref and raises the floating bark bubble. The
// first pop failure returns -2000, not -2001 like every sibling and like this command's own later
// pops. Two optional arguments are stashed on the bark-bubble panel (defaulting to -1) before
// anything is fetched, and they are written even when the bark is then suppressed. The speaker id
// decides which sink runs: OBJECT_INVALID barks through the GUI directly, anything else is
// translated server-to-client first. The talk-table row supplies both the text and the VO resref.
int CSWVirtualMachineCommands::ExecuteCommandBarkString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nStrRef;
    unsigned long long oidSpeaker;
    if (!g_pVirtualMachine->StackPopObject(&oidSpeaker))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 takes two arguments (no bubble position)
    K2_UNUSED(nArgCount);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return 0;
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    CResRef resRef = strRes.soundResRef;
    CExoString sText(strRes.text);
    if (nStrRef != STRREF_INCOMING_FIGHTERS) {
        if (oidSpeaker == OBJECT_INVALID)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowBarkBubble(oidSpeaker, sText,
                                                                          resRef, 0);
        else {
            oidSpeaker = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidSpeaker);
            g_pAppManager->m_pClientExoApp->BarkString(oidSpeaker, sText, resRef, 0);
        }
    }
    return 0;
#else
    int nBarkX = -1;
    int nBarkY = -1;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nBarkX))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nBarkY))
            return VM_ERROR_POP_FAILED;
    }
    CSWGuiBarkBubble* pBarkBubble =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pBarkBubble;
    if (pBarkBubble != 0) {
        int nX = nBarkX;
        int nY = nBarkY;
        pBarkBubble->m_nOverrideX = nX;
        pBarkBubble->m_nOverrideY = nY;
    }
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return 0;
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    CResRef resRef = strRes.soundResRef;
    CExoString sText(strRes.text);
    if (oidSpeaker == OBJECT_INVALID)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowBarkBubble(oidSpeaker, sText, resRef,
                                                                      0);
    else {
        oidSpeaker = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidSpeaker);
        g_pAppManager->m_pClientExoApp->BarkString(oidSpeaker, sText, resRef, 0);
    }
    return 0;
#endif
}

// GetEffect (first and next): one cursor-driven iterator over an object's
// effect list, with the command id deciding only whether the cursor is reset first. The cursor lives
// on the object, so two scripts iterating it share the cursor. Not every entry is script-visible: an
// entry must be exposed, must not be an icon effect, and its duration selector must be temporary or
// permanent; instantaneous, equipped and innate effects are skipped. Running off the end resets the
// cursor and pushes a fresh invalid effect, so a script loop terminates. The pushed structure is
// deleted on every path except the one where it came out of the list.
int CSWVirtualMachineCommands::ExecuteCommandGetEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = 0;
    int bFromList = 0;
    CGameObject* pGameObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    CSWSObject* pObject = 0;
    if (pGameObject != 0)
        pObject = pGameObject->AsSWSObject();
    if (pObject == 0) {
        pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
        if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
            return VM_ERROR_PUSH_FAILED;
        delete pEffect;
        return 0;
    } else {
        int nCursor;
        if (nCommand == CMD_GetFirstEffect) {
            pObject->m_nEffectIterator = 0;
            nCursor = 0;
        } else {
            nCursor = pObject->m_nEffectIterator;
        }
        int nCount = pObject->m_effects.m_count;
        while (nCursor < nCount) {
            CGameEffect* pEach = pObject->m_effects.m_pArray[nCursor];
            unsigned short nDuration = pEach->m_durationTypeBits & CGameEffect::DURATION_TYPE_MASK;
            if (pEach->m_bIsExposed != 0
                    && pEach->m_subType != CSWSEffectListHandler::EFFECT_EFFECT_ICON
                    && (nDuration == 1 || nDuration == 2)) {
                pEffect = pObject->m_effects.m_pArray[nCursor];
                pObject->m_nEffectIterator = static_cast<unsigned short>(nCursor + 1);
                bFromList = 1;
                break;
            }
            nCursor++;
        }
        if (bFromList == 0) {
            pObject->m_nEffectIterator = 0;
            pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
        }
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    if (pEffect == 0)
        return 0;
    if (bFromList != 0)
        return 0;
    delete pEffect;
    return 0;
}

// ChangeByFaction: moves every creature in OBJECT_SELF's area that currently belongs to one faction
// into another. The area is found from OBJECT_SELF itself -- an ordinary object answers through its
// GetArea(), an area answers as itself -- and the sweep walks the area's object list.
// The command id picks the follow-up. One arm pacifies each converted creature (drop combat, clear
// the action queue), anything else just rebuilds its visible list. The faction move is the same on
// both paths.
int CSWVirtualMachineCommands::ExecuteCommandChangeByFaction(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nOldFaction;
    int nNewFaction;
    unsigned long long oidEach;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nOldFaction))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nNewFaction))
        return VM_ERROR_POP_FAILED;
    CFactionManager* pFactionManager = g_pAppManager->m_pServerExoApp->GetFactionManager();
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (m_nScriptRunArg == 0)
        return 0;
    if (pObjectArray->GetGameObject(m_oidSelf, &pObject) != CGameObjectArray::SUCCESS)
        return 0;
    CSWSArea* pArea;
    if (pObject->AsSWSObject() != 0) {
        pArea = static_cast<CSWSArea*>(pObject->AsSWSObject()->GetArea());
    } else {
        if (pObject->AsSWSArea() == 0)
            return 0;
        pArea = pObject->AsSWSArea();
    }
    if (pFactionManager == 0 || pArea == 0)
        return 0;
    CSWSFaction* pFaction = pFactionManager->GetFaction(nNewFaction);
    if (pArea->GetFirstObjectInArea(oidEach) != 1)
        return 0;
    do {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidEach);
        if (pCreature != 0) {
            pCreature->m_sTag.CStr();
            if (pCreature->m_pStats->m_factionId == nOldFaction) {
                pFaction->AddMember(oidEach, 0);
                if (nCommand == CMD_SurrenderByFaction) {
#if KOTOR1
                    pCreature->SetCombatState(0, 1);
#else
                    pCreature->SetCombatState(0, 1, 0);
#endif
                    pCreature->ClearAllActions(1);
                    pCreature->PacifyCreature();
                } else {
                    pCreature->ClearVisibleList();
                    pCreature->UpdateVisibleList(0);
                }
            }
        }
    } while (pArea->GetNextObjectInArea(oidEach) == 1);
    return 0;
}

// ChangeFaction: one command copies another object's faction and another sets one by id, behind one
// handler. Neither form touches a player character, and the copy form also requires the donor to be a
// non-PC creature whose faction resolves. Placeables and triggers keep a plain faction field written
// directly; everything else is enrolled in the faction's member list.
int CSWVirtualMachineCommands::ExecuteCommandChangeFaction(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nFaction K2_UNSET;
    int nFactionId K2_UNSET;
    unsigned long long oidObject;
    unsigned long long oidDonor K2_UNSET;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (nCommand == CMD_ChangeFaction) {
        if (!g_pVirtualMachine->StackPopObject(&oidDonor))
            return VM_ERROR_POP_FAILED;
    } else if (nCommand == CMD_ChangeToStandardFaction) {
        if (!g_pVirtualMachine->StackPopInteger(&nFactionId))
            return VM_ERROR_POP_FAILED;
    }
    if (g_pAppManager->m_pServerExoApp->m_pInternal == 0)
        return 0;
    CFactionManager* pFactionManager =
        g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
    if (pFactionManager == 0)
        return 0;
    if (nCommand == CMD_ChangeFaction) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
        if (pCreature == 0)
            return 0;
        if (pCreature->m_pStats->m_bIsPC != 0)
            return 0;
        if (!g_pAppManager->m_pServerExoApp->GetFactionOfObject(oidDonor, &nFaction))
            return 0;
        CSWSCreature* pDonor =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidDonor);
        if (pDonor == 0)
            return 0;
        if (pDonor->m_pStats->m_bIsPC != 0)
            return 0;
    } else if (nCommand == CMD_ChangeToStandardFaction) {
        if (!pFactionManager->GetIsNPCFaction(nFactionId))
            return 0;
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
        if (pCreature != 0 && pCreature->m_pStats->m_bIsPC != 0)
            return 0;
        nFaction = nFactionId;
    }
#if KOTOR1  // K1 keeps placeable factions to themselves and does not special-case triggers
    CSWSPlaceable* pPlaceable =
        g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidObject);
    if (pPlaceable != 0)
        pPlaceable->m_nFaction = nFaction;
    else
        pFactionManager->GetFaction(nFaction)->AddMember(oidObject, 0);
    return 0;
#else
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject == 0)
        return 0;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        pPlaceable->m_nFaction = nFaction;
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
        CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
        pTrigger->m_nFaction = nFaction;
    } else
        pFactionManager->GetFaction(nFaction)->AddMember(oidObject, 0);
    return 0;
#endif
}

// Answers the level of whatever is casting right now: a creature from its own pending cast, a
// placeable from the power's table (it has no class levels), and an area-of-effect object from its
// creator.
int CSWVirtualMachineCommands::ExecuteCommandGetCasterLevel(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nCasterLevel = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        unsigned int nSpellId = 0;
        if (pCreature != 0) {
            nSpellId = pCreature->m_castSpellId;
        } else if (pPlaceable == 0 && pObject->AsSWSAreaOfEffectObject() != 0) {
            nSpellId = pObject->AsSWSAreaOfEffectObject()->GetEffectSpellId();
            unsigned long long oidCreator = pObject->AsSWSAreaOfEffectObject()->m_oidCreator;
            pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreator);
            pPlaceable = g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidCreator);
        }
        CSWSpell* pSpell = 0;
        if (pCreature != 0) {
            if (pCreature->m_bItemCastSpell != 0) {
                nCasterLevel = pCreature->m_nItemCastSpellLevel;
            } else if (pCreature->m_nCastClassIndex == CAST_CLASS_NONE) {
                pSpell = g_pRules->GetSpell(pCreature->m_castSpellId);
            } else if (pCreature->m_nCastClassIndex == CAST_CLASS_SPELL_LIKE_ABILITY) {
                nCasterLevel = pCreature->m_pStats->GetSpellLikeAbilityCasterLevel(nSpellId);
            } else {
                nCasterLevel = pCreature->m_pStats->GetClassLevel(pCreature->m_nCastClassIndex, 1);
            }
        } else if (pPlaceable != 0) {
            pSpell = g_pRules->GetSpell(pPlaceable->m_castSpellId);
        }
        if (pSpell != 0) {
            nCasterLevel = pSpell->m_nSpellLevelOther * 2 - 1;
            if (nCasterLevel < 10)
                nCasterLevel = 10;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nCasterLevel))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetAbilityScore: the six ability getters behind one handler, selected by the popped ability constant
// rather than by the command id. The selector constants are the EFFECT_ABILITY_* statics, so the
// script-visible ability ids and the engine's effect subtypes are the same six values. The score is a
// byte, truncated on the way into the push.
int CSWVirtualMachineCommands::ExecuteCommandGetAbilityScore(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAbility;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAbility))
        return VM_ERROR_POP_FAILED;
    unsigned char nScore = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH)
            nScore = pObject->AsSWSCreature()->m_pStats->GetSTRStat();
        else if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY)
            nScore = pObject->AsSWSCreature()->m_pStats->GetDEXStat();
        else if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION)
            nScore = pObject->AsSWSCreature()->m_pStats->GetCONStat();
        else if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE)
            nScore = pObject->AsSWSCreature()->m_pStats->GetINTStat();
        else if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_WISDOM)
            nScore = pObject->AsSWSCreature()->m_pStats->GetWISStat();
        else if (nAbility == CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA)
            nScore = pObject->AsSWSCreature()->m_pStats->GetCHAStat();
    }
    if (!g_pVirtualMachine->StackPushInteger(nScore))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// MoveAwayFromObject: queues action 3 with four parameters -- the object to flee, the run flag, the
// radius and a constant 10. Both optional arguments default in their declarations.
int CSWVirtualMachineCommands::ExecuteCommandMoveAwayFromObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidTarget;
    int bRun = 0;
    float fRange = CSWSCreature::MOVE_AWAY_RADIUS;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bRun))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3 && !g_pVirtualMachine->StackPopFloat(&fRange))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    CGameObject* pTarget;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0 && pObject->AsSWSCreature()->m_bCommandable != 0
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pTarget)
               == CGameObjectArray::SUCCESS) {
        int nAttempts = 10;
        static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_CHECK_MOVE_AWAY_FROM_OBJECT, ACTION_GROUP_PREVIOUS, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &bRun, ACTION_PARAM_FLOAT, &fRange,
                ACTION_PARAM_INT, &nAttempts, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 0;
}

// Re-points the save-game party table at the live objects for every companion, matching each
// companion's fixed creature tag and writing the found id back into its slot; the thirteenth lookup
// does the same for the remote puppet. The tag-to-slot mapping is hard-coded here and unavailable
// companions are skipped rather than cleared.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandRebuildPartyTable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0)
        return 0;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule == 0)
        return 0;
    unsigned long long oidMember;
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("atton"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(0))
        pPartyTable->SetNPCObject(0, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("baodur"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(1))
        pPartyTable->SetNPCObject(1, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("mand"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(2))
        pPartyTable->SetNPCObject(2, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("g0t0"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(3))
        pPartyTable->SetNPCObject(3, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("handmaiden"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(4))
        pPartyTable->SetNPCObject(4, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("hk47"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(5))
        pPartyTable->SetNPCObject(5, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("kreia"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(6))
        pPartyTable->SetNPCObject(6, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("mira"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(7))
        pPartyTable->SetNPCObject(7, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("t3m4"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(8))
        pPartyTable->SetNPCObject(8, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("visasmarr"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(9))
        pPartyTable->SetNPCObject(9, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("hanharr"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(10))
        pPartyTable->SetNPCObject(10, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("disciple"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsNPCAvailable(11))
        pPartyTable->SetNPCObject(11, oidMember);
    oidMember = pModule->FindObjectByTagOrdinal(CExoString("remote"), 0);
    if (oidMember != OBJECT_INVALID && pPartyTable->GetIsPUPAvailable(0))
        pPartyTable->SetPUPObject(0, oidMember);
    return 0;
}
#endif

// One handler for 24 NWScript effect constructors. The shared prologue news the effect, seeds the
// invalid subtype, marks it magical and attaches OBJECT_SELF as creator; each case
// only stamps its own subtype and pops its extra arguments. Eight of the state-setting commands set
// slot 0 alone, three also set slot 1, and two pop an argument purely to keep the VM stack
// balanced.
int CSWVirtualMachineCommands::ExecuteCommandEffectComplex(int nCommand, int nArgCount)
{
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    switch (nCommand) {
    case CMD_EffectAssuredHit:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ASSUREDHIT);
        break;
    case CMD_EffectEntangle:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ENTANGLE);
        break;
    case CMD_EffectDeath: {
        int bSpectacularDeath = 0;
        int bDisplayFeedback = 1;
        int bNoFadeAway = 0;
        if (nArgCount >= 1) {
            if (!g_pVirtualMachine->StackPopInteger(&bSpectacularDeath))
                return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 EffectDeath has no bNoFadeAway argument
            if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bDisplayFeedback))
                return VM_ERROR_POP_FAILED;
        }
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
        pEffect->SetInteger(0, bSpectacularDeath);
        pEffect->SetInteger(1, bDisplayFeedback);
        (void)bNoFadeAway;
#else
            if (nArgCount >= 2) {
                if (!g_pVirtualMachine->StackPopInteger(&bDisplayFeedback))
                    return VM_ERROR_POP_FAILED;
                if (nArgCount >= 3 && !g_pVirtualMachine->StackPopInteger(&bNoFadeAway))
                    return VM_ERROR_POP_FAILED;
            }
        }
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
        pEffect->SetInteger(0, bSpectacularDeath);
        pEffect->SetInteger(1, bDisplayFeedback);
        pEffect->SetInteger(2, bNoFadeAway);
#endif
        break;
    }
    case CMD_EffectKnockdown:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_KNOCKDOWN);
        break;
    case CMD_EffectParalyze:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_PARALYZED);
        break;
    case CMD_EffectSleep:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_SLEEP);
        break;
    case CMD_EffectConfused:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_CONFUSED);
        break;
    case CMD_EffectFrightened:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_FRIGHTENED);
        break;
    case CMD_EffectStunned:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_STUNNED);
        break;
    case CMD_EffectRegenerate: {
        int nAmount;
        float fInterval;
        if (!g_pVirtualMachine->StackPopInteger(&nAmount))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopFloat(&fInterval))
            return VM_ERROR_POP_FAILED;
        int nMilliseconds = static_cast<int>(fInterval * 1000.0f);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_REGENERATE);
        pEffect->SetInteger(0, nAmount);
        pEffect->SetInteger(1, nMilliseconds);
        break;
    }
    case CMD_EffectAssuredDeflection: {
        int nAttacks = 0;
        if (!g_pVirtualMachine->StackPopInteger(&nAttacks))
            return VM_ERROR_POP_FAILED;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ASSUREDDEFLECTION);
        pEffect->SetInteger(0, nAttacks);
        break;
    }
    case CMD_EffectHaste:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_HASTE);
        break;
    case CMD_EffectForceResisted: {
        unsigned long long oidCaster = OBJECT_INVALID;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCERESISTED);
        if (!g_pVirtualMachine->StackPopObject(&oidCaster))
            return VM_ERROR_POP_FAILED;
        pEffect->SetObjectID(0, oidCaster);
        break;
    }
    case CMD_EffectForceFizzle:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEFIZZLE);
        break;
    case CMD_EffectDispelMagicAll: {
        int nCasterLevel;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DISPEL_ALL_MAGIC);
        if (!g_pVirtualMachine->StackPopInteger(&nCasterLevel))
            return VM_ERROR_POP_FAILED;
        break;
    }
    case CMD_EffectDisguise: {
        int nAppearance;
        if (!g_pVirtualMachine->StackPopInteger(&nAppearance))
            return VM_ERROR_POP_FAILED;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DISGUISE);
        pEffect->SetInteger(0, nAppearance);
        pEffect->SetInteger(1, 0);
        break;
    }
    case CMD_EffectTimeStop:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TIMESTOP);
        break;
    case CMD_EffectDispelMagicBest: {
        int nCasterLevel;
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DISPEL_BEST_MAGIC);
        if (!g_pVirtualMachine->StackPopInteger(&nCasterLevel))
            return VM_ERROR_POP_FAILED;
        break;
    }
    case CMD_EffectCutSceneHorrified:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_HORRIFIED);
        pEffect->SetInteger(1, 1);
        break;
    case CMD_EffectCutSceneParalyze:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_PARALYZED);
        pEffect->SetInteger(1, 1);
        break;
    case CMD_EffectCutSceneStunned:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_STUNNED);
        pEffect->SetInteger(1, 1);
        break;
#if !KOTOR1  // K2-only script commands
    case CMD_EffectDroidConfused:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_DROID_CONFUSED);
        break;
    case CMD_EffectMindTrick:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_MIND_TRICK);
        break;
    case CMD_EffectDroidScramble:
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
        pEffect->SetInteger(0, EFFECT_STATE_DROID_SCRAMBLE);
        break;
#endif
    }
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Resolves one touch attack from OBJECT_SELF against a target and pushes 0 (miss), 1 (hit) or 2
// (critical). A non-creature target is an automatic hit for a door or placeable and a miss for
// anything else, and a non-creature attacker always misses. Defensive effects resolve before the
// roll and override it, and a natural 20 re-rolls to confirm the critical.
int CSWVirtualMachineCommands::ExecuteCommandDoTouchAttack(int nCommand, int nArgCount)
{
    unsigned long long oidTarget;
    int bDisplayFeedback = 1;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bDisplayFeedback))
        return VM_ERROR_POP_FAILED;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pTarget;
    CGameObject* pSelf;
    int nResult = 0;
    int nAttackResult = 0;
    int nConcealment = 0;
#ifdef K2_FIX_ENGINE_BUGS
    int nAttackBonus = 0;
#else
    int nAttackBonus;
#endif
#ifdef K2_FIX_ENGINE_BUGS
    int nRoll = 0;
#else
    int nRoll;
#endif
    if (pArray->GetGameObject(oidTarget, &pTarget) == CGameObjectArray::SUCCESS
            && pArray->GetGameObject(m_oidSelf, &pSelf) == CGameObjectArray::SUCCESS) {
        if (pSelf->AsSWSCreature() != 0) {
            if (pTarget->AsSWSCreature() != 0) {
                CSWSCreatureStats* pStats = pSelf->AsSWSCreature()->m_pStats;
                if (nCommand == CMD_TouchAttackMelee)
                    nAttackBonus = pStats->GetMeleeAttackBonus(0, 1, 1, 0);
                else
                    nAttackBonus = pStats->GetRangedAttackBonus(0, 1, 1);
                short nArmorClass = pTarget->AsSWSCreature()->m_pStats
                        ->GetArmorClassVersus(pSelf->AsSWSCreature(), 1);
                nRoll = g_pRules->RollDice(1, 20);
                if (pSelf->AsSWSCreature()->ResolveDefensiveEffects(pTarget->AsSWSObject(), 1)) {
                    int nAttack = pSelf->AsSWSCreature()->m_pCombatRound->m_currentAttack;
                    CSWSCombatAttackData* pAttack =
                        pSelf->AsSWSCreature()->m_pCombatRound->GetAttack(nAttack);
                    nAttackResult = pAttack->m_nAttackResult;
                    nConcealment = pAttack->m_nConcealment;
                    nResult = 0;
                } else if (nRoll != 1
                        && (nRoll == 20 || nAttackBonus + nRoll >= nArmorClass)) {
                    nResult = 1;
                    if (nRoll == 20) {
                        if (nAttackBonus + g_pRules->RollDice(1, 20) >= nArmorClass)
                            nResult = 2;
                    }
                }
            } else {
                int bDoor = pTarget->m_objectType == CGameObject::OBJECT_TYPE_DOOR;
                int bPlaceable = pTarget->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE;
                nResult = bDoor || bPlaceable;
            }
        }
    } else {
        bDisplayFeedback = 0;
        nResult = 0;
    }
    if (bDisplayFeedback == 1) {
        CSWCCMessageData* pMessage = new CSWCCMessageData;
        pMessage->SetObjectID(0, m_oidSelf);
        pMessage->SetObjectID(1, pTarget->m_objectId);
        pMessage->SetInteger(0, nAttackBonus);
        pMessage->SetInteger(1, nRoll);
        if (nResult == 2) {
            pMessage->SetInteger(2, 2);
        } else if (nResult == 1) {
            pMessage->SetInteger(2, 1);
        } else if (nResult == 0) {
            if (nAttackResult != 0) {
                pMessage->SetInteger(2, nAttackResult);
                pMessage->SetInteger(4, nConcealment);
            } else {
                pMessage->SetInteger(2, 4);
            }
        }
        if (nCommand == CMD_TouchAttackRanged)
            pMessage->SetInteger(3, 1);
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_oidSelf);
        if (pPlayer)
            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_ATTACK_ROLL, pMessage, 0);
        pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pTarget->m_objectId);
        if (pPlayer)
            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_ATTACK_ROLL, pMessage, 0);
        delete pMessage;
    }
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Shifts a creature's good/evil score, and for a player-made character keeps the two running
// light/dark totals in step. Neutral walks the score toward 50; light and dark move it outright. The
// optional fourth argument gates the totals bookkeeping and the feedback message, so a nonzero one
// makes this a silent score change.
int CSWVirtualMachineCommands::ExecuteCommandAdjustAlignment(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
#if KOTOR1
    K2_UNUSED(nArgCount);
    // K1 has no silent flag and no PC light/dark totals; the feedback always goes out.
    unsigned long long oidTarget;
    int nAlignment;
    int nShift;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAlignment))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nShift))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pCreature == 0)
        return 0;
    CSWSCreatureStats* pStats = pCreature->m_pStats;
    pStats->GetSimpleAlignmentGoodEvil();
    int nAlign = pStats->m_nAlignment;
    short nDelta;
    if (nAlignment == 3)
        nDelta = static_cast<short>(-nShift);
    else if (nAlignment == 2)
        nDelta = static_cast<short>(nShift);
    else if (nAlignment == 1) {
        if (nAlign >= 51)
            nDelta = static_cast<short>((50 - nAlign >= -nShift) ? (50 - nAlign) : -nShift);
        else
            nDelta = static_cast<short>((nShift < 50 - nAlign) ? nShift : (50 - nAlign));
    } else
        nDelta = 0;
    pStats->ModifyAlignment(nDelta, 0);
    CSWCCMessageData* pMessage = new CSWCCMessageData();
    if (nAlignment == 1) {
        if (nAlign >= 51)
            pMessage->SetInteger(0, 3);
        else
            pMessage->SetInteger(0, 2);
        pMessage->SetInteger(1, nShift < 0 ? -nShift : nShift);
    } else {
        pMessage->SetInteger(0, nAlignment);
        pMessage->SetInteger(1, nShift);
    }
    pCreature->SendFeedbackMessage(FEEDBACK_ALIGNMENT_SHIFT, pMessage);
    if (nDelta != 0 && pCreature->m_bInParty != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ALIGNMENT_SHIFT, nDelta);
    return 0;
#else
    unsigned long long oidTarget;
    int nAlignment;
    int nShift;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAlignment))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nShift))
        return VM_ERROR_POP_FAILED;
    int bSilent = 0;
    if (nArgCount >= 4 && !g_pVirtualMachine->StackPopInteger(&bSilent))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pCreature == 0)
        return 0;
    CSWSCreatureStats* pStats = pCreature->m_pStats;
    int nAlign = pStats->m_nAlignment;
    CExoString sVarName;
    char cTotal;
    short nDelta = 0;
    int nTotal;
    CSWCCMessageData* pMessage;
    if (nAlignment == 3) {
        nDelta = static_cast<short>(-nShift);
        if (pCreature->m_bPlayerMadeCharacter != 0 && bSilent == 0) {
            sVarName = "G_PC_Dark_Total";
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
                    ->GetValueNumber(sVarName, cTotal);
            if (cTotal != 127) {
                nTotal = cTotal - nDelta;

                if (nTotal <= 127) {

                    cTotal = static_cast<char>(nTotal);

                    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                            ->SetValueNumber(sVarName, cTotal);

                } else {

                    cTotal = 127;

                    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                            ->SetValueNumber(sVarName, cTotal);

                }
            }
        }
    } else if (nAlignment == 2) {
        nDelta = static_cast<short>(nShift);
        if (pCreature->m_bPlayerMadeCharacter != 0 && bSilent == 0) {
            sVarName = "G_PC_Light_Total";
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
                    ->GetValueNumber(sVarName, cTotal);
            if (cTotal != 127) {
                nTotal = cTotal + nDelta;

                if (nTotal <= 127) {

                    cTotal = static_cast<char>(nTotal);

                    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                            ->SetValueNumber(sVarName, cTotal);

                } else {

                    cTotal = 127;

                    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                            ->SetValueNumber(sVarName, cTotal);

                }
            }
        }
    } else if (nAlignment == 1) {
        if (nAlign >= 51)
            nDelta = static_cast<short>((50 - nAlign >= -nShift) ? (50 - nAlign) : -nShift);
        else
            nDelta = static_cast<short>((nShift < 50 - nAlign) ? nShift : (50 - nAlign));
        if (pCreature->m_bPlayerMadeCharacter != 0 && bSilent == 0) {
            if (nDelta < 0) {
                sVarName = "G_PC_Dark_Total";
                g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
                        ->GetValueNumber(sVarName, cTotal);
                if (cTotal != 127) {
                    nTotal = cTotal - nDelta;

                    if (nTotal <= 127) {

                        cTotal = static_cast<char>(nTotal);

                        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                                ->SetValueNumber(sVarName, cTotal);

                    } else {

                        cTotal = 127;

                        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                                ->SetValueNumber(sVarName, cTotal);

                    }
                }
            } else if (nDelta > 0) {
                sVarName = "G_PC_Light_Total";
                g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
                        ->GetValueNumber(sVarName, cTotal);
                if (cTotal != 127) {
                    nTotal = nDelta + cTotal;

                    if (nTotal <= 127) {

                        cTotal = static_cast<char>(nTotal);

                        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                                ->SetValueNumber(sVarName, cTotal);

                    } else {

                        cTotal = 127;

                        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()

                                ->SetValueNumber(sVarName, cTotal);

                    }
                }
            }
        }
    }
    pStats->ModifyAlignment(nDelta, bSilent);
    if (bSilent == 0) {
        pMessage = new CSWCCMessageData();
        if (nAlignment == 1) {
            if (nAlign >= 51)
                pMessage->SetInteger(0, 3);
            else
                pMessage->SetInteger(0, 2);
            pMessage->SetInteger(1, nShift < 0 ? -nShift : nShift);
        } else {
            pMessage->SetInteger(0, nAlignment);
            pMessage->SetInteger(1, nShift);
        }
        pCreature->SendFeedbackMessage(FEEDBACK_ALIGNMENT_SHIFT, pMessage);
        if (nDelta != 0 && pCreature->m_bInParty != 0)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ALIGNMENT_SHIFT, nDelta);
    }
    return 0;
#endif
}

// Drops an armed mine at a location. Mines are CSWSTriggers flagged as traps, filled in from the
// traps.2da row named by the popped type and added to the module's active area. The creator supplies
// only its faction id and, for creatures, its Demolitions rank, which is stashed on the trigger for
// the later disarm or recover roll.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSpawnMine(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMineType;
    CScriptLocation* pLocation = 0;
    int nDetectDCBase;
    int nDisarmDCBase;
    unsigned long long oidCreator;
    if (g_pVirtualMachine->StackPopInteger(&nMineType)
            && g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation))
            && g_pVirtualMachine->StackPopInteger(&nDetectDCBase)
            && g_pVirtualMachine->StackPopInteger(&nDisarmDCBase)
            && g_pVirtualMachine->StackPopObject(&oidCreator)) {
        C2DA* pTraps = g_pRules->m_p2DArrays->m_pTraps;
        CExoString sTrapScript;
        CSWSCreature* pCreature = 0;
        int nFaction = 1;
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreator);
        if (pObject != 0) {
            if (pObject->AsSWSCreature() != 0) {
                pCreature = pObject->AsSWSCreature();
                nFaction = pCreature->m_pStats->m_factionId;
            } else if (pObject->AsSWSPlaceable() != 0) {
                CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
                nFaction = pPlaceable->m_nFaction;
            }
        }
        int nDetectDCMod;
        pTraps->GetINTEntry(nMineType, CExoString("DetectDCMod"), &nDetectDCMod);
        int nDisarmDCMod;
        pTraps->GetINTEntry(nMineType, CExoString("DisarmDCMod"), &nDisarmDCMod);
        pTraps->GetCExoStringEntry(nMineType, CExoString("TrapScript"), &sTrapScript);
        CSWSTrigger* pTrigger = new CSWSTrigger(OBJECT_INVALID);
        pTrigger->SetCreator(oidCreator);
        pTrigger->m_trapType = static_cast<unsigned char>(nMineType);
        pTrigger->m_scriptSlots[4] = sTrapScript;
        pTrigger->m_baseDetectDC = nDetectDCBase + nDetectDCMod;
        pTrigger->m_baseDisarmDC = nDisarmDCBase + nDisarmDCMod;
        pTrigger->m_bTrap = 1;
        pTrigger->m_bTrapDetectable = 1;
        pTrigger->m_bTrapDisarmable = 1;
        pTrigger->SetPosition(pLocation->m_vPosition, 1, 1, 0);
        pTrigger->m_nFaction = nFaction;
        if (pCreature != 0)
            pTrigger->m_nOwnerDemolitionsSkill = pCreature->m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0);
        int nTrapName = 0;
        pTraps->GetINTEntry(nMineType, CExoString("TrapName"), &nTrapName);
        pTrigger->m_localizedName.m_nStrRef = nTrapName;
        CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        pTrigger->CreateNewGeometry(2.0f, pLocation->m_vPosition, pArea);
        pTrigger->AddToArea(pArea, pLocation->m_vPosition.x, pLocation->m_vPosition.y,
                            pLocation->m_vPosition.z, 1);
        pArea->m_mineObjects.Add(pTrigger->m_objectId);
        if (pLocation != 0) {
            delete pLocation;
        }
        return 0;
    }
    if (pLocation != 0) {
        delete pLocation;
    }
    return VM_ERROR_POP_FAILED;
}
#endif // !KOTOR1

// Queues OBJECT_SELF to follow a target at a fixed distance: the follow point is the target's
// position pushed fFollowDistance along its facing. A client-visible party member follows its
// standing formation offset instead, which is why the client-side lookup appears in a server-side
// command.
int CSWVirtualMachineCommands::ExecuteCommandActionFollowObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidFollow;
    int nMoveParam = 1;
    float fFollowDistance = 0.0f;
    if (!g_pVirtualMachine->StackPopObject(&oidFollow))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fFollowDistance))
        return VM_ERROR_POP_FAILED;
    if (oidFollow == OBJECT_INVALID)
        return 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidFollow);
#ifdef K2_FIX_ENGINE_BUGS
    if (pCreature == 0 || pObject == 0 || pObject->AsSWSObject() == 0)
        return 0;
#endif
    Vector vTarget = pObject->AsSWSObject()->m_position
                     + pObject->AsSWSObject()->m_orientation * fFollowDistance;
    if (pCreature != 0) {
        unsigned long long oidClient =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pCreature->m_objectId);
        CSWCCreature* pClientCreature =
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oidClient);
        if (pClientCreature != 0 && pClientCreature->m_bInParty != 0) {
            int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(
                    pClientCreature->m_objectId);
            if (nIndex >= 0) {
                vTarget = g_pAppManager->m_pClientExoApp->GetSWParty()
                              ->m_members[nIndex].m_formationOffset;
                CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                vTarget = vTarget
                          + Vector(0.0f,
                                   -pParty->m_afFollowSpacing[pParty->m_nFormation],
                                   0.0f);
            }
        }
    }
    Vector vSelf = pCreature->m_position;
    int nCheckRange = 0;
    if (pCreature != 0) {
        pCreature->AddAction(AIACTION_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT,
                &oidFollow, ACTION_PARAM_INT, &nMoveParam, ACTION_PARAM_FLOAT, static_cast<void*>(&vTarget), ACTION_PARAM_FLOAT, static_cast<void*>(&vTarget.y),
                ACTION_PARAM_INT, &nCheckRange, ACTION_PARAM_FLOAT, static_cast<void*>(&vSelf), ACTION_PARAM_FLOAT, static_cast<void*>(&vSelf.y));
        pCreature->AddAction(AIACTION_CHECK_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT,
                &oidFollow, ACTION_PARAM_INT, &nMoveParam, ACTION_PARAM_FLOAT, static_cast<void*>(&vTarget), ACTION_PARAM_FLOAT, static_cast<void*>(&vTarget.y));
    }
    return 0;
}

// The location twin of ActionMoveAwayFromObject queues the retreat walk and then the abstract
// action the AI re-evaluates. The popped location is deleted only on the pop-failure arm, so the
// other paths leak it.
int CSWVirtualMachineCommands::ExecuteCommandActionMoveAwayFromLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CScriptLocation* pLocation;
    int bRun = 0;
    float fRange = CSWSCreature::MOVE_AWAY_RADIUS;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2
        && (!g_pVirtualMachine->StackPopInteger(&bRun)
            || (nArgCount >= 3 && !g_pVirtualMachine->StackPopFloat(&fRange)))) {
        if (pLocation != 0) {
            delete pLocation;
            pLocation = 0;
        }
        return VM_ERROR_POP_FAILED;
    }
    CGameObject* pObject;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0
        && pObject->AsSWSCreature()->m_bCommandable == 1) {
        Vector vTarget = pLocation->m_vPosition;
        CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea());
        CResRef resRef;
        if (pArea != 0) {
            const Vector& vPos = static_cast<CSWSObject*>(pObject)->m_position;
            if ((vPos.x - vTarget.x) * (vPos.x - vTarget.x)
                        + (vPos.y - vTarget.y) * (vPos.y - vTarget.y)
                        + (vPos.z - vTarget.z) * (vPos.z - vTarget.z)
                    <= fRange * fRange) {
                Vector vAway = pArea->ComputeAwayVector(vPos, vTarget, fRange,
                                                        pObject->AsSWSCreature()->m_pPathfindInfo);
                unsigned long long oidArea = static_cast<CSWSArea*>(pArea)->m_objectId;
                int nAttempts = 10;
                pObject->AsSWSCreature()->AddMoveToPointAction(ACTION_GROUP_NEW, vAway, oidArea,
                        CGameObjectArray::INVALIDOBJID, bRun, 0.0f, 0.0f,
                        0, 0, 0, 0, 1, 0, Vector{}, 0, 0, 0);
                static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_CHECK_MOVE_AWAY_FROM_LOCATION, ACTION_GROUP_PREVIOUS, ACTION_PARAM_FLOAT, &pLocation->m_vPosition.x,
                        ACTION_PARAM_FLOAT, &pLocation->m_vPosition.y, ACTION_PARAM_FLOAT, &pLocation->m_vPosition.z,
                        ACTION_PARAM_INT, &bRun, ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_INT, &nAttempts,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
            return 0;
        }
    }
    if (pLocation != 0)
        delete pLocation;
    return 0;
}

// SetIsDestroyable: three flags on OBJECT_SELF -- destroyable, raiseable and selectable-when-dead.
// Only object types above OBJECT_TYPE_AREA accept them. The interesting arm is turning destroyability
// back on for a creature that is already dead and not in the party: it drops any orientation lock and
// queues the destroy event three seconds out. The two optional flags share one argument-count guard.
int CSWVirtualMachineCommands::ExecuteCommandSetIsDestroyable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nDestroyable;
    int nRaiseable = 1;
    int nDeadSelectable = 0;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nDestroyable))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nRaiseable))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nDeadSelectable))
            return VM_ERROR_POP_FAILED;
    }
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
        return 0;
    static_cast<CSWSObject*>(pObject)->m_bDestroyable = (nDestroyable != 0);
    if (nDestroyable != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->GetDead() != 0 && pCreature->m_bInParty == 0) {
            pCreature->SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
            if (pCreature->m_bPlayerCharacter == 0)
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 3000, pCreature->m_killerId, pCreature->m_objectId,
                    CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
    }
    CSWSObject* pSWSObject = static_cast<CSWSObject*>(pObject);
    pSWSObject->m_bRaiseable = nRaiseable;
    pSWSObject->m_bDeadSelectable = nDeadSelectable;
    return 0;
}

// FloatToString: builds a printf format string from the popped width and decimal counts and runs the
// float through it. Both counts are clamped first -- width to 0..18, decimals to 0..9 -- and the width
// picks between a two-digit and a one-digit field spec so the generated format has no stray leading
// space. Both format strings end in a newline.
int CSWVirtualMachineCommands::ExecuteCommandFloatToString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fFloat;
    int nWidth;
    int nDecimals;
    if (!g_pVirtualMachine->StackPopFloat(&fFloat))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nWidth))
        return VM_ERROR_POP_FAILED;
    if (nWidth >= 18)
        nWidth = 18;
    else if (nWidth < 0)
        nWidth = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nDecimals))
        return VM_ERROR_POP_FAILED;
    if (nDecimals >= 10)
        nDecimals = 9;
    else if (nDecimals < 0)
        nDecimals = 0;
    CExoString sFormat;
    if (nWidth >= 10)
        sFormat.Format(const_cast<char*>("%%%2d.%02df\n"), nWidth, nDecimals);
    else
        sFormat.Format(const_cast<char*>("%%%1d.%02df\n"), nWidth, nDecimals);
    CExoString sValue;
    sValue.Format(static_cast<char*>(sFormat.CStr()), fFloat);
    if (!g_pVirtualMachine->StackPushString(CExoString(sValue.CStr())))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ActionUseTalent: one body behind two command ids -- 309 aims the talent at an object, 310 at a
// location. The popped talent is unpacked into locals before the creature lookup, and a negative
// talent type short-circuits straight to the free. The location arm owns the location it pops; the
// talent itself is freed on every path that got one.
int CSWVirtualMachineCommands::ExecuteCommandActionUseTalent(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CScriptTalent* pTalent;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_TALENT, reinterpret_cast<void**>(&pTalent)))
        return VM_ERROR_POP_FAILED;
    int nTalentType = pTalent->m_nType;
    if (nTalentType >= 0) {
        int nTalentId = pTalent->m_nId;
        unsigned char nMultiClass = pTalent->m_nMultiClass;
        unsigned long long oidItem = pTalent->m_oidItem;
        int nItemProperty = pTalent->m_nItemPropertyIndex;
        unsigned char nCasterLevel = pTalent->m_nCasterLevel;
        unsigned char nMetaType = pTalent->m_nMetaType;
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
        if (pCreature != 0) {
            if (nCommand == CMD_ActionUseTalentOnObject) {
                unsigned long long oidTarget = OBJECT_INVALID;
                if (!g_pVirtualMachine->StackPopObject(&oidTarget))
                    return VM_ERROR_POP_FAILED;
                pCreature->AddUseTalentOnObjectActions(nTalentType, nTalentId, oidTarget,
                                                       nMultiClass, oidItem, nItemProperty,
                                                       nCasterLevel, nMetaType);
            } else if (nCommand == CMD_ActionUseTalentAtLocation) {
                CScriptLocation* pLocation;
                if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
                    return VM_ERROR_POP_FAILED;
                pCreature->AddUseTalentAtLocationActions(nTalentType, nTalentId,
                                                         pLocation->m_vPosition, nMultiClass,
                                                         oidItem, nItemProperty, nCasterLevel,
                                                         nMetaType);
                if (pLocation != 0)
                    delete pLocation;
            }
        }
    }
    if (pTalent != 0)
        delete pTalent;
    return 0;
}

// ActionUseSkill: queues the skill-use action on OBJECT_SELF. The two optional arguments are popped
// only when the script passed them -- the sub-skill defaults to 0 and the used item to OBJECT_INVALID.
// The target position handed to UseSkill is a default-constructed zero Vector, because this entry
// point aims by object, not by point.
int CSWVirtualMachineCommands::ExecuteCommandActionUseSkill(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nSkill;
    int nSubSkill;
    unsigned long long oidTarget;
    unsigned long long oidItem;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    nSubSkill = 0;
    if (nArgCount > 2 && !g_pVirtualMachine->StackPopInteger(&nSubSkill))
        return VM_ERROR_POP_FAILED;
    oidItem = OBJECT_INVALID;
    if (nArgCount >= 4 && !g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0
        && pObject->AsSWSCreature()->GetArea() != 0) {
        pObject->AsSWSCreature()->UseSkill(
            static_cast<unsigned char>(nSkill), static_cast<unsigned char>(nSubSkill), oidTarget, Vector(),
            static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea())->m_objectId, oidItem, 0);
    }
    return 0;
}

// ActionEquipItem: the popped slot is an index, but the equip action wants the inventory-slot bitmask,
// so the body raises 2 to that power. Indices above 19, and any non-creature or non-item pair, are
// dropped silently; only a failed pop is an error.
int CSWVirtualMachineCommands::ExecuteCommandEquipItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CGameObject* pObject;
    CGameObject* pItemObject;
    int bInstant = 0;
    unsigned long long oidItem;
    int nSlot;
    unsigned long long oidSelf = m_oidSelf;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nSlot))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3 && !g_pVirtualMachine->StackPopInteger(&bInstant))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 has 18 equipment slots and raises 2 in double
    if (static_cast<unsigned int>(nSlot) <= 17) {
        nSlot = static_cast<int>(pow(2.0, nSlot));
#else
    if (static_cast<unsigned int>(nSlot) <= 19) {
        nSlot = static_cast<int>(powf(2.0f, static_cast<float>(nSlot)));
#endif
        if (m_nScriptRunArg == 1
            && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidSelf, &pObject)
                   == CGameObjectArray::SUCCESS) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature != 0
                && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidItem,
                       &pItemObject) == CGameObjectArray::SUCCESS) {
                CSWSItem* pItem = pItemObject->AsSWSItem();
                if (pItem != 0)
                    pCreature->AddEquipItemActions(pItem, nSlot, 0, 0, bInstant, 0, 0, 0);
            }
        }
    }
    return 0;
}

// GetXP: the creature stats' earned-experience counter. Anything that is not a creature, and a failed
// array lookup, pushes 0.
int CSWVirtualMachineCommands::ExecuteCommandGetXP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nXP = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
            nXP = pCreature->m_pStats->m_nExperience;
    }
    if (!g_pVirtualMachine->StackPushInteger(nXP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetAreaOfEffectCreator: the creature that spawned an area-of-effect object. Anything else, or a
// failed lookup, pushes OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetAOEObjectCreator(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidAOE;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidAOE))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidCreator = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidAOE, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSAreaOfEffectObject() != 0)
        oidCreator = pObject->AsSWSAreaOfEffectObject()->m_oidCreator;
    if (!g_pVirtualMachine->StackPushObject(oidCreator))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetFirstAttacker (727) / GetNextAttacker (728) share this handler. The object argument is optional
// and defaults to OBJECT_SELF. Both accessor results are discarded: the pushed value is always
// OBJECT_INVALID, so both commands return OBJECT_INVALID unconditionally.
int CSWVirtualMachineCommands::ExecuteCommandGetAttacker(int nCommand, int nArgCount)
{
    unsigned long long oidCreature = m_oidSelf;
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopObject(&oidCreature))
            return VM_ERROR_POP_FAILED;
    }
    unsigned long long oidAttacker = CGameObjectArray::INVALIDOBJID;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0) {
        if (nCommand == CMD_GetFirstAttacker)
            pCreature->GetFirstAttacker();
        else if (nCommand == CMD_GetNextAttacker)
            pCreature->GetNextAttacker();
    }
    if (!g_pVirtualMachine->StackPushObject(oidAttacker))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// IsFormActive: the creature's selected stance must be the queried form and, for the seven lightsaber
// forms, a lightsaber must actually be in hand.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandIsFormActive(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nForm;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nForm))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bActive = 0;
    if (pCreature != 0 && pCreature->m_stanceId == nForm) {
        if (pCreature->m_stanceId >= 258 && pCreature->m_stanceId <= 264)
            bActive = (pCreature->IsWieldingLightsaber() != 0);
        else
            bActive = 1;
    }
    if (!g_pVirtualMachine->StackPushInteger(bActive))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// The AddAvailableNPCByObject twin that names the creature by blueprint resref instead of by object.
int CSWVirtualMachineCommands::ExecuteCommandAddAvailableNPCByTemplate(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sTemplate;
    int nNPC;
    int nResult;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (!g_pVirtualMachine->StackPopInteger(&nNPC))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    nResult = pServer->GetPartyTable()->AddNPC(nNPC, sTemplate);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetDifficultyMultiplier: difficultyopt.2da's multiplier column, indexed by the client's difficulty
// setting. The 1.0 preset survives if the 2DA read fails.
int CSWVirtualMachineCommands::ExecuteCommandGetDifficultyMultiplier(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fMultiplier = 1.0f;
    int nDifficulty = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    g_pRules->m_p2DArrays->m_pDifficultyOptions->GetFLOATEntry(nDifficulty,
                                                              CExoString("MULTIPLIER"),
                                                              &fMultiplier);
    if (!g_pVirtualMachine->StackPushFloat(fMultiplier))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetCommandable: flips the object's accepts-new-actions flag. The object defaults to OBJECT_SELF and
// is only popped when the script passed one. The flag is normalised to 0/1 and stored straight through
// the CGameObject.
int CSWVirtualMachineCommands::ExecuteCommandSetCommandable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int bCommandable;
    unsigned long long oidObject = m_oidSelf;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&bCommandable))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA)
        static_cast<CSWSObject*>(pObject)->m_bCommandable = bCommandable != 0;
    return 0;
}

// StartCreditSequence: rolls the end credits over the in-game GUI. The int operand picks the backdrop
// -- nonzero asks for the all -1 transparent colour, zero for solid black -- and the two arms are
// otherwise the same call.
int CSWVirtualMachineCommands::ExecuteCommandStartCreditSequence(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sText;
    int bTransparentBackground;
#if !KOTOR1
    K2_UNUSED(nArgCount);
    CClientExoApp* pClient;
#endif
#if KOTOR1
    // K1 takes an optional text operand and forwards to the GUI itself
    if (!g_pVirtualMachine->StackPopInteger(&bTransparentBackground))
        return VM_ERROR_POP_FAILED;
    if (nArgCount > 1 && !g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    if (bTransparentBackground == 0)
        g_pAppManager->m_pClientExoApp->StartCreditSequence(Vector(0.0f, 0.0f, 0.0f), sText);
    else
        g_pAppManager->m_pClientExoApp->StartCreditSequence(Vector(-1.0f, -1.0f, -1.0f), sText);
    return 0;
#else
    if (!g_pVirtualMachine->StackPopInteger(&bTransparentBackground))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    pClient = g_pAppManager->m_pClientExoApp;
    if (bTransparentBackground != 0)
        pClient->GetInGameGui()->StartCreditSequence(Vector(-1.0f, -1.0f, -1.0f), sText);
    else
        pClient->GetInGameGui()->StartCreditSequence(Vector(0.0f, 0.0f, 0.0f), sText);
    return 0;
#endif
}

// SetFadeIn: SetFadeOut's twin on mode 1, with one extra step -- the in-game GUI's MainLoop gate is
// cleared before the fade starts.
int CSWVirtualMachineCommands::ExecuteCommandSetFadeIn(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fLength;
    float fDelay;
    float fRed;
    float fGreen;
    float fBlue;
    if (!g_pVirtualMachine->StackPopFloat(&fLength))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fDelay))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fRed))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fGreen))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fBlue))
        return VM_ERROR_POP_FAILED;
    Vector vColor(fRed, fGreen, fBlue);
#if !KOTOR1  // K1: no SetFadeUntilScript latch
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bFadeUntilScript = 0;
#endif
    g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(1, fLength, fDelay, vColor);
    return 0;
}

// GetHitDice: the creature's character level. Both the object-type floor and the creature cast are
// checked; either failing answers 0, which is still pushed.
int CSWVirtualMachineCommands::ExecuteCommandGetHitDice(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nHitDice = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
            nHitDice = pCreature->m_pStats->GetLevel(0);
    }
    if (!g_pVirtualMachine->StackPushInteger(nHitDice))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ObjectToString: the object id printed as bare hex. The formatted string is built in one CExoString
// and then copied through its own CStr() into a second one for the push.
int CSWVirtualMachineCommands::ExecuteCommandObjectToString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CExoString sObject;
    sObject.Format(const_cast<char*>("%x"), oidObject);
    if (!g_pVirtualMachine->StackPushString(CExoString(sObject.CStr())))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetFadeOut: five floats -- the fade length, its delay, then the RGB to fade to, assembled into the
// vector the GUI's global fade takes. Mode 0 is fade out.
int CSWVirtualMachineCommands::ExecuteCommandSetFadeOut(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fLength;
    float fDelay;
    float fRed;
    float fGreen;
    float fBlue;
    if (!g_pVirtualMachine->StackPopFloat(&fLength))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fDelay))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fRed))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fGreen))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fBlue))
        return VM_ERROR_POP_FAILED;
    Vector vColor(fRed, fGreen, fBlue);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(0, fLength, fDelay, vColor);
    return 0;
}

// The AddAvailableNPCByTemplate twin for puppets.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandAddAvailablePUPByTemplate(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sTemplate;
    int nPUP;
    int nResult;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (!g_pVirtualMachine->StackPopInteger(&nPUP))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    nResult = pServer->GetPartyTable()->AddPUP(nPUP, sTemplate);
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

// AddToParty(oPC, oPartyMember): both creatures must carry the party-member flag; the new member then
// joins the PC's faction. Arguments pop in reverse, so the member id comes off first.
int CSWVirtualMachineCommands::ExecuteCommandAddToParty(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidPartyMember;
    unsigned long long oidPC;
    if (!g_pVirtualMachine->StackPopObject(&oidPartyMember))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidPC))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidPC);
    CSWSCreature* pMember =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidPartyMember);
    if (pPC != 0 && pMember != 0 && pPC->m_bPlayerCharacter != 0 && pMember->m_bPlayerCharacter != 0) {
        CSWSFaction* pFaction = pPC->GetFaction();
        if (pFaction != 0)
            pFaction->AddMember(oidPartyMember, 0);
    }
    return 0;
}

// For the script-log sink, both ports compile the write itself away and keep only the two argument
// evaluations, so the real logger is a release-empty variadic. Model it as an empty body that
// still forces the CStr() calls.
int bDisableScriptLogging;
static inline void ScriptLogEntry(const char* pszTimestamp, const char* pszText) { K2_UNUSED(pszTimestamp); K2_UNUSED(pszText);  }

// PrintLogEntry (NWScript): timestamp + the script's text, gated on the global logging switch.
int CSWVirtualMachineCommands::ExecuteCommandPrintLogEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sText;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    CExoString sTimestamp;
    g_pExoBase->m_pDebug->GetCurrentTimestamp(sTimestamp);
    if (bDisableScriptLogging == 0)
        ScriptLogEntry(sTimestamp.CStr(), sText.CStr());
    return 0;
}

// The three talent commands (spell, feat, skill) share one handler; the command id is the
// talent type (0/1/2). The range test is only there because the shared handler cannot prove it, so an
// out-of-range id leaves the type field at whatever the constructor set.
int CSWVirtualMachineCommands::ExecuteCommandTalent(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CScriptTalent* pTalent = new CScriptTalent;
    int nId;
    if (!g_pVirtualMachine->StackPopInteger(&nId)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pTalent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    unsigned int nType = nCommand - CMD_TalentSpell;
    if (nType <= 2)
        pTalent->m_nType = nType;
    pTalent->m_nId = nId;
    int bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_TALENT, pTalent);
#if KOTOR1
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    delete pTalent;
#else
    delete pTalent;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
#endif
    return 0;
}

// GetChemicalPieceValue: GetGoldPieceValue's chemical-currency twin -- the same lookup, with the cost
// divided by 20.
int CSWVirtualMachineCommands::ExecuteCommandGetChemicalPieceValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    int nValue = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidItem, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSItem() != 0)
        nValue = pObject->AsSWSItem()->GetCost() / 20;
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetGold: the credits a creature is carrying -- or, when the object is a placeable, the stack size of
// the first credits pile in its repository. The repository lookup answers an object id, so the item is
// resolved back through the server app before its stack count can be read; any missing link answers 0,
// which is still pushed.
int CSWVirtualMachineCommands::ExecuteCommandGetGold(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nGold = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature != 0) {
        nGold = pCreature->GetGold();
    } else {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidObject);
        if (pPlaceable != 0 && pPlaceable->m_pItemRepository != 0) {
            unsigned long long oidPile =
                pPlaceable->m_pItemRepository->FindItemWithBaseItemType(ITEM_TYPE_CREDITS, 0);
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidPile);
            if (pItem != 0)
                nGold = pItem->m_stackCount;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nGold))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
// GetChemical: the chemicals a creature is carrying -- or, when the object is a placeable, the stack
// size of the first pile of item type ITEM_TYPE_CREDITS in its repository (GetGold's search, so a
// placeable answers its credits). The repository lookup answers an object id, so the
// item is resolved back through the server app before its stack count can be read; any missing link
// answers 0, which is still pushed.
int CSWVirtualMachineCommands::ExecuteCommandGetChemical(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nChemical = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature != 0) {
        nChemical = pCreature->GetChemical();
    } else {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidObject);
        if (pPlaceable != 0 && pPlaceable->m_pItemRepository != 0) {
#ifdef K2_FIX_ENGINE_BUGS
            unsigned long long oidPile =
                pPlaceable->m_pItemRepository->FindItemWithBaseItemType(ITEM_TYPE_CHEMICALS, 0);
#else
            unsigned long long oidPile =
                pPlaceable->m_pItemRepository->FindItemWithBaseItemType(ITEM_TYPE_CREDITS, 0);
#endif
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidPile);
            if (pItem != 0)
                nChemical = pItem->m_stackCount;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nChemical))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif // !KOTOR1

// ActionExchangeItem, two commands on one handler: one gives the item away, anything else takes it.
// The item id is looked up only as a guard and its result discarded.
int CSWVirtualMachineCommands::ExecuteCommandActionExchangeItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (pObject != 0
        && g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem) != 0) {
        CSWSObject* pSelf = pObject->AsSWSObject();
        if (nCommand == CMD_ActionGiveItem)
            pSelf->AddGiveItemActions(oidItem, oidTarget, -1, 1);
        else
            pSelf->AddTakeItemActions(oidItem, oidTarget, 1);
    }
    return 0;
}

// Queues the open-door action on OBJECT_SELF. Three gates: the local script frame, the self object
// being in an area at all, and its commandable flag.
int CSWVirtualMachineCommands::ExecuteCommandOpenDoor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidDoor;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidDoor))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && static_cast<CSWSObject*>(pObject)->GetArea() != 0
        && pObject->AsSWSObject()->m_bCommandable == 1)
        pObject->AsSWSObject()->AddOpenDoorAction(oidDoor, 0);
    return 0;
}


// LocationAccess, two commands on one handler: one answers the location's position vector, the other
// its facing in degrees. The location structure is owned here and deleted on every path, including the
// unrecognised-id one, which reports success rather than pushing anything.
int CSWVirtualMachineCommands::ExecuteCommandLocationAccess(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CScriptLocation* pLocation;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    int bPushed;
    if (nCommand == CMD_GetPositionFromLocation)
        bPushed = g_pVirtualMachine->StackPushVector(pLocation->m_vPosition);
    else if (nCommand == CMD_GetFacingFromLocation) {
        float fFacing =
            atan2f(pLocation->m_vOrientation.y, pLocation->m_vOrientation.x) * 57.295776f;
        bPushed = g_pVirtualMachine->StackPushFloat(fFacing);
    } else
        bPushed = 1;
    delete pLocation;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetFacing: the object's facing as a compass angle in degrees, or -1.0 when the operand does not
// resolve to a positionable object. The angle comes from the raw orientation while the
// wrap-into-range test reads the normalized vector's y.
int CSWVirtualMachineCommands::ExecuteCommandGetFacing(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vOrientation;
    unsigned long long oidObject;
    CGameObject* pObject;
    float fFacing = -1.0f;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        vOrientation.x = static_cast<CSWSObject*>(pObject)->m_orientation.x;
        vOrientation.y = static_cast<CSWSObject*>(pObject)->m_orientation.y;
        vOrientation.z = 0.0f;
        Vector vNormal = normalize(vOrientation);
        fFacing = atan2f(vOrientation.y, vOrientation.x) * 57.295776f;
        if (vNormal.y < 0.0f)
            fFacing = fFacing + 360.0f;
    }
    if (!g_pVirtualMachine->StackPushFloat(fFacing))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Answers the area an object is in, or the area itself when handed one. An unused CExoString is
// constructed and destroyed around the whole thing.
int CSWVirtualMachineCommands::ExecuteCommandGetArea(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sUnused;
    unsigned long long oidObject;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidArea = CGameObjectArray::INVALIDOBJID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0) {
        if (pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
            CSWSArea* pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pObject)->GetArea());
            if (pArea != 0)
                oidArea = pArea->m_objectId;
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
            oidArea = pObject->m_objectId;
    }
    if (!g_pVirtualMachine->StackPushObject(oidArea))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetAC: the object's armour class. Only creatures have a real one; doors, placeables and items answer
// a flat 0, and everything else, including a failed lookup, answers -1. The second operand is popped
// and thrown away.
int CSWVirtualMachineCommands::ExecuteCommandGetAC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nUnused = 0;
    unsigned long long oidObject;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&nUnused))
        return VM_ERROR_POP_FAILED;
    int nArmorClass = -1;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE:
            nArmorClass = pObject->AsSWSCreature()->GetArmorClass();
            break;
        case GAME_OBJECT_TYPE_ITEM:
        case GAME_OBJECT_TYPE_PLACEABLE:
        case GAME_OBJECT_TYPE_DOOR:
            nArmorClass = 0;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nArmorClass))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetHasInventory: whether the object can hold items. Creatures and stores always can; a placeable
// answers its own flag, and an item answers whether it is a container.
int CSWVirtualMachineCommands::ExecuteCommandGetHasInventory(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int bHasInventory = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        if (pObject->AsSWSCreature() != 0 || pObject->AsSWSStore() != 0)
            bHasInventory = 1;
        else if (pObject->AsSWSPlaceable() != 0)
            bHasInventory = pObject->AsSWSPlaceable()->m_bHasInventory;
        else if (pObject->AsSWSItem() != 0)
            bHasInventory = pObject->AsSWSItem()->m_pItemRepository != 0;
    }
    if (!g_pVirtualMachine->StackPushInteger(bHasInventory))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetItemPossessor: who is carrying the item. The lookup runs twice -- an item inside another item
// reports the container, so the second hop resolves that container's own possessor. Anything that is
// not an item leaves OBJECT_INVALID in place.
int CSWVirtualMachineCommands::ExecuteCommandGetItemPossessor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidPossessor = CGameObjectArray::INVALIDOBJID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidItem, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSItem() != 0)
        oidPossessor = pObject->AsSWSItem()->m_possessorId;
    CGameObject* pPossessor = g_pAppManager->m_pServerExoApp->GetGameObject(oidPossessor);
    if (pPossessor != 0 && pPossessor->AsSWSItem() != 0)
        oidPossessor = pPossessor->AsSWSItem()->m_possessorId;
    if (!g_pVirtualMachine->StackPushObject(oidPossessor))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// AdjustReputation: shifts how the target creature's faction feels about the source's. The gate is
// inverted from what the name suggests: the adjustment is skipped when the source creature exists and
// is flagged plot, so a missing source still adjusts.
int CSWVirtualMachineCommands::ExecuteCommandAdjustReputation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget = OBJECT_INVALID;
    unsigned long long oidSource = OBJECT_INVALID;
    int nAdjustment;
    int nFaction;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidSource))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAdjustment))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pSource =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidSource);
    if (pSource == 0 || pSource->m_bPlot == 0) {
        CSWSCreature* pTarget =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
        if (pTarget != 0
            && g_pAppManager->m_pServerExoApp->GetFactionOfObject(oidSource, &nFaction) == 1)
            pTarget->AdjustReputation(nFaction, nAdjustment);
    }
    return 0;
}

// Adds a delta to one skill rank. The delta is stored narrowed to a signed char.
int CSWVirtualMachineCommands::ExecuteCommandAdjustCreatureSkills(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nSkill;
    int nAdjustment;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAdjustment))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_pStats != 0) {
            char nRank = pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(nSkill), 0, 1);
            pCreature->m_pStats->SetSkillRank(static_cast<unsigned char>(nSkill), static_cast<char>(nRank + nAdjustment));
        }
    }
    return 0;
}

// SetFakeCombatState: forces the creature in or out of the looks-like-it-is-fighting state. The
// creature cast is only a guard -- every field read and the SetCombatState receiver go through the
// CGameObject -- and entering is suppressed only when the creature is already in real combat and
// already flagged as fighting.
int CSWVirtualMachineCommands::ExecuteCommandSetFakeCombatState(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int bFakeCombat;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bFakeCombat))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = static_cast<CSWSCreature*>(pObject);
        if (bFakeCombat != 0) {
            if (pCreature->m_bCombatState == 0 || pCreature->m_nCombatType != 1)
#if KOTOR1
                pCreature->SetCombatState(1, 2);
#else
                pCreature->SetCombatState(1, 2, 1);
#endif
        } else if (pCreature->m_nCombatType == 2) {
#if KOTOR1
            pCreature->SetCombatState(0, 2);
#else
            pCreature->SetCombatState(0, 2, 0);
#endif
        }
    }
    return 0;
}

// SetListenString: installs one listen pattern on the object. The array lookup's status is checked and
// the out pointer null-checked separately, and the expression is set straight through the CGameObject.
int CSWVirtualMachineCommands::ExecuteCommandSetListenString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sPattern;
    unsigned long long oidObject;
    CGameObject* pObject;
    int nIndex;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sPattern))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0) {
        CSWSObject* pSWSObject = static_cast<CSWSObject*>(pObject);
        pSWSObject->SetListenExpression(sPattern, nIndex);
    }
    return 0;
}

// SetBonusForcePoints: stores the flat Force-point bonus and, for a player-made character whose bonus
// actually changed, tells the in-game GUI which way it moved (0xb up, 0xc down). Both pop failures
// answer -2000 rather than -2001.
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWVirtualMachineCommands::ExecuteCommandSetBonusForcePoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nBonus;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nBonus))
        return VM_ERROR_PUSH_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            if (pCreature->m_bPlayerMadeCharacter != 0) {
                int nOldBonus = pCreature->m_nBonusForcePoints;
                int nNewBonus = nBonus;
                if (nOldBonus != nNewBonus) {
                    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                    if (nOldBonus < nNewBonus)
                        pGui->UpdateStatus(CGuiInGame::GUI_STATUS_MAX_FP_GAINED, nBonus);
                    else
                        pGui->UpdateStatus(CGuiInGame::GUI_STATUS_MAX_FP_LOST, nBonus);
                }
            }
            pCreature->m_nBonusForcePoints = nBonus;
        }
    }
    return 0;
}
#endif // !KOTOR1

// GetEncounterData, four commands on one handler: active flag, max respawns, respawns used and
// (280) and difficulty. The encounter operand is optional; the invalid-object value stays in the
// argument slot when the script omits it.
int CSWVirtualMachineCommands::ExecuteCommandGetEncounterData(int nCommand, int nArgCount)
{
    unsigned long long oidEncounter = OBJECT_INVALID;
    if (nArgCount >= 1 && !g_pVirtualMachine->StackPopObject(&oidEncounter))
        return VM_ERROR_POP_FAILED;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 leaves this uninitialised for a missing encounter or unknown command
    int nValue;
#else
    int nValue = 0;
#endif
    CSWSEncounter* pEncounter =
        g_pAppManager->m_pServerExoApp->GetEncounterByGameObjectID(oidEncounter);
    if (pEncounter != 0) {
        switch (nCommand) {
        case CMD_GetEncounterActive:
            nValue = pEncounter->m_bActive;
            break;
        case CMD_GetEncounterSpawnsMax:
            nValue = pEncounter->m_nMaxRespawns;
            break;
        case CMD_GetEncounterSpawnsCurrent:
            nValue = pEncounter->m_nCurrentRespawns;
            break;
        case CMD_GetEncounterDifficulty:
            nValue = pEncounter->m_nDifficulty;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetEncounterData (four ids on one handler): the write side of GetEncounterData -- active flag (277),
// max respawns (279), respawns used (281) and difficulty (296). The two respawn counters are written
// straight into the encounter; the other two go through setters with side effects. The operand order
// mirrors the getter's: the value pops first and the encounter is the optional second operand.
// PARKED (K1): LEN -24 -- K1 keeps the second pop failure's return as its own block; ours merges the two
// (docs/PARKED.md "v2256 v7b").
int CSWVirtualMachineCommands::ExecuteCommandSetEncounterData(int nCommand, int nArgCount)
{
    int nValue;
    unsigned long long oidEncounter = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopObject(&oidEncounter))
        return VM_ERROR_POP_FAILED;
    CSWSEncounter* pEncounter =
        g_pAppManager->m_pServerExoApp->GetEncounterByGameObjectID(oidEncounter);
    if (pEncounter != 0) {
        switch (nCommand) {
        case CMD_SetEncounterActive:
            pEncounter->SetActive(nValue);
            break;
        case CMD_SetEncounterSpawnsMax:
            pEncounter->m_nMaxRespawns = nValue;
            break;
        case CMD_SetEncounterSpawnsCurrent:
            pEncounter->m_nCurrentRespawns = nValue;
            break;
        case CMD_SetEncounterDifficulty:
            pEncounter->SetDifficulty(nValue);
            break;
        }
    }
    return 0;
}

// Attack: the immediate, non-queued attack command. Same fifteen-argument action pack as ActionAttack,
// except the optional second operand -- the passive flag -- rides in as the fourth argument instead of
// a hard 0.
int CSWVirtualMachineCommands::ExecuteCommandAttack(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidTarget;
    int bPassive = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&bPassive))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
    if (pCreature != 0)
        pCreature->AddAttackActions(oidTarget, 0, 0, bPassive, 0, 0, 0, 1, SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0,
                                    4, 0);
    return 0;
}

// The ActionCastSpell family behind one handler. The command id selects whether the target is an
// object id or a popped location and how many of the optional arguments are on the stack. KOTOR has
// no metamagic, so only the placeable path forwards the popped metamagic value.
int CSWVirtualMachineCommands::ExecuteCommandCastSpell(int nCommand, int nArgCount)
{
    int nSpellId;
    int nMetaMagic K2_UNSET;
    int nDomainLevel K2_UNSET;
    int bCheat K2_UNSET;
    int nProjectilePath;
    int bInstantSpell = 0;
    int bIsLocation;
    int bFakeCast;
    int bAtObject = (nCommand == CMD_ActionCastSpellAtObject);
    int bAtLocation = (nCommand == CMD_ActionCastSpellAtLocation);
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nSpellId))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidTarget = OBJECT_INVALID;
    CScriptLocation lTarget;
    if (nCommand == CMD_ActionCastSpellAtObject || nCommand == CMD_ActionCastFakeSpellAtObject) {
        if (!g_pVirtualMachine->StackPopObject(&oidTarget))
            return VM_ERROR_POP_FAILED;
        bIsLocation = 0;
    } else {
        CScriptLocation* pLocation;
        if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
            return VM_ERROR_POP_FAILED;
        lTarget = *pLocation;
        bIsLocation = 1;
        delete pLocation;
    }
    if (nCommand == CMD_ActionCastFakeSpellAtObject || nCommand == CMD_ActionCastFakeSpellAtLocation) {
        bFakeCast = 1;
    } else {
        if (!g_pVirtualMachine->StackPopInteger(&nMetaMagic))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&bCheat))
            return VM_ERROR_POP_FAILED;
        if (nCommand == CMD_ActionCastSpellAtObject) {
            if (!g_pVirtualMachine->StackPopInteger(&nDomainLevel))
                return VM_ERROR_POP_FAILED;
        } else {
            nDomainLevel = 0;
        }
        bFakeCast = 0;
    }
    if (!g_pVirtualMachine->StackPopInteger(&nProjectilePath))
        return VM_ERROR_POP_FAILED;
    if ((bAtObject || bAtLocation) && nArgCount >= 7) {
        if (!g_pVirtualMachine->StackPopInteger(&bInstantSpell))
            return VM_ERROR_POP_FAILED;
    }
    if (nSpellId > g_pRules->m_pSpellArray->m_nCount || nSpellId < 0)
        return 0;
    switch (nProjectilePath) {
    case PROJECTILE_PATH_TYPE_DEFAULT:
        nProjectilePath = 0;
        break;
    case PROJECTILE_PATH_TYPE_HOMING:
        nProjectilePath = CSWCProjectile::PROJECTILE_TYPE_HOMING;
        break;
    case PROJECTILE_PATH_TYPE_BALLISTIC:
        nProjectilePath = CSWCProjectile::PROJECTILE_TYPE_BALLISTIC;
        break;
    case PROJECTILE_PATH_TYPE_HIGH_BALLISTIC:
        nProjectilePath = CSWCProjectile::PROJECTILE_TYPE_HIGH_BALLISTIC;
        break;
    case PROJECTILE_PATH_TYPE_ACCELERATING:
        nProjectilePath = CSWCProjectile::PROJECTILE_TYPE_ACCELERATING;
        break;
    default:
        return 0;
    }
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (m_nScriptRunArg != 1 || pObject == 0)
        return 0;
    if (nCommand == CMD_ActionCastSpellAtObject || nCommand == CMD_ActionCastFakeSpellAtObject) {
        if (g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget) == 0)
            return 0;
    }
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
    if (bFakeCast) {
        if (pCreature != 0) {
            pCreature->AddCastSpellActions(nSpellId, 0, 0, 0, 0, lTarget.m_vPosition, oidTarget,
                                           0, bIsLocation, 0, 1, static_cast<unsigned char>(nProjectilePath), bInstantSpell,
                                           -1, 0xff, 0);
            return 0;
        }
        if (pPlaceable == 0)
            return 0;
        pPlaceable->AddCastSpellActions(nSpellId, 0, lTarget.m_vPosition, oidTarget, 1,
                                        static_cast<unsigned char>(nProjectilePath));
        return 0;
    }
    if (pCreature == 0) {
        if (pPlaceable == 0 || pPlaceable->m_bCommandable == 0)
            return 0;
        pPlaceable->AddCastSpellActions(nSpellId, nMetaMagic, lTarget.m_vPosition, oidTarget, 0,
                                        static_cast<unsigned char>(nProjectilePath));
        return 0;
    }
    if (pCreature->m_bCommandable == 0)
        return 0;
    unsigned char nClassIndex = 0;
    unsigned char nCasterLevel = 0;
    if (bCheat != 0) {
        nClassIndex = CAST_CLASS_NONE;
        nCasterLevel = 0xff;
    } else {
        int bFoundClass = 0;
        for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++) {
            if (CSWClass::IsJedi(pCreature->m_pStats->GetClass(static_cast<unsigned char>(i))) == 0)
                continue;
            if (pCreature->m_pStats->GetSpellUsesLeft(nSpellId, static_cast<unsigned char>(i), 0, 0) == 0)
                continue;
            CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
            if (pSpell == 0)
                continue;
            if (pSpell->GetSpellLevel(pCreature->m_pStats->GetClass(static_cast<unsigned char>(i))) == CSWSpell::SPELL_LEVEL_NONE)
                continue;
            nClassIndex = static_cast<unsigned char>(i);
            if (nClassIndex != 2) {
                nCasterLevel = 0xff;
                bFoundClass = 1;
            }
            break;
        }
        if (bFoundClass == 0) {
            if (pCreature->m_pStats->GetIsInSpellLikeAbilityList(nSpellId) != 1)
                return 0;
            unsigned char nAbilityLevel =
                pCreature->m_pStats->GetSpellLikeAbilityCasterLevel(nSpellId);
            nCasterLevel = nAbilityLevel;
            nClassIndex = CAST_CLASS_SPELL_LIKE_ABILITY;
        }
    }
    nMetaMagic = 0;
    pCreature->AddCastSpellActions(nSpellId, nClassIndex, nDomainLevel, nMetaMagic, 0,
                                   lTarget.m_vPosition, oidTarget, 0, bIsLocation, 0, 0,
                                   static_cast<unsigned char>(nProjectilePath), bInstantSpell, -1, nCasterLevel, 0);
    return 0;
}

// Spawns one of five object kinds from a blueprint resref at a location and pushes the new object's
// id; any other type value pushes OBJECT_INVALID. Only the creature arm is location-corrected, to a
// safe spot within 20 m and then to the walkmesh height, and only the placeable arm converts the
// orientation vector into a quaternion about +Z. A successful waypoint still pushes OBJECT_INVALID:
// its result is never assigned.
int CSWVirtualMachineCommands::ExecuteCommandCreateObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sTemplate;
    int nObjectType;
    CScriptLocation* pLocation;
    int bUseAppearAnimation = 0;
    if (!g_pVirtualMachine->StackPopInteger(&nObjectType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 4 && !g_pVirtualMachine->StackPopInteger(&bUseAppearAnimation))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidNew = CGameObjectArray::INVALIDOBJID;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        CSWSArea* pArea = pModule->GetArea();
        if (pArea != 0) {
            CResRef resRef(sTemplate.CStr());
            switch (nObjectType) {
            case OBJECT_TYPE_CREATURE: {
                Vector vSpawn;
                CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);
                if (!pCreature->LoadFromTemplate(resRef, 0)) {
                    delete pCreature;
                    delete pLocation;
                    if (!g_pVirtualMachine->StackPushObject(oidNew))
                        return VM_ERROR_PUSH_FAILED;
                    return 0;
                }
                pCreature->SetOrientation(pLocation->m_vOrientation);
                vSpawn = pLocation->m_vPosition;
                if (pArea->ComputeSafeLocation(pLocation->m_vPosition, 20.0f,
                                               pCreature->m_pPathfindInfo, 1, &vSpawn))
                    pLocation->m_vPosition = vSpawn;
                vSpawn.z = pArea->AsSWSArea()->ComputeHeight(vSpawn, 1, 0, 0);
                if (bUseAppearAnimation) {
                    CGameEffect* pEffect = new CGameEffect(1);
                    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_APPEAR);
                    pEffect->SetCreator(m_oidSelf);
                    pCreature->ApplyEffect(pEffect, 0, 0);
                }
                pCreature->AddToArea(pArea, vSpawn.x, vSpawn.y, vSpawn.z, 1, 1);
                oidNew = pCreature->m_objectId;
                break;
            }
            case OBJECT_TYPE_ITEM: {
                CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
                pItem->LoadFromTemplate(resRef);
                pItem->SetPosition(pLocation->m_vPosition, 1, 1, 0);
                pItem->SetOrientation(pLocation->m_vOrientation);
                pItem->AddToArea(pArea, pLocation->m_vPosition.x, pLocation->m_vPosition.y,
                                 pLocation->m_vPosition.z, 1);
                oidNew = pItem->m_objectId;
                break;
            }
            case OBJECT_TYPE_WAYPOINT: {
                CSWSWaypoint* pWaypoint = new CSWSWaypoint(OBJECT_INVALID);
                pWaypoint->LoadFromTemplate(resRef);
                pWaypoint->SetPosition(pLocation->m_vPosition, 1, 1, 0);
                pWaypoint->SetOrientation(pLocation->m_vOrientation);
                pWaypoint->AddToArea(pArea, pLocation->m_vPosition.x, pLocation->m_vPosition.y,
                                     pLocation->m_vPosition.z, 0);
                break;
            }
            case OBJECT_TYPE_PLACEABLE: {
                CSWSPlaceable* pPlaceable = new CSWSPlaceable(OBJECT_INVALID);
                pPlaceable->LoadFromTemplate(resRef, 0);
                pPlaceable->SetPosition(pLocation->m_vPosition, 1, 1, 0);
                pPlaceable->SetOrientation(pLocation->m_vOrientation);
                Quaternion qFacing(Vector(0.0f, 0.0f, 1.0f),
                                   Yaw(pLocation->m_vOrientation) * 0.017453292f);
                pPlaceable->SetOrientation(qFacing);
                pPlaceable->AddToArea(pArea, pLocation->m_vPosition, 1);
                oidNew = pPlaceable->m_objectId;
                break;
            }
            case OBJECT_TYPE_STORE: {
                CSWSStore* pStore = new CSWSStore(OBJECT_INVALID);
                pStore->LoadFromTemplate(resRef, 0);
                pStore->SetPosition(pLocation->m_vPosition, 1, 1, 0);
                pStore->SetOrientation(pLocation->m_vOrientation);
                pStore->AddToArea(pArea, pLocation->m_vPosition.x, pLocation->m_vPosition.y,
                                  pLocation->m_vPosition.z, 1);
                oidNew = pStore->m_objectId;
                break;
            }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidNew)) {
        delete pLocation;
        pLocation = 0;
        return VM_ERROR_PUSH_FAILED;
    }
    delete pLocation;
    pLocation = 0;
    return 0;
}

// Starts a dialog between the script owner and a target, from the resref the script names or from the
// owner's own default conversation, unless the in-game GUI already owns a dialog. Both participants
// are parked on the conversation-idle animation first, and which of them speaks depends on which one
// is the human player.
int CSWVirtualMachineCommands::ExecuteCommandBeginConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CSWSCreature* pTargetCreature;
    CSWSCreature* pSelfCreature;
    unsigned long long oidPlayer;
    int bStarted = 0;
    CExoString sDialogResRef;
    unsigned long long oidTarget = OBJECT_INVALID;
    CSWSObject* pSelfObject;
    CSWSObject* pTargetObject;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    Vector vSelfFacing;
    Vector vTargetFacing;
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopString(&sDialogResRef))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidTarget))
            return VM_ERROR_POP_FAILED;
    }
    unsigned int nDialogGuiState = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState;
    int bActionConversation = g_pAppManager->m_pServerExoApp->GetActionConversation();
    g_pAppManager->m_pServerExoApp->SetActionConversation(0);
    if ((nDialogGuiState == 0 || bActionConversation != 0) && m_nScriptRunArg == 1
        && pObjectArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pSelfObject))
               == CGameObjectArray::SUCCESS) {
        if (oidTarget == OBJECT_INVALID)
            oidTarget = pSelfObject->m_oidLastSpeaker;
        if (pObjectArray->GetGameObject(oidTarget, reinterpret_cast<CGameObject**>(&pTargetObject))
                == CGameObjectArray::SUCCESS
            && pTargetObject != 0 && pSelfObject != 0) {
            CSWSPlayer* pPlayer;
            oidPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
            pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidPlayer);
            pTargetCreature = pTargetObject->AsSWSCreature();
            if (pPlayer != 0 && pTargetCreature != 0) {
                if (pTargetObject->m_objectId != pPlayer->m_gameObjectId)
                    pTargetObject->AsSWSCreature()->ClearAllActions(1);
                vTargetFacing = pTargetObject->AsSWSCreature()->m_orientation;
                CLastUpdateObject* pTargetSnapshot =
                    pPlayer->GetLastUpdateObject(pTargetObject->m_objectId);
                if (pTargetSnapshot != 0) {
                    pTargetObject->AsSWSCreature()->SetAnimation(SERVER_ANIMATION_PAUSE);
                    pTargetObject->AsSWSCreature()->ClearDirtyFlag(0);
                    pTargetSnapshot->m_nAnimation = SERVER_ANIMATION_PAUSE;
                }
            }
            pSelfCreature = pSelfObject->AsSWSCreature();
            if (pPlayer != 0 && pSelfCreature != 0) {
                if (pSelfObject->m_objectId != pPlayer->m_gameObjectId)
                    pSelfObject->AsSWSCreature()->ClearAllActions(1);
                vSelfFacing = pSelfObject->AsSWSCreature()->m_orientation;
                CLastUpdateObject* pSelfSnapshot =
                    pPlayer->GetLastUpdateObject(pSelfObject->m_objectId);
                if (pSelfSnapshot != 0) {
                    pSelfObject->AsSWSCreature()->SetAnimation(SERVER_ANIMATION_PAUSE);
                    pSelfObject->AsSWSCreature()->ClearDirtyFlag(0);
                    pSelfSnapshot->m_nAnimation = SERVER_ANIMATION_PAUSE;
                }
            }
            if (sDialogResRef.IsEmpty())
                sDialogResRef = pSelfObject->m_sLastConversation;
            int nDialogFlag = pSelfObject->m_bPrivateConversation;
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pTargetObject->m_objectId);
#ifdef K2_FIX_ENGINE_BUGS
            if (pPlayer != 0 && oidTarget == pPlayer->m_gameObjectId) {
#else
            if (oidTarget == pPlayer->m_gameObjectId) {
#endif
                if (pSelfObject->StartDialog(oidTarget, sDialogResRef, nDialogFlag)) {
                    bStarted = 1;
                } else {
                    if (pTargetObject->AsSWSCreature() != 0) {
                        pTargetObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pTargetObject->AsSWSCreature()->SetOrientation(vTargetFacing);
                    }
                    if (pSelfObject->AsSWSCreature() != 0) {
                        pSelfObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pSelfObject->AsSWSCreature()->SetOrientation(vSelfFacing);
                    }
                }
#ifdef K2_FIX_ENGINE_BUGS
            } else if (pPlayer != 0 && m_oidSelf == pPlayer->m_gameObjectId) {
#else
            } else if (m_oidSelf == pPlayer->m_gameObjectId) {
#endif
                if (pTargetObject->StartDialog(m_oidSelf, sDialogResRef, nDialogFlag)) {
                    bStarted = 1;
                } else {
                    if (pTargetObject->AsSWSCreature() != 0) {
                        pTargetObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pTargetObject->AsSWSCreature()->SetOrientation(vTargetFacing);
                    }
                    if (pSelfObject->AsSWSCreature() != 0) {
                        pSelfObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pSelfObject->AsSWSCreature()->SetOrientation(vSelfFacing);
                    }
                }
            } else if (pSelfObject->AsSWSCreature() != 0
                       && pSelfObject->AsSWSCreature()->m_bInParty != 0) {
                if (pTargetObject->StartDialog(m_oidSelf, sDialogResRef, nDialogFlag)) {
                    bStarted = 1;
                } else {
                    if (pTargetObject->AsSWSCreature() != 0) {
                        pTargetObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pTargetObject->AsSWSCreature()->SetOrientation(vTargetFacing);
                    }
                    if (pSelfObject->AsSWSCreature() != 0) {
                        pSelfObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pSelfObject->AsSWSCreature()->SetOrientation(vSelfFacing);
                    }
                }
            } else {
                if (pSelfObject->StartDialog(oidTarget, sDialogResRef, nDialogFlag)) {
                    bStarted = 1;
                } else {
                    if (pTargetObject->AsSWSCreature() != 0) {
                        pTargetObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pTargetObject->AsSWSCreature()->SetOrientation(vTargetFacing);
                    }
                    if (pSelfObject->AsSWSCreature() != 0) {
                        pSelfObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                        pSelfObject->AsSWSCreature()->SetOrientation(vSelfFacing);
                    }
                }
            }
        }
    }
    if (bStarted == 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
    if (!g_pVirtualMachine->StackPushInteger(bStarted))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Queues the "talk to X" action on the script owner, after tearing the party out of its current
// actions and resurrecting every member at or below 0 hit points so the conversation can run. The six
// ignore resrefs share one string, each popped over the previous value, and the last two arguments are
// popped and discarded.
int CSWVirtualMachineCommands::ExecuteCommandActionStartConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sDialogResRef("");
    unsigned long long oidTarget;
    int bPrivateConversation = 0;
    int bAlwaysOne = 1;
    int nConversationType = 1;
    int bIgnoreStartRange = 0;
    CSWSObject* pObject;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_PUSH_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopString(&sDialogResRef))
        return VM_ERROR_PUSH_FAILED;
    if (nArgCount >= 3 && !g_pVirtualMachine->StackPopInteger(&bPrivateConversation))
        return VM_ERROR_PUSH_FAILED;
    if (nArgCount >= 4 && !g_pVirtualMachine->StackPopInteger(&nConversationType))
        return VM_ERROR_PUSH_FAILED;
    if (nArgCount >= 5 && !g_pVirtualMachine->StackPopInteger(&bIgnoreStartRange))
        return VM_ERROR_PUSH_FAILED;
    bIgnoreStartRange = 1;
    CExoString sObjectToIgnore;
    if (nArgCount >= 6) {
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
        if (!g_pVirtualMachine->StackPopString(&sObjectToIgnore))
            return VM_ERROR_PUSH_FAILED;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AddToDialogNonDependantList(sObjectToIgnore);
    }
    int bUseLeader = 0;
    if (nArgCount > 11 && !g_pVirtualMachine->StackPopInteger(&bUseLeader))
        return VM_ERROR_PUSH_FAILED;
#if KOTOR1
    // K1 has no bark-position or don't-clear arguments, no dead-party check and no resurrection pass;
    // the owner is only raised to AI level 1 when it sits at level 0.
    if (bUseLeader != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bConversationUseLeader = 1;
    else
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bConversationUseLeader = 0;
    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pLeader != 0) {
        pLeader->ClearAllActions();
        unsigned long long oidLeader =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId);
        CGameObject* pLeaderObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidLeader);
        if (pLeaderObject != 0 && pLeaderObject->AsSWSCreature() != 0)
            pLeaderObject->AsSWSCreature()->ClearAllActions(1);
    }
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return 0;
    if (m_nScriptRunArg != 1)
        return 0;
    if (pObjectArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject->m_bCommandable == 0)
        return 0;
    if (pObject->m_aiLevel == 0)
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pObject, 1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(1);
    unsigned long long oidInvalid = OBJECT_INVALID;
    pObject->AddAction(AIACTION_DIALOG_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_STRING, &sDialogResRef,
                                      ACTION_PARAM_INT, &bPrivateConversation, ACTION_PARAM_INT, &bAlwaysOne,
                                      ACTION_PARAM_INT, &bIgnoreStartRange, ACTION_PARAM_OBJECT, &oidInvalid);
    return 0;
#else
    int nBarkX = -1;
    int nBarkY = -1;
    if (nArgCount > 13) {
        if (!g_pVirtualMachine->StackPopInteger(&nBarkX))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nBarkY))
            return VM_ERROR_POP_FAILED;
    }
    int bDontClearActions = 0;
    if (nArgCount >= 15 && !g_pVirtualMachine->StackPopInteger(&bDontClearActions))
        return VM_ERROR_POP_FAILED;
    CSWGuiBarkBubble* pBarkBubble =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pBarkBubble;
    if (pBarkBubble != 0) {
        pBarkBubble->m_nOverrideX = nBarkX;
        pBarkBubble->m_nOverrideY = nBarkY;
    }
    if (bUseLeader != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bConversationUseLeader = 1;
    else
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bConversationUseLeader = 0;
    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pLeader != 0) {
        pLeader->ClearAllActions();
        unsigned long long oidLeader =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId);
        CGameObject* pLeaderObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidLeader);
        if (pLeaderObject != 0 && pLeaderObject->AsSWSCreature() != 0)
            pLeaderObject->AsSWSCreature()->ClearAllActions(1);
    }
    // An empty leader slot means there is no party to check, which counts as living.
    int bNoLivingParty = 1;
    for (int i = 0; i < 3; i++) {
        CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
        if (pMember != 0) {
            CSWSCreature* pServer = pMember->GetServerCreature();
            if (pServer != 0 && pServer->GetCurrentHitPoints(0) > 0) {
                bNoLivingParty = 0;
                break;
            }
        } else if (i == 0) {
            bNoLivingParty = 0;
            break;
        }
    }
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState | bNoLivingParty) != 0)
        return 0;
    if (m_nScriptRunArg != 1)
        return 0;
    if (pObjectArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject->m_bCommandable == 0)
        return 0;
    for (int i = 0; i < 3; i++) {
        CSWCCreature* pDeadMember =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
        if (pDeadMember == 0)
            continue;
        CSWSCreature* pServerDead = pDeadMember->GetServerCreature();
        if (pServerDead == 0)
            continue;
        if (pServerDead->GetCurrentHitPoints(0) > 0)
            continue;
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
        pEffect->SetInteger(0, 2);
        CGameObject* pSelf = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
        if (pSelf != 0)
            pEffect->SetCreator(m_oidSelf);
        pEffect->m_durationTypeBits = pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK;
        pServerDead->AsSWSObject()->ApplyEffect(pEffect, 0, 1);
    }
    unsigned char nAILevel = static_cast<unsigned char>(pObject->m_aiLevel);
    if (nAILevel < 3) {
        pObject->m_nSavedAILevel = nAILevel;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pObject, 3);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(1);
    unsigned long long oidInvalid = OBJECT_INVALID;
    if (bDontClearActions == 0)
        pObject->ClearAllActions(1);
    pObject->AddAction(AIACTION_DIALOG_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_STRING, &sDialogResRef,
                                      ACTION_PARAM_INT, &bPrivateConversation, ACTION_PARAM_INT, &bAlwaysOne,
                                      ACTION_PARAM_INT, &bIgnoreStartRange, ACTION_PARAM_OBJECT, &oidInvalid);
    return 0;
#endif
}
// GetItemACValue: the item's own armour class plus every type-1 (AC bonus) passive property's
// cost-table value. The property count is re-read from the item on every iteration.
int CSWVirtualMachineCommands::ExecuteCommandGetItemACValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    int nArmorClass = 0;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0) {
        nArmorClass = pItem->m_nBaseArmorClass;
        for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
            CSWItemProperty* pProperty = pItem->GetPassiveProperty(i);
            if (pProperty != 0 && pProperty->m_type == ITEM_PROPERTY_AC_BONUS)
                nArmorClass += pProperty->m_costTableValue;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nArmorClass))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetLastAttackType: a two-value translation of the creature's last-attack code -- 11 answers 9 and 30
// answers 10, everything else answers 0. A creature that is not in combat also answers 0.
int CSWVirtualMachineCommands::ExecuteCommandGetLastAttackType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nAttackType = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->m_bCombatState != 0)
            nAttackType = pCreature->m_nLastAttackFeatId == 11 ? 9 : (pCreature->m_nLastAttackFeatId == 30 ? 10 : 0);
    }
    if (!g_pVirtualMachine->StackPushInteger(nAttackType))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetSpellTargetObject: the object OBJECT_SELF is currently casting at. Same shape as
// GetSpellForcePointCost -- the local-script-frame gate, then the object cast followed by the
// creature-or-placeable test -- but the answer defaults to OBJECT_INVALID rather than 0.
int CSWVirtualMachineCommands::ExecuteCommandGetSpellTargetObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget = CGameObjectArray::INVALIDOBJID;
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                == CGameObjectArray::SUCCESS) {
            CSWSObject* pSelf = pObject->AsSWSObject();
            if (pSelf != 0
                && (pSelf->AsSWSCreature() != 0 || pSelf->AsSWSPlaceable() != 0))
                oidTarget = pSelf->m_castTargetId;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidTarget))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// UnlockAllMusic: a cheat command that flips every planet song on at once by writing the all-songs
// mask straight into swkotor2.ini. It never touches the running game state, so the unlock only takes
// effect the next time the ini is read.
int CSWVirtualMachineCommands::ExecuteCommandUnlockAllMusic(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoIni ini;
    ini.WriteIniEntry(CExoString(UNLOCKED_PLANET_SONGS_ALL), ".\\swkotor2.ini", "Game Options",
                      "UnlockedPlanetSongs");
    return 0;
}

// MusicBattlePlay/Stop/ChangeTrack share this handler: one starts the area's battle music, one
// stops it and one selects a track. Only the track id is an operand and it is optional.
int CSWVirtualMachineCommands::ExecuteCommandMusicBattle(int nCommand, int nArgCount)
{
    unsigned long long oidArea;
    int nTrack = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopInteger(&nTrack))
        return VM_ERROR_POP_FAILED;
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea != 0) {
        if (nCommand == CMD_MusicBattlePlay)
            pArea->m_pAmbientSound->PlayBattleMusic(1);
        else if (nCommand == CMD_MusicBattleStop)
            pArea->m_pAmbientSound->PlayBattleMusic(0);
        else if (nCommand == CMD_MusicBattleChange)
            pArea->m_pAmbientSound->SetBattleMusicTrack(nTrack);
    }
    return 0;
}

// GetHasSkill: whether the creature may use the given skill at all. The skill id is popped first,
// before the object, and the tri-state can-use result is narrowed to == 1.
int CSWVirtualMachineCommands::ExecuteCommandGetHasSkill(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nSkill;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nSkill))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bHasSkill = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        bHasSkill = pObject->AsSWSCreature()->m_pStats->GetCanUseSkill(static_cast<unsigned char>(nSkill)) == 1;
    if (!g_pVirtualMachine->StackPushInteger(bHasSkill))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// EffectForceShield: the Effect* skeleton with two extras -- the shield id operand is pre-seeded to 0
// before the pop, and the effect is stamped with the running script's own object as its creator before
// the subtype is written.
int CSWVirtualMachineCommands::ExecuteCommandEffectForceShield(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nShield = 0;
    if (g_pVirtualMachine->StackPopInteger(&nShield) == 0)
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->SetCreator(m_oidSelf);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCESHIELD);
    pEffect->SetInteger(0, nShield);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// SetCreatureAILevel: re-buckets a creature in the AI master. The requested level is only honoured
// below 5 -- anything else becomes 3 -- and the creature's own stored level, which ResetCreatureAILevel
// restores, is refreshed from its current AI level first.
int CSWVirtualMachineCommands::ExecuteCommandSetCreatureAILevel(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAILevel;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAILevel))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            pCreature->m_nSavedAILevel = static_cast<unsigned char>(pCreature->m_aiLevel);
            int nLevel = (nAILevel >= 0 && nAILevel < 5) ? nAILevel : 3;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(pCreature, nLevel);
        }
    }
    return 0;
}

// SetGlobalLocation: the GetGlobalLocation writer. The location arrives as a heap-allocated structure
// this command owns -- it is deleted and the slot cleared before returning. The name-pop failure answers
// -2000 while the structure-pop failure answers -2001.
int CSWVirtualMachineCommands::ExecuteCommandSetGlobalLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptLocation* pLocation;
    CExoString sName;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (pLocation != 0) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueLocation(sName, *pLocation);
        delete pLocation;
        pLocation = 0;
    }
    return 0;
}

// GetGoldPieceValue: the item's full, unscaled cost. Not an item answers 0.
int CSWVirtualMachineCommands::ExecuteCommandGetGoldPieceValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    int nValue = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidItem, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSItem() != 0)
        nValue = pObject->AsSWSItem()->GetCost();
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetGlobalString: the SetGlobalString reader. The pop failure returns -2000 rather than -2001, the
// same transposition as its writer.
int CSWVirtualMachineCommands::ExecuteCommandGetGlobalString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    CExoString sValue;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueString(sName, sValue);
    if (!g_pVirtualMachine->StackPushString(sValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// FindSubString: index of the substring inside the string, -1 when absent. The search always starts at
// 0; the script has no start-offset operand.
int CSWVirtualMachineCommands::ExecuteCommandFindSubString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sString;
    CExoString sSubString;
    int nIndex;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sSubString))
        return VM_ERROR_POP_FAILED;
    nIndex = sString.Find(sSubString, 0);
    if (!g_pVirtualMachine->StackPushInteger(nIndex))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetGlobalString: writes one of the module's named global strings. Both pop failures return -2000
// rather than -2001.
int CSWVirtualMachineCommands::ExecuteCommandSetGlobalString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    CExoString sValue;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sValue))
        return VM_ERROR_PUSH_FAILED;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueString(sName, sValue);
    return 0;
}

// SetNPCAIStyle: the creature stats' AI-state id, defaulting to 0 when the pop is skipped. The store
// truncates it to 16 bits.
int CSWVirtualMachineCommands::ExecuteCommandSetNPCAIStyle(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStyle = 0;
    unsigned long long oidCreature;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nStyle))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        pStats->m_aiState = static_cast<unsigned short>(nStyle);
    }
    return 0;
}

// GetCreatureHasTalent: does this creature own the given talent. The creature is the optional second
// argument, defaulted to OBJECT_SELF. The talent structure is deleted only on the success path, so a
// failed push leaks it.
int CSWVirtualMachineCommands::ExecuteCommandGetCreatureHasTalent(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidCreature = OBJECT_INVALID;
    CScriptTalent* pTalent;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_TALENT, reinterpret_cast<void**>(&pTalent)))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2 && !g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    int bHasTalent = 0;
    if (pCreature != 0)
        bHasTalent = pCreature->m_pStats->GetCreatureHasTalent(pTalent->m_nType, pTalent->m_nId);
    if (!g_pVirtualMachine->StackPushInteger(bHasTalent))
        return VM_ERROR_PUSH_FAILED;
    delete pTalent;
    return 0;
}

// SetGlobalNumber: the shared writer for SetGlobalBoolean (579) and SetGlobalNumber (581). The boolean
// arm normalizes the value to 0/1, the number arm truncates it to the table's byte. Both pop failures
// answer -2000 rather than -2001.
int CSWVirtualMachineCommands::ExecuteCommandSetGlobalNumber(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sName;
    int nValue;
    int bValue;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_PUSH_FAILED;
    bValue = nValue;
    if (nCommand == CMD_SetGlobalBoolean)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueBoolean(sName,
                                                                                 bValue != 0);
    else if (nCommand == CMD_SetGlobalNumber)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sName, static_cast<char>(nValue));
    return 0;
}

// ChangeItemCost: rewrites the per-base-item price multiplier in baseitems.2da from the given item
// template. Two engine bugs in six lines: the probe item is allocated and never deleted, and the write
// lands on the shared base-item row, so it changes the price of every item of that base type for the
// rest of the session.
int CSWVirtualMachineCommands::ExecuteCommandChangeItemCost(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fCostMultiplier = 1.0f;
    CExoString sTemplate;
    CSWBaseItem* pBaseItem;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fCostMultiplier))
        return VM_ERROR_POP_FAILED;
    CResRef resTemplate(sTemplate.CStr());
    CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
    if (pItem->LoadFromTemplate(resTemplate) != 0) {
        pBaseItem = pItem->GetBaseItem();
        pBaseItem->m_costMultiplier = fCostMultiplier;
    }
    return 0;
}

// GetLastAttackMode: the creature's last combat mode, translated from the internal attack-mode byte to
// the script's COMBAT_MODE_* numbering through a five-entry lookup table (internal 4, and anything
// outside 1..6, answer 0). A creature that is not in combat, a non-creature and a missing object all
// answer OBJECT_SELF's bit pattern pushed as an integer, which no script constant matches.
int CSWVirtualMachineCommands::ExecuteCommandGetLastAttackMode(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nMode = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->m_bCombatState != 0) {
            switch (pCreature->m_nLastAttackMode) {
            case CSWSCreature::CREATURE_COMBAT_MODE_PARRY:
                nMode = COMBAT_MODE_PARRY;
                break;
            case CSWSCreature::CREATURE_COMBAT_MODE_POWER_ATTACK:
                nMode = COMBAT_MODE_POWER_ATTACK;
                break;
            case CSWSCreature::CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK:
                nMode = COMBAT_MODE_IMPROVED_POWER_ATTACK;
                break;
            case CSWSCreature::CREATURE_COMBAT_MODE_FLURRY:
                nMode = COMBAT_MODE_FLURRY_OF_BLOWS;
                break;
            case CSWSCreature::CREATURE_COMBAT_MODE_RAPID_SHOT:
                nMode = COMBAT_MODE_RAPID_SHOT;
                break;
            default:
                nMode = COMBAT_MODE_INVALID;
                break;
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nMode))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetStringByStrRef: the talk-table lookup. Always the male row -- the gender argument nwscript exposes
// is never popped, so the female table is unreachable from script. The fetched record's text is copied
// into a second string for the push.
int CSWVirtualMachineCommands::ExecuteCommandGetStrByStrRef(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStrRef;
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    CExoString sText(strRes.text);
    if (!g_pVirtualMachine->StackPushString(sText))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetUserDefinedEventNumber: which OnUserDefined event the running object is handling. Three object
// types keep the number on their own class (module, area, encounter); everything else uses the shared
// object field, which is why the default arm is that cast rather than an error. Not a script-visible
// object answers -1.
int CSWVirtualMachineCommands::ExecuteCommandGetUserDefinedEventNumber(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nEvent = -1;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
        == CGameObjectArray::SUCCESS) {
        switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_MODULE:
            nEvent = pObject->AsSWSModule()->m_nUserDefinedEventNumber;
            break;
        case GAME_OBJECT_TYPE_AREA:
            nEvent = pObject->AsSWSArea()->m_nUserDefinedEventNumber;
            break;
        case GAME_OBJECT_TYPE_ENCOUNTER:
            nEvent = pObject->AsSWSEncounter()->m_nUserDefinedEventNumber;
            break;
        default:
            nEvent = static_cast<CSWSObject*>(pObject)->m_nUserDefinedEventNumber;
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nEvent))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetStringLeft: the leading nCount characters. The result is assigned into a second string rather than
// pushed straight from the return value.
int CSWVirtualMachineCommands::ExecuteCommandGetStringLeft(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sString;
    CExoString sResult;
    int nCount;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nCount))
        return VM_ERROR_POP_FAILED;
    sResult = sString.Left(nCount);
    if (!g_pVirtualMachine->StackPushString(sResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetStringRight: GetStringLeft's mirror, with the same two-string shape.
int CSWVirtualMachineCommands::ExecuteCommandGetStringRight(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sString;
    CExoString sResult;
    int nCount;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nCount))
        return VM_ERROR_POP_FAILED;
    sResult = sString.Right(nCount);
    if (!g_pVirtualMachine->StackPushString(sResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// RoundsToSeconds (121) / HoursToSeconds (122) / TurnsToSeconds (123), one handler. A round is 3 s and a
// turn is 10 rounds, both hard-coded; only the hour arm is data-driven, scaling by the module's
// minutes-per-hour.
int CSWVirtualMachineCommands::ExecuteCommandTimeConversions(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    int nValue;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    float fSeconds = 0.0f;
    if (nCommand == CMD_RoundsToSeconds)
        fSeconds = static_cast<float>(nValue) * 3.0f;
    else if (nCommand == CMD_HoursToSeconds)
        fSeconds = static_cast<float>(nValue) * 60.0f
                   * static_cast<float>(g_pAppManager->m_pServerExoApp->GetWorldTimer()->m_nMinutesPerHour);
    else if (nCommand == CMD_TurnsToSeconds)
        fSeconds = static_cast<float>(nValue) * 3.0f * 10.0f;
    if (!g_pVirtualMachine->StackPushFloat(fSeconds))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ActionResumeConversation: queues the pick-the-conversation-back-up action on OBJECT_SELF. Takes no
// arguments; the only gates are that this is a real action script and that the object is still
// commandable.
int CSWVirtualMachineCommands::ExecuteCommandActionResumeConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && static_cast<CSWSObject*>(pObject)->m_bCommandable != 0)
        static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_RESUME_DIALOG, ACTION_GROUP_NEW);
    return 0;
}

// MagicalEffect (112) / SupernaturalEffect (113) / ExtraordinaryEffect (114): rewrite an effect's
// dispel-category bits in place and hand the same structure back to the script. A linked effect has to
// re-push the new subtype into both halves. The pop failure answers -2000 rather than -2001, and the
// returned structure is deleted even though the script now owns the pushed copy, with no destructor
// call -- so the pointer is freed through a type without one.
int CSWVirtualMachineCommands::ExecuteCommandSetSubType(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameEffect* pEffect;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_PUSH_FAILED;
    if (nCommand == CMD_ExtraordinaryEffect)
        pEffect->m_durationTypeBits = pEffect->m_durationTypeBits | SUBTYPE_EXTRAORDINARY;
    else if (nCommand == CMD_SupernaturalEffect)
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~CGameEffect::SUBTYPE_MASK) | SUBTYPE_SUPERNATURAL;
    else if (nCommand == CMD_MagicalEffect)
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LINK)
        pEffect->UpdateLinked();
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete reinterpret_cast<char*>(pEffect);
    return 0;
}

// GetIsRacialTypePlayable: whether the object's race is one a player character may take. Anything that
// is not a creature, and a missing object, falls back to the racialtypes.2da row count as the index,
// i.e. one past the last row. That fallback is read twice from g_pRules.
int CSWVirtualMachineCommands::ExecuteCommandGetIsRacialTypePlayable(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    CGameObject* pObject;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 leaves this uninitialised when the lookup fails
    unsigned short nRace;
#else
    unsigned short nRace = g_pRules->m_nNumRaces;
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
        == CGameObjectArray::SUCCESS) {
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE)
            nRace = pObject->AsSWSCreature()->m_pStats->m_nRace;
        else
            nRace = g_pRules->m_nNumRaces;
    }
    if (!g_pVirtualMachine->StackPushInteger(g_pRules->m_paRaces[nRace].m_bPlayerRace != 0))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Whether the object is trapped ("mined"): the three object types that can carry a trap each keep the
// flag on their own class. Any other type, and a missing object, answer 0.
int CSWVirtualMachineCommands::ExecuteCommandGetIsMined(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int bTrapped = 0;
    if (pObject != 0) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
            bTrapped = pObject->AsSWSTrigger()->m_bTrap;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            bTrapped = pObject->AsSWSPlaceable()->m_bTrapFlag;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
            bTrapped = pObject->AsSWSDoor()->m_bTrapFlag;
    }
    if (!g_pVirtualMachine->StackPushInteger(bTrapped))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}


// GetItemInSlot: the item equipped in an inventory slot. The script passes a slot index and the
// inventory wants the slot mask, and the conversion is written in floating point. Anything missing --
// not a creature, no inventory, empty slot -- answers OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetItemInSlot(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nSlot;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nSlot))
        return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 raises 2 in double (ldexp(1.0, n))
    nSlot = static_cast<int>(pow(2.0, nSlot));
#else
    nSlot = static_cast<int>(powf(2.0f, static_cast<float>(nSlot)));
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidItem = CGameObjectArray::INVALIDOBJID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
        == CGameObjectArray::SUCCESS) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_pInventory != 0) {
            CSWItem* pItem = pCreature->m_pInventory->GetItemInSlot(nSlot);
            if (pItem != 0)
                oidItem = static_cast<CSWSItem*>(pItem)->m_objectId;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// RemoveNPCFromPartyToBase: RemovePartyMember below plus the send-them-home step -- the party-table copy
// of the NPC is destroyed, and ITS answer, not RemoveMember's, is what the script gets back.
int CSWVirtualMachineCommands::ExecuteCommandRemoveNPCFromPartyToBase(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNpc;
    if (!g_pVirtualMachine->StackPopInteger(&nNpc))
        return VM_ERROR_POP_FAILED;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pPartyTable = pServer->GetPartyTable();
    unsigned long long oidCreature = pPartyTable->GetNPCObject(nNpc, 0, 1);
    if (pPartyTable->RemoveMember(nNpc) != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pCCreature =
            pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidCreature));
        CSWSCreature* pSCreature = pServer->GetCreatureByGameObjectID(oidCreature);
        if (pCCreature != 0)
            pCCreature->SetInParty(0);
        if (pSCreature != 0)
            pSCreature->SetInParty(0, 1);
    }
    int bKilled = pPartyTable->KillNPCObject(nNpc, 1);
    if (!g_pVirtualMachine->StackPushInteger(bKilled))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// RemovePartyMember: the undo of AddPartyMember. The creature id has to be resolved from the party table
// before the slot is released, since afterwards the slot no longer names it; both the client and server
// copies get their in-party flag cleared.
int CSWVirtualMachineCommands::ExecuteCommandRemovePartyMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNpc;
    if (!g_pVirtualMachine->StackPopInteger(&nNpc))
        return VM_ERROR_POP_FAILED;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pPartyTable = pServer->GetPartyTable();
    unsigned long long oidCreature = pPartyTable->GetNPCObject(nNpc, 0, 1);
    int bRemoved = pPartyTable->RemoveMember(nNpc);
    if (bRemoved != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pCCreature =
            pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidCreature));
        CSWSCreature* pSCreature = pServer->GetCreatureByGameObjectID(oidCreature);
        if (pCCreature != 0)
            pCCreature->SetInParty(0);
        if (pSCreature != 0)
            pSCreature->SetInParty(0, 1);
    }
    if (!g_pVirtualMachine->StackPushInteger(bRemoved))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// AddPartyMember: the party-slot twin of AddPartyPuppet, with the in-party flag set on both sides
// rather than the puppet marks.
int CSWVirtualMachineCommands::ExecuteCommandAddPartyMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNpc;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopInteger(&nNpc))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    int bAdded = pServer->GetPartyTable()->AddMember(nNpc, oidCreature);
    if (bAdded != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pCCreature =
            pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidCreature));
        CSWSCreature* pSCreature = pServer->GetCreatureByGameObjectID(oidCreature);
        if (pCCreature != 0)
            pCCreature->SetInParty(1);
        if (pSCreature != 0)
            pSCreature->SetInParty(1, 1);
    }
    if (!g_pVirtualMachine->StackPushInteger(bAdded))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// AddPartyPuppet: binds a creature into a puppet slot of the party table, then marks the same creature
// on both sides of the client/server split -- the client copy is looked up through ServerToClientObjectId
// first. Either side may not have the object yet; the pushed answer is the party table's own.
#if !KOTOR1
int CSWVirtualMachineCommands::ExecuteCommandAddPartyPuppet(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSlot;
    unsigned long long oidCreature;
    if (!g_pVirtualMachine->StackPopInteger(&nSlot))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    int bAdded = pServer->GetPartyTable()->AddPuppet(nSlot, oidCreature);
    if (bAdded != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pCCreature =
            pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidCreature));
        CSWSCreature* pSCreature = pServer->GetCreatureByGameObjectID(oidCreature);
        if (pCCreature != 0)
            pCCreature->m_bIsPuppet = 1;
        if (pSCreature != 0)
            pSCreature->SetIsPuppet(1);
    }
    if (!g_pVirtualMachine->StackPushInteger(bAdded))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}
#endif

// GiveXPToCreature: awards experience. The creature operand only decides whether the XP ticker animates
// (and only for a party member) -- the award itself always goes through the party table's distribute
// call, so handing this a non-party creature still pays the party. Non-positive amounts are dropped
// before anything is looked up.
int CSWVirtualMachineCommands::ExecuteCommandGiveXPToCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidCreature;
    int nXP;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nXP))
        return VM_ERROR_POP_FAILED;
    // The ticker gets a copy of the amount while the payout re-reads the popped slot.
    int nAmount = nXP;
    if (nAmount < 1)
        return 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            if (pCreature->m_bInParty != 0)
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, nAmount);
            if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
                g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nXP, 1);
        }
    }
    return 0;
}

// GiveGoldToCreature: hands credits to a creature, refusing non-positive amounts outright. The status
// bar is refreshed only when the recipient is in the party, and the value it is handed is the amount
// given, not the new total.
int CSWVirtualMachineCommands::ExecuteCommandGiveGoldToCreature(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidCreature;
    int nGold;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nGold))
        return VM_ERROR_POP_FAILED;
    if (nGold >= 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0) {
            pCreature->AddGold(nGold, 1);
            if (pCreature->m_bInParty != 0)
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_CREDITS, nGold);
        }
    }
    return 0;
}


// AngleToVector (144) / VectorToAngle (145), one handler. The angle is in degrees on both sides:
// AngleToVector converts to radians before the sin/cos pair, and VectorToAngle subtracts the arc-cosine
// from 360 for the lower half-plane rather than negating it.
int CSWVirtualMachineCommands::ExecuteCommandVectorConversions(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    if (nCommand == CMD_VectorToAngle) {
        Vector vDirection(0.0f, 0.0f, 0.0f);
        if (!g_pVirtualMachine->StackPopVector(&vDirection))
            return VM_ERROR_POP_FAILED;
        vDirection = normalize(vDirection);
        float fAngle = acosf(vDirection.x);
        if (vDirection.y < 0.0f)
            fAngle = 360.0f - fAngle;
        if (!g_pVirtualMachine->StackPushFloat(fAngle))
            return VM_ERROR_PUSH_FAILED;
    } else if (nCommand == CMD_AngleToVector) {
        float fAngle;
        if (!g_pVirtualMachine->StackPopFloat(&fAngle))
            return VM_ERROR_POP_FAILED;
        float fRadians = fAngle * 0.017453292f;
        Vector vDirection(cosf(fRadians), sinf(fRadians), 0.0f);
        if (!g_pVirtualMachine->StackPushVector(vDirection))
            return VM_ERROR_PUSH_FAILED;
    }
    return 0;
}

// ApplyEffectAtLocation: stamps the popped effect's duration type (0 instantaneous, 1 temporary with the
// popped duration, anything else permanent) and hands it to the player's area -- not the script owner's --
// at the popped location. The two failure paths free the half-popped structures with a bare operator
// delete and no destructor, while the success path uses a real delete. If there is no player area the
// effect is destroyed properly and the command still succeeds.
int CSWVirtualMachineCommands::ExecuteCommandApplyEffectAtPoint(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nDurationType;
    void* pEffect;
    void* pLocation;
    float fDuration;
    if (!g_pVirtualMachine->StackPopInteger(&nDurationType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, static_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, static_cast<void**>(&pLocation))) {
        if (pEffect != 0) {
            operator delete(pEffect);
            pEffect = 0;
        }
        return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopFloat(&fDuration)) {
        if (pLocation != 0) {
            operator delete(pLocation);
            pLocation = 0;
        }
        if (pEffect != 0) {
            operator delete(pEffect);
            pEffect = 0;
        }
        return VM_ERROR_POP_FAILED;
    }
    CScriptLocation* pScriptLocation = static_cast<CScriptLocation*>(pLocation);
    CGameEffect* pGameEffect = static_cast<CGameEffect*>(pEffect);
    switch (nDurationType) {
        case DURATION_TYPE_INSTANT:
            pGameEffect->m_durationTypeBits = pGameEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK;
            break;
        case DURATION_TYPE_TEMPORARY:
            pGameEffect->m_durationTypeBits = (pGameEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | DURATION_TYPE_TEMPORARY;
            pGameEffect->m_fDuration = fDuration;
            break;
        default:  // DURATION_TYPE_PERMANENT
            pGameEffect->m_durationTypeBits = (pGameEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | DURATION_TYPE_PERMANENT;
            break;
    }
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());
    CSWSArea* pArea = 0;
    if (pPlayer != 0)
        pArea = static_cast<CSWSArea*>(pPlayer->GetArea());
    if (pArea != 0)
        pArea->ApplyEffect(pGameEffect, pScriptLocation->m_vPosition,
                           pScriptLocation->m_vOrientation);
    else
        delete pGameEffect;
    if (pScriptLocation != 0)
        delete pScriptLocation;
    return 0;
}

// GetWaypointByTag: the module-wide waypoint lookup, tag-insensitive (the tag is lower-cased before the
// scan). Unusually for this file it null-checks the app manager and the server app themselves, so it is
// safe to call before the server exists; that path answers OBJECT_SELF's bit pattern, the module's own
// no-waypoint sentinel.
int CSWVirtualMachineCommands::ExecuteCommandGetWaypointByTag(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sTag;
    CSWSModule* pModule;
    if (!g_pVirtualMachine->StackPopString(&sTag))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidWaypoint = OBJECT_INVALID;
    if (g_pAppManager != 0 && g_pAppManager->m_pServerExoApp != 0) {
        pModule = g_pAppManager->m_pServerExoApp->GetModule();
        if (pModule != 0)
            oidWaypoint = pModule->GetWaypoint(sTag.LowerCase());
    }
    if (!g_pVirtualMachine->StackPushObject(oidWaypoint))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// PlayRoomAnimation: runs one of a room model's baked animations. The room is found on the client area by
// name, and the name goes through a fresh CExoString built from the popped string's buffer because the
// lookup takes a const reference.
int CSWVirtualMachineCommands::ExecuteCommandPlayRoomAnimation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sRoom;
    int nAnimation;
    CSWCRoom* pRoom;
    if (!g_pVirtualMachine->StackPopString(&sRoom))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAnimation))
        return VM_ERROR_POP_FAILED;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule != 0 && pModule->m_pArea != 0) {
        pRoom = pModule->m_pArea->GetRoom(sRoom.CStr());
        if (pRoom != 0)
            pRoom->PlayAnimation(static_cast<unsigned short>(nAnimation));
    }
    return 0;
}

// DeleteJournalWorldEntry: drops one world-journal entry by index and mirrors the delete to every
// connected client. Same shape as DeleteJournalWorldAllEntries -- message object first, module second,
// and the message null check inside the per-player walk.
int CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nEntry;
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (!g_pVirtualMachine->StackPopInteger(&nEntry))
        return VM_ERROR_POP_FAILED;
    if (pModule != 0)
        pModule->DeleteWorldJournalEntry(nEntry);
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 0;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
            pMessage->SendServerToPlayerJournalDeleteWorld(pPlayer, nEntry);
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return 0;
}

// DeleteJournalWorldEntryStrref: the by-strref twin of the above. The module call takes an unsigned long
// (the popped int is sign-extended into it) while the network message takes the raw 32-bit value, so a
// negative strref reaches the two sides differently.
int CSWVirtualMachineCommands::ExecuteCommandDeleteJournalWorldEntryStrref(int nCommand,
                                                                          int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nStrRef;
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (!g_pVirtualMachine->StackPopInteger(&nStrRef))
        return VM_ERROR_POP_FAILED;
    if (pModule != 0)
        pModule->DeleteWorldJournalEntryStrref(nStrRef);
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 0;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
            pMessage->SendServerToPlayerJournalDeleteWorldStrref(pPlayer, nStrRef);
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return 0;
}

// GetSubString: the three-pop cousin of GetStringLeft/Right, with the same two-string shape.
int CSWVirtualMachineCommands::ExecuteCommandGetSubString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sString;
    CExoString sResult;
    int nStart;
    int nCount;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nStart))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nCount))
        return VM_ERROR_POP_FAILED;
    sResult = sString.SubString(nStart, nCount);
    if (!g_pVirtualMachine->StackPushString(sResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetAbilityModifier: five of the six abilities answer their cached modifier byte straight off the stats
// block; dexterity alone goes through GetDEXMod, where the armour cap and the debilitated clamp live.
// Every modifier is read signed even where the field is declared unsigned.
int CSWVirtualMachineCommands::ExecuteCommandGetAbilityModifier(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAbility;
    if (!g_pVirtualMachine->StackPopInteger(&nAbility))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nModifier = 0;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (pCreature != 0) {
        switch (nAbility) {
        case ABILITY_STRENGTH:
            nModifier = static_cast<signed char>(pCreature->m_pStats->m_nSTRMod);
            break;
        case ABILITY_DEXTERITY:
            nModifier = static_cast<signed char>(pCreature->m_pStats->GetDEXMod(0));
            break;
        case ABILITY_CONSTITUTION:
            nModifier = pCreature->m_pStats->m_nCONMod;
            break;
        case ABILITY_INTELLIGENCE:
            nModifier = static_cast<signed char>(pCreature->m_pStats->m_nINTMod);
            break;
        case ABILITY_WISDOM:
            nModifier = pCreature->m_pStats->m_nWISMod;
            break;
        case ABILITY_CHARISMA:
            nModifier = static_cast<signed char>(pCreature->m_pStats->m_nCHAMod);
            break;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nModifier))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetHasFeat / GetFeatAcquired, one handler: the first asks whether the creature can use
// the feat right now, the second only whether it owns it. Not a creature answers 0.
int CSWVirtualMachineCommands::ExecuteCommandGetHasFeat(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
#if KOTOR1
    K2_UNUSED(nCommand);
#endif
    unsigned long long oidCreature;
    int nFeat;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nFeat))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int bHasFeat = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
#if KOTOR1  // K1 has only the remaining-uses test
        bHasFeat = (pStats->GetFeatRemainingUses(static_cast<unsigned short>(nFeat)) != 0);
#else
        if (nCommand == CMD_GetFeatAcquired)
            bHasFeat = pStats->HasFeat(static_cast<unsigned short>(nFeat));
        else
            bHasFeat = (pStats->GetFeatRemainingUses(static_cast<unsigned short>(nFeat)) != 0);
#endif
    }
    if (!g_pVirtualMachine->StackPushInteger(bHasFeat))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetGlobalBoolean (578) / GetGlobalNumber (580), the read side of SetGlobalNumber. Both funnel through
// one signed byte accumulator, so a global number above 127 comes back negative. Same
// -2000-for-a-pop-failure transposition as the writer.
int CSWVirtualMachineCommands::ExecuteCommandGetGlobalNumber(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sName;
    char nValue = 0;
    int bValue;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_PUSH_FAILED;
    if (nCommand == CMD_GetGlobalBoolean) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueBoolean(sName, bValue);
        nValue = (bValue != 0);
    } else if (nCommand == CMD_GetGlobalNumber) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sName, nValue);
    }
    if (!g_pVirtualMachine->StackPushInteger(nValue))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetGoodEvilValue (125) / GetAlignment (127), one handler: the first answers the raw 0..100 light/dark
// score, the second buckets it (below 41 dark, above 59 light, otherwise neutral). The type gate is
// "above OBJECT_TYPE_AREA" rather than "is a creature", so every object type above area is fed to the
// creature cast and only its own null check saves it.
int CSWVirtualMachineCommands::ExecuteCommandGetAlignment(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    int nAlignment = -1;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreature, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
#if KOTOR1  // K1 reads the good/evil class from the stats helper
        if (nCommand == CMD_GetGoodEvilValue)
            nAlignment = pStats->m_nAlignment;
        else if (nCommand == CMD_GetAlignmentGoodEvil)
            nAlignment = static_cast<unsigned char>(pStats->GetSimpleAlignmentGoodEvil());
#else
        if (nCommand == CMD_GetGoodEvilValue)
            nAlignment = pStats->m_nAlignment;
        else if (nCommand == CMD_GetAlignmentGoodEvil)
            nAlignment = (pStats->m_nAlignment < 41) ? ALIGNMENT_DARK_SIDE
                         : ((pStats->m_nAlignment > 59) ? ALIGNMENT_LIGHT_SIDE : ALIGNMENT_NEUTRAL);
#endif
    }
    if (!g_pVirtualMachine->StackPushInteger(nAlignment))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetDamageDealtByType: how much of the last damage OBJECT_SELF took came in with the given damage flag.
// Only creatures, placeables and doors keep the per-type breakdown, and the flag has to be a single real
// damage bit -- the base-damage sentinel and anything above it are rejected before the lookup.
int CSWVirtualMachineCommands::ExecuteCommandGetDamageDealtByType(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 leaves this uninitialised when the self object is not a CSWSObject
    CSWSObject* pObject;
#else
    CSWSObject* pObject = 0;
#endif
    CGameObject* pGameObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSelf);
    if (pGameObject != 0 && pGameObject->AsSWSObject() != 0)
        pObject = pGameObject->AsSWSObject();
    int nDamageFlag;
    if (!g_pVirtualMachine->StackPopInteger(&nDamageFlag))
        return VM_ERROR_POP_FAILED;
    int nDamage = 0;
    if (pObject != 0 && nDamageFlag >= 1
        && nDamageFlag < CSWSEffectListHandler::DAMAGE_FLAG_BASE_DAMAGE
        && (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE || pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE
            || pObject->m_objectType == GAME_OBJECT_TYPE_DOOR))
        nDamage = pObject->GetLastDamageAmountByFlags(nDamageFlag);
    if (!g_pVirtualMachine->StackPushInteger(nDamage))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// TestString: compiles the first operand as a listen-pattern expression and matches the second against
// it, answering the matched pattern id (0 = no match). The expression object is built, used and torn down
// per call.
int CSWVirtualMachineCommands::ExecuteCommandTestString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMatched;
    CExoString sPattern;
    CExoString sText;
    if (!g_pVirtualMachine->StackPopString(&sPattern))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    CSWSExpression expression;
    expression.ParseString(sPattern);
    nMatched = expression.TestString(sText, 1);
    if (!g_pVirtualMachine->StackPushInteger(nMatched))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetDistanceBetween: the full 3D distance between two objects -- the 2D twin below with the height term
// kept.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetween(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidA;
    unsigned long long oidB;
    CGameObject* pObjectA;
    CGameObject* pObjectB;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (!g_pVirtualMachine->StackPopObject(&oidA))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidB))
        return VM_ERROR_POP_FAILED;
    float fDistance = 0.0f;
    if (pArray->GetGameObject(oidA, &pObjectA) == CGameObjectArray::SUCCESS
        && pArray->GetGameObject(oidB, &pObjectB) == CGameObjectArray::SUCCESS) {
        Vector vDelta(static_cast<CSWSObject*>(pObjectA)->m_position.x - static_cast<CSWSObject*>(pObjectB)->m_position.x,
                      static_cast<CSWSObject*>(pObjectA)->m_position.y - static_cast<CSWSObject*>(pObjectB)->m_position.y,
                      static_cast<CSWSObject*>(pObjectA)->m_position.z - static_cast<CSWSObject*>(pObjectB)->m_position.z);
        fDistance = magnitude(vDelta);
    }
    if (!g_pVirtualMachine->StackPushFloat(fDistance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetWillSavingThrow: the object's will save. Three object kinds answer it, each from its own storage: a
// creature computes it through its stats block, while doors and placeables read their own field.
int CSWVirtualMachineCommands::ExecuteCommandGetWillSavingThrow(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nSave = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (pCreature != 0 && pCreature->m_pStats != 0)
                nSave = pCreature->m_pStats->GetWillSavingThrow(0);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            if (pDoor != 0)
                nSave = pDoor->m_willSave;
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            if (pPlaceable != 0)
                nSave = pPlaceable->m_willSave;
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nSave))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ModifyBaseSavingThrow (three ids on one handler): adds to one of the creature's three per-creature
// save bonuses -- 828 reflex, 829 fortitude, 830 will. The bonus bytes are additive and never clamped,
// so a script can wrap one round.
int CSWVirtualMachineCommands::ExecuteCommandModifyBaseSavingThrow(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAmount;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_pStats != 0) {
            if (nCommand == CMD_ModifyReflexSavingThrowBase)
                pCreature->m_pStats->m_nReflexSaveBonus =
                    static_cast<signed char>(pCreature->m_pStats->m_nReflexSaveBonus + nAmount);
            else if (nCommand == CMD_ModifyFortitudeSavingThrowBase)
                pCreature->m_pStats->m_nFortSaveBonus =
                    static_cast<signed char>(pCreature->m_pStats->m_nFortSaveBonus + nAmount);
            else if (nCommand == CMD_ModifyWillSavingThrowBase)
                pCreature->m_pStats->m_nWillSaveBonus =
                    static_cast<signed char>(pCreature->m_pStats->m_nWillSaveBonus + nAmount);
        }
    }
    return 0;
}

// SetMapPinEnabled: shows or hides a waypoint's map note. The flag lives on the server waypoint, and
// every connected player is told about the change individually; waypoints carrying no map note are
// ignored outright.
int CSWVirtualMachineCommands::ExecuteCommandSetMapPinEnabled(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoLinkedListNode* pNodePos;
    unsigned long long oidWaypoint;
    int bEnabled;
    if (!g_pVirtualMachine->StackPopObject(&oidWaypoint))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bEnabled))
        return VM_ERROR_POP_FAILED;
    CSWSWaypoint* pWaypoint =
        g_pAppManager->m_pServerExoApp->GetWaypointByGameObjectID(oidWaypoint);
    if (pWaypoint != 0 && pWaypoint->m_bHasMapNote != 0) {
        pWaypoint->m_bMapNoteEnabled = bEnabled;
        CSWSMessage* pMessage;
        CExoLinkedList<CSWSClient>* ppPlayers;
        pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
        if (ppPlayers == 0)
            return 0;
        pNodePos = ppPlayers->m_pInternal->m_pHead;
        while (pNodePos != 0) {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
            if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
                pMessage->SendServerToPlayerMapPinEnabled(pPlayer, oidWaypoint, bEnabled);
            ppPlayers->m_pInternal->GetNext(pNodePos);
        }
    }
    return 0;
}

// GetDistanceBetween2D: the plain XY distance between two objects, ignoring height. The object array is
// fetched once, before either pop, and reused for both lookups; the two position components are
// subtracted as one 2-float vector op and handed to magnitude with a zeroed Z.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetween2D(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidA;
    unsigned long long oidB;
    CGameObject* pObjectA;
    CGameObject* pObjectB;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (!g_pVirtualMachine->StackPopObject(&oidA))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidB))
        return VM_ERROR_POP_FAILED;
    float fDistance = 0.0f;
    if (pArray->GetGameObject(oidA, &pObjectA) == CGameObjectArray::SUCCESS
        && pArray->GetGameObject(oidB, &pObjectB) == CGameObjectArray::SUCCESS) {
        Vector vDelta(static_cast<CSWSObject*>(pObjectA)->m_position.x - static_cast<CSWSObject*>(pObjectB)->m_position.x,
                      static_cast<CSWSObject*>(pObjectA)->m_position.y - static_cast<CSWSObject*>(pObjectB)->m_position.y,
                      0.0f);
        fDistance = magnitude(vDelta);
    }
    if (!g_pVirtualMachine->StackPushFloat(fDistance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ExecuteScript: runs another script immediately, in the current VM. The optional third argument is the
// script-variant slot GetRunScriptVar reads back, popped only when the script passed one and pre-seeded
// with -1. Outside a real action script the target is replaced by OBJECT_SELF's bit pattern and the
// valid-caller flag cleared, so the script runs ownerless.
int CSWVirtualMachineCommands::ExecuteCommandExecuteScript(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sScript;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopString(&sScript))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        int nScriptVar = -1;
        if (!g_pVirtualMachine->StackPopInteger(&nScriptVar))
            return VM_ERROR_POP_FAILED;
        g_nRunScriptVar = nScriptVar;
    }
    if (m_nScriptRunArg == 1)
        g_pVirtualMachine->RunScript(&sScript, oidTarget, 1);
    else
        g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
    return 0;
}

// Sets or clears the locked flag: doors and placeables keep it on their own class, everything else is
// silently ignored.
int CSWVirtualMachineCommands::ExecuteCommandSetLocked(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameObject* pObject;
    int bLocked = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bLocked))
        return VM_ERROR_POP_FAILED;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
        == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSDoor() != 0) {
            CSWSDoor* pDoor = pObject->AsSWSDoor();
            pDoor->m_bLocked = bLocked;
        } else if (pObject->AsSWSPlaceable() != 0) {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            pPlaceable->m_bLocked = bLocked;
        }
    }
    return 0;
}


// Builds a location engine structure from a position vector and a facing angle in degrees. The angle
// is turned into a unit heading vector by converting it to radians and running sin/cos over it.
int CSWVirtualMachineCommands::ExecuteCommandLocation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vPosition;
    float fOrientation;
    CScriptLocation* pLocation = new CScriptLocation;
    if (!g_pVirtualMachine->StackPopVector(&vPosition)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pLocation;
#endif
        return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopFloat(&fOrientation)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pLocation;
#endif
        return VM_ERROR_POP_FAILED;
    }
    pLocation->m_vPosition = vPosition;
    double dRadians = static_cast<double>(fOrientation) * 0.017453292f;
    pLocation->m_vOrientation = normalize(Vector(static_cast<float>(cos(dRadians)), static_cast<float>(sin(dRadians)), 0.0f));
    int bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, pLocation);
#if KOTOR1
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    delete pLocation;
#else
    delete pLocation;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
#endif
    return 0;
}

// EventActivateItem: builds the item-activation event an activated item raises. Four object slots --
// the item,
// the activator (OBJECT_SELF), the item's area and the target object -- plus the target location's
// position in the three float slots. The area is read off the item's own area cache, not out of the popped
// location, and falls back to OBJECT_INVALID when the item id does not resolve. The popped location is
// freed only on the successful push path; the failure arms leak it.
int CSWVirtualMachineCommands::ExecuteCommandEventActivateItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptEvent* pEvent = new CScriptEvent;
    unsigned long long oidItem;
    unsigned long long oidTarget;
    CScriptLocation* pLocation = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidItem)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation))) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopObject(&oidTarget)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    unsigned long long oidArea = OBJECT_INVALID;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0)
        oidArea = pItem->m_cachedAreaId;
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_ACTIVATE_ITEM;
    pEvent->SetObjectID(0, oidItem);
    pEvent->SetObjectID(1, m_oidSelf);
    pEvent->SetObjectID(2, oidArea);
    pEvent->SetObjectID(3, oidTarget);
    pEvent->SetFloat(0, pLocation->m_vPosition.x);
    pEvent->SetFloat(1, pLocation->m_vPosition.y);
    pEvent->SetFloat(2, pLocation->m_vPosition.z);
    int bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EVENT, pEvent);
#if KOTOR1
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    delete pEvent;
#else
    delete pEvent;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
#endif
    if (pLocation != 0) {
        delete pLocation;
        pLocation = 0;
    }
    return 0;
}

// EventSpellCastAt: packs the caster, the spell id and the harmful flag into a type-2 event. The flag
// defaults to on and is only popped when the script passed it. The event is deleted on every exit.
int CSWVirtualMachineCommands::ExecuteCommandEventSpellCastAt(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CScriptEvent* pEvent = new CScriptEvent;
    int nSpell;
    int bHarmful = 1;
    unsigned long long oidCaster;
    if (!g_pVirtualMachine->StackPopObject(&oidCaster)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopInteger(&nSpell)) {
#if !KOTOR1  // K1 frees the object only after a successful push
        delete pEvent;
#endif
        return VM_ERROR_POP_FAILED;
    }
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&bHarmful)) {
#if !KOTOR1  // K1 frees the object only after a successful push
            delete pEvent;
#endif
            return VM_ERROR_POP_FAILED;
        }
    }
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT;
    pEvent->SetInteger(0, nSpell);
    pEvent->SetInteger(1, bHarmful);
    pEvent->SetObjectID(0, oidCaster);
    int bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EVENT, pEvent);
#if KOTOR1
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    delete pEvent;
#else
    delete pEvent;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
#endif
    return 0;
}

// EffectSpellImmunity: immunity to one force power, or to all of them when the id is -1. An id past the
// end of spells.2da produces an invalid-effect stub instead, which is still pushed and whose creator and
// parameter are never filled in.

// EffectSpellLevelAbsorption: three pops, then a range check on the maximum absorbed level. Out of range
// downgrades the effect to an invalid-effect stub and skips the payload; in range it stamps the
// supernatural subtype and fills slots 0-2. Integer slot 3 is written on both paths: it is the unlimited-total
// flag, set when the total-levels argument is zero.
int CSWVirtualMachineCommands::ExecuteCommandEffectSpellLevelAbsorption(int nCommand, int nArgCount) {
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nMaxSpellLevel;
    if (!g_pVirtualMachine->StackPopInteger(&nMaxSpellLevel))
        return VM_ERROR_POP_FAILED;
    int nTotalSpellLevels;
    if (!g_pVirtualMachine->StackPopInteger(&nTotalSpellLevels))
        return VM_ERROR_POP_FAILED;
    int nSpellSchool;
    if (!g_pVirtualMachine->StackPopInteger(&nSpellSchool))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (nMaxSpellLevel < -1 || nMaxSpellLevel > 9) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
        pEffect->SetCreator(m_oidSelf);
        pEffect->SetInteger(0, nMaxSpellLevel);
        pEffect->SetInteger(1, nTotalSpellLevels);
        pEffect->SetInteger(2, nSpellSchool);
    }
    pEffect->SetInteger(3, nTotalSpellLevels == 0);
    if (g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect) == 0)
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}
// The push failure path leaks the effect; only the success path deletes it.
int CSWVirtualMachineCommands::ExecuteCommandEffectSpellImmunity(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nSpell;
    if (!g_pVirtualMachine->StackPopInteger(&nSpell))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    if (nSpell >= -1 && nSpell <= g_pRules->m_pSpellArray->m_nCount) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY);
        pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
        pEffect->SetCreator(m_oidSelf);
        pEffect->SetInteger(0, nSpell);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// GetDistanceBetweenLocations2D: planar distance between two locations -- the delta is built with its Z
// component forced to zero rather than by dropping the term. Both locations are owned by this command and
// freed on every exit.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetweenLocations2D(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptLocation* pLocationA;
    CScriptLocation* pLocationB;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocationA)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocationB))) {
        if (pLocationA != 0) {
            delete pLocationA;
            pLocationA = 0;
        }
        return VM_ERROR_POP_FAILED;
    }
    Vector vDelta(pLocationA->m_vPosition.x - pLocationB->m_vPosition.x,
                  pLocationA->m_vPosition.y - pLocationB->m_vPosition.y,
                  0.0f);
    float fDistance = magnitude(vDelta);
    if (!g_pVirtualMachine->StackPushFloat(fDistance)) {
        if (pLocationA != 0) {
            delete pLocationA;
            pLocationA = 0;
        }
        if (pLocationB != 0) {
            delete pLocationB;
            pLocationB = 0;
        }
        return VM_ERROR_PUSH_FAILED;
    }
    if (pLocationA != 0) {
        delete pLocationA;
        pLocationA = 0;
    }
    if (pLocationB != 0) {
        delete pLocationB;
        pLocationB = 0;
    }
    return 0;
}

// PlayPazaak: hands the whole minigame off to the client. Only the first three operands are mandatory;
// the show-tutorial flag defaults to off and the opponent object to the invalid-object value, and the
// flag is
// normalised to 0/1.
int CSWVirtualMachineCommands::ExecuteCommandPlayPazaak(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nOpponentDeckId;
    if (!g_pVirtualMachine->StackPopInteger(&nOpponentDeckId))
        return VM_ERROR_POP_FAILED;
    CExoString sEndScript;
    if (!g_pVirtualMachine->StackPopString(&sEndScript))
        return VM_ERROR_POP_FAILED;
    int nMaxWager;
    if (!g_pVirtualMachine->StackPopInteger(&nMaxWager))
        return VM_ERROR_POP_FAILED;
    int bShowTutorial = 0;
    if (nArgCount > 3) {
        if (!g_pVirtualMachine->StackPopInteger(&bShowTutorial))
            return VM_ERROR_POP_FAILED;
    }
    unsigned long long oidOpponent = OBJECT_INVALID;
    if (nArgCount >= 5) {
        if (!g_pVirtualMachine->StackPopObject(&oidOpponent))
            return VM_ERROR_POP_FAILED;
    }
    g_pAppManager->m_pClientExoApp->StartPazaakGame(nOpponentDeckId, sEndScript, nMaxWager,
                                                    bShowTutorial != 0, oidOpponent);
    return 0;
}

// GetDistanceBetweenLocations: the 3D twin of GetDistanceBetweenLocations2D, with the same ownership
// rules and the Z term carried instead of zeroed.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceBetweenLocations(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CScriptLocation* pLocationA;
    CScriptLocation* pLocationB;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocationA)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocationB))) {
        if (pLocationA != 0) {
            delete pLocationA;
            pLocationA = 0;
        }
        return VM_ERROR_POP_FAILED;
    }
    Vector vDelta(pLocationA->m_vPosition.x - pLocationB->m_vPosition.x,
                  pLocationA->m_vPosition.y - pLocationB->m_vPosition.y,
                  pLocationA->m_vPosition.z - pLocationB->m_vPosition.z);
    float fDistance = magnitude(vDelta);
    if (!g_pVirtualMachine->StackPushFloat(fDistance)) {
        if (pLocationA != 0) {
            delete pLocationA;
            pLocationA = 0;
        }
        if (pLocationB != 0) {
            delete pLocationB;
            pLocationB = 0;
        }
        return VM_ERROR_PUSH_FAILED;
    }
    if (pLocationA != 0) {
        delete pLocationA;
        pLocationA = 0;
    }
    if (pLocationB != 0) {
        delete pLocationB;
        pLocationB = 0;
    }
    return 0;
}

// UnequipItem: queues the take-it-off action on OBJECT_SELF. Both the actor and the item are resolved
// through the object array. The item pointer is taken through the item cast without a null check, so a
// non-item object id that happens to exist queues an unequip of NULL.
int CSWVirtualMachineCommands::ExecuteCommandUnequipItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    unsigned long long oidItem;
    int bInstant = 0;
    CGameObject* pSelfObject;
    CGameObject* pItemObject;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&bInstant))
            return VM_ERROR_POP_FAILED;
    }
    if (m_nScriptRunArg == 1) {
        unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()
                                    ->GetGameObject(m_oidSelf, &pSelfObject);
        if (nStatus == CGameObjectArray::SUCCESS
            && g_pAppManager->m_pServerExoApp->GetObjectArray()
                   ->GetGameObject(oidItem, &pItemObject) == nStatus) {
            CSWSCreature* pCreature = pSelfObject->AsSWSCreature();
            pCreature->AddUnequipActions(pItemObject->AsSWSItem(),
                                         CGameObjectArray::INVALIDOBJID, 0, 0, bInstant, 0, 0);
        }
    }
    return 0;
}

// GetFirstFactionMember (380) / GetNextFactionMember (381), one handler: the faction-member iterator.
// First restarts the walk at index 0, Next resumes from the cursor the faction itself keeps. Every broken
// link in the chain -- unknown object, no faction manager, no faction record -- answers OBJECT_INVALID
// rather than an error, and so does an unrecognised command id.
int CSWVirtualMachineCommands::ExecuteCommandGetFactionMember(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidMemberOfFaction = OBJECT_INVALID;
    int bPCOnly = 1;
    CGameObject* pObject;
    int nFaction;
    if (!g_pVirtualMachine->StackPopObject(&oidMemberOfFaction))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bPCOnly))
        return VM_ERROR_POP_FAILED;
    unsigned long long oidMember = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()
            ->GetGameObject(oidMemberOfFaction, &pObject) == CGameObjectArray::SUCCESS) {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        if (pServer->m_pInternal != 0 && pServer->m_pInternal->m_pFactionManager != 0) {
            CFactionManager* pManager = pServer->m_pInternal->m_pFactionManager;
            if (pServer->GetFactionOfObject(oidMemberOfFaction, &nFaction) == 1) {
                CSWSFaction* pFaction = pManager->GetFaction(nFaction);
                if (pFaction != 0) {
                    if (nCommand == CMD_GetFirstFactionMember)
                        oidMember = pFaction->GetFactionMember(0, bPCOnly);
                    else if (nCommand == CMD_GetNextFactionMember)
                        oidMember = pFaction->GetFactionMember(pFaction->m_nMemberIterator, bPCOnly);
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidMember))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// ActionRandomWalk: queues the wander action anchored at the actor's current position, in the actor's
// current area. Only runs on OBJECT_SELF and only inside an action context. The CResRef declared here is
// dead: constructed before the guard and destroyed with nothing in between.
int CSWVirtualMachineCommands::ExecuteCommandActionRandomWalk(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS) {
            Vector vPosition = static_cast<CSWSObject*>(pObject)->m_position;
            CSWArea* pArea = static_cast<CSWSObject*>(pObject)->GetArea();
            CResRef resRef;
            if (pArea != 0 && static_cast<CSWSObject*>(pObject)->m_bCommandable != 0) {
                unsigned long long oidArea = static_cast<CSWSArea*>(pArea)->m_objectId;
                static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_RANDOM_WALK, ACTION_GROUP_NEW, ACTION_PARAM_FLOAT, &vPosition.x, ACTION_PARAM_FLOAT, &vPosition.y,
                        ACTION_PARAM_FLOAT, &vPosition.z, ACTION_PARAM_OBJECT, &oidArea, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0);
            }
        }
    }
    return 0;
}

// GetCurrentAction: the script-visible id of the action the object is running. The walk covers the whole
// leading action group (every queued node sharing the head node's group id) and does not stop at the
// first acceptable one -- the last acceptable node in the group wins. The two saturated ids stand for
// "no acceptable action" and "queue empty".
int CSWVirtualMachineCommands::ExecuteCommandGetCurrentAction(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CExoLinkedListNode* pPos;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    int nAction = ACTION_INVALID;
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        CSWSObjectActionNode* pAction = pSWSObject->m_actionList.GetHead();
        pPos = pSWSObject->m_actionList.GetHeadPos();
        if (pAction == 0) {
            nAction = ACTION_QUEUEEMPTY;
        } else {
            unsigned short nGroupId = pAction->m_nGroupId;
            do {
                if (pSWSObject->GetAcceptableAction(pAction->m_nActionId))
                    nAction = pSWSObject->GetAQActionIDByID(static_cast<unsigned short>(pAction->m_nActionId), 0);
                pAction = static_cast<CSWSObjectActionNode*>(pSWSObject->m_actionList.GetNext(pPos));
            } while (pAction != 0 && pAction->m_nGroupId == nGroupId);
        }
    }
#if KOTOR1  // K1 reads the caller's tag for the party-follow debug toggle
    if (RenderPartyFollow) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSelf);
        if (pCreature != 0 && pCreature->m_bInParty != 0 && pCreature->m_bCommandable == 1) {
            const char* pszTag = pCreature->m_sTag.CStr();
            (void)pszTag;
        }
    }
#endif
    if (!g_pVirtualMachine->StackPushInteger(nAction))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetTypeFromTalent (362) / GetIdFromTalent (363) / GetCategoryFromTalent (735), one handler over the
// popped talent structure. The category arm resolves the id through the rules tables -- feats via the feat
// table, force powers via the spell array -- and answers 0 for any other type or a missing row. An
// unrecognised command id pushes nothing and still reports success. The talent is owned here and freed on
// the way out.
int CSWVirtualMachineCommands::ExecuteCommandTalentAccess(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CScriptTalent* pTalent;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_TALENT, reinterpret_cast<void**>(&pTalent)))
        return VM_ERROR_POP_FAILED;
    int bPushed = 1;
    if (nCommand == CMD_GetTypeFromTalent) {
        bPushed = g_pVirtualMachine->StackPushInteger(pTalent->m_nType);
    } else if (nCommand == CMD_GetIdFromTalent) {
        bPushed = g_pVirtualMachine->StackPushInteger(pTalent->m_nId);
    } else if (nCommand == CMD_GetCategoryFromTalent) {
        int nCategory = 0;
        if (pTalent->m_nType == TALENT_TYPE_FORCE) {
            CSWSpell* pSpell = g_pRules->GetSpell(pTalent->m_nId);
            if (pSpell != 0)
                nCategory = pSpell->m_nCategory;
        } else if (pTalent->m_nType == TALENT_TYPE_FEAT) {
            CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(pTalent->m_nId));
            if (pFeat != 0)
                nCategory = pFeat->m_nCategory;
        }
        bPushed = g_pVirtualMachine->StackPushInteger(nCategory);
    }
    if (pTalent != 0)
        delete pTalent;
    if (!bPushed)
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetJournalQuestEntryPicture: stores the picture index as the script-local int
// "NW_JOURNAL_PICTURE<plotid>" on the target. The all-players flag fans it out to every connected player's
// creature instead of the popped object; otherwise the object gets it, plus every other member of its
// faction when the all-party-members flag is set. A null player list is the one path that answers 1
// rather than 0.
int CSWVirtualMachineCommands::ExecuteCommandSetJournalQuestEntryPicture(int nCommand,
                                                                        int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sPlotId;
    CExoString sVarName;
    int bAllPartyMembers;
    int bAllPlayers;
    int nPictureIndex;
    unsigned long long oidObject;
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    CSWSCreature* pCreature;
    CSWSFaction* pFaction;
    unsigned long long* paMembers;
    int nMembers;
    if (!g_pVirtualMachine->StackPopString(&sPlotId))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nPictureIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bAllPartyMembers))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bAllPlayers))
        return VM_ERROR_POP_FAILED;
    sVarName.Format(const_cast<char*>("NW_JOURNAL_PICTURE%s"), sPlotId.CStr());
    if (bAllPlayers) {
        ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
        if (ppPlayers == 0)
            return 1;
        pNodePos = ppPlayers->GetHeadPos();
        while (pNodePos != 0) {
            CSWSPlayer* pPlayer = reinterpret_cast<CSWSPlayer*>(ppPlayers->GetAtPos(pNodePos));
            if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
                pCreature = pPlayer->GetGameObject()->AsSWSCreature();
                if (pCreature != 0)
                    pCreature->m_scriptVars.SetInt(sVarName, nPictureIndex, 0);
            }
            ppPlayers->GetNext(pNodePos);
        }
    } else {
        pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObject);
        if (pCreature != 0) {
            pCreature->m_scriptVars.SetInt(sVarName, nPictureIndex, 0);
            if (bAllPartyMembers) {
                pFaction = pCreature->GetFaction();
                if (pFaction != 0) {
                    paMembers = pFaction->m_members.m_pArray;
                    nMembers = pFaction->m_members.m_count;
                    if (paMembers != 0) {
                        for (int i = 0; i < nMembers; i++) {
                            CSWSCreature* pMember =
                                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                                    paMembers[i]);
                            if (pMember != 0 && pMember != pCreature)
                                pMember->m_scriptVars.SetInt(sVarName, nPictureIndex, 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// GetItemPossessedBy: finds the item whose tag matches the given tag inside the given creature. The tag is
// lower-cased once up front and every comparison runs against that copy. Three possessor dialects are
// tried in order: a creature (its carried-item repository first, then a sweep of the 20 equipped slot
// bits), a placeable with an inventory, and a container item. Anything else, and every miss, answers
// OBJECT_INVALID. Each downcast is re-called at every use rather than bound to a local.
int CSWVirtualMachineCommands::ExecuteCommandGetItemPossessedBy(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sItemTag;
    unsigned long long oidPossessor;
    CGameObject* pObject;
    CSWSCreature* pCreature;
    if (!g_pVirtualMachine->StackPopObject(&oidPossessor))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sItemTag))
        return VM_ERROR_POP_FAILED;
    CExoString sLowerTag = sItemTag.LowerCase();
    unsigned long long oidItem = OBJECT_INVALID;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidPossessor, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSCreature() != 0) {
            pCreature = pObject->AsSWSCreature();
            oidItem = pCreature->GetItemRepository(1)->FindItemWithTag(sLowerTag);
            if (oidItem == OBJECT_INVALID) {
                CSWInventory* pInventory = pCreature->m_pInventory;
#if KOTOR1  // K1 has 18 equipment slots (fully unrolled)
                for (unsigned int i = 0; i < 18; i++) {
#else
                for (unsigned int i = 0; i < 20; i++) {
#endif
                    CSWSItem* pItem = static_cast<CSWSItem*>(pInventory->GetItemInSlot(1 << i));
                    if (pItem != 0 && pItem->m_sTag == sLowerTag) {
                        oidItem = pItem->m_objectId;
                        break;
                    }
                }
            }
        } else if (pObject->AsSWSPlaceable() != 0) {
            if (pObject->AsSWSPlaceable()->m_bHasInventory != 0
                && pObject->AsSWSPlaceable()->m_pItemRepository != 0)
                oidItem =
                    pObject->AsSWSPlaceable()->m_pItemRepository->FindItemWithTag(sLowerTag);
        } else if (pObject->AsSWSItem() != 0) {
            if (pObject->AsSWSItem()->m_pItemRepository != 0)
                oidItem = pObject->AsSWSItem()->m_pItemRepository->FindItemWithTag(sLowerTag);
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetCreatureTalentBest (308) / GetCreatureTalentRandom (307) share this handler. Best takes six script
// arguments (category, max CR, creature, inclusion, exclude type, exclude id), Random only three
// (category, creature, inclusion): the nested argument-count ladders pop exactly as many as were
// supplied and leave the rest on their defaults, and the Random arm hands the CR and exclusion through
// as wildcards.
int CSWVirtualMachineCommands::ExecuteCommandGetCreatureTalent(int nCommand, int nArgCount)
{
    int nCRMax K2_UNSET;
    int nCategory;
    int nInclusion = 0;
    int nExcludeType = -1;
    int nExcludeId = -1;
    unsigned long long oidCreature = OBJECT_INVALID;
    if (!g_pVirtualMachine->StackPopInteger(&nCategory))
        return VM_ERROR_POP_FAILED;
    if (nCommand == CMD_GetCreatureTalentBest) {
        if (!g_pVirtualMachine->StackPopInteger(&nCRMax))
            return VM_ERROR_POP_FAILED;
        if (nArgCount >= 3) {
            if (!g_pVirtualMachine->StackPopObject(&oidCreature))
                return VM_ERROR_POP_FAILED;
            if (nArgCount >= 4) {
                if (!g_pVirtualMachine->StackPopInteger(&nInclusion))
                    return VM_ERROR_POP_FAILED;
                if (nArgCount >= 5) {
                    if (!g_pVirtualMachine->StackPopInteger(&nExcludeType))
                        return VM_ERROR_POP_FAILED;
                    if (nArgCount >= 6) {
                        if (!g_pVirtualMachine->StackPopInteger(&nExcludeId))
                            return VM_ERROR_POP_FAILED;
                    }
                }
            }
        }
    } else if (nCommand == CMD_GetCreatureTalentRandom) {
        if (nArgCount >= 2) {
            if (!g_pVirtualMachine->StackPopObject(&oidCreature))
                return VM_ERROR_POP_FAILED;
            if (nArgCount >= 3) {
                if (!g_pVirtualMachine->StackPopInteger(&nInclusion))
                    return VM_ERROR_POP_FAILED;
            }
        }
    }
    CScriptTalent talent;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
    if (!pCreature) {
        talent.m_nId = -1;
    } else {
        int nType K2_UNSET;
        int nId K2_UNSET;
        unsigned char nMultiClass K2_UNSET;
        unsigned long long oidItem K2_UNSET;
        int nItemPropertyIndex K2_UNSET;
        unsigned char nCasterLevel K2_UNSET;
        if (nCommand == CMD_GetCreatureTalentBest) {
            pCreature->m_pStats->GetCreatureTalentRandom(nCategory, nCRMax, nType, nId,
                    nMultiClass, oidItem, nItemPropertyIndex, nCasterLevel, nInclusion,
                    nExcludeType, nExcludeId);
        } else if (nCommand == CMD_GetCreatureTalentRandom) {
            pCreature->m_pStats->GetCreatureTalentRandom(nCategory, -1, nType, nId,
                    nMultiClass, oidItem, nItemPropertyIndex, nCasterLevel, nInclusion,
                    -1, -1);
        }
        talent.m_nId = nId;
        talent.m_nType = nType;
        talent.m_nMultiClass = nMultiClass;
        talent.m_oidItem = oidItem;
        talent.m_nItemPropertyIndex = nItemPropertyIndex;
        talent.m_nCasterLevel = nCasterLevel;
    }
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_TALENT, &talent))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SpeakString (221) / ActionSpeakString (39): both pop the text and a talk-volume code, then either send
// the line straight down the chat channel (221) or queue action 0xe on OBJECT_SELF (39). The
// volume-to-chat-channel remap is a switch (whisper to 3, shout to 2, silent talk to 0xd, silent shout to
// 0xe); plain talk keeps the seeded 1.
int CSWVirtualMachineCommands::ExecuteCommandSpeakString(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sString;
    CGameObject* pObject;
    int nTalkVolume;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nTalkVolume))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1) {
        CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
        if (pArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS) {
            int nChatVolume = MSG_CHAT_TALK;
            switch (nTalkVolume) {
            case TALKVOLUME_WHISPER:
                nChatVolume = MSG_CHAT_WHISPER;
                break;
            case TALKVOLUME_SHOUT:
                nChatVolume = MSG_CHAT_SHOUT;
                break;
            case TALKVOLUME_SILENT_TALK:
                nChatVolume = MSG_CHAT_SILENT_TALK;
                break;
            case TALKVOLUME_SILENT_SHOUT:
                nChatVolume = MSG_CHAT_SILENT_SHOUT;
                break;
            }
            if (pObject) {
                if (nCommand == CMD_SpeakString) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()
                        ->SendServerToPlayerChatMessage(static_cast<unsigned char>(nChatVolume), m_oidSelf, sString, 0xffffffff,
                                                        CExoString(""));
                } else if (static_cast<CSWSObject*>(pObject)->m_bCommandable) {
                    static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_SPEAK, ACTION_GROUP_NEW, ACTION_PARAM_STRING, &sString, ACTION_PARAM_INT, &nChatVolume);
                }
            }
        }
    }
    return 0;
}

// SpeakOneLinerConversation: makes OBJECT_SELF say a barked one-liner from the given dialog resref (empty
// means the object's own default conversation) at an optional listener. Both operands are optional and
// defaulted, and both pop failures answer -2000 rather than -2001. The object array is fetched before the
// pops.
int CSWVirtualMachineCommands::ExecuteCommandSpeakOneLinerConversation(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sDialog("");
    unsigned long long oidListener = OBJECT_INVALID;
    CGameObject* pObject;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopString(&sDialog))
            return VM_ERROR_PUSH_FAILED;
        if (nArgCount >= 2) {
            if (!g_pVirtualMachine->StackPopObject(&oidListener))
                return VM_ERROR_PUSH_FAILED;
        }
    }
    if (m_nScriptRunArg == 1) {
        if (pArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS
            && pObject->AsSWSObject() != 0)
            pObject->AsSWSObject()->RunDialogOneLiner(sDialog, oidListener);
    }
    return 0;
}

// GetDistanceToObject: straight-line distance from OBJECT_SELF to the popped object, or -1.0 when either
// end is missing, the target is an area-or-lower object type, or the two are in different areas. The
// second array lookup is compared against the first lookup's saved status code rather than against the
// success code again.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceToObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pSelfObject;
    unsigned long long oidTarget;
    CGameObject* pTargetObject;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    float fDistance = -1.0f;
    if (m_nScriptRunArg == 1) {
        unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()
                                    ->GetGameObject(m_oidSelf, &pSelfObject);
        if (nStatus == CGameObjectArray::SUCCESS) {
            Vector vSelf = static_cast<CSWSObject*>(pSelfObject)->m_position;
            CSWArea* pSelfArea = static_cast<CSWSObject*>(pSelfObject)->GetArea();
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                        ->GetGameObject(oidTarget, &pTargetObject) == nStatus
                && pTargetObject != 0
                && pTargetObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
                Vector vTarget = static_cast<CSWSObject*>(pTargetObject)->m_position;
                if (static_cast<CSWSObject*>(pTargetObject)->GetArea() == pSelfArea) {
                    Vector vDelta(vTarget.x - vSelf.x, vTarget.y - vSelf.y, vTarget.z - vSelf.z);
                    fDistance = magnitude(vDelta);
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushFloat(fDistance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GetDistanceToObject2D: the planar twin of GetDistanceToObject -- the Z term is zeroed rather than
// dropped, so only the X/Y pair of each position is ever loaded.
int CSWVirtualMachineCommands::ExecuteCommandGetDistanceToObject2D(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pSelfObject;
    unsigned long long oidTarget;
    CGameObject* pTargetObject;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    float fDistance = -1.0f;
    if (m_nScriptRunArg == 1) {
        unsigned char nStatus = g_pAppManager->m_pServerExoApp->GetObjectArray()
                                    ->GetGameObject(m_oidSelf, &pSelfObject);
        if (nStatus == CGameObjectArray::SUCCESS) {
            Vector vSelf = static_cast<CSWSObject*>(pSelfObject)->m_position;
            CSWArea* pSelfArea = static_cast<CSWSObject*>(pSelfObject)->GetArea();
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                        ->GetGameObject(oidTarget, &pTargetObject) == nStatus
                && pTargetObject != 0
                && pTargetObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
                Vector vTarget = static_cast<CSWSObject*>(pTargetObject)->m_position;
                if (static_cast<CSWSObject*>(pTargetObject)->GetArea() == pSelfArea) {
                    Vector vDelta(vTarget.x - vSelf.x, vTarget.y - vSelf.y, 0.0f);
                    fDistance = magnitude(vDelta);
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushFloat(fDistance))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// EffectTemporaryForcePoints: a supernatural-subtype temporary Force-point pool. A non-positive amount
// produces an invalid-effect stub that is still pushed. The creator is only stamped when OBJECT_SELF
// resolves to a creature, but the id stamped is OBJECT_SELF's either way.
int CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryFP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (nAmount >= 1) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TEMPORARY_FORCE);
        pEffect->SetInteger(0, nAmount);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        pEffect->SetCreator(m_oidSelf);
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The hit-point twin of EffectTemporaryForcePoints, with the same skeleton and the same creature-only
// creator stamp.
int CSWVirtualMachineCommands::ExecuteCommandEffectTemporaryHP(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nAmount;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    if (nAmount >= 1) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TEMPORARY_HITPOINTS);
        pEffect->SetInteger(0, nAmount);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_INVALIDEFFECT);
    }
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0)
        pEffect->SetCreator(m_oidSelf);
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// Both readings are virtual and dispatched through the creature override or the object base; both
// getters return a short, so the pushed value is sign-extended.
int CSWVirtualMachineCommands::ExecuteCommandGetHitpoints(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameObject* pObject;
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    int nHitPoints = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidObject, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (nCommand == CMD_GetCurrentHitPoints) {
            if (pCreature != 0)
                nHitPoints = pObject->AsSWSCreature()->GetCurrentHitPoints(0);
            else
                nHitPoints = pObject->AsSWSObject()->GetCurrentHitPoints(0);
        } else {
            if (pCreature != 0)
                nHitPoints = pObject->AsSWSCreature()->GetMaxHitPoints(1);
            else
                nHitPoints = pObject->AsSWSObject()->GetMaxHitPoints(1);
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(nHitPoints))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Mirrors GetEnteringObject, returning OBJECT_SELF's OnExit caller, dispatched on the object type.
// Trigger and encounter share a tail because both keep the id at the same place. Types with no exit
// event answer OBJECT_INVALID.
int CSWVirtualMachineCommands::ExecuteCommandGetLeavingObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidLeaving = CGameObjectArray::INVALIDOBJID;
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS) {
            switch (pObject->m_objectType) {
            case GAME_OBJECT_TYPE_MODULE: {
                CSWSModule* pModule = pObject->AsSWSModule();
                if (pModule != 0)
                    oidLeaving = pModule->m_oidLeavingClient;
                break;
            }
            case GAME_OBJECT_TYPE_AREA: {
                CSWSArea* pArea = pObject->AsSWSArea();
                if (pArea != 0)
                    oidLeaving = pArea->m_onExitScriptCallerId;
                break;
            }
            case GAME_OBJECT_TYPE_TRIGGER: {
                CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
                if (pTrigger != 0)
                    oidLeaving = pTrigger->m_oidExitingObject;
                break;
            }
            case GAME_OBJECT_TYPE_AREA_OF_EFFECT: {
                CSWSAreaOfEffectObject* pAoE = pObject->AsSWSAreaOfEffectObject();
                if (pAoE != 0)
                    oidLeaving = pAoE->m_exitObjectId;
                break;
            }
            case GAME_OBJECT_TYPE_ENCOUNTER: {
                CSWSEncounter* pEncounter = pObject->AsSWSEncounter();
                if (pEncounter != 0)
                    oidLeaving = pEncounter->m_exitObjectId;
                break;
            }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidLeaving))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetFacingPoint-style "turn your back on him" (553): points the first object's facing along the vector
// away from the second. Both ends must exist, both must be real server objects and both must share an area
// cache key; the two positions must also differ by more than 1e-4 squared, so the guard is the squared
// length written out rather than a magnitude call. The missing-second-object exit returns the null pointer
// itself as the status code, which is 0, the same value the success path answers.
int CSWVirtualMachineCommands::ExecuteCommandFaceObjectAwayFromObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObjectRaw = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    CSWSObject* pObject = pObjectRaw != 0 ? pObjectRaw->AsSWSObject() : 0;
    CGameObject* pTargetRaw = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pTargetRaw == 0)
        return 0;
    CSWSObject* pTarget = pTargetRaw->AsSWSObject();
    if (pObject == 0 || pTarget == 0)
        return 0;
    if (pObject->m_cachedAreaId == pTarget->m_cachedAreaId) {
        Vector vAway(pObject->m_position.x - pTarget->m_position.x,
                     pObject->m_position.y - pTarget->m_position.y,
                     pObject->m_position.z - pTarget->m_position.z);
        if (vAway.x * vAway.x + vAway.y * vAway.y + vAway.z * vAway.z > 0.0001f) {
            vAway = normalize(vAway);
            pObject->SetOrientation(vAway);
        }
    }
    return 0;
}

// The six ambient-sound commands (433 play, 434 stop, 435 change day, 436 change night, 567 set day
// volume, 568 set night volume), one handler over the area's ambient sound. The int operand is optional and
// defaults to 0; the play/stop pair ignores it and passes its own 1/0. The missing-area exit reuses the
// null area pointer as the return code.
int CSWVirtualMachineCommands::ExecuteCommandAmbientSound(int nCommand, int nArgCount)
{
    unsigned long long oidArea;
    int nValue = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nValue))
            return VM_ERROR_POP_FAILED;
    }
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea != 0) {
        switch (nCommand) {
        case CMD_AmbientSoundPlay: pArea->m_pAmbientSound->PlayAmbientSound(1); break;
        case CMD_AmbientSoundStop: pArea->m_pAmbientSound->PlayAmbientSound(0); break;
        case CMD_AmbientSoundChangeDay: pArea->m_pAmbientSound->SetAmbientDayTrack(nValue); break;
        case CMD_AmbientSoundChangeNight: pArea->m_pAmbientSound->SetAmbientNightTrack(nValue); break;
        case CMD_AmbientSoundSetDayVolume: pArea->m_pAmbientSound->SetAmbientDayVolume(nValue); break;
        case CMD_AmbientSoundSetNightVolume: pArea->m_pAmbientSound->SetAmbientNightVolume(nValue); break;
        }
    }
    return 0;
}

// SurrenderToEnemies (379, the action form) plus its two direct twins (476 and 762, which call the
// creature method outright with a 0/1 flag). All three refuse to run on a player character, and the action
// form queues the surrender action with no parameters. The object array is fetched before the
// action-context test.
int CSWVirtualMachineCommands::ExecuteCommandSurrenderToEnemies(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CGameObject* pObject;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    if (m_nScriptRunArg == 1
        && pArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_pStats->m_bIsPC == 0) {
            if (nCommand == CMD_ActionSurrenderToEnemies) {
                pCreature->AddAction(AIACTION_SURRENDER_TO_ENEMIES, ACTION_GROUP_NEW, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            } else if (nCommand == CMD_SurrenderToEnemies) {
                pCreature->SurrenderToEnemies(0);
            } else if (nCommand == CMD_SurrenderRetainBuffs) {
                pCreature->SurrenderToEnemies(1);
            }
        }
    }
    return 0;
}

// Wait / DelayCommand's sleep primitive: queues the sleep action carrying the popped duration as its
// single
// float parameter. The server-object probe is spelled out three times -- guard, commandable read and action
// receiver.
int CSWVirtualMachineCommands::ExecuteCommandWait(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    float fSeconds;
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopFloat(&fSeconds))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject->AsSWSObject() != 0
        && pObject->AsSWSObject()->m_bCommandable != 0) {
        pObject->AsSWSObject()->AddAction(AIACTION_WAIT, ACTION_GROUP_NEW, ACTION_PARAM_FLOAT, &fSeconds, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 0;
}

// PlayMovie: flushes pending input, drops the driving accelerator, optionally silences everything but the
// video, and hands the resref to the client. The skippable flag doubles as the mute switch -- 0 pauses all
// sound for the duration -- and the movie's own result slot is a by-reference int the command never reads.
// The two operands are popped with the input and accelerator reset between them.
int CSWVirtualMachineCommands::ExecuteCommandPlayMovie(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sMovie;
    if (!g_pVirtualMachine->StackPopString(&sMovie))
        return VM_ERROR_POP_FAILED;
    g_pExoInput->ClearEvents();
    g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
#if KOTOR1  // K1 PlayMovie takes no skippable operand; every movie pauses all sound
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL);
    int nResult;
    g_pAppManager->m_pClientExoApp->HideLoadScreen(0, 1);
    g_pAppManager->m_pClientExoApp->PlayMovie(sMovie, nResult, 0, 0);
#else
    int bSkippable = 0;
    if (!g_pVirtualMachine->StackPopInteger(&bSkippable))
        return VM_ERROR_POP_FAILED;
    if (bSkippable == 0)
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
    int nResult;
    g_pAppManager->m_pClientExoApp->HideLoadScreen(0, 1, 1);
    g_pAppManager->m_pClientExoApp->PlayMovie(sMovie, nResult, 0, 0, bSkippable != 0);
#endif
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
    return 0;
}

// The five background-music commands (425 play, 426 stop, 427 set delay, 428 change day, 429 change
// night), one handler over the area's ambient sound. Both int operands default to 0, and the second is only
// popped when the first was -- the argument-count tests nest.
int CSWVirtualMachineCommands::ExecuteCommandMusicBackground(int nCommand, int nArgCount)
{
    unsigned long long oidArea;
    int nTrack = 0;
#if !KOTOR1
    int nStreamingMusic;
#endif
    if (!g_pVirtualMachine->StackPopObject(&oidArea))
        return VM_ERROR_POP_FAILED;
    if (nArgCount > 1) {
        if (!g_pVirtualMachine->StackPopInteger(&nTrack))
            return VM_ERROR_POP_FAILED;
#if KOTOR1  // K1 has no streaming-music argument
    }
#else
        nStreamingMusic = 0;
        if (nArgCount >= 3) {
            if (!g_pVirtualMachine->StackPopInteger(&nStreamingMusic))
                return VM_ERROR_POP_FAILED;
        }
    } else {
        nStreamingMusic = 0;
    }
#endif
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidArea);
    if (pArea != 0) {
        switch (nCommand) {
        case CMD_MusicBackgroundPlay: pArea->m_pAmbientSound->PlayMusic(1); break;
        case CMD_MusicBackgroundStop: pArea->m_pAmbientSound->PlayMusic(0); break;
        case CMD_MusicBackgroundSetDelay: pArea->m_pAmbientSound->SetMusicDelay(nTrack); break;
#if KOTOR1
        case CMD_MusicBackgroundChangeDay: pArea->m_pAmbientSound->SetMusicDayTrack(nTrack); break;
        case CMD_MusicBackgroundChangeNight: pArea->m_pAmbientSound->SetMusicNightTrack(nTrack); break;
#else
        case CMD_MusicBackgroundChangeDay: pArea->m_pAmbientSound->SetMusicDayTrack(nTrack, nStreamingMusic); break;
        case CMD_MusicBackgroundChangeNight: pArea->m_pAmbientSound->SetMusicNightTrack(nTrack, nStreamingMusic); break;
#endif
        }
    }
    return 0;
}

// PopUpGUIPanel (388) / PopUpDeathGUIPanel (554), one handler over the same client message. Every operand
// past the player object is optional and defaulted, and the two commands fill different slots -- 388 the
// panel id, 554 the death panel's respawn and wait flags, help strref and help text. An OBJECT_INVALID
// player is silently dropped.
int CSWVirtualMachineCommands::ExecuteCommandPopUpGUIPanel(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidPlayer = OBJECT_INVALID;
    int nGUIPanel = 0;
    int bRespawnButtonEnabled = 1;
    int bWaitForViewer = 1;
    int nHelpStringReference = 0;
    CExoString sHelpString("");
    if (!g_pVirtualMachine->StackPopObject(&oidPlayer))
        return VM_ERROR_POP_FAILED;
    if (nCommand == CMD_PopUpGUIPanel) {
        if (!g_pVirtualMachine->StackPopInteger(&nGUIPanel))
            return VM_ERROR_POP_FAILED;
    } else if (nCommand == CMD_PopUpDeathGUIPanel) {
        if (!g_pVirtualMachine->StackPopInteger(&bRespawnButtonEnabled))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&bWaitForViewer))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopInteger(&nHelpStringReference))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopString(&sHelpString))
            return VM_ERROR_POP_FAILED;
    }
    if (oidPlayer != OBJECT_INVALID) {
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerPopUpGUIPanel(
            oidPlayer, nGUIPanel, bRespawnButtonEnabled != 0, bWaitForViewer != 0,
            nHelpStringReference, sHelpString);
    }
    return 0;
}

// SetCustomToken: writes one custom talk-table token and fans the new value out to every connected player.
// Tokens at or below the reserved-token end are engine-reserved and silently ignored. Both pop failures
// answer -2000 rather than -2001.
int CSWVirtualMachineCommands::ExecuteCommandSetCustomToken(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sValue;
    int nToken;
    if (!g_pVirtualMachine->StackPopInteger(&nToken))
        return VM_ERROR_PUSH_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sValue))
        return VM_ERROR_PUSH_FAILED;
    if (nToken > CTlkTable::CUSTOMTOKEN_RESERVED_END) {
        g_pTlkTable->SetCustomToken(nToken, sValue);
        CSWSMessage* pMessage;
        CExoLinkedList<CSWSClient>* ppPlayers;
        pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
        if (pMessage != 0 && ppPlayers != 0) {
            CExoLinkedListNode* pNodePos = ppPlayers->GetHeadPos();
            while (pNodePos != 0) {
                CSWSPlayer* pEach = reinterpret_cast<CSWSPlayer*>(ppPlayers->GetAtPos(pNodePos));
                if (pEach != 0 && pEach->GetGameObject() != 0)
                    pMessage->SendServerToPlayerSetCustomToken(pEach->m_nPlayerId, nToken, sValue);
                ppPlayers->GetNext(pNodePos);
            }
        }
    }
    return 0;
}

// AddJournalWorldEntry: records a world-journal entry carrying a literal name and text pair, stamped with
// the current world time, then broadcasts the same four values to every connected client. The message and
// the module are both fetched before the pops.
int CSWVirtualMachineCommands::ExecuteCommandAddJournalWorldEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sName;
    CExoString sText;
    int nIndex;
    unsigned int nDay;
    unsigned int nTime;
    CExoLinkedList<CSWSClient>* ppPlayers;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopString(&sText))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
    if (pModule != 0)
        pModule->AddWorldJournalEntry(nIndex, sName, sText, nDay, nTime);
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers != 0) {
        CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
        while (pNodePos != 0) {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
            if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
                pMessage->SendServerToPlayerJournalAddWorld(pPlayer, nIndex, sName, sText, nDay,
                                                            nTime);
            ppPlayers->m_pInternal->GetNext(pNodePos);
        }
    }
    return 0;
}

// AddJournalWorldEntryStrref: the strref-keyed twin of AddJournalWorldEntry -- both halves of the entry are
// talk-table references rather than literal strings, so nothing here touches a CExoString. The module
// records the entry stamped with the current world time, and the same four values go out to every connected
// client. The message and the module are both fetched before the pops.
int CSWVirtualMachineCommands::ExecuteCommandAddJournalWorldEntryStrref(int nCommand,
                                                                       int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    int nNameStrRef;
    int nTextStrRef;
    unsigned int nDay;
    unsigned int nTime;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    if (!g_pVirtualMachine->StackPopInteger(&nNameStrRef))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nTextStrRef))
        return VM_ERROR_POP_FAILED;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
    if (pModule != 0)
        pModule->AddWorldJournalEntryStrref(nNameStrRef, nTextStrRef, nDay, nTime);
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 0;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0 && pMessage != 0)
            pMessage->SendServerToPlayerJournalAddWorldStrref(pPlayer, nNameStrRef, nTextStrRef,
                                                              nDay, nTime);
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return 0;
}

// AddMultiClass: grants a creature a level in a new class. The experience needed to reach the next level,
// minus what the creature has already earned, is the catch-up grant -- and where it goes is the point of the
// split: a player-made character hands it to the party table so the whole party levels together, while a
// plain NPC swallows it itself, records it as joining XP, and auto-levels on the spot when the client option
// says NPCs level up themselves. There is no null check on the creature.
int CSWVirtualMachineCommands::ExecuteCommandAddMultiClass(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nClassType;
    if (!g_pVirtualMachine->StackPopInteger(&nClassType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreature);
#if KOTOR1
    // K1 ignores a null target and always gives the level's experience to the party (no player-made
    // distinction, no NPC auto-level-up).
    if (pCreature == 0)
        return 0;
    int nCurrentXP = pCreature->m_pStats->m_nExperience;
    int nXPNeeded = pCreature->m_pStats->CalculateExpNeededForNextLevel();
    pCreature->m_pStats->AddClass(static_cast<unsigned char>(nClassType), 0);
    if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0) {
        int nPartyGain = nXPNeeded - nCurrentXP;
        g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nPartyGain, 1);
    }
    return 0;
#else
    int bPlayerMade = pCreature->m_bPlayerMadeCharacter;
    int nCurrentXP = pCreature->m_pStats->m_nExperience;
    int nXPNeeded = pCreature->m_pStats->GetExpNeededForLevelUp();
    if (bPlayerMade) {
        pCreature->m_pStats->AddClass(static_cast<unsigned char>(nClassType), 0);
        if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0) {
            int nPartyGain = nXPNeeded - nCurrentXP;
            g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nPartyGain, 1);
        }
    } else {
        int nOwnGain = nXPNeeded - nCurrentXP;
        pCreature->m_pStats->AddClass(static_cast<unsigned char>(nClassType), 0);
        pCreature->m_pStats->AddExperience(nOwnGain);
        pCreature->m_nJoiningXP += nOwnGain;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoLevelUpNPCs
            && pCreature->m_pStats->CanLevelUp())
            pCreature->m_pStats->AutoLevelup(1);
    }
    return 0;
#endif
}

// DuplicateHeadAppearance: copies one creature's head model onto another. The head is not copied verbatim
// -- it is re-resolved through appearance.2da from the donor's head id, and which column supplies it
// depends on the donor's own appearance flag: set picks the backup head, clear the normal head. The
// resolved row value lands in the recipient's appearance byte.
// ModifyGlobalNumber: one body behind two command ids -- 799 increments the named global-variable-table
// number by the popped amount, 800 decrements it. Both saturate at the signed-char ends rather than
// wrapping, so the sum is computed in int before the clamp.
int CSWVirtualMachineCommands::ExecuteCommandModifyGlobal(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CExoString sName;
    int nAmount;
    char nCurrent;
    if (!g_pVirtualMachine->StackPopString(&sName))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    if (nCommand == CMD_IncrementGlobalNumber) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sName, nCurrent);
        if (nCurrent + nAmount >= 128)
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sName, 127);
        else
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
                sName, static_cast<char>(nCurrent + nAmount));
    } else if (nCommand == CMD_DecrementGlobalNumber) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sName, nCurrent);
        if (nCurrent - nAmount < -128)
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sName, -128);
        else
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
                sName, static_cast<char>(nCurrent - nAmount));
    }
    return 0;
}

int CSWVirtualMachineCommands::ExecuteCommandDuplicateHeadAppearance(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObjectToChange;
    unsigned long long oidObjectToChangeTo;
    int nHead;
    if (!g_pVirtualMachine->StackPopObject(&oidObjectToChange))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidObjectToChangeTo))
        return VM_ERROR_POP_FAILED;
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObjectToChange);
    CSWSCreature* pDonor =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidObjectToChangeTo);
    if (pTarget != 0 && pDonor != 0) {
        C2DA* pAppearance = g_pRules->m_p2DArrays->m_pAppearance;
        unsigned short nHeadId = pDonor->m_appearance.m_headId;
        if (pDonor->m_appearance.m_bUseBackupHead != 0) {
            CExoString sColumn("BACKUPHEAD");
            pAppearance->GetINTEntry(nHeadId, sColumn, &nHead);
        } else {
            CExoString sColumn("NORMALHEAD");
            pAppearance->GetINTEntry(nHeadId, sColumn, &nHead);
        }
        pTarget->m_appearance.m_nDuplicatingHead = static_cast<unsigned char>(nHead);
    }
    return 0;
}

// ModuleAccess: five commands that answer a single field of the module record, four object ids and one
// location -- GetPCItemLastEquipped (52), GetLastPlayerDied (291), GetLastPlayerDying (410),
// GetStartingLocation (411) and GetLastRespawnButtonPresser (419). Every one degrades to OBJECT_INVALID or
// the zeroed location when no module is loaded, so the module null check is a guard around the read rather
// than an early bail.
int CSWVirtualMachineCommands::ExecuteCommandModuleAccess(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CScriptLocation sLocation;
    // The literal invalid-object id, not the global, which would cost a load.
    unsigned long long oidObject = OBJECT_INVALID;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        switch (nCommand) {
        case CMD_GetLastItemEquipped:
            oidObject = pModule->m_oidEquippedItem;
            break;
        case CMD_GetLastPlayerDied:
            oidObject = pModule->m_oidDeadPlayer;
            break;
        case CMD_GetLastPlayerDying:
            oidObject = pModule->m_oidDyingPlayer;
            break;
        case CMD_GetLastRespawnButtonPresser:
            oidObject = pModule->m_oidSpawningPlayer;
            break;
        case CMD_GetStartingLocation:
            sLocation.m_vPosition.x = pModule->m_pEntryInfo->m_entryX;
            sLocation.m_vPosition.y = pModule->m_pEntryInfo->m_entryY;
            sLocation.m_vPosition.z = pModule->m_pEntryInfo->m_entryZ;
            sLocation.m_vOrientation.x = pModule->m_pEntryInfo->m_entryDirX;
            sLocation.m_vOrientation.y = pModule->m_pEntryInfo->m_entryDirY;
            break;
        }
    }
    switch (nCommand) {
    case CMD_GetLastItemEquipped:
    case CMD_GetLastPlayerDied:
    case CMD_GetLastPlayerDying:
    case CMD_GetLastRespawnButtonPresser:
        if (!g_pVirtualMachine->StackPushObject(oidObject))
            return VM_ERROR_PUSH_FAILED;
        break;
    case CMD_GetStartingLocation:
        if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, &sLocation))
            return VM_ERROR_PUSH_FAILED;
        break;
    }
    return 0;
}

// FortitudeSave (108) / ReflexSave (109) / WillSave (110): one handler, with the command id selecting the
// save index the creature's roll expects. The last two arguments are optional -- the save type and the
// versus object are popped only when they were supplied -- and the versus object defaults to
// OBJECT_SELF. The save type has no default, so when the script omits it the roll reads an
// uninitialised local; nwscript's own declaration always emits all four arguments, so in practice it
// is always set.
int CSWVirtualMachineCommands::ExecuteCommandSavingThrow(int nCommand, int nArgCount)
{
    unsigned long long oidTarget = OBJECT_INVALID;
    unsigned long long oidSaveVersus = m_oidSelf;
    int nDC;
#ifdef K2_FIX_ENGINE_BUGS
    int nSaveType = 0;
#else
    int nSaveType;
#endif
    CGameObject* pObject;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nDC))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nSaveType))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPopObject(&oidSaveVersus))
            return VM_ERROR_POP_FAILED;
    }
    int nResult = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
        int nSave = 1;
        if (nCommand == CMD_WillSave)
            nSave = 3;
        else if (nCommand == CMD_ReflexSave)
            nSave = 2;
        nResult = pObject->AsSWSCreature()->SavingThrowRoll(static_cast<unsigned char>(nSave), static_cast<unsigned short>(nDC), static_cast<unsigned char>(nSaveType), oidSaveVersus,
                                                            1, 0, 0);
    }
    if (!g_pVirtualMachine->StackPushInteger(nResult))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// RemoveEffect: unlinks an effect from an object. It does not touch the list here -- it clears the effect's
// exposed flag and hands the AI master an EVENT_REMOVE_EFFECT, which is what actually retires it. The scan
// matches on the effect id and does not stop at the first hit: it clears the flag on every copy and queues
// the last one. The popped engine structure is this command's to free either way.
int CSWVirtualMachineCommands::ExecuteCommandRemoveEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pTarget = pObject->AsSWSObject();
        CGameEffect* pFound = 0;
        for (int i = 0; i < pTarget->m_effects.m_count; i++) {
            if (pTarget->m_effects.m_pArray[i]->m_effectId == pEffect->m_effectId) {
                pTarget->m_effects.m_pArray[i]->m_bIsExposed = 0;
                pFound = pTarget->m_effects.m_pArray[i];
            }
        }
        if (pFound != 0)
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_oidSelf, oidObject, CServerAIMaster::EVENT_REMOVE_EFFECT, pFound);
    }
    delete pEffect;
    return 0;
}

// The by-integer twin of RemoveEffect. Instead of matching on the effect's own id it matches on integer
// parameter 0, and instead of remembering the last hit it queues an EVENT_REMOVE_EFFECT for every match
// as it goes, so the list count is re-read on each iteration. The exposed-flag store runs before the
// null test on the pointer it just wrote through.
int CSWVirtualMachineCommands::ExecuteCommandRemoveEffectByID(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nEffectId;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nEffectId))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pTarget = pObject->AsSWSObject();
        for (int i = 0; i < pTarget->m_effects.m_count; i++) {
            if (pTarget->m_effects.m_pArray[i]->GetInteger(0) == nEffectId) {
                CGameEffect* pEffect = pTarget->m_effects.m_pArray[i];
                pEffect->m_bIsExposed = 0;
                if (pEffect != 0)
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_oidSelf, oidObject, CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            }
        }
    }
    return 0;
}

// RemoveEffectByExactMatch: the strictest of the three remove twins -- an entry only matches when its
// subtype and integer parameters 0 and 1 all equal the popped structure's, and unlike its siblings the scan
// stops at the first hit. Same retirement mechanism: clear the exposed flag and queue an EVENT_REMOVE_EFFECT
// for the AI master to unlink. The exposed-flag store runs before the null test on the pointer it just wrote
// through, as in RemoveEffectByID.
int CSWVirtualMachineCommands::ExecuteCommandRemoveEffectByExactMatch(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    CGameEffect* pEffect = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, reinterpret_cast<void**>(&pEffect)))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pTarget = pObject->AsSWSObject();
        for (int i = 0; i < pTarget->m_effects.m_count; i++) {
            if (pTarget->m_effects.m_pArray[i]->m_subType == pEffect->m_subType
                    && pTarget->m_effects.m_pArray[i]->GetInteger(0) == pEffect->GetInteger(0)
                    && pTarget->m_effects.m_pArray[i]->GetInteger(1) == pEffect->GetInteger(1)) {
                CGameEffect* pFound = pTarget->m_effects.m_pArray[i];
                pFound->m_bIsExposed = 0;
                if (pFound != 0)
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_oidSelf, oidObject, CServerAIMaster::EVENT_REMOVE_EFFECT, pFound);
                break;
            }
        }
    }
    delete pEffect;
    return 0;
}

// The two writers of an object's small fixed-slot variable table. Areas and modules keep theirs
// separately and are reached through their virtual downcasts; everything else is a plain CSWSObject.
// The number writer's slot is a byte, so the value is range-checked to [-128,127] first.
int CSWVirtualMachineCommands::ExecuteCommandSetLocalValue(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    int nIndex;
    int nValue;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nIndex))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nValue))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    if (pObject != 0) {
        CSWVarTable* pVarTable;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
            pVarTable = &pObject->AsSWSArea()->m_varTable;
        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE)
            pVarTable = &pObject->AsSWSModule()->m_varTable;
        else
            pVarTable = &static_cast<CSWSObject*>(pObject)->m_varTable;
        if (nCommand == CMD_SetLocalBoolean) {
            pVarTable->SetLocalBoolean(nIndex, nValue);
        } else if (nCommand == CMD_SetLocalNumber) {
            if (nValue < -128 || nValue > 127) {
                static_cast<CSWSObject*>(pObject)->m_sTag.CStr();
                static_cast<CSWSObject*>(pObject)->m_sTag.CStr();
            } else {
                pVarTable->SetLocalNumber(nIndex, static_cast<unsigned char>(nValue));
            }
        }
    }
    return 0;
}

// GetItemHasItemProperty: whether an item carries a given item-property type, in either of its two
// property lists. The active list is filtered -- a property is live only when its slot bit is set in the
// item's upgrade mask, an all-ones mask meaning always live -- while the passive list is not filtered
// at all.
int CSWVirtualMachineCommands::ExecuteCommandGetItemHasItemProperty(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    int nProperty;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nProperty))
        return VM_ERROR_POP_FAILED;
    int bHasProperty = 0;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem != 0) {
        for (int i = 0; i < pItem->m_nActiveProperties; i++) {
            CSWItemProperty* pProperty = pItem->GetActiveProperty(i);
            if (pProperty != 0
                && (pProperty->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE
                    || (pItem->m_nUpgrades & (1 << pProperty->m_nUpgradeType)) != 0)
                && pProperty->m_type == nProperty) {
                bHasProperty = 1;
                break;
            }
        }
        if (bHasProperty == 0) {
            for (int i = 0; i < pItem->m_nPassiveProperties; i++) {
                CSWItemProperty* pProperty = pItem->GetPassiveProperty(i);
                if (pProperty != 0 && pProperty->m_type == nProperty) {
                    bHasProperty = 1;
                    break;
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushInteger(bHasProperty))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Adds a signed delta to one of the six base ability scores, in STR/DEX/CON/INT/WIS/CHA order.
// Constitution is the odd one out because its setter also re-derives hit points. The new value is
// computed as a byte sum, so a delta that pushes the score past 255 wraps.
int CSWVirtualMachineCommands::ExecuteCommandAdjustCreatureAttributes(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidCreature;
    int nAbility;
    int nAmount;
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAbility))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nAmount))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature == 0)
            return 0;
        CSWSCreatureStats* pStats = pCreature->m_pStats;
        if (pStats == 0)
            return 0;
        switch (nAbility) {
        case ABILITY_STRENGTH:
            pStats->SetSTRBase(static_cast<unsigned char>(pStats->m_nSTRBase + nAmount));
            break;
        case ABILITY_DEXTERITY:
            pStats->SetDEXBase(static_cast<unsigned char>(pStats->m_nDEXBase + nAmount));
            break;
        case ABILITY_CONSTITUTION:
            pStats->SetCONBase(static_cast<unsigned char>(pStats->m_nCONBase + nAmount), 1);
            break;
        case ABILITY_INTELLIGENCE:
            pStats->SetINTBase(static_cast<unsigned char>(pStats->m_nINTBase + nAmount));
            break;
        case ABILITY_WISDOM:
            pStats->SetWISBase(static_cast<unsigned char>(pStats->m_nWISBase + nAmount));
            break;
        case ABILITY_CHARISMA:
            pStats->SetCHABase(static_cast<unsigned char>(pStats->m_nCHABase + nAmount));
            break;
        }
    }
    return 0;
}

// EffectVisualEffect: builds the visual-effect engine structure. Integer 0 is the vfx_*.2da row and integer
// 2 the optional miss-effect flag. The creator is stamped only when OBJECT_SELF actually resolves.
int CSWVirtualMachineCommands::ExecuteCommandEffectVisualEffect(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    int nVisualEffectId;
    int bMissEffect = 0;
    CGameObject* pSelfObject;
    if (!g_pVirtualMachine->StackPopInteger(&nVisualEffectId))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&bMissEffect))
            return VM_ERROR_POP_FAILED;
    }
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
    pEffect->SetInteger(0, nVisualEffectId);
    pEffect->SetInteger(2, bMissEffect);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pSelfObject)
            == CGameObjectArray::SUCCESS
        && pSelfObject != 0)
        pEffect->SetCreator(m_oidSelf);
    if (!g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_EFFECT, pEffect))
        return VM_ERROR_PUSH_FAILED;
    delete pEffect;
    return 0;
}

// The one-argument sibling of the script-log entry above, the same release-empty variadic, so only
// the CStr() call survives.
static inline void ScriptLogString(const char* pszText) { K2_UNUSED(pszText);  }

// PrintFloat: formats a float with a caller-chosen field width and decimal count. What Format() builds is
// not the output line but the printf format string for it, which is then handed to the release-empty logger
// along with the value -- which is why the popped float is never read again. Both width and decimals are
// clamped, and the two literal families differ only in the one- versus two-digit field of the
// format-string builder.
static inline void ScriptLogFloat(const char* pszFormat, float fValue) { K2_UNUSED(pszFormat); K2_UNUSED(fValue);  }

int CSWVirtualMachineCommands::ExecuteCommandPrintFloat(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    float fValue;
    int nWidth;
    int nDecimals;
    int nDestination;
    if (!g_pVirtualMachine->StackPopFloat(&fValue))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nWidth))
            return VM_ERROR_POP_FAILED;
        if (nWidth >= 18)
            nWidth = 18;
        else if (nWidth < 0)
            nWidth = 0;
    } else
        nWidth = 18;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&nDecimals))
            return VM_ERROR_POP_FAILED;
        if (nDecimals >= 10)
            nDecimals = 9;
        else if (nDecimals < 0)
            nDecimals = 0;
    } else
        nDecimals = 9;
    if (nArgCount >= 4) {
        if (!g_pVirtualMachine->StackPopInteger(&nDestination))
            return VM_ERROR_POP_FAILED;
    } else
        nDestination = 0;
    CExoString sFormat;
    if (nDestination != 0) {
        if (nWidth >= 10)
            sFormat.Format(const_cast<char*>("PRINTFLOAT2: %%%2d.%02df\n"), nWidth, nDecimals);
        else
            sFormat.Format(const_cast<char*>("PRINTFLOAT2: %%%1d.%02df\n"), nWidth, nDecimals);
    } else {
        if (nWidth >= 10)
            sFormat.Format(const_cast<char*>("%%%2d.%02df\n"), nWidth, nDecimals);
        else
            sFormat.Format(const_cast<char*>("%%%1d.%02df\n"), nWidth, nDecimals);
    }
    if (bDisableScriptLogging == 0)
        ScriptLogFloat(sFormat.CStr(), fValue);
    return 0;
}

// PrintString: writes the script's string to the engine's script log. Destination 1 prefixes it, anything
// else does not. The debug object's own string field is the formatting scratch buffer: Format() builds the
// line into it and the log writer then takes it by reference, which is why the debug
// pointer is passed as both receiver and argument.
int CSWVirtualMachineCommands::ExecuteCommandPrintString(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sString;
    int nDestination;
    if (!g_pVirtualMachine->StackPopString(&sString))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nDestination))
            return VM_ERROR_POP_FAILED;
    } else {
        nDestination = 0;
    }
    if (nDestination == 1) {
        if (bDisableScriptLogging == 0)
            ScriptLogString(sString.CStr());
        g_pExoBase->m_pDebug->m_sScratch.Format(const_cast<char*>("PRINTSTRING: %s\n"), sString.CStr());
    } else {
        if (bDisableScriptLogging == 0)
            ScriptLogString(sString.CStr());
        g_pExoBase->m_pDebug->m_sScratch.Format(const_cast<char*>("%s\n"), sString.CStr());
    }
    if (bDisableScriptLogging == 0) {
        g_pExoBase->m_pDebug->WriteToScriptLogFile(g_pExoBase->m_pDebug->m_sScratch);
        g_pExoBase->m_pDebug->FlushScriptLogFile();
    }
    return 0;
}

// PlaySound: queues the play-sound action on OBJECT_SELF with the sound's resref string as its single
// parameter. Only
// runs inside an action context, and only while the object is still accepting commands. AddAction's receiver
// is the raw object cast, not the object probe -- that call only gates the queue.
int CSWVirtualMachineCommands::ExecuteCommandPlaySound(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sSound;
    if (!g_pVirtualMachine->StackPopString(&sSound))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && pObject->AsSWSObject() != 0 && static_cast<CSWSObject*>(pObject)->m_bCommandable != 0) {
        static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_PLAY_SOUND, ACTION_GROUP_NEW, ACTION_PARAM_STRING, &sSound, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 0;
}

// GetSpellSaveDC: the DC of the spell OBJECT_SELF is currently resolving. A creature answers for its own
// in-flight cast; an area-of-effect object answers for its creator's cast of the effect's spell, and falls
// back to the area's own recorded DC when that creator is gone. Everything else answers 14.
int CSWVirtualMachineCommands::ExecuteCommandGetSpellSaveDC(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObject* pObject;
    int nDC = 14;
    int nSpellId = 0;
    CSWSCreature* pCaster = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSCreature() != 0) {
            pCaster = pObject->AsSWSCreature();
            if (pCaster != 0)
                nSpellId = pCaster->m_castSpellId;
        } else if (pObject->AsSWSAreaOfEffectObject() != 0) {
            pCaster = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pObject->AsSWSAreaOfEffectObject()->m_oidCreator);
            if (pCaster == 0)
                nDC = pObject->AsSWSAreaOfEffectObject()->m_nSpellSaveDC;
            nSpellId = pObject->AsSWSAreaOfEffectObject()->GetEffectSpellId();
        }
    }
    if (pCaster != 0)
        nDC = pCaster->CalculateSpellSaveDC(nSpellId);
    if (!g_pVirtualMachine->StackPushInteger(nDC))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// GiveItem: hands an item to another object. The three possible recipients each have their own acquire call
// -- only the creature's takes a give-to id (the command always passes OBJECT_INVALID), and a container item
// has to prove it really is a container first.
int CSWVirtualMachineCommands::ExecuteCommandGiveItem(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidTarget;
    unsigned long long oidItem;
    CSWSItem* pItem;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pTarget != 0 && pItem != 0) {
        if (pTarget->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
            pTarget->AsSWSCreature()->AcquireItem(&pItem, pItem->m_possessorId, OBJECT_INVALID, 0);
        } else if (pTarget->m_objectType == GAME_OBJECT_TYPE_ITEM) {  // a container item
            CSWRules* pRules = g_pRules;
            int nBaseItem = pTarget->AsSWSItem()->m_baseItem;
            if (pRules->GetBaseItem(nBaseItem)->m_bContainer != 0)
                pTarget->AsSWSItem()->AcquireItem(&pItem, pItem->m_possessorId, 0);
        } else if (pTarget->m_objectType == GAME_OBJECT_TYPE_PLACEABLE) {
            pTarget->AsSWSPlaceable()->AcquireItem(&pItem, pItem->m_possessorId, 0);
        }
    }
    return 0;
}

// GetItemActivated (439) / GetItemActivator (440) / GetItemActivatedTargetLocation (441) /
// GetItemActivatedTarget (442): the OnActivateItem event's four accessors. Three answer an object id off the
// module record; 441 builds a location whose facing points from the activator to the target point, and only
// when the two are actually different points.
int CSWVirtualMachineCommands::ExecuteCommandItemActivated(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (nCommand == CMD_GetItemActivatedTargetLocation) {
        // The push result is bound, then the location is destroyed, then the result is tested.
        int bPushed;
        {
        CScriptLocation sLocation;
        if (pModule != 0) {
            CSWSCreature* pActivator = g_pAppManager->m_pServerExoApp
                    ->GetCreatureByGameObjectID(pModule->m_oidItemActivator);
            sLocation.m_vPosition =
                    Vector(pModule->m_fActivateTargetX, pModule->m_fActivateTargetY,
                           pModule->m_fActivateTargetZ);
            if (pActivator != 0 && pActivator->m_position != sLocation.m_vPosition)
                sLocation.m_vOrientation =
                        normalize(sLocation.m_vPosition - pActivator->m_position);
        }
        bPushed = g_pVirtualMachine->StackPushEngineStructure(ENGINE_STRUCTURE_LOCATION, &sLocation);
        }
        if (bPushed == 0)
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    unsigned long long oidObject = OBJECT_INVALID;
    if (pModule != 0) {
        if (nCommand == CMD_GetItemActivated)
            oidObject = pModule->m_oidActivatedItem;
        else if (nCommand == CMD_GetItemActivator)
            oidObject = pModule->m_oidItemActivator;
        else if (nCommand == CMD_GetItemActivatedTarget)
            oidObject = pModule->m_oidItemActivateTargetObject;
    }
    if (!g_pVirtualMachine->StackPushObject(oidObject))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Handles the location/point movement actions. Both ids that reach this handler (21 =
// ActionMoveToLocation, 382 = ActionForceMoveToLocation) queue the same move-to-point action; only
// 382 carries nwscript's 30-second timeout default, which is why the seed pair is stored ahead of
// everything and why the run flag's pop is guarded by an argument count rather than the id. The 21
// arm passes a literal 0.0f for the timeout rather than the seeded local.
int CSWVirtualMachineCommands::ExecuteCommandMoveToPoint(int nCommand, int nArgCount)
{
    CScriptLocation* pLocation;
    int bRun = 0;
    float fTimeout = 30.0f;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&bRun))
            return VM_ERROR_POP_FAILED;
        if (nCommand == CMD_ActionForceMoveToLocation && nArgCount >= 3) {
            if (!g_pVirtualMachine->StackPopFloat(&fTimeout))
                return VM_ERROR_POP_FAILED;
        }
    }
    if (m_nScriptRunArg == 1) {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
                    == CGameObjectArray::SUCCESS
                && pObject->AsSWSCreature() != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea());
            if (pArea != 0) {
                unsigned long long oidInvalid = CGameObjectArray::INVALIDOBJID;
                if (nCommand == CMD_ActionMoveToLocation)
                    pObject->AsSWSCreature()->AddMoveToPointAction(
                        ACTION_GROUP_NEW, pLocation->m_vPosition, pArea->m_objectId,
                        oidInvalid, bRun, 0.0f, 0.0f,
                        0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
                else if (nCommand == CMD_ActionForceMoveToLocation)
                    pObject->AsSWSCreature()->AddMoveToPointAction(
                        ACTION_GROUP_NEW, pLocation->m_vPosition, pArea->m_objectId,
                        oidInvalid, bRun, 0.0f, fTimeout,
                        0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
            }
        }
    }
    // No null-out after the delete here, because the return is a single block.
    if (pLocation != 0)
        delete pLocation;
    return 0;
}

// The object twin of MoveToPoint. 17 is ActionMoveToObject and 383 ActionForceMoveToObject, which
// defaults the timeout to 30 seconds. The action node carries the range in both the third and the
// fourth parameter slots and never passes the timeout.
int CSWVirtualMachineCommands::ExecuteCommandMoveToObject(int nCommand, int nArgCount)
{
    unsigned long long oidTarget;
    int bRun = 0;
    float fUseRange = 0.5f;
    float fRange = 1.0f;
    float fTimeout = 0.0f;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&bRun))
            return VM_ERROR_POP_FAILED;
        if (nArgCount >= 3 && !g_pVirtualMachine->StackPopFloat(&fRange))
            return VM_ERROR_POP_FAILED;
    }
    if (nCommand == CMD_ActionForceMoveToObject) {
        if (nArgCount >= 4) {
            if (!g_pVirtualMachine->StackPopFloat(&fTimeout))
                return VM_ERROR_POP_FAILED;
        } else {
            fTimeout = 30.0f;
        }
    }
    CGameObject* pObject;
    CGameObject* pTarget;
    if (m_nScriptRunArg == 1
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
               == CGameObjectArray::SUCCESS
        && g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pTarget)
               == CGameObjectArray::SUCCESS
        && pObject->AsSWSCreature() != 0
        && pObject->AsSWSCreature()->m_bCommandable != 0) {
        Vector vUsePosition = Vector();
        pObject->AsSWSCreature()->GetUseRange(oidTarget, vUsePosition, fUseRange, 0, 0);
        fRange = fRange > fUseRange ? fRange : fUseRange;
        Vector vTargetPosition = static_cast<CSWSObject*>(pTarget)->m_position;
        CSWSArea* pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pTarget)->GetArea());
        CResRef resRef;
        if (pArea != 0) {
            unsigned long long oidArea = pArea->m_objectId;
            int nOne = 1;
            pObject->AsSWSCreature()->AddMoveToPointAction(ACTION_GROUP_NEW, vTargetPosition, oidArea,
                    oidTarget, bRun, fRange, fTimeout,
                    0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
            static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, ACTION_GROUP_PREVIOUS, ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &bRun,
                    ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_INT, &nOne,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            if (pObject->AsSWSPlaceable() != 0
                && pObject->AsSWSPlaceable()->m_bPreciseUse != 0)
                static_cast<CSWSObject*>(pObject)->AddAction(AIACTION_CHANGE_FACING_OBJECT, ACTION_GROUP_PREVIOUS, ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
    }
    return 0;
}

// Whether the area can see the target. When no area can be resolved the handler pushes false and then
// continues into the line-of-sight test with a null receiver, so a self-object that is neither an area
// nor a creature, door or placeable double-pushes and then dereferences null.
int CSWVirtualMachineCommands::ExecuteCommandHasLineOfSight(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    Vector vSource = Vector();
    Vector vTarget = Vector();
    unsigned long long oidSource;
    unsigned long long oidTarget;
    if (!g_pVirtualMachine->StackPopVector(&vSource))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopVector(&vTarget))
        return VM_ERROR_POP_FAILED;
    if (m_nScriptRunArg == 0) {
        if (!g_pVirtualMachine->StackPushInteger(0))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    if (!g_pVirtualMachine->StackPopObject(&oidSource))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = 0;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_oidSelf, &pObject)
            != CGameObjectArray::SUCCESS) {
        if (!g_pVirtualMachine->StackPushInteger(0))
            return VM_ERROR_PUSH_FAILED;
        return 0;
    }
    CSWSArea* pArea = 0;
    if (pObject->AsSWSArea() != 0)
        pArea = pObject->AsSWSArea();
    else if (pObject->AsSWSCreature() != 0)
        pArea = static_cast<CSWSArea*>(pObject->AsSWSCreature()->GetArea());
    else if (pObject->AsSWSDoor() != 0)
        pArea = static_cast<CSWSArea*>(pObject->AsSWSDoor()->GetArea());
    else if (pObject->AsSWSPlaceable() != 0)
        pArea = static_cast<CSWSArea*>(pObject->AsSWSPlaceable()->GetArea());
    if (pArea == 0) {
        if (!g_pVirtualMachine->StackPushInteger(0))
            return VM_ERROR_PUSH_FAILED;
        // Falls through into the clear-line-of-sight call; see the note above.
    }
    unsigned long long oidHit = OBJECT_INVALID;
    Vector vHit = Vector();
    int bClear = pArea->ClearLineOfSight(vSource, vTarget, &vHit, &oidHit,
                                         oidSource, oidTarget, 0, 0);
    if (!g_pVirtualMachine->StackPushInteger(bClear))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Handles the two floating-text broadcasts; 525 pops a talk-table strref, 526 a literal string. The
// string local is constructed for both ids and destroyed on every exit. The third argument defaults
// to true, and note the inversion: a true value sends nothing here, and only the personal case
// reaches the per-player message -- the faction broadcast is handled by the client on the sub-type.
int CSWVirtualMachineCommands::ExecuteCommandFloatingText(int nCommand, int nArgCount)
{
    CExoString sText;
    unsigned long long oidCreature;
    // The strref is popped into one local and immediately copied into a second, because the pop target is
    // address-taken.
    int nPoppedStrRef;
    int bBroadcastToFaction = 1;
    int nStrRef K2_UNSET;
    if (nCommand == CMD_FloatingTextStrRefOnCreature) {
        if (!g_pVirtualMachine->StackPopInteger(&nPoppedStrRef))
            return VM_ERROR_POP_FAILED;
        nStrRef = nPoppedStrRef;
    } else if (nCommand == CMD_FloatingTextStringOnCreature) {
        if (!g_pVirtualMachine->StackPopString(&sText))
            return VM_ERROR_POP_FAILED;
    }
    if (!g_pVirtualMachine->StackPopObject(&oidCreature))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&bBroadcastToFaction))
            return VM_ERROR_POP_FAILED;
    }
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreature);
    if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE
            && pObject->AsSWSObject() != 0) {
        CSWCCMessageData* pMsg = new CSWCCMessageData;
        pMsg->SetObjectID(0, oidCreature);
        if (nCommand == CMD_FloatingTextStrRefOnCreature) {
            pMsg->SetInteger(9, FEEDBACK_FLOATING_TEXT_STRREF);
            pMsg->SetInteger(0, nStrRef);
        } else if (nCommand == CMD_FloatingTextStringOnCreature) {
            pMsg->SetInteger(9, FEEDBACK_FLOATING_TEXT_STRING);
            pMsg->SetString(0, sText);
        }
        if (bBroadcastToFaction == 0) {
            CSWSPlayer* pPlayer =
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(oidCreature);
            if (pPlayer)
                g_pAppManager->m_pServerExoApp->GetSWSMessage()
                    ->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_FEEDBACK, pMsg, 0);
        }
        delete pMsg;
    }
    return 0;
}

// Applies an effect to a target with the given duration type. The duration type is written into the low
// three bits of the effect's bitfield: temporary is the only arm that also stores the duration, and the
// instantaneous arm just clears the field. Ownership is split: a target that cannot be resolved deletes
// the effect here, a target whose object cast is null leaks it, and on the success path the target's
// apply call takes ownership.
int CSWVirtualMachineCommands::ExecuteCommandApplyEffectOnObject(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    int nDurationType;
    void* pEngineStruct;
    unsigned long long oidTarget;
    float fDuration;
    if (!g_pVirtualMachine->StackPopInteger(&nDurationType))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_EFFECT, &pEngineStruct))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopObject(&oidTarget))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopFloat(&fDuration))
        return VM_ERROR_POP_FAILED;
    // The cast lives below every pop, because the pop target is address-taken.
    CGameEffect* pEffect = static_cast<CGameEffect*>(pEngineStruct);
    switch (nDurationType) {
        case DURATION_TYPE_INSTANT:
            pEffect->m_durationTypeBits = pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK;
            break;
        case DURATION_TYPE_TEMPORARY:
            pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | DURATION_TYPE_TEMPORARY;
            pEffect->m_fDuration = fDuration;
            break;
        default:  // DURATION_TYPE_PERMANENT
            pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | DURATION_TYPE_PERMANENT;
            break;
    }
    CGameObject* pTarget;
    if (pArray->GetGameObject(oidTarget, &pTarget) != CGameObjectArray::SUCCESS || pTarget == 0) {
        delete pEffect;
        return 0;
    }
    CGameObject* pCreator;
    if (pArray->GetGameObject(pEffect->m_creatorId, &pCreator) == CGameObjectArray::SUCCESS
            && pCreator != 0 && pCreator->m_objectType == GAME_OBJECT_TYPE_CREATURE
            && pCreator->AsSWSCreature()->m_bItemCastSpell != 0)
        pEffect->m_nCasterLevel = pCreator->AsSWSCreature()->m_nItemCastSpellLevel;
    if (pTarget->AsSWSObject() != 0) {
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LINK)
            pTarget->AsSWSObject()->m_bApplyingLinkEffect = 1;
        pTarget->AsSWSObject()->ApplyEffect(pEffect, 0, 1);
        pTarget->AsSWSObject()->m_bApplyingLinkEffect = 0;
    }
    return 0;
}

// CreateItemOnFloor(template, location, bUseAppearAnimation). The third argument is popped and thrown
// away, so the appear animation never happens on this path. The item is built by hand (allocate, load the
// template, set position and orientation, add to area) rather than through a factory, and the client is told
// about it with a direct message send whose player id is the invalid-object sentinel.
int CSWVirtualMachineCommands::ExecuteCommandCreateItemOnFloor(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoString sTemplate;
    CScriptLocation* pLocation;
    int bUseAppearAnimation = 0;
    if (!g_pVirtualMachine->StackPopString(&sTemplate))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopEngineStructure(ENGINE_STRUCTURE_LOCATION, reinterpret_cast<void**>(&pLocation)))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&bUseAppearAnimation))
        return VM_ERROR_POP_FAILED;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CSWSArea* pArea;
    CSWSItem* pItem;
    unsigned long long oidItem = CGameObjectArray::INVALIDOBJID;
    if (pModule != 0) {
        pArea = pModule->GetArea();
        if (pArea != 0) {
            {
                CResRef resRef(static_cast<char*>(sTemplate.CStr()));
                pItem = new CSWSItem(OBJECT_INVALID);
                pItem->LoadFromTemplate(resRef);
                pItem->SetPosition(pLocation->m_vPosition, 1, 1, 0);
                pItem->SetOrientation(pLocation->m_vOrientation);
                pItem->AddToArea(pArea, pLocation->m_vPosition.x, pLocation->m_vPosition.y,
                                 pLocation->m_vPosition.z, 1);
            }
            oidItem = pItem->m_objectId;
            if (pMessage != 0)
                pMessage->SendServerToPlayerItemScriptAdd(static_cast<unsigned int>(CGameObjectArray::INVALIDOBJID), pArea,
                                                          pItem, pLocation->m_vPosition);
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem)) {
        if (pLocation != 0) {
            delete pLocation;
            pLocation = 0;
        }
        return VM_ERROR_PUSH_FAILED;
    }
    if (pLocation != 0) {
        delete pLocation;
        pLocation = 0;
    }
    return 0;
}

// GetJournalQuestExperience(plot id). It re-reads global.jrl from disk on every call -- a fresh reader, a
// linear scan of the categories list comparing each entry's tag against the argument, and a delete of the
// whole reader at the end. The invalid-data arm jumps straight to the push and leaks the reader.
int CSWVirtualMachineCommands::ExecuteCommandGetJournalQuestExperience(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    CExoString sPlotId;
    CResList categories;
    CResStruct topLevel;
    CResStruct category;
    CResGFF* pGff;
    int nCount;
    int nXP = 0;
    int bSuccess = 1;
    if (!g_pVirtualMachine->StackPopString(&sPlotId))
        return VM_ERROR_POP_FAILED;
    pGff = new CResGFF(RESTYPE_JRL, const_cast<char*>("JRL "), CResRef("Global"));
    if (pGff->m_nValid != 0) {
        pGff->GetTopLevelStruct(&topLevel);
        pGff->GetList(&categories, &topLevel, const_cast<char*>("Categories"));
        nCount = pGff->GetListCount(&categories);
        for (int i = 0; i < nCount; i++) {
            pGff->GetListElement(&category, &categories, i);
            if (pGff->ReadFieldCExoString(&category, const_cast<char*>("Tag"), bSuccess, CExoString("")) == sPlotId) {
                nXP = pGff->ReadFieldDWORD(&category, const_cast<char*>("XP"), bSuccess, 0);
                break;
            }
        }
        delete pGff;
    }
    if (!g_pVirtualMachine->StackPushInteger(nXP))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// SetItemStackSize(oItem, nSize). The requested size is clamped into [1, the base item's stacking column],
// and a no-op change returns early -- the feedback message and the GUI status are only produced for a real
// change, and only when the possessor is a party member. The message object is allocated and never deleted on
// any normal path, so every stack-size change on a party item leaks it.
int CSWVirtualMachineCommands::ExecuteCommandSetItemStackSize(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand); K2_UNUSED(nArgCount);
    unsigned long long oidItem;
    int nStackSize = 0;
    if (!g_pVirtualMachine->StackPopObject(&oidItem))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nStackSize))
        return VM_ERROR_POP_FAILED;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0)
        return 0;
    int nMaxStack = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nStacking;
    if (nStackSize > nMaxStack)
        nStackSize = nMaxStack;
    if (nStackSize < 1)
        nStackSize = 1;
    int nOldStackSize = pItem->m_stackCount;
    if (nStackSize != nOldStackSize) {
        pItem->m_stackCount = nStackSize;
        CSWSCreature* pPossessor =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pItem->m_possessorId);
        CSWCCMessageData* pMsg = new CSWCCMessageData;
        pMsg->SetObjectID(0, pItem->m_objectId);
        pMsg->SetInteger(0, 1);
        if (nStackSize > nOldStackSize) {
            if (pPossessor != 0 && pPossessor->m_bInParty != 0) {
                pPossessor->SendFeedbackMessage(FEEDBACK_ITEM_ACQUIRED, pMsg);
#if !KOTOR1  // K1 has no received-item name list
                g_pAppManager->m_pClientExoApp->GetInGameGui()->AddReceivedItemToList(
                    pItem->m_localizedName);
#endif
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_RECEIVED, 0);
            }
        } else {
            if (pPossessor != 0 && pPossessor->m_bInParty != 0) {
                pPossessor->SendFeedbackMessage(FEEDBACK_ITEM_LOST, pMsg);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_LOST, 0);
            }
        }
    }
    return 0;
}

// Two commands share this handler. 10 = SetFacing(degrees) turns the degrees into a unit vector,
// 143 = SetFacingPoint(vTarget) aims at a point by normalising the difference from the object's
// position. Either way the new orientation is only committed when it is well defined. Committing
// also clears the orientation lock on both sides, the client one through ServerToClientObjectId,
// otherwise the lock would immediately overwrite the new facing.
int CSWVirtualMachineCommands::ExecuteCommandSetFacing(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    Vector vOrientation = Vector();
    CSWSObject* pObject;
    int bHaveOrientation = 0;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    Vector vTarget = Vector();
    if (nCommand == CMD_SetFacing) {
        float fDirection;
        if (!g_pVirtualMachine->StackPopFloat(&fDirection))
            return VM_ERROR_POP_FAILED;
        float fRadians = fDirection * 0.017453292f;
        vOrientation.x = cosf(fRadians);
        vOrientation.y = sinf(fRadians);
        vOrientation.z = 0.0f;
        bHaveOrientation = 1;
    } else if (nCommand == CMD_SetFacingPoint) {
        if (!g_pVirtualMachine->StackPopVector(&vTarget))
            return VM_ERROR_POP_FAILED;
    }
    if (pArray->GetGameObject(m_oidSelf, reinterpret_cast<CGameObject**>(&pObject)) == CGameObjectArray::SUCCESS
            && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
        if (nCommand == CMD_SetFacingPoint) {
            Vector vDelta;
            vDelta.x = vTarget.x - pObject->m_position.x;
            vDelta.y = vTarget.y - pObject->m_position.y;
            vDelta.z = vTarget.z - pObject->m_position.z;
            if (vDelta.x != 0.0f || vDelta.y != 0.0f || vDelta.z != 0.0f) {
                vOrientation = normalize(vDelta);
                bHaveOrientation = 1;
            }
        }
        if (m_nScriptRunArg == 1 && bHaveOrientation != 0) {
            // The inner call must be its own statement, so the outer receiver is re-loaded after it.
            unsigned long long oidClient =
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pObject->m_objectId);
            CGameObject* pClientObject =
                g_pAppManager->m_pClientExoApp->GetGameObject(oidClient);
            if (pClientObject != 0 && pClientObject->AsSWCCreature() != 0)
                pClientObject->AsSWCCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
            pObject->SetLockOrientationToObject(OBJECT_INVALID);
            pObject->SetOrientation(vOrientation);
        }
    }
    return 0;
}

// AddJournalQuestEntry(plot id, state, bAllowOverrideHigher). Every connected player's own journal is
// stamped independently, and the last entry's quest XP wins the single party-wide award. A negative state is
// folded to its absolute value first, and the no-player-list path returns 1 rather than 0.
int CSWVirtualMachineCommands::ExecuteCommandAddJournalQuestEntry(int nCommand, int nArgCount)
{
    K2_UNUSED(nCommand);
    CExoString sPlotId;
    int nState;
    CExoLinkedList<CSWSClient>* ppPlayers;
    CExoLinkedListNode* pNodePos;
    unsigned int nWorldDay;
    unsigned int nWorldTime;
    int nQuestXP = 0;
    int bAllowOverrideHigher = 0;
    if (!g_pVirtualMachine->StackPopString(&sPlotId))
        return VM_ERROR_POP_FAILED;
    if (!g_pVirtualMachine->StackPopInteger(&nState))
        return VM_ERROR_POP_FAILED;
    if (nArgCount >= 3) {
        if (!g_pVirtualMachine->StackPopInteger(&bAllowOverrideHigher))
            return VM_ERROR_POP_FAILED;
    }
    if (nState < 0)
        nState = -nState;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nWorldDay, &nWorldTime);
    ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppPlayers == 0)
        return 1;
    pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0
                    && (bAllowOverrideHigher != 0
                        || pCreature->GetJournal()->GetState(sPlotId) < nState)) {
                pCreature->GetJournal()->SetDate(sPlotId, nWorldDay, 0);
                pCreature->GetJournal()->SetTime(sPlotId, nWorldTime, 0);
                pCreature->GetJournal()->SetState(sPlotId, nState, 0);
                nQuestXP = pCreature->GetJournal()->GetQuestEntryXP(sPlotId);
            }
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    if (nQuestXP != 0 && g_pAppManager->m_pServerExoApp->GetPartyTable() != 0) {
        g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nQuestXP, 1);
#if !KOTOR1  // K1 has no quest-XP status line
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, nQuestXP);
#endif
    }
    return 0;
}

// Handles the numeric command group, where 77 is the integer abs and 67..76 the float intrinsics.
// sin, cos and tan take their argument in degrees, acos, asin and atan return degrees.
// Out-of-domain arguments leave the result at 0 rather than producing a NaN or an infinity.

int CSWVirtualMachineCommands::ExecuteCommandMath(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    if (nCommand == CMD_abs) {  // integer
        int nValue;
        if (!g_pVirtualMachine->StackPopInteger(&nValue))
            return VM_ERROR_POP_FAILED;
        if (!g_pVirtualMachine->StackPushInteger(abs(nValue)))
            return VM_ERROR_PUSH_FAILED;
    }
    else {
        float fValue;
        float fExponent;
        if (!g_pVirtualMachine->StackPopFloat(&fValue))
            return VM_ERROR_POP_FAILED;
        float fResult = 0.0f;
        switch (nCommand) {
        case CMD_fabs:
            fResult = fabsf(fValue);
            break;
        case CMD_cos:  // argument in degrees
            fResult = static_cast<float>(cos(static_cast<double>(fValue) * 0.017453292f));
            break;
        case CMD_sin:  // argument in degrees
            fResult = static_cast<float>(sin(static_cast<double>(fValue) * 0.017453292f));
            break;
        case CMD_tan:  // argument in degrees
            fResult = static_cast<float>(tan(static_cast<double>(fValue) * 0.017453292f));
            break;
        case CMD_acos:  // result in degrees
            if (fabsf(fValue) <= 1.0f)
                fResult = static_cast<float>(acos(static_cast<double>(fValue))) * 57.295776f;
            break;
        case CMD_asin:  // result in degrees
            if (fabsf(fValue) <= 1.0f)
                fResult = static_cast<float>(asin(static_cast<double>(fValue))) * 57.295776f;
            break;
        case CMD_atan:  // result in degrees
            fResult = static_cast<float>(atan(static_cast<double>(fValue))) * 57.295776f;
            break;
        case CMD_log:
            if (fValue > 0.0f)
                fResult = static_cast<float>(log(static_cast<double>(fValue)));
            break;
        case CMD_pow:
            if (!g_pVirtualMachine->StackPopFloat(&fExponent))
                return VM_ERROR_POP_FAILED;
            if (fValue != 0.0f || fExponent >= 0.0f)
                fResult = static_cast<float>(pow(static_cast<double>(fValue), static_cast<double>(fExponent)));
            break;
        case CMD_sqrt:
            fResult = fValue > 0.0f ? sqrtf(fValue) : 0.0f;
            break;
        }
        if (!g_pVirtualMachine->StackPushFloat(fResult))
            return VM_ERROR_PUSH_FAILED;
    }
    return 0;
}

// The shared GetItemInInventory/GetNextItemInInventory implementation, a depth-first cursor over
// nested item repositories, keeping a saved-state triple on the container (the top-level index, the
// descended index and the last returned item's id, which the next call re-fetches to decide whether
// to descend). A store has no walk and only answers the invalid object id.

int CSWVirtualMachineCommands::ExecuteCommandGetItemInInventory(int nCommand, int nArgCount)
{
    K2_UNUSED(nArgCount);
    unsigned long long oidObject;
    if (!g_pVirtualMachine->StackPopObject(&oidObject))
        return VM_ERROR_POP_FAILED;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidObject);
    unsigned long long oidItem = OBJECT_INVALID;
    if (pObject != 0) {
        CItemRepository* pRepo = 0;
        CItemRepository* pCursor = 0;
        unsigned int nIndex = 0;
        if (pObject->AsSWSCreature() != 0) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            pRepo = pCreature->GetItemRepository(1);
            if (nCommand == CMD_GetFirstItemInInventory) {
                nIndex = 0;
                pCreature->m_nItemIteratorIndex = 1;
                pCreature->m_nContainerItemIteratorIndex = 0;
                pCreature->m_oidItemIteratorContainer = CGameObjectArray::INVALIDOBJID;
                pCursor = pRepo;
            }
            else {
                CSWSItem* pPrev = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pCreature->m_oidItemIteratorContainer);
                if (pPrev != 0 && pPrev->m_possessorId == pCreature->m_objectId) {
                    nIndex = pCreature->m_nContainerItemIteratorIndex;
                    pCursor = pPrev->m_pItemRepository;
                    pCreature->m_nContainerItemIteratorIndex = static_cast<unsigned short>(nIndex + 1);
                }
                else {
                    nIndex = pCreature->m_nItemIteratorIndex;
                    pCreature->m_nContainerItemIteratorIndex = 0;
                    pCreature->m_oidItemIteratorContainer = CGameObjectArray::INVALIDOBJID;
                    pCreature->m_nItemIteratorIndex = static_cast<unsigned short>(nIndex + 1);
                    pCursor = pRepo;
                }
            }
        }
        else if (pObject->AsSWSPlaceable() != 0) {
            CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
            pRepo = pPlaceable->m_pItemRepository;
            if (nCommand == CMD_GetFirstItemInInventory) {
                nIndex = 0;
                pPlaceable->m_nInventoryWalkIndex = 1;
                pPlaceable->m_nInventoryWalkSubIndex = 0;
                pPlaceable->m_oidInventoryWalkItem = CGameObjectArray::INVALIDOBJID;
                pCursor = pRepo;
            }
            else {
                CSWSItem* pPrev = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pPlaceable->m_oidInventoryWalkItem);
                if (pPrev != 0 && pPrev->m_possessorId == pPlaceable->m_objectId) {
                    nIndex = pPlaceable->m_nInventoryWalkSubIndex;
                    pCursor = pPrev->m_pItemRepository;
                    pPlaceable->m_nInventoryWalkSubIndex = static_cast<unsigned short>(nIndex + 1);
                }
                else {
                    nIndex = pPlaceable->m_nInventoryWalkIndex;
                    pPlaceable->m_nInventoryWalkSubIndex = 0;
                    pPlaceable->m_oidInventoryWalkItem = CGameObjectArray::INVALIDOBJID;
                    pPlaceable->m_nInventoryWalkIndex = static_cast<unsigned short>(nIndex + 1);
                    pCursor = pRepo;
                }
            }
        }
        else if (pObject->AsSWSItem() != 0) {
            pCursor = pObject->AsSWSItem()->m_pItemRepository;
            if (nCommand == CMD_GetFirstItemInInventory)
                nIndex = 0;
            else
                nIndex = pObject->AsSWSItem()->m_nInventoryWalkIndex;
            pObject->AsSWSItem()->m_nInventoryWalkIndex = static_cast<unsigned short>(nIndex + 1);
            pRepo = 0;
        }
        else if (pObject->AsSWSStore() != 0) {
            CSWSStore* pStore = pObject->AsSWSStore();
            if (nCommand == CMD_GetFirstItemInInventory)
                pStore->m_nInventoryWalkIndex = 1;
            else
                pStore->m_nInventoryWalkIndex += 1;
        }
        if (pCursor != 0) {
            if ((pCursor != pRepo) & (pCursor->ItemListGetCount() <= nIndex)) {
                if (pObject->AsSWSCreature() != 0) {
                    nIndex = pObject->AsSWSCreature()->m_nItemIteratorIndex;
                    pObject->AsSWSCreature()->m_nItemIteratorIndex = static_cast<unsigned short>(nIndex + 1);
                    pObject->AsSWSCreature()->m_oidItemIteratorContainer = CGameObjectArray::INVALIDOBJID;
                    pObject->AsSWSCreature()->m_nContainerItemIteratorIndex = 0;
                }
                else if (pObject->AsSWSPlaceable() != 0) {
                    nIndex = pObject->AsSWSPlaceable()->m_nInventoryWalkIndex;
                    pObject->AsSWSPlaceable()->m_nInventoryWalkIndex = static_cast<unsigned short>(nIndex + 1);
                    pObject->AsSWSPlaceable()->m_oidInventoryWalkItem = CGameObjectArray::INVALIDOBJID;
                    pObject->AsSWSPlaceable()->m_nInventoryWalkSubIndex = 0;
                }
            }
            else
                pRepo = pCursor;
            if (pRepo->ItemListGetCount() > nIndex) {
                CSWSItem* pItem = pRepo->ItemListGetItem(nIndex);
                if (pItem != 0) {
                    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
                    if (pBaseItem != 0 && pBaseItem->m_bContainer != 0) {
                        if (pObject->AsSWSCreature() != 0) {
                            pObject->AsSWSCreature()->m_oidItemIteratorContainer = pItem->m_objectId;
                            pObject->AsSWSCreature()->m_nContainerItemIteratorIndex = 0;
                        }
                        else if (pObject->AsSWSPlaceable() != 0) {
                            pObject->AsSWSPlaceable()->m_oidInventoryWalkItem = pItem->m_objectId;
                            pObject->AsSWSPlaceable()->m_nInventoryWalkSubIndex = 0;
                        }
                    }
                    oidItem = pItem->m_objectId;
                }
            }
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidItem))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

// Resolves the area (from the popped area object, else from the area the script owner is standing in),
// then walks its object list for the first entry whose engine type is enabled in the 16-bit type mask,
// which defaults to all types when the argument is omitted. The walk keeps a cursor: a failed step
// stores the invalid object id into it and the loop's own lookup miss breaks out, so the cursor ends at
// the last match or at the invalid id, which is exactly what gets pushed.
int CSWVirtualMachineCommands::ExecuteCommandGetObjectInArea(int nCommand, int nArgCount)
{
    CExoString sTemp;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    unsigned long long oidFound = CGameObjectArray::INVALIDOBJID;
    CSWSArea* pArea = 0;
    unsigned long long oidArea;
    CGameObject* pObject;
    int nType;
    if (nArgCount >= 1) {
        if (!g_pVirtualMachine->StackPopObject(&oidArea))
            return VM_ERROR_POP_FAILED;
        if (pObjectArray->GetGameObject(oidArea, &pObject) == CGameObjectArray::SUCCESS
                && pObject != 0)
            pArea = pObject->AsSWSArea();
    }
    if (nArgCount >= 2) {
        if (!g_pVirtualMachine->StackPopInteger(&nType))
            return VM_ERROR_POP_FAILED;
    } else
        nType = OBJECT_TYPE_ALL;
    if (pArea == 0) {
        if (pObjectArray->GetGameObject(m_oidSelf, &pObject) == CGameObjectArray::SUCCESS
                && pObject != 0) {
            if (pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA)
                pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pObject)->GetArea());
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
                pArea = pObject->AsSWSArea();
        }
    }
    if (pArea != 0) {
        if (nCommand == CMD_GetFirstObjectInArea) {
            if (!pArea->GetFirstObjectInArea(oidFound))
                oidFound = CGameObjectArray::INVALIDOBJID;
        } else {
            if (!pArea->GetNextObjectInArea(oidFound))
                oidFound = CGameObjectArray::INVALIDOBJID;
        }
        for (;;) {
            CGameObject* pFound;
            if (pObjectArray->GetGameObject(oidFound, &pFound) != CGameObjectArray::SUCCESS)
                break;
            if (pFound->m_objectType == 0)
                break;
            if ((nType & OBJECT_TYPE_CREATURE) && pFound->m_objectType == GAME_OBJECT_TYPE_CREATURE)
                break;
            if ((nType & OBJECT_TYPE_ITEM) && pFound->m_objectType == GAME_OBJECT_TYPE_ITEM)
                break;
            if ((nType & OBJECT_TYPE_TRIGGER) && pFound->m_objectType == GAME_OBJECT_TYPE_TRIGGER)
                break;
            if ((nType & OBJECT_TYPE_DOOR) && pFound->m_objectType == GAME_OBJECT_TYPE_DOOR)
                break;
            if ((nType & OBJECT_TYPE_AREA_OF_EFFECT) && pFound->m_objectType == GAME_OBJECT_TYPE_AREA_OF_EFFECT)
                break;
            if ((nType & OBJECT_TYPE_WAYPOINT) && pFound->m_objectType == GAME_OBJECT_TYPE_WAYPOINT)
                break;
            if ((nType & OBJECT_TYPE_PLACEABLE) && pFound->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
                break;
            if ((nType & OBJECT_TYPE_STORE) && pFound->m_objectType == GAME_OBJECT_TYPE_STORE)
                break;
            if ((nType & OBJECT_TYPE_ENCOUNTER) && pFound->m_objectType == GAME_OBJECT_TYPE_ENCOUNTER)
                break;
            if ((nType & OBJECT_TYPE_SOUND) && pFound->m_objectType == GAME_OBJECT_TYPE_SOUND)
                break;
            if (!pArea->GetNextObjectInArea(oidFound))
                oidFound = CGameObjectArray::INVALIDOBJID;
        }
    }
    if (!g_pVirtualMachine->StackPushObject(oidFound))
        return VM_ERROR_PUSH_FAILED;
    return 0;
}

