// Android entry point: k2_main, which stock SDL2's SDLActivity calls on its own thread (K2Activity's
// getMainFunction; SDL_main is the engine's own, which K2 does not run here).
//
// The shipped SDL_main registers ASL's string table and event watch, mounts the OBB VFS
// (ASL::FsApi::Obb::initVfs) and hands WinMain to ASL::Main. Here, as on macOS, the shim owns the
// window and the event pump, so this does the bring-up and calls WinMain directly:
//   1. stdout/stderr -> logcat (tag "k2"), so every fprintf(stderr) the shim and engine make shows;
//   2. the environment: <files>/k2env.txt, one KEY=VALUE per line (K2_STUBS=soft, K2_SKIP_MOVIES,
//      K2_JNI_*, ...; '#' comments), on top of K2Activity's `k2env` intent extra, which it set
//      already;
//   3. chdir to the app's external files dir, the working directory the shipped build uses, where
//      its saves/ and swkotor2.ini live, so this build and the hybrid APK (same package) share them;
//   4. mount main.*.obb, then patch.*.obb, from the app's obb dir, the read-only layer under the
//      working directory;
//   5. a GLES 2 context (gl4es turns the engine's GL 1.x into it).
//
// KOTOR 1 (K2P_GAME=k1, K2P_K1): the engine's own SDL_main is the app and creates the window and
// context itself, so K1Activity has SDL call k1_main instead. It does 1-3, mounts K1's OBBs as the
// shipped game's Java does (zips, read through ObbFile), puts the dialog font from the APK's assets
// where the engine opens it (override/iosdialog.otf), answers the Java screen queries through the
// stub JNIEnv, and enters SDL_main.
#include <SDL.h>
#include <android/log.h>
#include <pthread.h>
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
#endif

namespace {

#ifdef K2P_K1
constexpr bool kKotor1 = true;
#else
constexpr bool kKotor1 = false;
#endif

int s_logPipe[2];

void* LogThread(void*)
{
    char buf[2048];
    size_t used = 0;
    while (true) {
        ssize_t n = read(s_logPipe[0], buf + used, sizeof buf - 1 - used);
        if (n <= 0) break;
        used += static_cast<size_t>(n);
        buf[used] = 0;
        char* start = buf;
        while (char* nl = strchr(start, '\n')) {
            *nl = 0;
            __android_log_write(ANDROID_LOG_INFO, "k2", start);
            start = nl + 1;
        }
        used = strlen(start);
        if (used == sizeof buf - 1) {   // a line longer than the buffer: flush it as is
            __android_log_write(ANDROID_LOG_INFO, "k2", start);
            used = 0;
        } else {
            memmove(buf, start, used);
        }
    }
    return 0;
}

void RedirectStdioToLogcat()
{
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);
    if (pipe(s_logPipe) != 0) return;
    dup2(s_logPipe[1], STDOUT_FILENO);
    dup2(s_logPipe[1], STDERR_FILENO);
    pthread_t t;
    if (pthread_create(&t, 0, LogThread, 0) == 0) pthread_detach(t);
}

void LoadEnvFile(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "r");
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

// Mounts main.<v>.<pkg>.obb and then patch.<v>.<pkg>.obb, the later mount winning, as initVfs does.
// (KOTOR 1: the engine itself looks in the patch archive first, then main.)
void MountObbs(const char* pszDir)
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
    if (main.empty())
        fprintf(stderr, "k2: no main.*.obb in %s -- install the expansion files (platforms/README.md)\n", pszDir);
}

// Pauses the audio device while the app is in the background. SDL sends these from inside the event
// pump, synchronously, before it blocks the main thread on an Android pause.
int AudioLifecycleWatch(void*, SDL_Event* e)
{
    if (e->type == SDL_APP_WILLENTERBACKGROUND) k2audio_set_suspended(1);
    else if (e->type == SDL_APP_DIDENTERFOREGROUND) k2audio_set_suspended(0);
    return 0;
}

#ifdef K2P_K1
// Installs the engine's FTGL dialog font: the APK's assets/iosdialog.otf, as override/iosdialog.otf
// under the working directory, where the engine opens it.
void InstallK1Font()
{
    const char* pszDst = "override/iosdialog.otf";
    struct stat st;
    if (stat(pszDst, &st) == 0 && st.st_size > 0) return;
    mkdir("override", 0770);
    SDL_RWops* src = SDL_RWFromFile("iosdialog.otf", "rb");   // falls through to the APK's assets
    if (!src) {
        fprintf(stderr, "k1: no iosdialog.otf in the APK's assets\n");
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

} // namespace

#ifdef K2P_K1
extern "C" int SDL_main(int argc, char** argv);

extern "C" int k1_main(int argc, char** argv)
#else
extern "C" int k2_main(int argc, char** argv)
#endif
{
    RedirectStdioToLogcat();

    const char* pszFiles = SDL_AndroidGetExternalStoragePath();
    std::string files = pszFiles ? pszFiles : ".";
    mkdir(files.c_str(), 0770);
    LoadEnvFile(files + "/k2env.txt");
    if (const char* dir = getenv("K2_DATA_DIR")) files = dir;
    if (chdir(files.c_str()) != 0) {
        fprintf(stderr, "k2: cannot chdir to %s\n", files.c_str());
        return 1;
    }
    fprintf(stderr, "k2: working directory %s\n", files.c_str());
    if (const char* obbDir = getenv("K2_OBB_DIR")) MountObbs(obbDir);
#ifdef K2P_K1
    InstallK1Font();
    k2qol_startup();
    SDL_AddEventWatch(AudioLifecycleWatch, 0);
    // SDL_main's Java screen queries (the stub JNIEnv answers K2_JNI_<method>): high resolution, so
    // the back buffer is the whole display.
    setenv("K2_JNI_GetHighResolution", "1", 0);
    setenv("K2_JNI_GetScreenHeightPixel", "4320", 0);
    fprintf(stderr, "k1: entering SDL_main\n");
    int nK1Rc = SDL_main(argc, argv);
    fprintf(stderr, "k1: SDL_main returned %d\n", nK1Rc);
    exit(nK1Rc);
#endif
#ifdef K2_QOL_CONTROLLER
    k2qol_startup();
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
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
    // The engine's globals do not survive a second WinMain in the same process -- the activity is
    // singleInstance, but Android keeps a finished app's process around -- so end the process.
    exit(rc);
}
