#include "k2stdint.h"
#include "exosound.h"
#include "nwcambientsound.h"
#include "swgui.h"
#include "nwcmodule.h"
#include "nwchandlemessage.h"
#include "nwcarea.h"
#ifndef CCLIENTOPTIONSINTERNAL_H
#define CCLIENTOPTIONSINTERNAL_H


#include "clientoptions.h"
#include "clientexoapp.h"
#include "partytables.h"
#include "serverexoapp.h"
#include "appmanager.h"

#include "exoiniinternal.h"
#include "globalvartables.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwccreatureappearance.h"
#include "nwscreature.h"

extern CSWGuiManager* g_pGuiMan;

#endif
#include "nwsrules.h"
#include "twodimarrays.h"
#include "gameobjectarray.h"
#include "enginevfx.h"
#include "nwscriptconst.h"

int GAME_OPTION_DEFAULT_SHADOWS;
int GAME_OPTION_DEFAULT_GRASS;
int GAME_OPTION_DEFAULT_FRAMEBUFFER;

#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "nwcanimbase.h"
#include "exostringinline.h"


// Aurora renderer feature toggles. Each graphics setter stores the option and then calls the
// matching enable or disable hook.
extern void AurEnableShadows();
extern void AurDisableShadows();
extern void AurEnableSoftShadows();
extern void AurDisableSoftShadows();
extern void AurEnableGrass();
extern void AurDisableGrass();
extern void AurEnableTexControllers();
extern void AurDisableTexControllers();
extern int AurSetAnisotropy(int n);
extern void AurSetVSyncState(bool bEnabled);
extern int AurSetAntiAliasingMode(int n);
extern void AurATIUnlockSoftShadowsForHotFix();
extern void AurEnableFrameBufferEffects();
extern void AurDisableFrameBufferEffects();
extern bool  AurCheckFrameBufferModificationsAvailable();
#if KOTOR1
bool GetHighResolution();
#endif

// Camera-keyboard tuning overrides. A value of -1.0 means unset: the getter returns the per-options
// field and the setter leaves the global alone.
float KeyboardCameraRotationSpeed = -1.0f;
float KeyboardCameraRotationAccl = -1.0f;
float KeyboardCameraRotationDecel = -1.0f;

extern unsigned char g_ForceSight;

float creaturewindintensity = 2.5f;

extern bool g_bFrameRateLocked;

extern int g_nReInitializing;

extern CExoSound* g_pExoSound;

CClientOptions::CClientOptions()
{
    m_bAlwaysRun = 0;
    m_bCameraShakeEnabled = 1;
    m_bFullScreen = 0;
    m_bShowTimingEventBar = 1;
    m_bReserved1 = 1;
    m_nMinSpecMode = 16;
    m_fMouseWheelSpeedMult = 3.0f;
    m_fCameraTurnMult = 1.0f;
    m_fToolTipTimeDelay = 1.0f;
    m_nHideFadeGeometryMode = 0;
    m_nShadowDetail = 2;
    m_nCameraMode = CAMERA_MODE_ON_A_STICK;
    m_nSavedCameraMode = CAMERA_MODE_TOP_DOWN;
    m_nNumShadowLights = 3;
    m_nReserved4 = 2;
    m_nCreatureWindMode = 1;
    m_nViolenceLevel = 2;
    m_nFontSize = 2;
    m_fReserved1 = 5.0f;
    if (g_pGuiMan)
        g_pGuiMan->m_fToolTipTimeDelay = 1.0f;
#if KOTOR1
    m_bEnableToolTips = 0;
    m_bMouseTeleportToButtons = 1;
#else
    m_bEnableToolTips = 1;
    m_bMouseTeleportToButtons = 1;
    m_nCurrentSithLord = 0;
    m_nGamesCompleted = 0;
    m_nPCGender = 0;
    m_nAppearanceType = 0;
    m_nBodyVariation = 0;
    m_nTextureVariation = 0;
#endif
    SetDefaultOptions();
}

// Resets every option to its shipped default. The graphics defaults must stay in this order:
// shadows, screen resolution, force speed, grass.
void CClientOptions::SetDefaultOptions()
{
    SetDefaultGameplayOptions();
    SetDefaultFeedbackOptions();
    SetDefaultAutopauseOptions();
    SetDefaultGamma();
    SetDefaultGraphicsOptions();
    SetDefaultAdvancedGraphicsOptions();
    SetDefaultSoundOptions();
    SetDefaultMouseSen();
    SetDefaultMouseOptions();
    SetDefaultScreenShotOptions();
#if KOTOR1
    SetDefaultGUIsInScreenShotOptions();
#endif
    SetDefaultKeyboardCamera();
    __builtin_memset(m_movieShown, 0, sizeof(m_movieShown));
    m_bUnlimitedItemUses = 0;
    m_bEnableCheats = 0;
    m_bTurboSpeed = 0;
}

CClientOptions::~CClientOptions()
{
}

void CClientOptions::ApplySoundOptions()
{
    if (g_pExoSound) {
        g_pExoSound->SetMusicVolume(m_nMusicVolume / 100.0f, 0);
        g_pExoSound->SetDialogVolume(m_nVoiceVolume / 100.0f);
        g_pExoSound->SetSoundEffectVolume(m_nSoundEffectsVolume / 100.0f);
    }
}

void CClientOptions::SetShadows(unsigned int b)
{
    m_bShadows = b;
    if (b) AurEnableShadows();
    else AurDisableShadows();
}

void CClientOptions::SetSoftShadows(unsigned int b)
{
    m_bSoftShadows = b;
    if (b) AurEnableSoftShadows();
    else AurDisableSoftShadows();
}

void CClientOptions::SetGrass(unsigned int b)
{
    m_bGrass = b;
    if (b) AurEnableGrass();
    else AurDisableGrass();
}

void CClientOptions::SetTexCon(unsigned int b)
{
    m_bTextureContrast = b;
    if (b) AurEnableTexControllers();
    else AurDisableTexControllers();
}

void CClientOptions::SetAnisotropy(unsigned int n)
{
    m_nAnisotropy = n;
    AurSetAnisotropy(n);
}

void CClientOptions::SetVSync(unsigned int b)
{
    m_bVSync = b;
    AurSetVSyncState(b != 0);
}

void CClientOptions::SetAntiAlias(unsigned char n)
{
    m_nAntiAlias = n;
    AurSetAntiAliasingMode(n);
}

// The mode guard is a switch rather than an if, so the whole case group shares one compare.
void CClientOptions::SetCameraMode(unsigned char mode)
{
    m_nCameraMode = mode;
#if KOTOR1  // K1 has no Force Sight camera mode to clear
    if (mode < 2)
        m_nSavedCameraMode = mode;
#else
    switch (mode) {
    case CAMERA_MODE_CHASE_CAMERA:
    case CAMERA_MODE_TOP_DOWN:
        m_nSavedCameraMode = mode;
        return;
    case CAMERA_MODE_DIALOG:
        g_ForceSight = 0;
        return;
    }
#endif
}

