#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguicontrol.h"
#define CSWGUI_FORCE_WIDGET_INLINE 1

#include "CExoArrayListImpl.h"
#include <stdio.h>
#include <string>
#include "swguimainmenu.h"
#include "swgui.h"
#include "guiingame.h"
#include "swguisaveload.h"
#include "swguidrawable.h"
#include "partytables.h"
#include "serverexoapp.h"
#include "globalvartables.h"
#include "worldtimer.h"
#include "netlayer.h"
#include "nwchandlemessage.h"
#include "exoresman.h"
#include "appmanager.h"
#include "exobase.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "Achievements.h"
#include "exoresmaninternal.h"
#include "swguiclassselection.h"
#include "exoiniinternal.h"
#include "exotlktable.h"
#include "swguiingame.h"
#include "nwccreature.h"
#include "swguimovie.h"
#include "swguimusic.h"
#include "swguioptions.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "nwcanimbase.h"
#include "nwccreaturestats.h"
#include "Vector.h"
#include "Quaternion.h"
#include "exosound.h"
#include "AndroidJNI.h"
#include "port/k1gui.h"
#if KOTOR1
#include <algorithm>
#include "sdlrwops.h"
#endif
#include "port/libcstat.h"
#include "exoinput.h"
#include "restype.h"
#include "serverexoappinternal.h"
#include "strref.h"
#include "exostringinline.h"

// Rows of the port's override/custom.txt (GetLocalizedCustomString).
enum {
    CUSTOMSTRING_REGISTER_YOUR_GAME = 0,
    CUSTOMSTRING_MORE_KOTOR = 10,
};


extern int g_bDo3DGUIs;
extern CExoSound* g_pExoSound;
void ASLAnalyticsTrackScreen(const char* pszScreen);

extern std::string gVendorId;
extern int g_nScreenHeight;
void OutputDebugPrint(const char* pszFormat, ...);


bool HasTouchScreen();
bool IsAmazon();
void ToggleRenderOverNotch(bool bRenderOverNotch);
extern bool g_facebookLoggedIn;
extern volatile bool g_GooglePlay;
extern CTlkTable* g_pTlkTable;

const char* GetOverridePath(int nIndex, bool bFlag);

void MoreGamesButton();
void RegistrationButton();
void UpdateModsButton(CSWGuiButton& button);

extern char g_useModFolder;

typedef void (CSWGuiObject::*CSWGuiEventHandler)(CSWGuiControl*);

#if KOTOR1
// K1: no More Games, Facebook or registration buttons, and no version label.
CSWGuiMainMenu::CSWGuiMainMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    if (g_pExoResMan->Exists(CResRef("MAINMENU"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        g_pExoResMan->AddResourceImageFile(CExoString("RIMS:MAINMENU"));
    }
    g_pExoResMan->bMainMenuImageFileLoaded = 0;

    LoadFromLayout(pGuiManager);

    m_btnWarp.m_tail.bVisible = 0;
    m_lblNewContent.m_tail.bVisible = 0;
#ifdef K2_UI_TOUCH
    m_bShowExit = 0;
#else
    m_bShowExit = 1;   // pc / xbox: BTN_EXIT is Quit, always shown
#endif
    m_lbModules.m_tail.bVisible = 0;

    m_btnContinue.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnContinueButton));
    m_btnContinue.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnContinueButton));
    m_btnContinue.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnContinue.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnContinue.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));

    m_btnGPlay.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnGooglePlayButton));
    m_btnGPlay.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnGooglePlayButton));
    m_btnGPlay.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnGPlay.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnGPlay.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));

    m_btnAchieve.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnAchievementsButton));
    m_btnAchieve.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnAchievementsButton));
    m_btnAchieve.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnAchieve.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnAchieve.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

    m_btnSignIn.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnSignOutButton));
    m_btnSignIn.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnSignOutButton));
    m_btnSignIn.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnSignIn.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnSignIn.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));
    m_btnBack.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));
    m_btnBack.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnBack.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnBack.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnExit.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnNewGamePicked));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnLoadSaveGame));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMoviesPicked));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnOptionsPicked));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnWarpButtonPressed));
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnQuitButtonPressed));

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnNewGamePicked));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnLoadSaveGame));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMoviesPicked));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnOptionsPicked));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnWarpButtonPressed));
    m_btnExit.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnQuitButtonPressed));

    m_btnNewGame.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnLoadGame.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnMovies.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnOptions.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnWarp.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnExit.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));

    m_btnNewGame.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnLoadGame.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnMovies.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnOptions.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnWarp.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnExit.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));

    if (g_bDo3DGUIs)
    {
        CAurScene*  pScene  = m_3DView.m_scene.m_pScene;
        CAurCamera* pCamera = m_3DView.m_scene.m_pCamera;

        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

        CAurObject* pModel = m_3DView.m_scene.AddModel(CExoString("mainmenu"), -1);
        if (pModel)
        {
            pModel->PlayAnimation(const_cast<char*>("default"), 1.0f, 0, 0.0f);
            pModel->Animate(1.0f);
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 0);
            pCamera->SetViewAngle(22.726f);
        }
    }

    SetActiveControl(&m_btnNewGame, 0);
    if (m_btnContinue.m_tail.bVisible)
        SetActiveControl(&m_btnContinue, 0);
    m_nMenuMode = 1;

    g_pExoResMan->RemoveFile(CExoString("HD0:GAMEINPROGRESS"), 0x56);
    g_pExoResMan->CleanDirectory(CExoString("GAMEINPROGRESS:"), 0, 0);
    g_pExoResMan->RemoveFile(CExoString("HD0:CURRENTGAME"), 0x56);
    g_pExoResMan->CleanDirectory(CExoString("CURRENTGAME:"), 0, 0);
    g_pExoResMan->CleanDirectory(CExoString("REBOOTDATA:"), 0, 0);

    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);

    m_bInputReady = 0;
    m_nDrawFrames = 0;
}
#else
CSWGuiMainMenu::CSWGuiMainMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
    , m_nReserved1(0x100)
{
    if (g_pExoResMan->Exists(CResRef("MAINMENU"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        g_pExoResMan->AddResourceImageFile(CExoString("RIMS:MAINMENU"));
    }
    g_pExoResMan->bMainMenuImageFileLoaded = 0;

    LoadFromLayout(pGuiManager);

    CSWGuiExtent extent = m_btnMoreGames.m_originalExtent;
    extent.y += extent.h + 5;
    AddButton(&m_btnRegistration, extent, CExoString(GetLocalizedCustomString(CUSTOMSTRING_REGISTER_YOUR_GAME)));

    CSWGuiBorderParams moreGamesBorder = m_btnMoreGames.m_border.m_params;
    CSWGuiBorderParams moreGamesBorderHilight = m_btnMoreGames.m_borderHilight.m_params;
    m_btnRegistration.m_border.Initialize(m_btnRegistration.m_border.m_extent, moreGamesBorder);
    m_btnRegistration.m_borderHilight.Initialize(m_btnRegistration.m_borderHilight.m_extent,
                                           moreGamesBorderHilight);

    m_btnRegistration.ReSetFont();
    m_btnRegistration.SetMoveToControl(2, &m_btnNewGame);
    m_btnRegistration.SetMoveToControl(0, &m_btnMoreGames);
    m_btnMoreGames.SetMoveToControl(2, &m_btnRegistration);
    m_btnNewGame.SetMoveToControl(0, &m_btnRegistration);

    ShowVersionNumber();

    m_lblBW.m_tail.bVisible = 0;
    m_lblLucas.m_tail.bVisible = 0;
    m_btnWarp.m_tail.bVisible = 0;

    ASLAnalyticsTrackScreen("Main Menu");

    if (HasTouchScreen())
    {
        m_btnMoreGames.m_tail.bVisible = 1;
        m_btnMoreGames.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMoreGamesPressed));
    }
    else
    {
        m_btnMoreGames.m_tail.bVisible = 0;
        m_btnGPlay.SetMoveToControl(2, &m_btnNewGame);
        m_btnNewGame.SetMoveToControl(0, &m_btnGPlay);
    }

    m_bShowExit = 1;

    m_lblNewContent.m_tail.bVisible = 0;
    m_lbModules.m_tail.bVisible = 0;

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnExit.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnMusic.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnGPlay.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnMoreGames.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnNewGamePicked));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnLoadSaveGame));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMoviesPicked));
    m_btnMusic.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMusicPicked));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnOptionsPicked));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnWarpButtonPressed));
    m_btnGPlay.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnQuitButtonPressed));

    m_btnNewGame.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnNewGamePicked));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnLoadSaveGame));
    m_btnMovies.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMoviesPicked));
    m_btnMusic.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnMusicPicked));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnOptionsPicked));
    m_btnWarp.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnWarpButtonPressed));
    m_btnExit.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnQuitButtonPressed));

    m_btnNewGame.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnLoadGame.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnMovies.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnMusic.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnOptions.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnWarp.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnExit.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));

    m_btnNewGame.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnLoadGame.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnMovies.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnMusic.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnOptions.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnWarp.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnExit.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));

    m_btnFacebook.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnFacebook.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnFacebook.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnRegistration.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackPressed));
    m_btnRegistration.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnRegistration.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));

    m_btnFacebook.AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnFacebookButtonPressed));
    m_btnFacebook.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnFacebookButtonPressed));
    m_btnRegistration.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnRegistrationPressed));
    m_btnRegistration.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnRegistrationPressed));

    if (g_bDo3DGUIs)
    {
        CAurScene*  pScene  = m_3DView.m_scene.m_pScene;
        CAurCamera* pCamera = m_3DView.m_scene.m_pCamera;

        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

        g_pAppManager->m_pClientExoApp->LoadOptions(1);
        int nSithLord =
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCurrentSithLord;

        CExoString sModel;
        switch (nSithLord)
        {
        case 0:  sModel = "mainmenu01"; break;
        case 1:  sModel = "mainmenu02"; break;
        case 2:  sModel = "mainmenu03"; break;
        case 3:  sModel = "mainmenu04"; break;
        case 4:  sModel = "mainmenu05"; break;
        default: sModel = "mainmenu01"; break;
        }

        CAurObject* pModel = m_3DView.m_scene.AddModel(sModel, -1);
        if (pModel)
        {
            pModel->PlayAnimation(const_cast<char*>("default"), 1.0f, 0, 0.0f);
            pModel->Animate(1.0f);
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 0);
            pCamera->SetViewAngle(22.726f);
        }

        if (pModel != 0 && nSithLord == 4)
        {
            m_pPCCreature = new CSWCCreature();

            // Seed from the creature's appearance block, then apply the player's chargen choices.
            CCAppearanceInfo appearance =
                *reinterpret_cast<CCAppearanceInfo*>(m_pPCCreature->m_pCreatureAppearance);
            appearance.core.m_nGender =
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nPCGender;
            appearance.core.m_bArmorEquipped = 1;
            appearance.core.m_nAppearanceId =
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nAppearanceType;
            appearance.core.m_bTypeVariant =
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nBodyVariation;
            appearance.core.m_bTypeNumber =
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nTextureVariation;
            m_pPCCreature->m_pStats->m_nAppearanceType = appearance.core.m_nAppearanceId;
            m_pPCCreature->SetAppearance(&appearance, 3, 1, 0);

            m_pPCCreature->AttachToScene(pScene);
            m_pPCCreature->SetOrientation(Vector(0.0f, -1.0f, 0.0f));
            m_pPCCreature->SetPosition(pModel->GetCutsceneDummyPosition());

            // CSWCCreature declares its own GetAnimationBase, so upcast through CSWCObject to
            // reach the real one.
            CAurObject* pPart = static_cast<CAurObject*>(static_cast<CSWCObject*>(m_pPCCreature)
                                    ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY));
            if (pPart != 0)
                pPart->PlayAnimation(const_cast<char*>("evil"), 1.0f, 2, 0.0f);
            pPart = static_cast<CAurObject*>(static_cast<CSWCObject*>(m_pPCCreature)
                        ->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD));
            if (pPart != 0)
                pPart->PlayAnimation(const_cast<char*>("evil"), 1.0f, 2, 0.0f);
        }
        else
        {
            m_pPCCreature = 0;
        }
    }

    SetActiveControl(&m_btnNewGame, 0);
    m_nMenuMode = 1;

    g_pExoResMan->RemoveFile(CExoString("HD0:GAMEINPROGRESS"), 0x56);
    g_pExoResMan->CleanDirectory(CExoString("GAMEINPROGRESS:"), 0, 0);
    g_pExoResMan->RemoveFile(CExoString("HD0:CURRENTGAME"), 0x56);
    g_pExoResMan->CleanDirectory(CExoString("CURRENTGAME:"), 0, 0);
    g_pExoResMan->CleanDirectory(CExoString("REBOOTDATA:"), 0, 0);

    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);

    m_bIdleTimerDisabled = 0;
    m_bInputReady = 0;
    m_nDrawFrames = 0;
}
#endif

