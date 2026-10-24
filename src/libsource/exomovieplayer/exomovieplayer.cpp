#include "k2stdint.h"
#include "exomovieplayer.h"
#include "exomovieplayerinternal.h"

CExoMoviePlayer::CExoMoviePlayer()
{
    m_pInternal = new CExoMoviePlayerInternal();
}

CExoMoviePlayer::~CExoMoviePlayer()
{
    if (m_pInternal)
    {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

int CExoMoviePlayer::StartMovie(CExoString const& sMovie)
{
    return m_pInternal->StartMovie(sMovie);
}

void CExoMoviePlayer::StopMovie()
{
    m_pInternal->StopMovie();
}

void CExoMoviePlayer::CancelMovie(int a, int b)
{
    m_pInternal->CancelMovie(a, b);
}

int CExoMoviePlayer::IsMoviePlaying()
{
    return m_pInternal->IsMoviePlaying();
}

void CExoMoviePlayer::PlayMoviesAsync(CExoArrayList<CExoString> const& movies, int bLoop,
                                      CExoArrayList<int> const* pChapters)
{
    m_pInternal->PlayMoviesAsync(movies, bLoop, pChapters);
}

void CExoMoviePlayer::ShowFadePanel()
{
    m_pInternal->ShowFadePanel();
}

void CExoMoviePlayer::HideFadePanel()
{
    m_pInternal->HideFadePanel();
}

void CExoMoviePlayer::RestartMoviesAsync()
{
    m_pInternal->RestartMoviesAsync();
}

void CExoMoviePlayer::RenderFrame(int nFrame)
{
    if (m_pInternal)
        m_pInternal->RenderFrame(nFrame);
}

void CExoMoviePlayer::PauseMovie()
{
    if (m_pInternal)
        m_pInternal->PauseMovie();
}

void CExoMoviePlayer::UnpauseMovie()
{
    if (m_pInternal)
        m_pInternal->UnpauseMovie();
}

int CExoMoviePlayer::GetNumberOfMoviesToPlay()
{
    return m_pInternal ? m_pInternal->GetNumberOfMoviesToPlay() : 0;
}

void CExoMoviePlayer::WindowsPosChanging(int64_t lParam)
{
    if (m_pInternal)
#if KOTOR1
        m_pInternal->WindowsPosChanging(static_cast<int>(lParam));
#else
        m_pInternal->WindowsPosChanging(lParam);
#endif
}

void* CExoMoviePlayer::GetMovieWindow()
{
    return m_pInternal ? m_pInternal->m_movieWindow : 0;
}

void CExoMoviePlayer::WindowsPosChanged() {}

void CExoMoviePlayer::ClearMovies()
{
    if (m_pInternal)
        m_pInternal->m_aMovies.Clear();
}

int CExoMoviePlayer::Initialize(void* p)
{
    K2_UNUSED(p);
    return 0;
}