void CClientOptions::ToggleCameraMode()
{
    if (m_nCameraMode == CAMERA_MODE_STIFF_CHASE_CAMERA) {
        m_nCameraMode = m_nSavedCameraMode;
#if !KOTOR1
        if (m_nSavedCameraMode == CAMERA_MODE_DIALOG)
            g_ForceSight = 0;
#endif
    } else {
        m_nCameraMode = CAMERA_MODE_STIFF_CHASE_CAMERA;
    }
}

void CClientOptions::SetActionFeedbackMode(unsigned char n)
{
    K2_UNUSED(n);
}

void CClientOptions::SetDefaultMouseSen()
{
    m_nMouseSensitivity = 44;
}

void CClientOptions::SetDefaultMouseOptions()
{
    m_bReverseMouseButtons = 0;
    m_nMouseSensitivity = 44;
}

#if KOTOR1
void SetHighResolution(bool bHigh);
#endif
void CClientOptions::SetScreenRes(unsigned char n)
{
    m_nScreenRes = n;
#if KOTOR1  // K1 tells the Android activity when the high, index 2, resolution is chosen
    SetHighResolution(n == 2);
#endif
}

void CClientOptions::SetDefaultScreenShotOptions()
{
    m_bScreenShotEnabled = 0;
}

#if KOTOR1
void CClientOptions::SetDefaultGUIsInScreenShotOptions()
{
    m_bGUIsInScreenShot = 1;
}
#endif

void CClientOptions::SetDefaultGamma()
{
    m_nGammaIndex = 57;
    m_fGammaMin = 0.23f;
    m_fGammaMax = 2.0f;
}

float CClientOptions::GetGammaSetting()
{
#if KOTOR1  // K1: the raw index, no clamp and no +15
    return m_fGammaMax + (m_fGammaMax - m_fGammaMin) * (m_nGammaIndex / -100.0f);
#else
    int n = (m_nGammaIndex < 85) ? m_nGammaIndex : 85;
    n += 15;
    return m_fGammaMax + (m_fGammaMax - m_fGammaMin) * (n / -100.0f);
#endif
}

float CClientOptions::GetMouseSenSetting()
{
    return m_nMouseSensitivity / 100.0f * 45.0f + 10.0f;
}

float CClientOptions::GetCameraKeyboardDPS()
{
    if (KeyboardCameraRotationSpeed != -1.0f)
        return KeyboardCameraRotationSpeed;
    return m_fCameraKeyboardDPS;
}

float CClientOptions::GetCameraKeyboardAcceleration()
{
    if (KeyboardCameraRotationAccl != -1.0f)
        return KeyboardCameraRotationAccl;
    return m_fCameraKeyboardAcceleration;
}

float CClientOptions::GetCameraKeyboardDeceleration()
{
    if (KeyboardCameraRotationDecel != -1.0f)
        return KeyboardCameraRotationDecel;
    return m_fCameraKeyboardDeceleration;
}

void CClientOptions::SetCameraKeyboardDPS(float f)
{
    if (KeyboardCameraRotationSpeed != -1.0f)
        KeyboardCameraRotationSpeed = f;
    m_fCameraKeyboardDPS = f;
}

void CClientOptions::SetCameraKeyboardAcceleration(float f)
{
    if (KeyboardCameraRotationAccl != -1.0f)
        KeyboardCameraRotationAccl = f;
    m_fCameraKeyboardAcceleration = f;
}

void CClientOptions::SetCameraKeyboardDeceleration(float f)
{
    if (KeyboardCameraRotationDecel != -1.0f)
        KeyboardCameraRotationDecel = f;
    m_fCameraKeyboardDeceleration = f;
}

void CClientOptions::SetFullScreenEnabled(int b)
{
    if (static_cast<int>(m_bFullScreen) != b) {
        g_nReInitializing = (b != 0) ? 2 : 1;
        m_bFullScreen = b;
    }
}

void CClientOptions::SetNumShadowLights(unsigned char n)
{
    if (m_nNumShadowLights != n) {
        m_nNumShadowLights = n;
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0 && pModule->m_pArea != 0)
            pModule->m_pArea->m_bShadowLightsChanged = 1;
    }
}

void CClientOptions::SetHideFadeGeometryMode(unsigned char mode)
{
    if (m_nHideFadeGeometryMode != mode) {
        if (g_pAppManager->m_pClientExoApp->GetModule() != 0 &&
            g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0)
            g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->SetHideFadeGeometryMode(mode);
        m_nHideFadeGeometryMode = mode;
    }
}

#ifndef K2_UI_TOUCH
extern "C" int k2shim_cursor_enable(int bWanted, int bKotor1);   // platforms/shim/shim_cursor.cpp
#endif

void CClientOptions::SetEnableHardwareMouseCursor(int b)
{
#ifndef K2_UI_TOUCH
    // The PC art's .cur files when the platform layer finds them, otherwise the software cursor:
    // gui_mouse plus the gui_mp_* textures.
#if KOTOR1
    b = k2shim_cursor_enable(b, 1);
#else
    b = k2shim_cursor_enable(b, 0);
#endif
#endif
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0) {
        g_pAppManager->m_pClientExoApp->SetMouseMode(b == 0);
        m_bHardwareMouseCursor = b;
    }
}

void CClientOptions::SetAlwaysRun(int b)
{
    if (m_bAlwaysRun != static_cast<unsigned>(b)) {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg != 0 && pMsg->SendPlayerToServerInput_AlwaysRun(b) != 0)
            m_bAlwaysRun = b;
    }
}

