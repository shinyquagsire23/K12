// The title screen's music player page shows the "MusicTable" 2DA as a listbox with one button per
// unlocked song, a volume slider, a now-playing label and the Play/Loop/Stop/Next transport.
// PopulateSongList walks the 2DA and builds one row per accepted song; the transport handlers drive
// a CExoStreamingSoundSource the constructor allocates.

#include "swguimusic.h"
#include "strref.h"

#include "twodimarray.h"
#include "appmanager.h"
#include "clientoptions.h"
#include "exobase.h"
#include "exoiniinternal.h"
#include "exosound.h"
#include "exotimersinternal.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "exoresman.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "port/k2qolapi.h"
#include "exoinput.h"

// K1 has no title-screen music page.
#if !KOTOR1

#include "port/libcbase.h"
#include "exostringinline.h"

extern CExoSound* g_pExoSound;

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

float GetResolutionTextScalar();

// Loads "titlemusic_p", binds the 15 layout controls, wires the five transport buttons and the
// slider, reads the music volume and the "UnlockedPlanetSongs" ini key, builds the song list, and
// finally allocates the streaming source.
CSWGuiTitleMusic::CSWGuiTitleMusic(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_pSelectedRow = 0;
    CSWGuiControl* pVolumeControl = &m_sliderVolume;
    int nMax;
    int nCur;

    StartLoadFromLayout(CResRef("titlemusic_p"), 1, 1, 1);

    InitControl(m_labelMusicTitle, CExoString("LBL_MUSIC_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnPlay, CExoString("BTN_PLAY"), 1, 1, 0, 0, 0);
    InitControl(m_btnLoop, CExoString("BTN_LOOP"), 1, 1, 0, 0, 0);
    InitControl(m_btnStop, CExoString("BTN_STOP"), 1, 1, 0, 0, 0);
    InitControl(m_btnNext, CExoString("BTN_NEXT"), 1, 1, 0, 0, 0);
    InitControl(m_listBox, CExoString("LB_MUSIC"), 1, 1, 0, 0, 0);
    m_listBox.bSelectOnLeftClick = 1;
    InitControl(m_labelTrackName, CExoString("LBL_TRACKNAME"), 1, 1, 0, 0, 0);
    InitControl(m_labelUnlocked, CExoString("LBL_UNLOCKED"), 1, 1, 0, 0, 0);
    InitControl(m_labelTrackNum, CExoString("LBL_TRACKNUM"), 1, 1, 0, 0, 0);
    InitControl(m_sliderVolume, CExoString("SLI_VOLUME"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitLabel(&m_labelMusicTitle.m_text);
    K2QolFitLabel(&m_labelTrackName.m_text);
    K2QolFitLabel(&m_labelUnlocked.m_text);
    K2QolFitLabel(&m_labelTrackNum.m_text);
#endif

    m_btnPlay.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnPlayPicked));
    m_btnPlay.m_tail.m_nClickSound = GUISOUND_NONE;
    m_btnPlay.m_tail.m_nActivateSound = GUISOUND_NONE;
    m_btnPlay.m_tail.bFocusOnHover = 0;

    m_btnLoop.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnLoopPicked));
    m_btnLoop.m_tail.m_nClickSound = GUISOUND_NONE;
    m_btnLoop.m_tail.m_nActivateSound = GUISOUND_NONE;
    m_btnLoop.m_tail.bFocusOnHover = 0;

    m_btnStop.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnStopPicked));
    m_btnStop.m_tail.m_nClickSound = GUISOUND_NONE;
    m_btnStop.m_tail.m_nActivateSound = GUISOUND_NONE;
    m_btnStop.m_tail.bFocusOnHover = 0;

    m_btnNext.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnNextPicked));
    m_btnNext.m_tail.m_nClickSound = GUISOUND_NONE;
    m_btnNext.m_tail.m_nActivateSound = GUISOUND_NONE;
    m_btnNext.m_tail.bFocusOnHover = 0;

    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnBack.m_tail.m_nClickSound = GUISOUND_NONE;
    m_btnBack.m_tail.m_nActivateSound = GUISOUND_NONE;
    m_btnBack.m_tail.bFocusOnHover = 0;

    m_nMusicVolume = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMusicVolume;

    m_sliderVolume.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnEnterSlider));
    m_sliderVolume.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnExitSlider));
    m_sliderVolume.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnMoveVolumeSlider));
    m_sliderVolume.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnMoveVolumeSlider));
    m_sliderVolume.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnMoveVolumeSlider));
    m_sliderVolume.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnMoveVolumeSlider));
    m_sliderVolume.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnMoveVolumeSlider));

    m_sliderVolume.SetCurValue(m_nMusicVolume);
    nMax = pVolumeControl->AsSlider()->m_nMaxValue;
    nCur = pVolumeControl->AsSlider()->m_nCurValue;
    g_pExoSound->SetMusicVolume(static_cast<float>(nCur) / static_cast<float>(nMax) + 0.0f, 0);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMusicVolume = static_cast<unsigned char>(nCur);

    // The "UnlockedPlanetSongs" bit field. Element 0 is always unlocked; elements 1..10 each take
    // their own bit.
    CExoIni ini;
    CExoString sUnlockedSongs;
    ini.ReadIniEntry(sUnlockedSongs, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                     CExoString("UnlockedPlanetSongs"));
    int nBits = sUnlockedSongs.AsINT();
    m_aUnlockedPlanets[1] = nBits & (1 << 1);
    m_aUnlockedPlanets[2] = nBits & (1 << 2);
    m_aUnlockedPlanets[3] = nBits & (1 << 3);
    m_aUnlockedPlanets[4] = nBits & (1 << 4);
    m_aUnlockedPlanets[5] = nBits & (1 << 5);
    m_aUnlockedPlanets[6] = nBits & (1 << 6);
    m_aUnlockedPlanets[7] = nBits & (1 << 7);
    m_aUnlockedPlanets[8] = nBits & (1 << 8);
    m_aUnlockedPlanets[9] = nBits & (1 << 9);
    m_aUnlockedPlanets[10] = nBits & (1 << 10);
    m_aUnlockedPlanets[0] = 1;

    PopulateSongList();
    m_nNumSongs = m_listBox.m_aControls.m_count;
    SetActiveControl(&m_listBox, 0);

    m_nPriority = 8;
    m_nLooping = 0;
    m_nPlaying = 0;
    m_labelUnlocked.m_text.m_params.SetStrRef(STRREF_UNLOCKED);
    m_labelTrackName.m_text.m_params.SetText(CExoString(""));
    m_pMusic = new CExoStreamingSoundSource(0);
    m_fTimer = 0.0f;

    AspyrAddButtonIcon(&m_btnPlay, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnLoop, 'x');
    AspyrAddButtonIcon(&m_btnStop, 'y');
}

