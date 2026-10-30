// The application entry point, with WinMain, the window and message pump, the debug console
// commands and the mod-folder hooks.

#include "k2stdint.h"
#include "port/aslglapi.h"
#include "port/sdlapi.h"
#include "port/Mac/MacMain.h"
#include "main.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwccreature.h"
#include "exoresman.h"
#include "include/aurora.h"
#include "aurinternal.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "exosound.h"
#include "exosoundinternal.h"
#include "serverexoapp.h"
#include "guiingame.h"
#include "clientoptions.h"
#include "exoinput.h"
#include "exofile.h"
#include "worldtimer.h"
#include "exostring.h"
#include "exoiniinternal.h"
#include "List.h"
#include "aurfont.h"
#include "netlayer.h"
#include "nwscreature.h"
#include "exobase.h"
#include "ClientExoAppInternal.h"
#include "GraphicsSettings.h"
#include "aurglcore.h"
#include "restype.h"
#include "port/win32const.h"
#include "port/sdlconst.h"
#include "port/k2viewerapi.h"

// The Xbox-era memory budgets XbxMem reports against, in bytes.
enum {
    XBOX_MEMORY_BUDGET = 27 * 1024 * 1024,
    XBOX_LEVEL_BUDGET = 13 * 1024 * 1024,
    XBOX_LEVEL_BUDGET_CAP = 11776 * 1024,     // 11.5 MiB
    XBOX_LEVEL_BUDGET_BONUS = 1536 * 1024,    // 1.5 MiB
};

// The character that toggles the console: the backquote, or on the German layout the key in the
// same place, which types the degree sign (Latin-1 0xb0).
enum { CONSOLE_KEY = '`', CONSOLE_KEY_GERMAN = 176 };

// The mouse mode ReInitAurora records when there is no input object to read one from.
enum { MOUSEMODE_UNSET = 0xff };

// The cinematic black-screen backdrop's size.
enum { BACKDROP_WIDTH = 1600, BACKDROP_HEIGHT = 1200 };

int g_nDesiredFullScreenRefreshRate = 60;
unsigned char g_nBitsPerPixel = 32;
int g_bDo3DGUIs = 1;
class CAuroraInterface;
class CScriptCompiler;
class CTlkTable;
class CSWTlkTable;
CAuroraInterface* g_pAuroraInterface;
CScriptCompiler* g_pScriptCompiler;
CTlkTable* g_pTlkTable;
CSWTlkTable* g_pSWTlkTable;
#if !KOTOR1
int g_bDemoMode;
#endif

#if KOTOR1
float g_SwapTime;
#endif
unsigned char g_nClientServerMode;
CExoBase* g_pExoBase;
#if !KOTOR1
int g_nDesiredStretch_Mac;
#endif

#if KOTOR1
int mouseDeltaX;
int mouseDeltaY;
// The SDL port's input state (K2 keeps its own in MacMain.cpp), holding the smoothed and the
// accumulated mouse deltas, the on-screen Esc during a minigame, and the minigame's pause.
float Ios_lastfDeltaX;
float Ios_lastfDeltaY;
int   Ios_accumulatedDeltaX;
int   Ios_accumulatedDeltaY;
int   Ios_escPress;
bool  Ios_minigamePaused;
#endif
CExoInput* g_pExoInput;
#if !KOTOR1
char mac_showFPS;
CExoSound* g_pExoSound;
#endif
int g_nReInitializing;
int g_bAllowWindowedMode;
int g_nScreenLeftOffset;
int g_nScreenTopOffset;
#if !KOTOR1
int g_bRenderWindowActive;
#endif
int maxmemoryused;
float displayFPS;
float g_ClientUpdateTime;
float g_ServerUpdateTime;
int g_mdlMem;
int g_mdxMem;
int g_txbMem;
int g_wavMem;
int verts;
int anims;
int texts;
int parts;
int verts2;
int g_bExitProgram;
#if !KOTOR1
char g_GamePauseNeeded;
#endif

char g_useModFolder = 0;
int s_numSubscribedMods = 0;

bool IsGameModded()
{
    return g_useModFolder || s_numSubscribedMods > 0;
}

#include "port/libcbase.h"
#include "port/libcstring.h"

#if KOTOR1
#include "port/k1gui.h"
#endif
#include "port/crtapi.h"
#include "exostringinline.h"

struct HWND__;
extern "C" int SystemParametersInfoA(unsigned int uiAction, unsigned int uiParam, void* pvParam,
                                     unsigned int fWinIni);
int g_nMouseTrails;
int g_nScreenSaverEnabled;
HWND__* g_hWnd;
#ifdef K2_HOST_BUILD
K2HostViewer* g_pK2HostViewer;
#endif

static int sprintf_s(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nResult = __vsprintf_chk(pszBuf, 0, nSize, pszFmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

// The memory-report counters, engine-wide BSS globals the resource manager maintains.
extern int usedtexturememory;
extern int maxmemoryused;
extern int g_nTotalContiguousMemory;
extern int g_nTotalNewAllocated;
#if KOTOR1 && !defined(K2_HOST_BUILD)
extern char g_szProcessModuleMemName[];
#else
extern char* g_szProcessModuleMemName;
#endif
extern int usedlightmapmemory;
extern int usedcomptexmemory;
extern int usedcubemapmemory;
extern int usedbumpmapmemory;
extern int animationfootprint;
extern int countpart;
extern int totalvertexpool;
extern int totalvertexpool4k;
extern int totalindexpool;
extern int totalindexpool4;
extern int RyanTotalLevelVertices;
extern int RyanTotalLevelVertSize;
extern int RyanStartTextureSize;
extern int RyanStartParts;
extern int RyanEndParts;
extern int RyanEndTextureSize;
extern int RyanLevelTextureSize;
extern int RyanLevelTotalParts;
extern int RyanLevelVertSize;
extern int RyanLevelVertices;
extern int g_mdlMem;
extern int g_mdxMem;
extern int g_txbMem;
extern int g_wavMem;
extern float g_ClientUpdateTime;
extern float g_SndUpdateTime;
extern float g_AIUpdateTime;
extern float g_GuiUpdateTime;
extern float g_ServerUpdateTime;
extern float g_AIModuleTime;
extern float g_AIObjectTime;
CAppManager* g_pAppManager;
CExoResMan* g_pExoResMan;
extern int g_nReInitializing;      // 1 = windowed, 2 = fullscreen re-init in flight
CAurInternal* g_pAurora;

#if !KOTOR1  // K1's SDL port has its own entry point
// Handles the console command `SetResolution nWidth nHeight` by parsing both dimensions, then
// arming the windowed or the fullscreen re-initialisation the frame loop consumes.
char* SetResolution(char* pTag)
{
    static char s_szResolution[80];
    int nWidth = 0, nHeight = 0;
    static_cast<void>(sscanf(pTag, "%d%d", &nWidth, &nHeight));
    if (nWidth == 0 || nHeight == 0) {
        sprintf_s(s_szResolution, sizeof(s_szResolution),
                  "Screen resolution is %d x %d.  Usage:  SetResolution nWidth nHeight",
                  g_nScreenWidth, g_nScreenHeight);
        return s_szResolution;
    }
    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    if (g_pAurora->FullScreen() == 0) {
        g_nReInitializing = 1;
        sprintf_s(s_szResolution, sizeof(s_szResolution), "Setting %d x %d windowed...", nWidth,
                  nHeight);
    } else {
        g_nReInitializing = 2;
        int nRefresh = g_nDesiredFullScreenRefreshRate;
        g_pAurora->GetValidMode(reinterpret_cast<unsigned int*>(&nWidth), reinterpret_cast<unsigned int*>(&nHeight), &g_nBitsPerPixel,
                                reinterpret_cast<unsigned int*>(&nRefresh), 1);
        sprintf_s(s_szResolution, sizeof(s_szResolution), "Setting %d x %d, %d bpp, %d refresh...",
                  nWidth, nHeight, g_nBitsPerPixel, nRefresh);
    }
    return s_szResolution;
}
#endif

// The console command `setclearcolor r g b a`.
void SetClearColor(char* pTag)
{
    float fRed = 0.0f, fGreen = 0.0f, fBlue = 0.0f, fAlpha = 0.0f;
    static_cast<void>(sscanf(pTag, "%f %f %f %f", &fRed, &fGreen, &fBlue, &fAlpha));
    ASLgl::glClearColor(fRed, fGreen, fBlue, fAlpha);
}

// The console command `savecreature`.
void SaveCreature(char* pTag)
{
    K2_UNUSED(pTag);
    g_pAppManager->m_pClientExoApp->GetPlayerCreature()->SaveCharGenCreature();
}

char* Buffers(char* pTag)
{
    static char s_szBuffers[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szBuffers, sizeof(s_szBuffers), "VB[%d][%d] IB[%d][%d]",
              totalvertexpool / 1024, totalvertexpool4k / 1024,
              totalindexpool / 1024, totalindexpool4 / 1024);
    return s_szBuffers;
}

char* C_Stats(char* pTag)
{
    static char s_szClientStats[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szClientStats, sizeof(s_szClientStats), "tot[%06.1f] snd[%06.1f] AI:[%06.1f] gui:[%06.1f]",
              g_ClientUpdateTime, g_SndUpdateTime, g_AIUpdateTime, g_GuiUpdateTime);
    return s_szClientStats;
}

char* S_Stats(char* pTag)
{
    static char s_szServerStats[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szServerStats, sizeof(s_szServerStats), "tot[%07.2f]", g_ServerUpdateTime);
    return s_szServerStats;
}

char* AI_Stats(char* pTag)
{
    static char s_szAIStats[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szAIStats, sizeof(s_szAIStats), "Mod[%05.2f] Obj[%05.2f]",
              g_AIModuleTime, g_AIObjectTime);
    return s_szAIStats;
}

char* TexMem(char* pTag)
{
    static char s_szTexMem[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szTexMem, sizeof(s_szTexMem), "(kB) Uncmp %d Cmp %d LM %d CM %d BM %d",
              (usedtexturememory - usedlightmapmemory - usedcomptexmemory - usedcubemapmemory
               - usedbumpmapmemory) / 1024,
              usedcomptexmemory / 1024, usedlightmapmemory / 1024,
              usedcubemapmemory / 1024, usedbumpmapmemory / 1024);
    return s_szTexMem;
}

char* GfxMemOld(char* pTag)
{
    static char s_szGfxMemOld[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szGfxMemOld, sizeof(s_szGfxMemOld), "(kB) Textures %d Verts %d Anims %d",
              usedtexturememory / 1024, totalvertexpool / 1024, animationfootprint / 1024);
    return s_szGfxMemOld;
}

// The Xbox-era budget report, in which `Total Mem` is 27 MiB minus what is in use and `Level Mem`
// is the 13 MiB level budget minus the vertex pool and the textures, clamped to 11.5 MiB and then
// raised by 1.5 MiB. The compressed-texture arm is a separate formula: without it the uncompressed
// texture bytes are weighted by 0.23.
char* XbxMem(char* pTag)
{
    static char s_szXbxMem[128];
    int nTotal;
    int nLevelMem;
    K2_UNUSED(pTag);
    if (usedcomptexmemory != 0) {
        nTotal = totalvertexpool + animationfootprint + usedtexturememory;
    } else {
        nTotal = usedlightmapmemory + usedcubemapmemory + usedbumpmapmemory + totalvertexpool
               + animationfootprint
               + static_cast<int>(0.23f * static_cast<float>(usedtexturememory - usedlightmapmemory
                                        - usedcubemapmemory - usedbumpmapmemory));
    }
    nLevelMem = XBOX_LEVEL_BUDGET - (totalvertexpool + usedtexturememory);
    nLevelMem = (nLevelMem < XBOX_LEVEL_BUDGET_CAP) ? nLevelMem : XBOX_LEVEL_BUDGET_CAP;
    sprintf_s(s_szXbxMem, sizeof(s_szXbxMem), "Parts %d Total Mem %d Level Mem %d", countpart,
              (XBOX_MEMORY_BUDGET - nTotal) / 1024, (nLevelMem + XBOX_LEVEL_BUDGET_BONUS) / 1024);
    return s_szXbxMem;
}

char* GuiMem(char* pTag)
{
    static char s_szGuiMem[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szGuiMem, sizeof(s_szGuiMem), "Tex %d Part %d", RyanStartTextureSize / 1024,
              RyanStartParts);
    return s_szGuiMem;
}

char* LevMem(char* pTag)
{
    static char s_szLevMem[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szLevMem, sizeof(s_szLevMem), "Tot %d Tex %d Part %d Vert %d-%d",
              (RyanLevelTextureSize + RyanLevelVertSize) / 1024, RyanLevelTextureSize / 1024,
              RyanLevelTotalParts, RyanLevelVertices, RyanLevelVertSize / 1024);
    return s_szLevMem;
}

// The load-time delta report, showing what the level cost against the `RyanEnd*` snapshot.
char* DynMem(char* pTag)
{
    static char s_szDynMem[128];
    int nTexBytes = usedtexturememory - RyanEndTextureSize;
    int nVertBytes = RyanTotalLevelVertSize - RyanLevelVertSize;
    K2_UNUSED(pTag);
    sprintf_s(s_szDynMem, sizeof(s_szDynMem), "Tot %d Tex %d Part %d Vert %d-%d",
              (nVertBytes + nTexBytes) / 1024, nTexBytes / 1024, countpart - RyanEndParts,
              RyanTotalLevelVertices - RyanLevelVertices, nVertBytes / 1024);
    return s_szDynMem;
}

// Formats the resource manager's own census. The four leading out-parameters are unused by the
// string, but their addresses still go to GetResourcesInMemory, which is why they are passed.
char* GfxMem(char* pTag)
{
    static char s_szGfxMem[128];
    int nResourceCount, nKeyEntryCount, nResourceBytes, nPackFileBytes;
    K2_UNUSED(pTag);
    g_pExoResMan->GetResourcesInMemory(&nResourceCount, &nKeyEntryCount, &nResourceBytes,
                                       &nPackFileBytes, &g_mdlMem, &g_mdxMem, &g_txbMem,
                                       &g_wavMem);
    sprintf_s(s_szGfxMem, sizeof(s_szGfxMem), "tot: %d md:%d mx:%d tx:%d",
              (g_mdlMem + g_mdxMem + g_txbMem) / 1024, g_mdlMem / 1024, g_mdxMem / 1024,
              g_txbMem / 1024);
    return s_szGfxMem;
}

// Same census, but it reports the call's own return value plus the tail of the out-parameters.
char* ResMem(char* pTag)
{
    static char s_szResMem[128];
    int nResourceCount, nKeyEntryCount, nResourceBytes, nPackFileBytes;
    int nResult;
    K2_UNUSED(pTag);
    nResult = g_pExoResMan->GetResourcesInMemory(&nResourceCount, &nKeyEntryCount, &nResourceBytes,
                                                 &nPackFileBytes, &g_mdlMem, &g_mdxMem, &g_txbMem,
                                                 &g_wavMem);
    sprintf_s(s_szResMem, sizeof(s_szResMem), "%d %d %d/%d %d", nResult / 1024,
              nResourceBytes / 1024, nResourceCount, nKeyEntryCount, nPackFileBytes / 1024);
    return s_szResMem;
}

// The census without the resource-manager call, which reads the per-type totals the census writes
// into globals. `wav` is included in the grand total here, unlike GfxMem above.
char* ResMem2(char* pTag)
{
    static char s_szResMem2[128];
    K2_UNUSED(pTag);
    sprintf_s(s_szResMem2, sizeof(s_szResMem2), "tot: %d md:%d mx:%d tx:%d wv:%d",
              (g_mdlMem + g_mdxMem + g_txbMem + g_wavMem) / 1024, g_mdlMem / 1024,
              g_mdxMem / 1024, g_txbMem / 1024, g_wavMem / 1024);
    return s_szResMem2;
}

char const* ClearLogCounters(char* pTag)
{
    K2_UNUSED(pTag);
    return "NOT ENABLED";
}

char const* DumpLogCounters(char* pTag)
{
    K2_UNUSED(pTag);
    return "NOT ENABLED";
}

void ChangeResolutionForMovie()
{
}

void RestoreResolutionFromMovie()
{
}

void HandleLoadError(uint64_t strRef)
{
    K2_UNUSED(strRef);
}

// SPI_GETMOUSETRAILS / SPI_SETMOUSETRAILS and SPI_GETSCREENSAVEACTIVE / SPI_SETSCREENSAVEACTIVE.
// Store* snapshots the OS setting into the port's own global and pins the OS setting off;
// Restore* puts it back. The movie player brackets its playback with the pair.
enum {
    SPI_GETSCREENSAVEACTIVE = 0x10,
    SPI_SETSCREENSAVEACTIVE = 0x11,
    SPI_SETMOUSETRAILS = 0x5d,
    SPI_GETMOUSETRAILS = 0x5e,
};

void StoreMouseTrailsState()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    int nMouseTrails = 0;
    if (SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &nMouseTrails, 0) && nMouseTrails >= 1) {
        g_nMouseTrails = nMouseTrails;
        SystemParametersInfoA(SPI_SETMOUSETRAILS, 0, 0, 0);
    }
#endif
}