void CClientOptions::SetAutoLevelUpNPCs(int b)
{
    m_bAutoLevelUpNPCs = b;
    if ((b & 1) != 0 && g_pAppManager->m_pServerExoApp != 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->AutoLevelUpNPCs();
}

void CClientOptions::SetDefaultSoundOptions()
{
    m_nMusicVolume        = 85;
    m_nVoiceVolume        = 85;
    m_nSoundEffectsVolume = 85;
#if KOTOR1
    m_nMovieVolume        = 100;
#else
    m_nMovieVolume        = 85;
#endif
}

void CClientOptions::SetTexQual(unsigned char n)
{
    m_nTextureQuality = n;
    g_pAppManager->m_pClientExoApp->SetTexturePack(n);
}

void CClientOptions::SetToolTipTimeDelay(float f)
{
    m_fToolTipTimeDelay = f;
    if (g_pGuiMan)
        g_pGuiMan->m_fToolTipTimeDelay = f;
}

int GAME_OPTION_DEFAULT_HIDEINGAMEGUI;
extern int GAME_OPTION_DEFAULT_FRAMEBUFFER;
extern int GAME_OPTION_DEFAULT_SHADOWS;
int GAME_OPTION_DEFAULT_SCREENRES = 1;
extern int GAME_OPTION_DEFAULT_GRASS;

extern bool g_allowForceSpeedEffect;

#include "GraphicsSettings.h"

// Turns frame-buffer effects on or off and mirrors the change onto every live party member: a
// creature carrying the other effect is switched over by delete and re-add. The loop counter is a
// byte, and the bound is re-read every iteration because GetCharacter can change the party.
void CClientOptions::SetFrameBuffer(unsigned int n)
{
#if KOTOR1
    // K1 makes the party lookup but performs no effect swap.
#ifdef K2_QOL_K1_DISTORTION
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
#else
    g_pAppManager->m_pClientExoApp->GetSWParty();
#endif
    m_nFrameBuffer = n;
    if (n)
        AurEnableFrameBufferEffects();
    else
        AurDisableFrameBufferEffects();
#ifdef K2_QOL_K1_DISTORTION
    // A stealthed party member trades the stealth shell for the distortion effect and back, as
    // SetStealthState would pick it now.
    if (pParty != 0 && (AurCheckFrameBufferModificationsAvailable() & 1) != 0) {
        unsigned short nFrom = n ? VFX_DUR_STEALTH_FIELD : VFX_DUR_DISTORTION;
        unsigned short nTo = n ? VFX_DUR_DISTORTION : VFX_DUR_STEALTH_FIELD;
        for (unsigned char i = 0; i < pParty->m_nCharacterCount; i++) {
            CSWCCreature* pChar = pParty->GetCharacter(i);
            if (pChar != 0 && pChar->HasVisualEffectApplied(nFrom)) {
                pChar->DeleteVisualEffect(nFrom);
                Vector vZero = {};
                pChar->AddVisualEffect(nTo, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
            }
        }
    }
#endif
#elif defined(K2_QOL_FRAME_BUFFER_EFFECTS)
    // The stealth visuals stay the stock pair, which never asks about the switch, so a swap here
    // would leave a shell that leaving stealth does not remove.
    g_pAppManager->m_pClientExoApp->GetSWParty();
    m_nFrameBuffer = n;
    if (n)
        AurEnableFrameBufferEffects();
    else
        AurDisableFrameBufferEffects();
#else
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    m_nFrameBuffer = n;
    if (n) {
        AurEnableFrameBufferEffects();
        if (pParty != 0) {
            for (unsigned char i = 0; i < pParty->m_nCharacterCount; i++) {
                CSWCCreature* pChar = pParty->GetCharacter(i);
                if (pChar != 0 && pChar->HasVisualEffectApplied(VFX_DUR_STEALTH_FIELD)
                    && (AurCheckFrameBufferModificationsAvailable() & 1) != 0) {
                    pChar->DeleteVisualEffect(VFX_DUR_STEALTH_FIELD);
                    Vector vZero = {};
                    pChar->AddVisualEffect(VFX_DUR_DISTORTION, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
                }
            }
        }
    } else {
        AurDisableFrameBufferEffects();
        if (pParty != 0) {
            for (unsigned char i = 0; i < pParty->m_nCharacterCount; i++) {
                CSWCCreature* pChar = pParty->GetCharacter(i);
                if (pChar != 0 && pChar->HasVisualEffectApplied(VFX_DUR_DISTORTION)
                    && (AurCheckFrameBufferModificationsAvailable() & 1) != 0) {
                    pChar->DeleteVisualEffect(VFX_DUR_DISTORTION);
                    Vector vZero = {};
                    pChar->AddVisualEffect(VFX_DUR_STEALTH_FIELD, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
                }
            }
        }
    }
#endif
}

void CClientOptions::SetUseSmallFonts(int b)
{
    m_bUseSmallFonts = b;
    m_nFontSize = b ? 1 : 2;
    if (g_pGuiMan)
        g_pGuiMan->UpdateAllFonts();
}

void CClientOptions::SetDefaultFeedbackOptions()
{
    m_bFeedback0     = 0;
    m_bTutorialWindows     = 1;
    m_bFeedback2     = 1;
    m_bFeedback3     = 1;
    m_bFeedback4     = 1;
    m_bFeedback5     = 1;
    m_bFeedback6     = 0;
    m_bFeedback7     = 1;
    m_bUseSmallFonts = 0;
    m_bHideInGameGui = GAME_OPTION_DEFAULT_HIDEINGAMEGUI;
    m_bEnableToolTips    = 0;
    m_nFontSize      = 2;
    if (g_pGuiMan)
        g_pGuiMan->UpdateAllFonts();
}

void CClientOptions::SetDefaultAutopauseOptions()
{
    m_bAutoPauseEndOfCombatRound = 0;
    m_bAutoPauseEnemySighted = 1;
#if KOTOR1  // K1 also pauses on the second option by default
    m_bAutoPauseMineSighted = 1;
#else
    m_bAutoPauseMineSighted = 0;
#endif
    m_bAutoPausePartyKilled = 1;
    m_bAutoPauseActionMenu = 0;
    m_bAutoPauseNewTarget = 1;
}

// EFFECTIVE: same instructions, scheduler tie-break in the four graphics default assignments.
void CClientOptions::SetDefaultGraphicsOptions()
{
    m_nGammaIndex = 57;
    m_fGammaMin = 0.23f;
    m_fGammaMax = 2.0f;
#if KOTOR1  // K1 only stores the defaults: no quality level, speed blur, or grass/shadow toggles
    m_bShadows = GAME_OPTION_DEFAULT_SHADOWS;
    m_bGrass = GAME_OPTION_DEFAULT_GRASS;
    m_nScreenRes = static_cast<unsigned char>(GAME_OPTION_DEFAULT_SCREENRES);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_bGrass) AurEnableGrass(); else AurDisableGrass();
#endif
#else
    m_bShadows = GraphicsSettings::GetQualityLevel() ? GAME_OPTION_DEFAULT_SHADOWS : 0;
    m_nScreenRes = static_cast<unsigned char>(GAME_OPTION_DEFAULT_SCREENRES);
    g_allowForceSpeedEffect = 1;
    m_bGrass = GAME_OPTION_DEFAULT_GRASS;
    if (m_bGrass) AurEnableGrass(); else AurDisableGrass();
    if (m_bShadows) AurEnableShadows(); else AurDisableShadows();
#endif
}

void CClientOptions::SetDefaultAdvancedSoundOptions()
{
    // The EAX level is clamped by evaluating the query twice, as a min macro does.
#if KOTOR1
    int nEAX = g_pExoSound->GetBestEAXAvailable();
    m_nEAXLevel = static_cast<unsigned char>(nEAX < 3 ? nEAX : 3);
#else
    m_nEAXLevel = static_cast<unsigned char>(g_pExoSound->GetBestEAXAvailable() > 3 ? 3 : g_pExoSound->GetBestEAXAvailable());
#endif
    m_bForceSoftware = 0;
}

#if KOTOR1
extern int doframebuffer;           // K1's renderer takes the defaults writes
extern int preferPerfToVisual;
#if KOTOR1
int preferPerfToVisual;
#endif
extern unsigned char g_is_antialias_on;
#endif

void CClientOptions::SetDefaultAdvancedGraphicsOptions()
{
#if KOTOR1  // K1 defaults: anisotropy 1, low texture quality, no vsync or anti-aliasing
    m_nFrameBuffer     = GAME_OPTION_DEFAULT_FRAMEBUFFER;
    m_nAnisotropy      = 1;
    m_bTextureContrast = 1;
    m_nTextureQuality  = 0;
    m_bVSync           = 0;
    m_nAntiAlias       = 0;
    m_bSoftShadows     = 0;
    doframebuffer = 1;
    preferPerfToVisual = m_nFrameBuffer == 0;
    g_is_antialias_on = 0;
#else
    m_nFrameBuffer     = GAME_OPTION_DEFAULT_FRAMEBUFFER;
    m_nAnisotropy      = 4;
    m_bTextureContrast = 1;
    m_nTextureQuality  = 2;
    m_bVSync           = 1;
    m_nAntiAlias       = GraphicsSettings::GetQualityLevel() ? 4 : 0;
    m_bSoftShadows     = 0;
#endif
}

void CClientOptions::SetAllMoviesShown()
{
    __builtin_memset(m_movieShown, -1, sizeof(m_movieShown));
}

void CClientOptions::SetDefaultKeyboardCamera()
{
    if (KeyboardCameraRotationSpeed != -1.0f)
        KeyboardCameraRotationSpeed = 200.0f;
    m_fCameraKeyboardDPS = 200.0f;
    if (KeyboardCameraRotationAccl != -1.0f)
        KeyboardCameraRotationAccl = 500.0f;
    m_fCameraKeyboardAcceleration = 500.0f;
    if (KeyboardCameraRotationDecel != -1.0f)
        KeyboardCameraRotationDecel = 2000.0f;
    m_fCameraKeyboardDeceleration = 2000.0f;
}

void CClientOptions::SetDefaultGameplayOptions()
{
    m_difficultyLevel            = 1;
    m_bAutoLevelUpNPCs = 0;
    m_bGameplay1       = 0;
    m_bAutoSave        = 1;
#if KOTOR1
    m_bGameplay3       = 0;
    m_bGameplay5       = 1;
    m_bAutoRun         = 0;
#else
    m_bGameplay3       = 0;
    m_bGameplay4       = 0;
    m_bGameplay5       = 1;
    m_nCurrentSithLord = 0;
    m_nGamesCompleted  = 0;
    m_nPCGender        = 0;
    m_nAppearanceType  = 0;
    m_nBodyVariation   = 0;
    m_nTextureVariation = 0;
    m_bAutoRun            = 0;
#endif
}


// Pending movie-unlock name, consumed elsewhere.
CExoString g_sUnlockedMovie;

// Forwards as a string.
void CClientOptions::SetMovieShown(const CResRef& resRef, int bShown)
{
    CExoString sName;
    resRef.CopyToString(sName);
    SetMovieShown(sName, bShown);
}

// Marks or clears one movie's bit in m_movieShown, then persists the options. The name usually
// arrives as a full path, so when the "Movies" 2DA has no row for it the basename between the last
// '\\' and the last '.' is retried: the 2DA lists "movie" where the caller passes "movie.bik".
void CClientOptions::SetMovieShown(const CExoString& sName, int bShown)
{
    if (g_pRules == 0 || g_pRules->m_p2DArrays == 0)
        return;
    int nRow = g_pRules->m_p2DArrays->m_pMovies->GetRowIndex(sName);
    if (nRow < 0) {
        const char* pszDot = strrchr(sName.CStr(), '.');
        const char* pszSlash = strrchr(sName.CStr(), '\\');
        if (pszDot == 0 || pszSlash == 0)
            return;
        int nLen = static_cast<int>(strlen(sName.CStr()));
        int nDot = static_cast<int>(pszDot - sName.CStr());
        int nSlash = static_cast<int>(pszSlash - sName.CStr() + 1);
        CExoString sMovieName = sName.Right(nLen - nSlash);
        sMovieName = sMovieName.Left(nDot - nSlash);
        nRow = static_cast<unsigned char>(g_pRules->m_p2DArrays->m_pMovies->GetRowIndex(sMovieName));
    }
    if (static_cast<unsigned int>(nRow) > 87)
        return;
    if (bShown)
        m_movieShown[nRow / 8] |= 1 << (nRow & 7);
    else
        m_movieShown[nRow / 8] &= ~(1 << (nRow & 7));
    g_pAppManager->m_pClientExoApp->SaveOptions();
}

void UnlockMovie(char* pszMovieName) {
    g_sUnlockedMovie = pszMovieName;
}

// Per-movie "already shown" query. The row comes from the "Movies" 2DA and then addresses
// m_movieShown as a bit array, even though the ini round trip reads and writes that block as 11
// plain bytes.
int CClientOptions::GetMovieShown(const CResRef& resRef)
{
    CExoString sName;
    resRef.CopyToString(sName);
    // CompareNoCase returns nonzero when the two strings are equal, so this arm means the pending
    // unlock counts as already shown.
    if (sName.CompareNoCase(g_sUnlockedMovie) != 0)
        return 1;
    int nRow = g_pRules->m_p2DArrays->m_pMovies->GetRowIndex(sName);
    if (nRow < 0)
        return 0;
    // The byte index is a narrow local so the two range guards stay separate, and the bit offset is
    // the division's remainder.
    unsigned char nByteIndex = static_cast<unsigned char>(nRow / 8);
    if (nByteIndex > 10)
        return 0;
    return (1 << (nRow - nByteIndex * 8)) & m_movieShown[nByteIndex];
}

const char* showallmovies(char* pszArgs) {
    K2_UNUSED(pszArgs);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAllMoviesShown();
    return "Success";
}

// The client-options console commands. Each checks g_pAppManager, then the client app, then the
// options, acts, and finally reads the field back through another GetClientOptions call outside the
// guard. That extra call is deliberate.

const char* enablefloatyfeedback(char* pszArgs) {
    int nVal = atoi(pszArgs);
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFloatyFeedback = nVal;
        return "Splendid!";
    }
    return "Awww... nuts.";
}

const char* enableminspec(char* pszArgs) {
    int nVal = atoi(pszArgs);
    if (nVal != 0) {
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetClientOptions() == 0
            || g_pAppManager->m_pClientExoApp->GetModule()->m_pArea == 0) {
            return "Failure";
        }
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_nMinSpecMode = 1;
    } else {
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetClientOptions() == 0
            || g_pAppManager->m_pClientExoApp->GetModule()->m_pArea == 0) {
            return "Failure";
        }
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_nMinSpecMode = 16;
    }
    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->StopSounds(0);
    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->StartSounds();
    return "Success";
}

