#include "k2stdint.h"
#include "nwchandlemessage.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "netlayer.h"
#include "Vector.h"
#include "exoresman.h"
#include "serverexoapp.h"
#include "exosound.h"
#include "nwcitem.h"
#include "guiingame.h"
#include "gameobjectarray.h"
#include "exofile.h"
#include "nwccreaturestats.h"
#include "nwsrules.h"
#include "List.h"

extern CExoSound* g_pExoSound;

#include "port/libcstring.h"
#include "nwmessagetypes.h"

// Resource types (CExoResMan, CExoFile and the directory lister).
enum {
    kResTypeBic = 2015,
};

extern int g_bDemoMode;
#if !KOTOR1
extern int g_bDemoMode;
#endif

CExoString g_returnCode;

int CSWCMessage::SendPlayerToServerArea_AreaLoaded()
{
    return SendPlayerToServerMessage(MSG_MAJOR_AREA, MSG_AREA_LOADED, 0, 0);
}

// Stamps the three-byte client header over an already-built payload and sends it to the server
// through the in-process net layer. An empty payload gets a bare 3-byte header.
int CSWCMessage::SendPlayerToServerMessage(unsigned char nCategory, unsigned char nType,
                                           unsigned char* pBuffer, unsigned int nSize)
{
    unsigned int nSend = nSize;
    if (nSize == 0)
    {
        pBuffer = new unsigned char[3];
        nSend = 3;
    }

    int bSent = 0;
    if (pBuffer != 0)
    {
        pBuffer[0] = MSG_PLAYER_TO_SERVER;
        pBuffer[1] = nCategory;
        pBuffer[2] = nType;

        bSent = g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(
            CNetLayer::PLAYERID_SERVER, pBuffer, nSend, 0);
        if (nSize == 0)
            delete[] pBuffer;
    }

    return bSent;
}

int CSWCMessage::SendPlayerToServerCheat_ComputeSafeLocation()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_COMPUTE_SAFE_LOCATION, 0, 0);
}

int CSWCMessage::SendPlayerToServerCheat_ShowServerMem()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_SHOW_SERVER_MEM, 0, 0);
}

int CSWCMessage::SendPlayerToServerCheat_Invulnerability()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_INVULNERABILITY, 0, 0);
}

int CSWCMessage::SendPlayerToServerCheat_ToggleFloatyEffects()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_TOGGLE_FLOATY_EFFECTS, 0, 0);
}

int CSWCMessage::SendPlayerToServerInput_AbortDriveControl()
{
    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_ABORT_DRIVE_CONTROL, 0, 0);
}

int CSWCMessage::SendPlayerToServerInput_CancelGuiTimingEvent()
{
    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CANCEL_GUI_TIMING_EVENT, 0, 0);
}

int CSWCMessage::SendPlayerToServerInput_Rest()
{
    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_REST, 0, 0);
}

int CSWCMessage::SendPlayerToServerInput_StopDragMode()
{
    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_STOP_DRAG_MODE, 0, 0);
}

int CSWCMessage::SendPlayerToServerLogin_LoginIFOCharacter()
{
    return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_IFO_CHARACTER, 0, 0);
}

int CSWCMessage::SendPlayerToServer_RequestServerCharacterList()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHAR_LIST, MSG_CHAR_LIST_REQUEST, 0, 0);
}

int CSWCMessage::SendPlayerToServerModule_ModuleLoaded()
{
    return SendPlayerToServerMessage(MSG_MAJOR_MODULE, MSG_MODULE_LOADED, 0, 0);
}

int CSWCMessage::SendPlayerToServerModule_RunModule()
{
    return SendPlayerToServerMessage(MSG_MAJOR_MODULE, MSG_MODULE_RUN_MODULE, 0, 0);
}

int CSWCMessage::SendPlayerToServerInput_ExportRequest()
{
    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_EXPORT_REQUEST, 0, 0);
}

int CSWCMessage::SendPlayerToServerStore_Close()
{
    return SendPlayerToServerMessage(MSG_MAJOR_STORE, MSG_STORE_CLOSE, 0, 0);
}

int CSWCMessage::SendPlayerToServerJournalMessage_QuestScreenOpen()
{
    return SendPlayerToServerMessage(MSG_MAJOR_JOURNAL, MSG_JOURNAL_QUEST_SCREEN_OPEN, 0, 0);
}

int CSWCMessage::SendPlayerToServerJournalMessage_QuestScreenClosed()
{
    return SendPlayerToServerMessage(MSG_MAJOR_JOURNAL, MSG_JOURNAL_QUEST_SCREEN_CLOSED, 0, 0);
}

int CSWCMessage::SendPlayerToServerCheatLevelUp()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_LEVEL_UP, 0, 0);
}

