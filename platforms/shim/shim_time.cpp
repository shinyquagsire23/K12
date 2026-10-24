// Win32 time (performance counter, tick counts, SYSTEMTIME/FILETIME, Sleep) and the framerate
// lock's frame pacing (k2host_pace_frame).
#include <stdint.h>
#include "k2shim.h"
#include "port/win32api.h"
#include "Win32Find.h"

#include <SDL.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#ifdef _WIN32
// The UCRT's RtlGenRandom wrapper; <stdlib.h> declares it only under _CRT_RAND_S.
extern "C" int __cdecl rand_s(unsigned int* pValue);
#else
#include <fcntl.h>
#endif
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

static_assert(sizeof(FILETIME) == 8, "FILETIME");
static_assert(sizeof(SYSTEMTIME) == 16, "SYSTEMTIME");

// 100 ns ticks between 1601-01-01 (FILETIME's epoch) and 1970-01-01.
static const uint64_t kEpochDelta = 116444736000000000ull;

static uint64_t FtGet(const FILETIME* ft) { return (static_cast<uint64_t>(ft->dwHighDateTime) << 32) | ft->dwLowDateTime; }
static void FtSet(FILETIME* ft, uint64_t v) { ft->dwLowDateTime = static_cast<DWORD>(v); ft->dwHighDateTime = static_cast<DWORD>(v >> 32); }

static uint64_t NowFileTime()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return kEpochDelta + static_cast<uint64_t>(tv.tv_sec) * 10000000ull + static_cast<uint64_t>(tv.tv_usec) * 10ull;
}

static void TmToSystemTime(const struct tm* t, unsigned ms, SYSTEMTIME* st)
{
    st->wYear = static_cast<uint16_t>(t->tm_year + 1900);
    st->wMonth = static_cast<uint16_t>(t->tm_mon + 1);
    st->wDayOfWeek = static_cast<uint16_t>(t->tm_wday);
    st->wDay = static_cast<uint16_t>(t->tm_mday);
    st->wHour = static_cast<uint16_t>(t->tm_hour);
    st->wMinute = static_cast<uint16_t>(t->tm_min);
    st->wSecond = static_cast<uint16_t>(t->tm_sec);
    st->wMilliseconds = static_cast<uint16_t>(ms);
}

static uint64_t StartTicks()
{
    static uint64_t s_start = SDL_GetTicks64();
    return s_start;
}

static uint64_t MonotonicNs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
}

static void SleepUntilNs(uint64_t nDeadline)
{
#ifdef __APPLE__
    // No clock_nanosleep, so sleep to a millisecond short of the deadline, then wait out the rest.
    for (uint64_t nNow = MonotonicNs(); nNow < nDeadline; nNow = MonotonicNs()) {
        uint64_t nLeft = nDeadline - nNow;
        if (nLeft > 1500000) {
            nLeft -= 1000000;
            struct timespec ts = { static_cast<time_t>(nLeft / 1000000000ull), static_cast<long>(nLeft % 1000000000ull) };
            nanosleep(&ts, 0);
        }
    }
#else
    struct timespec ts = { static_cast<time_t>(nDeadline / 1000000000ull), static_cast<long>(nDeadline % 1000000000ull) };
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, 0) == EINTR) {
    }
#endif
}