float setmousewheelspeedmultiplier(char* pszArgs) {
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0
        && strlen(pszArgs) != 0)
    {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_fMouseWheelSpeedMult = static_cast<float>(atof(pszArgs));
    }
    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every console command
    return g_pAppManager->m_pClientExoApp->GetClientOptions()->m_fMouseWheelSpeedMult;
}

float setcameraturnmultiplier(char* pszArgs) {
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0
        && strlen(pszArgs) != 0)
    {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_fCameraTurnMult = static_cast<float>(atof(pszArgs));
    }
    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every console command
    return g_pAppManager->m_pClientExoApp->GetClientOptions()->m_fCameraTurnMult;
}

unsigned char setnumshadowlights(char* pszArgs) {
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0
        && strlen(pszArgs) != 0)
    {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        unsigned char nVal = static_cast<unsigned char>(atoi(pszArgs));
        if (pOptions->m_nNumShadowLights != nVal) {
            pOptions->m_nNumShadowLights = nVal;
            CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
            if (pModule != 0 && pModule->m_pArea != 0)
                pModule->m_pArea->m_bShadowLightsChanged = 1;
        }
    }
    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every console command
    return g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nNumShadowLights;
}

// Returns the current mode byte rather than a "Success" string like the other console commands, and
// read the value back through another GetClientOptions call outside the guard, so the failure path
// still dereferences. Both are deliberate.
unsigned char setcreaturewindmode(char* pszArgs) {
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0
        && strlen(pszArgs) != 0)
    {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCreatureWindMode(
            static_cast<unsigned char>(atoi(pszArgs)));
    }
    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every console command
    return g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCreatureWindMode;
}

