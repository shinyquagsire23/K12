// CExoMoviePlayerInternal's movie-player pimpl leaf accessors. See the header for the model.

#include "k2stdint.h"
#include "port/aslglapi.h"
#include "exomovieplayerinternal.h"
#include "exostring.h"
#include "exocritsect.h"
#include "CExoArrayListDtors.h"
#if KOTOR1
#include "CExoArrayListImpl.h"
#endif
#include "exobase.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "clientoptions.h"
#include "exoinput.h"
#include "restype.h"
#include "port/win32const.h"
#include "exoinputinternal.h"
#include "include/aurora.h"
#include "aurinternal.h"
#include "aurglcore.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwcambientsound.h"

// Renderer entry points and screen state used by ShowFrame, declared here rather than by pulling
// the GL headers in.

extern int g_nScreenWidth;
extern int g_nScreenHeight;

#include "port/libcstring.h"

// Defined in the ClientExoApp TU.
char* FindMovieInOverrides(const char* pszMovieName);
#if KOTOR1
// K1: the movie plays through MacPlayBinkGL, and the intro's end reaches an analytics screen.
void MacPlayBinkGL(const char* pszMovie, bool bSound, bool& bStarted, int nVolume);
void ASLAnalyticsTrackScreen(const char* pszScreen);
#endif

// The movie volume a full-scale option maps to (Bink's 1.0).
enum {
    BINK_FULL_VOLUME = 32768,
};

// Aurora's software movie-player entry point. The trailing argument is always null here.
namespace ASL {
int PlayBinkMovieGL(const char* pszMovie, float fVolume, unsigned int a, unsigned int b,
                    bool* pbOut, bool* pbQuit);
}

// Bink 1.x ships statically linked and stripped of its dynamic symbols, so its prototypes are
// declared here.
#if KOTOR1
struct BinkSummary { unsigned char _bytes[0x90]; };
#else
struct BinkSummary { unsigned char _bytes[0xf8]; };
#endif

// An opaque decoder handle; only the two frame counters are ever read, by the render loop's gate.
#if KOTOR1
struct BinkHandle { char _pad00[8]; int m_nFrames; int m_nFrameNum; };
#else
struct BinkHandle { char _pad00[0x10]; void* m_nFrames; void* m_nFrameNum; };
#endif

#include "port/win32api.h"

// The player the play thread is driving.
static CExoMoviePlayerInternal* g_pAsyncMoviePlayer;

extern "C" void BinkGetSummary(void* pBink, BinkSummary* pSummary);
#if KOTOR1
extern "C" void BinkPause(void* pBink, int bPause);
#else
extern "C" void BinkPause(void* pBink, int64_t bPause);
#endif
extern "C" void BinkClose(void* pBink);
extern "C" int64_t BinkNextFrame(void* pBink);
extern "C" void BinkDoFrame(void* pBink);
#if KOTOR1
extern "C" int  BinkWait(void* pBink);
#else
extern "C" int64_t BinkWait(void* pBink);
#endif

#if KOTOR1
// K1: a bare name is looked up under the movie alias (no override search), the volume is passed
// as an int, and an evil ending plays the movie without sound.
int CExoMoviePlayerInternal::StartMovie(const CExoString& sMovie)
{
    char* pszExtension = strchr(sMovie.CStr(), '.');
    m_sMoviePath = sMovie;
    if (pszExtension == 0)
    {
        if (strchr(m_sMoviePath.CStr(), ':') == 0)
        {
            m_sMoviePath.Format(const_cast<char*>("MOVIES:%s"), m_sMoviePath.CStr());
        }
        m_sMoviePath = g_pExoBase->m_pAliasList->ResolveFileName(m_sMoviePath, RESTYPE_BIK);
    }

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    int bEvilEnding = g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress;
    int nVolume = pOptions->m_nMovieVolume * BINK_FULL_VOLUME / 100;
    if (bEvilEnding != 0)
    {
        nVolume = 0;
    }

    bool bStarted = false;
    if (strstr(m_sMoviePath.CStr(), "01c.bik") != 0)
        ASLAnalyticsTrackScreen("Completed Intro Level");
    MacPlayBinkGL(m_sMoviePath.CStr(), bEvilEnding == 0, bStarted, nVolume);
    m_bSkipped = bStarted;
    return 1;
}
#else
int CExoMoviePlayerInternal::StartMovie(const CExoString& sMovie)
{
    char* pszExtension = strchr(sMovie.CStr(), '.');
    m_sMoviePath = sMovie;
    if (pszExtension == 0)
    {
        if (strchr(m_sMoviePath.CStr(), ':') == 0)
        {
            m_sMoviePath = FindMovieInOverrides(m_sMoviePath.CStr());
        }
        m_sMoviePath = g_pExoBase->m_pAliasList->ResolveFileName(m_sMoviePath, RESTYPE_BIK);
    }

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    int nVolume = pOptions->m_nMovieVolume * BINK_FULL_VOLUME / 100;
    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
    {
        nVolume = 0;
    }
    float fVolume = static_cast<float>(nVolume) * (1.0f / 32768.0f);

    bool bStarted = false;
    ASL::PlayBinkMovieGL(m_sMoviePath.CStr(), fVolume, 0, 250, &bStarted, 0);
    m_bSkipped = bStarted;
    return 1;
}
#endif

