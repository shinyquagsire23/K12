#include "k2stdint.h"
#include "netlayer.h"

#include "port/libcstring.h"
#include "port/libcstdlib.h"

CNetLayer* g_pcTmpNetLayer;

const unsigned int  CNetLayer::PLAYERID_ALL_CLIENTS      = static_cast<unsigned int>(-1);
const unsigned int  CNetLayer::PLAYERID_INVALIDID        = static_cast<unsigned int>(-2);
const unsigned int  CNetLayer::PLAYERID_SERVER           = static_cast<unsigned int>(-3);
const unsigned int  CNetLayer::PLAYERID_ALL_PLAYERS      = static_cast<unsigned int>(-9);
const unsigned int  CNetLayer::PLAYERID_ALL_GAMEMASTERS  = static_cast<unsigned int>(-10);
const unsigned int  CNetLayer::PLAYERID_ALL_SERVERADMINS = static_cast<unsigned int>(-11);
// The loopback handshake's two message tags, read as little-endian words.
enum {
    FOURCC_BNCS = 0x53434e42,  // "BNCS"
    FOURCC_BNCR = 0x52434e42,  // "BNCR"
};

const unsigned int  CNetLayer::APPLICATION_CLIENT        = 0x00400000;
const unsigned int  CNetLayer::APPLICATION_SERVER        = 0x00800000;
const unsigned char CNetLayer::CONNECTTYPE_PLAYER        = 0x10;
const unsigned char CNetLayer::CONNECTTYPE_GAMEMASTER    = 0x20;
const unsigned char CNetLayer::CONNECTTYPE_SERVERADMIN   = 0x40;
const unsigned int  CNetLayer::INSTANCE_CLIENT           = 0;
const unsigned int  CNetLayer::INSTANCE_SERVER           = 1;
const unsigned int  CNetLayer::CONNECTION_PENDING        = 2;

CNetLayerPlayerInfo::CNetLayerPlayerInfo()
{
    m_bValid = 0;
    m_sName = "";
    m_isPlayerPermitted = 0;
    m_isGameMasterPermitted = 0;
    m_isAdminPermitted = 0;
}

void CNetLayerPlayerInfo::Initialize()
{
    m_bValid = 0;
    m_sName = "";
    m_isPlayerPermitted = 0;
    m_isGameMasterPermitted = 0;
    m_isAdminPermitted = 0;
}

CNetLayer::CNetLayer()
{
    m_pDestNetLayer = 0;
    m_bInitialized = 0;
}

CNetLayer::~CNetLayer()
{
    if (m_bInitialized == 1)
    {
        if (m_bServerModeActive == 1) m_bServerModeActive = 0;
        CNetLayer* pPeer = m_pDestNetLayer;
        if (pPeer) pPeer->m_pDestNetLayer = 0;
        if (g_pcTmpNetLayer == this) g_pcTmpNetLayer = m_pDestNetLayer;
        m_bInitialized = 0;
    }
}

bool CNetLayer::ShutDown()
{
    if (!m_bInitialized) return false;
    if (m_bServerModeActive == 1) m_bServerModeActive = 0;
    CNetLayer* pPeer = m_pDestNetLayer;
    if (pPeer) pPeer->m_pDestNetLayer = 0;
    if (g_pcTmpNetLayer == this) g_pcTmpNetLayer = m_pDestNetLayer;
    m_bInitialized = 0;
    return true;
}

// Pairs this layer with whichever peer is waiting in g_pcTmpNetLayer: if one is pending, the two
// point at each other, otherwise this layer becomes the pending one. Returns whether the init
// actually happened.
int CNetLayer::Initialize(CBaseExoApp* pApp)
{
    CNetLayer** ppTarget = &g_pcTmpNetLayer;
    CNetLayer* pPending = *ppTarget;
    if (pPending != 0) {
        m_pDestNetLayer = pPending;
        ppTarget = &pPending->m_pDestNetLayer;
    }
    *ppTarget = this;
    m_nReadPos = 0;
    m_nWritePos = 0;
    int bDidInit = 0;
    if (m_bInitialized != 1)
    {
        m_pApp = pApp;
        m_bServerModeActive = 0;
        m_playerInfo.m_bValid = 0;
        bDidInit = 1;
        m_bInitialized = bDidInit;
        m_playerInfo.m_sName = "";
        m_playerInfo.m_isPlayerPermitted = 0;
        m_playerInfo.m_isGameMasterPermitted = 0;
        m_playerInfo.m_isAdminPermitted = 0;
        m_bConnecting = 0;
        m_nConnectType = 0;
        m_bConnectResponseReceived = 0;
        m_nConnectResult = 0;
        m_bAccepted = 0;
    }
    return bDidInit;
}