void StoreScreenSaverState()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    int nScreenSaver = 0;
    if (SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &nScreenSaver, 0) && nScreenSaver >= 1) {
        g_nScreenSaverEnabled = nScreenSaver;
        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, 0, 0, 0);
    }
#endif
}

void RestoreMouseTrailsState()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    if (g_nMouseTrails >= 1)
        SystemParametersInfoA(SPI_SETMOUSETRAILS, g_nMouseTrails, 0, 0);
#endif
}

void RestoreScreenSaverState()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    if (g_nScreenSaverEnabled >= 1)
        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, g_nScreenSaverEnabled, 0, 0);
#endif
}

HWND__* GetHWnd()
{
    return g_hWnd;
}

// The five resource-delta counters the debug HUD reads; `tc` is the only writer here.
extern int verts, anims, texts, parts, verts2;
char* spawn(char* pTag);          // scene spawn pass
void AurTextureMakeCurrent();

// Handles the console command `tc`, which snapshots the resource counters, runs the scene's spawn
// and texture-make-current pass, then reports the delta (raw vertex counts, kB on the others). The
// deltas are published to the five globals before being formatted.
char* tc(char* pTag)
{
    static char s_szStats[128];
    int nTextures = usedtexturememory;
    int nVerts = totalvertexpool;
    int nLevelVerts = RyanTotalLevelVertices;
    int nAnims = animationfootprint;
    int nParts = countpart;
    spawn(pTag);
    AurTextureMakeCurrent();
    parts = countpart - nParts;
    verts = (totalvertexpool - nVerts) / 1024;
    anims = (animationfootprint - nAnims) / 1024;
    texts = (usedtexturememory - nTextures) / 1024;
    verts2 = RyanTotalLevelVertices - nLevelVerts;
    sprintf_s(s_szStats, sizeof(s_szStats), "Verts %d-%d Anims %d Text %d Parts %d", verts2, verts,
              anims, texts, parts);
    return s_szStats;
}

void SetupOpenGL()
{
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    GLRender::Enable(Aurora::kCapCullFace);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Enable(Aurora::kCapAlphaTest);
}


// Window and focus state. g_nPauseStateOnDeactivate is file scope and therefore `static` below;
// every other global here is exported.
#if KOTOR1
extern bool g_bRenderWindowActive;
#else
extern int g_bRenderWindowActive;
#endif
struct HWND__;
extern int g_bAllowWindowedMode;
HWND__* g_hRenderWnd;
extern char g_GamePauseNeeded;
extern int  Ios_escPress;
extern int  g_bExitProgram;
uint64_ll_t s_focusLostTime;
uint64_ll_t g_nCurrentSessionStartFILETIME;
extern CExoSound* g_pExoSound;
// The build and version strings. Only the build number has a literal; the rest are filled in at
// startup.
CExoString g_sBuildNumber("427");
CExoString g_sInstallBuildNumber;
CExoString g_sVersionNumber;
CExoString g_sGameName;
CExoString g_sCopyrightInfo;
int GetMiniGamePlayingType();
#include "port/win32api.h"
#if !KOTOR1
static int g_nPauseStateOnDeactivate;
static int g_nMessagePumpTimer = 10;
#endif

#if KOTOR1
// K1 has no flag argument and switches the sound mode last, only when the window was active.
void DeactivateRenderWindow()
{
    if (g_bRenderWindowActive) {
        if (g_pAppManager->m_pServerExoApp != 0) {
            g_pAppManager->m_nPauseStateOnDeactivate =
                g_pAppManager->m_pServerExoApp->GetPauseState(2);
            if (g_pAppManager->m_nPauseStateOnDeactivate == 0)
                g_pAppManager->m_pServerExoApp->SetPauseState(2, 1);
        }
        g_pAppManager->m_pClientExoApp->SetInputActive(0, 0);
        if (g_pExoSound != 0)
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_LOSTFOCUS);
        g_bRenderWindowActive = 0;
    }
}
#endif

// Mutes the sound system as the render window goes away, remembers the server's pause state and
// pauses it if it was running, then drops the client's input. The pause state is cached in
// CAppManager, not in the file-local GameLostFocus uses.
void DeactivateRenderWindow(int nFlag)
{
    if (g_pExoSound != 0)
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_LOSTFOCUS, nFlag);

    if (g_bRenderWindowActive != 0) {
        if (g_pAppManager->m_pServerExoApp != 0) {
            g_pAppManager->m_nPauseStateOnDeactivate =
                g_pAppManager->m_pServerExoApp->GetPauseState(2);
            if (g_pAppManager->m_nPauseStateOnDeactivate == 0)
                g_pAppManager->m_pServerExoApp->SetPauseState(2, 1);
        }
        g_pAppManager->m_pClientExoApp->SetInputActive(0, 0);
        g_bRenderWindowActive = 0;
    }
}

// Mirrors the deactivate arm by unpausing only what it paused, taking the input back, switching the
// sound system to the gain-focus mode and restoring the user's gamma.
void ActivateRenderWindow()
{
    if (g_bRenderWindowActive != 0)
        return;

    g_bRenderWindowActive = 1;
    if (g_pAppManager->m_pServerExoApp != 0 && g_pAppManager->m_nPauseStateOnDeactivate == 0)
        g_pAppManager->m_pServerExoApp->SetPauseState(2, 0);
    g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);

    if (g_pExoSound != 0) {
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GAINFOCUS, 0);
    }
    g_pAurora->ResetGammaRamp(g_pAppManager->m_pClientExoApp->GetClientOptions()->GetGammaSetting(), 0);
}

// The focus pair's other mechanism, where a file-local global holds the pause state instead of the
// CAppManager field, and the timestamp block bills the unfocused time against the session start.
void GameGainedFocus()
{
#if KOTOR1  // K1: only the session-time bookkeeping
    if (g_pAppManager->m_pClientExoApp != 0) {
        uint64_ll_t nNow;
        GetSystemTimeAsFileTime(&nNow);
        uint64_ll_t nElapsed = nNow - s_focusLostTime;
        s_focusLostTime = nElapsed;
        g_nCurrentSessionStartFILETIME += nElapsed;
    }
#else
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0) {
        if (g_nPauseStateOnDeactivate == 0)
            g_pAppManager->m_pClientExoApp->SetPauseState(2, 0, 0);

        uint64_ll_t nNow;
        GetSystemTimeAsFileTime(&nNow);
        uint64_ll_t nElapsed = nNow - s_focusLostTime;
        s_focusLostTime = nElapsed;
        g_nCurrentSessionStartFILETIME += nElapsed;
    }

    if (g_pExoSound != 0) {
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GAINFOCUS, 0);
        g_pExoSound->StartDuckingMusicAndAmbientSounds();
    }
#endif
}