void CExoMoviePlayerInternal::ShowFrame(int, int, int, int)
{
    double dHalfWidth, dHalfHeight;
    if (g_pAurora->FullScreen() != 0)
    {
        dHalfWidth = 1.0;
        dHalfHeight = 1.0;
    }
    else
    {
        dHalfWidth = g_nScreenWidth / 800.0f;
        dHalfHeight = g_nScreenHeight / 600.0f;
    }

    ASLgl::glClear(GL_COLOR_BUFFER_BIT);  // GL_COLOR_BUFFER_BIT
    GLRender::SetAlphaFunc(Aurora::kCmpAlways, 0.0f);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapBlend);
    ASLgl::glOrtho(-dHalfHeight, dHalfHeight, -dHalfWidth, dHalfWidth, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
    ASLgl::glLoadIdentity();
    GLRender::RenderObjectsVTQuad(m_nVertexPool, 4);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    BinkDoFrame(m_pBink);
}

#if KOTOR1
// K1: decode until the last frame or a cancel, with no unpause, input polling, pause abort or
// message pump.
int CExoMoviePlayerInternal::RenderFrame(int)
{
    if (m_pBink != 0)
    {
        while (static_cast<BinkHandle*>(m_pBink)->m_nFrames != static_cast<BinkHandle*>(m_pBink)->m_nFrameNum)
        {
            BinkNextFrame(m_pBink);
            BinkDoFrame(m_pBink);
            while (BinkWait(m_pBink) == 1)
                Sleep(1);
            if (m_bSkipped == 1)
                break;
        }
        BinkSummary summary;
        BinkGetSummary(m_pBink, &summary);
    }
    return 0;
}
#else
int CExoMoviePlayerInternal::RenderFrame(int)
{
    if (m_pBink == 0)
        return 0;
    UnpauseMovie();

    while (true)
    {
        if (static_cast<BinkHandle*>(m_pBink)->m_nFrames == static_cast<BinkHandle*>(m_pBink)->m_nFrameNum)
            break;

        int nEvents;
        CExoArrayList<CExoInputEvent> events = g_pExoInput->GetEvents(nEvents, 5);
        for (int i = 0; i < nEvents; i++)
        {
            if (events.m_pArray[i].m_nValue == 0)
                continue;
            int nId = events.m_pArray[i].m_pEvent->m_nId;
            if (nId == INPUTEVENT_MOVIE_SKIP_A || nId == INPUTEVENT_MOVIE_SKIP_B || nId == INPUTEVENT_MOVIE_SKIP_X ||
                nId == INPUTEVENT_MOVIE_SKIP_Y)
            {
                m_bSkipped = 1;
                g_pExoInput->ClearEvents();
            }
        }

        BinkNextFrame(m_pBink);
        BinkDoFrame(m_pBink);
        int64_t nWait;
        while ((nWait = BinkWait(m_pBink)) == 1)
        {
            Sleep(static_cast<unsigned int>(nWait));
            if (m_bPaused != 0)
                break;
        }

        if (m_bSkipped == 1 || m_bPaused != 0)
            break;

        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, 0) != 0)
        {
            if (GetMessageA(&msg, 0, 0, 0) == 0)
                break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    BinkSummary summary;
    BinkGetSummary(m_pBink, &summary);
    return 0;
}
#endif

