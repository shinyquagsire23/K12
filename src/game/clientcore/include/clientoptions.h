#ifndef CCLIENTOPTIONS_H
#define CCLIENTOPTIONS_H

// The client-side settings record holds the graphics, sound, gameplay and camera options as one
// flat struct with trivial get/set accessors. The graphics setters write the field and then call a
// subsystem enable/disable hook.
//
// Only the fields the engine uses are named; the gaps are padding.


struct CClientOptions {
    // m_nCameraMode past the script's CAMERA_MODE_* (0-2), each the camera behaviour CSWCModule
    // sets with it.
    enum {
        CAMERA_MODE_ON_A_STICK = 3,
        CAMERA_MODE_DIALOG = 4,
        CAMERA_MODE_FREE_LOOK = 5,
        CAMERA_MODE_COMBAT = 6,
        CAMERA_MODE_NAVIGATE = 7,
    };

#if KOTOR1  // K1 packs these as separate bytes; K2 uses one 16-bit flag word
    unsigned char  m_bAlwaysRun   : 1;
    unsigned char  m_bHardwareMouseCursor : 1;
    unsigned char  m_bCameraShakeEnabled : 1;
    unsigned char  m_bFullScreen  : 1;
    unsigned char  m_bShowTimingEventBar       : 1;
    unsigned char  m_bFloatyFeedback       : 1;
    unsigned char  m_bReserved1       : 1;
    unsigned char  m_bFlags0Hi     : 1;
    unsigned char  m_difficultyLevel;
    unsigned char  m_bAutoLevelUpNPCs : 1;
    unsigned char  m_bGameplay1       : 1;
    unsigned char  m_bAutoSave        : 1;
    unsigned char  m_bGameplay3       : 1;
    unsigned char  m_bGameplay5       : 1;
    unsigned char  m_bAutoRun         : 1;
    unsigned char  m_nReserved5      : 2;
    float          m_fToolTipTimeDelay;
    unsigned int   m_bMouseTeleportToButtons;
#else
    unsigned short m_bAutoRun      : 1;
    unsigned short m_bAlwaysRun   : 1;
    unsigned short m_bHardwareMouseCursor : 1;
    unsigned short m_bCameraShakeEnabled : 1;
    unsigned short m_bFullScreen  : 1;
    unsigned short m_bShowTimingEventBar       : 1;
    unsigned short m_bFloatyFeedback       : 1;
    unsigned short m_bReserved1       : 1;
    unsigned short m_bFlags0Hi     : 8;
    unsigned char  m_difficultyLevel;
    unsigned char  m_bAutoLevelUpNPCs : 1;
    unsigned char  m_bGameplay1       : 1;
    unsigned char  m_bAutoSave        : 1;
    unsigned char  m_bGameplay3       : 1;
    unsigned char  m_bGameplay4       : 1;
    unsigned char  m_bGameplay5       : 1;
    unsigned char  m_nReserved5      : 2;
    float          m_fToolTipTimeDelay;
    unsigned int   m_bMouseTeleportToButtons;
    unsigned char  m_nCurrentSithLord;
    unsigned char  m_nGamesCompleted;
    unsigned char  m_nPCGender;
    unsigned char  m_nReserved1;
    unsigned short m_nAppearanceType;
    unsigned char  m_nBodyVariation;
    unsigned char  m_nTextureVariation;
#endif
    unsigned char  m_bFeedback0     : 1;
    unsigned char  m_bTutorialWindows : 1;
    unsigned char  m_bFeedback2     : 1;
    unsigned char  m_bFeedback3     : 1;
    unsigned char  m_bFeedback4     : 1;
    unsigned char  m_bFeedback5     : 1;
    unsigned char  m_bFeedback6     : 1;
    unsigned char  m_bFeedback7     : 1;
    unsigned char  m_bUseSmallFonts : 1;
    unsigned char  m_bHideInGameGui : 1;
    unsigned char  m_bEnableToolTips : 1;
    unsigned char  m_bAutoPauseEndOfCombatRound    : 1;
    unsigned char  m_bAutoPauseEnemySighted    : 1;
    unsigned char  m_bAutoPauseMineSighted    : 1;
    unsigned char  m_bAutoPausePartyKilled    : 1;
    unsigned char  m_bAutoPauseActionMenu    : 1;
    unsigned char  m_bAutoPauseNewTarget    : 1;
    unsigned char  m_flagsHi        : 7;
    unsigned char  m_nGammaIndex;
    float          m_fGammaMin;
    float          m_fGammaMax;
    unsigned int   m_bShadows;
    unsigned int   m_bSoftShadows;
    unsigned int   m_bGrass;
    unsigned int   m_nFrameBuffer;
    unsigned int   m_nAnisotropy;
    unsigned int   m_bTextureContrast;
    unsigned char  m_nTextureQuality;
    unsigned char  m_nScreenRes;
    unsigned char  m_aReserved1[2];
    unsigned int   m_bVSync;
    unsigned char  m_nAntiAlias;
    unsigned char  m_nMusicVolume;
    unsigned char  m_nVoiceVolume;
    unsigned char  m_nSoundEffectsVolume;
    unsigned char  m_nMovieVolume;
    unsigned char  m_nEAXLevel;
    unsigned char  m_aReserved2[0x48-0x46];
    unsigned int   m_bForceSoftware;
    unsigned char  m_nMouseSensitivity;
    unsigned char  m_aReserved3[3];
    unsigned int   m_bReverseMouseButtons;
    unsigned char  m_bScreenShotEnabled;
#if KOTOR1  // K1 has the screenshot GUI toggle as one bit; K2 keeps a spare byte
    unsigned char  m_bGUIsInScreenShot : 1;
    unsigned char  m_nReserved2          : 7;
#else
    unsigned char  m_nReserved3;
#endif
    unsigned char  m_movieShown[0x61-0x56];
    unsigned char  m_nHideFadeGeometryMode;
    unsigned char  m_nShadowDetail;
    unsigned char  m_nCameraMode;
    unsigned char  m_nSavedCameraMode;
    unsigned char  m_nNumShadowLights;
    unsigned char  m_nReserved4;
    unsigned char  m_nCreatureWindMode;
    unsigned char  m_nViolenceLevel;
    unsigned char  m_nMinSpecMode;
    unsigned char  m_aReserved4[0x6c-0x6a];
    float          m_fMouseWheelSpeedMult;
    float          m_fCameraTurnMult;
    unsigned char  m_nFontSize;
    unsigned char  m_aReserved5[0x78-0x75];
    float          m_fReserved1;
    int            m_bTurboSpeed;
    int            m_bUnlimitedItemUses;
    int            m_bEnableCheats;
    float          m_fCameraKeyboardDPS;
    float          m_fCameraKeyboardAcceleration;
    float          m_fCameraKeyboardDeceleration;

