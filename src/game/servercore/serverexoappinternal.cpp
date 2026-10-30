#include "k2stdint.h"
#include "serverexoappinternal.h"
#include "CExoLinkedList.h"
#include "CExoArrayListImpl.h"

#include "SWCollisionMesh.h"
#include "swplayercontrol.h"
#include "nwsdoor.h"
#include "nwsencounter.h"
#include "nwsplaceable.h"
#include "nwstrigger.h"
#include "nwsaiaction.h"
#include "nwsscriptevent.h"
#include "factionmanager.h"
#include "nwmessagetypes.h"
#include "restype.h"
#include "include/aurbehavior.h"

// The coin flip ShuffleLoadScreenHints swaps two hints on: half of rand()'s range.
enum {
    RAND_HALF_RANGE = 1 << 30,
};

// Timing: the autosave interval in milliseconds (fifteen minutes), and the period of a player's
// game-object update in the 64-microsecond units of a high-resolution time difference shifted
// right by six (200 ms).
enum {
    AUTOSAVE_INTERVAL_MS          = 900000,
    GAME_OBJECT_UPDATE_INTERVAL   = 3125,
};

// Save sizing: the size assumed for the save directory when it is empty, the size assumed for
// each file of it that will not open and for a module saved with the game, and the free space a
// save needs, in the 16 KiB units of the free byte count shifted right by fourteen (25 MiB).
enum {
    SAVE_DIRECTORY_SIZE_DEFAULT = 3750000,
    SAVE_FILE_SIZE_ESTIMATE     = 2500000,
    SAVE_MIN_FREE_SPACE_16K     = 1600,
};

// The struct id SaveGame tags each creature of the saved party list with.
enum {
    GFF_STRUCT_ID_PARTY_CREATURE = 0xbead,
};

// The netlayer's player limit for a single-player server.
enum {
    MAX_SERVER_PLAYERS = 16,
};

#include <stdio.h>

extern CScriptCompiler* g_pScriptCompiler;
extern CTlkTable* g_pTlkTable;
class CSWTlkTable;
extern CSWTlkTable* g_pSWTlkTable;

int g_bSavePlayerBIC;
int EnablePathValidation;

extern unsigned char g_nClientServerMode;

// GetLocalTime's SYSTEMTIME as eight words: the fields are never read, and the array form occupies
// the stack protector's array slots.
#include "port/win32api.h"
struct SystemTimeWords { unsigned short w[8]; };
static inline void GetLocalTime(SystemTimeWords* pTime)
{
    GetLocalTime(reinterpret_cast<SYSTEMTIME*>(pTime));
}
static inline void SystemTimeToFileTime(const SystemTimeWords* pSystemTime, unsigned long long* pFileTime)
{
    SystemTimeToFileTime(reinterpret_cast<const SYSTEMTIME*>(pSystemTime), pFileTime);
}
static inline unsigned long long GetLocalFileTime()
{
    SystemTimeWords localTime;
    GetLocalTime(&localTime);
    unsigned long long fileTime;
    SystemTimeToFileTime(&localTime, &fileTime);
    return fileTime;
}

#if KOTOR1  // K1 builds paths with std::string
#include <string>
#include <algorithm>
#include <string.h>
#include "port/k1obb.h"
#endif
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "exostringinline.h"


CServerExoAppInternal* CServerExoAppInternal::s_pServerExoAppInternal;

CServerExoAppInternal::CServerExoAppInternal()
{
    m_nShutdownTimeRemaining = 0;
    m_debugMode = 0;
    m_pServerInfo = 0;
    m_serverMode = 0;
    m_moduleObjectId = CGameObjectArray::INVALIDOBJID;

    m_pPlayerList = new CExoLinkedList<CSWSClient>;

    m_pSysAdminList = new CExoLinkedList<CSWSClient>;

    m_forceUpdate = 0;
    m_pSWSMessage = 0;

    s_pServerExoAppInternal = this;

    m_pPendingAuthIds = new int[16];
    for (int i = 0; i < 16; i++)
        m_pPendingAuthIds[i] = CNetLayer::PLAYERID_INVALIDID;

    m_pauseFlags = 0;
    m_doingStartNewModule = 0;
    m_moveToModulePending = 0;

    m_moveToModuleString = "";

    m_loadingModule = 0;
    m_autoSavePending = 0;
    m_exportAllCharactersPending = 0;
    m_bNeedCharacterPending = 0;

    m_exclusionList1 = new CExoLinkedList<unsigned long long>;

    m_exclusionList2 = new CExoLinkedList<unsigned long long>;

    m_maxSaveDirectorySize = 0;
    m_pcObjectIterPos = 0;

    m_lastConversation = "";

    m_actionConversation = 0;
    m_ignoreConversationStartRange = 0;
    m_loadFromSaveGame = 0;
    m_buttonMashCheck = 0;
    m_lastModuleFoundId = OBJECT_INVALID;
    m_pLastModuleFound = 0;
    m_areaCacheId = OBJECT_INVALID;
    m_moveToPausedTimeOfDay = 0;
    m_moveToPausedCalendarDay = 0;
    m_autoSaveTimerFlag = 0;
    m_autoSaveTimerDay = 0;
    m_autoSaveTimerLastTime = 0;
    m_pCachedArea = 0;
    m_nReserved2 = 0;
    m_nReserved3 = 0;

    m_saveGameName = "";
}

CServerExoAppInternal::~CServerExoAppInternal()
{
    if (m_pPlayerList)
    {
        while (m_pPlayerList->m_pInternal->m_pHead)
        {
            CSWSClient* pData = m_pPlayerList->RemoveHead();
            if (pData)
            {
                delete pData;
            }
        }
        delete m_pPlayerList;
    }
    if (m_pSysAdminList)
    {
        while (m_pSysAdminList->m_pInternal->m_pHead)
        {
            CSWSClient* pData = m_pSysAdminList->RemoveHead();
            if (pData)
            {
                delete pData;
            }
        }
        delete m_pSysAdminList;
    }
    if (m_pPendingAuthIds)
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pPendingAuthIds;
#else
        delete m_pPendingAuthIds;  // array-new'd but scalar-deleted
#endif
    delete m_exclusionList1;
    m_exclusionList1 = NULL;
    delete m_exclusionList2;
    m_exclusionList2 = NULL;

    s_pServerExoAppInternal = NULL;
    g_pAppManager->GetObjectTableManager(1)->ClearAll();
}

void CServerExoAppInternal::UpdateWindowTitle()
{
}

void CServerExoAppInternal::OnCDChange()
{
}

void CServerExoAppInternal::OnExit()
{
}

void CServerExoAppInternal::OnGainFocus()
{
}

void CServerExoAppInternal::OnLostFocus()
{
}

void CServerExoAppInternal::OnVideoChange()
{
}

void CServerExoAppInternal::Uninitialize()
{
}

void CServerExoAppInternal::DebugValidatePosition(unsigned long long a, unsigned long long b, const Vector& c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
}

int CServerExoAppInternal::GetModuleLanguage()
{
    return 0;
}

int CServerExoAppInternal::GetLoadMinGlobalRim()
{
    return 0;
}

// Progress callback handed to CERFFile::ImportFiles; reports a percentage and returns 0.
int UpdateLoadScreen(int current, int total, int rangeStart, int rangeWidth)
{
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(
        static_cast<int>(static_cast<float>(current) / static_cast<float>(total) * static_cast<float>(rangeWidth)) + rangeStart, 0);
    UpdateScreen(0.033333335f, 0, 0);
    return 0;
}

void CServerExoAppInternal::ClearLastModuleFound()
{
#if defined(_MSC_VER) || !defined(__SIZEOF_INT128__)
    m_lastModuleFoundId = OBJECT_INVALID;
    m_pLastModuleFound = 0;
#else
    // raw-offset: the id / pointer pair is cleared as one 16-byte store (two typed stores schedule differently)
    *reinterpret_cast<unsigned __int128*>(&m_lastModuleFoundId) = OBJECT_INVALID;
#endif
}

void CServerExoAppInternal::ClearAreaCache()
{
#if defined(_MSC_VER) || !defined(__SIZEOF_INT128__)
    m_areaCacheId = OBJECT_INVALID;
    m_pCachedArea = 0;
#else
    // raw-offset: the id / pointer pair is cleared as one 16-byte store (two typed stores schedule differently)
    *reinterpret_cast<unsigned __int128*>(&m_areaCacheId) = OBJECT_INVALID;
#endif
}

void CServerExoAppInternal::StartShutdownTimer(uint64_ll_t a, uint64_ll_t b)
{
    m_nShutdownTimeRemaining = b;
    m_nShutdownLastTick = a;
}

void CServerExoAppInternal::SetMoveToWorldTime(unsigned int a, unsigned char b, unsigned char c, unsigned char d,
                                                unsigned short e, unsigned short f, unsigned short g)
{
    m_moveToWorldTimeDate = a;
    m_moveToWorldTimeHour = b;
    m_moveToWorldTimeMinute = c;
    m_moveToWorldTimeSecond = d;
    m_moveToWorldTimeMillisecond = e;
    m_moveToStartSecond = f;
    m_moveToStartMillisecond = g;
}

void CServerExoAppInternal::GetMoveToWorldTime(unsigned int& a, unsigned char& b, unsigned char& c, unsigned char& d,
                                                unsigned short& e, unsigned short& f, unsigned short& g)
{
    a = m_moveToWorldTimeDate;
    b = m_moveToWorldTimeHour;
    c = m_moveToWorldTimeMinute;
    d = m_moveToWorldTimeSecond;
    e = m_moveToWorldTimeMillisecond;
    f = m_moveToStartSecond;
    g = m_moveToStartMillisecond;
}

int CServerExoAppInternal::GetPauseState(unsigned char a)
{
    return (a & m_pauseFlags) != 0;
}

unsigned char CServerExoAppInternal::GetActivePauseState()
{
    return (m_pauseFlags & 2) ? 2 : (m_pauseFlags & 1);
}

CExoLinkedList<unsigned long long>* CServerExoAppInternal::GetActiveExclusionList()
{
    int state = GetActivePauseState();
    if (state == 2)
        return m_exclusionList1;
    if (state == 1)
        return m_exclusionList2;
    return 0;
}

void CServerExoAppInternal::TogglePauseState(unsigned char a)
{
    SetPauseState(a, (m_pauseFlags & a) == 0);
}

void CServerExoAppInternal::SetPauseState(unsigned char a, int b)
{
    CSWSMessage* pMsg = m_pSWSMessage;
    unsigned char flags = m_pauseFlags;
    if (((flags & a) != 0) == b)
        return;

    if (b == 1)
    {
        m_pauseFlags = flags | a;
        static_cast<CWorldTimer*>(m_pWorldTimer)->PauseWorldTimer();
        if (m_pauseFlags & 2)
            static_cast<CWorldTimer*>(m_pTimestopTimer)->PauseWorldTimer();
        pMsg->SendServerToPlayerModule_SetPauseState(a, 1);
    }
    else
    {
        m_pauseFlags = flags & ~a;
        if (a == 2)
        {
            m_pPauseTimer->PauseWorldTimer();
            ReprocessExclusionListActions(2);
            m_pPauseTimer->ResetTimer(static_cast<CWorldTimer*>(m_pTimestopTimer));
            m_pPauseTimer->SetSnapshotTime();
            static_cast<CWorldTimer*>(m_pTimestopTimer)->SetSnapshotTime();
            m_pPauseTimer->ResetLastSnapshotTime();
            static_cast<CWorldTimer*>(m_pTimestopTimer)->ResetLastSnapshotTime();
            if ((m_pauseFlags & 3) != 1)
            {
                static_cast<CWorldTimer*>(m_pWorldTimer)->SetSnapshotTime();
                m_pWorldTimer->ResetLastSnapshotTime();
                static_cast<CWorldTimer*>(m_pWorldTimer)->UnpauseWorldTimer();
            }
            static_cast<CWorldTimer*>(m_pTimestopTimer)->UnpauseWorldTimer();
            m_pPauseTimer->UnpauseWorldTimer();
        }
        else if (a == 1)
        {
            static_cast<CWorldTimer*>(m_pTimestopTimer)->PauseWorldTimer();
            m_pPauseTimer->PauseWorldTimer();
            ReprocessExclusionListActions(1);
            static_cast<CWorldTimer*>(m_pTimestopTimer)->ResetTimer(static_cast<CWorldTimer*>(m_pWorldTimer));
            m_pPauseTimer->ResetTimer(static_cast<CWorldTimer*>(m_pWorldTimer));
            static_cast<CWorldTimer*>(m_pWorldTimer)->SetSnapshotTime();
            static_cast<CWorldTimer*>(m_pTimestopTimer)->SetSnapshotTime();
            m_pPauseTimer->SetSnapshotTime();
            m_pPauseTimer->ResetLastSnapshotTime();
            static_cast<CWorldTimer*>(m_pTimestopTimer)->ResetLastSnapshotTime();
            m_pWorldTimer->ResetLastSnapshotTime();
            static_cast<CWorldTimer*>(m_pWorldTimer)->UnpauseWorldTimer();
            static_cast<CWorldTimer*>(m_pTimestopTimer)->UnpauseWorldTimer();
            m_pPauseTimer->UnpauseWorldTimer();
        }
        pMsg->SendServerToPlayerModule_SetPauseState(a, b);
    }
}

unsigned long long CServerExoAppInternal::ClientToServerObjectId(unsigned long long a)
{
    if (a == OBJECT_INVALID)
        return a;
    const unsigned int nIdBits = ~CGameObjectArray::LISTTYPE_MASK;
    return nIdBits & a;
}

void CServerExoAppInternal::RunDeathSequence()
{
    g_pAppManager->m_pClientExoApp->RunDeathSequence();
}

void CServerExoAppInternal::KillActiveCharacter()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pCreature == NULL)
        return;
    CSWSObject* pServerCreature = pCreature->GetServerCreature();
    if (pServerCreature == NULL)
        return;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(OBJECT_INVALID);
    pEffect->SetInteger(1, 1);
    pServerCreature->ApplyEffect(pEffect, 0, 0);
}

void CServerExoAppInternal::ForceRunAreaOnExitScripts()
{

    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            void* pElem = m_pPlayerList->GetAtPos(pNode);
            CSWSPlayer* pPlayer = static_cast<CSWSClient*>(pElem)->AsSWSPlayer();
            if (pPlayer != 0)
            {
                CGameObject* pGameObj = pPlayer->GetGameObject();
                if (pGameObj != 0)
                {
                    CSWSObject* pServerObj = pGameObj->AsSWSCreature();
                    if (pServerObj != 0)
                    {
                        CSWSArea* pArea = static_cast<CSWSArea*>(pServerObj->GetArea());
                        if (pArea != 0)
                        {
                            pArea->m_onExitScriptCallerId = pServerObj->m_objectId;
                            g_pVirtualMachine->RunScript(&pArea->m_scriptOnExit, pArea->m_objectId, 1);
                        }
                    }
                }
            }
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
}

int CServerExoAppInternal::SendEnteringStartNewModuleMessage()
{
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerModule_StartStartNewModule();
    return 1;
}

void CServerExoAppInternal::SendStartStallEvent(unsigned int a)
{
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_StartStallEvent(a);
}

int CServerExoAppInternal::SendExitingStartNewModuleMessage(int a)
{
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerModule_EndStartNewModule(a);
    return 1;
}

#if KOTOR1
#include "nwcjournal.h"
#include "nwsjournal.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwscreaturestats.h"
#include "clientoptions.h"
#include "port/ios/IosLocalizedStrings.h"
void IosSetNotificiation(const char* const* apMessages);
extern char g_IosTakeScreenShot;
#if KOTOR1
char g_IosTakeScreenShot = 1;
#endif
#if KOTOR1
extern int g_bMouseCameraRotateToggle;
#endif

// Schedules the "come back" local notification before saving: up to four hints drawn from the
// PC's level progress, the newest active quest and a random party member's XP, only below level 15.
int CServerExoAppInternal::SaveGame(unsigned int saveNumber, const CExoString& name, CSWSPlayer* pPlayer, int d)
{
    K2_UNUSED(d);
    static IosLocalizedStrings s_notifications("data/localnot.txt", false);
    const char* pszLevelNote = s_notifications.GetString(0, 3);
    const char* pszQuestNote = s_notifications.GetString(1, 3);
    const char* pszPartyNote = s_notifications.GetString(2, 3);
    const char* apNotes[5] = {};

    unsigned int nLevel = 1;
    CSWSPlayer* pClient =
        static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1));
    if (pClient != 0) {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pClient->m_gameObjectId);
        nLevel = pCreature != 0 ? pCreature->m_pStats->GetLevel(0) : 1;
    }
    char szLevel[256] = {};
    sprintf(szLevel, pszLevelNote, nLevel * 100 / 21);
    apNotes[0] = szLevel;

    CExoString sQuest;
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    if (pJournal->GetNumActiveEntries() != 0) {
        SJournalEntry* pEntry = pJournal->GetActiveEntryAt(0);
        if (pEntry != 0)
            pEntry->m_locName.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                        &sQuest, 0);
    }
    char szQuest[256] = {};
    if (!sQuest.IsEmpty()) {
        sprintf(szQuest, pszQuestNote, sQuest.CStr());
        apNotes[1] = szQuest;
    } else {
        apNotes[1] = szLevel;
    }

    CExoString sMember;
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    CSWParty* pParty = pApp->GetSWParty();
    unsigned int nMemberLevel = 1;
    int nXP = 0;
    int nXPNeeded = 0;
    if (pParty->m_nCharacterCount != 0) {
        CSWCCreature* pMember =
            pApp->GetCreatureByGameObjectID(pParty->m_members[rand() % pParty->m_nCharacterCount].id);
        CSWSCreature* pServerMember = pMember->GetServerCreature();  // fetched before the null check
        if (pMember != 0 && pServerMember->m_pStats != 0) {
            CSWSCreatureStats* pStats = pServerMember->m_pStats;
            nMemberLevel = pStats->GetLevel(0);
            nXP = pStats->m_nExperience;  // m_nExperience holds the XP value
            nXPNeeded = pStats->GetExpNeededForLevelUp();
            sMember = pMember->GetName();
            sMember.CStr();
        }
    }
    char szParty[256] = {};
    if (!sMember.IsEmpty() && nMemberLevel < 20) {
        int nToGo = nXPNeeded - nXP;
        sprintf(szParty, pszPartyNote, sMember.CStr(), nToGo > 0 ? nToGo : 0, nMemberLevel + 1);
        apNotes[2] = szParty;
    } else {
        apNotes[2] = szLevel;
    }

    const char* pSwap;
    if (rand() >= RAND_HALF_RANGE) { pSwap = apNotes[0]; apNotes[0] = apNotes[1]; apNotes[1] = pSwap; }
    if (rand() >= RAND_HALF_RANGE) { pSwap = apNotes[1]; apNotes[1] = apNotes[2]; apNotes[2] = pSwap; }
    if (rand() >= RAND_HALF_RANGE) { pSwap = apNotes[0]; apNotes[0] = apNotes[2]; apNotes[2] = pSwap; }
    apNotes[4] = 0;
    apNotes[3] = apNotes[0];
    if (nLevel * 100 < 1470)
        IosSetNotificiation(apNotes);

    CExoString saveSlot;
    CExoString saveGameName;
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL);

    g_pExoResMan->CreateDirectory2("SAVES:");

    saveSlot.Format(const_cast<char*>("SAVES:%06d - %s"), saveNumber, name.CStr());

    uint64_ll_t freeSpace;
    int freeSpaceStatus;
    freeSpaceStatus = g_pExoResMan->GetFreeDiskSpace("SAVES:", &freeSpace);
    if (freeSpaceStatus == 1)
    {
        if (m_maxSaveDirectorySize > freeSpace + g_pExoBase->GetDirectorySize(saveSlot))
        {
            m_pSWSMessage->SendServerToPlayerSaveLoad_Status(pPlayer, 2, 1);
#ifdef K2_FIX_ENGINE_BUGS
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#endif
            return 0;
        }
    }

    if (g_pExoResMan->CreateDirectory2(saveSlot) == 0 &&
        g_pExoResMan->CleanDirectory(saveSlot, 0, 0) == 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#endif
        return 0;
    }

    saveGameName = saveSlot + "/" + "SAVEGAME";

    CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
    pInfo->m_flag = 1;
    pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME;
    pInfo->m_bStallFirstFrame = 1;
    pInfo->m_errorCode = 0;
    pInfo->m_saveDirectory = saveSlot;
    g_pAppManager->m_pLoadGameErrorInfo->m_saveGameName = saveGameName;
    g_pAppManager->m_pLoadGameErrorInfo->m_nLoadBarPair = 0;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_StartStallEvent(
        CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME);

    saveSlot = g_pExoBase->m_pAliasList->GetAliasPath("SAVES", 0);

    if (saveSlot != "" && g_IosTakeScreenShot)
    {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0)
        {
            pOnAStick->SetCameraForScreenShot();
        }
        saveGameName.Format(const_cast<char*>("%s%06d - %s%sScreen"), saveSlot.CStr(), saveNumber, name.CStr(), "/");
        g_pAppManager->DoSaveGameScreenShot(saveGameName);
    }

    if (saveNumber != 0)
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
        UpdateScreen(0.033333335f, 0, 0);
    }

    m_autoSaveTimerDay = 0;
    m_autoSaveTimerFlag = 0;
    return 1;
}
#endif

