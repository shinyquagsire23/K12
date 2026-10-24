// The Bink handle API CExoMoviePlayerInternal declares, with null bodies. The engine never opens a
// handle itself: every movie plays through ASL::PlayBinkMovieGL, which here is
// video/player/asl_playbink.cpp (k2bink + k2movie). Nothing assigns
// CExoMoviePlayerInternal::m_pBink, so these entry points are link-only.
#include <stdint.h>

// Signatures as src/libsource/exomovieplayer/exomovieplayerinternal.cpp declares them. BinkSummary
// is that file's (its size is per game); with no handle there is no summary to write.
struct BinkSummary;
extern "C" {
void BinkGetSummary(void*, BinkSummary*) {}
void BinkPause(void*, int) {}
void BinkClose(void*) {}
int64_t BinkNextFrame(void*) { return 0; }
void BinkDoFrame(void*) {}
int64_t BinkWait(void*) { return 0; }
}
