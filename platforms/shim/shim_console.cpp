// The OS services of the console action-bar driver (platforms/console/k2console.cpp).
//
// The driver is compiled with the engine's flags so it can use src/'s headers, which means it may
// not include a system header (compat/k2host_prelude.h). The three things it needs from the OS are
// here instead:
//   k2console_log     one line: logcat tag K2CONSOLE on Android, stderr ("k2: console: ...") on
//                     the desktop
//   k2console_prop    the hybrid's `debug.k2.*` knobs: the system property itself on Android, the
//                     environment on the desktop, named without `debug.`, upper-cased, dots to
//                     underscores (debug.k2.console.engine -> K2_CONSOLE_ENGINE)
//   k2console_now_ms  a monotonic millisecond clock
#include <stdint.h>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#include <sys/system_properties.h>
#endif

extern "C" void k2console_log(const char* pszLine)
{
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, "K2CONSOLE", pszLine);
#else
    fprintf(stderr, "k2: console: %s\n", pszLine);
#endif
}

extern "C" int k2console_prop(const char* pszName, char* pBuf, int nBuf)
{
    if (!pBuf || nBuf <= 0) return 0;
    pBuf[0] = 0;
#ifdef __ANDROID__
    char v[PROP_VALUE_MAX] = {0};
    int n = __system_property_get(pszName, v);
    if (n <= 0) return 0;
    snprintf(pBuf, static_cast<size_t>(nBuf), "%s", v);
    return static_cast<int>(strlen(pBuf));
#else
    char sEnv[96];
    const char* p = strncmp(pszName, "debug.", 6) ? pszName : pszName + 6;
    size_t i = 0;
    for (; *p && i + 1 < sizeof sEnv; ++p, ++i)
        sEnv[i] = *p == '.' ? '_' : static_cast<char>(*p >= 'a' && *p <= 'z' ? *p - 'a' + 'A' : *p);
    sEnv[i] = 0;
    const char* v = getenv(sEnv);
    if (!v || !*v) return 0;
    snprintf(pBuf, static_cast<size_t>(nBuf), "%s", v);
    return static_cast<int>(strlen(pBuf));
#endif
}

extern "C" int64_t k2console_now_ms(void)
{
    return static_cast<int64_t>(SDL_GetPerformanceCounter() * 1000 / SDL_GetPerformanceFrequency());
}