CSWGuiMainMenu::~CSWGuiMainMenu()
{
#if !KOTOR1  // K1 has no PC creature
    if (m_pPCCreature != 0)
    {
        delete m_pPCCreature;
        m_pPCCreature = 0;
    }
#endif

    ClearListBox();

    if (g_pExoResMan->Exists(CResRef("MAINMENU"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        g_pExoResMan->bMainMenuImageFileLoaded = 1;
    }
}

void CSWGuiMainMenu::OnEnterButton(CSWGuiControl* pControl)
{
#if KOTOR1
    static_cast<CSWGuiButton*>(pControl)->m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
    static_cast<CSWGuiButton*>(pControl)->m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
}

void MoreGamesButton()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jstring url = env->NewStringUTF("https://play.google.com/store/apps/details?id=com.aspyr.swkotor");
    env->CallStaticVoidMethod(cls, mid, url);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
}

void RegistrationButton()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID midLaunchUrl = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jmethodID midRegister =
        env->GetStaticMethodID(cls, "OnRegistrationButtonClicked", "()V");
    jstring url = env->NewStringUTF("https://ctep.aspyr.com/k2_android");
    env->CallStaticVoidMethod(cls, midLaunchUrl, url);
    env->CallStaticVoidMethod(cls, midRegister);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
}

void UpdateModsButton(CSWGuiButton& button)
{
    if (g_useModFolder != 0)
    {
        button.m_text.m_params.SetText("Disable Mods Folder");
    }
    else
    {
        button.m_text.m_params.SetText("Enable Mods Folder");
    }
}

void CSWGuiMainMenu::OnMoreGamesPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jstring url = env->NewStringUTF("https://play.google.com/store/apps/details?id=com.aspyr.swkotor");
    env->CallStaticVoidMethod(cls, mid, url);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
}

void CSWGuiMainMenu::OnRegistrationPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID midLaunchUrl = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jmethodID midRegister =
        env->GetStaticMethodID(cls, "OnRegistrationButtonClicked", "()V");
    jstring url = env->NewStringUTF("https://ctep.aspyr.com/k2_android");
    env->CallStaticVoidMethod(cls, midLaunchUrl, url);
    env->CallStaticVoidMethod(cls, midRegister);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
}

void CSWGuiMainMenu::OnFacebookButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID midGetActivity =
        env->GetStaticMethodID(cls, "GetActivity", "()Landroid/app/Activity;");
    jobject activity = env->CallStaticObjectMethod(cls, midGetActivity);
    jmethodID midLogin = env->GetMethodID(cls, "loginInToFacebook", "()V");
    env->CallVoidMethod(activity, midLogin);
    env->DeleteLocalRef(cls);
}

void CSWGuiMainMenu::OnBackPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SDL_LockMutex(g_UI_mutex);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "showQuitPopup", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);

    SDL_CondWait(g_UI_cond, g_UI_mutex);

    if (g_done)
        g_pAppManager->m_pClientExoApp->ExitProgram();

    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
extern bool g_bIsInBackground;
void UpdateScreen(float fDelta, int a, int b);
// K1: no Google Play gate; the game is flagged as backgrounded and one frame is drawn
// before the UI thread waits on the Java side.
void CSWGuiMainMenu::OnAchievementsButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SDL_LockMutex(g_UI_mutex);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "openAchievements", "()V");
    env->CallStaticVoidMethod(cls, mid);
    g_bIsInBackground = 1;
    UpdateScreen(1.0f / 30.0f, 0, 0);
    env->DeleteLocalRef(cls);

    SDL_CondWait(g_UI_cond, g_UI_mutex);
    SDL_UnlockMutex(g_UI_mutex);
}
#else
void CSWGuiMainMenu::OnAchievementsButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_GooglePlay)
        return;

    SDL_LockMutex(g_UI_mutex);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "openAchievements", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);

    SDL_CondWait(g_UI_cond, g_UI_mutex);
    SDL_UnlockMutex(g_UI_mutex);
}
#endif

void CSWGuiMainMenu::OnSignOutButton(CSWGuiControl* pControl)
{
#if !KOTOR1  // K1 has no Google Play gate
    if (!g_GooglePlay)
        return;
#endif

    SDL_LockMutex(g_UI_mutex);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "signOutGooglePlay", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);

    SDL_CondWait(g_UI_cond, g_UI_mutex);
    SDL_UnlockMutex(g_UI_mutex);

    OnBackButton(pControl);
}

void CSWGuiMainMenu::ClearListBox()
{
    m_lbModules.ClearItems();
}

void CSWGuiMainMenu::OnWarpButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    OnLoadGamePicked();
}

void CSWGuiMainMenu::OnExitButton(CSWGuiControl* pControl)
{
#if KOTOR1  // K1: the focus-lost half of OnEnterButton
    static_cast<CSWGuiButton*>(pControl)->m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
#else
    K2_UNUSED(pControl);
#endif
}

void CSWGuiMainMenu::LaunchDashboard(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1  // K1: the not-enough-disk-space box callback quits the game
    g_pAppManager->m_pClientExoApp->ExitProgram();
#endif
}

#if KOTOR1

// K1: the Exit button is the "Find More Games" store link, with no state or event-argument
// guards; a Chromebook draws one backgrounded frame before the browser opens.
void CSWGuiMainMenu::OnQuitButtonPressed(CSWGuiControl* pControl)
{
#ifndef K2_UI_TOUCH
    // KOTOR 1, pc / xbox: the layout's BTN_EXIT is Quit -- the engine's own "are you sure?" box.
    if (pControl->m_tail.nCurrentEventArg != 0)
        g_pAppManager->m_pClientExoApp->QueryExitProgram();
#else
    K2_UNUSED(pControl);
    if (m_bInputReady == 0)
        return;

    if (SDL_IsChromebook())
    {
        g_bIsInBackground = 1;
        UpdateScreen(1.0f / 30.0f, 0, 0);
    }

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jstring url = env->NewStringUTF(
        "http://kotor.aspyr.com/?utm_source=kotor&utm_medium=android&utm_campaign=menu-link");
    env->CallStaticVoidMethod(cls, mid, url);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
#endif
}

