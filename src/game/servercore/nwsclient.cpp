#include "nwsplayer.h"

CSWSClient::CSWSClient(unsigned int nPlayerId)
    : m_nPlayerId(nPlayerId)
    , m_nLanguage(0)
{
}

// A CSWSClient is never a player. CSWSPlayer overrides this to return itself.
CSWSPlayer* CSWSClient::AsSWSPlayer()
{
    return 0;
}

CSWSClient::~CSWSClient()
{
}
