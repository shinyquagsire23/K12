// The client's sys-admin message channel, with the "Control.<verb>. <value>" senders, the
// 's'-framed send hub, and the inbound ServerStatus / Module / option-line handler.
//
// Wire grammar:
//   "Control.<verb>. <value>"             e.g. "Control.Boot. 00000042"
//   "ServerStatus.<state>[. <module>]"    states: Idle, ModuleLoaded, ModuleRunning
//   "ServerStatus.<list>"                 lists: ModuleList, GetBannedList, CharactervaultList,
//                                         PlayerList
//   "Module.SavegameList. <value>"
// The handler's last arm is generic. Any three-token line is looked up in the server-info option
// table, and further newline-separated lines in the same string are parsed the same way, so one
// message can set several options.

#include "k2stdint.h"
#include "clientmsgsysadmin.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "netlayer.h"
#include "serverinfo.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "exostringinline.h"

// CServerInfo::m_nUpdatedFlags: which parts of the server information a sysadmin reply refreshed.
enum {
    SERVER_INFO_UPDATED_STATUS               = 0x001,
    SERVER_INFO_UPDATED_MODULE               = 0x010,
    SERVER_INFO_UPDATED_PLAYER_LIST          = 0x020,
    SERVER_INFO_UPDATED_SAVEGAME_LIST        = 0x080,
    SERVER_INFO_UPDATED_CHARACTER_VAULT_LIST = 0x100,
};

int CClientMsgSysAdmin::GetTokenPair(unsigned char* pMessage, unsigned int nSize,
                                     CExoString& sToken1, CExoString& sToken2,
                                     CExoString& sToken3)
{
    unsigned char nDelim;
#ifdef K2_FIX_ENGINE_BUGS
    nDelim = 0;
#endif
    int nEnd1 = 0;
    if (nSize != 0) {
        do {
            nDelim = pMessage[nEnd1];
            if (nDelim == '.')
                break;
            if (nDelim == ' ') {
                sToken1 = "";
                sToken2 = "";
                sToken3 = "";
                return 0;
            }
            nEnd1++;
        } while (static_cast<unsigned int>(nEnd1) != nSize);
    }

    {
        CExoString sTemp(reinterpret_cast<char*>(pMessage), nEnd1);
        sToken1 = sTemp;
    }

    if (nDelim != '.') {
        sToken2 = "";
        sToken3 = "";
        return nEnd1 != 0;
    }

    int nStart2 = nEnd1 + 1;
    int nEnd2;
    if (static_cast<unsigned int>(nStart2) < nSize) {
        int i = nStart2;
        do {
            nDelim = pMessage[i];
            if (nDelim == ' ')
                break;
            if (nDelim == '.') {
                sToken2 = "";
                sToken3 = "";
                return 0;
            }
            i++;
        } while (static_cast<unsigned int>(i) != nSize);
        nEnd2 = i;
    } else {
        nDelim = '.';
        nEnd2 = nStart2;
    }

    {
        CExoString sTemp(reinterpret_cast<char*>(pMessage + nStart2), nEnd2 - nStart2);
        sToken2 = sTemp;
    }

    if (nDelim != ' ') {
        sToken3 = "";
        return nEnd2 > nStart2 ? 2 : 0;
    }

    int nStart3 = nEnd2 + 1;
    if (static_cast<unsigned int>(nStart3) >= nSize) {
        sToken3 = "";
        return 0;
    }
    {
        CExoString sTemp(reinterpret_cast<char*>(pMessage + nStart3), nSize - nStart3);
        sToken3 = sTemp;
    }
    return 3;
}

// Prefixes the message with an 's' byte and ships it to the server. The buffer holds the payload
// plus that byte and is never freed.
#if KOTOR1  // K1 reaches this through a call, not an inline
__attribute__((noinline))
#endif
int CClientMsgSysAdmin::SendSysAdminToServerMessage(const CExoString& sMessage)
{
    unsigned int nLength = sMessage.GetLength() + 1;

    unsigned char* pBuffer = new unsigned char[nLength];
    pBuffer[0] = 's';
    memcpy(pBuffer + 1, sMessage.CStr(),
           sMessage.GetLength());

    return g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(
        CNetLayer::PLAYERID_SERVER, pBuffer, nLength, 0);
}

// The control senders all format their command, hand it to the send hub and return its result. The
// integer verbs use a zero-padded field.
int CClientMsgSysAdmin::SendSysAdminToServerBootPlayer(unsigned int nPlayerId)
{
    CExoString sMessage;
    sMessage.Format(const_cast<char*>("%s.%s. %08i"), const_cast<char*>("Control"), const_cast<char*>("Boot"), nPlayerId);
    return SendSysAdminToServerMessage(sMessage);
}

