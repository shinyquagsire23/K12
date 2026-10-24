// ASL::PlayBinkMovieGL on k2bink: the entry point the movie player calls for every movie. The player
// itself is k2movie.c; this file is the engine-facing ABI, the path fix-up and the optional
// subtitles.
//
// Arguments:
//   pszMovie      the resolved movie path, DOS separators allowed
//   fVolume       linear, 0..1 (the engine's movie volume option, 0 during the "evil ending" hack)
//   nSoundTrack   the Bink audio track (BinkSetSoundTrack); the engine passes 0
//   nInputDelayMs input is ignored for this long after the movie starts; the engine passes 250
//   pbSkipped     written only if the movie opened: true when the user ended it early. The engine
//                 stores it in CExoMoviePlayerInternal::m_bSkipped, which its play loops read as
//                 "skipped" (a skip breaks out of a movie list when m_bCancelAll is set)
//   pbQuit        macOS only (the Android engine passes NULL): true when an SDL_QUIT ended it
// Returns 1 when the movie opened, 0 otherwise.
//
// It plays into the GL context current on the calling thread. Called on a thread with no context
// (the engine's AsyncPlayThreadProc path), it reports "did not open" and returns at once.
#include "k2movie.h"

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef K2P_BINK_SUBTITLES
#include "aurbink.h"
#endif

namespace {

// Case-insensitive path lookup, one component at a time: the engine's resource names are
// case-blind (Windows and Aspyr's VFS), host file systems may not be.
bool FindPathNoCase(const char* pszIn, char* pszOut, size_t nOut)
{
    if (access(pszIn, R_OK) == 0) {
        snprintf(pszOut, nOut, "%s", pszIn);
        return true;
    }
    char szBuilt[1024] = "";
    const char* p = pszIn;
    if (*p == '/') {
        strcpy(szBuilt, "/");
        while (*p == '/') ++p;
    }
    while (*p) {
        const char* pEnd = strchr(p, '/');
        size_t n = pEnd ? static_cast<size_t>(pEnd - p) : strlen(p);
        char szPart[256];
        if (n >= sizeof szPart) return false;
        memcpy(szPart, p, n);
        szPart[n] = 0;

        DIR* pDir = opendir(szBuilt[0] ? szBuilt : ".");
        if (!pDir) return false;
        bool bFound = false;
        while (struct dirent* e = readdir(pDir)) {
            if (strcasecmp(e->d_name, szPart) == 0) {
                snprintf(szPart, sizeof szPart, "%s", e->d_name);
                bFound = true;
                break;
            }
        }
        closedir(pDir);
        if (!bFound) return false;

        size_t nLen = strlen(szBuilt);
        if (nLen && szBuilt[nLen - 1] != '/' && nLen + 1 < sizeof szBuilt) strcat(szBuilt, "/");
        if (strlen(szBuilt) + strlen(szPart) >= sizeof szBuilt) return false;
        strcat(szBuilt, szPart);

        p += n;
        while (*p == '/') ++p;
    }
    snprintf(pszOut, nOut, "%s", szBuilt);
    return true;
}

// Resolves an engine DOS path, with the Android player's "./" strip, by turning backslashes into
// slashes and then finding the file as it exists on disk.
bool ResolveMoviePath(const char* pszMovie, char* pszOut, size_t nOut)
{
    char szPath[1024];
    snprintf(szPath, sizeof szPath, "%s", pszMovie);
    for (char* c = szPath; *c; ++c)
        if (*c == '\\') *c = '/';
    const char* p = szPath;
    if (p[0] == '.' && p[1] == '/') p += 2;
    return FindPathNoCase(p, pszOut, nOut);
}

#ifdef K2P_BINK_SUBTITLES
// Subtitles are drawn after the picture at frame / fps, Bink's frame numbers being 1-based.
int DrawSubtitles(void* user, const k2movie_frame* f)
{
    double t = static_cast<double>(f->planes->frame_index + 1) * f->info->fps_den / f->info->fps_num;
    static_cast<BinkSubtitles*>(user)->Draw(static_cast<float>(t));
    return 0;
}
#endif

} // namespace

namespace ASL {

int PlayBinkMovieGL(const char* pszMovie, float fVolume, unsigned int nSoundTrack,
                    unsigned int nInputDelayMs, bool* pbSkipped, bool* pbQuit)
{
    if (pszMovie == 0) return 0;
    char szPath[1024];
    if (!ResolveMoviePath(pszMovie, szPath, sizeof szPath)) {
        fprintf(stderr, "PlayBinkMovieGL: %s: not found\n", pszMovie);
        return 0;
    }

    // K2_SKIP_MOVIES=1: report every movie as found and skipped at once (unattended runs: the
    // K2_INPUT frame counter does not advance while a movie plays).
    static const bool s_bSkipMovies = getenv("K2_SKIP_MOVIES") && atoi(getenv("K2_SKIP_MOVIES"));
    if (s_bSkipMovies) {
        fprintf(stderr, "PlayBinkMovieGL: %s: skipped (K2_SKIP_MOVIES)\n", pszMovie);
        if (pbSkipped) *pbSkipped = true;
        if (pbQuit) *pbQuit = false;
        return 1;
    }

    k2movie_opts opts;
    k2movie_default_opts(&opts);
    opts.volume = fVolume;
    opts.audio_track = static_cast<int>(nSoundTrack);
    opts.input_delay_ms = nInputDelayMs;
#ifdef K2P_BINK_SUBTITLES
    BinkSubtitles subtitles(szPath);
    opts.after_draw = DrawSubtitles;
    opts.user = &subtitles;
#endif

    k2movie_result res;
    k2movie_play(szPath, &opts, &res);
    if (!res.opened) {
        fprintf(stderr, "PlayBinkMovieGL: %s: %s\n", szPath,
                res.error == K2BINK_ERR_ARG ? "no usable GL context on this thread" : k2bink_strerror(res.error));
        return 0;
    }
    if (pbSkipped) *pbSkipped = res.skipped != 0;
    if (pbQuit) *pbQuit = res.quit != 0;
    return 1;
}

} // namespace ASL