// Records the client's pause state for the gain arm, auto-pauses when the player was in game and
// nothing was obscuring the screen, and raises the escape latch while a minigame is running.
void GameLostFocus()
{
#if KOTOR1  // K1: no auto-pause; a running minigame only gets the escape press
    if (g_pAppManager->m_pClientExoApp != 0) {
        uint64_ll_t nNow;
        GetSystemTimeAsFileTime(&nNow);
        s_focusLostTime = nNow;
    }

    if (GetMiniGamePlayingType() != 0 && !Ios_minigamePaused)
        Ios_escPress = 1;
#else
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0) {
        g_nPauseStateOnDeactivate = g_pAppManager->m_pClientExoApp->GetPauseState(2);
        if (g_nPauseStateOnDeactivate == 0
            && g_pAppManager->m_pClientExoApp->GetLoadScreenVisible() == 0
            && g_pAppManager->m_pClientExoApp->GetInGameGui() != 0
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->IsGlobalFadeObscuring() == 0) {
            g_pAppManager->m_pClientExoApp->SetPauseState(2, 1, 0);
            g_GamePauseNeeded = 1;
        }

        uint64_ll_t nNow;
        GetSystemTimeAsFileTime(&nNow);
        s_focusLostTime = nNow;
    }

    if (GetMiniGamePlayingType() != 0 && !Ios_minigamePaused) {
        Ios_escPress = 1;
        Ios_minigamePaused = 1;
    }
#endif
}

// Drops the client input, then either posts an activate-app message to the render window (when
// windowed mode is off) or arms a re-initialisation: 1 for windowed, 2 for fullscreen.
void HandleAltReturn(int nKey, int nFlag)
{
#if KOTOR1
    K2_UNUSED(nKey); K2_UNUSED(nFlag);
#endif
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);

    if (g_bAllowWindowedMode == 0) {
        SendMessageA(g_hRenderWnd, WM_ACTIVATEAPP, 0, 0);
        return;
    }
    if (g_pAurora == 0)
        return;

    if (nKey == 1) {
        if (nFlag == 1)
            g_nReInitializing = 2;
        else
            g_nReInitializing = 1;
    } else {
        g_nReInitializing = (g_pAurora->FullScreen() == 0) ? 2 : 1;
    }
#endif
}

#if KOTOR1
// K1's SDL pointer glue: the event loop turns touches and mouse events into these calls, each
// packing its position the Win32 way (x | y << 16) into a mouse message for
// CExoInput::BufferEvent. The drag accumulates into mouseDelta, which
// CExoRawInputInternal::GetMouseState drains.
extern int oldMouseX;              // initialised to -1
#if KOTOR1
int oldMouseX = -1;
#endif
extern int oldMouseY;
#if KOTOR1
int oldMouseY = -1;
#endif
extern int mouseDeltaX;
extern int mouseDeltaY;
extern int Ios_accumulatedDeltaX;
extern int Ios_accumulatedDeltaY;
extern float Ios_lastfDeltaX;
extern float Ios_lastfDeltaY;

// nPhase: 0 = move, 1 = press, 2 = release.
void GameMouseDo(int x, int y, int nPhase, bool bLeft)
{
    MSG msg;
    msg.message = WM_MOUSEMOVE;
    msg.wParam = 0;
    msg.lParam = (x & 0xffff) | (y << 16);
    if (nPhase == 0)
        g_pExoInput->BufferEvent(&msg);
    msg.message = bLeft ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
    if (nPhase == 1)
        g_pExoInput->BufferEvent(&msg);
    msg.message = bLeft ? WM_LBUTTONUP : WM_RBUTTONUP;
    if (nPhase == 2)
        g_pExoInput->BufferEvent(&msg);
}

void GameMouseWheel(int x, int y, int nDelta)
{
    MSG msg;
    msg.message = WM_MOUSEWHEEL;
    msg.wParam = 1 | (nDelta << 16);
    msg.lParam = (x & 0xffff) | (y << 16);
    g_pExoInput->BufferEvent(&msg);
}

// A press restarts the drag; during a minigame a press in the bottom-right corner button is Escape.
void GameMouseDown(int x, int y)
{
    MSG msg;
    msg.message = WM_LBUTTONDOWN;
    msg.wParam = 0;
    msg.lParam = (x & 0xffff) | (y << 16);
    g_pExoInput->BufferEvent(&msg);
    oldMouseX = x;
    oldMouseY = y;
    mouseDeltaX = 0;
    mouseDeltaY = 0;
    int nHalfWidth = g_SDL_mode.w / 2;
    int nHeight = g_SDL_mode.h;
    if (GetMiniGamePlayingType() != 0) {
        int nSize = static_cast<int>(__builtin_ceilf(static_cast<float>(nHeight) / 640.0f * 64.0f));
        if (nHeight - nSize - 5 < y) {
            int nRight = (nHeight * 2) / 3 + nHalfWidth + 5;
            if (x <= nRight && nRight - nSize - 10 < x)
                Ios_escPress = 1;
        }
    }
}

bool IsGUIInGameOn()
{
    return g_pAppManager->m_pClientExoApp->GetInGameGui()->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN;
}

bool IsGamePaused()
{
    if (g_pAppManager == 0)
        return false;
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    return pApp != 0 && pApp->GetPauseState(2) != 0;
}

// bNoDrag: a hover move that does not feed the camera drag.
void GameMouseMove(bool bNoDrag, int x, int y)
{
    MSG msg;
    msg.message = WM_MOUSEMOVE;
    msg.wParam = 0;
    msg.lParam = (x & 0xffff) | (y << 16);
    g_pExoInput->BufferEvent(&msg);
    if (!bNoDrag && (oldMouseX != x || oldMouseY != y)) {
        mouseDeltaX += x - oldMouseX;
        mouseDeltaY += y - oldMouseY;
    }
    oldMouseX = x;
    oldMouseY = y;
}

// A release ends the drag: the release itself, then a move to (0, 0).
void GameMouseUp(int x, int y)
{
    Ios_lastfDeltaY = 0;
    Ios_lastfDeltaX = 0;
    Ios_accumulatedDeltaY = 0;
    Ios_accumulatedDeltaX = 0;
    MSG msg;
    msg.message = WM_LBUTTONUP;
    msg.wParam = 0;
    msg.lParam = (x & 0xffff) | (y << 16);
    g_pExoInput->BufferEvent(&msg);
    msg.message = WM_MOUSEMOVE;
    msg.wParam = 0;
    msg.lParam = 0;
    g_pExoInput->BufferEvent(&msg);
    mouseDeltaX = 0;
    mouseDeltaY = 0;
}

void GameClick(int x, int y)
{
    oldMouseX = x;
    oldMouseY = y;
}
#endif

// The sound system is ticked first, then a countdown gates the whole pump: it starts at 10 and is
// never reset, so only the first 10 calls pump the queue. The pump stops on the first empty
// PeekMessageA and sets g_bExitProgram when GetMessageA reports WM_QUIT.
void messagepump()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    g_pExoSound->m_pInternal->m_audioSystem.UpdateSystem();

    if (--g_nMessagePumpTimer < 0)
        return;

    MSG msg;
    if (PeekMessageA(&msg, 0, 0, 0, 0) == 0)
        return;

    do {
        if (GetMessageA(&msg, 0, 0, 0) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            g_bExitProgram = 1;
        }
    } while (PeekMessageA(&msg, 0, 0, 0, 0) != 0);
#endif
}

// The first local is built and destroyed unread; the string handed to SetWindowTextA is a separate
// copy of g_sGameName.
void SetWindowTitle()
{
#if !KOTOR1  // K1: an empty stub (the SDL port owns the window)
    CExoString sCopyrightAndName = g_sCopyrightInfo + CExoString(" ") + g_sGameName;
    CExoString sName = g_sGameName;
    (void)sCopyrightAndName;
    SetWindowTextA(g_hWnd, sName.CStr());
#endif
}

// Appends one tab-separated level-stats line to D:\Logs\Levelstats.txt. Two slots are the literal
// -1: the engine has no counter for them.
void LogLevelStats()
{
#if KOTOR1
    // K1 logs through the port's file/line-tagged _fopen
    FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/swmain/main.cpp",
                         "LogLevelStats", 1143, "D:\\Logs\\Levelstats.txt", "a");
#else
    FILE* pFile = fopen("D:\\Logs\\Levelstats.txt", "a");
#endif
    if (pFile == 0)
        return;

    fputs(g_szProcessModuleMemName, pFile);
    fprintf(pFile, "\t%d", RyanLevelTotalParts);
    fprintf(pFile, "\t%d", RyanLevelVertices);
    fprintf(pFile, "\t%d", RyanLevelVertSize / 1024);
    fprintf(pFile, "\t%d", -1);
    fprintf(pFile, "\t%d", RyanLevelTextureSize / 1024);
    fprintf(pFile, "\t%d", -1);
    fprintf(pFile, "\t%d", countpart - RyanEndParts);
    fprintf(pFile, "\t%d", (usedtexturememory - RyanEndTextureSize) / 1024);
    fprintf(pFile, "\t%d", RyanTotalLevelVertices - RyanLevelVertices);
    fprintf(pFile, "\t%d", (RyanTotalLevelVertSize - RyanLevelVertSize) / 1024);
    fprintf(pFile, "\t%d", maxmemoryused);
    fprintf(pFile, "\t%d", usedtexturememory);
    fprintf(pFile, "\t%d", g_nTotalContiguousMemory);
    fprintf(pFile, "\t%d", g_nTotalNewAllocated);
    fputc('\n', pFile);
    fclose(pFile);
}

// The mobile port's console state, shared with DoConsole, DoConsoleHist and UpdateScreen, made of
// the line-active flag HandleWMCharMessage toggles, the cursor position DoConsole paints at, the
// "enter was hit" latch the frame loop consumes, and the input line itself.
struct ConsoleState {
    int  m_nActive;
    int  m_nCursor;
    int  m_bEnterPending;
    char m_szLine[512];
};
static ConsoleState g_console;

int g_bGoToKeyboardModeOff;
extern List<char*> g_listHistory;   // the console's typed history
extern int g_nHistoryPosition;      // the recall cursor into it

// The console's TAB completion and the command interpreter's dry run.
char* CommandCompletion(char* pszPartial, List<char*>& matches);
char* FuncPreInterp(char* pszCommand);

// The TAB completion's scroll offset into the candidate window, reset on a short candidate list and
// advanced by the window height per TAB.
static int s_nConsoleScroll;

#if KOTOR1
// K1's console clamps go through a min function: each argument is evaluated once, left first.
template <class T> static inline T K1Min(T a, T b) { return a < b ? a : b; }
#endif

// The console line editor. Backspace deletes the character before the cursor and shifts the tail
// left; Tab completes and scrolls a 15-line candidate window; Enter clears the line and latches
// m_bEnterPending for the frame loop to run the interpreter; Esc drops keyboard mode and clears.
// Anything else inserts at the cursor and clamps the line at 500 characters.
void DoConsole(char c)
{
    List<char*> matches;
    int nLen;
    int nCursor;
    float fSize;
    int i;
    switch (static_cast<unsigned char>(c)) {
    case '\b': {
        if (__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)) == 0)
            break;
        if (g_console.m_nCursor == 0)
            break;
#if KOTOR1  // K1 measures the line once per clamp
        g_console.m_nCursor = static_cast<int>(K1Min(static_cast<size_t>(static_cast<unsigned int>(g_console.m_nCursor)),
                                         __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))));
        nLen = static_cast<int>(K1Min(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)), static_cast<size_t>(500)));
#else
        g_console.m_nCursor = static_cast<unsigned int>(g_console.m_nCursor) >=
                                      __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))
                                  ? static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)))
                                  : g_console.m_nCursor;
        nLen = __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)) <= 500
                       ? static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)))
                       : 500;