// Applies the new wind mode to every visible creature in the current area. A creature counts as
// visible when its fade state is 1, and the player is skipped in the loop because it is handled
// afterwards.
void CClientOptions::SetCreatureWindMode(unsigned char nMode)
{
    if (m_nCreatureWindMode == nMode)
        return;

    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    unsigned long long oidPlayer = pPlayerCreature ? pPlayerCreature->m_objectId
                                                   : CGameObjectArray::INVALIDOBJID;
    CSWCObject* pObject;  // last creature the loop visited; not initialized when the area is empty
    // The module is fetched twice: the first result is only null-tested, and the second is the one
    // whose area is read, so the chain is re-spelled rather than cached in a local.
    if (g_pAppManager->m_pClientExoApp->GetModule() != 0) {
        CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
        if (pArea != 0) {
            for (int64_t i = 0; i < pArea->m_objectIdList.m_count; i++) {
                CGameObject* pGameObject =
                    g_pAppManager->m_pClientExoApp->GetGameObject(pArea->m_objectIdList.m_pArray[i]);
                if (pGameObject == 0)
                    continue;
                pObject = static_cast<CSWCObject*>(pGameObject->AsSWCObject());
                if (pObject == 0)
                    continue;
                if (pObject->m_objectType != GAME_OBJECT_TYPE_CREATURE)
                    continue;
                if (pObject->m_fadeState != 1)
                    continue;
                if (oidPlayer == pObject->m_objectId)
                    continue;
                // The `>= 2` plus nested `== 2` form keeps the two compares sharing one immediate.
                if (nMode >= 2) {
                    if (nMode == 2)
                        pObject->SetWind(1, creaturewindintensity);
                } else {
                    pObject->SetWind(0, 0.0f);
                }
            }
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    pObject = pPlayerCreature;
#endif
    if (pPlayerCreature != 0 && pPlayerCreature->m_fadeState == 1) {
        if (nMode == 1 || nMode == 2)
            pObject->SetWind(1, creaturewindintensity);
        else if (nMode == 0)
            pObject->SetWind(0, 0.0f);
    }
    m_nCreatureWindMode = nMode;
}

void CClientOptions::SetShadowDetail(unsigned char nMode)
{
    if (nMode > 2)
        return;
    if (m_nShadowDetail == nMode)
        return;
    m_nShadowDetail = nMode;

    // Each arm re-spells the module and area prefix, and ends with a module lookup whose result is
    // unused.
    if (nMode == 2) {
        if (g_pAppManager->m_pClientExoApp->GetModule() != 0
            && g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0)
            g_pAppManager->m_pClientExoApp->GetModule();
    } else if (nMode == 1) {
        // The module and area tests come first: the player probe sits inside that guard.
        if (g_pAppManager->m_pClientExoApp->GetModule() != 0
            && g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0) {
            g_pAppManager->m_pClientExoApp->GetModule();
            CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pPlayerCreature != 0)
                pPlayerCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);   // result unused
        }
    }

    if (g_pAppManager->m_pClientExoApp->GetModule() != 0) {
        CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
        if (pArea != 0) {
            for (int64_t i = 0; i < pArea->m_objectIdList.m_count; i++) {
                CGameObject* pGameObject =
                    g_pAppManager->m_pClientExoApp->GetGameObject(pArea->m_objectIdList.m_pArray[i]);
                if (pGameObject == 0)
                    continue;
                CSWCObject* pObject = static_cast<CSWCObject*>(pGameObject->AsSWCObject());
                if (pObject == 0)
                    continue;
                if (pObject->AsSWCPlaceable() != 0) {
                    pObject->RemoveShadowBlob();
                    if (nMode != 2)
                        pObject->ApplyShadowBlob();
                } else if (pObject->AsSWCCreature() != 0
                           && pObject->m_objectId
                                  != g_pAppManager->m_pClientExoApp->GetPlayerCreatureId()) {
                    pObject->RemoveShadowBlob();
                    if (nMode != 2)
                        pObject->ApplyShadowBlob();
                } else if (pObject->GetAnimationBase() != 0) {
                    pObject->RemoveShadowBlob();
                }
            }
        }
    }

    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayerCreature != 0) {
        pPlayerCreature->RemoveShadowBlob();
        if (nMode == 0)
            pPlayerCreature->ApplyShadowBlob();
    }
}