// The 2DA walk. Every row of the "MusicTable" contributes a CMusicItemInfo, but only when its
// UnlockPlanet column indexes m_aUnlockedPlanets (<= 10) and that entry is non-zero; a second pass
// then builds one CSWGuiButton per accepted song and hands them to the listbox in one AddControls.
void CSWGuiTitleMusic::PopulateSongList()
{
    C2DA* pTable = g_pRules->m_p2DArrays->m_pMusic;
    int nRows = pTable->m_nRows;

    CMusicItemInfo info;
    for (int i = 0; i < nRows; i++)
    {
        CExoString sFileName;
        int nStrrefName = 0;
        int nUnlockPlanet = 0;

        pTable->GetCExoStringEntry(i, CExoString("FileName"), &sFileName);
        pTable->GetINTEntry(i, CExoString("StrrefName"), &nStrrefName);
        pTable->GetINTEntry(i, CExoString("UnlockPlanet"), &nUnlockPlanet);

        if (static_cast<unsigned>(nUnlockPlanet) <= 10 && m_aUnlockedPlanets[nUnlockPlanet] != 0)
        {
            info.m_sFileName = sFileName;
            info.m_nStrrefName = nStrrefName;
            m_aSongs.Add(info);
        }
    }

    CExoArrayList<CSWGuiControl*> aRows;
    CSWGuiExtent extent = m_listBox.m_innerExtent;
    for (int i = 0; i < m_aSongs.m_count; i++)
    {
        CSWGuiButton* pRow = new CSWGuiButton();
        pRow->Initialize(extent, static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem));
        pRow->m_text.m_params.SetStrRef(m_aSongs.m_pArray[i].m_nStrrefName);
        extent.h = 36;
        pRow->m_tail.m_nUserData = reinterpret_cast<unsigned long long>(&m_aSongs.m_pArray[i]);
        extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
        pRow->SetExtent(extent);
        pRow->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnPlayPicked));
        pRow->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMusic::OnSongPicked));
        pRow->m_tail.bFocusOnHover = 0;
        aRows.Add(pRow);
    }

    m_listBox.AddControls(aRows, 1, 0, 0);
}

