#include "resmp3.h"

CResMP3::CResMP3()
{
}

CResMP3::~CResMP3()
{
}

// MP3 data is streamed, so there is nothing to unpublish.
int CResMP3::OnResourceFreed()
{
    return 1;
}

// MP3 data is streamed, so there is nothing to publish.
int CResMP3::OnResourceServiced()
{
    return 1;
}
