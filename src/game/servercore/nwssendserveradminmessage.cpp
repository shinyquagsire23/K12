#include "nwssendplayermessage.h"
#include "restype.h"
#include "appmanager.h"
#include "netlayer.h"
#include "serverexoapp.h"
#include "exobase.h"
#include <string.h>

// The server half of the network message codec.


// Sends a message to the player as a 'S'-prefixed copy of the payload.
int CSWSMessage::SendServerToServerAdminMessage(unsigned int nModuleId, const CExoString& message)
{
#if KOTOR1  // K1: CExoString::GetLength is out of line
    unsigned int nLen = message.GetLength() + 1;
#else
    unsigned int nLen = static_cast<unsigned int>(message.m_sString != 0 ? strlen(message.m_sString) + 1 : 1);
#endif
    unsigned char* pMsg = new unsigned char[nLen];
    pMsg[0] = 'S';
#if KOTOR1  // K1: CExoString::GetLength is out of line
    memcpy(pMsg + 1, message.CStr(), message.GetLength());  // idiom-exempt: runtime length (strlen-derived, no compile-time size)
#else
    memcpy(pMsg + 1, message.CStr(), message.m_sString != 0 ? static_cast<int>(strlen(message.m_sString)) : 0);  // idiom-exempt: runtime length (strlen-derived, no compile-time size)
#endif
    int nResult = g_pAppManager->m_pServerExoApp->GetNetLayer()->SendMessageToPlayer(nModuleId, pMsg, nLen, 0);
    delete[] pMsg;
    return nResult;
}

// Broadcasts the module list: the module directory and the extra-files directory share one loop
// counter, so the second pass continues where the first stopped. Each entry's extension is
// stripped and the entry is sent as "\n<name>:<prefix><name>".
void CSWSMessage::SendServerToServerAdminModuleList(unsigned int nModuleId)
{
    CExoString moduleName;
    CExoString moduleList;
    CExoString moduleList2;
    CExoArrayList<CExoString> modules;
#if KOTOR1  // K1: the six-argument GetDirectoryList
    g_pExoBase->GetDirectoryList(&modules, CExoString("MODULES:"), RESTYPE_MOD, 0, 0, 0);
#else
    g_pExoBase->GetDirectoryList(&modules, CExoString("MODULES:"), RESTYPE_MOD, 0, 0);
#endif
    int i;
    for (i = 0; i < modules.m_count; i++) {
        moduleName = modules.m_pArray[i];
#if KOTOR1  // K1: CExoString::GetLength is out of line
        moduleName = moduleName.SubString(0, moduleName.GetLength() - 4);
#else
        moduleName = moduleName.SubString(0, static_cast<int>((moduleName.m_sString != 0 ? strlen(moduleName.m_sString) : 0) - 4));
#endif
        moduleList = moduleList + CExoString("\n");
        moduleList = moduleList + moduleName;
        moduleName = CExoString("MODULES:") + moduleName;
        moduleList = moduleList + CExoString(":");
        moduleList = moduleList + CExoString(moduleName.CStr());
    }
#if KOTOR1  // K1: the six-argument GetDirectoryList
    g_pExoBase->GetDirectoryList(&modules, CExoString("NWMFILES:"), RESTYPE_NWM, 0, 0, 0);
#else
    g_pExoBase->GetDirectoryList(&modules, CExoString("NWMFILES:"), RESTYPE_NWM, 0, 0);
#endif
    for (; i < modules.m_count; i++) {
        moduleName = modules.m_pArray[i];
#if KOTOR1  // K1: CExoString::GetLength is out of line
        moduleName = moduleName.SubString(0, moduleName.GetLength() - 4);
#else
        moduleName = moduleName.SubString(0, static_cast<int>((moduleName.m_sString != 0 ? strlen(moduleName.m_sString) : 0) - 4));
#endif
        moduleList = moduleList + CExoString("\n");
        moduleList = moduleList + moduleName;
        moduleName = CExoString("NWMFILES:") + moduleName;
        moduleList = moduleList + CExoString(":");
        moduleList = moduleList + CExoString(moduleName.CStr());
    }
    // The id in the message body is the entry count, not nModuleId.
    moduleList = CExoString(modules.m_count) + moduleList;
    moduleList = CExoString("ServerStatus.ModuleList ") + moduleList;
    SendServerToServerAdminMessage(nModuleId, CExoString(moduleList.CStr()));
}

// Broadcasts the server status for the current server mode: idle, module loaded or module
// running. Unknown modes send nothing.
void CSWSMessage::SendServerToServerAdminServerStatus(unsigned int nModuleId)
{
    CExoString status;
    CExoString moduleName;
    unsigned short nMode = g_pAppManager->m_pServerExoApp->GetServerMode();
    if (nMode != 0)
    {
        moduleName = g_pAppManager->m_pServerExoApp->GetModuleName();
        if (nMode == 1)
        {
            status.Format(const_cast<char*>("%s.%s %s"), "ServerStatus", "ModuleLoaded", moduleName.CStr());
            SendServerToServerAdminMessage(nModuleId, status);
        }
        else if (nMode == 2)
        {
            status.Format(const_cast<char*>("%s.%s %s"), "ServerStatus", "ModuleRunning", moduleName.CStr());
            SendServerToServerAdminMessage(nModuleId, status);
        }
    }
    else
    {
        status.Format(const_cast<char*>("%s.%s"), "ServerStatus", "Idle");
        SendServerToServerAdminMessage(nModuleId, status);
    }
}

// Broadcasts the server's player list.
void CSWSMessage::SendServerToServerAdminPlayerList(unsigned int nModuleId)
{
    CExoString status;
    status.Format(const_cast<char*>("%s.%s "), "ServerStatus", "PlayerList");
    status = status + g_pAppManager->m_pServerExoApp->GetPlayerListString();
    SendServerToServerAdminMessage(nModuleId, status);
}