void CNetLayer::SetDestNetLayer(CNetLayer* pDest)
{
    m_pDestNetLayer = pDest;
}

bool CNetLayer::EndServerMode()
{
    if (m_bServerModeActive)
    {
        m_bServerModeActive = 0;
        return true;
    }
    return false;
}

// The flags parameter is unused.
bool CNetLayer::StartServerMode(const CExoString& name, unsigned int flags)
{
    K2_UNUSED(flags);
    if (m_bServerModeActive == 1) return false;
    m_bServerModeActive = 1;
    m_sServerName = name;
    m_sPlayerPassword = "";
    m_sGameMasterPassword = "";
    m_sServerAdminPassword = "";
    return true;
}

// Builds and sends the outbound connect request: a 4-byte tag, the requested connect type, the
// target-string length and the raw target bytes. `target` supplies the message payload, while
// `param` is stored into m_sConnectTarget.
bool CNetLayer::StartConnectToSession(unsigned int playerId, const CExoString& target, const CExoString& param, unsigned int a4, unsigned int connectType)
{
    if (m_bConnecting == 1) return false;
    m_bConnecting = 0;

#if KOTOR1  // K1: CExoString::GetLength is out of line
    unsigned int msgLen = target.GetLength() + 9;
#else
    unsigned int msgLen = target.m_sString ? static_cast<unsigned int>(strlen(target.m_sString)) + 9 : 9;
#endif
    K2_ASSUME(msgLen >= 9);   // the 9-byte header plus the target length, which is far below 4 GB
    unsigned char* msg = new unsigned char[msgLen];
    msg[6] = 0;
    *reinterpret_cast<unsigned int*>(msg) = FOURCC_BNCS;
    msg[4] = 'U';
    msg[5] = 0;
    msg[7] = static_cast<unsigned char>(connectType);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    msg[8] = static_cast<unsigned char>(target.GetLength());
    memcpy(msg + 9, target.CStr(), static_cast<unsigned int>(target.GetLength()));  // idiom-exempt: runtime length (strlen-derived, no compile-time size)
#else
    msg[8] = target.m_sString ? static_cast<unsigned char>(strlen(target.m_sString)) : 0;
    memcpy(msg + 9, target.CStr(), target.m_sString ? static_cast<unsigned int>(strlen(target.m_sString)) : 0);  // idiom-exempt: runtime length (strlen-derived, no compile-time size)
#endif

    SendMessageToPlayer(playerId, msg, msgLen, a4);

    m_bConnecting = 1;
    m_sConnectTarget = param;
    m_nConnectType = connectType;
    m_bConnectResponseReceived = 0;
    m_nConnectResult = 0;
    m_bAccepted = 0;

    delete[] msg;
    return true;
}

bool CNetLayer::EndConnectToSession()
{
    if (m_bConnecting)
    {
        m_bConnecting = 0;
        m_bConnectResponseReceived = 0;
        m_nConnectResult = 0;
        m_bAccepted = 0;
        return true;
    }
    return false;
}

void CNetLayer::SetUpPlayBackConnection()
{
}

bool CNetLayer::UpdateStatusLoop(unsigned int application)
{
    if (application == APPLICATION_CLIENT &&
        m_bConnecting == 1 &&
        m_bConnectResponseReceived == 1 &&
        m_nConnectResult != 2)
    {
        m_bConnecting = 0;
        m_bConnectResponseReceived = 0;
        m_nConnectResult = 0;
        m_bAccepted = 0;
    }
    return true;
}

int CNetLayer::ConnectToSessionLoop()
{
    if (!m_bConnecting) return 0;
    if (m_bConnectResponseReceived == 1) return m_nConnectResult;
    return CONNECTION_PENDING;
}

CNetLayerPlayerInfo* CNetLayer::GetPlayerInfo(unsigned int playerId)
{
    K2_UNUSED(playerId);
    return &m_playerInfo;
}