#endif
        nCursor = g_console.m_nCursor;
        g_console.m_nCursor = nCursor - 1;
        if (nCursor > nLen)
            break;
        for (i = nCursor - 1; i < nLen; i++)
            g_console.m_szLine[i] = g_console.m_szLine[i + 1];
        break;
    }
    case '\t': {
        __strcpy_chk(g_console.m_szLine, CommandCompletion(g_console.m_szLine, matches),
                     sizeof(g_console.m_szLine));
        char* pCom = FuncPreInterp(g_console.m_szLine);
        AurPostString(pCom, 0, 1, 1.0f);
        if (matches.m_count > 1) {
            fSize = 1.0f;
            if (matches.m_count > 15)
                fSize = 4.0f;
            if (matches.m_count < 15)
                s_nConsoleScroll = 0;
#if KOTOR1
            for (i = 0; i < matches.m_count && i < 15; i++)
                AurPostString(matches.m_pData[(s_nConsoleScroll + i) % matches.m_count], 4, i + 6, fSize);
#else
            for (i = 0; i < 15 && i < matches.m_count; i++)
                AurPostString(matches.m_pData[(s_nConsoleScroll + i) % matches.m_count], 4, i + 6,
                              fSize);
#endif
            s_nConsoleScroll += 5;
        }
        g_console.m_nCursor = static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)));
        break;
    }
    case '\n':
    case '\r': {
        if (g_bGoToKeyboardModeOff != 0)
            g_pExoInput->KeyboardModeOff();
        g_console.m_nActive = 0;
        AurPostString(const_cast<char*>("* "), 0, 0, 5.0f);
        AurPostString(g_console.m_szLine, 2, 0, 5.0f);
        g_console.m_bEnterPending = 1;
        break;
    }
    case EDITKEY_ESCAPE: {
        if (g_bGoToKeyboardModeOff != 0)
            g_pExoInput->KeyboardModeOff();
        g_console.m_szLine[0] = 0;
        g_console.m_nActive = 0;
        g_console.m_nCursor = 0;
        break;
    }
    default: {
#if KOTOR1
        nLen = static_cast<int>(K1Min(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)), static_cast<size_t>(500)));
        g_console.m_nCursor = static_cast<int>(K1Min(static_cast<size_t>(static_cast<unsigned int>(g_console.m_nCursor)),
                                         __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))));
#else
        nLen = __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)) <= 500
                       ? static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)))
                       : 500;
        g_console.m_nCursor = static_cast<unsigned int>(g_console.m_nCursor) >=
                                      __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))
                                  ? static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)))
                                  : g_console.m_nCursor;
#endif
        if (nLen > g_console.m_nCursor) {
            for (i = nLen; i > g_console.m_nCursor; i--)
                g_console.m_szLine[i] = g_console.m_szLine[i - 1];
        }
        g_console.m_szLine[g_console.m_nCursor] = c;
        g_console.m_nCursor = g_console.m_nCursor + 1;
        // nLen + 1, not nLen: the shift above moved the old terminator one slot right, so the new
        // one belongs one past it.
        g_console.m_szLine[nLen + 1] = 0;
        break;
    }
    }
}

// Handles the console line's arrow-key editing. Left and right move the cursor (right clamps it at
// the line end), up and down walk the typed history.
void DoConsoleHist(unsigned short nKey)
{
#if KOTOR1
    bool bConsumed;
    if (nKey == VK_LEFT && g_console.m_nActive != 0) {   // cursor back
        bConsumed = false;
        int nPos = g_console.m_nCursor > 1 ? g_console.m_nCursor : 1;
        g_console.m_nCursor = nPos - 1;
    } else {
        bConsumed = nKey == VK_RIGHT;
        if (bConsumed && g_console.m_nActive != 0) {   // forward, clamped at the line end
            int nCursor = g_console.m_nCursor + 1;
            g_console.m_nCursor = nCursor;
            g_console.m_nCursor = static_cast<int>(K1Min(static_cast<size_t>(static_cast<unsigned int>(nCursor)),
                                             __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))));
            bConsumed = true;
        }
    }

#else
    bool bActive = g_console.m_nActive != 0;
    bool bConsumed = false;

    if (nKey == VK_LEFT) {          // cursor back
        if (bActive) {
            int nPos = g_console.m_nCursor > 1 ? g_console.m_nCursor : 1;
            g_console.m_nCursor = nPos - 1;
            bConsumed = false;
        }
    } else if (nKey == VK_RIGHT) {   // cursor forward, clamped at the line end
        if (bActive) {
            int nCursor = g_console.m_nCursor + 1;
            g_console.m_nCursor = nCursor;
            g_console.m_nCursor = __strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine))
                                      <= static_cast<uint64_t>(static_cast<unsigned int>(nCursor))
                                      ? static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)))
                                      : g_console.m_nCursor;
        }
        bConsumed = true;
    }

#endif
    if (g_listHistory.m_count == 0)
        return;

    // m_nActive is re-read here: the calls above may have changed it.
    if (nKey == VK_UP && g_console.m_nActive != 0) {   // one entry back
        int nPos = g_nHistoryPosition > 1 ? g_nHistoryPosition : 1;
        g_nHistoryPosition = nPos - 1;
#if KOTOR1
        g_console.m_nCursor =
            static_cast<int>(__strlen_chk(strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80), sizeof(g_console.m_szLine)));
#else
        strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80);
        g_console.m_nCursor = static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)));
#endif
    }

    // A cursor key can leave the line empty: put the parked history entry back into it, stepping
    // off the end first.
    if (bConsumed && g_console.m_nActive != 0) {
        if (__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)) == 0) {
            if (g_nHistoryPosition == g_listHistory.m_count)
                g_nHistoryPosition = g_nHistoryPosition - 1;
#if KOTOR1
            g_console.m_nCursor =
                static_cast<int>(__strlen_chk(strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80), sizeof(g_console.m_szLine)));
#else
            strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80);
            g_console.m_nCursor = static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)));
#endif
        }
    }

    if (nKey == VK_DOWN && g_console.m_nActive != 0) {   // one entry forward, or clear
        int nNext = g_nHistoryPosition + 1;
        g_nHistoryPosition = g_listHistory.m_count < nNext ? g_listHistory.m_count : nNext;
        if (g_listHistory.m_count > nNext) {
#if KOTOR1
            g_console.m_nCursor =
                static_cast<int>(__strlen_chk(strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80), sizeof(g_console.m_szLine)));
#else
            strncpy(g_console.m_szLine, g_listHistory.m_pData[g_nHistoryPosition], 80);
            g_console.m_nCursor = static_cast<int>(__strlen_chk(g_console.m_szLine, sizeof(g_console.m_szLine)));
#endif
        } else {
            g_console.m_szLine[0] = 0;
            g_console.m_nCursor = 0;
        }
    }
}

#if KOTOR1
extern int g_bMouseCameraRotateToggle;
#endif
// A character first joins the capture log; then, when it is the platform's console key, it reads
// EnableCheats from the ini and, only when cheats are on, toggles the soft keyboard and console.
// Everything else goes to the console line editor (when it is open) or the GUI manager.
int HandleWMCharMessage(unsigned short nChar)
{
    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() != 0) {
        if (g_pAppManager->m_pClientExoApp->GetCaptureEventFile() != 0) {
            CExoString sText;
            sText.Format(const_cast<char*>("%d %c %d\n"),
                         static_cast<unsigned long long>(g_pAppManager->m_pClientExoApp->GetWorldTimer()
                             ->GetSnapshotTime()) / 1000,
                         'C', nChar);
            static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sText);
        }
    }

    int nLang = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    unsigned short nConsoleKey = (nLang == 2) ? CONSOLE_KEY_GERMAN : CONSOLE_KEY;
    if (nLang == 5)
        nConsoleKey = 3;

    if (nConsoleKey == nChar) {
#if KOTOR1  // K1 clears the toggle global directly and reads "swkotor.ini" with CExoString::AsINT
        if (nLang == 5)
            g_bMouseCameraRotateToggle = 0;

        CExoIni ini;
        CExoString sValue;
        ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                         CExoString("EnableCheats"));
        if (sValue.AsINT() == 1) {
#else
        if (nLang == 5)
            EnableMouseCameraRotateToggle(false);

        CExoIni ini;
        CExoString sValue;
        ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                         CExoString("EnableCheats"));
        if (sValue.AsINT() == 1) {
#endif
            if (g_console.m_nActive != 0) {
                if (g_bGoToKeyboardModeOff != 0)
                    g_pExoInput->KeyboardModeOff();
                g_console.m_nActive = 0;
            } else {
                g_bGoToKeyboardModeOff = (g_pExoInput->GetKeyboardMode() == 0);
                g_pExoInput->KeyboardModeOn();
                g_pExoInput->ClearEvents();
                g_pAppManager->m_pClientExoApp->TurnOffKeyboardModal();
                g_console.m_nActive = 1;
            }
        }
        return 1;
    }

    if (g_pExoInput->GetKeyboardMode() != 0) {
        if (g_console.m_nActive != 0) {
            DoConsole(static_cast<char>(nChar));
            return 1;
        }
        if (g_pGuiMan != 0) {
            if (g_pGuiMan->HandleKeyPress(static_cast<int>(nChar)))
                return 1;
        }
    }
    return 0;
}

#if KOTOR1
// The on-screen keyboard's text box hands back its whole line: erase what the field held (25
// backspaces) and type the new text, dropping newlines.
void GameTextField(const char* pText)
{
    for (int i = 0; i < 25; i++)
        HandleWMCharMessage(8);
    for (; *pText != 0; pText++) {
        if (*pText != '\n')
            HandleWMCharMessage(*pText);
    }
}
#endif

#if KOTOR1
// K1: a stub; the port feeds keys through SDL.
int HandleWMKeyDownMessage(void* hWnd, unsigned int nKey, int lParam)
{
    K2_UNUSED(hWnd); K2_UNUSED(nKey); K2_UNUSED(lParam);
    return 0;
}
#else
// Runs the capture/replay logger, then the console key routing, which translates the arrow keys to
// the console's own codes when the soft keyboard is up and the console is open.
int HandleWMKeyDownMessage(HWND__* hWnd, unsigned long long nKey, int64_t lParam)
{
    K2_UNUSED(hWnd);
    K2_UNUSED(lParam);

    if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() != 0)
        return 0;

    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() != 0) {
        if (g_pAppManager->m_pClientExoApp->GetCaptureEventFile() != 0) {
            CExoString sText;
            sText.Format(const_cast<char*>("%d %c %d\n"),
                         static_cast<unsigned long long>(g_pAppManager->m_pClientExoApp->GetWorldTimer()
                             ->GetSnapshotTime()) / 1000,
                         'K', nKey);
            static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sText);
        }
    }

    switch (static_cast<int64_t>(nKey)) {
    case VK_RETURN:
    case VK_ESCAPE:
        HandleWMCharMessage(static_cast<unsigned short>(nKey));
        break;
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
        if (g_pExoInput->GetKeyboardMode() != 0) {
            if (g_console.m_nActive != 0) {
                DoConsoleHist(static_cast<unsigned short>(nKey));
            } else {
                switch (nKey) {
                case VK_LEFT: HandleWMCharMessage(EDITKEY_LEFT); break;
                case VK_UP: HandleWMCharMessage(EDITKEY_UP); break;
                case VK_RIGHT: HandleWMCharMessage(EDITKEY_RIGHT); break;
                case VK_DOWN: HandleWMCharMessage(EDITKEY_DOWN); break;
                }
            }
        }
        break;
    case VK_F10:
        return 1;
    }
    return 0;
}
#endif

// Parses the port's capture/replay flags: -ecf [file] records, -epf [file] plays back, -epr [rate]
// sets the playback rate (default 30). The two file arms start from the name "capture" and
// overwrite it from the next argument when one is present.
void ProcessCommandLine(int nArgc, char** ppszArgv)
{
    if (nArgc < 2)
        return;

    for (int i = 1; i < nArgc; ++i) {
        char* pszArg = ppszArgv[i];
        if (strncmp(pszArg, "-ecf", 4) == 0) {
            ++i;
            CExoString sFile("capture");
            if (i < nArgc)
                sFile = ppszArgv[i];
            g_pAppManager->m_pClientExoApp->SetCaptureEvents(sFile);
        } else if (strncmp(pszArg, "-epf", 4) == 0) {
            ++i;
            CExoString sFile("capture");
            if (i < nArgc)
                sFile = ppszArgv[i];
            g_pAppManager->m_pClientExoApp->SetCaptureEventPlayBack(sFile);
        } else if (strncmp(pszArg, "-epr", 4) == 0) {
            int nRate = 30;
            if (++i < nArgc)
                nRate = atoi(ppszArgv[i]);
            g_pAppManager->m_pClientExoApp->SetCaptureEventPlayBackRate(nRate);
        }
    }
}