#ifdef __ANDROID__
// The time SurfaceFlinger is to show the next swap's buffer (EGL_ANDROID_presentation_time; the
// clock is CLOCK_MONOTONIC), or EGL's own stamp again for INT64_MIN (NATIVE_WINDOW_TIMESTAMP_AUTO).
// The build does not link libEGL, so SDL, which loaded it, looks the entry points up.
static void SetPresentationTime(int64_t nTime)
{
    typedef EGLDisplay (*CurrentDisplayFn)(void);
    typedef EGLSurface (*CurrentSurfaceFn)(EGLint);
    typedef EGLBoolean (*PresentationTimeFn)(EGLDisplay, EGLSurface, EGLnsecsANDROID);
    static CurrentDisplayFn s_display =
        reinterpret_cast<CurrentDisplayFn>(SDL_GL_GetProcAddress("eglGetCurrentDisplay"));
    static CurrentSurfaceFn s_surface =
        reinterpret_cast<CurrentSurfaceFn>(SDL_GL_GetProcAddress("eglGetCurrentSurface"));
    static PresentationTimeFn s_present =
        reinterpret_cast<PresentationTimeFn>(SDL_GL_GetProcAddress("eglPresentationTimeANDROID"));
    if (!s_display || !s_surface || !s_present)
        return;
    EGLDisplay dpy = s_display();
    EGLSurface surf = s_surface(EGL_DRAW);
    if (dpy != EGL_NO_DISPLAY && surf != EGL_NO_SURFACE)
        s_present(dpy, surf, nTime);
}
#endif

K2_WIN32_API_BEGIN

int QueryPerformanceFrequency(long long* pFreq)
{
    *pFreq = static_cast<long long>(SDL_GetPerformanceFrequency());
    return 1;
}

int QueryPerformanceCounter(long long* pCount)
{
    *pCount = static_cast<long long>(SDL_GetPerformanceCounter());
    return 1;
}

// Milliseconds since the process started (Windows counts from boot; nothing depends on the base).
DWORD GetTickCount() { return static_cast<DWORD>(SDL_GetTicks64() - StartTicks() + 1); }

void Sleep(DWORD dwMilliseconds) { SDL_Delay(dwMilliseconds); }

DWORD timeGetTime() { return GetTickCount(); }

DWORD SleepEx(DWORD dwMilliseconds, int /*bAlertable*/) throw()
{
    SDL_Delay(dwMilliseconds);
    return 0;
}

K2_WIN32_API_END

extern "C" {

void k2host_achievements_frame(void);   // Steam's callbacks, the unlock notice

// The framerate lock's wait (K2_QOL_FRAME_PACING), called once a frame after the swap with the lock's
// rate, or 0 while it is off. Sleeping the lock's whole milliseconds since the last frame gave 62 fps
// out of 8 / 17 / 25 ms frames on a 120 Hz panel. Here frame starts keep a fixed schedule of 1/fFps,
// which a frame more than one period late starts over (a load, a pause, the app in the background),
// and on Android each frame's buffer is shown at its slot's end, so how long the frame takes to draw
// no longer decides which vsync it lands on.
void k2host_pace_frame(float fFps)
{
    k2host_achievements_frame();   // once a frame, lock or no lock
    static uint64_t s_nNextStart;   // 0: the lock was off last frame
    if (fFps <= 0.0f) {
#ifdef __ANDROID__
        if (s_nNextStart)
            SetPresentationTime(INT64_MIN);
#endif
        s_nNextStart = 0;
        return;
    }
    uint64_t nPeriod = static_cast<uint64_t>(1e9 / fFps);
    uint64_t nNow = MonotonicNs();
    if (s_nNextStart == 0 || nNow > s_nNextStart + nPeriod)
        s_nNextStart = nNow;
    else
        SleepUntilNs(s_nNextStart);
#ifdef __ANDROID__
    SetPresentationTime(static_cast<int64_t>(s_nNextStart + nPeriod));
#endif
    s_nNextStart += nPeriod;
}

// Seed bytes for CExoRandInternal's entropy ring, from the OS generator (rand_s / /dev/urandom) with
// the performance counter, the wall clock and a stack address mixed over it, so a missing generator
// still differs from launch to launch. Bug fix: the stock engine seeded from an uninitialised heap
// block.
void k2host_entropy(unsigned char* pOut, int nLen)
{
    memset(pOut, 0, static_cast<size_t>(nLen));
#ifdef _WIN32
    for (int i = 0; i + 4 <= nLen; i += 4) {
        unsigned int nValue;
        if (rand_s(&nValue) != 0)
            break;
        memcpy(pOut + i, &nValue, 4);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        for (int nGot = 0; nGot < nLen;) {
            ssize_t n = read(fd, pOut + nGot, static_cast<size_t>(nLen - nGot));
            if (n <= 0)
                break;
            nGot += static_cast<int>(n);
        }
        close(fd);
    }
#endif
    uint64_t nMix = SDL_GetPerformanceCounter() ^ (static_cast<uint64_t>(time(0)) << 20) ^
                    static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&nMix));
    for (int i = 0; i < nLen; i += 8) {
        nMix += 0x9e3779b97f4a7c15ull;   // splitmix64
        uint64_t z = nMix;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        z ^= z >> 31;
        for (int k = 0; k < 8 && i + k < nLen; k++)
            pOut[i + k] ^= static_cast<unsigned char>(z >> (k * 8));
    }
}

} // extern "C"

