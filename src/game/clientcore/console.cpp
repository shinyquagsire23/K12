// The typed-console command table holds one free function per command, each a thin wrapper over an
// engine entry point, plus the static registration objects that publish them.

#include "k2stdint.h"
#include "restype.h"
#include "console.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "exoresman.h"
#include "exoiniinternal.h"
#include "exostring.h"
#include "serverexoapp.h"
#include "nwchandlemessage.h"
#include "partytables.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwsplayer.h"
#include "nwclass.h"
#include "exostringlist.h"
#include "twodimarray.h"
#include "worldtimer.h"
#include "nwcmodule.h"
#include "aurcamera.h"
#include "AURORA/aurbehavior.h"
#include "include/aurbehavior.h"
#include "nwcarea.h"
#include "swminigame.h"
#include "guiingame.h"
#include "aurfont.h"
#include "exoinput.h"
#include "exosound.h"
#include "exobase.h"
#include "exodebug.h"
#include "nwscriptconst.h"

int g_bProcessModuleMemUsage;
char* g_szProcessModuleMemName;

extern CExoSound* g_pExoSound;
int g_bEnableMovieNotification;
int g_bEnableLiveContentIgnore;

#include "port/libcbase.h"
#include "port/libcstring.h"

// Reads the EnableCheats ini key; a nonzero value also flags the save as cheated.
int GetCheatsTurnedOn() {
    CExoIni ini;
    CExoString sValue;
    if (g_pAppManager->m_pServerExoApp) {
        if (ini.ReadIniEntry(sValue, CExoString(EXO_GAME_INI), CExoString("Game Options"),
                             CExoString("EnableCheats"))) {
#if KOTOR1
            int nValue = sValue.AsINT();
            if (nValue) {
                g_pAppManager->m_pServerExoApp->GetPartyTable()->m_cheatUsedFlags = 1;
                return nValue == 1;
            }
#else
            if (sValue.m_sString) {
                int nValue = atoi(sValue.m_sString);
                if (nValue) {
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->m_cheatUsedFlags = 1;
                    return nValue == 1;
                }
            }
#endif
        }
    }
    return 0;
}

void heal() {
    if (GetCheatsTurnedOn()) {
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerCheat_Resurrect();
    }
}

// Shared body of the set<skill> commands; the rank is clamped to [0, 99].
void CheatSetSkill(int nSkill, char* sArgs) {
    int nRank = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nRank));
        if (g_pAppManager->m_pServerExoApp->GetPartyTable()) {
            if (nRank < 0) {
                nRank = 0;
            } else if (nRank >= 100) {
                nRank = 99;
            }
            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            if (pCreature) {
                pCreature->m_pStats->SetSkillRank(static_cast<unsigned char>(nSkill), static_cast<char>(nRank));
            }
        }
    }
}

void setcomputeruse(char* sArgs) { CheatSetSkill(0, sArgs); }
void setdemolitions(char* sArgs) { CheatSetSkill(1, sArgs); }
void setstealth(char* sArgs) { CheatSetSkill(2, sArgs); }
void setawareness(char* sArgs) { CheatSetSkill(3, sArgs); }
void setpersuade(char* sArgs) { CheatSetSkill(4, sArgs); }
void setrepair(char* sArgs) { CheatSetSkill(5, sArgs); }
void setsecurity(char* sArgs) { CheatSetSkill(6, sArgs); }
void settreatinjury(char* sArgs) { CheatSetSkill(7, sArgs); }

