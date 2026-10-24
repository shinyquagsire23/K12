// ASL2 Mac port utilities: the conversions between Unix time and the Win32 FILETIME the port's
// file API reports.
#include "k2stdint.h"
#include "Win32Find.h"

// FILETIME counts 100 ns ticks from 1601-01-01; Unix seconds count from 1970-01-01.
void MacSecondsToFileTime(uint64_t nSeconds, struct _FILETIME* pFileTime) throw()
{
    uint64_t nTime = nSeconds * 10000000 + 116444736000000000ULL;
    pFileTime->dwLowDateTime = static_cast<unsigned int>(nTime);
    pFileTime->dwHighDateTime = static_cast<unsigned int>(nTime >> 32);
}