int CServerExoAppInternal::SaveGame(unsigned int saveNumber, const CExoString& name, CSWSPlayer* pPlayer, int d, int tempSave)
{
    K2_UNUSED(d);
    CSWPlayerControl* pPlayerControl = g_pAppManager->m_pClientExoApp->m_pInternal->m_pPlayerControl;
    if (pPlayerControl != 0)
    {
        pPlayerControl->ResetDriveAcceleration();
    }

    CExoString saveSlot;
    CExoString saveGameName;
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);

    g_pExoResMan->CreateDirectory2("SAVES:");

    uint64_ll_t freeSpace;
    int freeSpaceStatus;
    CLoadGameErrorInfo* pInfo;
    CExoString tempName(name);
    if (tempSave != 0)
        tempName = "tempsave";

    saveSlot.Format(const_cast<char*>("SAVES:%06d - %s"), saveNumber, tempName.CStr());

    freeSpaceStatus = g_pExoResMan->GetFreeDiskSpace("SAVES:", &freeSpace);
    if (freeSpaceStatus == 1)
    {
        if (m_maxSaveDirectorySize > freeSpace + g_pExoBase->GetDirectorySize(saveSlot))
        {
            m_pSWSMessage->SendServerToPlayerSaveLoad_Status(pPlayer, 2, 1);
#ifdef K2_FIX_ENGINE_BUGS
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
            return 0;
        }
    }

    if (g_pExoResMan->CreateDirectory2(saveSlot) == 0 &&
        g_pExoResMan->CleanDirectory(saveSlot, 0, 0) == 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        return 0;
    }

    saveGameName = saveSlot + "\\" + "SAVEGAME";

    pInfo = g_pAppManager->m_pLoadGameErrorInfo;
    pInfo->m_flag = 1;
    pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME;
    pInfo->m_bStallFirstFrame = 1;
    pInfo->m_errorCode = 0;
    pInfo->m_saveDirectory = saveSlot;
    g_pAppManager->m_pLoadGameErrorInfo->m_saveGameName = saveGameName;
    g_pAppManager->m_pLoadGameErrorInfo->m_nLoadBarPair = 0;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_StartStallEvent(
        CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME);

    saveSlot = g_pExoBase->m_pAliasList->GetAliasPath("SAVES", 0);

    if (saveSlot != "")
    {
        // GetModule() and the camera are used without a null check.
        CSWCModule* pModule;
        CSWCameraOnAStick* pOnAStick;
        pModule = g_pAppManager->m_pClientExoApp->GetModule();
        pOnAStick = static_cast<CSWCameraOnAStick*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0)
        {
            pOnAStick->SetCameraForScreenShot();
        }
        saveGameName.Format(const_cast<char*>("%s%06d - %s%sScreen"), saveSlot.CStr(), saveNumber, tempName.CStr(), "\\");
        g_pAppManager->DoSaveGameScreenShot(saveGameName);
    }

    if (saveNumber != 0)
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
        UpdateScreen(0.033333335f, 0, 0);
    }

    m_autoSaveTimerDay = 0;
    m_autoSaveTimerFlag = 0;
    return 1;
}

int CServerExoAppInternal::LoadGame(unsigned int a, const CExoString& b, const CExoString& c, CSWSPlayer* d)
{
    CExoString saveSlot;
    CExoString saveName;
    CExoString unusedA;
    CExoString unusedB;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 1);
    g_pAppManager->m_pClientExoApp->SetLoadStep(0, 10);
    g_pAppManager->m_pClientExoApp->SetLoadStep(1, 20);
    g_pAppManager->m_pClientExoApp->SetLoadStep(2, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(3, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(4, 23);

    if (a == 0xffffffff)
        saveName = b;
    else
        saveName.Format(const_cast<char*>("%06d - %s"), a, b.CStr());

    saveSlot = "SAVES:";
    saveSlot = saveSlot + saveName;

    CSWPartyTable& partyTable = m_partyTable;
    g_pExoResMan->AddResourceDirectory(saveSlot);
    partyTable.LoadTableInfo();
    m_globalVariableTable.Load();
    g_pExoResMan->RemoveResourceDirectory(saveSlot);
    partyTable.m_bKeepSavedPositions = 1;

    m_autoSaveTimerDay = 0;
    m_autoSaveTimerFlag = 0;
    return LoadModule(c, d);
}

void CServerExoAppInternal::ExecuteLoadModule(const CExoString& moduleName)
{
    const char* pDesc = "";
    if (GetModule() != 0 && !GetModule()->m_sModuleResourceName.IsEmpty())
        pDesc = GetModule()->m_sModuleResourceName.CStr();
    DumpLogCounters(const_cast<char*>(pDesc));
    SendEnteringStartNewModuleMessage();
    g_pExoInput->ClearEvents();
#if KOTOR1
    g_bMouseCameraRotateToggle = 0;
#else
    EnableMouseCameraRotateToggle(false);
#endif
    g_bLeftMouseDown = 0;
    g_bShiftActionState = 0;
    g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->PauseWorldTimer();
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->PauseWorldTimer();
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetSnapshotTime();
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->SetSnapshotTime();
    g_pAppManager->m_pClientExoApp->DisableInput();
    LoadModule(moduleName, 0);
}

// Dispatches on loadType: a reload of an already-picked server-vault character; the default path
// that loads a just-received character sheet; load-by-numeric-character-id; load-the-default
// character and create the party; and the server-vault lookup by player name.
bool CServerExoAppInternal::LoadCharacterStart(unsigned char loadType, CSWSPlayer* pPlayer, CResRef resRef, void* pData, unsigned int dataSize)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CExoString playerVaultName;
    CExoArrayList<CExoString> vaultFiles;
    CExoString vaultPath;

    switch (loadType)
    {
    case MSG_LOGIN_SERVER_CHARACTER:
        if (pPlayer->GetGameObject() == 0)
            pPlayer->LoadServerCharacter(resRef);
        break;

    case MSG_LOGIN_SERVER_CHARACTER_JOIN:
        if (pPlayer->GetGameObject() == 0)
        {
            CExoString resRefStr;
            resRef.CopyToString(resRefStr);
            pPlayer->LoadCharacterFromIFO(resRefStr.AsINT(), 0, 1);
        }
        goto finishLoad;

    case MSG_LOGIN_IFO_CHARACTER:
        pPlayer->LoadCharacterFromIFO(0xffffffff, 1, 1);
        m_partyTable.CreateParty();
        goto finishLoad;

    case MSG_LOGIN_SERVER_CHARACTER_CREATE:
        if (pPlayer->GetGameObject() == 0)
        {
            {
                CExoString name = pPlayer->GetPlayerName();
                playerVaultName = name;
            }
            g_pExoBase->GetDirectoryList(&vaultFiles, CExoString("SERVERVAULT:"), RESTYPE_INVALID, 1, 0);
            if (vaultFiles.Contains(playerVaultName) != 0)
            {
                vaultPath.Format(const_cast<char*>("SERVERVAULT:%s"), playerVaultName.CStr());
                g_pExoResMan->AddResourceDirectory(vaultPath);
                pPlayer->LoadServerCharacter(resRef);
                pPlayer->m_state = 4;
                g_pExoResMan->RemoveResourceDirectory(vaultPath);
            }
        }
        break;

    default:
        if (pPlayer->GetGameObject() == 0)
        {
            {
                CExoFile* pFile = new CExoFile(CExoString("TEMP:temp_char"), RESTYPE_BIC, CExoString("wb"));
                if (!pFile->FileOpened())
                {
                    delete pFile;
                    return false;
                }
                pFile->Write(pData, 1, dataSize);
                delete pFile;
            }

            {
                CExoString dir("TEMP:");
                g_pExoResMan->AddResourceDirectory(dir);
                if (loadType == MSG_LOGIN_LOCAL_CHARACTER_JOIN || loadType == MSG_LOGIN_LOCAL_CHARACTER)
                    pPlayer->LoadLocalCharacter();
                g_pExoResMan->RemoveResourceDirectory(dir);
                g_pExoResMan->CleanDirectory(dir, 0, 0);
                m_partyTable.CreateParty();
            }

            if (loadType == MSG_LOGIN_IFO_CHARACTER)
                goto finishLoad;
        }
        break;
    }

    if (loadType == MSG_LOGIN_SERVER_CHARACTER_JOIN || pModule->m_bIsSaveGame == 0)
    {
    finishLoad:
        LoadCharacterFinish(pPlayer, 0);
    }
    else
    {
        int playerIndex = pModule->GetPlayerIndexInPlayerList(pPlayer);
        if (playerIndex == -1)
            LoadCharacterFinish(pPlayer, 0);
        else
        {
            pPlayer->m_playerIndex = playerIndex;
            SendCharacterQuery(pPlayer);
        }
    }

    return true;
}

void CServerExoAppInternal::InitializeNetLayer()
{
    static_cast<CNetLayer*>(m_pNetLayer)->Initialize(reinterpret_cast<CBaseExoApp*>(g_pAppManager->m_pServerExoApp));
}

int CServerExoAppInternal::ContinueMessageProcessing()
{
    return g_pAppManager->m_pLoadGameErrorInfo->m_flag == 1;
}

unsigned int CServerExoAppInternal::ResolvePlayerByFirstName(const CExoString& name)
{

    CExoString colName;
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            CGameObject* pGameObj;
            CSWSCreature* pCreature;
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
            if (pPlayer != 0)
            {
                pGameObj = pPlayer->GetGameObject();
                if (pGameObj != 0)
                {
                    pCreature = pGameObj->AsSWSCreature();
                    if (pCreature != 0)
                    {
                        CExoLocString outLoc;
                        unsigned int playerId = pPlayer->m_nPlayerId;
                        CExoLocString* pNameLoc = &pCreature->GetFirstName();
                        CSWSMessage::GetLocStringServer(playerId, *pNameLoc, outLoc, colName, 0);
                        if (colName == name)
                            return pPlayer->m_nPlayerId;
                    }
                }
            }
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
    return static_cast<unsigned int>(CGameObjectArray::INVALIDOBJID);
}

CExoString CServerExoAppInternal::GetPlayerListString()
{

    CExoString result;
    CExoString name;

    result = result + "ServerAdmins: " + CExoString(static_cast<int>(m_pSysAdminList->m_pInternal->m_nCount));

    CExoLinkedListNode* pos = m_pSysAdminList->m_pInternal->m_pHead;
    if (pos != 0)
    {
        do
        {
            CSWSClient* elem;
            void* pNetLayer;
            unsigned int playerId;
            CNetLayerPlayerInfo* pInfo;
            elem = static_cast<CSWSClient*>(m_pSysAdminList->GetAtPos(pos));
            pNetLayer = g_pAppManager->m_pServerExoApp->GetNetLayer();
            playerId = elem->m_nPlayerId;
            pInfo = static_cast<CNetLayer*>(pNetLayer)->GetPlayerInfo(playerId);
            name = pInfo->m_sName.CStr();
            result = result + "\n" + name;
            result = result + " [ServerAdmin]";
            m_pSysAdminList->GetNext(pos);
        } while (pos != 0);
    }
    result = result + "\n";

    result = result + "Players: " + CExoString(static_cast<int>(m_pPlayerList->m_pInternal->m_nCount));

    pos = m_pPlayerList->m_pInternal->m_pHead;
    if (pos != 0)
    {
        do
        {
            CSWSClient* elem;
            void* pNetLayer;
            unsigned int playerId;
            CNetLayerPlayerInfo* pInfo;
            elem = static_cast<CSWSClient*>(m_pPlayerList->GetAtPos(pos));
            pNetLayer = g_pAppManager->m_pServerExoApp->GetNetLayer();
            playerId = elem->m_nPlayerId;
            pInfo = static_cast<CNetLayer*>(pNetLayer)->GetPlayerInfo(playerId);
            name = pInfo->m_sName.CStr();
            result = result + "\n" + name;
            result = result + " [Player]";
            m_pPlayerList->GetNext(pos);
        } while (pos != 0);
    }
    result = result + "\n";

    result = result + "Total: " + CExoString(static_cast<int>(m_pSysAdminList->m_pInternal->m_nCount) + static_cast<int>(m_pPlayerList->m_pInternal->m_nCount));

    return result;
}

bool CServerExoAppInternal::ValidateCreateServerCharacter(CSWSPlayer* pPlayer, void* pData, unsigned int dataSize)
{
    // Constructed and destroyed on every path but never read.
    CExoLocString unusedLoc0;
    CExoLocString unusedLoc1;
    CExoString unusedStr;
    CExoString vaultPath;
    CResList classList;
    CResStruct topStruct;

    bool bResult;

    CExoFile* pFile = new CExoFile(CExoString("TEMP:temp_char"), RESTYPE_BIC, CExoString("wb"));
    if (!pFile->FileOpened())
    {
        pFile->~CExoFile();
        operator delete(pFile);
    }
    else
    {
        pFile->Write(pData, 1, dataSize);
        pFile->~CExoFile();
        operator delete(pFile);

        { CExoString dir("TEMP:"); g_pExoResMan->AddResourceDirectory(dir); }

        CResGFF* pGff = new CResGFF(RESTYPE_BIC, const_cast<char*>("BIC "), CResRef("temp_char"));

        if (pGff->m_nValid == 0)
        {
            delete pGff;
            { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
            g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
        }
        else
        {
            pGff->GetTopLevelStruct(&topStruct);

            int found;
            unsigned int experience = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("Experience"), found, 0);
            if (experience == 0)
            {
                unsigned char invulnerable = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("Invulnerable"), found, 0);
                if (invulnerable == 0)
                {
                    int hasClassList = pGff->GetList(&classList, &topStruct, const_cast<char*>("ClassList"));
                    if (hasClassList == 0)
                    {
                        delete pGff;
                        { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                        g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
                    }
                    else
                    {
                        int classCount = pGff->GetListCount(&classList);
                        if (classCount == 1)
                        {
                            CResStruct classElem;
                            int gotElem = pGff->GetListElement(&classElem, &classList, 0);
                            if (gotElem == 0)
                            {
                                delete pGff;
                                { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                                g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
                            }
                            else
                            {
                                unsigned short classLevel = pGff->ReadFieldSHORT(&classElem, const_cast<char*>("ClassLevel"), found, 1);
                                delete pGff;

                                if (classLevel == 1)
                                {
                                    pPlayer->LoadLocalCharacter();
                                    { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                                    g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);

                                    vaultPath.Format(const_cast<char*>("SERVERVAULT:%s"), pPlayer->GetPlayerName().CStr());
                                    pPlayer->m_state = 4;
                                    pPlayer->SetFileName(CResRef(""));
                                    bResult = pPlayer->SaveServerCharacter() != 0;
                                    goto epilogue;
                                }
                                else
                                {
                                    { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                                    g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
                                }
                            }
                        }
                        else
                        {
                            delete pGff;
                            { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                            g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
                        }
                    }
                }
                else
                {
                    delete pGff;
                    { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                    g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
                }
            }
            else
            {
                delete pGff;
                { CExoString dir("TEMP:"); g_pExoResMan->RemoveResourceDirectory(dir); }
                g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);
            }
        }
    }

    bResult = false;
epilogue:
    return bResult;
}

int CServerExoAppInternal::GetDifficultyOption(int a)
{
    return g_pRules->GetDifficultyOption(a, m_pServerInfo->m_difficultyLevel);
}

unsigned int CServerExoAppInternal::GetPlayerIDByGameObjectID(unsigned long long a)
{
    CSWSPlayer* pPlayer = GetClientObjectByObjectId(a);
    const unsigned int* p = (pPlayer != 0) ? &pPlayer->m_nPlayerId : &CNetLayer::PLAYERID_INVALIDID;
    return *p;
}

uint64_ll_t CServerExoAppInternal::GetSystemTimeAsDWORD64()
{
    SystemTimeWords st;
    unsigned long long ft;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);
    return ft;
}

unsigned long long CServerExoAppInternal::GetPlayerCreatureId()
{
    CSWSPlayer* pClientObj = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1));
    if (pClientObj == 0)
        return OBJECT_INVALID;
    if (pClientObj->GetGameObject() == 0)
        return OBJECT_INVALID;
    return pClientObj->GetGameObject()->m_objectId;
}

int CServerExoAppInternal::ValidatePlayerLogin(void* a)
{
    if (static_cast<CSWSPlayer*>(a)->m_connectionState != 1)
    {
        static_cast<CSWSPlayer*>(a)->m_connectionState = 1;
        m_pSWSMessage->SendServerToPlayerLogin_Confirm(static_cast<CSWSPlayer*>(a)->m_nPlayerId);
        m_pSWSMessage->SendServerToPlayerModule_Info(static_cast<CSWSPlayer*>(a)->m_nPlayerId);
    }
    return 1;
}

int CServerExoAppInternal::UpdateLogHeartbeatTimer(uint64_ll_t a)
{
    unsigned long long prevTime = m_logHeartbeatLastTime;
    if (prevTime == 0)
    {
        m_logHeartbeatInterval = 10000000;
        m_logHeartbeatLastTime = a;
        return 0;
    }
    unsigned long long elapsed = a - prevTime;
    unsigned long long remaining = m_logHeartbeatInterval - elapsed;
    if (m_logHeartbeatInterval <= elapsed)
    {
        m_logHeartbeatLastTime = a;
        m_logHeartbeatInterval = 10000000;
        return 1;
    }
    m_logHeartbeatInterval = remaining;
    m_logHeartbeatLastTime = a;
    return 0;
}

int CServerExoAppInternal::AddPendingAuthorization(unsigned int a)
{
    int* arr = m_pPendingAuthIds;
    unsigned int sentinel = CNetLayer::PLAYERID_INVALIDID;
    int idx;
    if (static_cast<unsigned int>(arr[0]) == sentinel) idx = 0;
    else if (static_cast<unsigned int>(arr[1]) == sentinel) idx = 1;
    else if (static_cast<unsigned int>(arr[2]) == sentinel) idx = 2;
    else if (static_cast<unsigned int>(arr[3]) == sentinel) idx = 3;
    else if (static_cast<unsigned int>(arr[4]) == sentinel) idx = 4;
    else if (static_cast<unsigned int>(arr[5]) == sentinel) idx = 5;
    else if (static_cast<unsigned int>(arr[6]) == sentinel) idx = 6;
    else if (static_cast<unsigned int>(arr[7]) == sentinel) idx = 7;
    else if (static_cast<unsigned int>(arr[8]) == sentinel) idx = 8;
    else if (static_cast<unsigned int>(arr[9]) == sentinel) idx = 9;
    else if (static_cast<unsigned int>(arr[10]) == sentinel) idx = 10;
    else if (static_cast<unsigned int>(arr[11]) == sentinel) idx = 11;
    else if (static_cast<unsigned int>(arr[12]) == sentinel) idx = 12;
    else if (static_cast<unsigned int>(arr[13]) == sentinel) idx = 13;
    else if (static_cast<unsigned int>(arr[14]) == sentinel) idx = 14;
    else if (static_cast<unsigned int>(arr[15]) == sentinel) idx = 15;
    else
        return 0;
    arr[idx] = a;
    return 1;
}

int CServerExoAppInternal::RemovePendingAuthorization(unsigned int a)
{
    int* arr = m_pPendingAuthIds;
    int idx;
    if (static_cast<unsigned int>(arr[0]) == a) idx = 0;
    else if (static_cast<unsigned int>(arr[1]) == a) idx = 1;
    else if (static_cast<unsigned int>(arr[2]) == a) idx = 2;
    else if (static_cast<unsigned int>(arr[3]) == a) idx = 3;
    else if (static_cast<unsigned int>(arr[4]) == a) idx = 4;
    else if (static_cast<unsigned int>(arr[5]) == a) idx = 5;
    else if (static_cast<unsigned int>(arr[6]) == a) idx = 6;
    else if (static_cast<unsigned int>(arr[7]) == a) idx = 7;
    else if (static_cast<unsigned int>(arr[8]) == a) idx = 8;
    else if (static_cast<unsigned int>(arr[9]) == a) idx = 9;
    else if (static_cast<unsigned int>(arr[10]) == a) idx = 10;
    else if (static_cast<unsigned int>(arr[11]) == a) idx = 11;
    else if (static_cast<unsigned int>(arr[12]) == a) idx = 12;
    else if (static_cast<unsigned int>(arr[13]) == a) idx = 13;
    else if (static_cast<unsigned int>(arr[14]) == a) idx = 14;
    else if (static_cast<unsigned int>(arr[15]) == a) idx = 15;
    else
        return 0;
    arr[idx] = static_cast<int>(CNetLayer::PLAYERID_INVALIDID);
    return 1;
}

int CServerExoAppInternal::IsOnActiveExclusionList(unsigned long long a)
{
    unsigned char pauseState = m_pauseFlags & 1;
    if (m_pauseFlags & 2)
        pauseState = 2;

    CExoLinkedList<unsigned long long>* pList;
    if (pauseState == 1)
        pList = m_exclusionList2;
    else if (pauseState == 2)
        pList = m_exclusionList1;
    else
        return 1;

    if (pList == 0)
        return 1;

    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    while (pNode != 0)
    {
        if (reinterpret_cast<unsigned long long>(pList->GetAtPos(pNode)) == a)
            return 1;
        pList->GetNext(pNode);
    }
    return 0;
}

int CServerExoAppInternal::IsOnExclusionList(unsigned long long a)
{
    unsigned char pauseState = m_pauseFlags & 1;
    if (m_pauseFlags & 2)
        pauseState = 2;

    if (pauseState == 1)
    {
        CExoLinkedListNode* pNode = m_exclusionList2->m_pInternal->m_pHead;
        if (pNode == 0)
            return 0;
        while (reinterpret_cast<unsigned long long>(m_exclusionList2->GetAtPos(pNode)) != a)
        {
            m_exclusionList2->GetNext(pNode);
            if (pNode == 0)
                return 0;
        }
    }
    else if (pauseState == 2)
    {
        CExoLinkedListNode* pNode = m_exclusionList1->m_pInternal->m_pHead;
        if (pNode == 0)
            return 0;
        while (reinterpret_cast<unsigned long long>(m_exclusionList1->GetAtPos(pNode)) != a)
        {
            m_exclusionList1->GetNext(pNode);
            if (pNode == 0)
                return 0;
        }
    }
    return 1;
}