// Shared body of the set<ability> commands; the score is clamped to [0, 99].
void CheatSetAbility(int nAbility, char* sArgs) {
    int nScore = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nScore));
        if (g_pAppManager->m_pServerExoApp->GetPartyTable()) {
            if (nScore < 0) {
                nScore = 0;
            } else if (nScore >= 100) {
                nScore = 99;
            }
            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            if (pCreature && pCreature->m_pStats) {
                CSWSCreatureStats* pStats = pCreature->m_pStats;
                switch (nAbility) {
                case ABILITY_STRENGTH: pStats->SetSTRBase(static_cast<unsigned char>(nScore)); break;
                case ABILITY_DEXTERITY: pStats->SetDEXBase(static_cast<unsigned char>(nScore)); break;
                case ABILITY_CONSTITUTION: pStats->SetCONBase(static_cast<unsigned char>(nScore), 1); break;
                case ABILITY_INTELLIGENCE: pStats->SetINTBase(static_cast<unsigned char>(nScore)); break;
                case ABILITY_WISDOM: pStats->SetWISBase(static_cast<unsigned char>(nScore)); break;
                case ABILITY_CHARISMA: pStats->SetCHABase(static_cast<unsigned char>(nScore)); break;
                }
            }
        }
    }
}

void setstrength(char* sArgs) { CheatSetAbility(ABILITY_STRENGTH, sArgs); }
void setdexterity(char* sArgs) { CheatSetAbility(ABILITY_DEXTERITY, sArgs); }
void setconstitution(char* sArgs) { CheatSetAbility(ABILITY_CONSTITUTION, sArgs); }
void setintelligence(char* sArgs) { CheatSetAbility(ABILITY_INTELLIGENCE, sArgs); }
void setwisdom(char* sArgs) { CheatSetAbility(ABILITY_WISDOM, sArgs); }
void setcharisma(char* sArgs) { CheatSetAbility(ABILITY_CHARISMA, sArgs); }

void addexp(char* sArgs) {
    int nXP = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nXP));
        if (nXP >= 1) {
            CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
            if (pParty) {
                pParty->DistributeExperience(nXP, 1);
            }
        }
    }
}

extern int g_bShowFogOfWar;

void turbo() {
    if (GetCheatsTurnedOn()) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bTurboSpeed = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTurboSpeed;
    }
}

void invulnerability() {
    if (GetCheatsTurnedOn()) {
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerCheat_Invulnerability();
    }
}

void restartminigame() {
    if (GetCheatsTurnedOn()) {
        if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pMiniGame) {
            g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->RestartMiniGame();
        }
    }
}

int GetMiniGamePlayingType() {
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule && pModule->m_pArea && pModule->m_pArea->m_pMiniGame) {
        return pModule->m_pArea->m_pMiniGame->m_type + 1;
    }
    return 0;
}

void bright() {
    if (GetCheatsTurnedOn()) {
        if (g_pAppManager && g_pAppManager->m_pClientExoApp) {
            CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
            if (pModule && pModule->m_pArea) {
                pModule->m_pArea->ForceBrightSunAndMoon();
            }
        }
    }
}

void addlightside(int nAmount) {
    if (GetCheatsTurnedOn()) {
        if (g_pAppManager->m_pServerExoApp->GetPartyTable()) {
            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            if (pCreature) {
                CSWSCreatureStats* pStats = pCreature->m_pStats;
                int nValue = nAmount;
                if (nValue > 100) {
                    nValue = 100;
                }
                if (nValue < -100) {
                    nValue = -100;
                }
                pStats->ModifyAlignment(static_cast<short>(nValue), 0);
            }
        }
    }
}

void adddarkside(int nAmount) {
    if (GetCheatsTurnedOn()) {
        if (g_pAppManager->m_pServerExoApp->GetPartyTable()) {
            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            if (pCreature) {
                CSWSCreatureStats* pStats = pCreature->m_pStats;
                int nValue = -nAmount;
                if (nValue < -100) {
                    nValue = -100;
                }
                if (nValue > 100) {
                    nValue = 100;
                }
                pStats->ModifyAlignment(static_cast<short>(nValue), 0);
            }
        }
    }
}

void revealmap() {
    if (GetCheatsTurnedOn()) {
        g_bShowFogOfWar = !g_bShowFogOfWar;
    }
}


// Anything below 99 still pays out 99; this is a floor rather than a cap.
void givecredits(char* sArgs) {
    int nCredits = 0;
    static_cast<void>(sscanf(sArgs, "%d", &nCredits));
    if (GetCheatsTurnedOn()) {
        if (nCredits >= 1) {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            CSWSCreature* pCreature =
                pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());
            if (pCreature) {
                pCreature->SetGold(pCreature->GetGold() +
                                   (nCredits < 99 ? 99 : nCredits));
            }
        }
    }
}