// Server-side handler for an inbound connect request, the peer of StartConnectToSession.
// Always returns true, including on the early length-check reject, unlike HandleBNCRMessage.
bool CNetLayer::HandleBNCSMessage(unsigned int a, unsigned char* data, unsigned int len)
{
    CExoString name;
    CExoString unused;  // sic: default-constructed/destructed, never otherwise touched

    if (len < 9 || static_cast<unsigned int>(data[8] + 9) != len) goto Cleanup;

    {
        unsigned char connectType = data[7];
        {
            CExoString sender(reinterpret_cast<const char*>(data + 9), data[8]);
            name = sender;
        }

        unsigned char* reply = new unsigned char[10];
        *reinterpret_cast<unsigned int*>(reply) = FOURCC_BNCR;
        m_playerInfo.m_sName = name;
        m_playerInfo.m_bValid = 1;

        bool granted;
        if (connectType == CONNECTTYPE_SERVERADMIN)
        {
            if (m_sServerAdminPassword == "")
            {
                m_playerInfo.m_isAdminPermitted = 1;
                m_playerInfo.m_isPlayerPermitted = 0;
                m_playerInfo.m_isGameMasterPermitted = 0;
            }
            reply[4] = 'A';
            goto RecheckServerAdmin;
        }
        else if (connectType == CONNECTTYPE_GAMEMASTER)
        {
            if (m_sGameMasterPassword == "")
            {
                m_playerInfo.m_isAdminPermitted = 0;
                m_playerInfo.m_isPlayerPermitted = 0;
                m_playerInfo.m_isGameMasterPermitted = 1;
            }
            reply[4] = 'A';
            granted = (m_sGameMasterPassword == "");
            goto CheckGranted;
        }
        else
        {
            if (m_sPlayerPassword == "")
            {
                m_playerInfo.m_isAdminPermitted = 1;
                m_playerInfo.m_isPlayerPermitted = 1;
                m_playerInfo.m_isGameMasterPermitted = 0;
            }
            reply[4] = 'A';
            if (connectType == CONNECTTYPE_SERVERADMIN) goto RecheckServerAdmin;
            if (connectType != CONNECTTYPE_PLAYER) goto ReplyNotGranted;
            granted = (m_sPlayerPassword == "");
            goto CheckGranted;
        }

    RecheckServerAdmin:
        granted = (m_sServerAdminPassword == "");
        goto CheckGranted;

    CheckGranted:
        if (!granted) goto ReplyNotGranted;
        reply[5] = 'C';
        m_pApp->PlayerListChange(0, 1, 0);
        goto SendReply;

    ReplyNotGranted:
        reply[5] = 'P';

    SendReply:
        reply[6] = connectType;
        reply[7] = 'U';
        reply[8] = 0;
        reply[9] = 0;
        SendMessageToPlayer(a, reply, 10, 0);
        delete[] reply;
    }

Cleanup:
    return true;
}

// sic: an "A" that is not followed by "C" is silently a no-op: it writes nothing and returns 1.
// Only the "AC" accept path and the not-an-A reject path touch state.
int CNetLayer::HandleBNCRMessage(unsigned int a, unsigned char* data, unsigned int len)
{
    K2_UNUSED(a);
    if (len != 10) return 0;
    if (data[4] == 'A')
    {
        if (data[5] == 'C')
        {
            m_bConnectResponseReceived = 1;
            m_nConnectResult = 1;
            m_bAccepted = 1;
        }
        return 1;
    }
    m_bAccepted = 0;
    return 1;
}

void CNetLayer::ProcessReceivedFrames(int bProcess)
{
    if (!bProcess) return;
    if (m_nReadPos == m_nWritePos) return;
    do
    {
        unsigned int offset = static_cast<unsigned int>(m_nReadPos);
        if (offset >= FRAME_RING_SIZE)
        {
            offset = 0;
            m_nReadPos = 0;
        }
        unsigned int len = *reinterpret_cast<unsigned int*>(m_abFrameBuffer + offset);  // raw-offset: network frame buffer
        unsigned int payloadOff = offset + 4;
        m_nReadPos = payloadOff;
        unsigned char* payload = m_abFrameBuffer + payloadOff;
        unsigned int nextPos = ((len + payloadOff) - (len & 3)) + 4;
        if (payload[0] == 'B' && m_abFrameBuffer[offset + 5] == 'N')
        {
            NonWindowMessages(0, payload, len);
        }
        else
        {
            m_pApp->HandleMessage(0, payload, len, 0);
        }
        m_nReadPos = nextPos;
    } while (m_nReadPos != m_nWritePos);
}

int CNetLayer::NonWindowMessages(unsigned int a, unsigned char* data, unsigned int len)
{
    if (data[0] == 'B' && data[1] == 'N')
    {
        if (m_bServerModeActive == 1 && data[2] == 'C' && data[3] == 'S' && len > 6)
        {
            HandleBNCSMessage(a, data, len);
        }
        if (m_bConnecting == 1 && data[2] == 'C' && data[3] == 'R')
            HandleBNCRMessage(a, data, len);
    }
    else
    {
        m_pApp->HandleMessage(a, data, len, 1);
    }
    return 0;
}