void CSWGuiMainMenu::OnURLButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jstring url = env->NewStringUTF(
        "http://kotor.aspyr.com/?utm_source=kotor&utm_medium=android&utm_campaign=menu-link");
    env->CallStaticVoidMethod(cls, mid, url);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);
}

bool CSWGuiMainMenu::CheckURLButton()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "checkLaunchURL", "(Ljava/lang/String;)Z");
    jstring url = env->NewStringUTF(
        "http://kotor.aspyr.com/?utm_source=kotor&utm_medium=android&utm_campaign=menu-link");
    jboolean bCanLaunch = env->CallStaticBooleanMethod(cls, mid, url);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(cls);

    static char s_bHasTouchScreen = HasTouchScreen();
    return s_bHasTouchScreen && bCanLaunch;
}

#ifdef K2_HOST_BUILD
#define st_mtime_sec st_mtime  // host: bionic's name for the seconds field
#endif
bool CSWGuiMainMenu::FindNewestSaveGame(CExoString& sSave, CExoString& sName)
{
    CExoArrayList<CExoString> aSaves;
    CExoString sUnused;
    g_pExoBase->GetDirectoryList(&aSaves, CExoString("SAVES:"), RESTYPE_INVALID, 1, 1, 0);

    int64_t nNewest = 0;
    bool bFound = false;
    for (int i = 0; i < aSaves.m_count; i++)
    {
        CExoString sDir = CExoString("SAVES:") + aSaves[i];
        CExoString sInfo = g_pExoBase->m_pAliasList->ResolveFileName(
            sDir + CExoString("/") + CExoString("savenfo"), 0);

        std::string sPath(sInfo.CStr());
        std::replace(sPath.begin(), sPath.end(), '\\', '/');
        SDL_RWops* pFile = SDL_RWFromFile(sPath.c_str(), "r");
        if (pFile)
        {
            struct stat st;
            if (fstat(fileno(pFile->hidden.stdio.fp), &st) == 0 && st.st_mtime_sec > nNewest)
            {
                sSave = sInfo;
                sName = aSaves[i];
                bFound = true;
                nNewest = st.st_mtime_sec;
            }
#ifdef K2_FIX_ENGINE_BUGS
            SDL_RWclose(pFile);
#endif
        }
    }
    return bFound;
}
#ifdef K2_HOST_BUILD
#undef st_mtime_sec
#endif

void LoadGameName(CSWGuiManager* pGuiManager, CExoString sName)
{
    CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
    pEntry->LoadData(sName);
    CSWGuiSaveLoad* pPanel = new CSWGuiSaveLoad(pGuiManager, 0, 1);
    pGuiManager->AddPanel(pPanel, 2, 1);
    pPanel->LoadGame(pEntry);
    delete pEntry;
}

bool TestLoadNewestGame(CSWGuiManager* pGuiManager)
{
    CExoString sSave;
    CExoString sName;
    if (CSWGuiMainMenu::FindNewestSaveGame(sSave, sName))
    {
        printf("%s!!!!\n", sSave.CStr());
        LoadGameName(pGuiManager, sName);
        return true;
    }
    return false;
}

void CSWGuiMainMenu::OnContinueButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (TestLoadNewestGame(m_pGuiManager))
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
}
#else
void CSWGuiMainMenu::OnQuitButtonPressed(CSWGuiControl* pControl)
{
    if (m_bInputReady != 0 && (m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE &&
        pControl->m_tail.nCurrentEventArg != 0)
    {
#ifdef K2_UI_TOUCH
        OnGooglePlayButton(pControl);
#else
        // pc / xbox: the button is Quit -- the engine's own "are you sure?" box.
        g_pAppManager->m_pClientExoApp->QueryExitProgram();
#endif
    }
}
#endif

void CSWGuiMainMenu::HandlePrivacyChoice()
{
    SDL_LockMutex(g_UI_mutex);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "showPrivacyPopup", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);

    SDL_CondWait(g_UI_cond, g_UI_mutex);

    if (g_PrivacyChecked)
    {
        CExoIni ini;
        CExoString sValue;

        if (g_PrivacyConsentGiven)
            sValue.Format(const_cast<char*>("1"));
        else
            sValue.Format(const_cast<char*>("0"));

        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                          CExoString("PrivacyConsent"));
    }

    SDL_UnlockMutex(g_UI_mutex);
}

void CSWGuiMainMenu::OnLoadGamePicked()
{
#if KOTOR1
    // K1: no effects-cache warm-up, and the continue button stays visible.
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }

    if (m_lbModules.m_aControls.m_count == 0)
    {
        PopulateModules();
    }

    m_lbModules.m_tail.bVisible = 1;
    m_3DView.m_tail.bVisible = 0;
    m_btnLoadGame.m_tail.bVisible = 0;
    m_btnMovies.m_tail.bVisible = 0;
    m_btnNewGame.m_tail.bVisible = 0;
    m_btnOptions.m_tail.bVisible = 0;
    m_btnExit.m_tail.bVisible = 0;
    m_btnWarp.m_tail.bVisible = 0;
    m_lblGameLogo.m_tail.bVisible = 0;
    m_lblMenuBG.m_tail.bVisible = 0;

    SetActiveControl(&m_lbModules, 0);
    m_lbModules.SetActiveControl(m_lbModules.GetControl(0), 0);
    m_nMenuMode = 2;
#else
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }

    g_pExoResMan->CacheResource(CExoString("HD0:effects"), 0x270d);

    if (m_lbModules.m_aControls.m_count == 0)
    {
        PopulateModules();
    }

    m_lbModules.m_tail.bVisible = 1;
    m_3DView.m_tail.bVisible = 0;
    m_btnLoadGame.m_tail.bVisible = 0;
    m_btnMovies.m_tail.bVisible = 0;
    m_btnMusic.m_tail.bVisible = 0;
    m_btnNewGame.m_tail.bVisible = 0;
    m_btnOptions.m_tail.bVisible = 0;
    m_btnRegistration.m_tail.bVisible = 0;
    m_btnFacebook.m_tail.bVisible = 0;
    m_lblVersion.m_tail.bVisible = 0;
    m_lblDeviceId.m_tail.bVisible = 0;
    m_btnExit.m_tail.bVisible = 0;
    m_btnWarp.m_tail.bVisible = 0;
    m_lblGameLogo.m_tail.bVisible = 0;
    m_lblMenuBG.m_tail.bVisible = 0;

    SetActiveControl(&m_lbModules, 0);
    m_lbModules.SetActiveControl(m_lbModules.GetControl(0), 0);
    m_nMenuMode = 2;
#endif
}

void CSWGuiMainMenu::OnLoadSaveGame(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: stamps state 4 on this panel only, with no effects precache, no bg_replacement border
    // panel and no cloud sync.
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    if (m_bInputReady == 0)
        return;

    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiSaveLoad* pPanel = new CSWGuiSaveLoad(m_pGuiManager, 0, 1);
    m_pGuiManager->AddPanel(pPanel, 2, 1);

    if ((pPanel->m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    SetToBeDeleted(0);
#else
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    if (m_bInputReady == 0)
        return;

    g_pExoResMan->CacheResource(CExoString("HD0:effects"), 0x270d);

    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    m_bIdleTimerDisabled = 1;

    CSWGuiSaveLoad* pPanel = new CSWGuiSaveLoad(m_pGuiManager, 0, 1, 0);
    pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pPanel, 2, 1);
    pPanel->CheckCloudSync(false);

    if ((pPanel->m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    SetToBeDeleted(0);
    g_pAppManager->m_pClientExoApp->ClearMainMenuPointer();
#endif
}

void CSWGuiMainMenu::OnMoviesPicked(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: no bg_replacement border panel.
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }
    if (pControl->m_tail.nCurrentEventArg == 0)
    {
        return;
    }

    g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();

    CSWGuiTitleMovies* pPanel = new CSWGuiTitleMovies(m_pGuiManager);
    m_pGuiManager->AddPanel(pPanel, 3, 1);
#else
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }
    if (pControl->m_tail.nCurrentEventArg == 0)
    {
        return;
    }

    g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();

    m_bIdleTimerDisabled = 1;

    CSWGuiTitleMovies* pPanel = new CSWGuiTitleMovies(m_pGuiManager);
    pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pPanel, 3, 1);
#endif
}

#if !KOTOR1  // K2-only: OnMusicPicked is absent from K1 (no music page)
void CSWGuiMainMenu::OnMusicPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
    {
        return;
    }

    g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();

    m_bIdleTimerDisabled = 1;

    CSWGuiTitleMusic* pPanel = new CSWGuiTitleMusic(m_pGuiManager);
    pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pPanel, 3, 1);
}
#endif

void CSWGuiMainMenu::OnOptionsPicked(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: no bg_replacement border panel.
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }
    if (pControl->m_tail.nCurrentEventArg == 0)
    {
        return;
    }

    CSWGuiOptionsMain* pPanel = new CSWGuiOptionsMain(m_pGuiManager);
    m_pGuiManager->AddPanel(pPanel, 3, 1);