K2_WIN32_API_BEGIN
void GetSystemTimeAsFileTime(unsigned long long* pFt) { *pFt = NowFileTime(); }

void GetSystemTime(SYSTEMTIME* pSt)
{
    uint64_t ft = NowFileTime();
    time_t t = static_cast<time_t>((ft - kEpochDelta) / 10000000ull);
    struct tm tmv;
    gmtime_r(&t, &tmv);
    TmToSystemTime(&tmv, static_cast<unsigned>((ft / 10000ull) % 1000ull), pSt);
}

void GetLocalTime(SYSTEMTIME* pSt)
{
    uint64_t ft = NowFileTime();
    time_t t = static_cast<time_t>((ft - kEpochDelta) / 10000000ull);
    struct tm tmv;
    localtime_r(&t, &tmv);
    TmToSystemTime(&tmv, static_cast<unsigned>((ft / 10000ull) % 1000ull), pSt);
}

int FileTimeToSystemTime(const FILETIME* pFt, SYSTEMTIME* pSt)
{
    uint64_t ft = FtGet(pFt);
    if (ft < kEpochDelta) { memset(pSt, 0, sizeof *pSt); return 0; }
    time_t t = static_cast<time_t>((ft - kEpochDelta) / 10000000ull);
    struct tm tmv;
    gmtime_r(&t, &tmv);
    TmToSystemTime(&tmv, static_cast<unsigned>((ft / 10000ull) % 1000ull), pSt);
    return 1;
}
// The engine's FILETIME out-parameter is a 64-bit integer (as for GetSystemTimeAsFileTime).
int SystemTimeToFileTime(const SYSTEMTIME* pSt, unsigned long long* pFt)
{
    struct tm tmv;
    memset(&tmv, 0, sizeof tmv);
    tmv.tm_year = pSt->wYear - 1900;
    tmv.tm_mon = pSt->wMonth - 1;
    tmv.tm_mday = pSt->wDay;
    tmv.tm_hour = pSt->wHour;
    tmv.tm_min = pSt->wMinute;
    tmv.tm_sec = pSt->wSecond;
    time_t t = timegm(&tmv);
    *pFt = kEpochDelta + static_cast<uint64_t>(t) * 10000000ull + static_cast<uint64_t>(pSt->wMilliseconds) * 10000ull;
    return 1;
}

int FileTimeToLocalFileTime(const FILETIME* pFt, FILETIME* pLocal)
{
    time_t now = time(0);
    struct tm lt;
    localtime_r(&now, &lt);
#ifdef _WIN32
    // The UCRT's struct tm has no tm_gmtoff, so the offset is the local wall clock read back as
    // UTC, less the time.
    int64_t nGmtOff = static_cast<int64_t>(timegm(&lt) - now);
#else
    int64_t nGmtOff = static_cast<int64_t>(lt.tm_gmtoff);
#endif
    FtSet(pLocal, FtGet(pFt) + nGmtOff * 10000000ll);
    return 1;
}

int CompareFileTime(const FILETIME* a, const FILETIME* b)
{
    uint64_t x = FtGet(a), y = FtGet(b);
    return x < y ? -1 : x > y ? 1 : 0;
}

K2_WIN32_API_END