void CServerExoAppInternal::AddToExclusionList(unsigned long long a, unsigned char b)
{
    if (b == 0)
        return;

    if (b & 2)
    {
        CExoLinkedListNode* pNode = m_exclusionList1->m_pInternal->m_pHead;
        while (pNode != 0)
        {
            if (reinterpret_cast<unsigned long long>(m_exclusionList1->GetAtPos(pNode)) == a)
                return;
            m_exclusionList1->GetNext(pNode);
        }
        m_exclusionList1->AddTail(reinterpret_cast<unsigned long long*>(a));
    }
    else if (b & 1)
    {
        CExoLinkedListNode* pNode = m_exclusionList2->m_pInternal->m_pHead;
        while (pNode != 0)
        {
            if (reinterpret_cast<unsigned long long>(m_exclusionList2->GetAtPos(pNode)) == a)
                return;
            m_exclusionList2->GetNext(pNode);
        }
        m_exclusionList2->AddTail(reinterpret_cast<unsigned long long*>(a));
    }
}

void CServerExoAppInternal::RemoveFromExclusionList(unsigned long long a, unsigned char b)
{
    if (b == 0)
        return;

    if (b & 1)
    {
        CExoLinkedListNode* pNode = m_exclusionList2->m_pInternal->m_pHead;
        if (pNode == 0)
            return;
        while (reinterpret_cast<unsigned long long>(m_exclusionList2->GetAtPos(pNode)) != a)
        {
            m_exclusionList2->GetNext(pNode);
            if (pNode == 0)
                return;
        }
        m_exclusionList2->Remove(pNode);
    }
    else if (b & 2)
    {
        CExoLinkedListNode* pNode = m_exclusionList1->m_pInternal->m_pHead;
        if (pNode == 0)
            return;
        while (reinterpret_cast<unsigned long long>(m_exclusionList1->GetAtPos(pNode)) != a)
        {
            m_exclusionList1->GetNext(pNode);
            if (pNode == 0)
                return;
        }
        m_exclusionList1->Remove(pNode);
    }
}

void* CServerExoAppInternal::GetActiveTimer(unsigned long long a)
{
    void** ppTimer = reinterpret_cast<void**>(&m_pWorldTimer);
    if (a != OBJECT_INVALID)
    {
        unsigned char pauseState = m_pauseFlags & 1;
        if (m_pauseFlags & 2)
            pauseState = 2;

        if (pauseState == 2)
        {
            CExoLinkedList<unsigned long long>* pList = m_exclusionList1;
            if (pList == 0)
                ppTimer += 2;
            else
            {
                CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
                while (pNode != 0)
                {
                    if (reinterpret_cast<unsigned long long>(pList->GetAtPos(pNode)) == a)
                    {
                        ppTimer += 2;
                        break;
                    }
                    pList->GetNext(pNode);
                }
            }
        }
        else if (pauseState != 0)
        {
            if (pauseState == 1)
            {
                CExoLinkedList<unsigned long long>* pList = m_exclusionList2;
                if (pList == 0)
                    ppTimer += 1;
                else
                {
                    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
                    while (pNode != 0)
                    {
                        if (reinterpret_cast<unsigned long long>(pList->GetAtPos(pNode)) == a)
                        {
                            ppTimer += 1;
                            break;
                        }
                        pList->GetNext(pNode);
                    }
                }
            }
            else
                ppTimer += 1;
        }
    }
    return *ppTimer;
}

void CServerExoAppInternal::ReprocessExclusionListActions(unsigned char pauseState)
{

    CExoLinkedList<unsigned long long>* pList = (pauseState == 2) ? m_exclusionList1 : m_exclusionList2;
    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    while (pNode != 0)
    {
        unsigned long long id = reinterpret_cast<unsigned long long>(pList->GetAtPos(pNode));
        CWorldTimer* pTimer = static_cast<CWorldTimer*>(GetActiveTimer(id));
        CGameObject* pObject;
        unsigned int a, b;
        pTimer->GetWorldTime(&a, &b);
        m_pObjectArray->GetGameObject(id, &pObject);
        if (pObject != 0)
            pObject->ResetUpdateTimes(a, b);

        pList->GetNext(pNode);
    }
}

int CServerExoAppInternal::CreatureAcquireItem(unsigned long long id, const CResRef& resRef)
{

    CGameObject* pObject = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject == 0)
        return 0;

    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;

    CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
    pItem->LoadFromTemplate(resRef);
    pItem->m_flags &= ~4;

    CSWSCreature* pCreature2 = pObject->AsSWSCreature();
    return pCreature2->AcquireItem(&pItem, OBJECT_INVALID, OBJECT_INVALID, 1);
}

void CServerExoAppInternal::UpdateSubareasOnMoveTo(unsigned long long id, const Vector& from, const Vector& to)
{
    CGameObject* pObject = 0;
    unsigned long long serverId = ClientToServerObjectId(id);
    m_pObjectArray->GetGameObject(serverId, &pObject);
    if (pObject == 0)
        return;

    CGameObject* pObj2 = pObject->AsSWSObject();
    CSWSCreature* pCreature = pObj2->AsSWSCreature();
    if (pCreature == 0)
        return;

    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
    if (pArea == 0)
        return;

    CExoArrayList<unsigned long long> list;
    pArea->IntersectingLineSegment(from, to, &list);
    pCreature->UpdateSubareasOnMoveTo(from, to, 1, &list, 0);
}

int CServerExoAppInternal::GetFactionOfObject(unsigned long long id, int* pFaction)
{
    CGameObject* pFound = 0;
#if KOTOR1  // K1 tests the lookup status first
    int bFound = 0;
    if (m_pObjectArray->GetGameObject(id, &pFound) == CGameObjectArray::SUCCESS && pFound != 0) {
        CGameObject* pObject = pFound;
        if (pObject->AsSWSCreature() != 0) {
            *pFaction = pObject->AsSWSCreature()->m_pStats->m_factionId;
            bFound = 1;
        } else if (pObject->AsSWSDoor() != 0) {
            *pFaction = pObject->AsSWSDoor()->m_nFaction;
            bFound = 1;
        } else if (pObject->AsSWSEncounter() != 0) {
            *pFaction = pObject->AsSWSEncounter()->m_nFaction;
            bFound = 1;
        } else if (pObject->AsSWSPlaceable() != 0) {
            *pFaction = pObject->AsSWSPlaceable()->m_nFaction;
            bFound = 1;
        } else if (pObject->AsSWSTrigger() != 0) {
            *pFaction = pObject->AsSWSTrigger()->m_nFaction;
            bFound = 1;
        }
    }
    return bFound;
#else
    m_pObjectArray->GetGameObject(id, &pFound);
    CGameObject* pObject = pFound;
    if (pObject == 0)
        return 0;

    if (pObject->AsSWSCreature() != 0)
    {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        *pFaction = pCreature->m_pStats->m_factionId;
        return 1;
    }
    if (pObject->AsSWSDoor() != 0)
    {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        *pFaction = pDoor->m_nFaction;
        return 1;
    }
    if (pObject->AsSWSEncounter() != 0)
    {
        CSWSEncounter* pEncounter = pObject->AsSWSEncounter();
        *pFaction = pEncounter->m_nFaction;
        return 1;
    }
    if (pObject->AsSWSPlaceable() != 0)
    {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        *pFaction = pPlaceable->m_nFaction;
        return 1;
    }
    if (pObject->AsSWSTrigger() != 0)
    {
        CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
        *pFaction = pTrigger->m_nFaction;
        return 1;
    }
    return 0;
#endif
}

unsigned long long CServerExoAppInternal::GetFirstPCObject()
{
    CExoLinkedListInternal* pList = m_pPlayerList->m_pInternal;
    CExoLinkedListNode* pNode = pList->m_pHead;
    m_pcObjectIterPos = pNode;
    if (pNode != 0)
    {
        CSWSPlayer* p = static_cast<CSWSPlayer*>(pList->GetAtPos(pNode));
        if (p != 0)
            return p->m_gameObjectId;
    }
    return CGameObjectArray::INVALIDOBJID;
}

unsigned long long CServerExoAppInternal::GetNextPCObject()
{
    if (m_pcObjectIterPos != 0)
    {
        CSWSPlayer* p = static_cast<CSWSPlayer*>(m_pPlayerList->GetNext(m_pcObjectIterPos));
        if (p != 0)
            return p->m_gameObjectId;
    }
    return CGameObjectArray::INVALIDOBJID;
}

#if KOTOR1
CGameObject* CServerExoAppInternal::GetGameObject(unsigned long long id)
{
    CGameObject* pObject = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) != CGameObjectArray::SUCCESS)
        return 0;
    return pObject;
}

int CServerExoAppInternal::DebugValidatePath_Point(CSWSArea* pArea, const Vector& v)
{
    int roomFound = pArea->GetRoom(v, 0) != 0;
#else
int CServerExoAppInternal::DebugValidatePath_Point(CSWSArea* pArea, const Vector& v, int* pb)
{
    int roomFound = pArea->GetRoom(v, 0, pb) != 0;
#endif
    int walkable = pArea->PositionWalkable(v) != 0;
    return roomFound && walkable;
}

Vector CServerExoAppInternal::GetNormalOfPoint(const Vector& point)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CSWSArea* pArea = pModule->GetArea();
    int roomIndex;
    CSWSRoom* pRoom = pArea->GetRoom(point, &roomIndex, 0);
    if (pRoom == 0)
        return Vector{0.0f, 0.0f, 1.0f};
    return pRoom->GetNormalOfPoint(roomIndex);
}

void CServerExoAppInternal::DealWithLoadGameError(unsigned int a)
{
    CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
    UnloadModule();
    pInfo->m_errorCode = a;
    pInfo->m_bStallFinished = 1;
    if (g_nClientServerMode == 2)
    {
        pInfo->m_bStallFinished = 0;
        pInfo->m_flag = 0;
        pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
    }
    m_serverMode = 0;
    m_loadingModule = 0;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_EndStallEvent(1, a);
}

int CServerExoAppInternal::Initialize()
{
    CSWGlobalVariableTable* pTable = &m_globalVariableTable;
    CResRef ref("globalcat");
    pTable->ReadCatalogue(ref);
    return 1;
}

void CServerExoAppInternal::SkipPastDialog(unsigned long long id)
{
    CGameObject* pOut = 0;
#if KOTOR1  // K1 tests the lookup status
    if (m_pObjectArray->GetGameObject(id, &pOut) != CGameObjectArray::SUCCESS)
        return;
#else
    m_pObjectArray->GetGameObject(id, &pOut);
#endif
    CGameObject* pObject = pOut;
    if (pObject != 0)
    {
        if (pObject->AsSWSObject() != 0)
        {
            pObject->AsSWSObject()->m_bSkipDialogRequested = 1;
        }
    }
}

int CServerExoAppInternal::SendCharacterQuery(CSWSPlayer* pPlayer)
{
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    unsigned char c[4];
    int b[2];
    unsigned char a[4];
    unsigned int d;
    pPlayer->GetCharacterInfoFromIFO(*a, b, c, d);
    pMessage->SendServerToPlayerLogin_CharacterQuery(pPlayer, *a, b, c, d);
    return 1;
}

int CServerExoAppInternal::IsInjured(unsigned long long id)
{
    CGameObject* pObject = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) != CGameObjectArray::SUCCESS)
        return 0;
    if (pObject == 0)
        return 0;
    if (pObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    return pCreature->IsInjured();
}

int CServerExoAppInternal::UpdateAutoSaveTimer()
{
    m_pPauseTimer->GetSnapshotTime();
    int now = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    if (m_autoSaveTimerFlag != 0)
    {
        m_autoSaveTimerLastTime = now;
        return 1;
    }
    int last = m_autoSaveTimerLastTime;
    if (last == 0)
    {
        last = now;
        m_autoSaveTimerLastTime = now;
    }
    unsigned int elapsed = m_autoSaveTimerDay + (now - last);
    m_autoSaveTimerLastTime = now;
    m_autoSaveTimerDay = elapsed;
    if (elapsed > AUTOSAVE_INTERVAL_MS)
    {
        m_autoSaveTimerDay = 0;
        m_autoSaveTimerFlag = 1;
        return 1;
    }
    return 0;
}

unsigned int CServerExoAppInternal::GetPlayerLanguage(unsigned int playerId)
{
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        while (1)
        {
            CSWSClient* p = m_pPlayerList->GetAtPos(pNode);
            if (p != 0 && p->m_nPlayerId == playerId)
                return p->m_nLanguage;
            m_pPlayerList->GetNext(pNode);
            if (pNode == 0)
                break;
        }
    }
    return 0;
}

int CServerExoAppInternal::HandleMessage(unsigned int playerId, unsigned char* pMsg, unsigned int len, int a4)
{
    if (a4 != 0)
        return 0;
    if (*pMsg == 's')
    {
        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isAdminPermitted == 1)
            m_pSWSMessage->HandleServerAdminToServerMessage(playerId, pMsg, len);
    }
    if (*pMsg == 'p')
    {
        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isPlayerPermitted == 1)
            m_pSWSMessage->HandlePlayerToServerMessage(playerId, pMsg, len);
    }
    return 1;
}

void CServerExoAppInternal::UpdateClientGameObjects(int nPlayerId)
{
    uint64_ll_t now = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
            UpdateClientGameObjectsForPlayer(pPlayer, nPlayerId, now);
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
}

void CServerExoAppInternal::UpDatePlayerPosition(unsigned long long id, const Vector& pos, const Vector& orient)
{
    CSWSCreature* pCreature = GetCreatureByGameObjectID(id);
    if (pCreature != 0)
    {
        pCreature->SetPosition(pos, 1, 1, 0);
        pCreature->SetOrientation(orient);
    }
}

// The timer is read and discarded.
void CServerExoAppInternal::UpdateClientsForObject(unsigned long long id)
{
    g_pExoBase->m_pTimers->GetHighResolutionTimer();
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
            if (pPlayer->GetLastUpdateObject(id) != 0)
                m_pSWSMessage->SendServerToPlayerGameObjUpdate(pPlayer, id);
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
}

void CServerExoAppInternal::MarkUpdateClientsForObject(unsigned long long id)
{
    uint64_ll_t now = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
            if (pPlayer->GetLastUpdateObject(id) != 0)
                UpdateClientGameObjectsForPlayer(pPlayer, 1, now);
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
}

void CServerExoAppInternal::UpdateClientGameObjectsForPlayer(CSWSPlayer* pPlayer, int nPlayerId, uint64_ll_t now)
{
    uint64_ll_t updateTime = now;
    if (nPlayerId == 1 && now == 0)
        updateTime = g_pExoBase->m_pTimers->GetHighResolutionTimer();

    if (pPlayer->m_connectionState != 2)
    {
        if (pPlayer->m_connectionState != 1)
            return;
        if (pPlayer->m_characterLoaded != 1)
            return;
        if (pPlayer->GetGameObject() == 0)
            return;
        CGameObject* pObject = pPlayer->GetGameObject();
        CSWSCreature* pServerObject = pObject->AsSWSCreature();
        if (pServerObject == 0)
            return;
        if (pServerObject->m_currentAreaId == OBJECT_INVALID)
            return;
    }

    if (nPlayerId == 1 || (now - pPlayer->m_lastGameObjUpdateTime) >> 6 >= GAME_OBJECT_UPDATE_INTERVAL)
    {
        m_pSWSMessage->SendServerToPlayerGameObjUpdate(pPlayer);
        if (pPlayer->GetGameObject() != 0)
        {
            unsigned long long areaObjId = pPlayer->GetGameObject()->m_objectId;
            if (pPlayer->m_lastKnownAreaId != areaObjId)
            {
                CSWSArea* pArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pPlayer->GetGameObject())->GetArea());
                if (pArea != 0)
                {
                    m_pSWSMessage->SendServerToPlayerGameObjUpdate_ObjControl(pPlayer->m_nPlayerId, areaObjId);
                    pPlayer->m_lastGameObjUpdateTime = updateTime;
                    pPlayer->m_lastKnownAreaId = areaObjId;
                }
            }
        }
        pPlayer->m_lastGameObjUpdateTime = updateTime;
    }

    if (pPlayer->GetGameObject() == 0)
        return;
    CGameObject* pObject2 = pPlayer->GetGameObject();
    CSWSObject* pServerObject2 = pObject2->AsSWSCreature();
    if (pServerObject2->GetArea() != 0)
        return;
    if (pPlayer->m_lastKnownAreaId == OBJECT_INVALID)
        return;
    pPlayer->m_lastKnownAreaId = OBJECT_INVALID;
    m_pSWSMessage->SendServerToPlayerGameObjUpdate_ObjControl(pPlayer->m_nPlayerId, OBJECT_INVALID);
    pPlayer->m_lastGameObjUpdateTime = updateTime;
}

int CServerExoAppInternal::IsActionsPending(unsigned long long id)
{
    CSWSCreature* pCreature = GetCreatureByGameObjectID(ClientToServerObjectId(id));
    if (pCreature == 0)
        return 0;
    return pCreature->m_actionList.m_pInternal->m_pHead != 0;
}

CExoString CServerExoAppInternal::GetModuleName()
{
    if (m_lastModuleFoundId != m_moduleObjectId)
    {
        CGameObject* pObject = 0;
        m_pObjectArray->GetGameObject(m_moduleObjectId, &pObject);
        if (pObject != 0)
        {
            m_lastModuleFoundId = m_moduleObjectId;
            CSWSModule* pModule = pObject->AsSWSModule();
            m_pLastModuleFound = pModule;
        }
        else
        {
            m_lastModuleFoundId = OBJECT_INVALID;
            m_pLastModuleFound = 0;
        }
    }
    if (m_pLastModuleFound != 0)
        return m_pServerInfo->m_sModuleName;
    return CExoString();
}

#if KOTOR1
// K1 normalizes both paths to '/', prefers the patch OBB over the main OBB (the main OBB
// overwrites the patch result), then falls back to the file system.
int CServerExoAppInternal::CopyModuleToCurrentGame(const CExoString& src, const CExoString& dst, unsigned short unused)
{
    K2_UNUSED(unused);
    std::string sSrc(src.CStr());
    std::string sDst(dst.CStr());
    std::replace(sSrc.begin(), sSrc.end(), '\\', '/');
    std::replace(sDst.begin(), sDst.end(), '\\', '/');
    SDL_RWops* pIn = 0;
    SDL_RWops* pOut;
    int result;
    char buf[1024];
    unsigned int n;
    if (g_patchObbMounted)
        pIn = g_patchObb->RWFromFile(sSrc.c_str());
#ifdef K2_FIX_ENGINE_BUGS
    if (pIn == 0 && g_obbMounted)
#else
    if (g_obbMounted)
#endif
        pIn = g_mainObb->RWFromFile(sSrc.c_str());
    if (pIn == 0)
        pIn = SDL_RWFromFile(sSrc.c_str(), "rb");
    pOut = SDL_RWFromFile(sDst.c_str(), "wb");
    if (pIn != 0 && pOut != 0)
    {
        while ((n = static_cast<unsigned int>(SDL_RWread(pIn, buf, 1, sizeof(buf)))) != 0)
            SDL_RWwrite(pOut, buf, 1, n);
        SDL_RWclose(pIn);
        SDL_RWclose(pOut);
        result = 1;
    }
    else
    {
        if (pIn != 0)
            SDL_RWclose(pIn);
        if (pOut != 0)
            SDL_RWclose(pOut);
        result = 0;
    }
    return result;
}
#else
int CServerExoAppInternal::CopyModuleToCurrentGame(const CExoString& src, const CExoString& dst, unsigned short unused)
{
    K2_UNUSED(unused);
    FILE* fin = fopen(src.CStr(), "rb");
    FILE* fout = fopen(dst.CStr(), "wb");
    int result;
    if (fin != 0 && fout != 0)
    {
        char buf[1024];
        unsigned int n;
        do
        {
            n = static_cast<unsigned int>(fread(buf, 1, sizeof(buf), fin));
            if (n == 0)
                break;
            fwrite(buf, 1, n, fout);
        } while (true);
        fclose(fin);
        result = 1;
    }
    else
    {
        if (fin != 0)
            fclose(fin);
        result = 0;
        if (fout == 0)
            return result;
    }
    fclose(fout);
    return result;
}
#endif

void CServerExoAppInternal::SetEstimatedSaveSize(const CExoString&, unsigned short nFileType)
{
    CExoArrayList<CExoString> fileList;
    CExoString unused;
    CExoBase* pBase = g_pExoBase;
    {
        CExoString searchDir("GAMEINPROGRESS:");
        pBase->GetDirectoryList(&fileList, searchDir, RESTYPE_INVALID, 0, 0);
    }

    int count = static_cast<int>(fileList.m_count);
    if (count == 0)
    {
        m_maxSaveDirectorySize = SAVE_DIRECTORY_SIZE_DEFAULT;
        return;
    }

    CExoString name;
    CExoString path;
    CExoString ext;
    unsigned int total = 0;
    for (int i = 0; i < count; i++)
    {
        name = fileList.m_pArray[i];

        {
            CExoString prefix("GAMEINPROGRESS:");
#if KOTOR1  // K1 calls GetLength()
            CExoString base = name.Left(name.GetLength() - 4);
#else
            int len = name.GetLength();
            CExoString base = name.Left(len - 4);
#endif
            path = prefix + base;
        }

        ext = name.Right(3);
        unsigned short resType = g_pExoBase->GetResTypeFromExtension(ext);

        CExoFile file(path, resType, CExoString("rb"));
        if (file.FileOpened())
            total += file.GetSize();
        else
            total += SAVE_FILE_SIZE_ESTIMATE;
    }

    if (nFileType != RESTYPE_SAV && nFileType != RESTYPE_RSV)
        total += SAVE_FILE_SIZE_ESTIMATE;

    m_maxSaveDirectorySize = total * 10 / 9;
}

CSWSModule* CServerExoAppInternal::GetModule()
{
    if (m_lastModuleFoundId == m_moduleObjectId)
        return m_pLastModuleFound;

    CGameObject* pObject = 0;
    m_pObjectArray->GetGameObject(m_moduleObjectId, &pObject);
    CSWSModule* pModule;
    if (pObject == 0)
    {
        m_lastModuleFoundId = OBJECT_INVALID;
        pModule = 0;
    }
    else
    {
        m_lastModuleFoundId = m_moduleObjectId;
        pModule = pObject->AsSWSModule();
    }
    m_pLastModuleFound = pModule;
    return pModule;
}

