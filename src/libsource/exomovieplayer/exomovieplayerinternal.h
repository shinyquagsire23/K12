#pragma once
#include "k2stdint.h"

#include "exostring.h"
#include "exocritsect.h"
#include "CExoArrayListDtors.h"
#include "port/win32api.h"

// Stand-in for the owned object that is destroyed through a vtable slot and never assigned
// anywhere in this unit: it gives that delete a polymorphic type.
// The window-position record main.cpp's two WM_WINDOWPOS* arms post to the movie window, whose
// procedure frees it.
struct MovieWinPos {
    void*        hwnd;             // g_hMovieWnd
    void*        hwndInsertAfter;  // 0
    RECT         rc;
    unsigned int flags;            // 0
    int          m_nReserved1;         // never written; the allocation size covers it
};

// The private messages (WM_USER + 7 and + 8) that carry a MovieWinPos to the movie window.
enum {
    MOVIEMSG_WINDOWPOSCHANGING = 0x407,
    MOVIEMSG_WINDOWPOSCHANGED = 0x408,
};

class CExoMovieReservedObject {
public:
    virtual ~CExoMovieReservedObject();
};

// The movie-player implementation, allocated directly by the facade.
class CExoMoviePlayerInternal {
public:
    // Declared so the facade's construction and destruction are real calls rather than a bare
    // allocation and a bare delete.
    CExoMoviePlayerInternal();
    ~CExoMoviePlayerInternal();

    int  GetNumberOfMoviesToPlay();
    int  IsMoviePlaying();

    int  StartMovie(CExoString const& sMovie);
    void StopMovie();
    void CancelMovie(int, int);
    // m_bDeactivated is the "a deactivation has been seen" latch and m_nDeactivationArg records the argument of the
    // first one that mattered: an argument of 1 always overwrites, any other only when nothing is
    // recorded yet.
    void OnDeactivation(int);
    void PlayMoviesAsync(CExoArrayList<CExoString> const& movies, int bLoop,
                         CExoArrayList<int> const* pChapters);
    void ShowFadePanel();
    void ShowFrame(int, int, int, int);
    void HideFadePanel();
    void RestartMoviesAsync();
    int  RenderFrame(int);
    void PauseMovie();
    void UnpauseMovie();
#if KOTOR1
    void WindowsPosChanging(int);
    void WindowsPosChanged(int);
#else
    void WindowsPosChanging(int64_t);
    void WindowsPosChanged(int64_t);
#endif

    // The movie-library teardown. Both the destructor and StopMovie carry this body rather than
    // calling it.
    void ShutDown();

    // All four return 1 unconditionally, so they are int.
    int  Initialize(void* p);
    int  IsReady();
    int  FindFreePacket(unsigned int* p);
    int  DecodeNext();

    // The play thread's stop flag.
    int   m_bStopThread;
    // The play thread's handle, waited on by the play-thread procedure. The constructor leaves it.
    void* m_hThread;
    // The movie queue.
    CExoArrayList<CExoString> m_aMovies;
    // StartMovie's "the movie started" out-parameter, shared with the other three words below.
    int   m_bSkipped;
    int   m_bCancelAll;
    int   m_bDeactivated;
    int   m_nDeactivationArg;
    int   m_bShuttingDown;
    CExoString m_sReserved1;
    // True while a movie is playing.
    int   m_bMoviePlaying;
    int   m_nReserved2;
    void* m_pReserved5;
    void* m_pReserved6;
    CExoCriticalSection m_crit;
    // Owned; the destructor deletes it. Never assigned in this unit.
    CExoMovieReservedObject* m_pReserved3;
    void* m_pBink;
    // The native movie window, destroyed by the destructor.
    HWND__* m_movieWindow;
    char  _pad80[0x90 - 0x80];
    // Nonzero gates the teardown of the field above.
    int   m_bPaused;
    CExoString m_sMoviePath;
    void* m_pReserved4;
    void* m_pReserved7;
    int   m_nVertexPool;
    char  _padBC[0xcc - 0xbc];
    int   m_bLoop;
    CExoArrayList<int> m_aInts;
    // The per-movie chapter value, written by StartMovie from a bool and read by the play path.
    int   m_bSkippable;
};
