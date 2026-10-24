#ifndef AURBINK_H
#define AURBINK_H

// The iOS/Android Bink video player's subtitle reader and SDL event filter.

#include <vector>

union SDL_Event;

// Reads "<name>.<lang>sub.txt"; the whole file image is kept and split in place.
class BinkSubtitles
{
public:
    BinkSubtitles(const char* pszName);
    ~BinkSubtitles();

    // Parses a line's "MM:SS - MM:SS <rest>" header into two second counts. Returns true when the
    // line index is out of range; otherwise reports whether the header was well formed.
    bool GetTimeOnLine(int nLine, int& nStart, int& nEnd);

    void Draw(float fTime);

private:
    char* m_pszData;
    std::vector<char*> m_lines;
};

// Called for every SDL event while a movie is up: a release, quit or focus change sets the skip
// request the player polls.
void BinkEventFilter(SDL_Event* pEvent);

#endif
