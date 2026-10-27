// The MSVC CRT names the engine calls (Aspyr's port layer provides them on Android), and the
// Win32 last-error slot.
#include <stdint.h>
#include "k2shim.h"
#include "port/win32api.h"
#include "port/crtapi.h"
#include <SDL_atomic.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static thread_local DWORD t_lastError;

K2_WIN32_API_BEGIN
DWORD GetLastError() { return t_lastError; }
void  SetLastError(DWORD dwErr) throw() { t_lastError = dwErr; }
K2_WIN32_API_END

extern "C" {

// The length is declared unsigned long, unsigned int or int depending on the engine TU, so read 32
// bits.
#ifdef _WIN32
// The UCRT's own takes a size_t; and its strncasecmp is this very name, so compare here.
int _strnicmp(const char* a, const char* b, unsigned int n) throw()
{
    for (; n; --n, ++a, ++b) {
        int d = tolower(static_cast<unsigned char>(*a)) - tolower(static_cast<unsigned char>(*b));
        if (d || !*a) return d;
    }
    return 0;
}
#else
int _strnicmp(const char* a, const char* b, unsigned int n) throw() { return strncasecmp(a, b, n); }
#endif

// Windows: the rest are the UCRT's own, MSVC's semantics exactly (and its strcasecmp, strdup ...
// are aliases of these names, so a definition here that called them would call itself).
#ifndef _WIN32
int _stricmp(const char* a, const char* b) throw() { return strcasecmp(a, b); }
char* _strdup(const char* s) throw() { return strdup(s); }
int _finite(double x) throw() { return isfinite(x); }

char* _strlwr(char* s) throw()
{
    for (char* p = s; *p; ++p) *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
    return s;
}

char* _strupr(char* s) throw()
{
    for (char* p = s; *p; ++p) *p = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
    return s;
}

char* _itoa(int value, char* buf, int radix)
{
    if (radix == 10) {
        snprintf(buf, 12, "%d", value);   // "-2147483648" + NUL
        return buf;
    }
    // MSVC: any other radix prints the value as unsigned
    unsigned int u = static_cast<unsigned int>(value);
    char tmp[40];
    int n = 0;
    do {
        unsigned d = u % static_cast<unsigned>(radix);
        tmp[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
        u /= static_cast<unsigned>(radix);
    } while (u);
    for (int i = 0; i < n; ++i) buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
    return buf;
}

// MSVC semantics: returns -1 (not the would-be length) on truncation, and then does not terminate.
int _vsnprintf(char* buf, size_t n, const char* fmt, va_list ap)
{
    int r = vsnprintf(buf, n, fmt, ap);
    return (r < 0 || static_cast<size_t>(r) >= n) ? -1 : r;
}
#endif // !_WIN32

// The MSVC rand(), which the engine's random sequences (ambient sounds, grass) were tuned against.
static thread_local unsigned int t_randState = 1;
void srand_wincompatible(unsigned int seed) { t_randState = seed; }
int rand_wincompatible()
{
    t_randState = t_randState * 214013u + 2531011u;
    return static_cast<int>((t_randState >> 16) & 0x7fff);
}

#ifdef _WIN32
// The C library's rand() as bionic has it, with the full 31-bit range and one sequence for the
// process. The Windows CRT's is 15-bit, and the engine calls rand() directly wherever it wants a
// wide range (KOTOR 1 everywhere, through crtapi.h's rand_wincompatible; serverexoappinternal's
// coin flip, which a 15-bit rand never wins). Defining it here takes the place of the ucrt import
// for the whole executable.
static uint64_t s_randState = 1;
static SDL_SpinLock s_randLock;
void srand(unsigned int seed)
{
    SDL_AtomicLock(&s_randLock);
    s_randState = seed;
    SDL_AtomicUnlock(&s_randLock);
}
int rand(void)
{
    SDL_AtomicLock(&s_randLock);
    s_randState = s_randState * 6364136223846793005ull + 1442695040888963407ull;
    int r = static_cast<int>(s_randState >> 33);
    SDL_AtomicUnlock(&s_randLock);
    return r;
}
#endif

} // extern "C"

DWORD k2shim_errno_to_win32(int e)
{
    switch (e) {
    case 0: return K2_ERROR_SUCCESS;
    case ENOENT: return K2_ERROR_FILE_NOT_FOUND;
    case ENOTDIR: return K2_ERROR_PATH_NOT_FOUND;
    case EACCES: case EPERM: return K2_ERROR_ACCESS_DENIED;
    case EEXIST: return K2_ERROR_ALREADY_EXISTS;
    default: return K2_ERROR_INVALID_PARAMETER;
    }
}
