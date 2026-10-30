// macOS entry point for the host build.
//
// On Android the process entry is SDL_main: it registers ASL's string table and event watch, mounts
// the OBB VFS, then hands WinMain to ASL::Main, which owns the SDL window and GL context. Here the
// minimum SDL bring-up is done and WinMain called directly; anything WinMain reaches that nobody
// implements lands in a generated stub that prints its name and aborts.
//
// Where the game is: an installed copy -- Steam, GOG, disc, the Mac bundles -- found in a fixed list of
// places, else the working directory. The game's files become the working directory and the player's
// (saves/, swkotor.ini) the shim's user directory. Ahead of that search:
//   K2_DATA_DIR=<dir>   one folder for both (the game's data root, writable)
//   K2_ASSET_DIR=<dir>  the game's files only (the folder with chitin.key)
//   K2_USER_DIR=<dir>   the player's files, wherever the game's came from
//   K2_WORKSHOP=0       leave out KOTOR II's Steam Workshop items (mounted as its mod folder)
//   K2_CLOUDSAVES=0     keep saves/ in the player's folder, not the install's Steam Cloud folder
//   K2_OBB_DIR=<dir>    run off the game's Android expansion files in <dir> (main.<v>.<package>.obb,
//                       patch.<v>.<package>.obb; both games' can share it), mounted under the working
//                       directory, read-only, as Android does. The older K2_OBB=<obb>;<obb> (main
//                       first; ':' also separates, but not on Windows) and KOTOR 1's
//                       K1_OBB_DIR=<dir> still work.
//   K2_STUBS=soft       log each stub once and return 0 instead of aborting (exploration only)
// With none of them and no game found, a message box says so and the program ends.
//
// KOTOR 1 (K2P_GAME=k1, K2P_K1): the engine's own SDL_main is the app, as on Android -- it creates the
// window and runs the frame loop. Before it, this mounts K1's two OBBs the way the Java side did and
// answers the Java queries SDL_main makes:
//   K2_OBB_DIR=<dir>    as above: main.53.com.aspyr.swkotor.obb / patch.53... (default: the game's
//                       folder, when they are there). Without them the game's files are read loose
//                       from the working directory -- a PC data set, the Steam install's
//                       Contents/Assets or a copy of it.
//   K1_WINDOW=<w>x<h>   the window, which K1 takes for the whole display
//   K1_CHROMEBOOK=1     K1's Chromebook mode: SDL_main reads the mouse and the keyboard keys itself.
//                       Default: on in the pc UI (=0 turns it off), else the mouse becomes a finger,
//                       as the device's touch screen.
#include <SDL.h>
#ifdef _WIN32
#include "build_id.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <string>
#include <vector>

#include "gamedirs.h"
#include "../../src/port/k2qolapi.h"

struct HINSTANCE__;
extern "C" int WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* pszCmdLine, int nCmdShow);
bool k2shim_obb_mount(const char* pszObbPath);
void k2shim_set_user_dir(const char* pszAbsDir);
void k2shim_add_mount(const char* pszRelDir, const char* pszAbsDir);
void k2shim_redirect(const char* pszRelDir, const char* pszAbsDir);
extern "C" void k2host_achievements_init(int nGame);

#ifdef K2P_K1
#include "../k1/k1port.h"
extern "C" int SDL_main(int argc, char** argv);
extern "C" int SDL_IsChromebook(void);

namespace {
// K1's SDL_main asks Java for the screen, and the stub JNIEnv answers each method from
// K2_JNI_<method>. High resolution, and a pixel height no window reaches, so the back buffer is the
// whole window at the display's density.
void K1JavaAnswers()
{
    setenv("K2_JNI_GetHighResolution", "1", 0);
    setenv("K2_JNI_GetScreenHeightPixel", "4320", 0);
    setenv("K2_JNI_xdpi", "110", 0);   // DisplayMetrics: the tap / drag tolerances
    setenv("K2_JNI_ydpi", "110", 0);
}

// Mounts the two OBBs -- main, then patch -- else the working directory twice: SDL_main waits for
// both mounts, and the streamed sounds read only through the main one.
bool MountK1Obbs(const std::vector<std::string>& obbs)
{
    if (obbs.size() == 2) {
        k1port_mount_obb(obbs[0].c_str(), 0);
        k1port_mount_obb(obbs[1].c_str(), 1);
        return true;
    }
    if (access("chitin.key", R_OK) != 0) {
        fprintf(stderr, "k1: no OBBs (set K2_OBB_DIR) and no chitin.key in the game's folder\n");
        return false;
    }
    fprintf(stderr, "k1: no OBBs: reading the game's files from the working directory\n");
    k1port_mount_obb(".", 0);
    k1port_mount_obb(".", 1);
    return true;
}
} // namespace
#endif

#ifdef _WIN32
extern "C" void k2host_log_open(const char* pszDir, const char* pszName);
extern "C" void k2host_exit(int nCode);
#endif