void AsyncPlayThreadProc(void* pArg)
{
    g_pAsyncMoviePlayer = static_cast<CExoMoviePlayerInternal*>(pArg);
    if (pArg == 0)
    {
        ExitThread(0);
#ifdef K2_FIX_ENGINE_BUGS
        return;
#endif
    }

    CExoMoviePlayerInternal* pPlayer = static_cast<CExoMoviePlayerInternal*>(pArg);
#if !KOTOR1
    WaitForSingleObject(pPlayer->m_hThread, INFINITE);
    if (pPlayer->m_bStopThread != 0)
    {
        ExitThread(0);
    }
#endif

    while (true)
    {
#if KOTOR1  // K1: ExitThread returns, so the wait stays at the top of the loop
        WaitForSingleObject(pPlayer->m_hThread, INFINITE);
        if (pPlayer->m_bStopThread != 0)
        {
            ExitThread(0);
        }
#endif
        if (pPlayer->m_aMovies.m_count >= 1)
        {
            for (int i = 0; i < pPlayer->m_aMovies.m_count; i++)
            {
                (void)(pPlayer->m_aMovies.m_pArray[i] != "");
            }
            pPlayer->m_bSkippable = 1;

            if (pPlayer->m_aMovies.m_count >= 1)
            {
                for (int i = 0; i < pPlayer->m_aMovies.m_count; i++)
                {
                    CExoString& sMovie = pPlayer->m_aMovies.m_pArray[i];
                    if (!(sMovie != ""))
                        continue;

                    if (pPlayer->m_aInts.m_count > 0)
                    {
                        if (i < pPlayer->m_aInts.m_count)
                            pPlayer->m_bSkippable = pPlayer->m_aInts.m_pArray[i];
                        else
                            pPlayer->m_bSkippable = 1;
                    }

                    pPlayer->StartMovie(sMovie);

                    if (pPlayer->m_bDeactivated == 0)
                        pPlayer->m_aMovies.m_pArray[i] = "";

                    if (pPlayer->m_bSkippable == 0)
                    {
                        pPlayer->m_bSkipped = 0;
                        continue;
                    }

                    if (pPlayer->m_bSkipped != 1)
                        continue;

                    if (pPlayer->m_bCancelAll != 0)
                        break;

                    pPlayer->m_bSkipped = 0;
                }
            }
        }
        else
        {
            pPlayer->m_bSkippable = 1;
        }

        if (pPlayer->m_bDeactivated == 0)
            pPlayer->m_aMovies.SetSize(0);

        pPlayer->m_bSkipped = 0;
        pPlayer->m_bCancelAll = 0;
        pPlayer->m_bMoviePlaying = 0;
#if !KOTOR1
        WaitForSingleObject(pPlayer->m_hThread, INFINITE);

        if (pPlayer->m_bStopThread != 0)
        {
            ExitThread(0);
        }
#endif
    }
}

void CExoMoviePlayerInternal::PlayMoviesAsync(const CExoArrayList<CExoString>& movies, int bLoop,
                                              const CExoArrayList<int>* pChapters)
{
    m_aMovies = movies;
    m_bLoop = bLoop;
    CExoArrayList<int>& aInts = m_aInts;
#if KOTOR1  // K1 empties the chapter list and re-adds the caller's entries one by one
    aInts.m_count = 0;
    if (pChapters != 0)
    {
        for (int i = 0; i < pChapters->m_count; i++)
            aInts.Add(pChapters->m_pArray[i]);
    }
#else
    if (pChapters != 0)
    {
        aInts = *pChapters;
    }
    else
    {
        aInts.m_count = 0;
    }
#endif
    m_bMoviePlaying = 1;

    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetClientOptions() != 0
        && g_pAppManager->m_pClientExoApp->GetModule() != 0
        && g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0)
    {
        g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->StopSounds(1);
    }

    for (int i = 0; i < m_aMovies.m_count; i++)
    {
        CExoString& sMovie = m_aMovies.m_pArray[i];
        if (sMovie != "")
        {
            if (m_aInts.m_count > 0)
            {
                if (i < m_aInts.m_count)
                    m_bSkippable = m_aInts.m_pArray[i];
                else
                    m_bSkippable = 1;
            }
            StartMovie(sMovie);
            if (m_bSkippable != 0)
            {
                if (m_bSkipped != 1)
                    continue;
                if (m_bCancelAll != 0)
                    break;
            }
            m_bSkipped = 0;
        }
    }

    m_aMovies.SetSize(0);
    m_bSkipped = 0;
    m_bCancelAll = 0;
    m_bMoviePlaying = 0;
}

struct HWND__* g_hMovieWnd;