// The Win32 windowing shim this file drives directly.
struct HINSTANCE__;
struct HDC__;
int64_t WinMessageHandler(HWND__*, unsigned int, unsigned long long, int64_t);
HINSTANCE__* g_hInst;
// CW_USEDEFAULT, for the four geometry arguments (the shim types them as `int`).
enum { kCWUsedDefault = -2147483647 - 1 };

// InitGameApp must be defined before the InitOpenGLWindow it forwards to.
extern CExoString g_sInstallBuildNumber;
extern CExoString g_sBuildNumber;
void GetGameVersionInfo();                 // called for its side effects
#if !KOTOR1
void InitOpenGLWindow(HINSTANCE__* hInstance, int nUnused);

// Second-stage init, which installs the render-window class, then publishes g_hInst and clears
// g_hWnd. The two CExoString chains are built and then destroyed unread.
void* InitGameApp(HINSTANCE__* hInstance, int nShowCmd)
{
    GetGameVersionInfo();

    CExoString sVersion = g_sCopyrightInfo + CExoString(" ") + g_sGameName;
    CExoString sBuild = g_sGameName + CExoString("    v") + g_sInstallBuildNumber + CExoString(".")
                        + g_sBuildNumber;

    LoadIconA(hInstance, 101);
    GetStockObject(BLACK_BRUSH);
    InitOpenGLWindow(hInstance, nShowCmd);

    g_hWnd = 0;
    g_hInst = hInstance;
    return 0;
}

#else
char* AurGetVersionString();               // the renderer tag appended to the build number
// K1 registers no window class: the build number gains " Graphics <renderer>" and the two title
// strings are built and dropped.
void* InitGameApp(void* hInstance, int nShowCmd)
{
    K2_UNUSED(nShowCmd);
    GetGameVersionInfo();
    g_sBuildNumber = g_sBuildNumber + CExoString(" Graphics ") + CExoString(AurGetVersionString());
    CExoString sVersion = g_sCopyrightInfo + CExoString(" ") + g_sGameName;
    CExoString sName = g_sGameName;
    g_hWnd = 0;
    g_hInst = static_cast<HINSTANCE__*>(hInstance);
    return 0;
}
#endif

int g_bInReInitAurora;
void AurDisableVertexBufferObjectsWriteOnly();
void AurEnableVertexBufferObjectsWriteOnly();

// Tear the renderer down and bring it back up at a new mode: hide the mouse, shut the aurora front
// end down, recreate the window, set the video mode, re-read the VBO option, re-enable the GL
// state, rebuild the input object and swap the window back in.
void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBitsPerPixel,
                  int bFullscreen, int bAllow)
{
    K2_UNUSED(bAllow);

    CExoInput* pInput;
    unsigned char nMouseMode;
    if (g_pAurora == 0)
        return;

    g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_REINIT_AURORA);

#if KOTOR1  // K1: no fullscreen query and no mode restore here
    int bNested = 1;
    if (g_bInReInitAurora != 1) {
        g_bInReInitAurora = 1;
        bNested = 0;
    }

    g_pAurora->ShutDown(0, 0);
#else
    bool bFlag = (g_pAurora->FullScreen() != 1) || (bFullscreen != 1);

    bool bNested = (g_bInReInitAurora == 1);
    if (!bNested)
        g_bInReInitAurora = 1;

    g_pAurora->ShutDown(0, bFlag);
#endif

    g_hRenderWnd = 0;

    CExoIni ini;
    CExoString sValue;
    GetTickCount();

    if (g_hRenderWnd != 0) {
        g_pAppManager->m_pClientExoApp->SetInputActive(0, 0);
        if (g_pExoInput != 0)
            delete g_pExoInput;
        g_pExoInput = 0;
    }

#if KOTOR1  // K1: the port keeps its window; no InitOpenGLWindow and no fullscreen mode probe
    int nRefresh = g_nDesiredFullScreenRefreshRate;
    g_pAurora->Initialize(g_hWnd, g_hRenderWnd, nWidth, nHeight, nBitsPerPixel, nRefresh,
                          bFullscreen, nBitsPerPixel, nBitsPerPixel);
#else
    InitOpenGLWindow(g_hInst, 0);

    int nRefresh = g_nDesiredFullScreenRefreshRate;
    if (bFullscreen != 0)
        g_pAurora->GetValidMode(&nWidth, &nHeight, &nBitsPerPixel, reinterpret_cast<unsigned int*>(&nRefresh), 1);

    g_pAurora->Initialize(g_hWnd, g_hRenderWnd, nWidth, nHeight, nBitsPerPixel, nRefresh,
                          bFullscreen, nBitsPerPixel, nBitsPerPixel);
#endif

    if (ini.ReadIniEntry(sValue, CExoString(EXO_GAME_INI), CExoString("Graphics Options"),
                         CExoString("Disable Write-Only VBO")) != 0) {
        if (sValue.AsINT() == 1)
            AurDisableVertexBufferObjectsWriteOnly();
        else
            AurEnableVertexBufferObjectsWriteOnly();
    }

    SetCapture(g_hRenderWnd);
    GetTickCount();
    SetupOpenGL();

    if (g_nReInitializing != 0)
        g_nReInitializing = 0;

    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    nMouseMode = MOUSEMODE_UNSET;
    if (g_pExoInput != 0) {
        nMouseMode = g_pExoInput->GetMouseMode();
        if (g_pExoInput != 0)
            delete g_pExoInput;
    }

    pInput = new CExoInput();
    g_pExoInput = pInput;
    if ((~nMouseMode & 0xff) != 0)
        pInput->SetMouseMode(nMouseMode);

    DeactivateRenderWindow(0);
    if (g_pAppManager->m_pClientExoApp->IsClientInitialized() != 0)
        g_pAppManager->m_pClientExoApp->SetEventDescriptions();
    ActivateRenderWindow();
    GetTickCount();

    if (!bNested)
        g_bInReInitAurora = 0;

    g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_REINIT_AURORA);
}

#if KOTOR1
// K1: an empty stub; SDL owns the window.
void InitOpenGLWindow(void* hInstance, int nUnused)
{
    K2_UNUSED(hInstance); K2_UNUSED(nUnused);
}
#else
// Registers the "Render Window" class and creates the 1600 x 1200 maximised render window the game
// draws into.
void InitOpenGLWindow(HINSTANCE__* hInstance, int nUnused)
{
    K2_UNUSED(nUnused);
    char szClassName[] = "Render Window";
    CExoString sName = g_sGameName;
    HWND__* hWnd;

    WNDCLASSA wc;
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WinMessageHandler;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(hInstance, 101);
    wc.hCursor = 0;
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = szClassName;
    RegisterClassA(&wc);

    hWnd = CreateWindowExA(WS_EX_APPWINDOW,
                           szClassName, sName.CStr(),
                           WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           kCWUsedDefault, kCWUsedDefault, kCWUsedDefault, kCWUsedDefault,
                           0, 0, hInstance, 0);
    if (hWnd != 0)
        g_hRenderWnd = hWnd;
}
#endif

#include "exomovieplayerinternal.h"
extern HWND__* g_hMovieWnd;
int g_bKillFocusWorked = 1;
int g_bActivationDueToFullScreenMovie;
int g_bActivationDueToAnyMovie;
int g_nWindowDeactivationTickCount;
int g_bMouseCameraRotateToggle;