namespace {
// Makes the game's files the working directory and the player's the shim's user directory, and fills
// in the expansion files to mount.
bool EnterGameDirs(std::vector<std::string>& obbs)
{
#ifdef K2P_K1
    const bool bKotor1 = true;
#else
    const bool bKotor1 = false;
#endif
    K2GameDirs dirs;
    std::string error;
    if (!k2_find_game_dirs(bKotor1, dirs, error)) {
        fprintf(stderr, "k2: %s\n", error.c_str());
        SDL_Init(SDL_INIT_VIDEO);   // macOS: a foreground app with a Dock icon, so the box can be found
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 bKotor1 ? "Knights of the Old Republic" : "Knights of the Old Republic II",
                                 error.c_str(), 0);
        SDL_Quit();
        return false;
    }
    if (chdir(dirs.assets.c_str()) != 0) {
        perror(dirs.assets.c_str());
        return false;
    }
    fprintf(stderr, "k2: game files: %s (%s)\n", dirs.assets.c_str(), dirs.how);
    if (dirs.user != dirs.assets) {
        k2shim_set_user_dir(dirs.user.c_str());   // creates it
        fprintf(stderr, "k2: player files: %s\n", dirs.user.c_str());
    }
#ifdef _WIN32
    // Beside swkotor.ini, k12.ini and achievements.ini in the player's folder, which exists by now.
    k2host_log_open(dirs.user.c_str(), bKotor1 ? "k1" : "k2");
#endif
    if (!dirs.saves.empty()) {
        k2shim_redirect("saves", dirs.saves.c_str());
        fprintf(stderr, "k2: saves: %s (Steam Cloud)\n", dirs.saves.c_str());
    }
    for (const std::string& mod : dirs.mods) {
        k2shim_add_mount("dlc/mods_english", mod.c_str());
        fprintf(stderr, "k2: Workshop item: %s (as dlc/mods_english)\n", mod.c_str());
    }
    for (const std::string& obb : dirs.obbs) fprintf(stderr, "k2: OBB: %s\n", obb.c_str());
    obbs = dirs.obbs;
    return true;
}
} // namespace

int main(int argc, char** argv)
{
#ifdef _WIN32
    // SDL picks its Windows condition-variable implementation in the first SDL_CreateCond, and the
    // engine waits on conds only Android's Java side creates (g_UI_cond,
    // CSWGuiMainMenu::HandlePrivacyChoice), so pick one now: until then a wait calls through null.
    SDL_DestroyCond(SDL_CreateCond());
    // Per-monitor DPI aware: sizes are the display's pixels. Unaware, Windows reports a scaled
    // desktop (1920x1080 at 150% is 1280x720) and stretches the window, so the mode list and
    // fullscreen size shrink to that and everything draws blurred.
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    fprintf(stderr, "k2: build %s\n", K2P_BUILD_ID);
#endif
    std::vector<std::string> obbs;
    if (!EnterGameDirs(obbs))
        return 1;
    // Steam, or the local record in the player's folder.
#ifdef K2P_K1
    k2host_achievements_init(1);
#else
    k2host_achievements_init(2);
#endif
#ifdef K2P_K1
    K1JavaAnswers();
    if (!MountK1Obbs(obbs))
        return 1;
    k2qol_startup();
#ifndef K1_UI_XBOX   // xbox: the mouse is a pointer (MacMain.cpp), not a finger
    if (!SDL_IsChromebook()) SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#endif
    fprintf(stderr, "k1: entering SDL_main\n");
    SDL_SetMainReady();
    {
        int rc = SDL_main(argc, argv);
#ifdef _WIN32
        k2host_exit(rc);
#endif
        return rc;
    }
#endif
    for (const std::string& obb : obbs)   // main first: the later mount wins
        k2shim_obb_mount(obb.c_str());
#ifdef K2_QOL_CONTROLLER
    k2qol_startup();
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    // Desktop GL in a 2.1 compatibility context: the engine renders with the GL 1.x/ARB surface, which
    // ASLgl wraps down to GLES on Android. macOS only offers that surface as a compatibility profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifdef K2_QOL_BUMPMAPS
    // Destination alpha: the bumpy-shiny pass weights its reflection by 1 - the base pass's alpha
    // (RenderEMBM's InvDstAlpha / One), as the PC's 32-bit modes had.
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
#endif

    char cmdline[1024] = "";
    for (int i = 1; i < argc; ++i) {
        strlcat(cmdline, argv[i], sizeof cmdline);
        if (i + 1 < argc)
            strlcat(cmdline, " ", sizeof cmdline);
    }
    fprintf(stderr, "k2: entering WinMain(\"%s\")\n", cmdline);
    int rc = WinMain(0, 0, cmdline, 1);
    SDL_Quit();
#ifdef _WIN32
    k2host_exit(rc);
#endif
    return rc;
}