// Ships the level-up delta between pReal (the live, already-levelled stats) and pCopy (the
// character generator's pristine backup): the class, at most one ability index, a 32-bit skill mask
// with one signed delta per set bit, the skill-point pool, the feat-count delta and the newly
// acquired feats, then the added and removed spell lists.
int CSWCMessage::SendPlayerToServer_LevelUp(CSWCLevelUpStats* pCopy, CSWCLevelUpStats* pReal)
{
    unsigned int nSize = 100;
    CreateWriteMessage(100, MSG_PLAYERID_ALL_CLIENTS, 1);

    WriteBYTE(pReal->GetClassLeveledUpIn(), 8);

    if ((pReal->GetLevel() & 3) == 0)
    {
        for (int i = 0; i < 6; i++) {
            if (pReal->m_abScoresBase[i] > pCopy->m_abScoresBase[i]) {
                WriteBYTE(static_cast<unsigned char>(i), 8);
                break;
            }
        }
    }

    unsigned int nMask = 0;
    for (uint64_t i = 0; i < g_pRules->m_nNumSkills; i++)
        if (pReal->m_pSkillRanksBase[i] != pCopy->m_pSkillRanksBase[i])
            nMask |= 1 << i;

    WriteDWORD(nMask, 32);

    for (uint64_t i = 0; i < g_pRules->m_nNumSkills; i++)
        if (nMask & (1 << i))
            WriteCHAR(pReal->m_pSkillRanksBase[i] - pCopy->m_pSkillRanksBase[i], 8);

    WriteWORD(pReal->m_nSkillPointsRemaining, 16);

    WriteBYTE(static_cast<unsigned char>(pReal->m_lstFeats.m_count) - static_cast<unsigned char>(pCopy->m_lstFeats.m_count), 8);

    for (unsigned short i = 0; i < static_cast<unsigned short>(pReal->m_lstFeats.m_count); i++) {
        unsigned short nFeat = pReal->GetFeat(i);
        if (pCopy->FeatAcquired(nFeat, 0) == 0)
            WriteWORD(nFeat, 16);
    }

    {
    List<int> lstAdd;
    List<int> lstRemove;
    unsigned char nClassPos = pReal->m_nNumClasses - 1;

    for (unsigned char i = 0; i < pReal->GetNumberKnownSpells(nClassPos, 0); i++) {
        int nSpell = pReal->GetKnownSpell(nClassPos, 0, i);
        int bFound = 0;
        for (unsigned char j = 0; j < pCopy->GetNumberKnownSpells(nClassPos, 0); j++) {
            if (pCopy->GetKnownSpell(nClassPos, 0, j) == nSpell) {
                bFound = 1;
                break;
            }
        }
        if (bFound == 0)
            lstAdd.Add(nSpell);
    }

    for (unsigned char i = 0; i < pCopy->GetNumberKnownSpells(nClassPos, 0); i++) {
        int nSpell = pCopy->GetKnownSpell(nClassPos, 0, i);
        int bFound = 0;
        for (unsigned char j = 0; j < pReal->GetNumberKnownSpells(nClassPos, 0); j++) {
            if (pReal->GetKnownSpell(nClassPos, 0, j) == nSpell) {
                bFound = 1;
                break;
            }
        }
        if (bFound == 0)
            lstRemove.Add(nSpell);
    }

    WriteBYTE(static_cast<unsigned char>(lstAdd.m_count), 8);
    for (unsigned char i = 0; i < lstAdd.m_count; i++)
        WriteDWORD(lstAdd.m_pData[i], 32);

    WriteBYTE(static_cast<unsigned char>(lstRemove.m_count), 8);
    for (unsigned char i = 0; i < lstRemove.m_count; i++)
        WriteDWORD(lstRemove.m_pData[i], 32);
    }

    unsigned char* pMsg;
    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;
    return SendPlayerToServerMessage(MSG_MAJOR_LEVEL_UP, MSG_LEVEL_UP_REQUEST, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerGuiPlayerDeath_Respawn()
{
    return SendPlayerToServerMessage(MSG_MAJOR_DEATH, MSG_DEATH_RESPAWN, 0, 0);
}

int CSWCMessage::SendPlayerToServerCharacterDownloadGimme()
{
    return SendPlayerToServerMessage(MSG_MAJOR_CHARACTER_DOWNLOAD, MSG_CHARACTER_DOWNLOAD_REQUEST,
                                     0, 0);
}

int CSWCMessage::SendPlayerToServerPlayModuleCharacterList_Start()
{
    return SendPlayerToServerMessage(MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST,
                                     MSG_PLAY_MODULE_CHARACTER_LIST_START, 0, 0);
}

int CSWCMessage::SendPlayerToServerPlayModuleCharacterList_Stop()
{
    return SendPlayerToServerMessage(MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST,
                                     MSG_PLAY_MODULE_CHARACTER_LIST_STOP, 0, 0);
}

int CSWCMessage::SendPlayerToServerChat_Party(const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sText.GetLength() + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0) + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_PARTY, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerChat_Shout(const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sText.GetLength() + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0) + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_SHOUT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerChat_Talk(const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sText.GetLength() + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0) + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_TALK, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerChat_Whisper(const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sText.GetLength() + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0) + 4,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_WHISPER, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_PlayerPathfindRule(unsigned char nRule)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(64, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nRule, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_PLAYER_PATHFIND_RULE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_Kill(unsigned long long objId,
                                              unsigned long long objId2)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteOBJECTIDClient(objId2);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_KILL, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_ToggleCombatDebugging(int bOn)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bOn);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_TOGGLE_COMBAT_DEBUGGING, pMsg,
                                     nSize);
}