#if !KOTOR1  // K1's library has no Win32 window: SDL owns it (MacMain.cpp's SDL_main)
// The port's window procedure is one switch on `msg`, with every arm that does not handle its
// message falling through to DefWindowProcA. The mouse arms are not uniform: the button-down and
// the double-click arms return 0 when the input object is absent or inactive where the others fall
// through, SetCapture is skipped on WM_LBUTTONDBLCLK, and the middle-button and wheel arms skip the
// "Reverse Mouse Buttons" check the other five run.
int64_t WinMessageHandler(HWND__* hWnd, unsigned int msg, unsigned long long wParam, int64_t lParam)
{
    unsigned int nMessage = msg;
    MSG event;
    switch (msg) {
    case WM_SIZE: {
        if (g_hRenderWnd == hWnd) {
            if (wParam == SIZE_MAXIMIZED) {
                g_nReInitializing = 2;
                return 0;
            }
        }
        break;
    }

    case WM_SETFOCUS: {
        if (g_bKillFocusWorked == 1)
            g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);
        break;
    }

    case WM_KILLFOCUS: {
        if (g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 0) {
            g_pAppManager->m_pClientExoApp->SetInputActive(0, 0);
            g_bKillFocusWorked = 1;
        } else {
            g_bKillFocusWorked = 0;
        }
        break;
    }

    case WM_CLOSE: {
        g_pAppManager->m_pClientExoApp->QueryExitProgram();
        return 0;
    }

    case WM_ACTIVATEAPP: {
        if (g_pExoInput == 0)
            break;
        if (wParam == 0) {
            g_bActivationDueToFullScreenMovie = 0;
            g_bActivationDueToAnyMovie = 0;
            int bMovie = 0;
            if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0 && g_pAurora != 0) {
                bMovie = g_pAppManager->m_pClientExoApp->IsMoviePlaying();
                if (bMovie == 1) {
                    if (g_pAurora->FullScreen() != 0)
                        g_bActivationDueToFullScreenMovie = 1;
                    g_bActivationDueToAnyMovie = 1;
                }
            }
            if (g_bActivationDueToFullScreenMovie == 0 && g_pAurora != 0 &&
                g_pAurora->FullScreen() == 1) {
                ShowWindow_Win32(g_hRenderWnd, SW_MINIMIZE);
                g_pAurora->RestoreDesktopDisplayMode();
            }
            if (g_hRenderWnd == hWnd) {
                DeactivateRenderWindow(bMovie);
                g_nWindowDeactivationTickCount = static_cast<int>(GetTickCount());
            }
            if (g_bActivationDueToAnyMovie == 0) {
                RestoreMouseTrailsState();
                RestoreScreenSaverState();
            }
        } else {
            if (g_bActivationDueToFullScreenMovie == 0 && g_pAurora != 0) {
                if (g_pAurora->FullScreen() == 0) {
                    if (g_bActivationDueToAnyMovie == 0 &&
                        g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 0 &&
                        static_cast<unsigned int>(GetTickCount() - g_nWindowDeactivationTickCount) > 100)
                        g_pAppManager->m_pClientExoApp->RestartMoviesIfNecessary();
                } else if (g_nReInitializing == 0) {
                    if (g_hRenderWnd == hWnd)
                        g_pAurora->RestoreAuroraDisplayMode();
                    g_nReInitializing = 2;
                }
            }
            if (g_hRenderWnd == hWnd &&
                (static_cast<unsigned int>(GetTickCount() - g_nWindowDeactivationTickCount) < 1000 ||
                 g_pAurora->FullScreen() == 0))
                ActivateRenderWindow();
            if (g_bActivationDueToAnyMovie == 0) {
                StoreMouseTrailsState();
                StoreScreenSaverState();
            }
        }
        break;
    }

    case WM_CANCELMODE: {
        if (g_bMouseCameraRotateToggle != 0)
            EnableMouseCameraRotateToggle(false);
        break;
    }

    case WM_SETCURSOR: {
        if ((lParam & 0xffff) == HTCLIENT && g_hRenderWnd == reinterpret_cast<HWND__*>(wParam) && g_pExoInput != 0) {
            g_pExoInput->RefreshMouseCursor(0);
            return 1;
        }
        break;
    }

    case WM_WINDOWPOSCHANGING: {
        if (g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 1) {
            MovieWinPos* pPos = static_cast<MovieWinPos*>(operator new(sizeof(MovieWinPos)));
            pPos->hwnd = g_hMovieWnd;
            pPos->hwndInsertAfter = 0;
            pPos->rc = reinterpret_cast<WINDOWPOS*>(lParam)->rc;
            pPos->flags = 0;
            PostMessageA(g_hMovieWnd, MOVIEMSG_WINDOWPOSCHANGING, wParam, reinterpret_cast<int64_t>(pPos));
        }
        break;
    }

    case WM_WINDOWPOSCHANGED: {
        if (g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 1) {
            MovieWinPos* pPos = static_cast<MovieWinPos*>(operator new(sizeof(MovieWinPos)));
            pPos->hwnd = g_hMovieWnd;
            pPos->hwndInsertAfter = 0;
            pPos->rc = reinterpret_cast<WINDOWPOS*>(lParam)->rc;
            pPos->flags = 0;
            PostMessageA(g_hMovieWnd, MOVIEMSG_WINDOWPOSCHANGED, wParam, reinterpret_cast<int64_t>(pPos));
        }
        break;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE && g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0 &&
            g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 1)
            g_pAppManager->m_pClientExoApp->CancelMovie();
        K2_ASSUME(g_pAppManager != 0);   // created before the first window message, freed after the last
        if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() == 0 &&
            HandleWMKeyDownMessage(hWnd, wParam, lParam) == 1)
            return 0;
        break;
    }

    case WM_CHAR: {
        if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() == 0 &&
            HandleWMCharMessage(static_cast<unsigned short>(wParam)) == 1)
            return 0;
        break;
    }

    case WM_SYSKEYDOWN: {
        int64_t nKey = static_cast<int64_t>(wParam);
        if (nKey < VK_F4) {
            if (nKey == VK_RETURN) {
                if (static_cast<int>(lParam) & KEYDATA_ALTDOWN)
                    return 0;
            } else if (nKey == VK_MENU) {
                return 0;
            }
        } else {
            if (nKey == VK_F10)
                return 0;
            if (nKey == VK_F4) {
                if (static_cast<int>(lParam) & KEYDATA_ALTDOWN)
                    g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);
                SendMessageA(hWnd, WM_CLOSE, VK_F4, lParam);
            }
        }
        break;
    }

    case WM_SYSCHAR: {
        if (wParam == VK_RETURN && (static_cast<int>(lParam) & KEYDATA_ALTDOWN))
            return 0;
        break;
    }

    case WM_SYSCOMMAND: {
        if ((wParam & SC_COMMAND_MASK) == SC_SIZE)
            return 0;
        if ((wParam & 0xffff) == SC_SCREENSAVE || (wParam & 0xffff) == SC_MONITORPOWER)
            return 0;
        break;
    }

    case WM_MOUSEMOVE: {
        if (g_pExoInput != 0 && g_pExoInput->GetActive() != 0) {
            event.message = nMessage;
            event.wParam = wParam;
            event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                               ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
            g_pExoInput->BufferEvent(&event);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            return 0;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_RBUTTONDOWN;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        SetCapture(hWnd);
        break;
    }

    case WM_LBUTTONUP: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            return 0;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_RBUTTONUP;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        break;
    }

    case WM_LBUTTONDBLCLK: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            return 0;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_RBUTTONDBLCLK;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        break;
    }

    case WM_RBUTTONDOWN: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            break;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_LBUTTONDOWN;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        SetCapture(hWnd);
        break;
    }

    case WM_RBUTTONUP: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            break;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_LBUTTONUP;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        break;
    }

    case WM_RBUTTONDBLCLK: {
        if (g_pExoInput == 0 || g_pExoInput->GetActive() == 0)
            return 0;
        event.message = nMessage;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0) {
            nMessage = WM_LBUTTONDBLCLK;
            event.message = nMessage;
        }
        event.wParam = wParam;
        event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                           ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
        g_pExoInput->BufferEvent(&event);
        SetCapture(hWnd);
        break;
    }

    case WM_MBUTTONDOWN: {
        if (g_pExoInput != 0 && g_pExoInput->GetActive() != 0) {
            event.message = nMessage;
            event.wParam = wParam;
            event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                               ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
            g_pExoInput->BufferEvent(&event);
            SetCapture(hWnd);
        }
        break;
    }

    case WM_MBUTTONUP: {
        if (g_pExoInput != 0 && g_pExoInput->GetActive() != 0) {
            event.message = nMessage;
            event.wParam = wParam;
            event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(lParam) |
                               ((g_nScreenHeight + ~static_cast<int>(lParam >> 16)) << 16));
            g_pExoInput->BufferEvent(&event);
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        if (g_pExoInput != 0 && g_pExoInput->GetActive() != 0) {
            POINT pt;
            pt.x = static_cast<int>(lParam & 0xffff);
            pt.y = static_cast<int>((lParam >> 16) & 0xffff);
            ScreenToClient(hWnd, &pt);
            event.message = nMessage;
            event.wParam = wParam;
            event.lParam = static_cast<unsigned int>(static_cast<unsigned short>(pt.x) | ((g_nScreenHeight + ~pt.y) << 16));
            g_pExoInput->BufferEvent(&event);
        }
        break;
    }
    }

    return DefWindowProcA(hWnd, nMessage, wParam, lParam);
}

// The black-screen overlay window's message handler, a stub that swallows everything.
long long BlackScreenWinMessageHandler(HWND__* hWnd, unsigned int uMsg, unsigned long long wParam,
                                       int64_t lParam)
{
    K2_UNUSED(hWnd);
    K2_UNUSED(uMsg);
    K2_UNUSED(wParam);
    K2_UNUSED(lParam);
    return 0;
}

// Set once the black-screen window class has been registered.
static bool s_bBlackScreenClassRegistered;

// Spawns the cinematic black-screen overlay. It installs the "KotorCin" class once, then creates a
// topmost 1600 x 1200 popup, blacks it out, maximises it and waits on the event handle it was
// given. The window itself is created with the class name "Knights Cinematic Backdrop".
int SpawnBlackScreen(void* pEvent)
{
    WNDCLASSA wc;
    WINDOWINFO info;

    if (!s_bBlackScreenClassRegistered) {
        wc.style = 0;
        wc.lpfnWndProc = reinterpret_cast<WNDPROC>(BlackScreenWinMessageHandler);
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInst;
        wc.hIcon = LoadIconA(g_hInst, 101);
        wc.hCursor = LoadCursorA(0, IDC_ARROW);
        wc.hbrBackground = 0;
        wc.lpszMenuName = 0;
        wc.lpszClassName = const_cast<char*>("KotorCin");
        if (RegisterClassA(&wc) == 0) {
            char* pszMessage;                         // deliberately uninitialised
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                           0, GetLastError(), LANG_USER_DEFAULT, &pszMessage, 0, 0);
            MessageBoxA(0, pszMessage, "Error", MB_ICONINFORMATION);
            LocalFree(pszMessage);
            return 0;
        }
        s_bBlackScreenClassRegistered = true;
    }

    info.cbSize = sizeof(WINDOWINFO);
    if (GetWindowInfo(g_hRenderWnd, &info) == 0)
        return 0;
    HWND__* hWnd = CreateWindowExA(WS_EX_TOPMOST,
                                   "Knights Cinematic Backdrop",
                                   "Knights Cinematic Backdrop",
                                   WS_POPUP | WS_VISIBLE,
                                   0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT, 0, 0, g_hInst, 0);
    SetWindowPos(hWnd, reinterpret_cast<HWND__*>(-1), 0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT,
                 SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSIZE);
    HDC__* hDC = GetDC(hWnd);
    PatBlt(hDC, 0, 0, BACKDROP_WIDTH, BACKDROP_HEIGHT, BLACKNESS);
    ReleaseDC(hWnd, hDC);
    ShowWindow_Win32(hWnd, SW_SHOWMAXIMIZED);
    while (WaitForSingleObject(pEvent, 1) == WAIT_TIMEOUT) {
    }
    DestroyWindow(hWnd);
    return 0;
}
#endif

// The GL shim and the aurora free functions this frame function needs.
void AurMakeCurrent();
void AurDisableHeadLight();

#if KOTOR1
extern SDL_Window* g_SDL_Window;
extern int g_bDisableSound;
#endif
// One frame. `bNoBackdrop` takes a short path that draws only the GUI manager; otherwise the
// backdrop is cleared with the head light off, the GUI manager is drawn, the console overlay is
// posted when the console is up, and the buffers are swapped. A non-zero `nMode` also pumps the
// server loop and the net layer's received-frame queue.
void UpdateScreen(float fDeltaTime, int nMode, int bNoBackdrop)
{
#if KOTOR1  // K1: the audio update runs here, SDL swaps the window, and there is no message pump
    if (!g_bDisableSound)
        g_pExoSound->m_pInternal->m_audioSystem.UpdateSystem();
    AurMakeCurrent();

    if (bNoBackdrop != 0) {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(fDeltaTime);
    } else {
        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying()) {
            ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            AurDisableHeadLight();
        }
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(fDeltaTime);

        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying()) {
            if (g_console.m_nActive != 0) {
                AurPostString(const_cast<char*>(">"), 0, 0, 5.0f);
                AurPostString(g_console.m_szLine, 2, 0, 5.0f);
            }
            AurRenderStrings();
            HDC__* hDC = GetDC(g_hRenderWnd);
            SDL_GL_SwapWindow(g_SDL_Window);
            ReleaseDC(g_hRenderWnd, hDC);
        }
    }

    if (nMode == 1 && g_pAppManager != 0) {
        if (g_pAppManager->m_pServerExoApp != 0) {
            g_pAppManager->m_pServerExoApp->MainLoop();
            if (g_pAppManager->m_pClientExoApp != 0)
                g_pAppManager->m_pClientExoApp->GetNetLayer()->ProcessReceivedFrames(0);
        }
        if (g_pAppManager->m_pClientExoApp != 0)
            g_pAppManager->m_pClientExoApp->GetNetLayer()->ProcessReceivedFrames(0);
    }

    if (g_pExoSound != 0)
        g_pExoSound->Render();

    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0) {
        if (g_pAppManager->m_pClientExoApp->GetNetLayer() != 0)
            g_pAppManager->m_pClientExoApp->GetNetLayer()->UpdateStatusLoop(
                CNetLayer::APPLICATION_CLIENT);
    }

    K2_ASSUME(g_pAppManager != 0);   // the frame loop runs between its creation and its delete
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.0f);
#else
    AurMakeCurrent();

    if (bNoBackdrop != 0) {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(fDeltaTime);
    } else {
        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying()) {
            ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            AurDisableHeadLight();
        }
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(fDeltaTime);

        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying()) {
            if (g_console.m_nActive != 0) {
                AurPostString(const_cast<char*>(">"), 0, 0, 5.0f);
                AurPostString(g_console.m_szLine, 2, 0, 5.0f);
            }
            AurRenderStrings();
            HDC__* hDC = GetDC(g_hRenderWnd);
            GLRender::SwapBuffers(hDC);
            ReleaseDC(g_hRenderWnd, hDC);
        }
    }

    messagepump();

    if (nMode == 1 && g_pAppManager != 0) {
        if (g_pAppManager->m_pServerExoApp != 0) {
            g_pAppManager->m_pServerExoApp->MainLoop();
            messagepump();
            if (g_pAppManager->m_pClientExoApp != 0)
                g_pAppManager->m_pClientExoApp->GetNetLayer()->ProcessReceivedFrames(0);
        }
        if (g_pAppManager->m_pClientExoApp != 0)
            g_pAppManager->m_pClientExoApp->GetNetLayer()->ProcessReceivedFrames(0);
    }

    if (g_pExoSound != 0)
        g_pExoSound->Render();

    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0) {
        if (g_pAppManager->m_pClientExoApp->GetNetLayer() != 0)
            g_pAppManager->m_pClientExoApp->GetNetLayer()->UpdateStatusLoop(
                CNetLayer::APPLICATION_CLIENT);
    }

    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    K2_ASSUME(g_pAppManager != 0);   // the frame loop runs between its creation and its delete
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->Draw(0.0f);
#endif
}

// The suspend/quicksave path.
extern char g_DuringSuspendSave;
#if !KOTOR1
char g_DuringSuspendSave;
#endif
int GetMiniGamePlayingType();

