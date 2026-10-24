// POSIX <strings.h> for the MSVC UCRT (windows/win_msvc_posix.cpp, over _stricmp / _strnicmp).
#ifndef K2_MSVC_STRINGS_H
#define K2_MSVC_STRINGS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int strcasecmp(const char* pszA, const char* pszB);
int strncasecmp(const char* pszA, const char* pszB, size_t n);
#ifdef __cplusplus
}
#endif
#endif