void CServerExoAppInternal::MovePlayerToArea(void* pPlayer)
{

    Vector safePos;
    Vector pos;

    if (static_cast<CSWSPlayer*>(pPlayer)->GetGameObject() != 0)
    {
        CGameObject* pGameObj = static_cast<CGameObject*>(static_cast<CSWSPlayer*>(pPlayer)->GetGameObject());
        CSWSCreature* pCreature = pGameObj->AsSWSCreature();

        if (m_moveToModuleStartWaypoint[0] != 0)
        {
            unsigned long long waypointId = GetModule()->GetWaypoint(m_moveToModuleStartWaypoint);
            if (waypointId != OBJECT_INVALID)
            {
                CSWSWaypoint* pWaypoint = g_pAppManager->m_pServerExoApp->GetWaypointByGameObjectID(waypointId);
                if (pWaypoint != 0)
                {
                    pCreature->m_vDesiredAreaLocation = pWaypoint->m_position;
                    static_cast<CSWSObject*>(pCreature)->SetOrientation(pWaypoint->m_orientation);
                }
            }
            m_moveToModuleStartWaypoint = "";
        }
        pos = pCreature->m_vDesiredAreaLocation;

        CSWSArea* pArea = GetModule()->GetArea();
        CPathfindInformation* pPathInfo = pCreature->m_pPathfindInfo;
        pPathInfo->m_oidCreature = static_cast<CSWSObject*>(pCreature)->m_objectId;
        if (pArea != 0)
        {
            if (pArea->ComputeSafeLocation(pos, 20.0f, pPathInfo, 1, &safePos) == 0)
                safePos = pos;
            pCreature->UpdateSubareasOnJumpPosition(safePos, pArea->m_objectId, m_loadFromSaveGame == 0);
            pCreature->AddToArea(pArea, safePos.x, safePos.y, safePos.z, 0, 0);
        }
        pCreature->UpdateVisibleList(0);
        m_partyTable.PositionParty();
        m_loadFromSaveGame = 0;
    }
}

void CServerExoAppInternal::InitiateModuleForPlayer(void* pPlayerVoid)
{
    CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(pPlayerVoid);

    CSWSCreature* pCreature;
    if (pPlayer->GetGameObject() == 0)
    {
        pCreature = new CSWSCreature(OBJECT_INVALID, 1);
        CPathfindInformation* pPathInfo = pCreature->m_pPathfindInfo;
        unsigned long long objId = static_cast<CSWSObject*>(pCreature)->m_objectId;
        pPathInfo->m_oidCreature = objId;
        pPlayer->SetGameObject(pCreature);
    }
    else
    {
        CGameObject* pGameObj = static_cast<CGameObject*>(pPlayer->GetGameObject());
        pCreature = pGameObj->AsSWSCreature();
    }

    int mode = pPlayer->m_bRestoredPosition;
    int isFirstEntry = (mode == 0);
    CSWSArea* pArea;

    if (mode != 0)
    {
        pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea == 0)
        {
            pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(pCreature->m_currentAreaId);
        }
        else
        {
            pCreature->m_currentAreaId = pArea->m_objectId;
            pCreature->m_vDesiredAreaLocation = pCreature->m_position;
            pCreature->m_bDesiredAreaUpdateComplete = 0;
        }
    }
    else
    {
        Vector startPos;
        Vector facing;
        CSWSModule* pMod = GetModule();

        pArea = pMod->GetArea();
        pCreature->m_currentAreaId = pArea->m_objectId;

        pMod = GetModule();
        startPos.x = pMod->m_pEntryInfo->m_entryX;

        pMod = GetModule();
        startPos.y = pMod->m_pEntryInfo->m_entryY;

        pMod = GetModule();
        startPos.z = pMod->m_pEntryInfo->m_entryZ;

        pCreature->m_vDesiredAreaLocation = startPos;
        pCreature->m_bDesiredAreaUpdateComplete = 0;

        pMod = GetModule();
        facing.x = pMod->m_pEntryInfo->m_entryDirX;

        pMod = GetModule();
        facing.y = pMod->m_pEntryInfo->m_entryDirY;
        facing.z = 0.0f;
        static_cast<CSWSObject*>(pCreature)->SetOrientation(facing);
    }

    if (pArea != 0)
    {
        static_cast<CSWSObject*>(pCreature)->SetArea(0);
        CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        if (pMsg != 0)
        {
            {
                CExoString empty("");
                pPlayer->SetAreaTransitionBMP(pArea->m_nLoadScreenID, empty);
            }
            pMsg->SendServerToPlayerArea_ClientArea(pPlayer, pArea, pCreature->m_vDesiredAreaLocation.x,
                pCreature->m_vDesiredAreaLocation.y, pCreature->m_vDesiredAreaLocation.z, pCreature->m_orientation, isFirstEntry);
            pMsg->SendServerToPlayerSetCustomTokenList(pPlayer->m_nPlayerId);
        }
    }
}

int CServerExoAppInternal::RunModule()
{
    CExoString scratch;
    CExoLinkedListNode* pNode;
    int result;

    if (m_serverMode == 1)
    {
        m_serverMode = 2;
        m_pSWSMessage->SendServerToPlayerServerStatus_Status(CNetLayer::PLAYERID_ALL_PLAYERS);
        m_pSWSMessage->SendServerToServerAdminServerStatus(CNetLayer::PLAYERID_ALL_SERVERADMINS);

        pNode = m_pPlayerList->m_pInternal->m_pHead;
        if (pNode != 0)
        {
            do
            {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
                if (pPlayer->m_connectionState != 0)
                    MovePlayerToArea(pPlayer);
                m_pPlayerList->GetNext(pNode);
            } while (pNode != 0);
        }

        if (GetModule()->m_bIsSaveGame != 0 && m_doingStartNewModule != 1)
        {
            pNode = m_pPlayerList->m_pInternal->m_pHead;
            LoadPrimaryPlayer(static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode)));
            m_partyTable.CreateParty();
        }

        if (m_doingStartNewModule == 1)
        {
            g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerModule_EndStartNewModule(1);
            m_doingStartNewModule = 0;
        }

        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

int CServerExoAppInternal::UnloadModule()
{
    // The module lookup and the teardown at the end share this snapshot.
    CGameObjectArray* pOldObjectArray = m_pObjectArray;

    CSWSModule* pModule = GetModule();

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (g_pAppManager->m_pClientExoApp != 0 && g_pAppManager->m_pClientExoApp->GetSoundPausedByCombat() != 0)
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);

    SetPauseState(2, 0);
    SetPauseState(1, 0);

    delete m_exclusionList1;
    m_exclusionList1 = new CExoLinkedList<unsigned long long>;

    delete m_exclusionList2;
    m_exclusionList2 = new CExoLinkedList<unsigned long long>;

    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        unsigned long long invalidAreaId = OBJECT_INVALID;
        do
        {
            void* pElem = m_pPlayerList->GetAtPos(pNode);
            CSWSPlayer* pPlayer = static_cast<CSWSClient*>(pElem)->AsSWSPlayer();
            if (pPlayer != 0)
            {
                CObjectTableManager* pObjTable = g_pAppManager->GetObjectTableManager(1);
                pObjTable->RemovePlayer(pPlayer->m_nPlayerId);
                pMessage->SendServerToPlayerModule_DumpPlayer(pPlayer);

                CGameObject* pGameObj = pPlayer->GetGameObject();
                if (pGameObj != 0)
                {
                    CSWSCreature* pCreature = pGameObj->AsSWSCreature();
                    if (pCreature != 0)
                    {
                        if (static_cast<CSWSObject*>(pCreature)->GetArea() != 0)
                            pCreature->RemoveFromArea(0);
                        delete pCreature;
                        pPlayer->SetGameObject(0);
                    }
                }

                pPlayer->ClearPlayerOnDestroyGame();
                pPlayer->m_connectionState = 0;
                pPlayer->m_characterLoaded = 0;
                pPlayer->m_lastKnownAreaId = invalidAreaId;
                m_pSWSMessage->SendServerToPlayerGameObjUpdate_ObjControl(pPlayer->m_nPlayerId, OBJECT_INVALID);
            }
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }

    if (g_pAppManager->m_pClientExoApp != 0 && g_pAppManager->m_pClientExoApp->GetSWParty() != 0)
        g_pAppManager->m_pClientExoApp->GetSWParty()->ClearParty();

    m_partyTable.ClearInventory();
    g_pTlkTable->ClearCustomTokens();

    if (pModule != 0)
        delete pModule;
    m_moduleObjectId = OBJECT_INVALID;

    CFactionManager* pOldFactionManager = m_pFactionManager;
    if (pOldFactionManager != 0)
    {
        pOldFactionManager->~CFactionManager();
        operator delete(pOldFactionManager);
    }
    m_pFactionManager = 0;
    m_pFactionManager = new CFactionManager();

    if (pOldObjectArray != 0)
    {
        pOldObjectArray->~CGameObjectArray();
        operator delete(pOldObjectArray);
    }
    m_pObjectArray = new CGameObjectArray(1);

    m_pServerAIMaster->ClearEventQueue();

    g_pExoResMan->NukeDirectory(CExoString("CURRENTGAME:"), 1, 1);

    return 1;
}

void CServerExoAppInternal::UpdateMapData()
{
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();

    CSWSModule* pModule = GetModule();

    if (pParty == 0 || pModule == 0)
        return;
    CSWSAreaMap* pAreaMap = pModule->m_pAreaMap;
    if (pAreaMap == 0)
        return;

    for (int i = 0; i < pParty->m_nCharacterCount; i++)
    {
        CSWCCreature* pCreature = pParty->GetCharacter(i);
        if (pCreature != 0)
        {
            pAreaMap->SetWorldPointExplored(pCreature->m_position, 1);
            pAreaMap->SetPartyMemberWorldLocation(i, pCreature->m_position);
            pAreaMap->SetPartyMemberWorldOrientation(i, pCreature->m_orientation);
        }
    }
}

bool CServerExoAppInternal::DebugValidatePath(unsigned long long objectId, int which)
{
    if (!EnablePathValidation)
        return true;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature == 0)
        return false;
    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
    if (pArea == 0)
        return false;
    CPathfindInformation* pPathInfo = pCreature->m_pPathfindInfo;
    if (pPathInfo == 0)
        return false;

    int count = which == 0 ? pPathInfo->m_nWayPointFloats : pPathInfo->m_nRawWayPointFloats;
    bool result = true;
    if (count != 0)
    {
        float* pPoints = which == 0 ? pPathInfo->m_paWayPoints : pPathInfo->m_paRawWayPoints;
        if (pPoints != 0)
        {
            Vector cur;
            cur.x = pPoints[0];
            cur.y = pPoints[1];
            cur.z = 0.0f;
#if KOTOR1  // K1 has no room-hint cache
            int roomHint = -1;
            cur.z = pArea->ComputeHeight(cur, 1, 0);
#else
            int roomHint = pCreature->m_lastRoomHint;
            cur.z = pArea->ComputeHeight(cur, 1, 0, &roomHint);
#endif
            pArea->m_pActivePathfindInfo = pPathInfo;
            CSWSRoom* pRoom = pArea->GetRoom(cur, 0, &roomHint);
            bool roomOk = pRoom != 0;
            int walkable = pArea->PositionWalkable(cur);
            result = roomOk & (walkable != 0);
            Vector prev = cur;

            if (count > 2)
            {
                float* p = pPoints + 2;
                int i = 2;
                do
                {
                    cur.x = p[0];
                    cur.y = p[1];
                    cur.z = 0.0f;
                    cur.z = pArea->ComputeHeight(cur, 1, 0, &roomHint);
                    CSWSRoom* pRoom2 = pArea->GetRoom(cur, 0, &roomHint);
                    bool roomOk2 = pRoom2 != 0;
                    int walkable2 = pArea->PositionWalkable(cur);
                    bool posOk = roomOk2 & (walkable2 != 0);
                    bool segOk = DebugValidatePath_Segment(pArea, pPathInfo, prev, cur);
                    result = segOk & result;
                    i += 2;
                    result = result & posOk;
                    prev = cur;
                    p += 2;
                } while (count > i);
            }
            pArea->m_pActivePathfindInfo = pPathInfo;
        }
    }
    return result;
}

bool CServerExoAppInternal::DebugValidatePath_Segment(CSWSArea* pArea, CPathfindInformation* pPathInfo,
                                                        const Vector& a, const Vector& b)
{
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;

    int status;
    {
        int flagValue = pPathInfo->m_nPersonalSpaceBits;
        status = pArea->TestDirectLine(a, b, *reinterpret_cast<const float*>(&flagValue), pPathInfo->m_fHeightAlias, 1, &hitInfo);
    }

    if (status > 0)
        return true;
    int savedStatus = status;
    if (status == -3)
        return true;
    if (savedStatus != -2)
        return false;

    CGameObject* pFound = 0;
    m_pObjectArray->GetGameObject(hitInfo.m_hitObjectId, &pFound);
    CGameObject* pObject = pFound;
    if (pObject == 0)
        return true;

    bool result;
    if (pObject->AsSWSDoor() == 0)
    {
        result = false;
        savedStatus = -2;
    }
    else
    {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        bool combatOk = pDoor->m_bStatic == 0;
        result = combatOk;
        savedStatus = combatOk ? 1 : -2;
    }

    void* p3 = pObject->AsSWSPlaceable();
    if (p3 != 0)
    {
        CSWSPlaceable* p4 = pObject->AsSWSPlaceable();
        if (p4->m_objectId == pPathInfo->m_oidGoalObject)
            return true;
    }
    if (savedStatus == -2)
        return true;
    return result;
}

void* CServerExoAppInternal::GetClientObjectByPlayerId(unsigned int playerId, unsigned char scope)
{
    if (scope < 2)
    {
        CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
        if (pNode != 0)
        {
            do
            {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
                if (pPlayer->m_nPlayerId == playerId)
                    return pPlayer;
                m_pPlayerList->GetNext(pNode);
            } while (pNode != 0);
        }
    }
    if (scope == 0 || scope == 2)
    {
        CExoLinkedListNode* pNode = m_pSysAdminList->m_pInternal->m_pHead;
        if (pNode != 0)
        {
            do
            {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pSysAdminList->GetAtPos(pNode));
                if (pPlayer->m_nPlayerId == playerId)
                    return pPlayer;
                m_pSysAdminList->GetNext(pNode);
            } while (pNode != 0);
        }
    }
    return 0;
}

int CServerExoAppInternal::IncludeModuleInSave(const CExoString& moduleName)
{
    C2DA* pC2da = new C2DA(CResRef("modulesave"), 0);
    if (pC2da->Load2DArray() != 0)
    {
        int value;
        int found;
        {
            CExoString colName("IncludeInSave");
            found = pC2da->GetINTEntry(moduleName, colName, &value);
        }
        delete pC2da;
        if (found != 0)
            return value != 0;
    }
    else
    {
        delete pC2da;
    }
    return 1;
}

int CServerExoAppInternal::StoreCurrentModule()
{
    CSWSModule* pModule = GetModule();
    if (pModule == 0)
        return 1;

    int included;
    {
        CExoString resourceName = pModule->GetModuleResourceName();
        included = IncludeModuleInSave(resourceName);
    }
    if (!included)
        return 1;

    CExoString resourceName("GAMEINPROGRESS:");
    CExoString tag(pModule->m_sModuleResourceName);
    int idx = tag.Find(':', 0);
    if (idx != -1)
    {
#if KOTOR1  // K1 calls GetLength()
        tag = resourceName + tag.Right(tag.GetLength() - idx - 1);
#else
        int len = tag.GetLength();
        CExoString subTag = tag.Right(len - idx - 1);
        tag = resourceName + subTag;
#endif
    }
    else
    {
        tag = resourceName + tag;
    }

    pModule->SaveModuleStart(resourceName, tag);
    pModule->SaveModuleInProgress();
    pModule->SaveModuleFinish(resourceName, tag);

    return 1;
}

int CServerExoAppInternal::UpdateShutdownTimer(uint64_ll_t now)
{
    if (m_nShutdownTimeRemaining == 0)
        return 0;

    unsigned long long elapsed = now - m_nShutdownLastTick;
    unsigned long long remaining = m_nShutdownTimeRemaining - elapsed;
    if (m_nShutdownTimeRemaining < elapsed)
    {
        m_nShutdownTimeRemaining = 0;
        return 1;
    }

    if ((m_nShutdownTimeRemaining > 60000000 && remaining < 60000000) ||
        (m_nShutdownTimeRemaining > 30000000 && (m_nShutdownTimeRemaining - now + m_nShutdownLastTick) < 30000000))
    {
        if (m_aShutdownPlayerIds.m_count > 0)
        {
            int i = 0;
            do
            {
                CSWSMessage* pMsg = m_pSWSMessage;
                unsigned int secondsRemaining = static_cast<unsigned int>((m_nShutdownTimeRemaining - (now - m_nShutdownLastTick)) / 1000000);
                pMsg->SendServerToPlayerShutDownServer(static_cast<unsigned int>(m_aShutdownPlayerIds[i]), secondsRemaining);
                i++;
            } while (i < m_aShutdownPlayerIds.m_count);
        }
    }

    m_nShutdownTimeRemaining = m_nShutdownLastTick - now + m_nShutdownTimeRemaining;
    m_nShutdownLastTick = now;
    return 0;
}

void CServerExoAppInternal::Shutdown(int a, int b)
{
    unsigned int flag = m_pServerInfo->m_bWarnPlayersOnShutdown;
    m_aShutdownPlayerIds.SetSize(0);

    CExoLinkedListNode* pos = m_pPlayerList->GetHeadPos();
    bool graceful = flag != 0 && a != 0;

    if (graceful)
    {
        if (pos != 0)
        {
            do
            {
                CSWSPlayer* pPlayer = reinterpret_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pos));
                if (pPlayer != 0)
                {
                    m_pSWSMessage->SendServerToPlayerShutDownServer(pPlayer->m_nPlayerId, static_cast<unsigned int>(m_nShutdownGracePeriod / 100000));
                    m_aShutdownPlayerIds.Add(pPlayer->m_nPlayerId);
                }
                m_pPlayerList->GetNext(pos);
            } while (pos != 0);
        }
    }
    else
    {
        int guard = 256;
        while (pos != 0 && --guard != 0)
        {
            CSWSPlayer* pPlayer = reinterpret_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pos));
            if (pPlayer == 0)
            {
                m_pPlayerList->GetNext(pos);
            }
            else if (b != 0)
            {
                m_aShutdownPlayerIds.Add(pPlayer->m_nPlayerId);
                m_pPlayerList->GetNext(pos);
            }
            else
            {
                pos = m_pPlayerList->m_pInternal->m_pHead;  // sic: rewinds to the head
            }
        }
    }

    pos = m_pSysAdminList->m_pInternal->m_pHead;
    {
        int guard = 256;
        while (pos != 0 && --guard != 0)
        {
            CSWSPlayer* pAdmin = reinterpret_cast<CSWSPlayer*>(m_pSysAdminList->GetAtPos(pos));
            if (pAdmin == 0)
            {
                m_pSysAdminList->GetNext(pos);
            }
            else
            {
                if (m_aShutdownPlayerIds.Contains(pAdmin->m_nPlayerId) != 0)
                    m_pSysAdminList->GetNext(pos);
                else
                    pos = m_pSysAdminList->m_pInternal->m_pHead;  // sic: rewinds to the head
            }
        }
    }

    unsigned long long duration;
    if (graceful && m_aShutdownPlayerIds.m_count != 0)
        duration = 180000000;
    else
        duration = 1000000;
    uint64_ll_t now = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_nShutdownTimeRemaining = duration;
    m_nShutdownLastTick = now;
}

// Passes through CSWSArea::ShrinkLineByCreatures' result.
int CServerExoAppInternal::ShrinkLineByCreatures(const Vector& from, const Vector& to, Vector& out)
{
    CSWSModule* pModule = GetModule();
    CSWSArea* pArea = pModule->GetArea();
    return pArea->ShrinkLineByCreatures(from, to, out);
}

// Debug console handler ("warpforward <dist>"): pushes a move action onto the front of the player
// creature's action queue, targeting the point <dist> along its facing vector (default 5.0).
char* warpforward(char* s)
{
    float fDist = static_cast<float>(atof(s));
    if (fDist <= 0.0f)
        fDist = 5.0f;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer != 0)
    {
        CSWSPlayer* pPlayer = pServer->GetClientObjectByObjectId(pServer->GetPlayerCreatureId());
        if (pPlayer != 0)
        {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && pObject->GetArea() != 0)
            {
                Vector vTarget = pObject->m_position;
                Vector vOrient = pObject->m_orientation;
                vTarget.x = vOrient.x * fDist + vTarget.x;
                vTarget.y = vOrient.y * fDist + vTarget.y;
                vTarget.z = vOrient.z * fDist + vTarget.z;
                unsigned long long areaId = static_cast<CSWSArea*>(pObject->GetArea())->m_objectId;
                int nOne = 1;
                int nZero = 0;
                pObject->AddActionToFront(AIACTION_JUMP_TO_POINT, ACTION_GROUP_NEW,
                                          ACTION_PARAM_FLOAT, &vTarget.x,
                                          ACTION_PARAM_FLOAT, &vTarget.y,
                                          ACTION_PARAM_FLOAT, &vTarget.z,
                                          ACTION_PARAM_OBJECT, &areaId, ACTION_PARAM_INT, &nOne,
                                          ACTION_PARAM_FLOAT, &nZero,
                                          ACTION_PARAM_FLOAT, &vOrient.x,
                                          ACTION_PARAM_FLOAT, &vOrient.y, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0);
                return const_cast<char*>("Success!");
            }
        }
    }
    return const_cast<char*>("Failure!");
}

int CServerExoAppInternal::ExportAllPlayers()
{
    CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
    if (pNode != 0)
    {
        do
        {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
            if (pPlayer->m_state == 3 || pPlayer->m_state == 4)
                pPlayer->SaveServerCharacter();
            else if (pPlayer->m_state == 1)
                m_pSWSMessage->SendServerToPlayerModule_ExportReply(pPlayer);
            m_pPlayerList->GetNext(pNode);
        } while (pNode != 0);
    }
    return 1;
}

