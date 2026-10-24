// iOS entry point. main() hands the process to UIKit (SDL_UIKitRunApp), which calls K2IosMain on
// the main thread once the application has launched; from there it is macos/main.cpp's bring-up:
//   1. the working directory is the app's Documents folder: saves/, swkotor2.ini (swkotor.ini) and
//      the expansion files live there, and Info.plist's UIFileSharingEnabled shows it in Finder and
//      the Files app, which is how the OBBs get onto the device;
//   2. stdout/stderr -> Documents/k2.log (the previous run's kept as k2.prev.log) unless they are
//      a terminal already (`xcrun simctl launch --console-pty`);
//   3. the environment: Documents/k2env.txt, one KEY=VALUE per line ('#' comments) -- K2_STUBS=soft
//      and the rest of the K2_* switches;
//   4. KOTOR II: mount main.*.obb, then patch.*.obb, from Documents (K2_OBB_DIR in k2env.txt names
//      another folder), a GLES 2 context (gl4es), WinMain;
//   5. KOTOR 1 (K2P_K1): the engine's own SDL_main is the app, as on Android: mount K1's two OBBs,
//      put the dialog font where the engine opens it (override/iosdialog.otf, from the app bundle),
//      answer the Java screen queries, SDL_main.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>
#include "../../src/port/k2qolapi.h"
#include "../shim/obbfiles.h"

struct HINSTANCE__;
extern "C" int WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* pszCmdLine, int nCmdShow);
bool k2shim_obb_mount(const char* pszObbPath);
void k2shim_list_dir(const char* pszHostDir, std::vector<std::string>& names);
extern "C" void k2audio_set_suspended(int suspended);
#ifdef K2P_K1
#include "../k1/k1port.h"
extern "C" int SDL_main(int argc, char** argv);   // the engine's
float k2shim_pixel_scalef();
#endif

namespace {

#ifdef K2P_K1
constexpr bool kKotor1 = true;
#else
constexpr bool kKotor1 = false;
#endif

void RedirectStdio()
{
    if (isatty(STDERR_FILENO)) return;
    rename("k2.log", "k2.prev.log");
    if (freopen("k2.log", "w", stderr)) setvbuf(stderr, 0, _IONBF, 0);
    dup2(fileno(stderr), STDOUT_FILENO);
    setvbuf(stdout, 0, _IOLBF, 0);
}

void LoadEnvFile(const char* pszPath)
{
    FILE* f = fopen(pszPath, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        char* eq = strchr(p, '=');
        if (!*p || *p == '#' || !eq) continue;
        *eq = 0;
        setenv(p, eq + 1, 1);
        fprintf(stderr, "k2: env %s=%s (k2env.txt)\n", p, eq + 1);
    }
    fclose(f);
}

// Mounts the game's main.<v>.<pkg>.obb first, then its patch.<v>.<pkg>.obb, so the later mount
// wins.
bool MountObbs(const char* pszDir)
{
    std::vector<std::string> names;
    k2shim_list_dir(pszDir, names);
    const K2ObbNames obbs = k2_pick_obbs(names, kKotor1);
    const std::string main = obbs.main.empty() ? "" : std::string(pszDir) + "/" + obbs.main;
    const std::string patch = obbs.patch.empty() ? "" : std::string(pszDir) + "/" + obbs.patch;
#ifdef K2P_K1
    if (!main.empty()) k1port_mount_obb(main.c_str(), 0);
    if (!patch.empty()) k1port_mount_obb(patch.c_str(), 1);
#else
    if (!main.empty()) k2shim_obb_mount(main.c_str());
    if (!patch.empty()) k2shim_obb_mount(patch.c_str());
#endif
    if (main.empty()) {
        fprintf(stderr, "k2: no main.*.obb in %s -- copy the expansion files into the app's "
                        "Documents folder (Finder / Files; platforms/ios/README.md)\n", pszDir);
        return false;
    }
    return true;
}

int AudioLifecycleWatch(void*, SDL_Event* e)
{
    if (e->type == SDL_APP_WILLENTERBACKGROUND) k2audio_set_suspended(1);
    else if (e->type == SDL_APP_DIDENTERFOREGROUND) k2audio_set_suspended(0);
    return 0;
}

void NoDataAlert()
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game data missing",
#ifdef K2P_K1
                             "Copy main.53.com.aspyr.swkotor.obb and patch.53.com.aspyr.swkotor.obb "
#else
                             "Copy main.*.com.aspyr.swkotorii.obb and patch.*.com.aspyr.swkotorii.obb "
#endif
                             "into this app's folder (Finder, or Files > On My iPhone), then start "
                             "it again.", 0);
}