// Re-arms the loop flag, restarts the song the selected row names (its m_nUserData is the
// CMusicItemInfo, whose CExoString member converts to the CResRef), and mirrors the row's caption
// into the now-playing label. m_nPlaying = 1 runs even when the block above is skipped.

void CSWGuiTitleMusic::OnPlayPicked(CSWGuiControl* pControl)
{
    if (m_aSongs.m_count > 0 && m_pSelectedRow != 0)
    {
        if (pControl->m_tail.nCurrentEventArg != 0)
        {
            CMusicItemInfo* pSong = reinterpret_cast<CMusicItemInfo*>(m_pSelectedRow->m_tail.m_nUserData);
            m_pMusic->SetLooping(m_nLooping);
            if (m_pMusic->IsPlaying())
                StopMusic();
            m_pMusic->Play(CResRef(pSong->m_sFileName), static_cast<unsigned short>(m_nPriority), 0, 1, 0);
            m_labelTrackName.m_text.m_params.SetText(
                static_cast<CSWGuiButton*>(m_pSelectedRow)->m_text.m_params.m_sText);
        }
        m_nPlaying = 1;
    }
}

// Toggles looping, pulsing the highlight border while it is on.
void CSWGuiTitleMusic::OnLoopPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (m_nLooping == 0)
    {
        m_pMusic->SetLooping(1);
        m_nLooping = 1;
        m_btnLoop.m_border.m_params.SetPulsingAlpha(1, 1, 0);
        m_btnLoop.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    }
    else
    {
        m_pMusic->SetLooping(0);
        m_nLooping = 0;
        m_btnLoop.m_border.m_params.SetPulsingAlpha(0, 0, 0);
        m_btnLoop.m_borderHilight.m_params.SetPulsingAlpha(0, 0, 0);
    }
}

// Stops the transport, but only when something is actually playing.
void CSWGuiTitleMusic::OnStopPicked(CSWGuiControl* pControl)
{
    if (m_aSongs.m_count > 0 && pControl->m_tail.nCurrentEventArg != 0 && m_pMusic->IsPlaying())
        StopMusic();
}

// Arms the grace timer on a click, then advances the listbox selection with a wrap, announces the
// newly selected row to OnSongPicked, and starts the song.
void CSWGuiTitleMusic::OnNextPicked(CSWGuiControl* pControl)
{
    if (pControl != 0 && pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_A)
    {
        // A running timer returns here, debouncing the click; it does not skip just the store.
        if (m_fTimer != 0.0f)
            return;
        m_fTimer = 1.0f;
    }

    int nIndex = 0;
    CSWGuiListBox* pList = &m_listBox;
    if (m_pSelectedRow != 0)
    {
        int nNumRows = pList->m_aControls.m_count;
        pList->SetSelectedControl(m_pSelectedRow, 0);
        int nSelected = pList->m_nSelectedIndex;
        if (nSelected + 1 < nNumRows)
            nIndex = nSelected + 1;
    }
    pList->SetSelectedControl(nIndex, 0);
    OnSongPicked(pList->GetSelectedControl());

    CMusicItemInfo* pSong = reinterpret_cast<CMusicItemInfo*>(m_pSelectedRow->m_tail.m_nUserData);
    m_pMusic->SetLooping(m_nLooping);
    if (m_pMusic->IsPlaying())
        StopMusic();
    m_pMusic->Play(CResRef(pSong->m_sFileName), static_cast<unsigned short>(m_nPriority), 0, 1, 0);
    m_labelTrackName.m_text.m_params.SetText(
        static_cast<CSWGuiButton*>(m_pSelectedRow)->m_text.m_params.m_sText);
    m_nPlaying = 1;
}

// The slider focus pair; both reach the thumb image's pulsing-alpha flag through AsSlider().

void CSWGuiTitleMusic::OnEnterSlider(CSWGuiControl* pControl)
{
    pControl->AsSlider()->m_imageThumb.m_params.bHilight = 1;
}

void CSWGuiTitleMusic::OnExitSlider(CSWGuiControl* pControl)
{
    pControl->AsSlider()->m_imageThumb.m_params.bHilight = 0;
}

// Applies the slider's value as the live music volume. Both AsSlider() calls are real: a virtual
// call cannot be commoned, and the "+ 0.0f" term is in the bytes.
void CSWGuiTitleMusic::OnMoveVolumeSlider(CSWGuiControl* pControl)
{
    int nMax = pControl->AsSlider()->m_nMaxValue;
    int nCur = pControl->AsSlider()->m_nCurValue;
    g_pExoSound->SetMusicVolume(static_cast<float>(nCur) / static_cast<float>(nMax) + 0.0f, 0);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMusicVolume = static_cast<unsigned char>(nCur);
}