#else
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
    {
        return;
    }
    if (m_bInputReady == 0)
    {
        return;
    }
    if (pControl->m_tail.nCurrentEventArg == 0)
    {
        return;
    }

    m_bIdleTimerDisabled = 1;

    CSWGuiOptionsMain* pPanel = new CSWGuiOptionsMain(m_pGuiManager);
    pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pPanel, 3, 1);
#endif
}

void CSWGuiMainMenu::OnNewGamePicked(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: starts in END_M01AA, with no server bootstrap, no effects precache, no module override
    // directories, no bg_replacement border panel and no tutorial-tracker reset.
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    if (m_bInputReady == 0)
        return;

    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWPartyTable::ResetCurrentSessionStartTime();

    CExoIni ini;
    CExoString sValue;
    CExoString sModule = "END_M01AA";

    g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));

    if (!g_pExoResMan->Exists(CResRef(sModule), RESTYPE_MOD, static_cast<unsigned int*>(0)) &&
        !g_pExoResMan->Exists(CResRef(sModule), RESTYPE_RIM, static_cast<unsigned int*>(0)))
    {
        sModule = "END_M01AA";
    }

    g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));

    CSWGuiClassSelection* pPanel = new CSWGuiClassSelection(m_pGuiManager, sModule);
    m_pGuiManager->AddPanel(pPanel, 2, 1);

    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);

    SetToBeDeleted(0);
#ifdef K2_FIX_ENGINE_BUGS
    if (CGuiInGame* pInGame = g_pAppManager->m_pClientExoApp->GetInGameGui())
        __builtin_memset(pInGame->m_tutorialWindowShown, 0, sizeof(pInGame->m_tutorialWindowShown));
#endif
#else
    if ((m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        return;

    if (m_bInputReady == 0)
        return;

    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWPartyTable::ResetCurrentSessionStartTime();
        g_pAppManager->CreateServer(0);

        CExoIni ini;
        CExoString sValue;
        CExoString sModule = "001ebo";

        g_pExoResMan->CacheResource(CExoString("HD0:effects"), 0x270d);
        g_pExoResMan->AddResourceDirectory(CExoString("MODULES:"));
        AddModuleOverrideResourceDirectories();

        if (!g_pExoResMan->Exists(CResRef(sModule), RESTYPE_MOD, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(CResRef(sModule), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        {
            sModule = "001ebo";
        }

        g_pExoResMan->RemoveResourceDirectory(CExoString("MODULES:"));
        RemoveModuleOverrideResourceDirectories();

        CSWGuiClassSelection* pPanel = new CSWGuiClassSelection(m_pGuiManager, sModule);
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
        m_pGuiManager->AddPanel(pPanel, 2, 1);

        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);

        SetToBeDeleted(0);
        g_pAppManager->m_pClientExoApp->ClearMainMenuPointer();
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ResetTutorialWindowTracker();
#endif
}

#if !KOTOR1
static int s_nPrivacyCheckFrames = 3;
static int s_nSoundCacheStep;
#endif

void CSWGuiMainMenu::Draw(float f)
{
#if KOTOR1
    // K1: no privacy dialog and no sound-bank precache; New Game takes the focus whenever
    // nothing has it.
    CSWGuiPanel::Draw(f);

    if (m_nDrawFrames >= 5)
    {
        m_bInputReady = 1;
    }
    else
    {
        m_nDrawFrames++;
    }

    if (m_pActiveControl == 0)
    {
        SetActiveControl(&m_btnNewGame, 0);
    }
#else
    if (s_nPrivacyCheckFrames > 0)
    {
        s_nPrivacyCheckFrames--;
    }
    if (s_nPrivacyCheckFrames == 0)
    {
        if (!g_PrivacyChecked)
        {
            HandlePrivacyChoice();
        }
        s_nPrivacyCheckFrames = -1;
    }

    CSWGuiPanel::Draw(f);

    if (m_nDrawFrames >= 5)
    {
        m_bInputReady = 1;
        if (s_nSoundCacheStep <= 2)
        {
            switch (s_nSoundCacheStep)
            {
            case 0:
                g_pExoResMan->CacheResource(CExoString("HD0:combat"), 0x270d);
                break;
            case 1:
                g_pExoResMan->CacheResource(CExoString("HD0:footsteps"), 0x270d);
                break;
            case 2:
                g_pExoResMan->CacheResource(CExoString("HD0:weapons"), 0x270d);
                break;
            }
            s_nSoundCacheStep++;
        }
    }
    else
    {
        m_nDrawFrames++;
    }

    if (!g_GooglePlay && m_btnBack.m_tail.bVisible)
    {
        OnBackButton(0);
    }
#endif
}

void CSWGuiMainMenu::OnBackButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);

#if KOTOR1
    // K1: no Amazon variant; the continue button shows only when a save exists, and the Exit
    // (store link) button follows m_bShowExit.
    m_btnAchieve.m_tail.bVisible = 0;
    m_btnSignIn.m_tail.bVisible = 0;
    m_btnBack.m_tail.bVisible = 0;

    CExoString sSave;
    CExoString sName;
    m_btnContinue.m_tail.bVisible = FindNewestSaveGame(sSave, sName);

    m_btnNewGame.m_tail.bVisible = 1;
    m_btnLoadGame.m_tail.bVisible = 1;
    m_btnMovies.m_tail.bVisible = 1;
    m_btnOptions.m_tail.bVisible = 1;
    m_btnExit.m_tail.bVisible = m_bShowExit;
    m_btnGPlay.m_tail.bVisible = 1;

    SetActiveControl(&m_btnGPlay, 0);
#else
    m_btnAchieve.m_tail.bVisible = 0;
    if (!IsAmazon())
    {
        m_btnSignIn.m_tail.bVisible = 0;
    }
    m_btnBack.m_tail.bVisible = 0;

    m_btnNewGame.m_tail.bVisible = 1;
    m_btnLoadGame.m_tail.bVisible = 1;
    m_btnMovies.m_tail.bVisible = 1;
    m_btnMusic.m_tail.bVisible = 1;
    m_btnOptions.m_tail.bVisible = 1;
    m_btnRegistration.m_tail.bVisible = 1;
    m_btnFacebook.m_tail.bVisible = 1;
    m_lblVersion.m_tail.bVisible = 1;
    m_lblDeviceId.m_tail.bVisible = 1;
    m_btnExit.m_tail.bVisible = m_bShowExit;
    m_btnGPlay.m_tail.bVisible = 1;
    if (HasTouchScreen())
    {
        m_btnMoreGames.m_tail.bVisible = 1;
    }

    SetActiveControl(&m_btnGPlay, 0);
#endif
}

void CSWGuiMainMenu::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1: the back pair is tested first; the Exit (store link) button follows m_bShowExit and
    // the continue button takes the focus when visible.
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_B || type == INPUTEVENT_GUI_BACK)
        {
            if (m_nMenuMode == 2)
            {
                m_lbModules.m_tail.bVisible = 0;
                m_3DView.m_tail.bVisible = 1;
                m_btnLoadGame.m_tail.bVisible = 1;
                m_btnMovies.m_tail.bVisible = 1;
                m_btnNewGame.m_tail.bVisible = 1;
                m_btnOptions.m_tail.bVisible = 1;
                m_btnWarp.m_tail.bVisible = 1;
                m_lblGameLogo.m_tail.bVisible = 1;
                m_btnExit.m_tail.bVisible = m_bShowExit;
                m_lblMenuBG.m_tail.bVisible = 1;

                SetActiveControl(&m_btnNewGame, 0);
                if (m_btnContinue.m_tail.bVisible)
                {
                    SetActiveControl(&m_btnContinue, 0);
                }
                m_nMenuMode = 1;
            }
        }
        else if (type == INPUTEVENT_GUI_START)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        }
    }
#else
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_START)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        }
        else if (type == INPUTEVENT_GUI_B || type == INPUTEVENT_GUI_BACK)
        {
            if (m_nMenuMode == 2)
            {
                m_lbModules.m_tail.bVisible = 0;
                m_3DView.m_tail.bVisible = 1;
                m_btnLoadGame.m_tail.bVisible = 1;
                m_btnMovies.m_tail.bVisible = 1;
                m_btnMusic.m_tail.bVisible = 1;
                m_btnNewGame.m_tail.bVisible = 1;
                m_btnOptions.m_tail.bVisible = 1;
                m_btnRegistration.m_tail.bVisible = 1;
                m_btnFacebook.m_tail.bVisible = 1;
                m_lblVersion.m_tail.bVisible = 1;
                m_lblDeviceId.m_tail.bVisible = 1;
                m_btnExit.m_tail.bVisible = 1;
                m_btnWarp.m_tail.bVisible = 1;
                m_lblGameLogo.m_tail.bVisible = 1;
                m_lblMenuBG.m_tail.bVisible = 1;

                SetActiveControl(&m_btnNewGame, 0);
                m_nMenuMode = 1;
            }
        }
    }
