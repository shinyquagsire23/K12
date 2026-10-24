#pragma once

#include "exostring.h"

// The client half of the sysadmin message channel. An empty messenger with no members, no ctor and
// no dtor; everything it reads and writes lives in the CServerInfo blob owned by the application
// manager.
class CClientMsgSysAdmin {
public:
    // Splits "A.B. C" (or "A.B") into up to three tokens and returns how many it filled, 0 on a
    // malformed input. The message pointer and size describe the payload after the leading type byte.
    int GetTokenPair(unsigned char* pMessage, unsigned int nSize,
                     CExoString& sToken1, CExoString& sToken2, CExoString& sToken3);

    // Builds a 's' + CExoString buffer and ships it to the server through the net layer.
    int SendSysAdminToServerMessage(const CExoString& sMessage);

    // Handles an inbound message. The first byte is the class byte, and only 'S'/'s' is accepted.
    int HandleServerToSysAdminMessage(unsigned char* pMessage, unsigned int nSize);

    // The four "Control.X. <value>" control-command senders.
    int SendSysAdminToServerBootPlayer(unsigned int nPlayerId);
    int SendSysAdminToServerBanPlayer(unsigned int nPlayerId);
    int SendSysAdminToServerChangeSAPassword(const CExoString& sPassword);
    int SendSysAdminToServerChangeGMPassword(const CExoString& sPassword);
};