// The record the render window posts to the movie window as the lParam of WM_WINDOWPOSCHANGING and
// WM_WINDOWPOSCHANGED; the receiving arm only frees it.
struct HWND__;
extern HWND__* g_hWnd;
CExoMoviePlayerInternal::CExoMoviePlayerInternal()
{
    m_sReserved1 = "";
    m_bStopThread = 0;
    m_bMoviePlaying = 0;
    m_nReserved2 = 0;
    m_pReserved4 = 0;
#if !KOTOR1  // K1 leaves the pause pin uninitialised
    m_bPaused = 0;
#endif
    m_bSkipped = 0;
    m_bCancelAll = 0;
    m_bDeactivated = 0;
    m_nDeactivationArg = 0;
    m_bShuttingDown = 0;
    m_pBink = 0;
    m_movieWindow = 0;
    m_pReserved3 = 0;
}

// Commits the last frame's stats and closes the handle unless m_bPaused pinned it. m_bShuttingDown stays
// set for the body so the render loop skips frames while the movie is torn down.
void CExoMoviePlayerInternal::ShutDown()
{
    m_bShuttingDown = 1;
    if (m_pBink != 0)
    {
        BinkSummary summary;
        BinkGetSummary(m_pBink, &summary);
        BinkPause(m_pBink, 1);
#if KOTOR1  // K1: no pause pin -- the handle is always closed
        BinkClose(m_pBink);
        m_pBink = 0;
#else
        if (m_bPaused == 0)
        {
            BinkClose(m_pBink);
            m_pBink = 0;
        }
#endif
        DestroyWindow(m_movieWindow);
        m_movieWindow = 0;
        g_hMovieWnd = 0;
    }
    m_bShuttingDown = 0;
}

CExoMoviePlayerInternal::~CExoMoviePlayerInternal()
{
    m_bStopThread = 1;
    if (m_pReserved3 != 0)
    {
        delete m_pReserved3;
        m_pReserved3 = 0;
    }
    ShutDown();
}

void CExoMoviePlayerInternal::StopMovie()
{
    ShutDown();
    m_bMoviePlaying = 0;
}

// m_bPaused keeps the decoder handle open across PauseMovie/UnpauseMovie.
void CExoMoviePlayerInternal::PauseMovie()
{
    if (m_pBink != 0 && m_bPaused == 0)
    {
        m_bPaused = 1;
        BinkPause(m_pBink, 1);
    }
}

void CExoMoviePlayerInternal::CancelMovie(int nMovie, int bImmediate)
{
#if KOTOR1  // no early return: the movie index is stored once, on both paths
    if (bImmediate || (m_bMoviePlaying == 1 && m_bSkippable))
        m_bSkipped = 1;
    m_bCancelAll = nMovie;
#else
    if (bImmediate || (m_bMoviePlaying == 1 && m_bSkippable))
    {
        m_bSkipped = 1;
        m_bCancelAll = nMovie;
        return;
    }
    m_bCancelAll = nMovie;
#endif
}

void CExoMoviePlayerInternal::OnDeactivation(int a)
{
    m_bDeactivated = 1;
    if (a == 1 || m_nDeactivationArg == 0)
        m_nDeactivationArg = a;
}

void CExoMoviePlayerInternal::UnpauseMovie()
{
    if (m_pBink != 0 && m_bPaused == 1)
    {
        m_bPaused = 0;
        BinkPause(m_pBink, 0);
    }
}

int CExoMoviePlayerInternal::IsMoviePlaying()
{
    return m_bMoviePlaying;
}

int CExoMoviePlayerInternal::GetNumberOfMoviesToPlay()
{
    return m_aMovies.m_count;
}

// Both window-move hooks are no-ops.

#if KOTOR1
void CExoMoviePlayerInternal::WindowsPosChanging(int) {}

void CExoMoviePlayerInternal::WindowsPosChanged(int) {}
#else
void CExoMoviePlayerInternal::WindowsPosChanging(int64_t) {}

void CExoMoviePlayerInternal::WindowsPosChanged(int64_t) {}
#endif

void CExoMoviePlayerInternal::ShowFadePanel() {}

void CExoMoviePlayerInternal::HideFadePanel() {}

void CExoMoviePlayerInternal::RestartMoviesAsync() {}

int CExoMoviePlayerInternal::Initialize(void* p) { K2_UNUSED(p); return 1; }

int CExoMoviePlayerInternal::IsReady() { return 1; }

