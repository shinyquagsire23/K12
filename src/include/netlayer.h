#pragma once

// The engine's in-process client/server transport. There is no socket or wire I/O: two CNetLayer
// instances in the same process, one in the client app and one in the server app, are cross-paired
// by Initialize's handshake, and SendMessageToPlayer copies straight into the peer object's own ring
// buffer. Protocol messages are "BN"-prefixed byte blobs: a chat and session-control tag, a
// connect-response tag and an outbound connect-request tag.

#include "exostring.h"

// Abstract app interface both CClientExoApp and CServerExoApp implement, with the same ten slots in
// the same order and no virtual destructor.
class CExoString;
class CBaseExoApp {
public:
    virtual void PlayerListChange(unsigned int a, int b, int c);
    virtual void HandleMessage(unsigned int a, unsigned char* b, unsigned int c, int d);
    virtual float GetFPS();
    virtual void ContinueMessageProcessing();
    virtual void* GetNetLayer();
    virtual void ShutDownToMainMenu();
    virtual void* GetConnectionLib();
    virtual CExoString* GetModuleDescription();
    virtual int GetApplicationId();
    virtual void SetApplicationIdsMatch(int a);
};

// One embedded per-peer info record. The player id a caller passes to GetPlayerInfo is never read,
// so despite the multi-player-shaped API this build tracks a single remote peer.
class CNetLayerPlayerInfo {
public:
    CNetLayerPlayerInfo();
    void Initialize();

    int         m_bValid;
    char        _pad04[4];
    CExoString  m_sName;
    char        _pad18[0x10];
    int         m_isPlayerPermitted;
    int         m_isGameMasterPermitted;
    int         m_isAdminPermitted;
    char        _pad34[4];
};

// The transport object. The frame buffer is a flat, double-sized ring buffer: the cursors always
// stay within the first 64 KiB half, but a write that starts near that boundary overruns into the
// second shadow half rather than wrapping mid-copy.
class CNetLayer {
public:
    // The loopback frame ring m_abFrameBuffer holds, and the shadow half a frame may run into.
    enum {
        FRAME_RING_SIZE = 64 * 1024,
    };

    CNetLayer();
    ~CNetLayer();

    bool ShutDown();
    int Initialize(CBaseExoApp* pApp);
    void SetDestNetLayer(CNetLayer* pDest);
    void ProcessReceivedFrames(int bProcess);
    int NonWindowMessages(unsigned int a, unsigned char* data, unsigned int len);
    int  SendMessageToPlayer(unsigned int playerId, unsigned char* data, unsigned int len, unsigned int a4);
    bool EndServerMode();
    bool StartServerMode(const CExoString& name, unsigned int flags);
    bool StartConnectToSession(unsigned int playerId, const CExoString& target, const CExoString& param, unsigned int a4, unsigned int connectType);
    bool EndConnectToSession();
    void SetUpPlayBackConnection();
    bool UpdateStatusLoop(unsigned int application);
    int  ConnectToSessionLoop();
    CNetLayerPlayerInfo* GetPlayerInfo(unsigned int playerId);
    // Always returns true, even on the early length-check reject; HandleBNCRMessage returns false
    // there.
    bool HandleBNCSMessage(unsigned int a, unsigned char* data, unsigned int len);
    int  HandleBNCRMessage(unsigned int a, unsigned char* data, unsigned int len);

    // Real, addressable objects in the data segment, defined with their values in the .cpp so a
    // consumer emits a real load rather than folding the value into the code.
    static const unsigned int PLAYERID_ALL_CLIENTS;        // (unsigned)-1
    static const unsigned int PLAYERID_INVALIDID;          // (unsigned)-2
    static const unsigned int PLAYERID_SERVER;             // (unsigned)-3
    static const unsigned int PLAYERID_ALL_PLAYERS;        // (unsigned)-9
    static const unsigned int PLAYERID_ALL_GAMEMASTERS;    // (unsigned)-10
    static const unsigned int PLAYERID_ALL_SERVERADMINS;   // (unsigned)-11
    static const unsigned int APPLICATION_CLIENT;          // 1 << 22
    static const unsigned int APPLICATION_SERVER;          // 1 << 23
    static const unsigned char CONNECTTYPE_PLAYER;         // 0x10
    static const unsigned char CONNECTTYPE_GAMEMASTER;     // 0x20
    static const unsigned char CONNECTTYPE_SERVERADMIN;    // 0x40
    static const unsigned int INSTANCE_CLIENT;             // 0
    static const unsigned int INSTANCE_SERVER;             // 1
    static const unsigned int CONNECTION_PENDING;          // 2

    unsigned char        m_abFrameBuffer[2 * FRAME_RING_SIZE];
    int                  m_nReadPos;
    int                  m_nWritePos;
    CBaseExoApp*         m_pApp;
    CNetLayer*           m_pDestNetLayer;
    int                  m_bInitialized;
    char                 _pad2001c[4];
    CNetLayerPlayerInfo  m_playerInfo;
    int                  m_bServerModeActive;
    char                 _pad2005c[4];
    CExoString           m_sServerName;
    CExoString           m_sPlayerPassword;
    CExoString           m_sGameMasterPassword;
    CExoString           m_sServerAdminPassword;
    int                  m_bConnecting;
    char                 _pad200a4[4];
    CExoString           m_sConnectTarget;
    int                  m_nConnectType;
    int                  m_bConnectResponseReceived;
    int                  m_nConnectResult;
    int                  m_bAccepted;
};

extern CNetLayer* g_pcTmpNetLayer;