    CClientOptions();
    ~CClientOptions();
    void SetDefaultOptions();
    void SetCreatureWindMode(unsigned char n);
    void SetShadowDetail(unsigned char n);
    void ApplySoundOptions();

    void SetFrameBuffer(unsigned int n);
    void SetShadows(unsigned int b);
    void SetSoftShadows(unsigned int b);
    void SetGrass(unsigned int b);
    void SetTexCon(unsigned int b);
    void SetTexQual(unsigned char n);
    void SetDefaultSoundOptions();
    void SetNumShadowLights(unsigned char n);
    void SetHideFadeGeometryMode(unsigned char mode);
    void SetEnableHardwareMouseCursor(int b);
    void SetAutoLevelUpNPCs(int b);
    void SetAlwaysRun(int b);
    void SetAnisotropy(unsigned int n);
    void SetVSync(unsigned int b);
    void SetAntiAlias(unsigned char n);
    void SetScreenRes(unsigned char n);
    void SetDefaultScreenShotOptions();
#if KOTOR1
    void SetDefaultGUIsInScreenShotOptions();
#endif

    void  SetCameraMode(unsigned char mode);
    void  ToggleCameraMode();
    void  SetActionFeedbackMode(unsigned char n);
    void  SetDefaultMouseSen();
    void  SetDefaultMouseOptions();
    void  SetDefaultGamma();
    float GetGammaSetting();
    float GetMouseSenSetting();
    float GetCameraKeyboardDPS();
    float GetCameraKeyboardAcceleration();
    float GetCameraKeyboardDeceleration();
    void  SetCameraKeyboardDPS(float f);
    void  SetCameraKeyboardAcceleration(float f);
    void  SetCameraKeyboardDeceleration(float f);
    void  SetToolTipTimeDelay(float f);
    void  SetFullScreenEnabled(int b);

    void  SetUseSmallFonts(int b);
    void  SetDefaultFeedbackOptions();
    void  SetDefaultAutopauseOptions();
    void  SetDefaultGameplayOptions();
    void  SetDefaultKeyboardCamera();
    void  SetAllMoviesShown();
    void  SetMovieShown(const CExoString& resref, int a);
    void  SetMovieShown(const CResRef& resRef, int a);
    int   GetMovieShown(const CResRef& resRef);
    int   SaveOptions();
#if KOTOR1  // K1's loader takes no argument and reads no anti-aliasing or anisotropy keys
    int   LoadOptions();
#else
    int   LoadOptions(int a);
#endif
    void  SetDefaultAdvancedGraphicsOptions();
    void  SetDefaultAdvancedSoundOptions();
    void  SetDefaultGraphicsOptions();
};

#if KOTOR1
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientOptions, m_nGammaIndex) == 0xf, "the flag run is three bytes");
#else
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientOptions, m_nGammaIndex) == 0x17, "the flag run is three bytes");
#endif

#endif