int CSWCMessage::SendPlayerToServerCheat_ToggleMovementSpeedDebugging(int bOn)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bOn);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_TOGGLE_MOVEMENT_SPEED_DEBUGGING,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_PonyRide()
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(1);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_PONY_RIDE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_RainOfCows()
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(1);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_RAIN_OF_COWS, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_DebugMode(int bOn)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bOn);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_DEBUG_MODE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerGuiInventory_Status(int nStatus)
{
    unsigned char* pMsg;
    unsigned int nSize = 1;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    // The wire byte is a predicate, not the argument, and is 1 only when the status is exactly 1.
    WriteBYTE(nStatus == 1, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_INVENTORY, MSG_GUI_INVENTORY_STATUS, pMsg,
                                     nSize);
}

int CSWCMessage::SendPlayerToServerGuiCharacterSheet_Status(char nStatus,
                                                            unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 9;

    CreateWriteMessage(9, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR(nStatus, 8);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_CHARACTER_SHEET, MSG_GUI_CHARACTER_SHEET_STATUS,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_AlwaysRun(int b)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(b);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_ALWAYS_RUN, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_Attack(unsigned long long objId,
                                                unsigned short a)
{

    unsigned char* pMsg = 0;
    unsigned int nSize = 10;

    CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteWORD(a, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_ATTACK, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_UseFeat(unsigned short nFeat, unsigned short nSubFeat,
                                                 unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 22;

    CreateWriteMessage(22, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteWORD(nFeat, 16);
    WriteWORD(nSubFeat, 16);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_USE_FEAT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_SwitchWeapons(unsigned long long objId, int bSwap)
{
    unsigned char* pMsg;
    unsigned int nSize = 12;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteBOOL(bSwap);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_SWITCH_WEAPONS, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_Stance(char nStance, unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 9;

    CreateWriteMessage(9, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR(nStance, 8);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_STANCE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_ToggleMode(unsigned char mode, unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 1;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(mode, 8);
    // The message has a variable length on the wire; the target id ships only in mode 5.
    if (mode == 5)
        WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_TOGGLE_MODE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_Examine(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_EXAMINE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_UseObject(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_USE_OBJECT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_ChangeDoorState(unsigned long long objId,
                                                          unsigned short state)
{
    unsigned char* pMsg;
    unsigned int nSize = 10;

    CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteWORD(state, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CHANGE_DOOR_STATE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_UnlockObject(unsigned long long objId,
                                                       unsigned long long a)
{
    unsigned char* pMsg;
    unsigned int nSize = 16;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteOBJECTIDClient(a);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_UNLOCK_OBJECT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_LockObject(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_LOCK_OBJECT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_DoorSaber(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_DOOR_SABER, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInventory_Equip(unsigned long long itemId,
                                                    unsigned int nSlot)
{
    unsigned char* pMsg;
    unsigned int nSize = 12;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteDWORD(nSlot, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_EQUIP, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInventory_Pickup(unsigned long long objId,
                                                     unsigned long long objId2)
{
    unsigned char* pMsg;
    unsigned int nSize = 16;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteOBJECTIDClient(objId2);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_PICKUP, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInventory_Unequip(unsigned long long itemId,
                                                      unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 16;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_UNEQUIP, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInventory_Use(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_USE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerGuiContainer_Close(unsigned long long objId, int bCancel)
{
    unsigned char* pMsg;
    unsigned int nSize = 12;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteBOOL(bCancel);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_CONTAINER, MSG_GUI_CONTAINER_CLOSE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_AssociateCommand(unsigned long long objId,
                                                            int nCommand)
{
    unsigned char* pMsg;
    unsigned int nSize = 12;

    CreateWriteMessage(12, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteINT(nCommand, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_ASSOCIATE_COMMAND, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerChat_Tell(unsigned int nPlayerId, const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sText.GetLength() + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0) + 12, MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    // Zero here; Chat_TellName sets the "addressed by name" flag instead.
    WriteBOOL(0);
    WriteDWORD(nPlayerId, 32);
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_TELL, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_MoveToArea(const CExoString& sArea, const Vector& pos)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(64, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sArea, 32);
    // A Vector ships as three WriteFLOAT calls with a 1.0f scale.
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_MOVE_TO_AREA, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_SummonNasty(unsigned long long objId,
                                                     unsigned char nType,
                                                     const CResRef& resRef)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(25, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nType, 8);
    WriteCResRef(resRef, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_SUMMON_NASTY, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_RunScript(const CExoString& sScript)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sScript.GetLength() + 4, MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sScript.m_sString ? static_cast<unsigned int>(strlen(sScript.m_sString)) : 0) + 4, MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCExoString(sScript, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_RUN_SCRIPT, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerDialog_Reply(unsigned long long objId, unsigned int nReply,
                                                unsigned char a, unsigned int b)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(13, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteDWORD(nReply, 32);
    WriteBYTE(a, 8);
    WriteDWORD(b, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_DIALOG, MSG_DIALOG_REPLY, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_UseSkill(unsigned char skill, unsigned char subType,
                                                  unsigned long long objId, const Vector& pos)
{
    unsigned char* pMsg;
    unsigned int nSize = 22;

    CreateWriteMessage(22, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(skill, 8);
    WriteBYTE(subType, 8);
    WriteOBJECTIDClient(objId);
    // A Vector ships as three WriteFLOAT calls with a 1.0f scale.
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_USE_SKILL, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServerStatus_RequestStatus()
{
    return SendPlayerToServerMessage(MSG_MAJOR_SERVER_STATUS, MSG_SERVER_STATUS_REQUEST, 0, 0);
}

// Despite the SendPlayerToServer* name this member sends nothing: the client toggles the
// in-process server's pause state directly and reports success unconditionally.
int CSWCMessage::SendPlayerToServerInput_TogglePauseRequest()
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer != 0)
        pServer->TogglePauseState(2);

    return 1;
}

// Plays the heal cue locally before asking the server to resurrect.
int CSWCMessage::SendPlayerToServerCheat_Resurrect()
{
    g_pExoSound->PlayOneShotSound(CResRef("HEAL_PLAYER"), 0x15, 0, 0, 0.0f, 0.0f);

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_RESURRECT, 0, 0);
}

int CSWCMessage::SendPlayerToServerShutDownServer(int bShutDown)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bShutDown);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_SHUTDOWN_SERVER, MSG_SHUTDOWN_SERVER_REQUEST, pMsg,
                                     nSize);
}

int CSWCMessage::SendPlayerToServerLogin_CharacterResponse(int bAccept)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(1, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bAccept);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_CHARACTER_RESPONSE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerQuickChat(unsigned short nChat)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(6, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteWORD(nChat, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_QUICK_CHAT, MSG_QUICK_CHAT_MESSAGE, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServerInput_GetObjectDebugInfo(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_GET_OBJECT_DEBUG_INFO, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerMapPinRemoved(int nPin)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nPin, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_MAP_PIN, MSG_MAP_PIN_REMOVE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInventory_LearnScroll(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 32;

    CreateWriteMessage(32, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_LEARN_SCROLL, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_SetPauseRequest(int bPause)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBOOL(bPause);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_SET_PAUSE_REQUEST, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_CancelDisguise(unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CANCEL_DISGUISE, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServer_RequestCharacterListUpdate(unsigned char nLocation,
                                                               const CResRef& resRef)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(17, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nLocation, 8);
    WriteCResRef(resRef, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAR_LIST, MSG_CHAR_LIST_REQUEST_UPDATE, pMsg,
                                     nSize);
}

int CSWCMessage::SendPlayerToServerInput_MergeItem(unsigned long long itemId,
                                                   unsigned long long targetId)
{
    unsigned char* pMsg;
    unsigned int nSize = 16;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteOBJECTIDClient(targetId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_MERGE_ITEM, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_SplitItem(unsigned long long itemId, int nCount)
{
    unsigned char* pMsg;
    unsigned int nSize = 10;

    CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteWORD(static_cast<unsigned short>(nCount), 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_SPLIT_ITEM, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerJournalMessage_RequestDelete(int nEntry, int bDelete)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteINT(nEntry, 32);
    WriteBOOL(bDelete);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_JOURNAL, MSG_JOURNAL_REQUEST_DELETE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_CancelAction(unsigned short nActionId,
                                                      unsigned short nGroupId)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteWORD(nActionId, 16);
    WriteWORD(nGroupId, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CANCEL_ACTION, pMsg, nSize);
}

// One of four Update_Appearance members, told apart by a leading tag character written with
// WriteCHAR before the object id: 'H' head variation, 'G' gender, 'R' race, 'T' phenotype.

int CSWCMessage::SendPlayerToServerUpdate_AppearanceHeadVariation(unsigned long long objId,
                                                                  unsigned char nVariation)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(2, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('H', 8);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nVariation, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerUpdate_AppearanceGender(unsigned long long objId,
                                                           unsigned char nGender)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(2, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('G', 8);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nGender, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerUpdate_AppearanceRace(unsigned long long objId,
                                                         unsigned short nRace)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(3, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('R', 8);
    WriteOBJECTIDClient(objId);
    WriteWORD(nRace, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerUpdate_AppearancePhenoType(unsigned long long objId,
                                                              unsigned char nPhenoType)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(2, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('T', 8);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nPhenoType, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

// Flags the item as sale-pending client-side once the message has been built.
int CSWCMessage::SendPlayerToServerStore_RequestSell(unsigned long long itemId)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0)
        pItem->m_bInventoryRequestPending = 1;

    return SendPlayerToServerMessage(MSG_MAJOR_STORE, MSG_STORE_REQUEST_SELL, pMsg, nSize);
}

// Only X and Y of the position ship; a turn on the spot has no Z.
int CSWCMessage::SendPlayerToServerInput_TurnOnSpot(const Vector& dir)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteFLOAT(dir.x, 1.0f, 32);
    WriteFLOAT(dir.y, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_TURN_ON_SPOT, pMsg, nSize);
}

// A running conversation suppresses the request, which reports success without sending anything.
int CSWCMessage::SendPlayerToServerInput_Dialog(unsigned long long objId)
{
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return 1;

    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_DIALOG, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServerModule_SaveGame(unsigned int nSlot, const CExoString& sName,
                                                   const CExoString& sDesc)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nSlot, 32);
    WriteCExoString(sName, 32);
    WriteCExoString(sDesc, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_MODULE, MSG_MODULE_SAVE_GAME, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerModule_LoadGame(unsigned int nSlot, const CExoString& sName,
                                                   const CExoString& sDesc)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(128, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nSlot, 32);
    WriteCExoString(sName, 32);
    WriteCExoString(sDesc, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_MODULE, MSG_MODULE_LOAD_GAME, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerSpellUnMemorize(unsigned char nClassSlot, unsigned char nLevel,
                                                   unsigned char nSlot)
{
    unsigned char* pMsg;
    unsigned int nSize = 3;

    CreateWriteMessage(3, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nClassSlot, 8);
    WriteBYTE(nLevel, 8);
    WriteBYTE(nSlot, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_SPELL_UNMEMORIZE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_GiveItem(unsigned long long giverId,
                                                  unsigned long long itemId,
                                                  unsigned long long objId, int nCount)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(28, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(giverId);
    WriteOBJECTIDClient(itemId);
    WriteOBJECTIDClient(objId);
    WriteINT(nCount, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_GIVE_ITEM, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerUpdate_AppearancePartVariation(unsigned long long objId,
                                                                  unsigned char nPart,
                                                                  unsigned char nVariation)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(3, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('V', 8);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nPart, 8);
    WriteBYTE(nVariation, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerUpdate_AppearancePLTColor(unsigned long long objId,
                                                             unsigned char nLayer,
                                                             unsigned char nColor)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(3, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('L', 8);
    WriteOBJECTIDClient(objId);
    WriteBYTE(nLayer, 8);
    WriteBYTE(nColor, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

// The second payload byte is the quickbar entry's type tag.
int CSWCMessage::SendPlayerToServerGuiQuickbar_SetButton_SpellLikeAbility(unsigned char nButton,
                                                                          unsigned int nSpellId,
                                                                          unsigned char nSubType)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(7, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nButton, 8);
    WriteBYTE(MSG_QUICKBAR_BUTTON_SPELL_LIKE_ABILITY, 8);
    WriteDWORD(nSpellId, 32);
    WriteBYTE(nSubType, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_QUICKBAR, MSG_GUI_QUICKBAR_SET_BUTTON, pMsg,
                                     nSize);
}

// The last two payload fields ship in the reverse of the parameter order, the quickbar slot before
// the class.
int CSWCMessage::SendPlayerToServerGuiQuickbar_SetButton_Spell(unsigned char nButton,
                                                               unsigned char nSubType,
                                                               unsigned int nSpellId,
                                                               unsigned char nClass,
                                                               unsigned char nSlot)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nButton, 8);
    WriteBYTE(MSG_QUICKBAR_BUTTON_SPELL, 8);
    WriteBYTE(nSubType, 8);
    WriteDWORD(nSpellId, 32);
    WriteBYTE(nSlot, 8);
    WriteBYTE(nClass, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_QUICKBAR, MSG_GUI_QUICKBAR_SET_BUTTON, pMsg,
                                     nSize);
}

// The message has a variable length. The second object id ships only when it is a real id, behind a
// leading "has a second item" flag.
int CSWCMessage::SendPlayerToServerInventory_EquipToggle(unsigned long long itemId1,
                                                         unsigned long long itemId2)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId1);

    if (itemId2 == CGameObjectArray::INVALIDOBJID)
    {
        WriteBOOL(0);
    }
    else
    {
        WriteBOOL(1);
        WriteOBJECTIDClient(itemId2);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_EQUIP_TOGGLE, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServerLogin_LoginLocalCharacter(unsigned char* pData,
                                                             unsigned int nLen, int bJoin)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(nLen + 4, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteDWORD(nLen, 32);
    WriteVOIDPtr(pData, nLen);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    if (bJoin)
        return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_LOCAL_CHARACTER_JOIN, pMsg,
                                         nSize);

    return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_LOCAL_CHARACTER, pMsg, nSize);
}


int CSWCMessage::SendPlayerToServerLogin_LoginServerCharacter(const CResRef& resRef, int bJoin,
                                                              int bCreate)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(16, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCResRef(resRef, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    if (bJoin)
        return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_SERVER_CHARACTER_JOIN, pMsg,
                                         nSize);

    if (bCreate)
        return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_SERVER_CHARACTER_CREATE, pMsg,
                                         nSize);

    return SendPlayerToServerMessage(MSG_MAJOR_LOGIN, MSG_LOGIN_SERVER_CHARACTER, pMsg, nSize);
}

// Same sale-pending latch as Store_RequestSell, raised inside the success path.
int CSWCMessage::SendPlayerToServerStore_RequestBuy(unsigned long long itemId,
                                                    unsigned char nStack, unsigned char nSlot)
{
    unsigned char* pMsg;
    unsigned int nSize = 10;

    CreateWriteMessage(10, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteBYTE(nStack, 8);
    WriteBYTE(nSlot, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0)
        pItem->m_bInventoryRequestPending = 1;

    return SendPlayerToServerMessage(MSG_MAJOR_STORE, MSG_STORE_REQUEST_BUY, pMsg, nSize);
}

// Two strings, so the reserve is the sum of two null-guarded lengths plus the fixed header.
int CSWCMessage::SendPlayerToServerJournalMessage_RequestAdd(const CExoString& sTitle,
                                                             const CExoString& sText)
{
    unsigned char* pMsg;
#if KOTOR1  // K1 calls CExoString::GetLength out of line
    unsigned int nTitle = sTitle.GetLength();
    unsigned int nText = sText.GetLength();
#else
    unsigned int nTitle = (sTitle.m_sString ? static_cast<unsigned int>(strlen(sTitle.m_sString)) : 0);
    unsigned int nText = (sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0);
#endif
    unsigned int nSize = nTitle + nText + 4;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCExoString(sTitle, 32);
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_JOURNAL, MSG_JOURNAL_REQUEST_ADD, pMsg, nSize);
}

// Sends a Chat_Tell addressed by name. It raises the leading "addressed by name" flag and ships the
// recipient's name instead of a player id.
int CSWCMessage::SendPlayerToServerChat_TellName(const CExoString& sName, const CExoString& sText)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sName.GetLength() + 12
                       + sText.GetLength(),
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sName.m_sString ? static_cast<unsigned int>(strlen(sName.m_sString)) : 0) + 12
                       + (sText.m_sString ? static_cast<unsigned int>(strlen(sText.m_sString)) : 0),
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteBOOL(1);
    WriteCExoString(sName, 32);
    WriteCExoString(sText, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHAT, MSG_CHAT_TELL, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerCheat_PlayVisualEffect(unsigned long long objId,
                                                          unsigned short nEffect, float fDuration,
                                                          unsigned long long targetId,
                                                          unsigned char nType)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(23, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteWORD(nEffect, 16);
    WriteFLOAT(fDuration, 1.0f, 32);
    WriteOBJECTIDClient(targetId);
    WriteBYTE(nType, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_PLAY_VISUAL_EFFECT, pMsg, nSize);
}

// Takes three loose floats rather than a Vector.
int CSWCMessage::SendPlayerToServerInventory_Drop(unsigned long long itemId, float x, float y,
                                                  float z)
{
    unsigned char* pMsg;
    unsigned int nSize = 20;

    CreateWriteMessage(20, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteFLOAT(x, 1.0f, 32);
    WriteFLOAT(y, 1.0f, 32);
    WriteFLOAT(z, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INVENTORY, MSG_INVENTORY_DROP, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerSpellMemorize(unsigned char nClassSlot, unsigned int nSpellId,
                                                 unsigned char nLevel, unsigned char nSlot,
                                                 unsigned char nMeta)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nClassSlot, 8);
    WriteDWORD(nSpellId, 32);
    WriteBYTE(nLevel, 8);
    WriteBYTE(nSlot, 8);
    WriteBYTE(nMeta, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_SPELL_MEMORIZE, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerMapPinAdded(const Vector& pos, const CExoString& sNote)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sNote.GetLength() + 12,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sNote.m_sString ? static_cast<unsigned int>(strlen(sNote.m_sString)) : 0) + 12,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);
    WriteCExoString(sNote, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_MAP_PIN, MSG_MAP_PIN_ADD, pMsg, nSize);
}

// The animation id ships first, ahead of the object id.
int CSWCMessage::SendPlayerToServerInput_PlayAnimation(unsigned long long objId,
                                                       const Vector& pos, unsigned short nAnim)
{
    unsigned char* pMsg;
    unsigned int nSize = 22;

    CreateWriteMessage(22, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteWORD(nAnim, 16);
    WriteOBJECTIDClient(objId);
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_PLAY_ANIMATION, pMsg, nSize);
}

// The heading ships at twelve bits, not the type's natural sixteen; only X and Y of the direction
// ship.
int CSWCMessage::SendPlayerToServerInput_DriveControl(const Vector& dir, unsigned short nHeading,
                                                      unsigned char a, unsigned char b)
{
    unsigned char* pMsg;
    unsigned int nSize = 11;

    CreateWriteMessage(11, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteFLOAT(dir.x, 1.0f, 32);
    WriteFLOAT(dir.y, 1.0f, 32);
    WriteWORD(nHeading, 12);
    WriteBYTE(a, 8);
    WriteBYTE(b, 8);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_DRIVE_CONTROL, pMsg, nSize);
}

// Has a variable length, like Inventory_EquipToggle. The trailing target id ships only when it is
// real, behind its own flag. Payload byte 1 is the quickbar type tag, 1 for an item.
int CSWCMessage::SendPlayerToServerGuiQuickbar_SetButton_Item(unsigned char nButton,
                                                              unsigned long long itemId,
                                                              int nCount,
                                                              unsigned long long targetId)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(14, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteBYTE(nButton, 8);
    WriteBYTE(MSG_QUICKBAR_BUTTON_ITEM, 8);
    WriteOBJECTIDClient(itemId);
    WriteINT(nCount, 32);

    if (targetId == CGameObjectArray::INVALIDOBJID)
    {
        WriteBOOL(0);
    }
    else
    {
        WriteBOOL(1);
        WriteOBJECTIDClient(targetId);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GUI_QUICKBAR, MSG_GUI_QUICKBAR_SET_BUTTON, pMsg,
                                     nSize);
}

// The reserve itself is conditional, 26 bytes when the gold is dropped at a point and 14 when it is
// handed to an object, and the same test gates the three floats at the end.
int CSWCMessage::SendPlayerToServerGold_TransferGold(unsigned long long objId, const Vector& pos,
                                                     int nAmount)
{
    unsigned char* pMsg;
    unsigned int nSize = (objId == OBJECT_INVALID) ? 26 : 14;

    CreateWriteMessage(nSize, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteINT(nAmount, 32);

    if (objId == OBJECT_INVALID)
    {
        WriteFLOAT(pos.x, 1.0f, 32);
        WriteFLOAT(pos.y, 1.0f, 32);
        WriteFLOAT(pos.z, 1.0f, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GOLD, MSG_GOLD_TRANSFER, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerMapPinChanged(const Vector& pos, const CExoString& sNote,
                                                 int nPin)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sNote.GetLength() + 16,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sNote.m_sString ? static_cast<unsigned int>(strlen(sNote.m_sString)) : 0) + 16,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);
    WriteCExoString(sNote, 32);
    WriteINT(nPin, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_MAP_PIN, MSG_MAP_PIN_CHANGE, pMsg, nSize);
}



int CSWCMessage::SendPlayerToServerCreature_Add(unsigned long long objId, const Vector& pos,
                                                const CExoString& sResRef)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sResRef.GetLength() + 21,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sResRef.m_sString ? static_cast<unsigned int>(strlen(sResRef.m_sString)) : 0) + 21,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCHAR('C', 8);
    WriteOBJECTIDClient(objId);
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);
    WriteCExoString(sResRef, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerParty_Add(unsigned long long objId, const Vector& pos,
                                             const CExoString& sResRef)
{
    unsigned char* pMsg;
    unsigned int nSize;

#if KOTOR1  // K1 calls CExoString::GetLength out of line
    CreateWriteMessage(sResRef.GetLength() + 21,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#else
    CreateWriteMessage((sResRef.m_sString ? static_cast<unsigned int>(strlen(sResRef.m_sString)) : 0) + 21,
                       MSG_PLAYERID_ALL_CLIENTS, 1);
#endif
    WriteCHAR('M', 8);
    WriteOBJECTIDClient(objId);
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);
    WriteCExoString(sResRef, 32);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

// The resref ships last here, behind the three counts.
int CSWCMessage::SendPlayerToServerItem_Add(unsigned long long objId, const Vector& pos,
                                            const CResRef& resRef, unsigned char a,
                                            unsigned char b, unsigned char c)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(40, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteCHAR('A', 8);
    WriteOBJECTIDClient(objId);
    WriteFLOAT(pos.x, 1.0f, 32);
    WriteFLOAT(pos.y, 1.0f, 32);
    WriteFLOAT(pos.z, 1.0f, 32);
    WriteBYTE(a, 8);
    WriteBYTE(b, 8);
    WriteBYTE(c, 8);
    WriteCResRef(resRef, 16);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_GAME_OBJECT_UPDATE, MSG_GAME_OBJECT_UPDATE_UPDATE,
                                     pMsg, nSize);
}

// Takes the position as three loose floats rather than a Vector.
int CSWCMessage::SendPlayerToServerInput_WalkToWayPoint(unsigned long long objId, float x, float y,
                                                        float z, unsigned char a, int b, int c,
                                                        unsigned char d, unsigned long long objId2)
{
    unsigned char* pMsg;
    unsigned int nSize = 22;

    CreateWriteMessage(22, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);
    WriteFLOAT(x, 1.0f, 32);
    WriteFLOAT(y, 1.0f, 32);
    WriteFLOAT(z, 1.0f, 32);
    WriteBYTE(a, 8);
    WriteBOOL(b);
    WriteBOOL(c);
    WriteBYTE(d, 8);
    WriteOBJECTIDClient(objId2);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_WALK_TO_WAYPOINT, pMsg, nSize);
}

// Each of the three cast packers ships a bit-mask byte naming which optional fields follow: bit 0
// the target object id, bit 1 the target point, bit 2 (Input_CastSpell only) the caster's own
// "instant" flag. Every optional field is written under the same test that raised its bit, and the
// two Input_* members offset the shipped Z by +0.3f where Cheat_CastSpell does not.

int CSWCMessage::SendPlayerToServerCheat_CastSpell(unsigned int nSpellId, const Vector& pos,
                                                   unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize;

    CreateWriteMessage(24, MSG_PLAYERID_ALL_CLIENTS, 1);

    float fDistSq = dot(pos, pos);
    unsigned char nMask = (objId != OBJECT_INVALID);
    if (fDistSq >= 0.00001f)
        nMask |= 2;

    WriteDWORD(nSpellId, 32);
    WriteBYTE(nMask, 8);

    if (objId != OBJECT_INVALID)
        WriteOBJECTIDClient(objId);

    if (fDistSq >= 0.00001f)
    {
        WriteFLOAT(pos.x, 1.0f, 32);
        WriteFLOAT(pos.y, 1.0f, 32);
        WriteFLOAT(pos.z, 1.0f, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_CHEAT, MSG_CHEAT_CAST_SPELL, pMsg, nSize);
}

int CSWCMessage::SendPlayerToServerInput_CastSpellLikeAbility(unsigned int nSpellId,
                                                              unsigned char nSubType,
                                                              const Vector& pos,
                                                              unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);

    float fDistSq = dot(pos, pos);
    unsigned char nMask = (objId != OBJECT_INVALID);
    if (fDistSq >= 0.00001f)
        nMask |= 2;

    WriteDWORD(nSpellId, 32);
    WriteBYTE(nSubType, 8);
    WriteBYTE(nMask, 8);

    if (objId != OBJECT_INVALID)
        WriteOBJECTIDClient(objId);

    if (fDistSq >= 0.00001f)
    {
        WriteFLOAT(pos.x, 1.0f, 32);
        WriteFLOAT(pos.y, 1.0f, 32);
        WriteFLOAT(pos.z + 0.3f, 1.0f, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CAST_SPELL_LIKE_ABILITY, pMsg,
                                     nSize);
}

int CSWCMessage::SendPlayerToServerInput_CastSpell(unsigned int nSpellId, unsigned char nClassSlot,
                                                   unsigned char a, unsigned char b, int bInstant,
                                                   const Vector& pos, unsigned long long objId)
{
    unsigned char* pMsg;
    unsigned int nSize = 4;

    CreateWriteMessage(4, MSG_PLAYERID_ALL_CLIENTS, 1);

    float fDistSq = dot(pos, pos);
    unsigned char nMask = (objId != OBJECT_INVALID);
    if (fDistSq >= 0.00001f)
        nMask |= 2;
    if (bInstant != 0)
        nMask |= 4;

    WriteDWORD(nSpellId, 32);
    WriteBYTE(nClassSlot, 8);
    WriteBYTE(a, 8);
    WriteBYTE(b, 8);
    WriteBYTE(nMask, 8);

    if (objId != OBJECT_INVALID)
        WriteOBJECTIDClient(objId);

    if (fDistSq >= 0.00001f)
    {
        WriteFLOAT(pos.x, 1.0f, 32);
        WriteFLOAT(pos.y, 1.0f, 32);
        WriteFLOAT(pos.z + 0.3f, 1.0f, 32);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_CAST_SPELL, pMsg, nSize);
}

// Three independent "flag then field" pairs, each spelled as a full if/else that writes the flag
// on both arms: a target object, a property, and a target point.
int CSWCMessage::SendPlayerToServerInput_UseItem(unsigned long long itemId, unsigned char a,
                                                 unsigned char b, unsigned long long objId,
                                                 const Vector& pos)
{
    unsigned char* pMsg;
    unsigned int nSize = 29;

    CreateWriteMessage(29, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(itemId);
    WriteBYTE(a, 8);

    if (static_cast<unsigned char>(~b) != 0)
    {
        WriteBOOL(1);
        WriteBYTE(b, 8);
    }
    else
    {
        WriteBOOL(0);
    }

    if (objId == OBJECT_INVALID)
    {
        WriteBOOL(0);
    }
    else
    {
        WriteBOOL(1);
        WriteOBJECTIDClient(objId);
    }

    Vector vZero = { 0.0f, 0.0f, 0.0f };
    if (pos != vZero)
    {
        WriteBOOL(1);
        WriteFLOAT(pos.x, 1.0f, 32);
        WriteFLOAT(pos.y, 1.0f, 32);
        WriteFLOAT(pos.z, 1.0f, 32);
    }
    else
    {
        WriteBOOL(0);
    }

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    return SendPlayerToServerMessage(MSG_MAJOR_INPUT, MSG_INPUT_USE_ITEM, pMsg, nSize);
}
// The switch picks the sub-opcode only. Case 2 has no arm of its own and falls in with the default,
// returning 0. Case 8 is not part of the family: it discards the payload it just built and sends the
// empty three-byte message instead.
int CSWCMessage::SendPlayerToServerParty_Action(unsigned long long objId, int nAction)
{
    unsigned char* pMsg;
    unsigned int nSize = 8;

    CreateWriteMessage(8, MSG_PLAYERID_ALL_CLIENTS, 1);
    WriteOBJECTIDClient(objId);

    if (GetWriteMessage(&pMsg, &nSize) == 0)
        return 0;

    switch (nAction)
    {
        case 0: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_LEAVE, pMsg, nSize);
        case 1: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_KICK, pMsg, nSize);
        case 3: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_TRANSFER_LEADERSHIP,
                                                 pMsg, nSize);
        case 4: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_INVITE, pMsg, nSize);
        case 5: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_IGNORE_INVITATION,
                                                 pMsg, nSize);
        case 6: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_ACCEPT_INVITATION,
                                                 pMsg, nSize);
        case 7: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_DECLINE_INVITATION,
                                                 pMsg, nSize);
        case 8: return SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_LIST_REQUEST, 0, 0);
    }

    return 0;
}

// Loads the saved character off disk and then calls the raw-buffer overload above. The read buffer
// is deliberately four bytes longer than the file, and the length handed on is the file size, not
// the allocation.
int CSWCMessage::SendPlayerToServerLogin_LoginLocalCharacter(const CResRef& resRef, int bJoin)
{
    CExoString sPath;
    resRef.CopyToString(sPath);

#if KOTOR1  // K1 has no demo build flag
    if (sPath.CompareNoCase("test"))
#else
    if (sPath.CompareNoCase("test") || g_bDemoMode)
#endif
        sPath = CExoString("LOCALVAULT:") + sPath;
    else
        sPath = CExoString("TEMP:") + sPath;

    CExoFile* pFile = new CExoFile(sPath, kResTypeBic, CExoString("rb"));
    if (pFile->FileOpened() == 0)
    {
        delete pFile;
        return 0;
    }

    int nFileSize = pFile->GetSize();
    unsigned char* pData = new unsigned char[static_cast<uint64_t>(nFileSize) + 4];
    pFile->Read(pData, 1, nFileSize);
    delete pFile;

    int nSent = SendPlayerToServerLogin_LoginLocalCharacter(pData, nFileSize, bJoin);
    delete[] pData;

    return nSent;
}