// Writes the client options to the ini with one CExoIni call per key, the value formatted into one
// reused string. Each key is paired with its own field, since the field offsets and bitfield
// indices differ between builds.
int CClientOptions::SaveOptions()
{
    CExoIni ini;
    CExoString sValue;

#if KOTOR1
    // K1 writes "swkotor.ini" without the ".\\" prefix, skips the Reverse Ingame YAxis, CurSithLord,
    // GameCompleted, PC-appearance, ForceSpeed and LockFramerate keys, and writes GUIsInScreenShot.
    sValue.Format(const_cast<char*>("%d"), m_bAutoRun);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Auto Run");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay5);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Combat Movement");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay3);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Reverse Minigame YAxis");
    sValue.Format(const_cast<char*>("%d"), m_bReverseMouseButtons);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Reverse Mouse Buttons");
    sValue.Format(const_cast<char*>("%d"), m_bScreenShotEnabled);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "EnableScreenShot");
    sValue.Format(const_cast<char*>("%d"), m_bGUIsInScreenShot);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "GUIsInScreenShot");
    sValue.Format(const_cast<char*>("%d"), m_bAutoSave);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "AutoSave");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay1);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Mouse Look");
    sValue.Format(const_cast<char*>("%d"), m_bAutoLevelUpNPCs);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Auto Level Up NPCs");
    sValue.Format(const_cast<char*>("%d"), m_difficultyLevel);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Difficulty Level");
    sValue.Format(const_cast<char*>("%d"), m_nMouseSensitivity);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Mouse Sensitivity");
    sValue.Format(const_cast<char*>("%d"), m_bMouseTeleportToButtons);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Enable Mouse Teleporting To Buttons");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback5);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Status Summary");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback4);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Floating Numbers");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback3);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Mini Map");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback2);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Subtitles");
    sValue.Format(const_cast<char*>("%d"), m_bTutorialWindows);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Tutorial Popups");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback0);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Hide Unequippable");
    sValue.Format(const_cast<char*>("%d"), m_bUseSmallFonts);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Use Small Fonts");
    sValue.Format(const_cast<char*>("%d"), m_bHideInGameGui);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Hide InGame GUI");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseNewTarget);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "New Target Selected");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseActionMenu);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "Action Menu");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPausePartyKilled);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "Party Killed");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseEnemySighted);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "Enemy Sighted");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseMineSighted);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "Mine Sighted");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseEndOfCombatRound);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Autopause Options", "End Of Combat Round");
    sValue.Format(const_cast<char*>("%d"), m_nGammaIndex);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Brightness");
    sValue.Format(const_cast<char*>("%d"), m_bShadows);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Shadows");
    sValue.Format(const_cast<char*>("%d"), m_bSoftShadows);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Soft Shadows");
    sValue.Format(const_cast<char*>("%d"), m_bGrass);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Grass");
#ifdef K2_QOL_ADVANCED_ROWS
    // Advanced Graphics' framerate lock, under KOTOR II's key.
    sValue.Format(const_cast<char*>("%d"), g_bFrameRateLocked);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "LockFramerate");
#endif
    sValue.Format(const_cast<char*>("%d"), m_nTextureQuality);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Texture Quality");
    sValue.Format(const_cast<char*>("%d"), m_nAntiAlias);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Anti Aliasing");
    sValue.Format(const_cast<char*>("%d"), m_nFrameBuffer);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Frame Buffer");
    sValue.Format(const_cast<char*>("%d"), m_nAnisotropy);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "Anisotropy");
    sValue.Format(const_cast<char*>("%d"), m_bVSync);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Graphics Options", "V-Sync");
    sValue.Format(const_cast<char*>("%d"), m_nMovieVolume);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Sound Options", "Movie Volume");
    sValue.Format(const_cast<char*>("%d"), m_nSoundEffectsVolume);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Sound Options", "Sound Effects Volume");
    sValue.Format(const_cast<char*>("%d"), m_nVoiceVolume);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Sound Options", "Voiceover Volume");
    sValue.Format(const_cast<char*>("%d"), m_nMusicVolume);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Sound Options", "Music Volume");
    CExoString sName;
    for (int i = 0; i < 11; i++) {
        sName.Format(const_cast<char*>("Movie%2d"), i);
        sValue.Format(const_cast<char*>("%d"), m_movieShown[i]);
        ini.WriteIniEntry(sValue, "swkotor.ini", "Movies Shown", sName.CStr());
    }
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardDPS);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera DPS");
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardAcceleration);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera Acceleration");
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardDeceleration);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera Deceleration");
    sValue.Format(const_cast<char*>("%d"), m_bEnableToolTips);
    ini.WriteIniEntry(sValue, "swkotor.ini", "Game Options", "Enable Tooltips");
    return 1;
