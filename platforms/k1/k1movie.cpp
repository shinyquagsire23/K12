// KOTOR 1's movie player on k2movie (K2P_GAME=k1; k1port.h).
//
// The playing half of the port library's MacPlayBinkGL; k1port.cpp finds the file. That function
// opens Bink on the OBB itself at the entry's offset, where k2bink reads the entry's stream. Its
// rules, as k2movie options:
//   * the volume is Bink's 0..0x8000 (BinkSetVolume(bink, 0, nVolume)); the evil ending passes 0;
//   * input ends the movie once more than a second of it has shown, and only when bSound is set:
//     the evil ending's silent movie cannot be skipped. Bink counts finger and joystick-button
//     presses, where k2movie counts any release (keys and mouse too, for the host);
//   * input queued before the movie is flushed (SDL_FlushEvents(SDL_KEYDOWN, SDL_FINGERMOTION)),
//     which k2movie's input delay stands in for.
// Subtitles (BinkSubtitles, <movie>.bik.<lang>sub.txt for de/es/fr/it) are not drawn.
#include "k1port.h"

#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "k2movie.h"

namespace {

size_t RwRead(void* user, void* dst, size_t n)
{
    return SDL_RWread(static_cast<SDL_RWops*>(user), dst, 1, n);
}

int RwSeek(void* user, uint64_t off)
{
    return SDL_RWseek(static_cast<SDL_RWops*>(user), static_cast<Sint64>(off), RW_SEEK_SET) < 0 ? -1 : 0;
}

} // namespace

extern "C" int k1movie_play(SDL_RWops* rw, const char* pszName, int bSound, int nVolume)
{
    // K2_SKIP_MOVIES=1: every movie ends at once (unattended runs, as ASL::PlayBinkMovieGL).
    static const bool s_bSkipMovies = getenv("K2_SKIP_MOVIES") && atoi(getenv("K2_SKIP_MOVIES"));
    if (s_bSkipMovies) {
        fprintf(stderr, "k1: movie %s skipped (K2_SKIP_MOVIES)\n", pszName);
        return 1;
    }

    k2bink_io io = { RwRead, RwSeek, 0, 0 };
    k2movie_opts opts;
    k2movie_default_opts(&opts);
    opts.volume = static_cast<float>(nVolume) / 32768.0f;
    opts.audio_track = 0;
    if (!bSound) opts.skip_min_frames = INT_MAX;

    k2movie_result res;
    k2movie_play_io(&io, rw, pszName, &opts, &res);
    if (!res.opened) {
        fprintf(stderr, "k1: movie %s: %s\n", pszName,
                res.error == K2BINK_ERR_ARG ? "no usable GL context on this thread" : k2bink_strerror(res.error));
        return 0;
    }
    return 1;
}