#endif

    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiMainMenu::OnGooglePlayButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1
    // K1: no Amazon arm; one backgrounded frame is drawn after the Java login call; the three
    // port buttons take the first three menu slots.
    SDL_LockMutex(g_UI_mutex);

    if (!g_GooglePlay)
    {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
        jmethodID mid = env->GetStaticMethodID(cls, "loginGooglePlay", "()V");
        env->CallStaticVoidMethod(cls, mid);
        g_bIsInBackground = 1;
        UpdateScreen(1.0f / 30.0f, 0, 0);
        env->DeleteLocalRef(cls);

        SDL_CondWait(g_UI_cond, g_UI_mutex);
    }

    if (g_GooglePlay)
    {
        if (m_btnContinue.m_tail.bVisible)
        {
            m_btnAchieve.SetExtent(m_btnContinue.m_extent);
            m_btnSignIn.SetExtent(m_btnNewGame.m_extent);
            m_btnBack.SetExtent(m_btnLoadGame.m_extent);
        }
        else
        {
            m_btnAchieve.SetExtent(m_btnNewGame.m_extent);
            m_btnSignIn.SetExtent(m_btnLoadGame.m_extent);
            m_btnBack.SetExtent(m_btnMovies.m_extent);
        }

        m_btnContinue.m_tail.bVisible = 0;
        m_btnNewGame.m_tail.bVisible = 0;
        m_btnLoadGame.m_tail.bVisible = 0;
        m_btnMovies.m_tail.bVisible = 0;
        m_btnOptions.m_tail.bVisible = 0;
        m_btnExit.m_tail.bVisible = 0;
        m_btnGPlay.m_tail.bVisible = 0;

        m_btnAchieve.m_tail.bVisible = 1;
        m_btnSignIn.m_tail.bVisible = 1;
        m_btnBack.m_tail.bVisible = 1;

        SetActiveControl(&m_btnAchieve, 0);
    }

    SDL_UnlockMutex(g_UI_mutex);
#else
    if (IsAmazon())
    {
        m_btnAchieve.SetExtent(m_btnNewGame.m_extent);
        m_btnBack.SetExtent(m_btnLoadGame.m_extent);

        m_btnContinue.m_tail.bVisible = 0;
        m_btnNewGame.m_tail.bVisible = 0;
        m_btnLoadGame.m_tail.bVisible = 0;
        m_btnMovies.m_tail.bVisible = 0;
        m_btnMusic.m_tail.bVisible = 0;
        m_btnOptions.m_tail.bVisible = 0;
        m_btnRegistration.m_tail.bVisible = 0;
        m_btnFacebook.m_tail.bVisible = 0;
        m_lblVersion.m_tail.bVisible = 0;
        m_lblDeviceId.m_tail.bVisible = 0;
        m_btnExit.m_tail.bVisible = 0;
        m_btnGPlay.m_tail.bVisible = 0;
        m_btnMoreGames.m_tail.bVisible = 0;

        m_btnAchieve.m_tail.bVisible = 1;
        m_btnBack.m_tail.bVisible = 1;

        SetActiveControl(&m_btnAchieve, 0);
        return;
    }

    SDL_LockMutex(g_UI_mutex);

    if (!g_GooglePlay)
    {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
        jmethodID mid = env->GetStaticMethodID(cls, "loginGooglePlay", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);

        SDL_CondWait(g_UI_cond, g_UI_mutex);
    }

    if (g_GooglePlay)
    {
        m_btnAchieve.SetExtent(m_btnNewGame.m_extent);
        m_btnSignIn.SetExtent(m_btnLoadGame.m_extent);
        m_btnBack.SetExtent(m_btnMovies.m_extent);

        m_btnContinue.m_tail.bVisible = 0;
        m_btnNewGame.m_tail.bVisible = 0;
        m_btnLoadGame.m_tail.bVisible = 0;
        m_btnMovies.m_tail.bVisible = 0;
        m_btnMusic.m_tail.bVisible = 0;
        m_btnOptions.m_tail.bVisible = 0;
        m_btnRegistration.m_tail.bVisible = 0;
        m_btnFacebook.m_tail.bVisible = 0;
        m_lblVersion.m_tail.bVisible = 0;
        m_lblDeviceId.m_tail.bVisible = 0;
        m_btnExit.m_tail.bVisible = 0;
        m_btnGPlay.m_tail.bVisible = 0;
        m_btnMoreGames.m_tail.bVisible = 0;

        m_btnAchieve.m_tail.bVisible = 1;
        m_btnSignIn.m_tail.bVisible = 1;
        m_btnBack.m_tail.bVisible = 1;

        SetActiveControl(&m_btnAchieve, 0);
    }

    SDL_UnlockMutex(g_UI_mutex);
#endif
}

#if KOTOR1
void SetIosButtonImage(CSWGuiButton* pButton, const char* pszImage)
{
    char szName[256];
    const char* pszLang;
    switch (ASLPlat_GetCurrentLanguage())
    {
    case ASL_LANGUAGE_FRENCH:   pszLang = "_fr"; break;
    case ASL_LANGUAGE_ITALIAN:  pszLang = "_it"; break;
    case ASL_LANGUAGE_GERMAN:   pszLang = "_de"; break;
    case ASL_LANGUAGE_SPANISH:  pszLang = "_es"; break;
    default:                    pszLang = "_en"; break;
    }

    sprintf(szName, "ios_mm_%s%s", pszImage, pszLang);
    pButton->m_border.m_params.SetFillImage(CResRef(szName), 0);
    sprintf(szName, "ios_mm_%s2%s", pszImage, pszLang);
    pButton->m_borderHilight.m_params.SetFillImage(CResRef(szName), 0);
}