// Advances to the next song when the current one ended and looping is off; the slider then tracks
// its own value, the base draws, and the grace timer counts down (never below zero).
void CSWGuiTitleMusic::Draw(float f)
{
    if (m_nPlaying != 0 && m_nLooping == 0 && m_pMusic->IsDone())
        OnNextPicked(0);

    m_sliderVolume.SetCurValue(m_sliderVolume.m_nCurValue);
    CSWGuiPanel::Draw(f);

    if (m_fTimer > 0.0f)
    {
        m_fTimer = m_fTimer - f;
        if (m_fTimer < 0.0f)
            m_fTimer = 0.0f;
    }
}

// A zero value skips the whole dispatch, and every arm falls into the base call.

void CSWGuiTitleMusic::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            if (m_pMusic->IsPlaying())
                m_pMusic->Stop(0);
            g_pAppManager->m_pClientExoApp->SetStartMenuMusicOnMovieEnd(1);
            g_pAppManager->m_pClientExoApp->StartMenuMusic(1, 127, 2);
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            m_pGuiManager->RemovePanel(this);
            break;
        case INPUTEVENT_GUI_X:
            m_btnLoop.m_tail.nCurrentEventArg = 1;
            OnLoopPicked(&m_btnLoop);
            break;
        case INPUTEVENT_GUI_Y:
            m_btnStop.m_tail.nCurrentEventArg = 1;
            if (m_aSongs.m_count > 0 && m_pMusic->IsPlaying())
                StopMusic();
            break;
        case INPUTEVENT_GUI_DPAD_LEFT:
            m_sliderVolume.HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, 1);
            break;
        case INPUTEVENT_GUI_DPAD_RIGHT:
            m_sliderVolume.HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, 1);
            break;
        case INPUTEVENT_GUI_RIGHT_SHOULDER:
            OnNextPicked(0);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// A row was picked: repaint the outgoing selection green, tint the incoming one, and restamp the
// "%d/%d" position label.
void CSWGuiTitleMusic::OnSongPicked(CSWGuiControl* pControl)
{
    if (m_pSelectedRow != 0)
    {
        static_cast<CSWGuiButton*>(m_pSelectedRow)->m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        static_cast<CSWGuiButton*>(m_pSelectedRow)->m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    }

    m_pSelectedRow = pControl->AsButton();
    static_cast<CSWGuiButton*>(m_pSelectedRow)->m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
    // COLOR_NEW_BONUS is typed CSWGuiColor here, but this call takes its address as a Vector: both
    // are the same 12-byte float triple, so the pun is intended.
    static_cast<CSWGuiButton*>(m_pSelectedRow)->m_text.m_params.SetDefaultColor(
        *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));

    int nTrack = m_listBox.m_nSelectedIndex + 1;
    CExoString sPosition;
    sPosition.Format(const_cast<char*>("%d/%d"), nTrack, m_nNumSongs);
    m_labelTrackNum.m_text.m_params.SetText(sPosition);
}

// Fades the source out over 500 ms, pumping the mixer until the wall clock has moved on, and
// clears the now-playing label. m_nPlaying is cleared first, so a Draw during the wait cannot
// re-enter the auto-advance.
void CSWGuiTitleMusic::StopMusic()
{
    m_nPlaying = 0;
    int nStart = static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    m_pMusic->FadeAndStop(500);
    do
    {
        g_pExoSound->Render();
    } while (static_cast<unsigned int>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000) - nStart) < 500);
    m_labelTrackName.m_text.m_params.SetText(CExoString(""));
}

// Stops the transport, frees the sound source and the song table, and saves the volume back into
// CClientOptions.
CSWGuiTitleMusic::~CSWGuiTitleMusic()
{
    if (m_pMusic->IsPlaying())
        m_pMusic->Stop(0);
    if (m_pMusic != 0)
        delete m_pMusic;

    m_aSongs.RemoveAll(1);

    g_pExoSound->SetMusicVolume(static_cast<float>(m_nMusicVolume) / static_cast<float>(m_sliderVolume.m_nMaxValue) + 0.0f, 0);
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_nMusicVolume = static_cast<unsigned char>(m_nMusicVolume);
}

// The CMusicItemInfo instantiation. Allocate's body must stay out of every function that uses the
// array, so the impl header is pulled in only here, after them.
#include "CExoArrayListImpl.h"

template void CExoArrayList<CMusicItemInfo>::Allocate(int);

#endif // !KOTOR1