int CClientMsgSysAdmin::SendSysAdminToServerBanPlayer(unsigned int nPlayerId)
{
    CExoString sMessage;
    sMessage.Format(const_cast<char*>("%s.%s. %08i"), const_cast<char*>("Control"), const_cast<char*>("Ban"), nPlayerId);
    return SendSysAdminToServerMessage(sMessage);
}

int CClientMsgSysAdmin::SendSysAdminToServerChangeSAPassword(const CExoString& sPassword)
{
    CExoString sMessage;
    sMessage.Format(const_cast<char*>("%s.%s. %s"), const_cast<char*>("Control"), const_cast<char*>("SAPass"), sPassword.CStr());
    return SendSysAdminToServerMessage(sMessage);
}

int CClientMsgSysAdmin::SendSysAdminToServerChangeGMPassword(const CExoString& sPassword)
{
    CExoString sMessage;
    sMessage.Format(const_cast<char*>("%s.%s. %s"), const_cast<char*>("Control"), const_cast<char*>("GMPass"), sPassword.CStr());
    return SendSysAdminToServerMessage(sMessage);
}

// Returns 1 when a branch acted, even for a no-op list query, and 0 when the payload is not a
// sys-admin line.
int CClientMsgSysAdmin::HandleServerToSysAdminMessage(unsigned char* pMessage, unsigned int nSize)
{
    int nLength;
    CExoString sMessage;
    if (pMessage[0] != 'S')
        return 0;

    sMessage = CExoString(reinterpret_cast<const char*>(pMessage + 1), nSize - 1);

    CExoString sToken1;
    CExoString sToken2;
    CExoString sToken3;
    int nTokens = GetTokenPair(pMessage + 1, nSize - 1, sToken1, sToken2, sToken3);

    if (nTokens >= 1 && sToken1 == "ServerStatus") {
        if (nTokens >= 2) {
            if (sToken2 == "Idle") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nServerStatus = 0;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_STATUS;
                return 1;
            }
            if (sToken2 == "ModuleLoaded") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nServerStatus = 1;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_STATUS;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->SetModuleName(sToken3);
                return 1;
            }
            if (sToken2 == "ModuleRunning") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nServerStatus = 2;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_STATUS;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->SetModuleName(sToken3);
                return 1;
            }
            if (sToken2 == "ModuleList") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_sModuleList = sToken3;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_MODULE;
                return 1;
            }
            if (sToken2 == "GetBannedList")
                return 1;
            if (sToken2 == "CharactervaultList") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_sCharacterVaultList = sToken3;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_CHARACTER_VAULT_LIST;
                return 1;
            }
            if (sToken2 == "PlayerList") {
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_sPlayerList = sToken3;
                g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_PLAYER_LIST;
                return 1;
            }
        }
        if (sToken2 == "ModuleLoaded") {
            g_pAppManager->m_pClientExoApp->GetServerInfo()->SetModuleName(sToken3);
            g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_MODULE;
            CExoString sCommand;
            sCommand.Format(const_cast<char*>("%s.%s"), const_cast<char*>("Module"), const_cast<char*>("Run"));
            SendSysAdminToServerMessage(sCommand);
            return 1;
        }
    }
    // Deliberately not an else if: a ServerStatus line whose second token is unknown still gets the
    // savegame-list chance.
    if (nTokens >= 2 && sToken1 == "Module") {
        if (sToken2 == "SavegameList") {
            g_pAppManager->m_pClientExoApp->GetServerInfo()->m_sSavegameList = sToken3;
            g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= SERVER_INFO_UPDATED_SAVEGAME_LIST;
            return 1;
        }
    }

    // The generic arm handles "Control.<option>. <value>" lines, one per newline.
    CServerInfo* pInfo = g_pAppManager->m_pClientExoApp->GetServerInfo();
    if (nTokens != 3)
        return 0;

    int nResult = 0;
    for (;;) {
        int nIndex = g_pAppManager->m_pClientExoApp->GetServerInfo()->FindOptionIndex(sToken1, sToken2);
        if (nIndex == 0)
            return nResult;

        g_pAppManager->m_pClientExoApp->GetServerInfo()->m_nUpdatedFlags |= 2;
        if (pInfo->m_Options[nIndex].m_nType == 2)
            *pInfo->m_Options[nIndex].m_pValue = atoi(sToken3.CStr());
        else if (pInfo->m_Options[nIndex].m_nType == 1)
            *pInfo->m_Options[nIndex].m_pValue = sToken3[0] == '1';

        unsigned int nNext = 1;
        while (sToken3[nNext - 1] != 0 && sToken3[nNext - 1] != '\n')
            nNext++;
        unsigned int nPos = nNext - 1;

        nResult = 1;
        if (sToken3[nPos] != '\n')
            break;
        nLength = sToken3.GetLength();
        if (GetTokenPair(reinterpret_cast<unsigned char*>(sToken3.CStr() + nPos + 1), static_cast<unsigned int>(nLength) - nNext,
                         sToken1, sToken2, sToken3) != 3)
            break;
    }
    return nResult;
}