#ifndef K2_UI_TOUCH
// K1 (pc / xbox): the game's own menu layout (mainmenu8x6 for the PC, mainmenu for the console),
// scaled into the port's 1024x768 units as it loads. None of the port's buttons is added to the
// panel, so they never draw; BTN_EXIT is Quit.
void CSWGuiMainMenu::LoadFromLayout(CSWGuiManager* pGuiManager)
{
    K2_UNUSED(pGuiManager);

#ifdef K1_UI_XBOX
    StartLoadFromLayout(CResRef("mainmenu"), 1, 1, 0);
#else
    StartLoadFromLayout(CResRef("mainmenu8x6"), 1, 1, 0);
#endif
    m_stateFlags |= 1;

    InitControl(m_lbModules, CExoString("LB_MODULES"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
    InitControl(m_btnWarp, CExoString("BTN_WARP"), 1, 1, 0, 0, 0);
    InitControl(m_3DView, CExoString("LBL_3DVIEW"), 1, 1, 0, 0, 0);
    InitControl(m_btnNewGame, CExoString("BTN_NEWGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnLoadGame, CExoString("BTN_LOADGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnMovies, CExoString("BTN_MOVIES"), 1, 1, 0, 0, 0);
    InitControl(m_btnOptions, CExoString("BTN_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewContent, CExoString("LBL_NEWCONTENT"), 1, 1, 0, 0, 0);
    InitControl(m_lblGameLogo, CExoString("LBL_GAMELOGO"), 1, 1, 0, 0, 0);
    InitControl(m_lblMenuBG, CExoString("LBL_MENUBG"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // The panel is in 1024x768 units; scale them to the screen height (the draw centres it).
    ScaleExtentForResolution();
}
#else
// K1 (touch): one "mainmenu" layout, with BTN_EXIT as the store link ("Find More Games"). The
// panel and a disabled full-screen "black" button cover the screen, the background and logo
// labels are laid out from the screen size, the continue and port buttons are copies of
// BTN_NEWGAME, and the menu rows are re-seated in one column under the logo.
void CSWGuiMainMenu::LoadFromLayout(CSWGuiManager* pGuiManager)
{
    K2_UNUSED(pGuiManager);

    StartLoadFromLayout(CResRef("mainmenu"), 1, 1, 0);
    m_stateFlags |= 1;

    InitControl(m_lbModules, CExoString("LB_MODULES"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
    m_btnExit.m_text.m_params.SetText(CExoString("Find More Games"));
    InitControl(m_btnWarp, CExoString("BTN_WARP"), 1, 1, 0, 0, 0);
    InitControl(m_3DView, CExoString("LBL_3DVIEW"), 1, 1, 0, 0, 0);
    InitControl(m_btnNewGame, CExoString("BTN_NEWGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnLoadGame, CExoString("BTN_LOADGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnMovies, CExoString("BTN_MOVIES"), 1, 1, 0, 0, 0);
    InitControl(m_btnOptions, CExoString("BTN_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewContent, CExoString("LBL_NEWCONTENT"), 1, 1, 0, 0, 0);
    InitControl(m_lblGameLogo, CExoString("LBL_GAMELOGO"), 1, 1, 0, 0, 0);
    InitControl(m_lblMenuBG, CExoString("LBL_MENUBG"), 1, 1, 0, 0, 0);

    CSWGuiExtent extent;
    extent.w = g_SDL_mode.w;
    extent.h = g_SDL_mode.h;
    float fScale = extent.h / 768.0f;
    SetExtent(extent);

    m_btnBlack.SetExtent(extent);
    m_btnBlack.m_text.m_params.SetText(CExoString(""));
    m_btnBlack.m_border.m_params.SetFillImage(CResRef("black"), 0);
    m_btnBlack.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnBlack.m_borderHilight.m_params.SetFillImage(CResRef("black"), 0);
    m_btnBlack.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_btnBlack.m_tail.bFocusOnHover = 0;
    m_btnBlack.SetEnabled(0);
    AddControl(&m_btnBlack);

    extent.w = extent.h * 756 / 1024;
    m_lblBackground.Initialize(extent, CSWGuiTextParams(),
                               CSWGuiBorderParams(CResRef(), CResRef(), CResRef("and_main_bg"), 2, 0,
                                                  1.0f, Vector(1.0f, 1.0f, 1.0f), 0),
                               1.0f);
    AddControl(&m_lblBackground);

    extent.h = g_SDL_mode.h * 419 / 1024;
    extent.w = extent.h * 778 / 419;
    extent.x = g_SDL_mode.w - extent.w;
    m_lblLogo.Initialize(extent, CSWGuiTextParams(),
                         CSWGuiBorderParams(CResRef(), CResRef(), CResRef("and_main_logo"), 2, 0,
                                            1.0f, Vector(1.0f, 1.0f, 1.0f), 0),
                         1.0f);
    AddControl(&m_lblLogo);

    m_btnContinue = m_btnNewGame;
    AddControl(&m_btnContinue);
    m_btnGPlay = m_btnAchieve = m_btnSignIn = m_btnBack = m_btnNewGame;
    AddControl(&m_btnGPlay);
    AddControl(&m_btnAchieve);
    AddControl(&m_btnSignIn);
    AddControl(&m_btnBack);

    CSWGuiButton* apButtons[] = {&m_btnContinue, &m_btnNewGame, &m_btnLoadGame, &m_btnMovies,
                                 &m_btnOptions, &m_btnExit, &m_btnGPlay, &m_btnAchieve,
                                 &m_btnSignIn, &m_btnBack};
    for (int i = 0; i < 10; i++)
    {
        CSWGuiButton* pButton = apButtons[i];
        pButton->m_text.m_params.SetText(CExoString(""));
        pButton->m_border.m_params.SetFillImage(CResRef("ios_help"), 0);
        pButton->m_border.m_params.SetCornerImage(CResRef(""), 0);
        pButton->m_border.m_params.SetEdgeImage(CResRef(""), 0);
        pButton->m_borderHilight.m_params.SetFillImage(CResRef("ios_help2"), 0);
        BringControlToFront(apButtons[i]);
    }

    // One column of 378x54 rows, centred under the logo in 768-line layout units.
    int x = static_cast<int>((extent.x + extent.w / 2) / fScale - 189.0f);

    extent.x = x;
    extent.y = 350;
    extent.w = 378;
    extent.h = 54;
    m_btnContinue.SetExtent(extent);
    SetIosButtonImage(&m_btnContinue, "con");
    m_btnContinue.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 400;
    extent.w = 378;
    extent.h = 54;
    m_btnNewGame.SetExtent(extent);
    SetIosButtonImage(&m_btnNewGame, "new");
    m_btnNewGame.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 450;
    extent.w = 378;
    extent.h = 54;
    m_btnLoadGame.SetExtent(extent);
    SetIosButtonImage(&m_btnLoadGame, "loa");
    m_btnLoadGame.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 500;
    extent.w = 378;
    extent.h = 54;
    m_btnMovies.SetExtent(extent);
    SetIosButtonImage(&m_btnMovies, "mov");
    m_btnMovies.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 550;
    extent.w = 378;
    extent.h = 54;
    m_btnOptions.SetExtent(extent);
    SetIosButtonImage(&m_btnOptions, "opt");
    m_btnOptions.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 600;
    extent.w = 378;
    extent.h = 54;
    m_btnExit.SetExtent(extent);
    SetIosButtonImage(&m_btnExit, "mor");
    m_btnExit.ScaleExtentForResolution();

    extent.x = x;
    extent.y = 650;
    extent.w = 378;
    extent.h = 54;
    m_btnGPlay.SetExtent(extent);
    SetIosButtonImage(&m_btnGPlay, "gp");
    m_btnGPlay.ScaleExtentForResolution();

    SetIosButtonImage(&m_btnAchieve, "ach");
    SetIosButtonImage(&m_btnSignIn, "so");
    SetIosButtonImage(&m_btnBack, "bak");

    StopLoadFromLayout();
}
#endif  // K2_UI_TOUCH
#else
void CSWGuiMainMenu::LoadFromLayout(CSWGuiManager* pGuiManager)
{
    K2_UNUSED(pGuiManager);

#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    const bool bRelayout = m_controls.m_count > 0;   // set when a resolution change reloads the layout
#endif
    CExoString sLayout;
#ifdef K2_UI_PC
    // PC UI: the PC menu layout, with Quit where the port has its storefront button.
    // "mainmenuA8x6_p" only exists in the Android override.
    sLayout = "mainmenu8x6_p";
#else
    sLayout = "mainmenuA8x6_p";
#ifndef K2_UI_TOUCH
    // xbox on a PC data set (the Steam install), which has no Android override: fall back to the
    // PC menu, whose BTN_EXIT is Quit.
    const bool bPcMenu = !g_pExoResMan->Exists(CResRef("mainmenuA8x6_p"), RESTYPE_GUI, static_cast<unsigned int*>(0));
    if (bPcMenu)
        sLayout = "mainmenu8x6_p";
#endif
#endif
    if (sLayout.GetLength() >= 1)
    {
        StartLoadFromLayout(CResRef(sLayout), 1, 1, 0);
        m_stateFlags |= 1;
    }
    else
    {
        StartLoadFromLayout(CResRef("mainmenu_p"), 1, 1, 0);
        m_stateFlags &= ~1;
    }

    // The bind order follows the layout's tag order, not the member order.
    InitControl(m_lbModules, CExoString("LB_MODULES"), 1, 1, 0, 0, 0);
#ifdef K2_UI_PC
    // PC UI: the same member carries the PC tag; its activate handler is already
    // OnQuitButtonPressed.
    InitControl(m_btnGPlay, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
#elif !defined(K2_UI_TOUCH)
    InitControl(m_btnGPlay, CExoString(bPcMenu ? "BTN_EXIT" : "BTN_GPLAY"), 1, 1, 0, 0, 0);
#else
    InitControl(m_btnGPlay, CExoString("BTN_GPLAY"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_btnWarp, CExoString("BTN_WARP"), 1, 1, 0, 0, 0);
    InitControl(m_3DView, CExoString("LBL_3DVIEW"), 1, 1, 0, 0, 0);
    InitControl(m_btnNewGame, CExoString("BTN_NEWGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnLoadGame, CExoString("BTN_LOADGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnMovies, CExoString("BTN_MOVIES"), 1, 1, 0, 0, 0);
    InitControl(m_btnMusic, CExoString("BTN_MUSIC"), 1, 1, 0, 0, 0);
    InitControl(m_btnOptions, CExoString("BTN_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewContent, CExoString("LBL_NEWCONTENT"), 1, 1, 0, 0, 0);
    InitControl(m_lblGameLogo, CExoString("LBL_GAMELOGO"), 1, 1, 0, 0, 0);
    InitControl(m_lblMenuBG, CExoString("LBL_MENUBG"), 1, 1, 0, 0, 0);

    // "More Games" only exists on a touch build, but its caption is set unconditionally below.
    if (HasTouchScreen())
    {
        InitControl(m_btnMoreGames, CExoString("BTN_MOREGAMES"), 1, 1, 0, 0, 0);
    }
    const char* pszMoreGames = GetLocalizedCustomString(CUSTOMSTRING_MORE_KOTOR);
    m_btnMoreGames.m_text.m_params.SetText(pszMoreGames);

    InitControl(m_btnAchieve, CExoString("BTN_ACHIEVE"), 1, 1, 0, 0, 0);
    InitControl(m_btnSignIn, CExoString("BTN_SIGNIN"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    m_btnAchieve.m_text.m_params.SetText(GetLocalizedGmenuString(0));
    m_btnSignIn.m_text.m_params.SetText(GetLocalizedGmenuString(1));
    m_btnBack.m_text.m_params.SetText(GetLocalizedGmenuString(2));

    // The three port-added buttons are hidden until their subsystem decides otherwise.
    m_btnAchieve.m_tail.bVisible = 0;
    m_btnSignIn.m_tail.bVisible = 0;
    m_btnBack.m_tail.bVisible = 0;

    // Each button registers the same five events: activate, activate-alt, focus-gained,
    // focus-lost and back. Only the activate and activate-alt handler differs per button.
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));
    m_btnBack.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));
    m_btnBack.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnBack.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnBack.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

    m_btnAchieve.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnAchievementsButton));
    m_btnAchieve.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnAchievementsButton));
    m_btnAchieve.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnAchieve.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnAchieve.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

    m_btnSignIn.AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnSignOutButton));
    m_btnSignIn.AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnSignOutButton));
    m_btnSignIn.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnEnterButton));
    m_btnSignIn.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnExitButton));
    m_btnSignIn.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnBackButton));

#ifdef K2_UI_TOUCH
    // The custom-mobile-string value is overwritten below; the call itself is kept.
    const char* pszGPlay = GetLocalizedCustomMobileString(0);
    pszGPlay = IsAmazon() ? "GameCircle" : "Google Play";
    m_btnGPlay.m_text.m_params.SetText(pszGPlay);
#else
    static_cast<void>(GetLocalizedCustomMobileString(0));
#if !defined(K2_UI_PC)
    // xbox: Quit, captioned like the PC layout's BTN_EXIT.
    m_btnGPlay.m_text.m_params.SetStrRef(STRREF_QUIT);
#endif
#endif

    // "Finished the game with a maxed-out dark-side character".
#if !KOTOR1
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCurrentSithLord == 4)
    {
        Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_SITH_LORD_MAIN_MENU);
    }
#endif

    StopLoadFromLayout();

#if defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    // The PC layout's own fill (pnl_mainmenu) lies under LBL_3DVIEW, whose scene covers the whole
    // panel whenever it draws; a frame presented without that scene shows black, not the art.
    if (sLayout.m_sString != 0 && strcmp(sLayout.m_sString, "mainmenu8x6_p") == 0)
        m_pBackground->m_params.SetFillImage(CResRef(""), 0);