CSWSPlayer* CServerExoAppInternal::GetClientObjectByObjectId(unsigned long long id)
{
    CSWSPlayer* pFound = 0;
    if (id != OBJECT_INVALID)
    {
        CExoLinkedListNode* pNode = m_pPlayerList->m_pInternal->m_pHead;
        if (pNode != 0)
        {
            while (1)
            {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pNode));
                if (pPlayer != 0 && pPlayer->GetGameObject() != 0 &&
                    pPlayer->GetGameObject()->m_objectId == id)
                {
                    pFound = pPlayer;
                    break;
                }
                m_pPlayerList->GetNext(pNode);
                if (pNode == 0)
                    break;
            }
        }
    }
    return pFound;
}

int CServerExoAppInternal::ChangePlayerGameObject(unsigned long long id)
{
    CGameObject* pObject = 0;
    unsigned long long moduleId = OBJECT_INVALID;
    if (id != OBJECT_INVALID) {
        const unsigned int nIdBits = ~CGameObjectArray::LISTTYPE_MASK;
        moduleId = nIdBits & id;
    }
    m_pObjectArray->GetGameObject(moduleId, &pObject);
    if (pObject == 0)
        return 0;

    CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1));
    if (pPlayer == 0)
        return 0;

    CSWSObject* pGameObj = pObject->AsSWSObject();
    pPlayer->SetGameObject(pGameObj);
    return 1;
}

int CServerExoAppInternal::HasEnoughDiskSpaceForSaveGame()
{
    CExoResMan* pResMan = g_pExoResMan;
    uint64_ll_t freeSpace = 0;
    {
        CExoString path("SAVES:");
        pResMan->GetFreeDiskSpace(path, &freeSpace);
    }
    return (freeSpace >> 14) > SAVE_MIN_FREE_SPACE_16K;
}

void CServerExoAppInternal::DumpServerOptionsToLog()
{
    CExoString timestamp;
    g_pExoBase->m_pDebug->GetCurrentTimestamp(timestamp);
    timestamp.CStr();
}

void CServerExoAppInternal::StartServices(int a)
{
    K2_UNUSED(a);
    CVirtualMachine* pVM;
    CSWVirtualMachineCommands* pCmdImpl;
    uint64_ll_t hiResTimer;
    CExoString timestamp;
    g_pExoBase->m_pDebug->GetCurrentTimestamp(timestamp);
    timestamp.CStr();

    m_pFactionManager = new CFactionManager();
    m_pWorldTimer = new CWorldTimer();
    m_pTimestopTimer = new CWorldTimer();
    m_pPauseTimer = new CWorldTimer();
    m_pObjectArray = new CGameObjectArray(1);

    if (g_nClientServerMode == 2)
    {
        g_pExoResMan = new CExoResMan();
    }

    m_pNetLayer = new CNetLayer();

    if (m_pServerInfo == 0)
    {
        m_pServerInfo = new CServerInfo();
    }

    m_pSWSMessage = new CSWSMessage();

    m_pServerAIMaster = new CServerAIMaster();
    m_pServerAIMaster->SetExoAppInternal(this);

    if (g_nClientServerMode == 2)
    {
        g_pExoResMan->AddResourceDirectory(CExoString("OVERRIDE:"));
        g_pExoResMan->AddResourceDirectory(CExoString("ERRORTEX:"));
        g_pExoResMan->AddFixedKeyTableFile(CExoString("HD0:chitin"));
        g_pExoResMan->AddResourceDirectory(CExoString("SERVERVAULT:"));
    }

    // 86 is no resource type (it gets the table's empty extension), so this names the TEMP directory
    // itself, which RemoveFile refuses to delete: a no-op in both games.
    g_pExoResMan->RemoveFile(CExoString("HD0:TEMP"), 0x56);

    g_pExoResMan->CreateDirectory2(CExoString("TEMP:"));
    g_pExoResMan->CreateDirectory2(CExoString("GAMEINPROGRESS:"));

    if (g_pTlkTable == 0)
    {
        g_pTlkTable = new CTlkTable();
        g_pTlkTable->OpenFile(CExoString("HD0:dialog"));
    }

    pVM = new CVirtualMachine();
    g_pVirtualMachine = pVM;

    pCmdImpl = new CSWVirtualMachineCommands;
    m_pVirtualMachineCommands = pCmdImpl;
    pVM->SetCommandImplementer(pCmdImpl);

    if (g_nClientServerMode != 2)
    {
        g_pExoBase->m_pDebug->OpenLogFiles(CExoString("swserver"));
    }

    m_nShutdownGracePeriod = 180000000;
    m_pNetLayer->Initialize(reinterpret_cast<CBaseExoApp*>(g_pAppManager->m_pServerExoApp));

    GetServerInfoFromIniFile();

    if (g_nClientServerMode == 2)
    {
        g_pRules = new CSWSRules();
    }
    else
    {
        DumpServerOptionsToLog();
    }

    m_logHeartbeatLastTime = 0;
    m_logHeartbeatInterval = 0;

    hiResTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_tempDeathReviveAccum = 0;
    m_tempDeathCheckAccum = 0;
    m_tempDeathLastUpdateTime = static_cast<unsigned int>(hiResTimer / 1000);
    m_autoSaveTimerDay = 0;
    m_autoSaveTimerFlag = 0;
}

void CServerExoAppInternal::StopServices()
{
    CGameObjectArray* pObjectArray;
    CServerAIMaster* pServerAIMaster;
    CExoString timestamp;
    g_pExoBase->m_pDebug->GetCurrentTimestamp(timestamp);
    timestamp.CStr();

    Shutdown(0, 0);

    pObjectArray = m_pObjectArray;
    if (pObjectArray != 0)
    {
        pObjectArray->~CGameObjectArray();
        operator delete(pObjectArray);
        m_pObjectArray = 0;
    }

    pServerAIMaster = m_pServerAIMaster;
    if (pServerAIMaster != 0)
    {
        pServerAIMaster->~CServerAIMaster();
        operator delete(pServerAIMaster);
        m_pServerAIMaster = 0;
    }

#if !KOTOR1
    if (m_pVirtualMachineCommands != 0)
    {
        delete m_pVirtualMachineCommands;
        m_pVirtualMachineCommands = 0;
    }
#endif

    if (m_pSWSMessage != 0)
    {
        delete m_pSWSMessage;
        m_pSWSMessage = 0;
    }

    if (m_pServerInfo != 0)
    {
        delete m_pServerInfo;
        m_pServerInfo = 0;
    }

    if (m_pNetLayer != 0)
    {
        m_pNetLayer->EndServerMode();
        m_pNetLayer->ShutDown();
    }
    CNetLayer* pNetLayer = m_pNetLayer;
    if (pNetLayer != 0)
    {
        pNetLayer->~CNetLayer();
        operator delete(pNetLayer);
    }
    m_pNetLayer = 0;

    CVirtualMachine* pVirtualMachine = g_pVirtualMachine;
    if (pVirtualMachine != 0)
    {
        pVirtualMachine->~CVirtualMachine();
        operator delete(pVirtualMachine);
        g_pVirtualMachine = 0;
    }

#if KOTOR1  // K1 deletes the command implementer after the virtual machine
    if (m_pVirtualMachineCommands != 0)
    {
        delete m_pVirtualMachineCommands;
        m_pVirtualMachineCommands = 0;
    }
#endif

    CScriptCompiler* pScriptCompiler = g_pScriptCompiler;
    if (pScriptCompiler != 0)
    {
        pScriptCompiler->~CScriptCompiler();
        operator delete(pScriptCompiler);
        g_pScriptCompiler = 0;
    }

    CExoResMan** ppExoResMan = &g_pExoResMan;
    (*ppExoResMan)->NukeDirectory(CExoString("TEMP:"), 1, 1);
    (*ppExoResMan)->NukeDirectory(CExoString("CURRENTGAME:"), 1, 1);
    (*ppExoResMan)->NukeDirectory(CExoString("GAMEINPROGRESS:"), 1, 1);

    // Only a standalone server owns the rules, key tables, TLK and resource manager; a
    // client-hosted server shares the client's.
    if (g_nClientServerMode == 2)
    {
        if (g_pRules != 0)
        {
            delete g_pRules;
            g_pRules = 0;
        }

        (*ppExoResMan)->RemoveFixedKeyTableFile(CExoString("HD0:chitin"));
        (*ppExoResMan)->RemoveResourceDirectory(CExoString("OVERRIDE:"));
        (*ppExoResMan)->RemoveResourceDirectory(CExoString("ERRORTEX:"));
        (*ppExoResMan)->RemoveResourceDirectory(CExoString("SERVERVAULT:"));

        CTlkTable* pTlkTable = g_pTlkTable;
        if (pTlkTable != 0)
        {
            pTlkTable->CTlkTable::~CTlkTable();
            operator delete(pTlkTable);
            g_pTlkTable = 0;
        }

        CExoResMan* pExoResMan = *ppExoResMan;
        if (pExoResMan != 0)
        {
            pExoResMan->~CExoResMan();
            operator delete(pExoResMan);
            *ppExoResMan = 0;
        }
    }

    CWorldTimer* pWorldTimer = static_cast<CWorldTimer*>(m_pWorldTimer);
    if (pWorldTimer != 0)
    {
        pWorldTimer->~CWorldTimer();
        operator delete(pWorldTimer);
        m_pWorldTimer = 0;
    }

    CWorldTimer* pTimestopTimer = static_cast<CWorldTimer*>(m_pTimestopTimer);
    if (pTimestopTimer != 0)
    {
        pTimestopTimer->~CWorldTimer();
        operator delete(pTimestopTimer);
        m_pTimestopTimer = 0;
    }

    CWorldTimer* pPauseTimer = m_pPauseTimer;
    if (pPauseTimer != 0)
    {
        pPauseTimer->~CWorldTimer();
        operator delete(pPauseTimer);
        m_pPauseTimer = 0;
    }

    CFactionManager* pFactionManager = m_pFactionManager;
    if (pFactionManager != 0)
    {
        pFactionManager->~CFactionManager();
        operator delete(pFactionManager);
        m_pFactionManager = 0;
    }
}

int CServerExoAppInternal::LoadPrimaryPlayer(CSWSPlayer* pPlayer)
{
    int idx = g_pAppManager->m_pServerExoApp->GetModule()->GetPrimaryPlayerIndex();
    int success;
    if (idx != -1)
    {
        pPlayer->m_playerIndex = idx;
        success = 1;
        LoadCharacterFinish(pPlayer, 1);
    }
    else
    {
        success = 0;
    }
    return success;
}

CSWSModule* CServerExoAppInternal::GetModuleByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSModule* pModule = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE)
        {
            pModule = pObject->AsSWSModule();
        }
    }
    return pModule;
}

CSWSArea* CServerExoAppInternal::GetAreaByGameObjectID(unsigned long long id)
{
    if (m_areaCacheId != id)
    {
#if defined(_MSC_VER) || !defined(__SIZEOF_INT128__)
        m_areaCacheId = OBJECT_INVALID;
        m_pCachedArea = 0;
#else
        // raw-offset: the id / pointer pair is cleared as one 16-byte store (two typed stores schedule differently)
    *reinterpret_cast<unsigned __int128*>(&m_areaCacheId) = OBJECT_INVALID;
#endif
        CGameObject* pObject = 0;
        if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0 &&
            pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
        {
            m_areaCacheId = id;
            m_pCachedArea = reinterpret_cast<unsigned long long>(pObject->AsSWSArea());
        }
    }
    return reinterpret_cast<CSWSArea*>(m_pCachedArea);
}

CSWSCreature* CServerExoAppInternal::GetCreatureByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSCreature* pCreature = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        {
            pCreature = pObject->AsSWSCreature();
        }
    }
    return pCreature;
}

CSWSItem* CServerExoAppInternal::GetItemByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSItem* pItem = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
        {
            pItem = pObject->AsSWSItem();
        }
    }
    return pItem;
}

CSWSTrigger* CServerExoAppInternal::GetTriggerByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSTrigger* pTrigger = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        {
            pTrigger = pObject->AsSWSTrigger();
        }
    }
    return pTrigger;
}

CSWSPlaceable* CServerExoAppInternal::GetPlaceableByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSPlaceable* pPlaceable = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            pPlaceable = pObject->AsSWSPlaceable();
        }
    }
    return pPlaceable;
}

CSWSDoor* CServerExoAppInternal::GetDoorByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSDoor* pDoor = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
        {
            pDoor = pObject->AsSWSDoor();
        }
    }
    return pDoor;
}

CSWSAreaOfEffectObject* CServerExoAppInternal::GetAreaOfEffectByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSAreaOfEffectObject* pAreaOfEffect = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT)
        {
            pAreaOfEffect = pObject->AsSWSAreaOfEffectObject();
        }
    }
    return pAreaOfEffect;
}

CSWSWaypoint* CServerExoAppInternal::GetWaypointByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSWaypoint* pWaypoint = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_WAYPOINT)
        {
            pWaypoint = pObject->AsSWSWaypoint();
        }
    }
    return pWaypoint;
}

CSWSEncounter* CServerExoAppInternal::GetEncounterByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSEncounter* pEncounter = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ENCOUNTER)
        {
            pEncounter = pObject->AsSWSEncounter();
        }
    }
    return pEncounter;
}

CSWSStore* CServerExoAppInternal::GetStoreByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSStore* pStore = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_STORE)
        {
            pStore = pObject->AsSWSStore();
        }
    }
    return pStore;
}

CSWSSoundObject* CServerExoAppInternal::GetSoundObjectByGameObjectID(unsigned long long id)
{
    CGameObject* pObject = 0;
    CSWSSoundObject* pSoundObject = 0;
    if (m_pObjectArray->GetGameObject(id, &pObject) == CGameObjectArray::SUCCESS && pObject != 0)
    {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_SOUND)
        {
            pSoundObject = pObject->AsSWSSoundObject();
        }
    }
    return pSoundObject;
}
int CServerExoAppInternal::AutoSaveOnEnteringModule(const CExoString& moduleName, C2DA* p2da)
{
    CExoString value;
    int found;
    {
        CExoString colName("AutoSaveOnEnter");
        found = p2da->GetCExoStringEntry(moduleName, colName, &value);
    }

    int result;
    if (found == 0 || value.CompareNoCase("force") != 0)
    {
        result = 1;
    }
    else if (value.CompareNoCase("no") == 0 && m_autoSaveTimerFlag != 0 &&
             (value.CompareNoCase("yes") == 0 ||
              g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoSave))
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    if (result)
    {
        m_autoSaveTimerDay = 0;
        m_autoSaveTimerFlag = 0;
    }
    return result;
}

void CServerExoAppInternal::MoveCreatureToPosition(unsigned long long id, const Vector& pos, unsigned long long areaId)
{
    unsigned long long maskedAreaId = areaId & 0x7fffffffffffffffULL;
    CGameObject* pObject = 0;
    if (m_pObjectArray->GetGameObject(id & 0x7fffffffffffffffULL, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0)
    {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0)
        {
            pCreature->ClearAllActions(1);
            int one = 1;
            unsigned long long noAreaSentinel = OBJECT_INVALID;
            pCreature->m_pPathfindInfo->m_bForceMoveDeadline = 0;
            pCreature->m_pPathfindInfo->m_nForceMoveDeadlineDay = 0;
            pCreature->m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay = 0;
            pCreature->AddAction(AIACTION_MOVE_TO_POINT, ACTION_GROUP_NEW,
                ACTION_PARAM_FLOAT, const_cast<Vector*>(&pos),
                ACTION_PARAM_FLOAT, const_cast<float*>(&pos.y),
                ACTION_PARAM_FLOAT, const_cast<float*>(&pos.z), ACTION_PARAM_OBJECT, &maskedAreaId,
                ACTION_PARAM_OBJECT, &noAreaSentinel, ACTION_PARAM_INT, &one, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0);
        }
    }
}

int CServerExoAppInternal::TestSafeLocation(unsigned long long id, const Vector& pos)
{
    CSWSCreature* pCreature = GetCreatureByGameObjectID(id);
    if (pCreature == 0)
        return 0;
    return GetAreaByGameObjectID(pCreature->m_cachedAreaId)
        ->TestSafeLocationPoint(pos, pCreature->m_pPathfindInfo);
}

void CServerExoAppInternal::GetServerInfoFromIniFile()
{
    CExoIni ini;
    CExoString a, b, c;
    CNetLayer* pNetLayer = m_pNetLayer;

    {
        CExoString name("Server");
        pNetLayer->StartServerMode(name, MAX_SERVER_PLAYERS);
    }

    m_pServerInfo->m_nMaxPlayers = MAX_SERVER_PLAYERS;
    m_pServerInfo->SetDifficultyLevel(2);
    m_pServerInfo->m_bPauseAndPlay = 1;
    m_pServerInfo->m_bAutoSaveAtIntervals = 0;
    m_pServerInfo->m_nAutoSaveInterval = 0;
    m_autoSaveTimerDay = 0;
    m_pServerInfo->m_bWarnPlayersOnShutdown = 1;
    m_pServerInfo->m_bItemLevelRestrictions = 1;
}

void CServerExoAppInternal::DoModuleEnterSaveCleanup(const CExoString& moduleName, C2DA* p2da)
{
    int found;
    int saveGroupId;
    {
        CExoString colName("DeleteSaveGroupOnEnter");
        found = p2da->GetINTEntry(moduleName, colName, &saveGroupId);
    }
    if (found != 0)
    {
        CExoString gamePrefix("GAMEINPROGRESS:");
        CExoString moduleFile;
        if (p2da->m_nRows > 0)
        {
            for (int i = 0; i < p2da->m_nRows; i++)
            {
                int rowFound;
                int rowSaveGroup;
                {
                    CExoString colName("SaveGroup");
                    rowFound = p2da->GetINTEntry(i, colName, &rowSaveGroup);
                }
                if (rowFound != 0 && rowSaveGroup == saveGroupId)
                {
                    int nameFound;
                    {
                        CExoString colName("ModuleName");
                        nameFound = p2da->GetCExoStringEntry(i, colName, &moduleFile);
                    }
                    CExoResMan* resMan = g_pExoResMan;
                    if (nameFound != 0)
                    {
                        CExoString fullPath = gamePrefix + moduleFile;
                        resMan->RemoveFile(fullPath, RESTYPE_SAV);
                    }
                }
            }
        }
    }
}

// Returns the area's ComputeSafeLocation result.
int CServerExoAppInternal::ComputeSafeLocation(const Vector& pos, float radius, unsigned long long id,
                                                int flag, Vector* out)
{
    CSWSCreature* pCreature = GetCreatureByGameObjectID(id);
    if (pCreature == 0)
        return 0;
    return GetAreaByGameObjectID(pCreature->m_cachedAreaId)
        ->ComputeSafeLocation(pos, radius, pCreature->m_pPathfindInfo, flag, out);
}

void CServerExoAppInternal::AddCharListRequest(unsigned int id)
{
    m_aCharListRequests.AddUnique(id);
}

// Writes every player's active creature into a temp GFF and re-numbers each player's save-slot
// index.
int CServerExoAppInternal::StorePlayerCharacters()
{

    CExoLinkedList<CSWSClient>* pPlayerList = m_pPlayerList;
    CResGFF gff;
    CResList list;
    CExoLinkedListNode* pos;
    CResStruct rootStruct;
    CResStruct elementStruct;
    int created;
    int count;
    void* pNode;
    CSWSPlayer* pTarget;
    CSWSCreature* pCreature;
    {
        CExoString fileType("IFO ");
        CExoString version("V2.0");
        created = gff.CreateGFFFile(&rootStruct, fileType, version);
    }

    int result;
    if (created)
    {
        gff.AddList(&list, &rootStruct, const_cast<char*>("Mod_PlayerList"));

        pos = pPlayerList->GetHeadPos();
        count = 0;
        while (pos != 0)
        {
            pNode = pPlayerList->GetAtPos(pos);
            pTarget = static_cast<CSWSClient*>(pNode)->AsSWSPlayer();
            if (pTarget != 0)
            {
                pTarget->m_saveListIndex = -1;
                pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                    pTarget->m_gameObjectId);
                if (pCreature != 0)
                {
                    gff.AddListElement(&elementStruct, &list, GFF_STRUCT_ID_PARTY_CREATURE);
                    pCreature->ClearAllActions(1);
                    pCreature->SaveCreature(&gff, &elementStruct);
                    pTarget->m_saveListIndex = count;
                    count = count + 1;
                }
            }
            pPlayerList->GetNext(pos);
        }

        if (count < 1)
        {
            result = 0;
        }
        else
        {
            CExoString ext;
            CExoString prefix;
            ext = "pifo";
            prefix = "TEMP:";
            prefix = prefix + ext;
            CExoString path(prefix.CStr());
            gff.WriteGFFFile(path, RESTYPE_IFO);
            result = 1;
        }

        m_partyTable.UpdateMembers(1);
        m_partyTable.UpdateInventory(1);
    }
    else
    {
        result = 0;
    }
    return result;
}