// `giveitem <resref>`: the first space terminates the resref so a trailing count is ignored.
void giveitem(char* sArgs) {
    if (GetCheatsTurnedOn()) {
        char* pSpace = strchr(sArgs, ' ');
        if (pSpace) {
            *pSpace = '\0';
        }
        char szItem[256];
        __strcpy_chk(szItem, sArgs, sizeof(szItem));
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        pClient->CreatureAcquireItem(pClient->GetPlayerCreatureId(), CResRef(szItem));
    }
}

void givemed(char* sArgs) {
    int nCount = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nCount));
        if (nCount == 0) {
            nCount = 99;
        } else if (nCount >= 100) {
            nCount = 99;
        } else if (nCount <= 0) {
            return;
        }
        for (int i = 0; i < nCount; i++) {
            g_pAppManager->m_pClientExoApp->CreatureAcquireItem(
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId(),
                CResRef("g_i_medeqpmnt02"));
        }
    }
}

void giverepair(char* sArgs) {
    int nCount = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nCount));
        if (nCount == 0) {
            nCount = 99;
        } else if (nCount >= 100) {
            nCount = 99;
        } else if (nCount <= 0) {
            return;
        }
        for (int i = 0; i < nCount; i++) {
            g_pAppManager->m_pClientExoApp->CreatureAcquireItem(
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId(),
                CResRef("g_i_drdrepeqp002"));
        }
    }
}

void givecomspikes(char* sArgs) {
    int nCount = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nCount));
        if (nCount == 0) {
            nCount = 99;
        } else if (nCount >= 100) {
            nCount = 99;
        } else if (nCount <= 0) {
            return;
        }
        for (int i = 0; i < nCount; i++) {
            g_pAppManager->m_pClientExoApp->CreatureAcquireItem(
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId(),
                CResRef("g_i_progspike01"));
        }
    }
}

void givesecspikes(char* sArgs) {
    int nCount = 0;
    if (GetCheatsTurnedOn()) {
        static_cast<void>(sscanf(sArgs, "%d", &nCount));
        if (nCount == 0) {
            nCount = 99;
        } else if (nCount >= 100) {
            nCount = 99;
        } else if (nCount <= 0) {
            return;
        }
        for (int i = 0; i < nCount; i++) {
            g_pAppManager->m_pClientExoApp->CreatureAcquireItem(
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId(),
                CResRef("g_i_secspike01"));
        }
    }
}

void givesitharmour() {
    if (GetCheatsTurnedOn()) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        pClient->CreatureAcquireItem(pClient->GetPlayerCreatureId(), CResRef("ptar_sitharmor"));
    }
}

// `warp <module>`: the trailing argument (if any) is stripped so only the resref survives.
void warp(char* sArgs) {
    if (GetCheatsTurnedOn()) {
        char* pSpace = strchr(sArgs, ' ');
        if (pSpace) {
            *pSpace = '\0';
        }
        char szModule[256];
        __strcpy_chk(szModule, sArgs, sizeof(szModule));
        g_pAppManager->m_pServerExoApp->SetMoveToModuleString(CExoString(szModule));
        g_pAppManager->m_pServerExoApp->SetMoveToModulePending(1);
    }
}

void infiniteuses() {
    if (GetCheatsTurnedOn()) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bUnlimitedItemUses =
            !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bUnlimitedItemUses;
    }
}

void whereami() {
    if (GetCheatsTurnedOn()) {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pCreature =
            pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());
        if (pCreature) {
            CExoString sMsg;
            if (g_pAppManager->m_pClientExoApp->GetModule()) {
                sMsg.Format(const_cast<char*>("Module: %s"),
                            g_pAppManager->m_pClientExoApp->GetModule()->m_sModuleName.CStr());
                AurPostString(sMsg.CStr(), 3, 3, 10.0f);
            }
            sMsg.Format(const_cast<char*>("Position: (%3.2f, %3.2f, %3.2f)"), pCreature->m_position.x,
                        pCreature->m_position.y, pCreature->m_position.z);
            AurPostString(sMsg.CStr(), 3, 4, 10.0f);
        }
    }
}