#endif

#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // A reload resets the menu's d-pad links, so restore the ones that wrap past the layout's
    // tag order; ConnectControlMoves has just read their pointers as control ids.
    if (bRelayout)
    {
        m_btnRegistration.SetMoveToControl(2, &m_btnNewGame);
        m_btnRegistration.SetMoveToControl(0, &m_btnMoreGames);
        if (HasTouchScreen())
        {
            m_btnMoreGames.SetMoveToControl(2, &m_btnRegistration);
            m_btnNewGame.SetMoveToControl(0, &m_btnRegistration);
        }
        else
        {
            m_btnGPlay.SetMoveToControl(2, &m_btnNewGame);
            m_btnNewGame.SetMoveToControl(0, &m_btnGPlay);
        }
    }
#endif

    // One-shot: the notch inset is restored when the first layout load completes.
    static bool s_bRenderOverNotchToggled = false;
    if (!s_bRenderOverNotchToggled)
    {
        s_bRenderOverNotchToggled = true;
        ToggleRenderOverNotch(true);
    }
}
#endif

void CSWGuiMainMenu::OnPanelAdded()
{
    unsigned long long nFreeBytes;
    unsigned long long nRequired;
    g_pExoResMan->GetFreeDiskSpace(CExoString("SAVES:"), &nFreeBytes);

    CExoString sSaveDir;
    sSaveDir.Format(const_cast<char*>("SAVES:%06d - %s"), 1, "AUTOSAVE");

    if (g_pExoResMan->CreateDirectory2(sSaveDir) != 0)
    {
        g_pExoResMan->NukeDirectory(sSaveDir, 1, 1);
        nRequired = 4802;
    }
    else
    {
        nRequired = 3202;
    }

    unsigned long long nFreeKB = nFreeBytes >> 14;
    if (nRequired > nFreeKB)
    {
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        CSWGuiMessageBox* pBox = pApp->GetInGameGui()->m_pSoloModeQueryBox;

        CExoString sMessage;
        CExoString sText;
        sText.Format(const_cast<char*>("%2.3f"), static_cast<double>(static_cast<float>(static_cast<int>(nRequired - nFreeKB) << 14) * 1e-6f));
        g_pTlkTable->SetCustomToken(0, sText);
        sMessage = g_pTlkTable->GetSimpleString(STRREF_NOT_ENOUGH_SPACE_TO_SAVE);
        g_pTlkTable->ParseStr(sMessage);

        pBox->SetAllowCancel(0);
        pBox->SetMessage(CExoString(sMessage));
        pBox->SetCallback(this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::LaunchDashboard), 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
    else
    {
        g_pAppManager->m_pClientExoApp->ShowMouse(0xffff);

#if KOTOR1
        // K1: the continue button shows (and takes the focus) only when a save exists. The menu
        // column is one wrapping ring; the Exit (store link) button leaves it when m_bShowExit is
        // clear and Google Play takes its slot. No Amazon variant of the port-button ring.
        CExoString sSave;
        CExoString sName;
        bool bFound = FindNewestSaveGame(sSave, sName);
        m_btnContinue.m_tail.bVisible = bFound;
        if (bFound)
            SetActiveControl(&m_btnContinue, 0);
        else
            SetActiveControl(&m_btnNewGame, 0);

        m_btnNewGame.SetMoveToControl(0, &m_btnExit);
        m_btnNewGame.SetMoveToControl(2, &m_btnLoadGame);
        m_btnLoadGame.SetMoveToControl(0, &m_btnNewGame);
        m_btnLoadGame.SetMoveToControl(2, &m_btnMovies);
        m_btnMovies.SetMoveToControl(0, &m_btnLoadGame);
        m_btnMovies.SetMoveToControl(2, &m_btnOptions);
        m_btnOptions.SetMoveToControl(0, &m_btnMovies);
        m_btnOptions.SetMoveToControl(2, &m_btnExit);
        m_btnExit.SetMoveToControl(0, &m_btnOptions);
        m_btnExit.SetMoveToControl(2, &m_btnGPlay);
        m_btnGPlay.SetMoveToControl(0, &m_btnExit);
        m_btnGPlay.SetMoveToControl(2, &m_btnNewGame);
        if (bFound)
        {
            m_btnContinue.SetMoveToControl(0, &m_btnGPlay);
            m_btnContinue.SetMoveToControl(2, &m_btnNewGame);
            m_btnNewGame.SetMoveToControl(0, &m_btnContinue);
            m_btnGPlay.SetMoveToControl(2, &m_btnContinue);
        }
        if (m_bShowExit == 0)
        {
            m_btnExit.m_tail.bVisible = 0;
            m_btnGPlay.SetMoveToControl(0, &m_btnOptions);
            m_btnOptions.SetMoveToControl(2, &m_btnGPlay);
            m_btnGPlay.SetExtent(m_btnExit.m_extent);
        }
#ifndef K2_UI_TOUCH
        // pc / xbox: the continue and Google Play buttons are not on the panel -- the ring closes
        // on Quit, and New Game takes the focus.
        m_btnExit.SetMoveToControl(2, &m_btnNewGame);
        m_btnNewGame.SetMoveToControl(0, &m_btnExit);
        SetActiveControl(&m_btnNewGame, 0);
#endif

        m_btnAchieve.SetMoveToControl(0, &m_btnBack);
        m_btnAchieve.SetMoveToControl(2, &m_btnSignIn);
        m_btnSignIn.SetMoveToControl(0, &m_btnAchieve);
        m_btnSignIn.SetMoveToControl(2, &m_btnBack);
        m_btnBack.SetMoveToControl(0, &m_btnSignIn);
        m_btnBack.SetMoveToControl(2, &m_btnAchieve);
#else
        // The navigation ring around the three port buttons. The Amazon build has no Sign-In
        // button, so Achieve and Back point at each other; everywhere else Sign-In sits in the
        // middle.
        if (IsAmazon())
        {
            m_btnAchieve.SetMoveToControl(0, &m_btnBack);
            m_btnAchieve.SetMoveToControl(2, &m_btnBack);
            m_btnBack.SetMoveToControl(0, &m_btnAchieve);
            m_btnBack.SetMoveToControl(2, &m_btnAchieve);
        }
        else
        {
            m_btnAchieve.SetMoveToControl(0, &m_btnBack);
            m_btnAchieve.SetMoveToControl(2, &m_btnSignIn);
            m_btnSignIn.SetMoveToControl(0, &m_btnAchieve);
            m_btnSignIn.SetMoveToControl(2, &m_btnBack);
            m_btnBack.SetMoveToControl(0, &m_btnSignIn);
            m_btnBack.SetMoveToControl(2, &m_btnAchieve);
        }
#endif
    }
}

void CSWGuiMainMenu::DisplayErrorMessage(CErrorMessageData& data)
{
    m_errorMessages.Add(data);

    if (m_errorMessages.m_count == 1)
    {
        DisplayCurrentErrorMessage();
    }
}

void CSWGuiMainMenu::DisplayCurrentErrorMessage()
{
#if !KOTOR1  // K1 has no idle-timer gate
    m_bIdleTimerDisabled = 1;
#endif

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    CErrorMessageData* pData = m_errorMessages.m_pArray;

    pBox->SetAllowCancel(pData->m_bAllowCancel);
    if (pData->m_bHideButtons != 0)
    {
        pBox->HideButtons();
    }
    pBox->SetMessage(pData->m_sMessage);
    pBox->SetCallback(this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::HandleErrorMessageExit), 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

#if !KOTOR1  // K2-only: ShowVersionNumber / ShowDeviceId are absent from K1
void CSWGuiMainMenu::ShowVersionNumber()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetVersionNumber", "()Ljava/lang/String;");
    jstring sVer = static_cast<jstring>(env->CallStaticObjectMethod(cls, mid));

    CExoString sVersion(env->GetStringUTFChars(sVer, 0));

    CSWGuiTextParams textParams;
    textParams.SetAlignment(SWGUIALIGN_MIDDLE | SWGUIALIGN_LEFT);
    textParams.SetText(sVersion);
    textParams.SetColor(Vector(0.5f, 0.5f, 0.5f));
    textParams.m_fAlpha = 0.2f;
    textParams.SetBaseFont(CResRef("fnt_d16x16"));

    CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(), 0, 0, 0, 1.0f,
                                    Vector(1.0f, 1.0f, 1.0f), 0, 0);
    borderParams.m_fAlpha = 0.0f;

    m_lblVersion.Initialize(CSWGuiExtent(22, 580, 40, 20), textParams, borderParams, 1.0f);
    InitControl(m_lblVersion, 1, 1, CExoString(static_cast<const char*>(0)), 0);
}

void CSWGuiMainMenu::ShowDeviceId()
{
    OutputDebugPrint("Vendor id:%s", gVendorId.c_str());

    CExoString sVendor(gVendorId.c_str());

    CSWGuiTextParams textParams;
    textParams.SetAlignment(SWGUIALIGN_MIDDLE | SWGUIALIGN_LEFT);
    textParams.SetText(sVendor);
    textParams.SetColor(Vector(0.5f, 0.5f, 0.5f));
    textParams.m_fAlpha = 0.5f;
    textParams.SetBaseFont(CResRef("fnt_d16x16"));

    CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(), 0, 0, 0, 1.0f,
                                    Vector(1.0f, 1.0f, 1.0f), 0, 0);
    borderParams.m_fAlpha = 0.0f;

    int nY = static_cast<int>(static_cast<float>(g_nScreenHeight) * 0.85f);
    m_lblDeviceId.Initialize(CSWGuiExtent(25, nY, 240, 30), textParams, borderParams, 1.0f);
    InitControl(m_lblDeviceId, 1, 1, CExoString(static_cast<const char*>(0)), 0);
}
#endif