// The character the overrun preview shows for an unprintable byte.
enum {
    PREVIEW_UNPRINTABLE = '_',
};

// sic: when the write would overrun the peer's read cursor by more than 64 KiB, a full hex and
// character preview of the block is formatted through CExoString::Format and then destroyed without
// ever being printed or used. The write to the peer's ring buffer proceeds regardless, and is
// allowed to physically overrun into the shadow half.
int CNetLayer::SendMessageToPlayer(unsigned int playerId, unsigned char* data, unsigned int len, unsigned int a4)
{
    K2_UNUSED(playerId); K2_UNUSED(a4);
    CNetLayer* peer = m_pDestNetLayer;
    if (!peer) return 0;

    unsigned int writePos = (peer->m_nWritePos >> 16 == 0) ? peer->m_nWritePos : 0;
    unsigned int nextPos = (len - (len & 3)) + writePos + 4 + 4;

    if (static_cast<unsigned int>(peer->m_nReadPos) + FRAME_RING_SIZE < nextPos)
    {
        CExoString msg;
        unsigned int c0, c1, c2, c3, c4, c5, c6, c7;
        unsigned int r0, r1, r2, r3, r4, r5, r6, r7;

        if (len == 0)
        {
            r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;
            c0 = c1 = c2 = c3 = c4 = c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
        }
        else
        {
            r0 = data[0];
            c0 = isgraph(r0) ? r0 : PREVIEW_UNPRINTABLE;
            if (len == 1)
            {
                r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;
                c1 = c2 = c3 = c4 = c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
            }
            else
            {
                r1 = data[1];
                c1 = isgraph(r1) ? r1 : PREVIEW_UNPRINTABLE;
                if (len < 3)
                {
                    r2 = r3 = r4 = r5 = r6 = r7 = 0;
                    c2 = c3 = c4 = c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
                }
                else
                {
                    r2 = data[2];
                    c2 = isgraph(r2) ? r2 : PREVIEW_UNPRINTABLE;
                    if (len == 3)
                    {
                        r3 = r4 = r5 = r6 = r7 = 0;
                        c3 = c4 = c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
                    }
                    else
                    {
                        r3 = data[3];
                        c3 = isgraph(r3) ? r3 : PREVIEW_UNPRINTABLE;
                        if (len < 5)
                        {
                            r4 = r5 = r6 = r7 = 0;
                            c4 = c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
                        }
                        else
                        {
                            r4 = data[4];
                            c4 = isgraph(r4) ? r4 : PREVIEW_UNPRINTABLE;
                            if (len == 5)
                            {
                                r5 = r6 = r7 = 0;
                                c5 = c6 = c7 = PREVIEW_UNPRINTABLE;
                            }
                            else
                            {
                                r5 = data[5];
                                c5 = isgraph(r5) ? r5 : PREVIEW_UNPRINTABLE;
                                if (len < 7)
                                {
                                    r6 = r7 = 0;
                                    c6 = c7 = PREVIEW_UNPRINTABLE;
                                }
                                else
                                {
                                    r6 = data[6];
                                    c6 = isgraph(r6) ? r6 : PREVIEW_UNPRINTABLE;
                                    if (len == 7)
                                    {
                                        r7 = 0;
                                        c7 = PREVIEW_UNPRINTABLE;
                                    }
                                    else
                                    {
                                        r7 = data[7];
                                        c7 = isgraph(r7) ? r7 : PREVIEW_UNPRINTABLE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        msg.Format(
            const_cast<char*>("CNetLayer::SendMessageToPlayer - Message Buffer Overflow! Message Header Follows:\n"
            "Size = %d! <%c %x, %c %x, %c %x, %c %x, %c %x, %c %x, %c %x, %c %x>\n"),
            len, c0, r0, c1, r1, c2, r2, c3, r3, c4, r4, c5, r5, c6, r6, c7, r7);
    }

    *reinterpret_cast<unsigned int*>(m_pDestNetLayer->m_abFrameBuffer + writePos) = len;  // raw-offset: network frame buffer
    memcpy(m_pDestNetLayer->m_abFrameBuffer + writePos + 4, data, len);  // idiom-exempt: runtime length (caller-supplied message size, no compile-time size)
    m_pDestNetLayer->m_nWritePos = nextPos;
    return 1;
}