// Grants the experience for one level unless the player is already at the cap: 15 for a
// non-Force-user, 20 otherwise.
void addlevel() {
    if (GetCheatsTurnedOn()) {
        CSWSPlayer* pPlayer =
            static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1));
        if (pPlayer) {
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
            if (pCreature) {
                int nJedi = 0;
                for (int i = 0; i < pCreature->m_pStats->m_numClasses; i++) {
                    if (CSWClass::IsJedi(pCreature->m_pStats->GetClass(static_cast<unsigned char>(i))))
                        nJedi = 1;
                }
                if (nJedi || pCreature->m_pStats->GetLevel(0) != 15) {
                    if (pCreature->m_pStats->GetLevel(0) != 20) {
                        g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(
                            pCreature->m_pStats->GetExpNeededForLevelUp(), 1);
                    }
                }
            }
        }
    }
}

void dancedancemalak() {
    if (GetCheatsTurnedOn()) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bDanceDanceMalak = 1;
    }
}

// Arms the camera-follow behaviour's two clip gates and answers whether the camera is currently
// locked. '0' locks, '1' unlocks, and a null argument only reads; the three query sites in the
// module class all pass null.
int lockcamera(char* sArgs) {
    static bool bLockcamera;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule && pModule->m_pCamera) {
        CAurBehavior* pBehavior = pModule->m_pCamera->GetBehavior(-1);
        if (pBehavior) {
            CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
            if (sArgs && pFollow) {
                switch (sArgs[0]) {
                case '0':
                    bLockcamera = true;
                    pFollow->m_fDistClipGate = 0.0f;
                    pFollow->m_fPitchClipGate = 0.0f;
                    break;
                case '1':
                    bLockcamera = false;
                    pFollow->m_fDistClipGate = 1.0f;
                    pFollow->m_fPitchClipGate = 1.0f;
                    break;
                }
            }
        }
    }
    return !bLockcamera;
}

int keyboardmode(char* sArgs) {
    if (sArgs) {
        switch (sArgs[0]) {
        case '0':
            g_pExoInput->KeyboardModeOff();
            break;
        case '1':
            g_pExoInput->KeyboardModeOn();
            break;
        }
    }
    return g_pExoInput->GetKeyboardMode() != 0;
}

int memtotalallocated(char* sArgs) {
    K2_UNUSED(sArgs);
    return g_pExoBase->m_pDebug->GetCurrentAllocatedMemory();
}

// Plays a movie, pausing all sound for its duration.
char* PlayMovie(char* sArgs) {
    if (sArgs[0]) {
        g_pExoInput->ClearEvents();
        g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
        int nResult;
        g_pAppManager->m_pClientExoApp->PlayMovie(CExoString(sArgs), nResult, 0, 0, 0);
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
        return const_cast<char*>("Playing movie...");
    }
    return const_cast<char*>("usage: PlayMovie sMovieName");
}

void ToggleMovieNotification() {
    g_bEnableMovieNotification = !g_bEnableMovieNotification;
}

void ToggleLiveContentIgnore() {
    g_bEnableLiveContentIgnore = !g_bEnableLiveContentIgnore;
}

extern int g_bEnableScreenCapture;

// The frame rate the client reports.
float g_fFPS = 1.0f;
// The averaging window's length in seconds.
static float fFPSWindowTime = 2.0f;

// Bounded sprintf.
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

