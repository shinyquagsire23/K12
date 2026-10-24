// Adds POSIX's setenv / unsetenv to the CRT's <stdlib.h> (Windows builds only;
// windows/win_posix.cpp defines them on _putenv_s).
#include_next <stdlib.h>
#ifndef K2_MINGW_STDLIB_H
#define K2_MINGW_STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif
int setenv(const char* pszName, const char* pszValue, int bOverwrite);
int unsetenv(const char* pszName);
#ifdef __cplusplus
}
#endif
#endif