int CExoMoviePlayerInternal::FindFreePacket(unsigned int* p) { K2_UNUSED(p); return 1; }

int CExoMoviePlayerInternal::DecodeNext() { return 1; }

#if KOTOR1
// K1: losing focus cancels the movie, Alt+Enter cancels too, and there are no window-position arms.
int MovieMessageHandler(void* hWnd, unsigned int msg, unsigned int wParam, int lParam)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        if (wParam == 0 && g_pAsyncMoviePlayer != 0 && g_pAsyncMoviePlayer->m_bShuttingDown == 0)
        {
            g_pAsyncMoviePlayer->OnDeactivation(0);
            g_pAsyncMoviePlayer->CancelMovie(1, 1);
        }
        break;

    case WM_KEYDOWN:
        if (wParam != VK_ESCAPE)
            break;
        if (g_pAsyncMoviePlayer != 0)
            g_pAsyncMoviePlayer->CancelMovie(0, 0);
        break;

    case WM_SYSKEYDOWN:
        if (wParam != VK_F4 || (lParam & KEYDATA_ALTDOWN) == 0)
            break;
        if (g_hWnd != 0)
            PostMessageA(g_hWnd, WM_CLOSE, 0, 0);
        break;

    case WM_SYSKEYUP:
        if (wParam != VK_RETURN || (lParam & KEYDATA_ALTDOWN) == 0)
            break;
        if (g_pAsyncMoviePlayer == 0)
            break;
        g_pAsyncMoviePlayer->OnDeactivation(1);
        g_pAsyncMoviePlayer->CancelMovie(1, 1);
        return 0;

    case WM_LBUTTONUP:
        if (g_pAsyncMoviePlayer == 0)
            return 0;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0)
            return 0;
        g_pAsyncMoviePlayer->CancelMovie(0, 0);
        return 0;

    case WM_RBUTTONUP:
        if (g_pAsyncMoviePlayer == 0)
            return 0;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons == 0)
            return 0;
        g_pAsyncMoviePlayer->CancelMovie(0, 0);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
#else
int64_t MovieMessageHandler(HWND__* hWnd, unsigned int msg, unsigned long long wParam, int64_t lParam)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        if (wParam != 0)
        {
            g_pAppManager->m_pClientExoApp->ResumeStreamMusicForMovie();
            break;
        }
        // The lost-focus sound pause sits inside the player guard; a null player falls through to
        // DefWindowProcA.
        if (g_pAsyncMoviePlayer != 0 && g_pAsyncMoviePlayer->m_bShuttingDown == 0)
        {
            g_pAsyncMoviePlayer->OnDeactivation(0);
            g_pAsyncMoviePlayer->PauseMovie();
            g_pAppManager->m_pClientExoApp->PauseAllSoundsForMovieLostFocus();
        }
        break;

    case WM_KEYDOWN:
        if (wParam != VK_ESCAPE)
            break;
        if (g_pAsyncMoviePlayer != 0)
            g_pAsyncMoviePlayer->CancelMovie(0, 0);
        break;

    case WM_SYSKEYDOWN:
        if (wParam != VK_F4 || (lParam & KEYDATA_ALTDOWN) == 0)
            break;
        if (g_hWnd != 0)
            PostMessageA(g_hWnd, WM_CLOSE, 0, 0);
        break;

    case WM_SYSKEYUP:
        if (wParam != VK_RETURN || (lParam & KEYDATA_ALTDOWN) == 0)
            break;
        if (g_pAsyncMoviePlayer == 0)
            break;      // No player: fall through to DefWindowProcA rather than returning 0
        g_pAsyncMoviePlayer->OnDeactivation(0);
        g_pAsyncMoviePlayer->PauseMovie();
        return 0;

    case WM_LBUTTONUP:
        if (g_pAsyncMoviePlayer == 0)
            return 0;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons != 0)
            return 0;
        g_pAsyncMoviePlayer->CancelMovie(0, 0);
        return 0;

    case WM_RBUTTONUP:
        if (g_pAsyncMoviePlayer == 0)
            return 0;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons == 0)
            return 0;
        g_pAsyncMoviePlayer->CancelMovie(0, 0);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        return 0;

    case MOVIEMSG_WINDOWPOSCHANGING:     // re-posted from the render window
    case MOVIEMSG_WINDOWPOSCHANGED:
        delete reinterpret_cast<MovieWinPos*>(lParam);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
#endif
