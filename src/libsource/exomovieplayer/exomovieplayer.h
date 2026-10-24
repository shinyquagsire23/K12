#pragma once
#include "k2stdint.h"

class CExoMoviePlayerInternal;
class CExoString;
template<class T> class CExoArrayList;

// The public facade over the movie player's pimpl.
class CExoMoviePlayer {
public:
    CExoMoviePlayer();
    ~CExoMoviePlayer();
    int   StartMovie(CExoString const& sMovie);
    void  StopMovie();
    void  CancelMovie(int a, int b);
    int   IsMoviePlaying();
    void  PlayMoviesAsync(CExoArrayList<CExoString> const& movies, int bLoop,
                          CExoArrayList<int> const* pChapters);
    void  ShowFadePanel();
    void  HideFadePanel();
    void  RestartMoviesAsync();
    void  RenderFrame(int nFrame);
    void  PauseMovie();
    void  UnpauseMovie();
    void  ClearMovies();
    void  WindowsPosChanged();
    int   GetNumberOfMoviesToPlay();
    void  WindowsPosChanging(int64_t lParam);
    void* GetMovieWindow();
    int   Initialize(void* p);

    CExoMoviePlayerInternal* m_pInternal;
};