void CSWGuiMainMenu::HandleErrorMessageExit(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);

#if !KOTOR1  // K1 has no idle-timer gate
    m_bIdleTimerDisabled = 0;
#endif

    CErrorMessageData* pData = m_errorMessages.m_pArray;
    if (pData->m_pfnCallback)
    {
        (pData->m_pCallbackObject->*pData->m_pfnCallback)(0);
    }

    m_errorMessages.DelIndex(0);

    if (m_errorMessages.m_count != 0)
    {
        DisplayCurrentErrorMessage();
    }
}

#if !KOTOR1  // K2-only: AddButton is absent from K1
void CSWGuiMainMenu::AddButton(CSWGuiButton* pButton, CSWGuiExtent extent, CExoString sName)
{
    pButton->m_tail.nControlId = -1;

    pButton->Initialize(extent, static_cast<CSWGuiButton*>(m_lbModules.m_pProtoItem));
    pButton->m_text.m_params.SetText(sName);
    pButton->SetExtent(extent);

    InitControl(*pButton, 1, 1, CExoString(static_cast<const char*>(0)), 0);
}
#endif

void CSWGuiMainMenu::PopulateModules()
{
    CExoString sName;
    CExoString sA;
    CExoString sB;
    CExoArrayList<CExoString> aModsB;
    CExoArrayList<CExoString> aModsA;
    CExoArrayList<CSWGuiControl*> aControls;
    CSWGuiExtent extent = m_lbModules.m_innerExtent;
    CExoString sUnused;

#if !KOTOR1  // K1 has no override-folder module lists
    int i = 0;
    const char* pszOverridePath;
    while ((pszOverridePath = GetOverridePath(i, false)) != 0)
    {
        char szPath[200];
        sprintf(szPath, "%s\\modules", pszOverridePath);
        g_pExoBase->GetDirectoryList(&aModsA, CExoString(szPath), RESTYPE_MOD, 0, 1);
        g_pExoBase->GetDirectoryList(&aModsB, CExoString(szPath), RESTYPE_RIM, 0, 1);
        i++;
    }
#endif

#if KOTOR1  // K1: the six-argument GetDirectoryList
    g_pExoBase->GetDirectoryList(&aModsA, CExoString("MODULES:"), RESTYPE_MOD, 0, 1, 0);
    g_pExoBase->GetDirectoryList(&aModsB, CExoString("MODULES:"), RESTYPE_RIM, 0, 1, 0);
#else
    g_pExoBase->GetDirectoryList(&aModsA, CExoString("MODULES:"), RESTYPE_MOD, 0, 1);
    g_pExoBase->GetDirectoryList(&aModsB, CExoString("MODULES:"), RESTYPE_RIM, 0, 1);
#endif

    CExoString sAlias;
    for (int nPkg = 1; nPkg < CTlkTable::NUM_CONTENT_PKGS; nPkg++)
    {
        sAlias.Format(const_cast<char*>("live%d"), nPkg);

#if KOTOR1  // K1: CExoString::GetLength is out of line
        if (g_pExoBase->m_pAliasList->GetAliasPath(sAlias, 0).GetLength() < 1)
#else
        const char* pszAliasPath = g_pExoBase->m_pAliasList->GetAliasPath(sAlias, 0).m_sString;
        if (pszAliasPath == 0 || static_cast<int>(strlen(pszAliasPath)) < 1)
#endif
        {
            continue;
        }

        sAlias = sAlias + CExoString(":modules");
#if KOTOR1
        g_pExoBase->GetDirectoryList(&aModsA, sAlias, RESTYPE_MOD, 0, 1, 0);
        g_pExoBase->GetDirectoryList(&aModsB, sAlias, RESTYPE_RIM, 0, 1, 0);
#else
        g_pExoBase->GetDirectoryList(&aModsA, sAlias, RESTYPE_MOD, 0, 1);
        g_pExoBase->GetDirectoryList(&aModsB, sAlias, RESTYPE_RIM, 0, 1);
#endif
    }

    int nIdxA = 0;
    int nIdxB = 0;
    while (nIdxA < aModsA.m_count || nIdxB < aModsB.m_count)
    {
        if (nIdxB < aModsB.m_count)
        {
            sA = aModsB.m_pArray[nIdxB];
            if (sA.Find(CExoString("_s.rim"), 0) != -1)
            {
                nIdxB++;
                continue;
            }
        }
        else
        {
            sA = CExoString("zzzzzzzz.zzz");
        }

        if (nIdxA < aModsA.m_count)
        {
            sB = aModsA.m_pArray[nIdxA];
            if (sB.Find(CExoString("_s.rim"), 0) != -1)
            {
                nIdxA++;
                continue;
            }
        }
        else
        {
            sB = CExoString("zzzzzzzz.zzz");
        }

        sA = sA.SubString(0, sA.GetLength() - 4);
        sB = sB.SubString(0, sB.GetLength() - 4);

        bool bAdvance = true;
        if (sA.LowerCase() <= sB.LowerCase())
        {
            sName = sA;
            nIdxB++;
            bAdvance = (sA == sB);
        }
        else
        {
            sName = sB;
        }

        CSWGuiButton* pButton = new CSWGuiButton();
        pButton->Initialize(extent, static_cast<CSWGuiButton*>(m_lbModules.m_pProtoItem));
        pButton->m_text.m_params.SetText(sName);
        // Called through the base text pointer, so the call stays virtual.
        CSWGuiText* pText = &pButton->m_text;
        extent.h = pText->GetIdealHeight()
                 + pButton->m_border.m_params.GetBorderDim() * 2;
        pButton->SetExtent(extent);
        pButton->AddEvent(INPUTEVENT_GUI_A, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnModulePicked));
        pButton->AddEvent(INPUTEVENT_GUI_START, this, static_cast<CSWGuiEventHandler>(&CSWGuiMainMenu::OnModulePicked));

        aControls.Add(pButton);
        m_aModuleNames.Add(sName);

        nIdxA += bAdvance;
    }

    m_lbModules.AddControls(aControls, 1, 0, 0);
}

void CSWGuiMainMenu::OnModulePicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CExoString sMsg;

    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

    CSWPartyTable::ResetCurrentSessionStartTime();
    g_pAppManager->CreateServer(0);

    g_pAppManager->m_pClientExoApp->GetWorldTimer()->PauseWorldTimer();
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->PauseWorldTimer();

#if !KOTOR1  // K1 has no default PC level / alignment globals
    char nLevel;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
        ->GetValueNumber(CExoString("G_PC_LEVEL"), nLevel);
    if (nLevel == 0)
    {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("G_PC_LEVEL"), 12);
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("G_PC_Align_Val"), 70);
    }
#endif

    pApp->GetNetLayer()->StartConnectToSession(0, pApp->GetClientName(), CExoString(""),
                                               10, CNetLayer::CONNECTTYPE_PLAYER);

    pApp->GetSWCMessage()->SendPlayerToServerStatus_RequestStatus();

    sMsg.Format(const_cast<char*>("%c%s.%s %s"), 's', "Module", "Load",
                m_aModuleNames[m_lbModules.m_nSelectedIndex].CStr());

    pApp->GetNetLayer()->SendMessageToPlayer(CNetLayer::PLAYERID_SERVER,
                                             reinterpret_cast<unsigned char*>(sMsg.CStr()),
                                             sMsg.GetLength(), 0);

    pApp->SetPlayerCharacterName(CExoString("test"));
    pApp->SetInputClass(INPUTCLASS_GAME, 1);
    pApp->DisableInput();

    pApp->SetLoadMusic(pApp->GetLoadMusicByModuleName(
        m_aModuleNames[m_lbModules.m_nSelectedIndex]));
    pApp->SetLoadScreenByModuleName(m_aModuleNames[m_lbModules.m_nSelectedIndex]);
#if KOTOR1
    pApp->ShowLoadScreen(1, 1, 0);
#else
    pApp->ShowLoadScreen(1, 1, 0, 1);
#endif
    pApp->SetLoadScreenHint(pApp->GetNextLoadScreenHintSTRREF(
        m_aModuleNames[m_lbModules.m_nSelectedIndex], 0));

    DeleteModuleList();
    m_lbModules.ClearItems();

    SetToBeDeleted(0);
#if !KOTOR1  // K1 does not clear the main-menu pointer
    g_pAppManager->m_pClientExoApp->ClearMainMenuPointer();
#endif
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(1);
}

void CSWGuiMainMenu::DeleteModuleList()
{
    while (m_aModuleNames.m_count > 0)
        m_aModuleNames.DelIndex(0);

    m_aModuleNames.SetSize(0);
}

#if !KOTOR1  // K2-only: K1 has no Facebook button
void CSWGuiMainMenu::UpdateFacebookButtonStatus()
{
    if (g_facebookLoggedIn != 0)
    {
        m_btnFacebook.m_text.m_params.SetText("Logout");
    }
    else
    {
        m_btnFacebook.m_text.m_params.SetText("Facebook Login");
    }
}
#endif

template void CExoArrayList<CSWGuiMainMenu::CErrorMessageData>::Allocate(int);