// Turns the frame's elapsed time into a rate on g_fFPS and keeps a rolling two-second window whose
// mean frame time becomes the bracketed figure. The parenthesised values are low-water marks.
char* fps(char* sArgs) {
    K2_UNUSED(sArgs);
    static char szBuffer[64];
    static float fLastFPS;
    static float fPrevFPS;
    static float fFrameTimeSum;
    static float fAvgFrameTime;
    static float fFrameCount;

    float fFPS = 1000000.0f / static_cast<float>(static_cast<int>(g_pAppManager->m_pClientExoApp
                                    ->GetWorldTimer()->GetSnapshotTimeDifference()));
    float fFrameTime = 1.0f / fFPS;
    g_fFPS = fFPS;
    fFPSWindowTime = fFPSWindowTime + fFrameTime;
    if (fFPSWindowTime > 2.0f) {
        fPrevFPS = fLastFPS;
        fLastFPS = fFPS;
        fAvgFrameTime = fFrameTimeSum / fFrameCount;
        fFrameTimeSum = 0.0f;
        fFPSWindowTime = fFPSWindowTime - 2.0f;
        fFrameCount = 0.0f;
    }
    if (fFPSWindowTime < -2.0f)
        fFPSWindowTime = -2.0f;
    fFrameTimeSum = fFrameTimeSum + fFrameTime;
    fFrameCount = fFrameCount + 1.0f;
    float fLowFPS = fLastFPS < fFPS ? fLastFPS : fFPS;
    fLastFPS = fLowFPS;
    float fLowestFPS = fLowFPS < fPrevFPS ? fLowFPS : fPrevFPS;
    sprintf_s(szBuffer, sizeof(szBuffer), "%04.1f (%04.1f) [%04.1f] %01.5f", static_cast<double>(fFPS),
              static_cast<double>(fLowestFPS), static_cast<double>(1.0f / fAvgFrameTime), static_cast<double>(fFrameTime));
    return szBuffer;
}

// Converts 2DA resources to binary form under logs/. With no argument it converts every 2DA the
// resource manager knows; an already binary file is skipped rather than failed.
char* Binarize2DA(char* sArgs) {
    static char szBuffer[64];
    CExoStringList* pList;
    if (sArgs && sArgs[0]) {
        CExoString* pName = new CExoString(sArgs);
        CExoStringList* pNew = new CExoStringList();
        pNew->Add(pName);
        pList = pNew;
    } else {
        pList = g_pExoResMan->GetResOfType(RESTYPE_2DA, 0);
    }

    int nSaved = 0;
    int nSkipped = 0;
    for (int i = 0; i < pList->GetCount(); i++) {
        char szPath[256];
        C2DA c2da(CResRef(pList->GetString(i)->CStr()), 0);
        if (c2da.Load2DArray()) {
            if (c2da.m_b2daBinary) {
                nSkipped = nSkipped + 1;
            } else {
                sprintf_s(szPath, sizeof(szPath), "logs\\%s.2da", pList->GetString(i)->CStr());
                if (c2da.SaveBinary2da(szPath))
                    nSaved = nSaved + 1;
            }
        }
    }

    int nFailed = pList->GetCount() - nSaved - nSkipped;
    if (nFailed != 0) {
        sprintf_s(szBuffer, sizeof(szBuffer),
                  "**** %d files failed!  Binarized %d files to logs\\; %d skipped", nFailed,
                  nSaved, nSkipped);
    } else {
        sprintf_s(szBuffer, sizeof(szBuffer), "Binarized %d files to logs\\; %d skipped", nSaved,
                  nSkipped);
    }
    delete pList;
    return szBuffer;
}

// Starts a macro recording, or stops the one already running. An empty argument records to the
// default macro.
char* MacroRecord(char* sArgs) {
    char cFirst = sArgs[0];
    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents()) {
        g_pAppManager->m_pClientExoApp->SetCaptureEvents(CExoString(""));
        return const_cast<char*>("Finished Macro recording");
    }
    g_pAppManager->m_pClientExoApp->SetCaptureEvents(
        CExoString(cFirst ? sArgs : "inputmacro.default"));
    return const_cast<char*>("Macro is recording...");
}

char* MacroStop(char* sArgs) {
    K2_UNUSED(sArgs);
    g_pAppManager->m_pClientExoApp->SetCaptureEvents(CExoString(""));
    return const_cast<char*>("Finished Macro recording");
}