// When suspending into the background with a live game, this writes a "quicksave" save with
// g_DuringSuspendSave set, then runs the temp-save rename, which nukes the "quicksave" directory
// and then renames the resolved "tempsave" slot onto the resolved "quicksave" slot.
#if KOTOR1
extern char g_IosTakeScreenShot;   // SaveGame's screenshot gate
#endif
void GameEnteredBackground()
{
#if KOTOR1  // K1: a "BACKUPSAVE" with no screenshot, then the queued app events are re-posted
    if (g_pAppManager->m_pServerExoApp != 0) {
        void* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1);
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

        if (pPlayer != 0 && pCreature != 0 && pCreature->GetArea() != 0
            && GetMiniGamePlayingType() == 0
            && !g_pAppManager->m_pClientExoApp->IsMoviePlaying()
            && g_pAppManager->m_pClientExoApp->GetInGameGui() != 0
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface != 0
            && (g_pAppManager->m_pLoadGameErrorInfo == 0
                || g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent != CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME)) {
            g_IosTakeScreenShot = 0;
            g_pAppManager->m_pServerExoApp->SaveGame(0, CExoString("BACKUPSAVE"),
                                                     static_cast<CSWSPlayer*>(pPlayer), 0);
            g_IosTakeScreenShot = 1;
            g_pAppManager->m_pServerExoApp->MainLoop();

            K2_ASSUME(g_pAppManager->m_pLoadGameErrorInfo != 0);   // CAppManager's own, ctor to dtor
            g_pAppManager->m_pLoadGameErrorInfo->m_bStallFinished = 0;
            g_pAppManager->m_pLoadGameErrorInfo->m_flag = 0;
            g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;

            // Drain the app-lifecycle events the save let pile up, keeping the newest eight, then
            // re-post the last "did enter background" and the (up to) two events after it, unless
            // the newest one is a "will enter background".
            SDL_PumpEvents();
            SDL_Event* pEvents = new SDL_Event[8];
            int nEvents = SDL_PeepEvents(pEvents, 8, SDL_GETEVENT, SDL_APP_WILLENTERBACKGROUND, SDL_APP_DIDENTERFOREGROUND);
            while (nEvents == 8) {
                for (int i = 0; i < 4; i++)
                    pEvents[i] = pEvents[i + 4];
                nEvents = SDL_PeepEvents(pEvents + 4, 4, SDL_GETEVENT, SDL_APP_WILLENTERBACKGROUND, SDL_APP_DIDENTERFOREGROUND) + 4;
            }
            SDL_Event* pEvent = pEvents + nEvents;
            if (nEvents > 0 && pEvent[-1].type != SDL_APP_WILLENTERBACKGROUND) {
                for (int64_t i = nEvents; i > 0; i--, pEvent--) {
                    if (pEvent[-1].type == SDL_APP_DIDENTERBACKGROUND) {
                        SDL_PushEvent(pEvent - 1);
                        if (static_cast<int>(i) < nEvents)
                            SDL_PushEvent(pEvent);
                        if (static_cast<int>(i) + 1 < nEvents)
                            SDL_PushEvent(pEvent + 1);
                        break;
                    }
                }
            }
            operator delete[](pEvents);
        }
    }
#else
    if (g_pAppManager != 0 && g_pAppManager->m_pServerExoApp != 0) {
        void* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1);
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

        if (pPlayer != 0 && pCreature != 0 && pCreature->GetArea() != 0
            && GetMiniGamePlayingType() == 0
            && !g_pAppManager->m_pClientExoApp->IsMoviePlaying()
            && g_pAppManager->m_pClientExoApp->GetInGameGui() != 0
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface != 0
            && (g_pAppManager->m_pLoadGameErrorInfo == 0
                || g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent != CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME)
            && g_pAppManager->m_pClientExoApp->m_pInternal->m_pPlayerControl != 0) {
            g_DuringSuspendSave = 1;
            g_pAppManager->m_pServerExoApp->SaveGame(0, CExoString("QUICKSAVE"),
                                                     static_cast<CSWSPlayer*>(pPlayer), 0, 1);
            g_pAppManager->m_pServerExoApp->MainLoop();
            g_DuringSuspendSave = 0;

            K2_ASSUME(g_pAppManager->m_pLoadGameErrorInfo != 0);   // CAppManager's own, ctor to dtor
            g_pAppManager->m_pLoadGameErrorInfo->m_bStallFinished = 0;
            g_pAppManager->m_pLoadGameErrorInfo->m_flag = 0;
            g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;

            CExoString sSaveDir;
            {
                CExoString sAlias("SAVES:");
                sSaveDir = g_pExoBase->m_pAliasList->ResolveFileName(sAlias, RESTYPE_INVALID);
            }

            CExoString sTempSave;
            CExoString sQuickSaveDir;
            sTempSave.Format(const_cast<char*>("SAVES:%06d - %s"), 0, "tempsave");
            sQuickSaveDir.Format(const_cast<char*>("SAVES:%06d - %s"), 0, "quicksave");

            g_pExoResMan->NukeDirectory(sQuickSaveDir, 1, 1);
            sTempSave = g_pExoBase->m_pAliasList->ResolveFileName(sTempSave, RESTYPE_INVALID);
            sQuickSaveDir = g_pExoBase->m_pAliasList->ResolveFileName(sQuickSaveDir, RESTYPE_INVALID);
            static_cast<void>(rename(sTempSave.CStr(), sQuickSaveDir.CStr()));
        }
    }

    if (g_pExoSound != 0) {
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_LOSTFOCUS, 0);
        g_pExoSound->StopAllOneShots();
        g_pExoSound->StopDuckingMusicAndAmbientSounds();
    }
#endif
}

// The platform functions this file declares rather than includes.
extern "C" {
int   CoInitialize(void* pvReserved);
void  CoUninitialize();
#ifdef K2_QOL_FRAME_PACING
void  k2host_pace_frame(float fFps);   // platforms/shim/shim_time.cpp
#endif
}
void ASLPlat_SetWorkingDirectoryToResources();
char* dofile(char* pszFile);
char* FuncInterp(char* pszExpr);
void UpdateCusorVisible(float fElapsed);
void AurSetPauseOverlayColor(float fRed, float fGreen, float fBlue, float fAlpha);
void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBitsPerPixel,
                  int bFullscreen, int bAllow);

// The hooks and gauges the frame loop reads.
extern void (*AurRenderCallback)();   // per-frame hook Scene.cpp drives
extern float memoryfree;              // the fake memory gauge
int memoryused;
extern float displayFPS;              // reported by the client
extern float limitFPS;                // the frame-rate cap
extern bool  g_bFrameRateLocked;      // non-zero when the cap is active
extern char  mac_showFPS;             // non-zero when the FPS readout is on
extern bool  g_bIsInBackground;       // set while the app is suspended
char s_playIntoMovies;
extern int   g_bDisableSound;
extern int   g_bInScreenshotMode;
int g_bSwappingBuffers;
unsigned int mac_starttime;
unsigned int mac_endtime;
int g_nResetPriority;
int g_nPriorityClass;
static uint64_ll_t g_starttime;

#if !KOTOR1  // K1's SDL port has its own entry point
#ifndef K2_MOBILE_OPT
extern int GAME_OPTION_DEFAULT_SHADOWS;
extern int GAME_OPTION_DEFAULT_GRASS;
extern int GAME_OPTION_DEFAULT_FRAMEBUFFER;
#endif

// The port's entry point brings up the exo base and app manager, reads the sound and present
// options, then runs the per-frame tick until g_bExitProgram.
extern "C" int WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* pszCmdLine,
                       int nCmdShow)
{
    K2_UNUSED(hPrevInstance);
    K2_UNUSED(pszCmdLine);

    static int    s_framesSinceUpdate;
    static double s_displayFps;
    static uint64_ll_t s_fpsWaitStamp;
#ifndef K2_QOL_FRAME_PACING
    static uint64_ll_t s_fpsLimitStamp;
#endif

    ASLPlat_SetWorkingDirectoryToResources();

    mac_showFPS = ASL::GetPrefAsBool("MacShowFPS", 0);

    while (g_bIsInBackground != 0)
        SDL_Delay(10);

    GraphicsSettings::Initialize();
#ifndef K2_MOBILE_OPT
    // The PC defaults for a fresh swkotor2.ini; the APK set them per device (Shadows / Grass on
    // every device, Frame Buffer Effects on a Shield only).
    GAME_OPTION_DEFAULT_SHADOWS = 1;
    GAME_OPTION_DEFAULT_GRASS = 1;
    GAME_OPTION_DEFAULT_FRAMEBUFFER = 1;
#endif
    CoInitialize(0);

    MSG msg;
    PeekMessageA(&msg, 0, 0, 0, 0);

    dofile(const_cast<char*>("config.txt"));

    CExoBase* pExoBase = new CExoBase();
    g_pExoBase = pExoBase;
    pExoBase->LoadAliases(CExoString("swKotor2.ini"));

    g_pAppManager = new CAppManager();

    InitGameApp(hInstance, nCmdShow);
    StoreMouseTrailsState();
    StoreScreenSaverState();

    CExoIni ini;
    CExoString sValue;
    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Disable Sound")) != 0) {
        g_bDisableSound = sValue.AsINT() != 0;
    } else {
        s_playIntoMovies = 1;
        sValue.Format(const_cast<char*>("%d"), 0);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                          CExoString("Disable Sound"));
    }

    AurRenderCallback = messagepump;

    while (g_bIsInBackground != 0)
        SDL_Delay(10);

    if (g_pAppManager->m_pClientExoApp->StartServices(0, CExoString(""),
                                   CExoString(""), CExoString(""), 0) != 0)
        return 0;

    while (g_bIsInBackground != 0)
        SDL_Delay(10);

    g_pAppManager->m_pClientExoApp->Initialize();
    g_bExitProgram = 0;
    g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);

    dofile(const_cast<char*>("startup.txt"));

    AurSetPauseOverlayColor(0.0f, 0.0f, 0.0f, 0.0f);

    g_pAppManager->m_pClientExoApp->SetLoadStep(0, 10);
    g_pAppManager->m_pClientExoApp->SetLoadStep(1, 20);
    g_pAppManager->m_pClientExoApp->SetLoadStep(2, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(3, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(4, 23);

#ifdef K2_HOST_BUILD
    if (g_pK2HostViewer == 0)
#endif
    g_pAppManager->m_pClientExoApp->BeginIntro();

    CExoString sSaveDir;
    sSaveDir.Format(const_cast<char*>("SAVES:%06d - %s"), 0, "tempsave");
    g_pExoResMan->NukeDirectory(sSaveDir, 1, 1);

#ifdef K2_HOST_BUILD
    if (g_pK2HostViewer != 0)
        g_pK2HostViewer->Start();
    else
#endif
    g_pAppManager->m_pClientExoApp->DisplayMainMenu(1);

    bool bFirstFrame = true;
    while (g_bExitProgram == 0) {
        g_pExoSound->m_pInternal->m_audioSystem.UpdateSystem();

        memoryused = static_cast<int>(262144.0f - memoryfree);

        if (PeekMessageA(&msg, 0, 0, 0, 0) != 0) {
            if (GetMessageA(&msg, 0, 0, 0) == 0)
                break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            continue;
        }

        // The first-frame latch is cleared at the end of the body, which is why the store appears
        // in two blocks before the two loop exits.
        if (bFirstFrame)
            g_pExoInput->ClearEvents();

        if (g_nResetPriority != 0) {
            g_nResetPriority = 0;
            if (g_nPriorityClass == 2)
                SetPriorityClass(GetCurrentProcess_Win32(), REALTIME_PRIORITY_CLASS);
            if (g_nPriorityClass == 1)
                SetPriorityClass(GetCurrentProcess_Win32(), HIGH_PRIORITY_CLASS);
            else
                SetPriorityClass(GetCurrentProcess_Win32(), NORMAL_PRIORITY_CLASS);
        }

        if (g_nReInitializing != 0) {
            if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
                ReInitAurora(640, 480, 32, g_nReInitializing == 2, 0);
            else
                ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel,
                             g_nReInitializing == 2, 0);
        }

        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying())
            ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        AurDisableHeadLight();

        g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
#ifdef K2_HOST_BUILD
        if (g_pK2HostViewer != 0)
            g_bExitProgram |= g_pK2HostViewer->Frame();
        else
#endif
        g_bExitProgram |= g_pAppManager->m_pClientExoApp->MainLoop();
        g_ClientUpdateTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - g_starttime)
                             * 0.001f;

        g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        if (g_pAppManager->m_pServerExoApp != 0)
            g_bExitProgram |= g_pAppManager->m_pServerExoApp->MainLoop();
        g_ServerUpdateTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - g_starttime)
                             * 0.001f;

        UpdateCusorVisible(g_ClientUpdateTime);

        if (g_console.m_nActive != 0) {
            AurPostString(const_cast<char*>(">"), 0, 0, 5.0f);
            AurPostString(g_console.m_szLine, 2, 0, 5.0f);
        }
        if (g_console.m_bEnterPending != 0) {
            AurPostString(FuncInterp(g_console.m_szLine), 0, 1, 5.0f);
            g_console.m_szLine[0] = 0;
            g_console.m_nCursor = 0;
            g_console.m_bEnterPending = 0;
        }
        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying())
            AurRenderStrings();
        if (g_console.m_nActive != 0) {
            ASLgl::glColor4f(0.7f, 0.7f, 0.3f, 1.0f);
            AurPrintString(const_cast<char*>("_"), g_console.m_nCursor + 2, 0);
        }

        if (mac_showFPS != 0 && g_bInScreenshotMode == 0) {
            ++s_framesSinceUpdate;
            unsigned int nNow = timeGetTime();
            mac_endtime = nNow;
            unsigned int nElapsed = nNow - mac_starttime;
            if (nElapsed >= 201) {
                s_displayFps = 1.0 / static_cast<double>(nElapsed) * 1000.0 * static_cast<double>(s_framesSinceUpdate);
                mac_starttime = timeGetTime();
                s_framesSinceUpdate = 0;
            }
            char szFPS[256];
            sprintf_s(szFPS, sizeof(szFPS), "%.2f", s_displayFps);
            sprintf_s(szFPS, sizeof(szFPS), "%.2f %d x %d", s_displayFps, g_nScreenWidth, g_nScreenHeight);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            AurPrintString(szFPS, 4, 4);
        } else {
            ASLgl::glPushAttrib(0);
            ASLgl::glPopAttrib();
        }

        if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying() && g_nReInitializing == 0) {
            g_bSwappingBuffers = 1;
            HDC__* hDC = GetDC(g_hRenderWnd);
            GLRender::SwapBuffers(hDC);
            ReleaseDC(g_hRenderWnd, hDC);
            g_bSwappingBuffers = 0;
        }

        g_pAppManager->m_pClientExoApp->EnableMouseAtEndOfLoadModuleMovie();

        if (displayFPS > 0.0f) {
            float fFrameTime = 1000.0f / displayFPS;
            while (static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - s_fpsWaitStamp)
                       * 0.001f < fFrameTime) {
            }
            s_fpsWaitStamp = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        }

        bFirstFrame = false;