#else
    sValue.Format(const_cast<char*>("%d"), m_bAutoRun);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Auto Run");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay5);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Combat Movement");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay3);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Minigame YAxis");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay4);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Ingame YAxis");
    sValue.Format(const_cast<char*>("%d"), m_bReverseMouseButtons);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Mouse Buttons");
    sValue.Format(const_cast<char*>("%d"), m_bScreenShotEnabled);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "EnableScreenShot");
    sValue.Format(const_cast<char*>("%d"), m_bAutoSave);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AutoSave");
    sValue.Format(const_cast<char*>("%d"), m_bGameplay1);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mouse Look");
    sValue.Format(const_cast<char*>("%d"), m_bAutoLevelUpNPCs);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Auto Level Up NPCs");
    sValue.Format(const_cast<char*>("%d"), m_difficultyLevel);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Difficulty Level");
    sValue.Format(const_cast<char*>("%d"), m_nMouseSensitivity);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mouse Sensitivity");
    sValue.Format(const_cast<char*>("%d"), m_bMouseTeleportToButtons);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Enable Mouse Teleporting To Buttons");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback5);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Status Summary");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback4);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Floating Numbers");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback3);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mini Map");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback2);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Subtitles");
    sValue.Format(const_cast<char*>("%d"), m_bTutorialWindows);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Tutorial Popups");
    sValue.Format(const_cast<char*>("%d"), m_bFeedback0);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Hide Unequippable");
    sValue.Format(const_cast<char*>("%d"), m_bUseSmallFonts);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Use Small Fonts");
    sValue.Format(const_cast<char*>("%d"), m_bHideInGameGui);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Hide InGame GUI");
    // CurSithLord is a live global rather than a cached field: mirror GBL_MAIN_SITH_LORD out of the
    // server's variable table while a server exists, and fall back to the cached byte otherwise.
    char nCurSithLord = 0;
    CExoString sVarName("GBL_MAIN_SITH_LORD");
    if (g_pAppManager->m_pServerExoApp != 0) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sVarName, nCurSithLord);
        sValue.Format(const_cast<char*>("%d"), nCurSithLord);
    } else {
        sValue.Format(const_cast<char*>("%d"), m_nCurrentSithLord);
    }
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "CurSithLord");
    sValue.Format(const_cast<char*>("%d"), m_nGamesCompleted);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "GameCompleted");
    // Two forms of the PC appearance record, discriminated by m_bPlayerMadeCharacter: one for no
    // server at all, one for a server that does not own the PC's appearance.
    if (g_pAppManager->m_pServerExoApp == 0) {
    sValue.Format(const_cast<char*>("%d"), m_nPCGender);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "PCGender");
    sValue.Format(const_cast<char*>("%d"), m_nAppearanceType);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AppearanceType");
    sValue.Format(const_cast<char*>("%d"), m_nBodyVariation);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "BodyVariation");
    sValue.Format(const_cast<char*>("%d"), m_nTextureVariation);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "TextureVariation");
    } else {
        CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        int bPlayerMadeCharacter = 0;
        if (pPC != 0) {
            CSWSCreature* pServerCreature = pPC->GetServerCreature();
            if (pServerCreature != 0)
                bPlayerMadeCharacter = (pServerCreature->m_bPlayerMadeCharacter != 0);
        }
        if (pPC == 0 || (!bPlayerMadeCharacter && m_nGamesCompleted <= 2)) {
        sValue.Format(const_cast<char*>("%d"), m_nPCGender);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "PCGender");
        sValue.Format(const_cast<char*>("%d"), m_nAppearanceType);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AppearanceType");
        sValue.Format(const_cast<char*>("%d"), m_nBodyVariation);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "BodyVariation");
        sValue.Format(const_cast<char*>("%d"), m_nTextureVariation);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "TextureVariation");
        } else {
        sValue.Format(const_cast<char*>("%d"), pPC->m_pStats->m_nGender);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "PCGender");
        CSWCCreatureAppearance* pAppearance = pPC->m_pCreatureAppearance;
        unsigned char nBodyVariation = pAppearance->m_cachedBTypeVariant;
        unsigned char nTextureVariation = pAppearance->m_cachedBTypeNumber;
        sValue.Format(const_cast<char*>("%d"), pAppearance->m_nAppearanceId);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AppearanceType");
        sValue.Format(const_cast<char*>("%d"), nBodyVariation);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "BodyVariation");
        sValue.Format(const_cast<char*>("%d"), nTextureVariation);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "TextureVariation");
        }
    }
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseNewTarget);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "New Target Selected");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseActionMenu);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Action Menu");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPausePartyKilled);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Party Killed");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseEnemySighted);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Enemy Sighted");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseMineSighted);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Mine Sighted");
    sValue.Format(const_cast<char*>("%d"), m_bAutoPauseEndOfCombatRound);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "End Of Combat Round");
    sValue.Format(const_cast<char*>("%d"), m_nGammaIndex);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Brightness");
    sValue.Format(const_cast<char*>("%d"), m_bShadows);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Shadows");
    sValue.Format(const_cast<char*>("%d"), m_bSoftShadows);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Soft Shadows");
    sValue.Format(const_cast<char*>("%d"), m_bGrass);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Grass");
    sValue.Format(const_cast<char*>("%d"), g_allowForceSpeedEffect);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "ForceSpeed");
    sValue.Format(const_cast<char*>("%d"), g_bFrameRateLocked);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "LockFramerate");
    sValue.Format(const_cast<char*>("%d"), m_nTextureQuality);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Texture Quality");
    sValue.Format(const_cast<char*>("%d"), m_nAntiAlias);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anti Aliasing");
    sValue.Format(const_cast<char*>("%d"), m_nFrameBuffer);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Frame Buffer");
    sValue.Format(const_cast<char*>("%d"), m_nAnisotropy);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anisotropy");
    sValue.Format(const_cast<char*>("%d"), m_bVSync);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "V-Sync");
    sValue.Format(const_cast<char*>("%d"), m_nMovieVolume);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Movie Volume");
    sValue.Format(const_cast<char*>("%d"), m_nSoundEffectsVolume);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Sound Effects Volume");
    sValue.Format(const_cast<char*>("%d"), m_nVoiceVolume);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Voiceover Volume");
    sValue.Format(const_cast<char*>("%d"), m_nMusicVolume);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Music Volume");
    // The per-movie flag keys are generated, so the loop builds each name.
    CExoString sName;
    for (int i = 0; i < 11; i++) {
        sName.Format(const_cast<char*>("Movie%2d"), i);
        sValue.Format(const_cast<char*>("%d"), m_movieShown[i]);
        ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Movies Shown", sName.CStr());
    }
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardDPS);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera DPS");
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardAcceleration);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera Acceleration");
    sValue.Format(const_cast<char*>("%f"), m_fCameraKeyboardDeceleration);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera Deceleration");
    sValue.Format(const_cast<char*>("%d"), m_bEnableToolTips);
    ini.WriteIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Enable Tooltips");
    return 1;
#endif

}

#if KOTOR1
// K1's LoadOptions takes no argument and reads no "Anti Aliasing" or "Anisotropy" key, though
// SaveOptions still writes both.
int CClientOptions::LoadOptions()
{
    SetDefaultOptions();
    CExoIni ini;
    CExoString sValue;
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Difficulty Level"))
        m_difficultyLevel = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Auto Level Up NPCs"))
        SetAutoLevelUpNPCs(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Mouse Look"))
        m_bGameplay1 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "AutoSave"))
        m_bAutoSave = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Reverse Minigame YAxis"))
        m_bGameplay3 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Combat Movement"))
        m_bGameplay5 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Auto Run"))
        m_bAutoRun = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Mouse Sensitivity"))
        m_nMouseSensitivity = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Enable Mouse Teleporting To Buttons"))
        m_bMouseTeleportToButtons = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Hide Unequippable"))
        m_bFeedback0 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Tutorial Popups"))
        m_bTutorialWindows = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Subtitles"))
        m_bFeedback2 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Mini Map"))
        m_bFeedback3 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Floating Numbers"))
        m_bFeedback4 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Status Summary"))
        m_bFeedback5 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Use Small Fonts"))
        SetUseSmallFonts(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Hide InGame GUI"))
        m_bHideInGameGui = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "End Of Combat Round"))
        m_bAutoPauseEndOfCombatRound = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "Enemy Sighted"))
        m_bAutoPauseEnemySighted = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "Mine Sighted"))
        m_bAutoPauseMineSighted = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "Party Killed"))
        m_bAutoPausePartyKilled = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "Action Menu"))
        m_bAutoPauseActionMenu = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Autopause Options", "New Target Selected"))
        m_bAutoPauseNewTarget = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Brightness"))
        m_nGammaIndex = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Shadows"))
        SetShadows(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "AllowSoftShadows"))
        if (sValue.AsINT() == 1)
            AurATIUnlockSoftShadowsForHotFix();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Soft Shadows"))
        SetSoftShadows(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Grass"))
        SetGrass(sValue.AsINT());
#ifdef K2_QOL_ADVANCED_ROWS
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "LockFramerate"))
        g_bFrameRateLocked = sValue.AsINT() != 0;