int CServerExoAppInternal::StartNewModule(const CExoString& moduleName)
{
    if (__builtin_expect(m_loadingModule == 1, 1))
        return 0;

    g_pAppManager->m_pClientExoApp->SetLoadScreenHint(
        g_pAppManager->m_pClientExoApp->GetNextLoadScreenHintSTRREF(moduleName, 0));
    g_pAppManager->m_pClientExoApp->SetLoadScreenHintUpdateTimer(0);
    g_pAppManager->m_pClientExoApp->SetCanSendDialog(0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->CloseDialog();
    g_pExoSound->StopAllOneShots();
    g_pAppManager->m_pClientExoApp->PlayLoadModuleMovies();

    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
#if !KOTOR1  // K2 only adds the module override directories
    AddModuleOverrideResourceDirectories();
#endif
    if (!g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_MOD, static_cast<unsigned int*>(0)))
    {
        if (!g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        {
            g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
#if !KOTOR1  // K2 only adds the module override directories
            RemoveModuleOverrideResourceDirectories();
#endif
            return 0;
        }
    }
    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
#if !KOTOR1  // K2 only adds the module override directories
    RemoveModuleOverrideResourceDirectories();
#endif

    m_loadingModule = 1;
    m_doingStartNewModule = 1;
    ForceRunAreaOnExitScripts();

    if (StorePlayerCharacters() != 0)
    {
        StoreCurrentModule();

        C2DA* pC2da = new C2DA(CResRef("modulesave"), 0);
        if (pC2da->Load2DArray() == 0)
        {
            delete pC2da;
            pC2da = 0;
        }
        DoModuleEnterSaveCleanup(moduleName, pC2da);

        CSWSModule* pModule = GetModule();

        if (pModule != 0)
        {
            if (pC2da == 0 || AutoSaveOnEnteringModule(moduleName, pC2da) != 0)
                DoPCAutosave(moduleName, pC2da);
        }
        if (pC2da != 0)
        {
            delete pC2da;
        }
    }
    else
    {
        m_doingStartNewModule = 0;
        return 0;
    }

    ExecuteLoadModule(moduleName);
    // The success path falls off the end without an explicit return; returning keeps execution
    // from running on into the next function.
    return 1;
}

int CServerExoAppInternal::LoadCharacterFinish(CSWSPlayer* pPlayer, int a)
{

    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CScriptEvent* pEvent1 = new CScriptEvent();

    if (pPlayer->m_connectionState != 1)
    {
        pPlayer->m_connectionState = 1;
        m_pSWSMessage->SendServerToPlayerLogin_Confirm(pPlayer->m_nPlayerId);
        m_pSWSMessage->SendServerToPlayerModule_Info(pPlayer->m_nPlayerId);
    }

    if (a != 0 && pPlayer->m_playerIndex != -1)
    {
        CSWSObject* pGameObj = pPlayer->GetGameObject();
        if (pGameObj != 0)
        {
            delete pGameObj;
        }
        pPlayer->LoadCharacterFromIFO(pPlayer->m_playerIndex, 1, 1);
    }

    pEvent1->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_ENTER;

    CScriptEvent* pEvent2;
    if (pPlayer->GetGameObject() == 0)
    {
        pEvent1->SetObjectID(0, OBJECT_INVALID);
        pEvent2 = 0;
    }
    else
    {
        if (pPlayer->GetGameObject()->AsSWSCreature()->GetDead() == 1
            || pPlayer->GetGameObject()->AsSWSCreature()->GetDeadTemp() == 1)
        {
            pEvent2 = new CScriptEvent();
            pEvent2->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH;
        }
        else
        {
            pEvent2 = 0;
        }

        CGameObject* pGameObj3 = pPlayer->GetGameObject();
        pEvent1->SetObjectID(0, pGameObj3->m_objectId);

        CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
        CGameObject* pGameObj4 = pPlayer->GetGameObject();
        CSWSCreature* pMyCreature = pGameObj4->AsSWSCreature();

        CExoLinkedListNode* pNode = pList->GetHeadPos();
        while (pNode != 0)
        {
            CSWSClient* pElem = pList->GetAtPos(pNode);
            if (pElem != 0 && static_cast<CSWSPlayer*>(pElem)->m_bPlayModuleListingCharacters != 0 && pMsg != 0)
            {
                pMsg->SendServerToPlayerPlayModuleCharacterListResponse(
                    pElem->m_nPlayerId,
                    pMyCreature->m_objectId, 1);
            }
            pList->GetNext(pNode);
        }
    }

    if (pModule == 0)
        return 1;

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, static_cast<unsigned long long>(static_cast<unsigned int>(pPlayer->m_nPlayerId)),
        pModule->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent1);

    if (pEvent2 != 0)
    {
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, pPlayer->GetGameObject()->m_objectId,
            pModule->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent2);
    }

    CSWSCreature* pCreature3 = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
    if (pCreature3 != 0)
    {
        C2DA* pTable = g_pRules->m_p2DArrays->m_pAppearance;
        unsigned short headId = pCreature3->m_appearance.m_headId;
        int headResult;
        {
            CExoString colName("NORMALHEAD");
            pTable->GetINTEntry(headId, colName, &headResult);
        }
        pModule->GetArea()->EnforceNoPCHeadCloning(headResult);
    }

    return 1;
}

void CServerExoAppInternal::TempDeathUpdate()
{

    if (g_pAppManager->m_pClientExoApp->GetGameOver())
        return;
    if (g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay())
        return;
#if !KOTOR1  // K1 has no modal-panel guard
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_modalPanels.m_count >= 1)
        return;
#endif

    {
        unsigned int oldTime = m_tempDeathLastUpdateTime;
        unsigned int newTime = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
        m_tempDeathLastUpdateTime = newTime;

        int count = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;

        bool allDead = true, anyDead = false;
        CSWSObject* pLastCreature;  // sic: uninitialized when the party is empty; only read when anyDead is set
#ifdef K2_FIX_ENGINE_BUGS
        pLastCreature = 0;
#endif
        for (int i = 0; i < count; i++)
        {
            CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
            if (pChar == 0)
                continue;
            CSWSCreature* pServerCreature = pChar->GetServerCreature();
#ifdef K2_FIX_ENGINE_BUGS
            if (pServerCreature != 0)
#endif
            pLastCreature = pServerCreature;
            if (pServerCreature == 0)
            {
                allDead = false;
                continue;
            }
            bool dead = pServerCreature->GetDeadTemp() != 0;
            if (dead)
                anyDead = true;
            else
                allDead = false;
        }

        if (count != 0 && allDead)
            g_pAppManager->m_pClientExoApp->RunDeathSequence();

        if (!anyDead)
        {
            m_tempDeathReviveAccum = 0;
            return;
        }

        {
            int deltaTime = newTime - oldTime;
            unsigned int checkAccum = m_tempDeathCheckAccum + deltaTime;
            m_tempDeathCheckAccum = checkAccum;
            if (checkAccum > 1000)
            {
                m_tempDeathCheckAccum = 0;

                CSWSArea* pArea = static_cast<CSWSArea*>(pLastCreature->GetArea());
                CGameObjectArray* pObjArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
                bool areaSafe = true;
                if (pArea != 0 && pObjArray != 0)
                {
                    unsigned long long objId;
                    pArea->GetFirstObjectInArea(objId);
                    int hasNext;
                    do
                    {
                        CGameObject* pObject = 0;
                        pObjArray->GetGameObject(objId, &pObject);
                        if (pObject != 0 && pObject->AsSWSCreature() != 0 &&
                            pObject->AsSWSCreature()->m_bInParty == 0)
                        {
                            CSWSObject* pOther = pObject->AsSWSCreature();
                            int reputation = pOther->GetReputation(pLastCreature->m_objectId, 0);
                            if (reputation < CFactionManager::REPUTATION_HOSTILE_BELOW && static_cast<CSWSCreature*>(pOther)->IsPartyVisible())
                                areaSafe = false;
                        }
                        hasNext = pArea->GetNextObjectInArea(objId);
                    } while (areaSafe && hasNext != 0);
                }
                if (!areaSafe)
                {
                    m_tempDeathReviveAccum = 0;
                    return;
                }
            }

            {
                unsigned int reviveAccum = m_tempDeathReviveAccum + deltaTime;
                m_tempDeathReviveAccum = reviveAccum;
                if (reviveAccum > 5000)
                {
                    CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
                    CSWSObject* pPCCreature = pPC->GetServerCreature();
                    if (count > 0)
                    {
                        Vector pcPos = pPCCreature->m_position;
                        for (unsigned int i = 0; i < static_cast<unsigned>(count); i++)
                        {
                            CSWCCreature* pChar2 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                            CSWSCreature* pServerCreature2 = pChar2->GetServerCreature();
                            if (pServerCreature2 == 0)
                                continue;

                            if (pServerCreature2->GetDeadTemp() != 0)
                            {
                                Vector pos = pServerCreature2->m_position;
                                g_pAppManager->m_pServerExoApp->ComputeSafeLocation(
                                    pos, 5.0f, pServerCreature2->m_objectId, 0, &pos);
                                pServerCreature2->SetPosition(pos, 1, 1, 0);

                                CGameEffect* pEffect = new CGameEffect(1);
                                pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
                                pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                               (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                                pServerCreature2->ApplyEffect(pEffect, 0, 0);
                                pServerCreature2->m_bRaiseable = 1;
                            }

                            CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
                            if (pPartyTable->m_soloMode == 0)
                            {
                                Vector delta;
                                delta.x = pServerCreature2->m_position.x - pcPos.x;
                                delta.y = pServerCreature2->m_position.y - pcPos.y;
                                delta.z = pServerCreature2->m_position.z - pcPos.z;
                                float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                                if (distSq > 1600.0f)
                                {
                                    Vector pos2 = g_pAppManager->m_pClientExoApp->GetSWParty()
                                                      ->m_members[i].m_followPoint;
                                    g_pAppManager->m_pServerExoApp->ComputeSafeLocation(
                                        pos2, 5.0f, pServerCreature2->m_objectId, 0, &pos2);
                                    pServerCreature2->SetPosition(pos2, 1, 1, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CServerExoAppInternal::PlayerListChange(unsigned int playerId, int changeType, int c)
{
    K2_UNUSED(c);
    CExoString timestamp;
    g_pExoBase->m_pDebug->GetCurrentTimestamp(timestamp);

    if (changeType != 0)
    {
        if (changeType != 1)
            return;

        CExoString name(m_pNetLayer->GetPlayerInfo(playerId)->m_sName);
        name.CStr();

        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isAdminPermitted == 1)
        {
            CSWSSysAdmin* pAdmin = new CSWSSysAdmin(playerId);
            pAdmin->m_nLanguage = 0;
            m_pSysAdminList->AddHead(pAdmin);
            timestamp.CStr();
            name.CStr();
        }

        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isPlayerPermitted == 1)
        {
            CSWSPlayer* pPlayer = new CSWSPlayer(playerId);
            pPlayer->m_nLanguage = 0;
            m_pPlayerList->AddHead(pPlayer);
            timestamp.CStr();
            name.CStr();
        }

        m_pSWSMessage->SendServerToServerAdminPlayerList(CNetLayer::PLAYERID_ALL_SERVERADMINS);
    }
    else
    {
        CExoString name(m_pNetLayer->GetPlayerInfo(playerId)->m_sName);
        CExoLinkedListNode* pos;
        CSWSSysAdmin* pAdmin;
        int shutdownFlag;
        CSWSPlayer* pPlayer;

        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isAdminPermitted == 1)
        {
            pos = m_pSysAdminList->m_pInternal->m_pHead;
            if (pos != 0)
            {
                while (pAdmin = reinterpret_cast<CSWSSysAdmin*>(m_pSysAdminList->GetAtPos(pos)), pAdmin->m_nPlayerId != playerId)
                {
                    m_pSysAdminList->GetNext(pos);
                    if (pos == 0)
                        goto sysadmin_done;
                }
                m_pSysAdminList->Remove(pos);
                {
                    delete pAdmin;
                }
            }
        sysadmin_done:
            timestamp.CStr();
            name.CStr();
        }

        if (m_pNetLayer->GetPlayerInfo(playerId)->m_isPlayerPermitted == 1)
        {
            pos = m_pPlayerList->m_pInternal->m_pHead;
            if (pos == 0)
            {
                shutdownFlag = 0;
            }
            else
            {
                while (pPlayer = reinterpret_cast<CSWSPlayer*>(m_pPlayerList->GetAtPos(pos)), pPlayer->m_nPlayerId != playerId)
                {
                    m_pPlayerList->GetNext(pos);
                    if (pos == 0)
                    {
                        shutdownFlag = 0;
                        goto player_removed;
                    }
                }
                if (m_pcObjectIterPos == pos)
                    m_pPlayerList->GetNext(m_pcObjectIterPos);
                m_pPlayerList->Remove(pos);
                m_pSWSMessage->SendServerToPlayerPlayerList_Delete(CNetLayer::PLAYERID_ALL_PLAYERS, pPlayer);
                shutdownFlag = pPlayer->m_bIsPrimaryPlayer;
                {
                    delete pPlayer;
                }
            }
        player_removed:
            g_pAppManager->GetObjectTableManager(1)->RemovePlayer(playerId);
            timestamp.CStr();
            name.CStr();
            if (shutdownFlag != 0)
                Shutdown(1, 0);
        }

        if (m_aShutdownPlayerIds.Contains(playerId) != 0)
        {
            m_aShutdownPlayerIds.Remove(playerId);
            if (m_aShutdownPlayerIds.m_count == 0)
                m_nShutdownTimeRemaining = 1;
        }
    }
}

int CServerExoAppInternal::GetModulePreviouslyVisited(const CExoString& moduleName)
{
    if (moduleName.GetLength() < 1)
        return true;

    CExoString unused1;
    CExoString unused2;
    int includedInSave;
    bool found;
    short resType;
    int exists;
    bool result;

    includedInSave = IncludeModuleInSave(moduleName);
    found = false;
    resType = -1;

    if (includedInSave != 0)
    {
        g_pAppManager->m_pServerExoApp->GetPartyTable()->AddGameInProgress();

        resType = RESTYPE_RSV;
        exists = g_pExoResMan->Exists(CResRef(moduleName), resType, static_cast<unsigned int*>(0));
        resType = exists ? resType : static_cast<short>(-1);
        if (exists != 0)
        {
            found = true;
        }
        else
        {
            exists = g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_SAV, static_cast<unsigned int*>(0));
            found = exists != 0;
            resType = exists ? static_cast<short>(RESTYPE_SAV) : resType;
        }

        g_pAppManager->m_pServerExoApp->GetPartyTable()->RemoveGameInProgress();
    }

    if (resType == -1)
    {
        resType = RESTYPE_NWM;
        {
            CExoString dir("NWMFILES:");
            g_pExoResMan->AddResourceDirectory(dir);
        }
        exists = g_pExoResMan->Exists(CResRef(moduleName), resType, static_cast<unsigned int*>(0));
        resType = exists ? resType : static_cast<short>(-1);
        {
            CExoString dir("NWMFILES:");
            g_pExoResMan->RemoveResourceDirectory(dir);
        }

        if (exists != 0)
        {
            found = false;
        }
        else
        {
            {
                CExoString dir("MODULES:");
                g_pExoResMan->AddResourceDirectory(dir);
            }
#if !KOTOR1  // K2 only adds the module override directories
            AddModuleOverrideResourceDirectories();
#endif

            exists = g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_MOD, static_cast<unsigned int*>(0));
            if (exists != 0)
            {
                resType = RESTYPE_MOD;
                found = false;
            }

            if (resType == -1)
            {
                resType = RESTYPE_RIM;
                exists = g_pExoResMan->Exists(CResRef(moduleName), resType, static_cast<unsigned int*>(0));
                if (exists == 0)
                    resType = -1;
                else
                    found = false;
            }

            {
                CExoString dir("MODULES:");
                g_pExoResMan->RemoveResourceDirectory(dir);
            }
#if !KOTOR1  // K2 only adds the module override directories
            RemoveModuleOverrideResourceDirectories();
#endif

            if (resType == -1)
            {
                CExoString alias;
                for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++)
                {
                    alias.Format(const_cast<char*>("LIVE%d"), i);
                    if (g_pExoBase->m_pAliasList->GetAliasPath(alias, 0).GetLength() != 0)
                    {
                        alias = alias + CExoString(":MODULES");
                        g_pExoResMan->AddResourceDirectory(alias);

                        resType = RESTYPE_MOD;
                        exists = g_pExoResMan->Exists(CResRef(moduleName), resType, static_cast<unsigned int*>(0));
                        if (exists == 0)
                        {
                            resType = -1;
                            exists = g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RIM, static_cast<unsigned int*>(0));
                            if (exists != 0)
                            {
                                resType = RESTYPE_RIM;
                                found = false;
                            }
                        }
                        else
                        {
                            found = false;
                        }
                        g_pExoResMan->RemoveResourceDirectory(alias);
                        if (resType != -1)
                            break;
                    }
                }
            }
        }
    }

    g_pExoInput->UnpauseRumble();
    result = false;
    if (resType != -1)
        result = found;
    return result;
}

// Per-tick server update: autosave timer, the module-load state machine, the normal tick, the
// party-injury-gated autosave trigger, character flushes and the shutdown countdown.
int CServerExoAppInternal::MainLoop()
{

    m_pPauseTimer->GetSnapshotTime();
    int now = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    if (m_autoSaveTimerFlag == 0)
    {
        int last = m_autoSaveTimerLastTime;
        if (last == 0)
        {
            m_autoSaveTimerLastTime = now;
            last = now;
        }
        unsigned int elapsed = m_autoSaveTimerDay + (now - last);
        m_autoSaveTimerDay = elapsed;
        m_autoSaveTimerLastTime = now;
        if (elapsed > 900000)
        {
            m_autoSaveTimerDay = 0;
            m_autoSaveTimerFlag = 1;
        }
    }
    else
    {
        m_autoSaveTimerLastTime = now;
    }

    if (g_pAppManager->m_pLoadGameErrorInfo->m_flag != 0)
    {
        m_pNetLayer->ProcessReceivedFrames(0);
        m_pNetLayer->UpdateStatusLoop(CNetLayer::APPLICATION_SERVER);
        CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;

        if (pInfo->m_errorCode != 0)
            goto tail;

        switch (pInfo->m_nStallEvent)
        {
        case CLoadGameErrorInfo::STALL_EVENT_LOAD_MODULE:
        case CLoadGameErrorInfo::STALL_EVENT_LOAD_SAVE:
        {
            CExoString moduleName(pInfo->m_saveDirectory);

            CSWSModule* pModule = GetModule();
            int bLoadFromSaveGame;
            unsigned int result;
#if KOTOR1
            bLoadFromSaveGame = g_pAppManager->m_pClientExoApp->GetLoadingSaveGame() == 1;
#else
            bLoadFromSaveGame = m_loadFromSaveGame != 0;
#endif

            if (pInfo->m_loadBar.m_progress == 0)
            {
                CExoString startModuleName(pInfo->m_saveDirectory);
                CExoString copySrcPath(pInfo->m_saveGameName);
                CExoString copyDstPath(pInfo->m_sCopyDstPath);
                unsigned short moduleTypeFlags = static_cast<unsigned short>(pInfo->m_nResType);
                CSWSModule* pNewModule;

                if (CopyModuleToCurrentGame(copySrcPath, copyDstPath, moduleTypeFlags) == 0)
                {
                    DealWithLoadGameError(5);
                }
                else
                {
#if !KOTOR1
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bFadeUntilScript = 0;
#endif
                    pNewModule = new CSWSModule(startModuleName, 0,
                        moduleTypeFlags == RESTYPE_SAV || moduleTypeFlags == RESTYPE_RSV);
                    m_moduleObjectId = pNewModule->m_objectId;
                    if (moduleTypeFlags == RESTYPE_RSV)
                    {
                        pNewModule->m_bIsNWMFile = 1;
                        pNewModule->m_sNWMResName = startModuleName;
                    }
                    result = pNewModule->LoadModuleStart(startModuleName, bLoadFromSaveGame);
                    if (result != 0)
                        DealWithLoadGameError(result);
                }
            }
            else if (pInfo->m_loadBar.m_progress == pInfo->m_loadBar.m_total)
            {
                if (pInfo->m_bStallFinished != 1)
                {
                    result = pModule->LoadModuleFinish();
                    if (result != 0)
                        DealWithLoadGameError(result);

                    pInfo->m_bStallFinished = 1;
                    if (g_nClientServerMode == 2)
                    {
                        pInfo->m_bStallFinished = 0;
                        pInfo->m_flag = 0;
                        pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
                    }
                    m_serverMode = 1;
                    m_loadingModule = 0;
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_EndStallEvent(1, 0);

                    m_pSWSMessage->SendServerToPlayerServerStatus_Status(CNetLayer::PLAYERID_ALL_PLAYERS);
                    m_pSWSMessage->SendServerToServerAdminServerStatus(CNetLayer::PLAYERID_ALL_SERVERADMINS);

                    if (m_aCharListRequests.m_count != 0)
                    {
                        for (int64_t i = 0; i < m_aCharListRequests.m_count; i++)
                        {
                            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(GetClientObjectByPlayerId(m_aCharListRequests[static_cast<int>(i)], 0));
                            if (pPlayer != 0)
                                m_pSWSMessage->SendServerToPlayerCharList(pPlayer);
                        }
                        m_aCharListRequests.SetSize(0);
                    }
                }
            }
            else
            {
                m_partyTable.ClearMembers();
#if !KOTOR1
                m_partyTable.ClearPuppets();
#endif
                result = pModule->LoadModuleInProgress(pInfo->m_loadBar.m_total, pInfo->m_loadBar.m_progress);
                if (result != 0)
                    DealWithLoadGameError(result);
            }
            break;
        }
        case CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME:
            StallEventSaveGame();
            ClearLogCounters(const_cast<char*>("2"));
            break;
        }
        goto tail;
    }

    {
        unsigned char pauseFlags = m_pauseFlags;
        if ((pauseFlags & 3) == 0)
        {
            static_cast<CWorldTimer*>(m_pWorldTimer)->SetSnapshotTime();
            pauseFlags = m_pauseFlags;
        }
        if (((pauseFlags >> 1) & 1) == 0)
            static_cast<CWorldTimer*>(m_pTimestopTimer)->SetSnapshotTime();
        m_pPauseTimer->SetSnapshotTime();
    }

    m_pNetLayer->ProcessReceivedFrames(1);
    m_pNetLayer->UpdateStatusLoop(CNetLayer::APPLICATION_SERVER);
    TempDeathUpdate();
    UpdateMapData();

    if (g_pAppManager->m_pLoadGameErrorInfo->m_flag == 0 && m_serverMode == 2)
    {
        m_pServerAIMaster->UpdateState();
        UpdateClientGameObjects(m_forceUpdate);
        m_forceUpdate = 0;
    }

    if (m_moveToModulePending == 1 && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bControllerPauseActive == 0)
    {
#if !KOTOR1
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(CExoString("000_Human_Conf_Active"), 0);
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(CExoString("000_Beast_Conf_Active"), 0);
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(CExoString("000_Droid_Conf_Active"), 0);
#endif

        unsigned int moduleDate, moduleHour, moduleMinute, moduleSecond, moduleTimeOfDayElapsed;
        unsigned int pausedDay, pausedTod;
        static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->GetWorldTime(&pausedDay, &pausedTod);

        unsigned int pausedHour, pausedMinute, pausedSecond, pausedMs;
        unsigned char moduleMs;
        static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->ConvertFromTimeOfDay(pausedTod, &pausedHour, &pausedMinute, &pausedSecond, &pausedMs);

        CSWSModule* pModule = GetModule();

        pModule->GetTime(&moduleDate, &moduleHour, &moduleMinute, &moduleSecond, &moduleMs, &moduleTimeOfDayElapsed);

        g_pAppManager->m_pServerExoApp->SetMoveToWorldTime(moduleDate, static_cast<unsigned char>(moduleHour), static_cast<unsigned char>(moduleMinute),
            static_cast<unsigned char>(moduleSecond), static_cast<unsigned short>(pausedMinute), static_cast<unsigned short>(pausedSecond), static_cast<unsigned short>(pausedMs));
        g_pAppManager->m_pServerExoApp->SetMoveToPausedTimeOfDay(pausedTod);
        g_pAppManager->m_pServerExoApp->SetMoveToPausedCalendarDay(pausedDay);

        m_partyTable.UpdateMembers(0);
        StartNewModule(m_moveToModuleString);
        m_moveToModulePending = 0;
    }

    if (g_pAppManager->m_pLoadGameErrorInfo->m_flag == 0)
    {
        if (g_pAppManager->m_pServerExoApp->GetPlayerCreature() != 0)
        {
            CSWSArea* pArea = static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetPlayerCreature()->GetArea());
            if (pArea != 0 && g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay() == 0)
            {
                if (m_autoSavePending == 1)
                {
#if !KOTOR1
                    CSWCCreature* pChar;
                    if (((pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0)) != 0
                         && pChar->GetServerCreature()->GetCurrentHitPoints(0) > 1)
                        || ((pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1)) != 0
                            && pChar->GetServerCreature()->GetCurrentHitPoints(0) > 1)
                        || ((pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2)) != 0
                            && pChar->GetServerCreature()->GetCurrentHitPoints(0) > 1))
#endif
                    {
                        uint64_ll_t freeSpace = 0;
                        g_pExoResMan->GetFreeDiskSpace(CExoString("SAVES:"), &freeSpace);
                        if ((freeSpace >> 14) <= SAVE_MIN_FREE_SPACE_16K || m_serverMode != 2)
                        {
                            int fadeState = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState;
                            CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                            if (fadeState == 0)
                            {
                                Vector zero = {0.0f, 0.0f, 0.0f};
                                pGui->StartGlobalFade(1, 0.5f, 1.0f, zero);
                            }
                            else
                            {
                                pGui->m_bHoldFadeInForDialog = 1;
                            }
                            g_pAppManager->m_pClientExoApp->SetCanSendDialog(1);
                            g_pExoInput->EndAttemptedBind();
                            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(0);
                            CSWCModule* pClientModule = g_pAppManager->m_pClientExoApp->GetModule();
                            pClientModule->m_pArea->StartSounds();
                        }
                        else
                        {
                            CExoString saveName;
                            saveName = "AUTOSAVE";
#if KOTOR1
                            SaveGame(1, saveName, 0, 0);
#else
                            SaveGame(1, saveName, 0, 0, 0);
#endif
                        }
                    }
                }
                m_autoSavePending = 0;
            }
        }
    }

    if (m_exportAllCharactersPending == 1)
    {
        ExportAllPlayers();
        m_exportAllCharactersPending = 0;
    }
    if (m_bNeedCharacterPending == 1)
    {
        // sic: reads the head node's payload directly instead of calling GetAtPos().
        CSWSClient* pFirstPlayer = static_cast<CSWSClient*>(m_pPlayerList->m_pInternal->m_pHead->m_pData);
        m_pSWSMessage->SendServerToPlayerLogin_NeedCharacter(pFirstPlayer->m_nPlayerId);
        m_bNeedCharacterPending = 0;
    }

