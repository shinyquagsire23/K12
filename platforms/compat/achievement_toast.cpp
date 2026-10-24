// Shows a desktop achievement unlock through the game's own GUI; this is the on-screen half of the
// fallback in shim/shim_achievements.cpp. Compiled with the engine's flags (K2P_CONSOLE_SOURCE) so
// it can read CGuiInGame; the shim, which knows nothing of the engine's layouts, calls it once a
// frame with the oldest queued notice.
//
// The notice is the bark bubble a script's BarkString shows when the speaker is no object
// (OBJECT_INVALID: no 3D position, no voice), moved to the bottom-right corner
// (K2HostBarkInCorner, swguibarkbubble.cpp), plus a line in the message log (the Messages screen's
// feedback list), which keeps it after the bubble times out.
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "nwsplayer.h"
#include "guiingame.h"
#include "exostring.h"
#include "exotlktable.h"
#include "gameobjectarray.h"

extern CTlkTable* g_pTlkTable;
void K2HostBarkInCorner();

extern "C" {

// 1 if the notice was shown, 0 if the game is not somewhere one can be (no module, the player's
// creature not there yet, a conversation or a movie): the shim keeps it and asks again next frame.
int k2host_achievement_show(const char* pszBubble, const char* pszLog)
{
    if (!g_pAppManager || !g_pAppManager->m_pClientExoApp || !g_pAppManager->m_pServerExoApp)
        return 0;
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CGuiInGame* pGui = pClient->GetInGameGui();
    if (!pGui || pGui->m_bInitialized == 0 || !pGui->m_pBarkBubble)
        return 0;   // the in-game GUI is not up (main menu, loading)
    if (pGui->m_nGlobalDialogState != 0 || pClient->IsMoviePlaying())
        return 0;   // in a conversation or a cutscene: wait until it is over
    if (!pClient->GetModule())
        return 0;
    // SetBark speaks the text through the talk table as the player's creature.
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSPlayer* pPlayer = pServer->GetClientObjectByObjectId(pServer->GetPlayerCreatureId());
    if (!pPlayer || !pClient->GetGameObject(pClient->ServerToClientObjectId(pPlayer->m_gameObjectId)))
        return 0;

    CResRef noVoice;
    K2HostBarkInCorner();
    pGui->ShowBarkBubble(OBJECT_INVALID, CExoString(pszBubble), noVoice, 0);
    if (pszLog && *pszLog)
        pGui->AppendToMsgBuffer(CExoString(pszLog), 0x80, 0);
    return 1;
}

// The language of the game's dialog.tlk (0 English, 1 French, 2 German, 3 Italian, 4 Spanish), which
// picks the notice's row of the name table; -1 before the talk table is open.
int k2host_achievement_language(void)
{
    return g_pTlkTable ? g_pTlkTable->GetLanguageVersion() : -1;
}

} // extern "C"