#endif
    m_nScreenRes = GetHighResolution() ? 2 : 1;
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Texture Quality"))
        SetTexQual(static_cast<unsigned char>(sValue.AsINT()));
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "Frame Buffer"))
        SetFrameBuffer(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Graphics Options", "V-Sync")) {
        m_bVSync = sValue.AsINT();
        if (m_bVSync) AurSetVSyncState(true);
        else AurSetVSyncState(false);
    }
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Sound Options", "Music Volume"))
        m_nMusicVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Sound Options", "Voiceover Volume"))
        m_nVoiceVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Sound Options", "Sound Effects Volume"))
        m_nSoundEffectsVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Sound Options", "Movie Volume"))
        m_nMovieVolume = static_cast<unsigned char>(sValue.AsINT());
    ApplySoundOptions();

    CExoString sName;
    for (int i = 0; i < 11; i++) {
        sName.Format(const_cast<char*>("Movie%2d"), i);
        ini.ReadIniEntry(sValue, "swkotor.ini", "Movies Shown", sName.CStr());
        m_movieShown[i] = static_cast<unsigned char>(sValue.AsINT());
    }
    m_bEnableCheats = 0;
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "EnableCheats"))
        m_bEnableCheats = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera DPS"))
        SetCameraKeyboardDPS(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera Acceleration"))
        SetCameraKeyboardAcceleration(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Keyboard Camera Deceleration"))
        SetCameraKeyboardDeceleration(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Enable Tooltips"))
        m_bEnableToolTips = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "Reverse Mouse Buttons"))
        m_bReverseMouseButtons = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "EnableScreenShot"))
        m_bScreenShotEnabled = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, "swkotor.ini", "Game Options", "GUIsInScreenShot"))
        m_bGUIsInScreenShot = sValue.AsINT();
    return 1;
}
#else
// Mirrors SaveOptions with one CExoIni read per key, each with the section and key as string
// literals. Receivers come in two kinds: plain fields and bitfields take the guarded atoi form, and
// the graphics and feedback options go through their Set* and Aur* functions.
//
// The int argument selects an alternate spelling for exactly two entries, the first "Anti Aliasing"
// and the first "Anisotropy". Both keys are re-read unconditionally right after those blocks, so the
// second read wins.
int CClientOptions::LoadOptions(int a)
{
    SetDefaultOptions();
    CExoIni ini;
    CExoString sValue;
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Difficulty Level"))
        m_difficultyLevel = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Auto Level Up NPCs"))
        SetAutoLevelUpNPCs(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mouse Look"))
        m_bGameplay1 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AutoSave"))
        m_bAutoSave = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Minigame YAxis"))
        m_bGameplay3 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Ingame YAxis"))
        m_bGameplay4 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Combat Movement"))
        m_bGameplay5 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Auto Run"))
        m_bAutoRun = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mouse Sensitivity"))
        m_nMouseSensitivity = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Enable Mouse Teleporting To Buttons"))
        m_bMouseTeleportToButtons = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Hide Unequippable"))
        m_bFeedback0 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Tutorial Popups"))
        m_bTutorialWindows = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Subtitles"))
        m_bFeedback2 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Mini Map"))
        m_bFeedback3 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Floating Numbers"))
        m_bFeedback4 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Status Summary"))
        m_bFeedback5 = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Use Small Fonts"))
        SetUseSmallFonts(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Hide InGame GUI"))
        m_bHideInGameGui = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "CurSithLord"))
        m_nCurrentSithLord = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "GameCompleted"))
        m_nGamesCompleted = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "PCGender"))
        m_nPCGender = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "AppearanceType"))
        m_nAppearanceType = static_cast<unsigned short>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "BodyVariation"))
        m_nBodyVariation = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "TextureVariation"))
        m_nTextureVariation = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "End Of Combat Round"))
        m_bAutoPauseEndOfCombatRound = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Enemy Sighted"))
        m_bAutoPauseEnemySighted = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Mine Sighted"))
        m_bAutoPauseMineSighted = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Party Killed"))
        m_bAutoPausePartyKilled = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "Action Menu"))
        m_bAutoPauseActionMenu = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Autopause Options", "New Target Selected"))
        m_bAutoPauseNewTarget = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Brightness"))
        m_nGammaIndex = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Shadows"))
        SetShadows(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "AllowSoftShadows"))
        if (sValue.AsINT() == 1)
            AurATIUnlockSoftShadowsForHotFix();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Soft Shadows"))
        SetSoftShadows(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Grass"))
        SetGrass(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "ForceSpeed"))
        g_allowForceSpeedEffect = (sValue.AsINT()) != 0;
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "LockFramerate"))
        g_bFrameRateLocked = (sValue.AsINT()) != 0;
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Texture Quality"))
        SetTexQual(static_cast<unsigned char>(sValue.AsINT()));
    if (a) {
        if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anti Aliasing"))
            SetAntiAlias(static_cast<unsigned char>(sValue.AsINT()));
    }
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anti Aliasing"))
        SetAntiAlias(static_cast<unsigned char>(sValue.AsINT()));
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Frame Buffer"))
        SetFrameBuffer(sValue.AsINT());
    if (a) {
        if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anisotropy"))
            SetAnisotropy(sValue.AsINT());
    }
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "Anisotropy"))
        SetAnisotropy(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Graphics Options", "V-Sync")) {
        m_bVSync = sValue.AsINT();
        if (m_bVSync) AurSetVSyncState(true);
        else AurSetVSyncState(false);
    }
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Music Volume"))
        m_nMusicVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Voiceover Volume"))
        m_nVoiceVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Sound Effects Volume"))
        m_nSoundEffectsVolume = static_cast<unsigned char>(sValue.AsINT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Movie Volume"))
        m_nMovieVolume = static_cast<unsigned char>(sValue.AsINT());
    ApplySoundOptions();

    CExoString sName;
    for (int i = 0; i < 11; i++) {
        sName.Format(const_cast<char*>("Movie%2d"), i);
        ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Movies Shown", sName.CStr());
        m_movieShown[i] = static_cast<unsigned char>(sValue.AsINT());
    }
    m_bEnableCheats = 0;
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "EnableCheats"))
        m_bEnableCheats = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera DPS"))
        SetCameraKeyboardDPS(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera Acceleration"))
        SetCameraKeyboardAcceleration(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Keyboard Camera Deceleration"))
        SetCameraKeyboardDeceleration(sValue.AsFLOAT());
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Enable Tooltips"))
        m_bEnableToolTips = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "Reverse Mouse Buttons"))
        m_bReverseMouseButtons = sValue.AsINT();
    if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Game Options", "EnableScreenShot"))
        m_bScreenShotEnabled = static_cast<unsigned char>(sValue.AsINT());
    return 1;
}
#endif