tail:
    {
        uint64_ll_t hiResNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        unsigned long long remaining = m_nShutdownTimeRemaining;
        if (remaining != 0)
        {
            unsigned long long last = m_nShutdownLastTick;
            if (remaining < hiResNow - last)
            {
                m_nShutdownTimeRemaining = 0;
                if (m_aShutdownPlayerIds.m_count == 0)
                    g_pAppManager->DestroyServer();
                else
                    Shutdown(0, 0);
            }
            else
            {
                if ((remaining > 60000000 && remaining - (hiResNow - last) < 60000000) ||
                    (remaining > 30000000 && (remaining - hiResNow) + last < 30000000))
                {
                    for (long long i = 0; i < m_aShutdownPlayerIds.m_count; i++)
                        m_pSWSMessage->SendServerToPlayerShutDownServer(static_cast<unsigned int>(m_aShutdownPlayerIds[static_cast<int>(i)]),
                            static_cast<unsigned int>((m_nShutdownLastTick - hiResNow + m_nShutdownTimeRemaining) / 1000000));
                }
                m_nShutdownTimeRemaining = (m_nShutdownTimeRemaining - hiResNow) + m_nShutdownLastTick;
                m_nShutdownLastTick = hiResNow;
            }
        }
    }

    {
        uint64_ll_t heartbeatNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        unsigned long long interval;
        if (m_logHeartbeatLastTime != 0
            && m_logHeartbeatInterval > heartbeatNow - m_logHeartbeatLastTime)
            interval = m_logHeartbeatInterval - (heartbeatNow - m_logHeartbeatLastTime);
        else
            interval = 10000000;
        m_logHeartbeatInterval = interval;
        m_logHeartbeatLastTime = heartbeatNow;
    }
    return 0;
}

// Writes the in-progress-game save: a savenfo GFF plus a .mod ERF archive of the current module
// state. Called from MainLoop's load-state machine at state 2.
void CServerExoAppInternal::StallEventSaveGame()
{

    CExoString moduleTag;
    CExoString saveSlot;
    CExoString saveGameName;
    CExoString portraitLabel;

    CSWSModule* pModule = GetModule();

    moduleTag = pModule->m_sModuleResourceName;
    int colon = moduleTag.Find(':', 0);
    if (colon != -1)
    {
#if KOTOR1
        int len = moduleTag.GetLength();
#else
        int len = 0;
        if (moduleTag.m_sString != 0)
            len = static_cast<int>(strlen(moduleTag.m_sString));
#endif
        CExoString tail = moduleTag.Right(len - colon - 1);
        moduleTag = tail;
    }

    g_pAppManager->m_pClientExoApp->SetLoadScreenByModuleName(moduleTag);
    uint64_t hint = g_pAppManager->m_pClientExoApp->GetNextLoadScreenHintSTRREF(moduleTag, 0);
    g_pAppManager->m_pClientExoApp->SetLoadScreenHint(hint);

#if KOTOR1
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(0, 1, 1);
#else
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(0, 1, 1, 1);
#endif
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(0, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    // The start and end stamps fed a timing log line the shipping build compiles out.
    unsigned int nStartTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    K2_UNUSED(nStartTime);
    CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
    CExoString saveDir;
    CExoString saveFilePath;
    saveDir = pInfo->m_saveDirectory;
    saveFilePath = pInfo->m_saveGameName;

    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerModule_SaveGameStatus(0);
    StoreCurrentModule();

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(5, 0);
    UpdateScreen(0.033333335f, 0, 0);
    m_partyTable.Save(saveDir, 1);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 0);
    UpdateScreen(0.033333335f, 0, 0);
    m_globalVariableTable.Save(saveDir);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(15, 0);
    UpdateScreen(0.033333335f, 0, 0);

    CSWSCreature* pPlayerCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    CSWSPlayer* pClientObj = GetClientObjectByObjectId(pPlayerCreature->m_objectId);

    if (g_bSavePlayerBIC != 0)
        GetModule()->SavePrimaryPlayerInfo(pPlayerCreature, saveDir);

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(20, 0);
    UpdateScreen(0.033333335f, 0, 0);

#if KOTOR1
    saveGameName = saveDir + CExoString("/") + CExoString("savenfo");
#else
    saveGameName = saveDir + CExoString("\\") + CExoString("savenfo");
#endif

    CResGFF* pGff;
    CResStruct topStruct;
    CSWSArea* pArea;
    pGff = new CResGFF();
    {
        CExoString nfoType("NFO ");
        CExoString nfoVer("V2.0");
        pGff->CreateGFFFile(&topStruct, nfoType, nfoVer);
    }

    pArea = static_cast<CSWSArea*>(pPlayerCreature->GetArea());
    pArea->m_areaName.GetString(pClientObj->m_nLanguage, &saveSlot, 0);
    pGff->WriteFieldCExoString(&topStruct, saveSlot, const_cast<char*>("AREANAME"));
    pGff->WriteFieldCExoString(&topStruct, moduleTag, const_cast<char*>("LASTMODULE"));
    pGff->WriteFieldDWORD(&topStruct, m_partyTable.GetTotalPlayedSeconds(), const_cast<char*>("TIMEPLAYED"));
#if KOTOR1
    unsigned int cheatFlags = m_partyTable.m_cheatUsedFlags;
#else
    unsigned int cheatFlags = m_partyTable.m_cheatUsedFlags | ((IsGameModded() & 1) << 1);
#endif
    pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(cheatFlags), const_cast<char*>("CHEATUSED"));
    pGff->WriteFieldCExoString(&topStruct, m_saveGameName, const_cast<char*>("SAVEGAMENAME"));
#if KOTOR1
    // Load-screen image for the module, stored under the screenshot field name.
    CExoString screenshot;
    {
        C2DA* pTable = g_pRules->m_p2DArrays->m_pAreaTransition;
        CExoString colName("BMPResRef");
        int found = pTable->GetCExoStringEntry(moduleTag, colName, &screenshot);
        if (found == 0)
        {
            screenshot.Format(const_cast<char*>("load_%s"), moduleTag.CStr());
            CResRef bmpRef(screenshot);
            if (g_pExoResMan->Exists(bmpRef, 3, static_cast<unsigned int*>(0)) == 0)
            {
                CResRef tgaRef(screenshot);
                if (g_pExoResMan->Exists(tgaRef, RESTYPE_TPC, static_cast<unsigned int*>(0)) == 0)
                {
                    C2DA* pTable2 = g_pRules->m_p2DArrays->m_pAreaTransition;
                    CExoString defaultRow("DEFAULT");
                    CExoString colName2("BMPResRef");
                    pTable2->GetCExoStringEntry(defaultRow, colName2, &screenshot);
                }
            }
        }
    }
    pGff->WriteFieldCExoString(&topStruct, screenshot, const_cast<char*>("SCREENSHOT"));
#endif

#if !KOTOR1
    pGff->WriteFieldDWORD64(&topStruct, GetLocalFileTime(), const_cast<char*>("TIMESTAMP"));

    {
        CExoString pcName(m_partyTable.m_saveTableName);
        pGff->WriteFieldCExoString(&topStruct, pcName, const_cast<char*>("PCNAME"));
    }

    pGff->WriteFieldDWORD(&topStruct, m_partyTable.m_saveNumber, const_cast<char*>("SAVENUMBER"));
#endif

    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetGamePlayHint(), const_cast<char*>("GAMEPLAYHINT"));
#if KOTOR1
    unsigned char storyHint = g_pAppManager->m_pClientExoApp->GetStoryHint();
    pGff->WriteFieldBYTE(&topStruct, storyHint, const_cast<char*>("STORYHINT"));
#else
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(0), const_cast<char*>("STORYHINT0"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(1), const_cast<char*>("STORYHINT1"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(2), const_cast<char*>("STORYHINT2"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(3), const_cast<char*>("STORYHINT3"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(4), const_cast<char*>("STORYHINT4"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(5), const_cast<char*>("STORYHINT5"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(6), const_cast<char*>("STORYHINT6"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(7), const_cast<char*>("STORYHINT7"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(8), const_cast<char*>("STORYHINT8"));
    pGff->WriteFieldBYTE(&topStruct, g_pAppManager->m_pClientExoApp->GetStoryHint(9), const_cast<char*>("STORYHINT9"));
#endif

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CExoString liveLabel;
    unsigned int liveFlags = 0;
    for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++)
    {
        liveLabel.Format(const_cast<char*>("LIVE%d"), i);
        pGff->WriteFieldCExoString(&topStruct, g_pAppManager->m_pClientExoApp->GetLiveContentName(static_cast<int>(i)),
                                   static_cast<char*>(liveLabel.CStr()));
        if (g_pExoBase->m_pAliasList->GetAliasPath(liveLabel, 0).GetLength() > 0)
            liveFlags |= 1 << (i - 1);
    }
    pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(liveFlags), const_cast<char*>("LIVECONTENT"));

    if (pParty->m_nCharacterCount > 0)
    {
        for (int i = 0; i < pParty->m_nCharacterCount; i++)
        {
            CSWCCreature* pClientCreature;
            CSWSObject* pServerObj;
            CSWSCreature* pServerCreature;
            CSWCCreature* pClientCr;
            pClientCreature = pParty->GetCharacter(i);
            pServerObj = pClientCreature->GetServerCreature();
            if (pServerObj != 0)
            {
                pServerCreature = static_cast<CSWSCreature*>(pServerObj);
                portraitLabel.Format(const_cast<char*>("%s%d"), "PORTRAIT", i);
                pClientCr = pServerCreature->GetClientCreature();
                if (pClientCr == 0)
                {
                    CResRef portraitRef = pServerCreature->GetPortrait();
                    pGff->WriteFieldCResRef(&topStruct, portraitRef, static_cast<char*>(portraitLabel.CStr()));
                }
                else
                {
                    CResRef portraitRef = pClientCr->GetPortrait(1);
                    pGff->WriteFieldCResRef(&topStruct, portraitRef, static_cast<char*>(portraitLabel.CStr()));
                }
            }
        }
    }

    {
        CExoString gffPath(saveGameName.CStr());
        pGff->WriteGFFFile(gffPath, 0);
    }
    delete pGff;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(25, 0);
    UpdateScreen(0.033333335f, 0, 0);

    CERFFile* pErf = new CERFFile();
    pErf->Create(saveFilePath);
    pErf->SetVersion(const_cast<char*>("MOD V1.0"));
    pErf->WriteHeader();
    pErf->WriteStringTable();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(30, 0);
    UpdateScreen(0.033333335f, 0, 0);
    {
        CExoString gipPrefix("GAMEINPROGRESS:");
        pErf->ImportFiles(gipPrefix, reinterpret_cast<void*>(-1), UpdateLoadScreen, 30, 60);
    }
    pErf->Finish();
    pErf->Read(saveFilePath);
    pErf->ReadHeaderVariance(saveFilePath);
    pErf->~CERFFile();
    operator delete(pErf);

    CGuiInGame* pGui;
    CSWCModule* pClientModule;
    CSWCArea* pClientArea;
    CSWSMessage* pMsg;
    unsigned int nEndTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    K2_UNUSED(nEndTime);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(100, 0);
    UpdateScreen(0.033333335f, 0, 0);

#if KOTOR1
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
    g_pAppManager->m_pClientExoApp->HideLoadScreen(0, 0);
#else
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
    g_pAppManager->m_pClientExoApp->HideLoadScreen(0, 1, 0);
#endif

    pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui->m_bInGameMenuShown == 0)
    {
        pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        if (pGui->m_nGlobalDialogState == 0)
        {
            pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
            Vector zero;
            pGui->StartGlobalFade(1, 0.5f, 1.0f, zero);
        }
        else
        {
            pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
            pGui->m_bHoldFadeInForDialog = 1;
        }
    }

    g_pAppManager->m_pClientExoApp->SetCanSendDialog(1);
    g_pExoInput->EndAttemptedBind();

    pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGui->SetGlobalUnpauseableState(0);
    pClientModule = g_pAppManager->m_pClientExoApp->GetModule();
    pClientArea = pClientModule->m_pArea;
    pClientArea->StartSounds();

    if (pInfo->m_bStallFinished != 1)
    {
        pInfo->m_bStallFinished = 1;
        if (g_nClientServerMode == 2)
        {
            pInfo->m_bStallFinished = 0;
            pInfo->m_flag = 0;
            pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
        }
        pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        pMsg->SendServerToPlayerModule_SaveGameStatus(1);
        pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        pMsg->SendServerToPlayerLoadBar_EndStallEvent(2, 0);
    }
#if KOTOR1
    // Re-stamp the client clock against the server's after the stall.
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetSnapshotTime();
    CWorldTimer* pClientTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    pClientTimer->m_nLastSnapshotTime = m_pWorldTimer->GetSnapshotTime();
#endif
}

// Formats "<overridePath>\modules\<moduleName>" for the override probe's two call sites.
#if !KOTOR1
static void __attribute__((noinline)) FormatOverrideModulePath(char* dest, const char* overridePath, const char* moduleName)
{
    sprintf(dest, "%s\\modules\\%s", overridePath, moduleName);
}
#endif

// Resolves the resource-manager path a module loads from (an in-progress save, a demo-module alias,
// a content-package live-modules alias, or a module under the override directory) and stores it in
// CAppManager::m_pLoadGameErrorInfo for the next tick of the load state machine.
int CServerExoAppInternal::LoadModule(const CExoString& moduleName, CSWSPlayer* pPlayer)
{
    moduleName.CStr();
#if !KOTOR1  // K1 has no achievements
    Achievements::ResetOnLoad();
#endif

    CSWSModule* pModule = GetModule();

    CExoString resolvedPath;
    CExoString copyDstPath;
    CExoString shortModuleName;
    CExoString estimateSizeArg;
    CExoString unusedScratch;
    CExoString dbgTimestamp;
    int slashPos;
    int resType;
    bool haveResult;

    g_pExoBase->m_pDebug->GetCurrentTimestamp(dbgTimestamp);
    dbgTimestamp.CStr();
    moduleName.CStr();

    if (m_loadingModule != 1)
        m_loadingModule = 1;

    if (pModule != 0)
        UnloadModule();

    if (g_pExoResMan->CreateDirectory2(CExoString("CURRENTGAME:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("CURRENTGAME:"), 1, 1);

    copyDstPath = "CURRENTGAME:";
#if KOTOR1  // K1 splits the module path on '/'
    slashPos = moduleName.Find(CExoString("/"), 0);
#else
    slashPos = moduleName.Find(CExoString("\\"), 0);
#endif
    if (slashPos == -1)
    {
        copyDstPath = copyDstPath + moduleName;
        shortModuleName = moduleName;
    }
    else
    {
#if KOTOR1  // K1 calls GetLength()
        shortModuleName = moduleName.Right(moduleName.GetLength() - slashPos - 1);
#else
        int len = moduleName.GetLength();
        shortModuleName = moduleName.Right(len - slashPos - 1);
#endif
        copyDstPath = copyDstPath + shortModuleName;
    }

    resType = RESTYPE_INVALID;

    if (IncludeModuleInSave(moduleName) != 0)
    {
        g_pAppManager->m_pServerExoApp->GetPartyTable()->AddGameInProgress();

        if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RSV, static_cast<unsigned int*>(0)))
        {
            resolvedPath = "GAMEINPROGRESS:";
            resolvedPath = resolvedPath + moduleName;
            estimateSizeArg = resolvedPath;
            resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_RSV);
            copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_RSV);
            resType = RESTYPE_RSV;
            SetEstimatedSaveSize(estimateSizeArg, RESTYPE_RSV);
        }
        else if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_SAV, static_cast<unsigned int*>(0)))
        {
            resolvedPath = "GAMEINPROGRESS:";
            resolvedPath = resolvedPath + moduleName;
            estimateSizeArg = resolvedPath;
            resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_SAV);
            copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_SAV);
            resType = RESTYPE_SAV;
            SetEstimatedSaveSize(estimateSizeArg, RESTYPE_SAV);
        }
        else
        {
            resType = RESTYPE_INVALID;
        }

        g_pAppManager->m_pServerExoApp->GetPartyTable()->RemoveGameInProgress();
        haveResult = (resType != RESTYPE_INVALID);
    }
    else
    {
        haveResult = false;
    }

    if (!haveResult)
    {
        goto LAB_NotSaved;
    }
    goto LAB_HaveResult;

LAB_NotSaved:
    g_pExoResMan->AddResourceDirectory(CExoString("NWMFILES:"));
    if (!g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_NWM, static_cast<unsigned int*>(0)))
    {
        resType = RESTYPE_INVALID;
    }
    else
    {
        resolvedPath = "NWMFILES:";
        resolvedPath = resolvedPath + moduleName;
        estimateSizeArg = resolvedPath;
        resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_NWM);
        copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_NWM);
        resType = RESTYPE_NWM;
        SetEstimatedSaveSize(estimateSizeArg, RESTYPE_NWM);
    }
    g_pExoResMan->RemoveResourceDirectory(CExoString("NWMFILES:"));
    if (resType != RESTYPE_INVALID)
        goto LAB_HaveResult;
#if KOTOR1  // K1 searches MODULES: before the LIVE%d content packages
    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
    if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_MOD, static_cast<unsigned int*>(0)))
    {
        resolvedPath = "MODULES:";
        resolvedPath = resolvedPath + moduleName;
        estimateSizeArg = resolvedPath;
        resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_MOD);
        copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_MOD);
        resType = RESTYPE_MOD;
        SetEstimatedSaveSize(estimateSizeArg, RESTYPE_MOD);
    }
    else if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        resolvedPath = "MODULES:";
        resolvedPath = resolvedPath + moduleName;
        estimateSizeArg = resolvedPath;
        resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_RIM);
        copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_RIM);
        resType = RESTYPE_RIM;
        SetEstimatedSaveSize(estimateSizeArg, RESTYPE_RIM);
    }
    else
    {
        resType = RESTYPE_INVALID;
    }
    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
    if (resType != RESTYPE_INVALID)
        goto LAB_HaveResult;