#ifdef K2P_K1
// Installs the engine's FTGL dialog font, the original APK's assets/iosdialog.otf, which is bundled
// with the app and opened as override/iosdialog.otf under the working directory.
void InstallK1Font()
{
    const char* pszDst = "override/iosdialog.otf";
    struct stat st;
    if (stat(pszDst, &st) == 0 && st.st_size > 0) return;
    mkdir("override", 0770);
    SDL_RWops* src = SDL_RWFromFile("iosdialog.otf", "rb");   // SDL looks in the bundle first
    if (!src) {
        fprintf(stderr, "k1: no iosdialog.otf in the app bundle\n");
        return;
    }
    FILE* dst = fopen(pszDst, "wb");
    char buf[65536];
    size_t n;
    while (dst && (n = SDL_RWread(src, buf, 1, sizeof buf)) > 0) fwrite(buf, 1, n, dst);
    if (dst) fclose(dst);
    SDL_RWclose(src);
    fprintf(stderr, "k1: installed %s\n", pszDst);
}
#endif

int K2IosMain(int argc, char** argv)
{
    std::string docs = std::string(getenv("HOME") ? getenv("HOME") : ".") + "/Documents";
    mkdir(docs.c_str(), 0770);
    if (chdir(docs.c_str()) != 0) return 1;
    RedirectStdio();
    setenv("MTL_CAPTURE_ENABLED", "1", 0);   // before Metal starts: the GPU frame capture hook
    fprintf(stderr, "k2: working directory %s\n", docs.c_str());
    LoadEnvFile("k2env.txt");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2");   // a swipe must start twice
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0"); // the joystick list is the paired pads only
    const char* pszObbDir = getenv("K2_OBB_DIR");   // k2env.txt may name another folder
    bool bData = MountObbs(pszObbDir && *pszObbDir ? pszObbDir : ".");
#ifdef K2P_K1
    InstallK1Font();
    k2qol_startup();
    SDL_AddEventWatch(AudioLifecycleWatch, 0);
    setenv("K2_JNI_GetHighResolution", "1", 0);
    setenv("K2_JNI_GetScreenHeightPixel", "4320", 0);
    {
        // DisplayMetrics.xdpi / ydpi: the tap and drag tolerances. An iPhone point is about 1/160
        // inch.
        char szDpi[16];
        snprintf(szDpi, sizeof szDpi, "%d", static_cast<int>(160.0f * k2shim_pixel_scalef() + 0.5f));
        setenv("K2_JNI_xdpi", szDpi, 0);
        setenv("K2_JNI_ydpi", szDpi, 0);
    }
    if (!bData) {
        NoDataAlert();
        return 1;
    }
    fprintf(stderr, "k1: entering SDL_main\n");
    SDL_SetMainReady();
    return SDL_main(argc, argv);
#else
#ifdef K2_QOL_CONTROLLER
    k2qol_startup();
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!bData) {
        NoDataAlert();
        return 1;
    }
    SDL_AddEventWatch(AudioLifecycleWatch, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    char cmdline[1024] = "";
    for (int i = 1; i < argc; ++i) {
        strlcat(cmdline, argv[i], sizeof cmdline);
        if (i + 1 < argc) strlcat(cmdline, " ", sizeof cmdline);
    }
    fprintf(stderr, "k2: entering WinMain(\"%s\")\n", cmdline);
    int rc = WinMain(0, 0, cmdline, 1);
    fprintf(stderr, "k2: WinMain returned %d\n", rc);
    SDL_Quit();
    exit(rc);   // the engine's globals do not survive a second WinMain
#endif
}

} // namespace

int main(int argc, char** argv)
{
    return SDL_UIKitRunApp(argc, argv, K2IosMain);
}