char* MacroPlay(char* sArgs) {
    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents()) {
        return const_cast<char*>("Ignoring that one...");
    }
    g_pAppManager->m_pClientExoApp->SetCaptureEventPlayBack(
        CExoString(sArgs[0] ? sArgs : "inputmacro.default"));
    g_pAppManager->m_pClientExoApp->SetCaptureEventPlayBackRate(5);
    g_bEnableScreenCapture = 0;
    return const_cast<char*>("Playing Macro...");
}

// One registration object per command; a command missing here is not found by the typed console.
#include "aurconsole.h"

ConsoleFunc ConFheal(const_cast<char*>("heal"), heal);
ConsoleFunc ConFsetcomputeruse(const_cast<char*>("setcomputeruse"), setcomputeruse);
ConsoleFunc ConFsetdemolitions(const_cast<char*>("setdemolitions"), setdemolitions);
ConsoleFunc ConFsetstealth(const_cast<char*>("setstealth"), setstealth);
ConsoleFunc ConFsetawareness(const_cast<char*>("setawareness"), setawareness);
ConsoleFunc ConFsetpersuade(const_cast<char*>("setpersuade"), setpersuade);
ConsoleFunc ConFsetrepair(const_cast<char*>("setrepair"), setrepair);
ConsoleFunc ConFsetsecurity(const_cast<char*>("setsecurity"), setsecurity);
ConsoleFunc ConFsettreatinjury(const_cast<char*>("settreatinjury"), settreatinjury);
ConsoleFunc ConFsetstrength(const_cast<char*>("setstrength"), setstrength);
ConsoleFunc ConFsetdexterity(const_cast<char*>("setdexterity"), setdexterity);
ConsoleFunc ConFsetconstitution(const_cast<char*>("setconstitution"), setconstitution);
ConsoleFunc ConFsetintelligence(const_cast<char*>("setintelligence"), setintelligence);
ConsoleFunc ConFsetwisdom(const_cast<char*>("setwisdom"), setwisdom);
ConsoleFunc ConFsetcharisma(const_cast<char*>("setcharisma"), setcharisma);
ConsoleFunc ConFaddexp(const_cast<char*>("addexp"), addexp);
ConsoleFunc ConFturbo(const_cast<char*>("turbo"), turbo);
ConsoleFunc ConFinvulnerability(const_cast<char*>("invulnerability"), invulnerability);
ConsoleFunc ConFrestartminigame(const_cast<char*>("restartminigame"), restartminigame);
ConsoleFunc ConFbright(const_cast<char*>("bright"), bright);
ConsoleFunc ConFaddlightside(const_cast<char*>("addlightside"), addlightside);
ConsoleFunc ConFadddarkside(const_cast<char*>("adddarkside"), adddarkside);
ConsoleFunc ConFrevealmap(const_cast<char*>("revealmap"), revealmap);
ConsoleFunc ConFgivecredits(const_cast<char*>("givecredits"), givecredits);
ConsoleFunc ConFgiveitem(const_cast<char*>("giveitem"), giveitem);
ConsoleFunc ConFgivemed(const_cast<char*>("givemed"), givemed);
ConsoleFunc ConFgiverepair(const_cast<char*>("giverepair"), giverepair);
ConsoleFunc ConFgivecomspikes(const_cast<char*>("givecomspikes"), givecomspikes);
ConsoleFunc ConFgivesecspikes(const_cast<char*>("givesecspikes"), givesecspikes);
ConsoleFunc ConFgivesitharmour(const_cast<char*>("givesitharmour"), givesitharmour);
ConsoleFunc ConFwarp(const_cast<char*>("warp"), warp);
ConsoleFunc ConFinfiniteuses(const_cast<char*>("infiniteuses"), infiniteuses);
ConsoleFunc ConFwhereami(const_cast<char*>("whereami"), whereami);
ConsoleFunc ConFaddlevel(const_cast<char*>("addlevel"), addlevel);
ConsoleFunc ConFdancedancemalak(const_cast<char*>("dancedancemalak"), dancedancemalak);