#endif

    {
        CExoString scratch;
        unsigned int i = 1;
        for (;;)
        {
            if (i >= CTlkTable::NUM_CONTENT_PKGS)
            {
                resType = RESTYPE_INVALID;
                break;
            }
            scratch.Format(const_cast<char*>("LIVE%d"), i);
            const CExoString& aliasPath = g_pExoBase->m_pAliasList->GetAliasPath(scratch, 0);
            if (aliasPath.GetLength() != 0)
            {
                scratch = scratch + CExoString(":MODULES");
                g_pExoResMan->AddResourceDirectory(scratch);

                if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_MOD, static_cast<unsigned int*>(0)))
                {
                    resolvedPath = scratch + CExoString("\\");
                    resolvedPath = resolvedPath + moduleName;
                    estimateSizeArg = resolvedPath;
                    resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_MOD);
                    copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_MOD);
                    resType = RESTYPE_MOD;
                    SetEstimatedSaveSize(estimateSizeArg, RESTYPE_MOD);
                }
                else if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RIM, static_cast<unsigned int*>(0)))
                {
                    resolvedPath = scratch + CExoString("\\");
                    resolvedPath = resolvedPath + moduleName;
                    estimateSizeArg = resolvedPath;
                    resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_RIM);
                    copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_RIM);
                    resType = RESTYPE_RIM;
                    SetEstimatedSaveSize(estimateSizeArg, RESTYPE_RIM);
                }
                else
                {
                    resType = RESTYPE_INVALID;
                }

                g_pExoResMan->RemoveResourceDirectory(scratch);
                if (resType != RESTYPE_INVALID)
                    break;
            }
            i = i + 1;
        }
    }
    if (resType != RESTYPE_INVALID)
        goto LAB_HaveResult;

#if !KOTOR1  // K2 searches MODULES: (and the override directories) last
    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
    AddModuleOverrideResourceDirectories();

    if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_MOD, static_cast<unsigned int*>(0)))
    {
        int overrideIdx = 0;
        const char* overridePath;
        int fileOpened;
        char pathBuf[200];
        do
        {
            overridePath = GetOverridePath(overrideIdx, true);
            if (overridePath == 0)
            {
                resolvedPath = "MODULES:";
                resolvedPath = resolvedPath + moduleName;
                estimateSizeArg = resolvedPath;
                goto LAB_ModFromDir;
            }
            FormatOverrideModulePath(pathBuf, overridePath, moduleName.CStr());
            CExoFile* pFile = new CExoFile(CExoString(pathBuf), RESTYPE_MOD, CExoString("rb"));
            fileOpened = pFile->FileOpened();
            pFile->~CExoFile();
            operator delete(pFile);
            overrideIdx = overrideIdx + 1;
        } while (fileOpened == 0);
        resolvedPath = pathBuf;
        estimateSizeArg = pathBuf;
LAB_ModFromDir:
        resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_MOD);
        copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_MOD);
        resType = RESTYPE_MOD;
        SetEstimatedSaveSize(estimateSizeArg, RESTYPE_MOD);
    }
    else if (g_pExoResMan->Exists(CResRef(moduleName), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        int overrideIdx = 0;
        const char* overridePath;
        int fileOpened;
        char pathBuf[200];
        do
        {
            overridePath = GetOverridePath(overrideIdx, true);
            if (overridePath == 0)
            {
                resolvedPath = "MODULES:";
                resolvedPath = resolvedPath + moduleName;
                estimateSizeArg = resolvedPath;
                goto LAB_RimFromDir;
            }
            FormatOverrideModulePath(pathBuf, overridePath, moduleName.CStr());
            CExoFile* pFile = new CExoFile(CExoString(pathBuf), RESTYPE_RIM, CExoString("rb"));
            fileOpened = pFile->FileOpened();
            pFile->~CExoFile();
            operator delete(pFile);
            overrideIdx = overrideIdx + 1;
        } while (fileOpened == 0);
        resolvedPath = pathBuf;
        estimateSizeArg = pathBuf;
LAB_RimFromDir:
        resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(resolvedPath, RESTYPE_RIM);
        copyDstPath = g_pExoBase->m_pAliasList->ResolveFileName(copyDstPath, RESTYPE_RIM);
        resType = RESTYPE_RIM;
        SetEstimatedSaveSize(estimateSizeArg, RESTYPE_RIM);
    }
    else
    {
        resType = RESTYPE_INVALID;
    }

    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
    RemoveModuleOverrideResourceDirectories();

    if (resType != RESTYPE_INVALID)
        goto LAB_HaveResult;
#endif

    m_pSWSMessage->SendServerToPlayerSaveLoad_Status(pPlayer, 1, 3);
    return 0;

LAB_HaveResult:
    {
        uint64_ll_t freeSpace;
        if (g_pExoResMan->GetFreeDiskSpace(CExoString("CURRENTGAME:"), &freeSpace) == 1)
        {
            CExoFile* pFile;
            int size;
            pFile = new CExoFile(estimateSizeArg, static_cast<unsigned short>(resType), CExoString("rb"));
            size = pFile->GetSize();
            pFile->~CExoFile();
            operator delete(pFile);
            if (freeSpace < static_cast<unsigned long long>(size))
            {
                m_pSWSMessage->SendServerToPlayerSaveLoad_Status(pPlayer, 1, 1);
                return 0;
            }
        }
    }

    g_pAppManager->m_pLoadGameErrorInfo->m_saveDirectory = shortModuleName;
    g_pAppManager->m_pLoadGameErrorInfo->m_saveGameName = resolvedPath;
    g_pAppManager->m_pLoadGameErrorInfo->m_sCopyDstPath = copyDstPath;
    g_pAppManager->m_pLoadGameErrorInfo->m_nResType = resType;

    if (resType == RESTYPE_RSV || resType == RESTYPE_SAV)
    {
        g_pAppManager->m_pLoadGameErrorInfo->m_flag = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_LOAD_SAVE;
        g_pAppManager->m_pLoadGameErrorInfo->m_bStallFirstFrame = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_errorCode = 0;
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_StartStallEvent(
            CLoadGameErrorInfo::STALL_EVENT_LOAD_SAVE);
    }
    else
    {
        g_pAppManager->m_pLoadGameErrorInfo->m_flag = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_LOAD_MODULE;
        g_pAppManager->m_pLoadGameErrorInfo->m_bStallFirstFrame = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_errorCode = 0;
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerLoadBar_StartStallEvent(
            CLoadGameErrorInfo::STALL_EVENT_LOAD_MODULE);
    }

    g_pAppManager->m_pLoadGameErrorInfo->m_nLoadBarPair = 0;
    g_pAppManager->m_pLoadGameErrorInfo->m_saveDirectory = shortModuleName;
    return 1;
}

// Writes the numbered PC autosave slot (savenfo GFF + a .mod ERF archive of the in-progress game),
// then copies the temp screenshot into the slot.
void CServerExoAppInternal::DoPCAutosave(const CExoString& moduleName, C2DA* p2da)
{
    g_pAppManager->m_pClientExoApp->SetLoadScreenByModuleName(moduleName);
    uint64_t hint = g_pAppManager->m_pClientExoApp->GetNextLoadScreenHintSTRREF(moduleName, 0);
    g_pAppManager->m_pClientExoApp->SetLoadScreenHint(hint);

#if KOTOR1
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(0, 1, 1);
#else
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(0, 1, 1, 1);
#endif
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(0, 0);
    // UpdateRumble is a CClientExoAppInternal method, not a CClientExoApp one.
    g_pAppManager->m_pClientExoApp->m_pInternal->UpdateRumble(0.0f);
    g_pExoInput->PauseRumble();
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    {
        CExoString savesDir("SAVES:");
        g_pExoResMan->CreateDirectory2(savesDir);
    }

    CExoString saveDir;
    saveDir.Format(const_cast<char*>("SAVES:%06d - %s"), 1, "AUTOSAVE");
    if (g_pExoResMan->CreateDirectory2(saveDir) == 0)
        g_pExoResMan->CleanDirectory(saveDir, 0, 0);

    unsigned int step = g_pAppManager->m_pClientExoApp->GetLoadStep(0) & 0xff;
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step / 20, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);
    m_partyTable.SaveTableInfo(saveDir);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step / 10, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);
    m_globalVariableTable.Save(saveDir);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step * 15 / 100, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    {
#if KOTOR1
        CExoString backslash("/");
#else
        CExoString backslash("\\");
#endif
        CExoString withSlash = saveDir + backslash;
        saveDir = withSlash;
    }

    CExoString saveGameName = saveDir + CExoString("savenfo");

    CResGFF* pGff = new CResGFF();
    CResStruct topStruct;
    {
        CExoString nfoType("NFO ");
        CExoString nfoVer("V2.0");
        pGff->CreateGFFFile(&topStruct, nfoType, nfoVer);
    }

    CExoString areaName;
    {
        int strRef;
        if (p2da->GetINTEntry(moduleName, CExoString("AreaName"), &strRef) != 0)
            areaName = g_pTlkTable->GetSimpleString(static_cast<uint64_t>(strRef));
    }

    CExoString screenshot;
    {
        C2DA* pTable = g_pRules->m_p2DArrays->m_pAreaTransition;
        int found = pTable->GetCExoStringEntry(moduleName, CExoString("BMPResRef"), &screenshot);
        if (found == 0)
        {
            screenshot.Format(const_cast<char*>("load_%s"), moduleName.CStr());
            CResRef bmpRef(screenshot);
            if (g_pExoResMan->Exists(bmpRef, 3, static_cast<unsigned int*>(0)) == 0)
            {
                CResRef tgaRef(screenshot);
                if (g_pExoResMan->Exists(tgaRef, RESTYPE_TPC, static_cast<unsigned int*>(0)) == 0)
                {
                    C2DA* pTable2 = g_pRules->m_p2DArrays->m_pAreaTransition;
                    pTable2->GetCExoStringEntry(CExoString("DEFAULT"), CExoString("BMPResRef"), &screenshot);
                }
            }
        }
    }

    pGff->WriteFieldCExoString(&topStruct, areaName, const_cast<char*>("AREANAME"));
    pGff->WriteFieldCExoString(&topStruct, moduleName, const_cast<char*>("LASTMODULE"));
    pGff->WriteFieldDWORD(&topStruct, m_partyTable.GetTotalPlayedSeconds(), const_cast<char*>("TIMEPLAYED"));
#if KOTOR1
    unsigned int cheatFlags = m_partyTable.m_cheatUsedFlags;
#else
    unsigned int cheatFlags = m_partyTable.m_cheatUsedFlags | ((IsGameModded() & 1) << 1);
#endif
    pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(cheatFlags), const_cast<char*>("CHEATUSED"));
    pGff->WriteFieldBYTE(&topStruct, 1, const_cast<char*>("PCAUTOSAVE"));
    pGff->WriteFieldCExoString(&topStruct, screenshot, const_cast<char*>("SCREENSHOT"));

#if !KOTOR1
    {
        SystemTimeWords localTime;
        GetLocalTime(&localTime);
        unsigned long long fileTime;
        SystemTimeToFileTime(&localTime, &fileTime);
        pGff->WriteFieldDWORD64(&topStruct, fileTime, const_cast<char*>("TIMESTAMP"));
    }

    {
        CExoString pcName(m_partyTable.m_saveTableName);
        pGff->WriteFieldCExoString(&topStruct, pcName, const_cast<char*>("PCNAME"));
    }

    pGff->WriteFieldDWORD(&topStruct, m_partyTable.m_saveNumber, const_cast<char*>("SAVENUMBER"));
#endif

    unsigned char gamePlayHint = g_pAppManager->m_pClientExoApp->GetGamePlayHint();
    pGff->WriteFieldBYTE(&topStruct, gamePlayHint, const_cast<char*>("GAMEPLAYHINT"));
#if KOTOR1
    unsigned char storyHint = g_pAppManager->m_pClientExoApp->GetStoryHint();
    pGff->WriteFieldBYTE(&topStruct, storyHint, const_cast<char*>("STORYHINT"));
#else
    unsigned char storyHint0 = g_pAppManager->m_pClientExoApp->GetStoryHint(0);
    pGff->WriteFieldBYTE(&topStruct, storyHint0, const_cast<char*>("STORYHINT0"));
    unsigned char storyHint1 = g_pAppManager->m_pClientExoApp->GetStoryHint(1);
    pGff->WriteFieldBYTE(&topStruct, storyHint1, const_cast<char*>("STORYHINT1"));
    unsigned char storyHint2 = g_pAppManager->m_pClientExoApp->GetStoryHint(2);
    pGff->WriteFieldBYTE(&topStruct, storyHint2, const_cast<char*>("STORYHINT2"));
    unsigned char storyHint3 = g_pAppManager->m_pClientExoApp->GetStoryHint(3);
    pGff->WriteFieldBYTE(&topStruct, storyHint3, const_cast<char*>("STORYHINT3"));
    unsigned char storyHint4 = g_pAppManager->m_pClientExoApp->GetStoryHint(4);
    pGff->WriteFieldBYTE(&topStruct, storyHint4, const_cast<char*>("STORYHINT4"));
    unsigned char storyHint5 = g_pAppManager->m_pClientExoApp->GetStoryHint(5);
    pGff->WriteFieldBYTE(&topStruct, storyHint5, const_cast<char*>("STORYHINT5"));
    unsigned char storyHint6 = g_pAppManager->m_pClientExoApp->GetStoryHint(6);
    pGff->WriteFieldBYTE(&topStruct, storyHint6, const_cast<char*>("STORYHINT6"));
    unsigned char storyHint7 = g_pAppManager->m_pClientExoApp->GetStoryHint(7);
    pGff->WriteFieldBYTE(&topStruct, storyHint7, const_cast<char*>("STORYHINT7"));
    unsigned char storyHint8 = g_pAppManager->m_pClientExoApp->GetStoryHint(8);
    pGff->WriteFieldBYTE(&topStruct, storyHint8, const_cast<char*>("STORYHINT8"));
    unsigned char storyHint9 = g_pAppManager->m_pClientExoApp->GetStoryHint(9);
    pGff->WriteFieldBYTE(&topStruct, storyHint9, const_cast<char*>("STORYHINT9"));
#endif

    KOTOR_AUTOSAVE_PARAMS params;
    {
        CExoString music = g_pAppManager->m_pClientExoApp->GetLoadMusicByModuleName(moduleName);
        params.m_loadMusic = music;
    }

    CExoArrayList<CExoString>* pMoviesSrc = g_pAppManager->m_pClientExoApp->GetMoveToModuleMovies();
    CExoArrayList<CExoString> movies(*pMoviesSrc);
    {
        CExoString m = (static_cast<int>(movies.m_count) < 1) ? CExoString("") : CExoString(movies.m_pArray[0]);
        params.m_movie1 = m;
    }
    {
        CExoString m = (static_cast<int>(movies.m_count) < 2) ? CExoString("") : CExoString(movies.m_pArray[1]);
        params.m_movie2 = m;
    }
    {
        CExoString m = (static_cast<int>(movies.m_count) < 3) ? CExoString("") : CExoString(movies.m_pArray[2]);
        params.m_movie3 = m;
    }
    {
        CExoString m = (static_cast<int>(movies.m_count) < 4) ? CExoString("") : CExoString(movies.m_pArray[3]);
        params.m_movie4 = m;
    }
    {
        CExoString m = (static_cast<int>(movies.m_count) < 5) ? CExoString("") : CExoString(movies.m_pArray[4]);
        params.m_movie5 = m;
    }
    {
        CExoString m = (static_cast<int>(movies.m_count) < 6) ? CExoString("") : CExoString(movies.m_pArray[5]);
        params.m_movie6 = m;
    }

    {
        CWorldTimer* pTimer = static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer());
        unsigned int worldDay, worldTod;
        pTimer->GetWorldTime(&worldDay, &worldTod);
        pTimer = static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer());
        unsigned int cfHour, cfMinute, cfSecond, cfMs;
        pTimer->ConvertFromTimeOfDay(worldTod, &cfHour, &cfMinute, &cfSecond, &cfMs);

        CSWSModule* pModule = GetModule();

        unsigned int gtDate, gtHour, gtMinute, gtSecond, gtTimeOfDayElapsed;
        unsigned char gtMs;
        pModule->GetTime(&gtDate, &gtHour, &gtMinute, &gtSecond, &gtMs, &gtTimeOfDayElapsed);

        params.m_year = gtDate;
        params.m_month = static_cast<unsigned char>(gtHour);
        params.m_day = static_cast<unsigned char>(gtMinute);
        params.m_hour = static_cast<unsigned char>(gtSecond);
        params.m_minute = static_cast<unsigned short>(cfMinute);
        params.m_second = static_cast<unsigned short>(cfSecond);
        params.m_millisecond = static_cast<unsigned short>(cfMs);
        params.m_pauseDay = worldDay;
        params.m_pauseTime = worldTod;
    }

    {
        CExoString waypoint = g_pAppManager->m_pServerExoApp->GetMoveToModuleStartWaypoint();
        params.m_startWaypoint = waypoint;
    }

    g_pAppManager->m_pClientExoApp->GetInGameGui()->GetStatusSummary(params.m_statusSummary);

    CResStruct autosaveStruct;
    if (pGff->AddStructToStruct(&autosaveStruct, &topStruct, const_cast<char*>("AUTOSAVEPARAMS"), 0) != 0)
    {
        params.SaveToGFF(pGff, autosaveStruct);
    }

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step / 5, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CExoString portraitLabel;
    if (pParty->m_nCharacterCount > 0)
    {
        for (int i = 0; i < pParty->m_nCharacterCount; i++)
        {
            CSWCCreature* pClientCreature = pParty->GetCharacter(i);
            CSWSObject* pServerObj = pClientCreature->GetServerCreature();
            if (pServerObj != 0)
            {
                CSWSCreature* pServerCreature = static_cast<CSWSCreature*>(pServerObj);
                portraitLabel.Format(const_cast<char*>("%s%d"), "PORTRAIT", i);
                CSWCCreature* pClientCr = pServerCreature->GetClientCreature();
                if (pClientCr == 0)
                {
                    CResRef portraitRef = pServerCreature->GetPortrait();
                    pGff->WriteFieldCResRef(&topStruct, portraitRef, static_cast<char*>(portraitLabel.CStr()));
                }
                else
                {
                    CResRef portraitRef = pClientCr->GetPortrait(1);
                    pGff->WriteFieldCResRef(&topStruct, portraitRef, static_cast<char*>(portraitLabel.CStr()));
                }
            }
        }
    }

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step >> 2, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    pGff->WriteGFFFile(saveGameName, 0);
    {
        delete pGff;
    }

    CERFFile* pErf = new CERFFile();
    CExoString erfPath = saveDir + CExoString("SAVEGAME");
    pErf->Create(erfPath);
    pErf->SetVersion(const_cast<char*>("MOD V1.0"));
    pErf->WriteHeader();
    pErf->WriteStringTable();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step * 30 / 100, 0);
    UpdateScreen(0.033333335f, 0, 0);
    {
        CExoString gipPrefix("GAMEINPROGRESS:");
        pErf->ImportFiles(gipPrefix, reinterpret_cast<void*>(-1), UpdateLoadScreen, step * 30 / 100, step * 60 / 100);
    }
    pErf->Finish();
    pErf->~CERFFile();
    operator delete(pErf);

    CExoString pifoDst = saveDir + CExoString("pifo");
    g_pExoResMan->FileCopy(CExoString("TEMP:pifo"), pifoDst, RESTYPE_IFO, RESTYPE_INVALID);

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(step, 0);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);
}

// Writes the autosave-params sub-struct of the savenfo GFF: the load-music, start-waypoint and movie
// strings, the TIME_* world-time scalars, then a status-summary sub-struct.
void KOTOR_AUTOSAVE_PARAMS::SaveToGFF(CResGFF* pGff, CResStruct& pStruct)
{
    pGff->WriteFieldCExoString(&pStruct, m_loadMusic, const_cast<char*>("LOADMUSIC"));
    pGff->WriteFieldCExoString(&pStruct, m_startWaypoint, const_cast<char*>("STARTWAYPOINT"));
    pGff->WriteFieldCExoString(&pStruct, m_movie1, const_cast<char*>("MOVIE1"));
    pGff->WriteFieldCExoString(&pStruct, m_movie2, const_cast<char*>("MOVIE2"));
    pGff->WriteFieldCExoString(&pStruct, m_movie3, const_cast<char*>("MOVIE3"));
    pGff->WriteFieldCExoString(&pStruct, m_movie4, const_cast<char*>("MOVIE4"));
    pGff->WriteFieldCExoString(&pStruct, m_movie5, const_cast<char*>("MOVIE5"));
    pGff->WriteFieldCExoString(&pStruct, m_movie6, const_cast<char*>("MOVIE6"));
    pGff->WriteFieldDWORD(&pStruct, m_year, const_cast<char*>("TIME_YEAR"));
    pGff->WriteFieldBYTE(&pStruct, m_month, const_cast<char*>("TIME_MONTH"));
    pGff->WriteFieldBYTE(&pStruct, m_day, const_cast<char*>("TIME_DAY"));
    pGff->WriteFieldBYTE(&pStruct, m_hour, const_cast<char*>("TIME_HOUR"));
    pGff->WriteFieldWORD(&pStruct, m_minute, const_cast<char*>("TIME_MINUTE"));
    pGff->WriteFieldWORD(&pStruct, m_second, const_cast<char*>("TIME_SECOND"));
    pGff->WriteFieldWORD(&pStruct, m_millisecond, const_cast<char*>("TIME_MILLISECOND"));
    pGff->WriteFieldDWORD(&pStruct, m_pauseDay, const_cast<char*>("TIME_PAUSEDAY"));
    pGff->WriteFieldDWORD(&pStruct, m_pauseTime, const_cast<char*>("TIME_PAUSETIME"));
    CResStruct subStruct;
    pGff->AddStructToStruct(&subStruct, &pStruct, const_cast<char*>("STATUSSUMMARY"), 0);
    m_statusSummary.SaveToGFF(pGff, subStruct);
}