#ifdef K2_QOL_FRAME_PACING
        k2host_pace_frame(limitFPS > 0.0f && g_bFrameRateLocked != 0 ? limitFPS : 0.0f);
#else
        if (limitFPS > 0.0f && g_bFrameRateLocked != 0) {
            float fFrameTime = 1000.0f / limitFPS;
            float fElapsed = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer()
                                     - s_fpsLimitStamp) * 0.001f;
            if (fElapsed < fFrameTime)
                SDL_Delay(static_cast<unsigned int>(fFrameTime - fElapsed));
            s_fpsLimitStamp = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        }
#endif
    }

    g_pAppManager->DestroyServer();
    g_pAppManager->m_pClientExoApp->Uninitialize();
    g_pAppManager->m_pClientExoApp->StopServices();

    RestoreMouseTrailsState();
    RestoreScreenSaverState();

    delete g_pAppManager;
    g_pAppManager = 0;
    delete g_pExoBase;
    g_pExoBase = 0;

    CoUninitialize();

    return static_cast<int>(msg.wParam);
}
#endif

#if KOTOR1
// K1's SDL frame entry points: SDL_main owns the loop and calls these in place of WinMain's
// phases.
static uint64_ll_t s_fpsWaitStamp;
float g_GameLoopTime;                        // last frame's total time in ms
void MacCreateBinkShaders();

void GameInit()
{
    dofile(const_cast<char*>("config.txt"));

    CExoBase* pExoBase = new CExoBase();
    g_pExoBase = pExoBase;
    pExoBase->LoadAliases(CExoString("swKotor.ini"));

    g_pAppManager = new CAppManager();

    InitGameApp(static_cast<void*>(0), 0);

    CExoIni ini;
    CExoString sValue;
    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Disable Sound")) != 0) {
        g_bDisableSound = sValue.AsINT() != 0;
    } else {
        s_playIntoMovies = 1;
        sValue.Format(const_cast<char*>("%d"), 0);
        ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                          CExoString("Disable Sound"));
    }

    AurRenderCallback = messagepump;

    if (g_pAppManager->m_pClientExoApp->StartServices(0, CExoString(""),
                                   CExoString(""), CExoString(""), 0) != 0)
        return;

    g_pAppManager->m_pClientExoApp->Initialize();
    g_bExitProgram = 0;
    g_pAppManager->m_pClientExoApp->SetInputActive(1, 0);

    dofile(const_cast<char*>("startup.txt"));

    AurSetPauseOverlayColor(0.0f, 0.0f, 0.0f, 0.0f);

    g_pAppManager->m_pClientExoApp->SetLoadStep(0, 10);
    g_pAppManager->m_pClientExoApp->SetLoadStep(1, 20);
    g_pAppManager->m_pClientExoApp->SetLoadStep(2, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(3, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(4, 23);

#ifdef K2_HOST_BUILD
    if (g_pK2HostViewer != 0)
        g_pK2HostViewer->Start();
    else
#endif
    {
        if (s_playIntoMovies)
            g_pAppManager->m_pClientExoApp->BeginIntro();
        g_pAppManager->m_pClientExoApp->DisplayMainMenu(1);
    }

    if (g_nReInitializing != 0)
        ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel, g_nReInitializing == 2, 0);
}

// Runs one frame, the WinMain loop body without the message pump, the cursor, the FPS readout or
// the swap, plus the frame-time stamp.
void GameUpdate()
{
    uint64_ll_t nFrameStart = g_pExoBase->m_pTimers->GetHighResolutionTimer();

    memoryused = static_cast<int>(262144.0f - memoryfree);

    if (g_nResetPriority != 0) {
        g_nResetPriority = 0;
        if (g_nPriorityClass == 2)
            SetPriorityClass(GetCurrentProcess_Win32(), REALTIME_PRIORITY_CLASS);
        if (g_nPriorityClass == 1)
            SetPriorityClass(GetCurrentProcess_Win32(), HIGH_PRIORITY_CLASS);
        else
            SetPriorityClass(GetCurrentProcess_Win32(), NORMAL_PRIORITY_CLASS);
    }

    if (g_nReInitializing != 0) {
        if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
            ReInitAurora(640, 480, 32, g_nReInitializing == 2, 0);
        else
            ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel,
                         g_nReInitializing == 2, 0);
    }

    if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying())
        ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    AurDisableHeadLight();

    g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
#ifdef K2_HOST_BUILD
    if (g_pK2HostViewer != 0)
        g_bExitProgram |= g_pK2HostViewer->Frame();
    else
#endif
    g_bExitProgram |= g_pAppManager->m_pClientExoApp->MainLoop();
    g_ClientUpdateTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - g_starttime)
                         * 0.001f;

    g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    if (g_pAppManager->m_pServerExoApp != 0)
        g_bExitProgram |= g_pAppManager->m_pServerExoApp->MainLoop();
    g_ServerUpdateTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - g_starttime)
                         * 0.001f;

    if (g_console.m_nActive != 0) {
        AurPostString(const_cast<char*>(">"), 0, 0, 5.0f);
        AurPostString(g_console.m_szLine, 2, 0, 5.0f);
    }
    if (g_console.m_bEnterPending != 0) {
        AurPostString(FuncInterp(g_console.m_szLine), 0, 1, 5.0f);
        g_console.m_szLine[0] = 0;
        g_console.m_nCursor = 0;
        g_console.m_bEnterPending = 0;
    }
    if (!g_pAppManager->m_pClientExoApp->IsMoviePlaying())
        AurRenderStrings();
    if (g_console.m_nActive != 0) {
        ASLgl::glColor4f(0.7f, 0.7f, 0.3f, 1.0f);
        AurPrintString(const_cast<char*>("_"), g_console.m_nCursor + 2, 0);
    }

    g_pAppManager->m_pClientExoApp->EnableMouseAtEndOfLoadModuleMovie();

    if (displayFPS > 0.0f) {
        float fFrameTime = 1000.0f / displayFPS;
        float fElapsed = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - s_fpsWaitStamp)
                         * 0.001f;
        uint64_ll_t nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        while (fElapsed < fFrameTime) {
            fElapsed = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - s_fpsWaitStamp)
                       * 0.001f;
            nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        }
        s_fpsWaitStamp = nNow;
    }

    MacCreateBinkShaders();

    g_GameLoopTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - nFrameStart) * 0.001f;
}

void GameDeinit()
{
    g_pAppManager->DestroyServer();
    g_pAppManager->m_pClientExoApp->Uninitialize();
    g_pAppManager->m_pClientExoApp->StopServices();

    delete g_pAppManager;
    g_pAppManager = 0;
    delete g_pExoBase;
    g_pExoBase = 0;
}
#endif

// The native-resolution scaler's screen-size globals.
extern int g_nOriginalScreenWidth;
extern int g_nOriginalScreenHeight;

// Snaps the render target to the game's 4:3-ish (1.33) aspect and publishes the letterbox bars.
void ScaleNativeResolution()
{
    g_nScaledNativeWidth = g_nScreenWidth;
    g_nScaledNativeHeight = g_nScreenHeight;
    g_nOriginalScreenWidth = g_nScreenWidth;
    g_nOriginalScreenHeight = g_nScreenHeight;

    int nScaledWidth;
    int nScaledHeight;

    if (g_nScreenWidth > g_nScreenHeight)
    {
        int nHeight = static_cast<int>(static_cast<float>(g_nScreenHeight) * 1.33f);
        if (g_nScreenWidth >= nHeight)
        {
            nScaledWidth = nHeight;
            nScaledHeight = g_nScreenHeight;
        }
        else
        {
            nScaledWidth = g_nScreenWidth;
            nScaledHeight = static_cast<int>(static_cast<float>(g_nScreenWidth) / 1.33f);
        }
    }
    else
    {
        int nWidth = static_cast<int>(static_cast<float>(g_nScreenWidth) * 1.33f);
        if (g_nScreenHeight >= nWidth)
        {
            nScaledWidth = g_nScreenWidth;
            nScaledHeight = nWidth;
        }
        else
        {
            nScaledWidth = static_cast<int>(static_cast<float>(g_nScreenHeight) / 1.33f);
            nScaledHeight = g_nScreenHeight;
        }
    }

    g_nScaledNativeWidth = nScaledWidth;
    g_nScaledNativeHeight = nScaledHeight;

    g_nScreenLeftOffset = (g_nScreenWidth - nScaledWidth) / 2;
    g_nScreenTopOffset = (g_nScreenHeight - nScaledHeight) / 2;
}

// The GL entry points this file calls.

// Set when the ARB program state has to be rebuilt.
extern bool gMac_ShadersNeedInit;

// Presents the frame, then resets the port's vertex and fragment program state: the viewport goes
// back to the current screen size, both ARB program targets are enabled and bound to the cached
// program ids (which nothing ever sets, so both stay 0), a three-vertex glBegin(GL_POINTS) run is
// flushed, and both are disabled again.
inline void GLRender::SwapBuffers(HDC__* hDC)
{
    static unsigned int simpleVS;
    static unsigned int simplePS;

    ::SwapBuffers(hDC);

    gMac_ShadersNeedInit = 0;

    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, simpleVS);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, simplePS);

    ASLgl::glBegin(0);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glEnd();

    ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
}
