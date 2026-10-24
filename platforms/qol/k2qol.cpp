// k2qol.cpp - K2_QOL_CONTROLLER (and K2_QOL_IMPROVEMENTS), the OS half: the settings, the
// button-prompt texture rename, the Switch / GameCube prompt art, the dialog-text-size bookkeeping
// and the rumble motors. The page itself is qol/k2qolpage.cpp, compiled with the engine; the engine
// reaches this file through the extern "C" k2qol_* calls declared, under `#ifdef
// K2_QOL_CONTROLLER` (prompts, rumble) or `#ifdef K2_QOL_IMPROVEMENTS` (pitch, text size), at each
// src/ call site:
//
//   AurTextureGet (aurtexman.cpp)            k2qol_texture_name: the prompt textures renamed
//   CClientExoAppInternal::UpdateCamera      k2qol_no_pitch: the right stick's Y is not fed to
//                                            AcclTiltCamera (free-look's own tilt is untouched)
//   NewCAurGUIString (aurgui.cpp)            k2qol_text_size_factor: the size factor of a string
//                                            built inside a dialog-text scope ...
//   the CSWGuiDialog* / CSWGuiBarkBubble     ... which those panels' constructors and SetReplies
//   constructors and SetReplies              open (k2qol_dialog_text_begin / _end)
//   wherever reading text is laid out        K2QolFitText (qol/k2qolpage.cpp): each string as big
//   (dialog panels, load tip, pop-ups and    as the setting asks and its box can still draw
//   datapads, HUD caption + target name,
//   Inventory / Equip / Abilities)
//   CExoInputInternal::SetRumble             k2qol_set_rumble_motors: the motors
//   shim/shim_dinput.cpp MergedButton,       k2qol_swap_face_buttons: SDL's A/B and X/Y read
//   k1/k1sdl.cpp SDL_PollEvent               swapped, for every consumer of the pad
//
// Which code is building the string decides the dialog text size, through a scope the dialog panels
// open instead of a frame-pointer walk. Only the methods that build the panels' strings open one
// (the constructors' layout loads, SetReplies' row clones), never an input handler, so a panel a
// conversation opens (a lab station, a store) is outside it. How big each string may then get is
// not a per-panel cap but a fit to its own box, done where the panel lays the text out:
// qol/k2qolpage.cpp's K2QolFitText.
//
// Settings: `k12.ini`, both games, every platform. On a desktop it is in the player's folder, beside
// swkotor.ini / swkotor2.ini and saves/: opened by its bare name, so the shim's user-data layer
// reads it there (else from the working directory) and writes it there (shim_path.cpp). On Android
// and iOS it is in the working directory (the engine's DOCUMENTS:). `[Controller]`
// `ButtonPrompts=auto|xbox|playstation|switch`, `DisableRightStickPitch=0|1`,
// `Rumble=off|controller|phone`, `SwapFaceButtons=0|1`, `[Display]` `DialogTextScale=25..250`,
// `[Gameplay]` `FasterMovement=0|1` (both games; default 1: KOTOR 1 on Android scales the player's
// direct-control top speed by 1.3, CSWPlayerControlCamRelative::GetMaxSpeed). The `GamepadUI` key
// (the runtime console-UI switch; here the UI is the build) is kept as found. One launch's
// override: K2_BUTTON_PROMPTS, K2_NO_PITCH, K2_RUMBLE, K2_SWAP_FACE, K2_DIALOG_TEXTSCALE,
// K2_FASTER_MOVEMENT.
//
// KOTOR 1 (K2_QOL_K1; its page has the prompts, rumble, swap, movement and achievements rows):
// settings in `k12.ini` (`ButtonPrompts`, `Rumble`, `SwapFaceButtons`, `FasterMovement`, and
// `[Gameplay]` `Achievements=0|1`, default 1: off, an unlock is neither recorded nor announced,
// shim_achievements.cpp; one launch's override K2_ACHIEVEMENTS), the prompt art (padglyphs.py
// --game k1) in `override/`, and its prompts are not KOTOR II's textures but K1's own: the
// ios_button_* icons, the tutorial boxes' abutton..ybutton (black / white / start under Switch),
// and the A/B/X/Y cells of the dialog font's atlases, which the K1 string binds in their place
// (k2qol_glyph_atlas, aurgui.cpp).
#ifdef K2_QOL_CONTROLLER

#include <SDL.h>
#include "port/k2qolapi.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <dlfcn.h>
#include <jni.h>
#endif

#ifndef K2P_MOBILE_HOST
const char* k2shim_user_dir();   // shim/shim_path.cpp: the player's folder, or "" (the working directory)
#endif

#ifndef K2_QOL_K1
bool IsPsButtonIcons();   // the engine's own pick (src/game/clientcore/clientexoappinternal.cpp)
#endif

extern "C" {
struct K2Asset { const char* name; const uint8_t* data; size_t size; };
extern const K2Asset k2_assets[];         // gen/k2assets.gen.cpp (platforms/tools/padglyphs.py)
extern const unsigned k2_asset_count;
}

#define LOG(...) (fprintf(stderr, "k2: qol: " __VA_ARGS__), fputc('\n', stderr))

namespace {

enum { ICONS_AUTO = 0, ICONS_XBOX, ICONS_PS, ICONS_SWITCH, ICONS_GAMECUBE, ICONS_COUNT };
const char* const kIconNames[ICONS_COUNT] = {"auto", "xbox", "playstation", "switch", "gamecube"};
// The suffix of each set's art (padglyphs.py's outputs); Automatic has none of its own.
const char* const kIconSuffix[ICONS_COUNT] = {"", "an", "ps", "nx", "gc"};

enum { RUMBLE_OFF = 0, RUMBLE_CONTROLLER, RUMBLE_PHONE, RUMBLE_COUNT };
const char* const kRumbleNames[RUMBLE_COUNT] = {"off", "controller", "phone"};

int  g_icons = ICONS_AUTO;
int  g_noPitch = 0;
int  g_dialogPct = 100;
int  g_rumble = RUMBLE_CONTROLLER;
int  g_swapFace = 0;            // SDL's A <-> B and X <-> Y (a pad that reports them crossed)
int  g_fasterMovement = 1;      // the drive speed x1.3, as KOTOR 1 on Android
#ifdef K2_QOL_K1
int  g_achievements = 1;        // KOTOR 1's local achievements: recorded and announced
#endif
int  g_gamepadUi = -1;          // the runtime console-UI key, written back as found (-1: absent)
bool g_haveSwitchArt = false;
bool g_haveGcArt = false;
bool g_haveShoulders = false;   // K2: the shoulder icons and the atlases that carry them
bool g_haveK1Art[ICONS_COUNT];  // K1: per style, its icons and both atlases installed
bool g_haveK1Own = false;       // K1: its own set's shoulder icons and atlases (k1fnt*_io)
bool g_assetOk[256];            // per k2_assets entry: in the override folder this launch

// Whether every named asset (".tga" implied) is in the override folder.
bool have_assets(const char* const* names) {
    for (; *names; ++names) {
        bool found = false;
        for (unsigned i = 0; i < k2_asset_count && i < 256 && !found; ++i) {
            size_t n = strlen(*names);
            found = g_assetOk[i] && !strncmp(k2_assets[i].name, *names, n) &&
                    !strcmp(k2_assets[i].name + n, ".tga");
        }
        if (!found) return false;
    }
    return true;
}
bool g_started = false;
char g_settingsPath[1024];
#ifndef K2P_MOBILE_HOST
char g_settingsShown[1024];   // where k12.ini is, for the log
#endif

int parse_icons(const char* v) {
    for (int i = 0; i < ICONS_COUNT; ++i)
        if (!strcasecmp(v, kIconNames[i])) return i;
    if (!strcasecmp(v, "ps") || !strcasecmp(v, "ps4") || !strcasecmp(v, "ps5")) return ICONS_PS;
    if (!strcasecmp(v, "nintendo") || !strcasecmp(v, "nx")) return ICONS_SWITCH;
    if (!strcasecmp(v, "gc") || !strcasecmp(v, "ngc")) return ICONS_GAMECUBE;
    return -1;
}

int parse_rumble(const char* v) {
    for (int i = 0; i < RUMBLE_COUNT; ++i)
        if (!strcasecmp(v, kRumbleNames[i])) return i;
    if (!strcasecmp(v, "device")) return RUMBLE_PHONE;
    return -1;
}

// 25..250 in 25 % steps (the page's step); anything else snaps to the nearest step.
int clamp_pct(int n) {
    n = n < 25 ? 25 : n > 250 ? 250 : n;
    return (n + 12) / 25 * 25;
}

void save_settings();

void load_settings() {
    FILE* f = fopen(g_settingsPath, "r");
#ifndef K2P_MOBILE_HOST
    const char* pszFrom = g_settingsShown;
#else
    const char* pszFrom = g_settingsPath;
#endif
    if (f) {
        char line[256];
        while (fgets(line, sizeof line, f)) {
            line[strcspn(line, "\r\n")] = 0;
            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = 0;
            const char* v = eq + 1;
            if (!strcasecmp(line, "ButtonPrompts")) {
                int s = parse_icons(v);
                if (s >= 0) g_icons = s;
            } else if (!strcasecmp(line, "DisableRightStickPitch")) {
                g_noPitch = atoi(v) != 0;
            } else if (!strcasecmp(line, "DialogTextScale")) {
                g_dialogPct = clamp_pct(atoi(v));
            } else if (!strcasecmp(line, "Rumble")) {
                int r = parse_rumble(v);
                if (r >= 0) g_rumble = r;
            } else if (!strcasecmp(line, "SwapFaceButtons")) {
                g_swapFace = atoi(v) != 0;
            } else if (!strcasecmp(line, "FasterMovement")) {
                g_fasterMovement = atoi(v) != 0;
#ifdef K2_QOL_K1
            } else if (!strcasecmp(line, "Achievements")) {
                g_achievements = atoi(v) != 0;
#endif
            } else if (!strcasecmp(line, "GamepadUI")) {
                g_gamepadUi = atoi(v) != 0;
            }
        }
        fclose(f);
    }
    if (const char* v = getenv("K2_BUTTON_PROMPTS")) {
        int s = parse_icons(v);
        if (s >= 0) g_icons = s;
    }
    if (const char* v = getenv("K2_NO_PITCH")) g_noPitch = atoi(v) != 0;
    if (const char* v = getenv("K2_RUMBLE")) {
        int r = parse_rumble(v);
        if (r >= 0) g_rumble = r;
    }
    if (const char* v = getenv("K2_SWAP_FACE")) g_swapFace = atoi(v) != 0;
    if (const char* v = getenv("K2_FASTER_MOVEMENT")) g_fasterMovement = atoi(v) != 0;
#ifdef K2_QOL_K1
    if (const char* v = getenv("K2_ACHIEVEMENTS")) g_achievements = atoi(v) != 0;
#endif
    if (const char* v = getenv("K2_DIALOG_TEXTSCALE")) {
        if (atoi(v) > 0) g_dialogPct = clamp_pct(atoi(v));
    }
#ifdef K2_QOL_K1
    LOG("settings: ButtonPrompts=%s Rumble=%s SwapFaceButtons=%d FasterMovement=%d Achievements=%d (%s)",
        kIconNames[g_icons], kRumbleNames[g_rumble], g_swapFace, g_fasterMovement, g_achievements,
        f ? pszFrom : "no settings file yet");
#else
    LOG("settings: ButtonPrompts=%s DisableRightStickPitch=%d Rumble=%s SwapFaceButtons=%d "
        "DialogTextScale=%d FasterMovement=%d (%s)", kIconNames[g_icons], g_noPitch, kRumbleNames[g_rumble],
        g_swapFace, g_dialogPct, g_fasterMovement, f ? pszFrom : "no settings file yet");
#endif
}

void save_settings() {
    if (!g_settingsPath[0]) return;
    char tmp[1040];
    snprintf(tmp, sizeof tmp, "%s.tmp", g_settingsPath);
    FILE* f = fopen(tmp, "w");
    if (!f) {
        LOG("cannot write %s: %s", tmp, strerror(errno));
        return;
    }
#ifdef K2_QOL_K1
    fprintf(f, "[Controller]\nButtonPrompts=%s\nRumble=%s\nSwapFaceButtons=%d\n",
            kIconNames[g_icons], kRumbleNames[g_rumble], g_swapFace);
#else
    fprintf(f, "[Controller]\nButtonPrompts=%s\nDisableRightStickPitch=%d\nRumble=%s\n"
            "SwapFaceButtons=%d\n", kIconNames[g_icons], g_noPitch, kRumbleNames[g_rumble], g_swapFace);
    if (g_gamepadUi >= 0) fprintf(f, "GamepadUI=%d\n", g_gamepadUi);
    fprintf(f, "[Display]\nDialogTextScale=%d\n", g_dialogPct);
#endif
    fprintf(f, "[Gameplay]\nFasterMovement=%d\n", g_fasterMovement);
#ifdef K2_QOL_K1
    fprintf(f, "Achievements=%d\n", g_achievements);
#endif
    bool ok = !ferror(f);
    if (fclose(f) != 0 || !ok || rename(tmp, g_settingsPath) != 0)
        LOG("cannot replace %s: %s", g_settingsPath, strerror(errno));
}

bool mkdirs(const char* path) {
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", path);
    for (char* p = buf + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = 0;
        if (mkdir(buf, 0775) != 0 && errno != EEXIST) return false;
        *p = '/';
    }
    return mkdir(buf, 0775) == 0 || errno == EEXIST;
}

// The prompt art goes into `DOCUMENTS:dlc\mods_english\override`, one of the override directories
// the engine registers at startup (GetOverridePath index 1/3); DOCUMENTS: is the working directory.
// KOTOR 1's goes into `override/` in its working directory. Written only when missing or
// different. It must be there before the resource manager indexes that directory, which is why
// k2qol_startup runs before WinMain (K1: before SDL_main).
void install_assets() {
    if (!k2_asset_count) {
#ifdef K2_QOL_K1
        LOG("this build carries no prompt art (padglyphs.py found no source data); every choice "
            "shows KOTOR's own prompts");
#else
        LOG("this build carries no prompt art (padglyphs.py found no rsvg-convert or font "
            "atlases); \"Switch\" and \"GameCube\" show the Xbox prompts");
#endif
        return;
    }
#ifdef K2_QOL_K1
    const char* dir = "override";
    int aHave[ICONS_COUNT] = {0};
#else
    const char* dir = "dlc/mods_english/override";
#endif
    if (!mkdirs(dir)) {
        LOG("cannot create %s: %s; no Switch / GameCube prompt art this launch", dir, strerror(errno));
        return;
    }
    unsigned written = 0, ok = 0;
    for (unsigned i = 0; i < k2_asset_count; ++i) {
        const K2Asset& a = k2_assets[i];
        char path[512];
        snprintf(path, sizeof path, "%s/%s", dir, a.name);
        bool same = false;
        if (FILE* f = fopen(path, "rb")) {
            // One byte of slack makes a file longer than the asset read back as a mismatch.
            char* buf = static_cast<char*>(malloc(a.size + 1));
            same = buf && fread(buf, 1, a.size + 1, f) == a.size && !memcmp(buf, a.data, a.size);
            free(buf);
            fclose(f);
        }
        if (!same) {
            FILE* f = fopen(path, "wb");
            bool good = f && fwrite(a.data, 1, a.size, f) == a.size;
            if (f && fclose(f) != 0) good = false;
            if (!good) {
                LOG("cannot write %s: %s", path, strerror(errno));
                continue;
            }
            ++written;
        }
        ++ok;
        if (i < 256) g_assetOk[i] = true;
#ifdef K2_QOL_K1
        // ios_button_<k><style>.tga (a/b/x/y/l/r), k1fnt16_<style>.tga, k1fnt32_<style>.tga: eight
        // per style.
        size_t n = strlen(a.name);
        if (n > 7 && (!strncmp(a.name, "ios_button_", 11) || !strncmp(a.name, "k1fnt", 5))) {
            const char* sfx = a.name + n - 6;   // "an.tga"
            for (int s = ICONS_XBOX; s < ICONS_COUNT; ++s)
                if (!strncmp(sfx, kIconSuffix[s], 2))
                    ++aHave[s];
        }
#endif
    }
#ifdef K2_QOL_K1
    for (int s = ICONS_XBOX; s < ICONS_COUNT; ++s)
        g_haveK1Art[s] = aHave[s] == 8;
    static const char* const kOwn[] = {"ios_button_l", "ios_button_r", "k1fnt16_io", "k1fnt32_io",
                                       0};
    g_haveK1Own = have_assets(kOwn);
    LOG("prompt art: %u file(s) in %s (%u written this launch); Xbox %s, PlayStation %s, Switch %s, "
        "GameCube %s, own shoulders %s",
        ok, dir, written, g_haveK1Art[ICONS_XBOX] ? "yes" : "no",
        g_haveK1Art[ICONS_PS] ? "yes" : "no", g_haveK1Art[ICONS_SWITCH] ? "yes" : "no",
        g_haveK1Art[ICONS_GAMECUBE] ? "yes" : "no", g_haveK1Own ? "yes" : "no");
#else
    static const char* const kSwitch[] = {
        "cus_button_anx", "cus_button_bnx", "cus_button_xnx", "cus_button_ynx", "cus_button_lnx",
        "cus_button_rnx", "k2fnt16_nx", "k2fnt32_nx", "k2fnt64_nx", "k2nx_a", "k2nx_b", "k2nx_x",
        "k2nx_y", "k2nx_black", "k2nx_white", "k2nx_ltrig", "k2nx_rtrig", "k2nx_start", "k2nx_back",
        "k2nx_4way", "k2nx_r_portsel", "k2nx_lstick", "k2nx_rstick", "k2nx_rstick_ud", 0};
    static const char* const kShoulders[] = {"cus_button_lan", "cus_button_ran", "cus_button_lps",
                                             "cus_button_rps", "k2fnt16_an", "k2fnt16_ps",
                                             "k2fnt32_ps", "k2fnt64_ps", 0};
    static const char* const kGameCube[] = {
        "cus_button_agc", "cus_button_bgc", "cus_button_xgc", "cus_button_ygc", "cus_button_lgc",
        "cus_button_rgc", "k2fnt16_gc", "k2fnt32_gc", "k2fnt64_gc", "k2gc_a", "k2gc_b", "k2gc_x",
        "k2gc_y", "k2gc_black", "k2gc_white", "k2gc_ltrig", "k2gc_rtrig", "k2gc_start", "k2gc_back",
        "k2gc_4way", "k2gc_r_portsel", "k2gc_lstick", "k2gc_rstick", "k2gc_rstick_ud", 0};
    g_haveSwitchArt = have_assets(kSwitch);
    g_haveGcArt = have_assets(kGameCube);
    g_haveShoulders = have_assets(kShoulders);
    LOG("prompt art: %u file(s) in %s (%u written this launch); Switch %s, GameCube %s, shoulders %s",
        ok, dir, written, g_haveSwitchArt ? "yes" : "no", g_haveGcArt ? "yes" : "no",
        g_haveShoulders ? "yes" : "no");
#endif
}

#ifdef K2_QOL_K1
// KOTOR 1's input layer has no "is it a PlayStation pad" flag (KOTOR II's IsPsButtonIcons): ask
// SDL about the pads it lists.
bool playstation_pad() {
    for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
        if (!SDL_IsGameController(i)) continue;
        SDL_GameControllerType t = SDL_GameControllerTypeForIndex(i);
        if (t == SDL_CONTROLLER_TYPE_PS3 || t == SDL_CONTROLLER_TYPE_PS4 || t == SDL_CONTROLLER_TYPE_PS5)
            return true;
    }
    return false;
}

// KOTOR 1's own A/B/X/Y art is the Android data's (the patch OBB's override/ios_button_?.tga). A PC
// install has none, and its ?button textures are keyboard keycaps, so there the automatic choice is
// the Xbox set (effective_icons).
extern "C" int k1port_data_has(const char* pszName);   // k1/k1port.cpp
bool k1_own_face_buttons() {
    static int s_nHave = -1;
    if (s_nHave < 0) s_nHave = k1port_data_has("override/ios_button_a.tga");
    return s_nHave != 0;
}

// ICONS_AUTO here means KOTOR 1's own art (no rename); a set this build lacks falls back to it.
// Automatic on data without K1's own face buttons is the Xbox set for every prompt -- icons, tutorial
// boxes and the dialog font's cells alike -- so one screen never mixes the two.
int effective_icons() {
    int s = g_icons;
    if (s == ICONS_AUTO) s = playstation_pad() ? ICONS_PS : k1_own_face_buttons() ? ICONS_AUTO : ICONS_XBOX;
    return g_haveK1Art[s] ? s : ICONS_AUTO;
}

#else

int effective_icons() {
    int s = g_icons;
    if (s == ICONS_AUTO) s = IsPsButtonIcons() ? ICONS_PS : ICONS_XBOX;
    if ((s == ICONS_SWITCH && !g_haveSwitchArt) || (s == ICONS_GAMECUBE && !g_haveGcArt))
        s = ICONS_XBOX;
    return s;
}
#endif

}  // namespace

// ---- startup + the settings API the page uses ---------------------------------------------------

// From the platform main, after the chdir to the data directory and before WinMain.
extern "C" void k2qol_startup(void) {
    if (g_started) return;
    g_started = true;
    char cwd[900];
    if (!getcwd(cwd, sizeof cwd)) strcpy(cwd, ".");
#ifndef K2P_MOBILE_HOST
    snprintf(g_settingsPath, sizeof g_settingsPath, "k12.ini");
    const char* pszUser = k2shim_user_dir();
    snprintf(g_settingsShown, sizeof g_settingsShown, "%s/k12.ini", pszUser && *pszUser ? pszUser : cwd);
#else
    snprintf(g_settingsPath, sizeof g_settingsPath, "%s/k12.ini", cwd);
#endif
    load_settings();
    install_assets();
}

extern "C" int k2qol_icons(void) { return g_icons; }
extern "C" int k2qol_no_pitch(void) { return g_noPitch; }
// Whether a button-prompts choice has its art in this build (Automatic always does).
extern "C" int k2qol_have_art(int nStyle) {
#ifdef K2_QOL_K1
    return nStyle == ICONS_AUTO || (nStyle > 0 && nStyle < ICONS_COUNT && g_haveK1Art[nStyle]);
#else
    return nStyle == ICONS_SWITCH ? g_haveSwitchArt : nStyle == ICONS_GAMECUBE ? g_haveGcArt : true;
#endif
}
extern "C" int k2qol_dialog_text_scale(void) { return g_dialogPct; }
extern "C" int k2qol_rumble(void) { return g_rumble; }
extern "C" int k2qol_swap_face_buttons(void) { return g_swapFace; }

extern "C" void k2qol_set_swap_face_buttons(int bOn) {
    if ((bOn != 0) == (g_swapFace != 0)) return;
    g_swapFace = bOn != 0;
    LOG("SwapFaceButtons -> %d", g_swapFace);
    save_settings();
}

extern "C" int k2qol_faster_movement(void) { return g_fasterMovement; }

extern "C" void k2qol_set_faster_movement(int bOn) {
    if ((bOn != 0) == (g_fasterMovement != 0)) return;
    g_fasterMovement = bOn != 0;
    LOG("FasterMovement -> %d", g_fasterMovement);
    save_settings();
}

#ifdef K2_QOL_K1
extern "C" int k2qol_achievements(void) { return g_achievements; }

extern "C" void k2qol_set_achievements(int bOn) {
    if ((bOn != 0) == (g_achievements != 0)) return;
    g_achievements = bOn != 0;
    LOG("Achievements -> %d", g_achievements);
    save_settings();
}
#endif

extern "C" void k2qol_set_icons(int nStyle) {
    if (nStyle < 0 || nStyle >= ICONS_COUNT || nStyle == g_icons) return;
    g_icons = nStyle;
    LOG("ButtonPrompts -> %s", kIconNames[nStyle]);
    save_settings();
}

extern "C" void k2qol_set_no_pitch(int bOn) {
    if ((bOn != 0) == (g_noPitch != 0)) return;
    g_noPitch = bOn != 0;
    LOG("DisableRightStickPitch -> %d", g_noPitch);
    save_settings();
}

extern "C" void k2qol_set_dialog_text_scale(int nPercent) {
    nPercent = clamp_pct(nPercent);
    if (nPercent == g_dialogPct) return;
    g_dialogPct = nPercent;
    LOG("DialogTextScale -> %d", nPercent);
    save_settings();
}

extern "C" void k2qol_set_rumble(int nMode) {
    if (nMode < 0 || nMode >= RUMBLE_COUNT || nMode == g_rumble) return;
    g_rumble = nMode;
    LOG("Rumble -> %s", kRumbleNames[nMode]);
    save_settings();
}

// ---- button prompts: the texture rename (AurTextureGet) ---------------------------------------
//
//   engine asks for               Xbox                 PlayStation          Switch (GameCube: gc)
//   cus_button_?an / ?ps          cus_button_?an       cus_button_?ps       cus_button_?nx
//   diafnt16x16b_an / _ps,        diafnt16x16b_an      diafnt16x16b_ps      k2fnt16_nx /
//   d2x/d3xfont16x16b_ps          (all three slots)    (_an -> _ps)         k2fnt32_nx /
//                                                                           k2fnt64_nx by slot
//   uibut_a/b/x/y (console _x)    unchanged            cus_button_?ps       k2nx_a/b/x/y
//   uibut_<other> (console _x)    unchanged            unchanged            k2nx_<other> (the
//                                                      Switch port's uibut_* set, padglyphs NX_UIBUT;
//                                                      GameCube: k2gc_<other>, GC_UIBUT)
//   cus_button_l/r?? (shoulders)  cus_button_?an       cus_button_?ps       cus_button_?nx
//   ios_button_l / _r             (the same: IosFloatingButtonIcon's names for them)
// With the shoulder art (g_haveShoulders) the Xbox / PlayStation atlases are the port's copies,
// whose atlases carry the shoulder cells: k2fnt16_an for every Xbox slot, k2fnt16_ps / k2fnt32_ps /
// k2fnt64_ps by slot.
//
// A texture is resolved when its control or string is created, so a prompt already on screen keeps
// its art until its screen is rebuilt. Null = keep the name. The result is one static buffer:
// AurTextureGet copies the name before it returns, and it runs on the game thread.
//
// KOTOR 1 (K1's own art when effective_icons() is ICONS_AUTO; on data without K1's own A/B/X/Y, a PC
// install, Automatic is the Xbox set):
//   engine asks for               Xbox / PlayStation / Switch / GameCube
//   ios_button_a/b/x/y/l/r        ios_button_?an / ?ps / ?nx / ?gc (l / r: the shoulders; K1's own set
//                                 has them as ios_button_l / _r, padglyphs.py's)
//   abutton..ybutton (xbox UI)    the same (the tutorial boxes' icons)
//   blackbutton / whitebutton /   Switch and GameCube only: blackbutton_nx / whitebutton_nx /
//   startbutton (xbox UI)         startbutton_nx (R / L / +, as the Switch port files them), _gc
//                                 (Z / ZL / Start)
#ifdef K2_QOL_K1
extern "C" const char* k2qol_texture_name(const char* name) {
    static char s_out[32];
    char c = name[0] | 0x20;
    if (c != 'i' && c != 'a' && c != 'b' && c != 'x' && c != 'y' && c != 'w' && c != 's') return 0;
    int style = effective_icons();
    size_t len = strnlen(name, 32);
    if (style == ICONS_AUTO) return 0;
    if (len == 12 && !strncasecmp(name, "ios_button_", 11) && strchr("abxylr", name[11] | 0x20)) {
        snprintf(s_out, sizeof s_out, "ios_button_%c%s", name[11] | 0x20, kIconSuffix[style]);
        return s_out;
    }
#ifdef K1_UI_XBOX
    if (len == 7 && !strcasecmp(name + 1, "button") && strchr("abxy", c)) {
        snprintf(s_out, sizeof s_out, "ios_button_%c%s", c, kIconSuffix[style]);
        return s_out;
    }
    if ((style == ICONS_SWITCH || style == ICONS_GAMECUBE) &&
        (!strcasecmp(name, "blackbutton") || !strcasecmp(name, "whitebutton") ||
         !strcasecmp(name, "startbutton"))) {
        snprintf(s_out, sizeof s_out, "%c%s_%s", c, name + 1, kIconSuffix[style]);
        return s_out;
    }
#endif
    return 0;
}

// Returns the atlas a K1 dialog-font string binds instead of its own for the prompt choice: nSize
// 16 for dialogfont16x16b, 32 for d2xfont16x16b. Null = K1's own atlas; K1's own art is its copy
// with the shoulder cells (k1fnt16_io / k1fnt32_io) when this build has it.
extern "C" const char* k2qol_glyph_atlas(int nSize) {
    static char s_out[2][16];
    int style = effective_icons();
    if (style == ICONS_AUTO && !g_haveK1Own) return 0;
    char* out = s_out[nSize == 32];
    snprintf(out, sizeof s_out[0], "k1fnt%d_%s", nSize == 32 ? 32 : 16,
             style == ICONS_AUTO ? "io" : kIconSuffix[style]);
    return out;
}
#else
extern "C" const char* k2qol_texture_name(const char* name) {
    static char s_out[32];
    char c = name[0] | 0x20;
    if (c != 'c' && c != 'd' && c != 'u' && c != 'i') return 0;
    size_t len = strnlen(name, 32);
    if (len == 12 && !strncasecmp(name, "ios_button_", 11)) {
        char k = name[11] | 0x20;
        if ((k != 'l' && k != 'r') || !g_haveShoulders) return 0;
        int style = effective_icons();
        snprintf(s_out, sizeof s_out, "cus_button_%c%s", k, kIconSuffix[style]);
    } else if (len == 14 && !strncasecmp(name, "cus_button_", 11)) {
        char k = name[11] | 0x20;
        if (!strchr("abxy", k) && !((k == 'l' || k == 'r') && g_haveShoulders)) return 0;
        if (strcasecmp(name + 12, "an") && strcasecmp(name + 12, "ps")) return 0;
        int style = effective_icons();
        snprintf(s_out, sizeof s_out, "cus_button_%c%s", k, kIconSuffix[style]);
    } else if (len > 6 && !strncasecmp(name, "uibut_", 6)) {
        int style = effective_icons();
        if (style == ICONS_SWITCH || style == ICONS_GAMECUBE) {
            static const char* const kNx[] = {"a", "b", "x", "y", "black", "white", "ltrig", "rtrig",
                                              "start", "back", "4way", "r_portsel", "lstick",
                                              "rstick", "rstick_ud", 0};
            const char* const* p = kNx;
            while (*p && strcasecmp(name + 6, *p)) ++p;
            if (!*p) return 0;
            snprintf(s_out, sizeof s_out, "k2%s_%s", kIconSuffix[style], *p);
        } else {
            char k = name[6] | 0x20;
            if (len != 7 || !strchr("abxy", k) || style != ICONS_PS) return 0;
            snprintf(s_out, sizeof s_out, "cus_button_%cps", k);
        }
    } else if (!strcasecmp(name, "diafnt16x16b_an") || !strcasecmp(name, "diafnt16x16b_ps") ||
               !strcasecmp(name, "d2xfont16x16b_ps") || !strcasecmp(name, "d3xfont16x16b_ps")) {
        int style = effective_icons();
        const int nSize = (name[1] | 0x20) == '2' ? 32 : (name[1] | 0x20) == '3' ? 64 : 16;
        if (g_haveShoulders && style == ICONS_XBOX)
            snprintf(s_out, sizeof s_out, "k2fnt16_an");
        else if ((g_haveShoulders && style == ICONS_PS) || style == ICONS_SWITCH ||
                 style == ICONS_GAMECUBE)
            snprintf(s_out, sizeof s_out, "k2fnt%d_%s", nSize, kIconSuffix[style]);
        else if (style == ICONS_XBOX)
            snprintf(s_out, sizeof s_out, "diafnt16x16b_an");
        else if (!strcasecmp(name, "diafnt16x16b_an"))
            snprintf(s_out, sizeof s_out, "diafnt16x16b_ps");
        else
            return 0;   // already one of the PlayStation atlases
    } else {
        return 0;
    }
    return strcasecmp(s_out, name) ? s_out : 0;
}
#endif

// ---- dialog text size ----------------------------------------------------------------------
// The size a dialog panel's strings are built at. Where they are laid out, qol/k2qolpage.cpp's
// K2QolFitText shrinks each one, if need be, to what its box can draw; there is no fixed cap.

namespace {
int g_dialogScope = 0;   // depth of open scopes (a terminal's SetReplies runs CSWGuiDialog's)
}

extern "C" void k2qol_dialog_text_begin(void) { ++g_dialogScope; }
extern "C" void k2qol_dialog_text_end(void) { --g_dialogScope; }

// The multiplier NewCAurGUIString applies to the string it is building.
extern "C" float k2qol_text_size_factor(void) {
    return g_dialogScope ? static_cast<float>(g_dialogPct) / 100.0f : 1.0f;
}

// rumble.
//
// The engine's whole rumble pipeline runs (PlayRumblePattern -> UpdateRumble once a frame ->
// CExoInput::SetRumble); the mobile build cut its two ends, and K2_QOL_CONTROLLER restores both
// (KOTOR 1's too): rumble.2da is loaded at the end of StartServices (clientexoappinternal.cpp, as
// the Switch does), and CExoInputInternal::SetRumble, a `return 0` stub on Android and macOS, lands
// here.
//
// Where it goes, in order, on SDL2 2.32:
//   Rumble=controller  every opened pad at once (shim_dinput.cpp merges them all into the one the
//                      engine reads): SDL_JoystickRumble on each (the HIDAPI drivers implement it
//                      on every platform; Android's plain joystick backend answers "unsupported",
//                      remembered per pad), else the SDL haptic device carrying that pad's name.
//   Rumble=phone       (Android) only the device's own vibrator, over JNI with
//                      AudioAttributes.USAGE_GAME: SDL's haptic device for it vibrates with no
//                      attributes, which Android files as touch feedback, a setting users turn off.
//                      For a handheld whose built-in controls report as an external pad with no
//                      motor SDL can reach.
// UpdateRumble calls SetRumble every frame (0/0 included) with a "10 minutes" duration, so only a
// change is sent (magnitudes quantised to 1/64) and a running effect is re-armed every ~800 ms for
// 1 s at a time: a stall, a crash or a backgrounded app can never leave a motor running.

namespace {

const uint32_t kPlayMs = 1000;
const uint32_t kRearmMs = 800;
const uint32_t kRescanMs = 3000;

enum Route { ROUTE_NONE = 0, ROUTE_JOYSTICK, ROUTE_HAPTIC, ROUTE_VIBRATOR };
struct Target {
    Route route;
    SDL_JoystickID joyInstance;   // ROUTE_JOYSTICK, by id (never a stale pointer)
    SDL_Haptic* haptic;           // ROUTE_HAPTIC: opened, rumble-initialised
    char name[64];
};
const int kMaxTargets = 8;
Target g_targets[kMaxTargets];
int g_nTargets;
char g_targetNames[160];          // "a", "b" -- for the log
SDL_JoystickID g_noJoyRumble[8];   // pads SDL_JoystickRumble has refused
unsigned g_noJoyRumbleCount;
uint32_t g_nextScan;
int g_lastJoyCount = -1;
int g_lastMode = -1;
float g_lo, g_hi;
uint32_t g_rearmAt;
bool g_sent;

bool joy_rumble_refused(SDL_JoystickID inst) {
    for (unsigned i = 0; i < g_noJoyRumbleCount && i < 8; ++i)
        if (g_noJoyRumble[i] == inst) return true;
    return false;
}

bool haptics_ready() {
    static int s_ok = -1;
    if (s_ok < 0) {
        s_ok = (SDL_WasInit(SDL_INIT_HAPTIC) & SDL_INIT_HAPTIC) || SDL_InitSubSystem(SDL_INIT_HAPTIC) == 0;
        if (!s_ok) LOG("SDL_InitSubSystem(HAPTIC): %s", SDL_GetError());
    }
    return s_ok;
}

#ifdef __ANDROID__
// The device vibrator (see above for why not SDL's haptic device).
struct Vibrator {
    bool tried, ok, amplitude;
    jobject vibrator;       // global ref, android.os.Vibrator
    jobject attrs;          // global ref, AudioAttributes{usage=GAME}
    jclass effectCls;       // global ref, android.os.VibrationEffect
    jmethodID createOneShot, vibrate, cancel;
};
Vibrator g_vib;

JNIEnv* jni() {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    if (env && env->ExceptionCheck()) env->ExceptionClear();
    return env;
}

bool jni_failed(JNIEnv* env) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

bool vibrator_init() {
    if (g_vib.tried) return g_vib.ok;
    g_vib.tried = true;
    JNIEnv* env = jni();
    jobject act = env ? static_cast<jobject>(SDL_AndroidGetActivity()) : 0;
    if (!act) {
        LOG("device vibrator: no JNIEnv / activity");
        return false;
    }
    jclass ctxCls = env->GetObjectClass(act);
    jmethodID gss = env->GetMethodID(ctxCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring svc = env->NewStringUTF("vibrator");
    jobject vib = gss ? env->CallObjectMethod(act, gss, svc) : 0;
    env->DeleteLocalRef(svc);
    env->DeleteLocalRef(ctxCls);
    env->DeleteLocalRef(act);
    if (jni_failed(env) || !vib) {
        LOG("device vibrator: getSystemService(\"vibrator\") failed");
        return false;
    }
    jclass vibCls = env->FindClass("android/os/Vibrator");
    jclass effCls = env->FindClass("android/os/VibrationEffect");   // API 26
    jclass bldCls = env->FindClass("android/media/AudioAttributes$Builder");
    if (jni_failed(env) || !vibCls || !effCls || !bldCls) {
        LOG("device vibrator: VibrationEffect / AudioAttributes unavailable (API < 26?)");
        return false;
    }
    jmethodID hasVib = env->GetMethodID(vibCls, "hasVibrator", "()Z");
    jmethodID hasAmp = env->GetMethodID(vibCls, "hasAmplitudeControl", "()Z");
    g_vib.vibrate = env->GetMethodID(vibCls, "vibrate",
                                     "(Landroid/os/VibrationEffect;Landroid/media/AudioAttributes;)V");
    g_vib.cancel = env->GetMethodID(vibCls, "cancel", "()V");
    g_vib.createOneShot = env->GetStaticMethodID(effCls, "createOneShot", "(JI)Landroid/os/VibrationEffect;");
    jmethodID bldNew = env->GetMethodID(bldCls, "<init>", "()V");
    jmethodID setUsage = env->GetMethodID(bldCls, "setUsage", "(I)Landroid/media/AudioAttributes$Builder;");
    jmethodID build = env->GetMethodID(bldCls, "build", "()Landroid/media/AudioAttributes;");
    if (jni_failed(env) || !hasVib || !hasAmp || !g_vib.vibrate || !g_vib.cancel ||
        !g_vib.createOneShot || !bldNew || !setUsage || !build) {
        LOG("device vibrator: a Vibrator / VibrationEffect method is missing");
        return false;
    }
    if (!env->CallBooleanMethod(vib, hasVib) || jni_failed(env)) {
        LOG("device vibrator: none (hasVibrator() == false)");
        return false;
    }
    g_vib.amplitude = env->CallBooleanMethod(vib, hasAmp) && !jni_failed(env);
    const int kUsageGame = 14;   // AudioAttributes.USAGE_GAME -> the MEDIA vibration intensity
    jobject bld = env->NewObject(bldCls, bldNew);
    jobject bld2 = bld ? env->CallObjectMethod(bld, setUsage, kUsageGame) : 0;
    jobject attrs = bld2 ? env->CallObjectMethod(bld2, build) : 0;
    if (jni_failed(env) || !attrs) {
        LOG("device vibrator: AudioAttributes.Builder failed");
        return false;
    }
    g_vib.vibrator = env->NewGlobalRef(vib);
    g_vib.attrs = env->NewGlobalRef(attrs);
    g_vib.effectCls = static_cast<jclass>(env->NewGlobalRef(effCls));
    env->DeleteLocalRef(attrs);
    if (bld2 != bld) env->DeleteLocalRef(bld2);
    env->DeleteLocalRef(bld);
    env->DeleteLocalRef(vib);
    env->DeleteLocalRef(vibCls);
    env->DeleteLocalRef(effCls);
    env->DeleteLocalRef(bldCls);
    g_vib.ok = true;
    LOG("device vibrator: ready (amplitude control %s, usage GAME)", g_vib.amplitude ? "yes" : "no");
    return true;
}

// strength 0 = stop. False = the call threw.
bool vibrator_play(float strength, uint32_t ms) {
    JNIEnv* env = jni();
    if (!env || !g_vib.ok) return false;
    if (!(strength > 0.0f)) {
        env->CallVoidMethod(g_vib.vibrator, g_vib.cancel);
        return !jni_failed(env);
    }
    int amp = -1;   // VibrationEffect.DEFAULT_AMPLITUDE, for a motor with no amplitude control
    if (g_vib.amplitude) {
        amp = static_cast<int>(strength * 255.0f + 0.5f);
        amp = amp < 1 ? 1 : amp > 255 ? 255 : amp;
    }
    jobject eff = env->CallStaticObjectMethod(g_vib.effectCls, g_vib.createOneShot, static_cast<jlong>(ms), amp);
    if (jni_failed(env) || !eff) return false;
    env->CallVoidMethod(g_vib.vibrator, g_vib.vibrate, eff, g_vib.attrs);
    env->DeleteLocalRef(eff);
    return !jni_failed(env);
}

// SDL only lists hot-plugged haptic devices when asked; the entry point is SDL-internal, so it
// may well not be exported.
void poll_haptics() {
    static void (*s_poll)() = reinterpret_cast<void (*)()>(dlsym(RTLD_DEFAULT, "Android_JNI_PollHapticDevices"));
    if (s_poll) s_poll();
}
#else
bool vibrator_init() { return false; }
bool vibrator_play(float, uint32_t) { return false; }
void poll_haptics() {}
#endif

void close_target() {
    for (int i = 0; i < g_nTargets; ++i) {
        Target& t = g_targets[i];
        if (t.route == ROUTE_HAPTIC && t.haptic) {
            SDL_HapticRumbleStop(t.haptic);
            SDL_HapticClose(t.haptic);
        }
        if (t.route == ROUTE_VIBRATOR) vibrator_play(0.0f, 0);
    }
    memset(g_targets, 0, sizeof g_targets);
    g_nTargets = 0;
    g_targetNames[0] = 0;
}

Target* add_target(Route route, const char* pName) {
    if (g_nTargets >= kMaxTargets) return 0;
    Target& t = g_targets[g_nTargets++];
    t.route = route;
    snprintf(t.name, sizeof t.name, "%s", pName ? pName : "?");
    return &t;
}

bool haptic_taken(int index) {
    const char* n = SDL_HapticName(index);
    for (int i = 0; i < g_nTargets; ++i)
        if (g_targets[i].route == ROUTE_HAPTIC && n && !strcmp(g_targets[i].name, n)) return true;
    return false;
}

bool open_haptic(int index) {
    if (g_nTargets >= kMaxTargets || haptic_taken(index)) return false;
    SDL_Haptic* h = SDL_HapticOpen(index);
    if (!h) return false;
    if (SDL_HapticRumbleSupported(h) != SDL_TRUE || SDL_HapticRumbleInit(h) != 0) {
        SDL_HapticClose(h);
        return false;
    }
    add_target(ROUTE_HAPTIC, SDL_HapticName(index))->haptic = h;
    return true;
}

void choose_target(int mode) {
    close_target();
    int nJoy = SDL_NumJoysticks();
    int nHap = -1;
    for (int i = 0; mode == RUMBLE_CONTROLLER && i < nJoy; ++i) {
        SDL_JoystickID inst = SDL_JoystickGetDeviceInstanceID(i);
        SDL_Joystick* joy = SDL_JoystickFromInstanceID(inst);   // only a pad somebody opened
        if (!joy) continue;
        const char* padName = SDL_JoystickName(joy);
        if (!joy_rumble_refused(inst)) {
            if (Target* t = add_target(ROUTE_JOYSTICK, padName)) t->joyInstance = inst;
            continue;
        }
        if (!padName || !haptics_ready()) continue;
        if (nHap < 0) {
            poll_haptics();
            nHap = SDL_NumHaptics();
        }
        for (int h = 0; h < nHap; ++h) {
            const char* n = SDL_HapticName(h);
            if (n && !strcmp(n, padName) && open_haptic(h)) break;
        }
    }
    // No pad motor by name: a lone pad-side haptic device is the pad's.
    if (g_nTargets == 0 && mode == RUMBLE_CONTROLLER && nJoy > 0 && haptics_ready()) {
        if (nHap < 0) {
            poll_haptics();
            nHap = SDL_NumHaptics();
        }
        int padSide = -1, padSideCount = 0;
        for (int i = 0; i < nHap; ++i) {
            const char* n = SDL_HapticName(i);
            if (!n || !strcmp(n, "VIBRATOR_SERVICE")) continue;   // the device's own: not a pad
            ++padSideCount;
            padSide = i;
        }
        if (padSideCount == 1) open_haptic(padSide);
    }
    if (g_nTargets == 0 && mode == RUMBLE_PHONE && vibrator_init())
        add_target(ROUTE_VIBRATOR, "android.os.Vibrator");
    for (int i = 0; i < g_nTargets; ++i) {
        size_t n = strlen(g_targetNames);
        snprintf(g_targetNames + n, sizeof g_targetNames - n, "%s%s", i ? ", " : "", g_targets[i].name);
    }
    // Logged when the answer changes: with no motor anywhere this runs every kRescanMs.
    static int s_lastJoy = -1;
    static char s_lastNames[sizeof g_targetNames] = "\x01";
    if (nJoy == s_lastJoy && !strcmp(s_lastNames, g_targetNames)) return;
    s_lastJoy = nJoy;
    strcpy(s_lastNames, g_targetNames);
    if (!g_nTargets) {
        LOG("rumble target: none (%d joystick(s), mode %s)", nJoy, kRumbleNames[mode]);
        return;
    }
    for (int i = 0; i < g_nTargets; ++i)
        LOG("rumble target %d/%d: %s\"%s\" (%d joystick(s), mode %s)", i + 1, g_nTargets,
            g_targets[i].route == ROUTE_JOYSTICK   ? "SDL_JoystickRumble on "
            : g_targets[i].route == ROUTE_VIBRATOR ? "the device vibrator "
                                                   : "SDL haptic device ",
            g_targets[i].name, nJoy, kRumbleNames[mode]);
}

// False = the route failed and must be re-chosen.
bool send_one(Target& t, float lo, float hi, uint32_t ms) {
    switch (t.route) {
    case ROUTE_JOYSTICK: {
        SDL_Joystick* joy = SDL_JoystickFromInstanceID(t.joyInstance);
        if (!joy) return false;   // unplugged / closed
        if (SDL_JoystickRumble(joy, static_cast<Uint16>(lo * 65535.0f), static_cast<Uint16>(hi * 65535.0f), ms) == 0)
            return true;
        // Windows: SDL rumbles an Xbox pad through XInput, and until a press tells it which XInput
        // slot the pad is ("Controller isn't correlated yet") it refuses. That is not the pad
        // lacking a motor: keep the target and let a later rumble reach it.
        if (strstr(SDL_GetError(), "correlated")) return true;
        LOG("SDL_JoystickRumble(\"%s\") refused: %s; trying a haptic device", t.name,
            SDL_GetError());
        if (g_noJoyRumbleCount < 8) g_noJoyRumble[g_noJoyRumbleCount++] = t.joyInstance;
        return false;
    }
    case ROUTE_HAPTIC: {
        float s = lo > hi ? lo : hi;
        int rc = s > 0.0f ? SDL_HapticRumblePlay(t.haptic, s, ms)
                          : SDL_HapticRumbleStop(t.haptic);
        if (rc == 0) return true;
        LOG("haptic \"%s\" failed: %s", t.name, SDL_GetError());
        return false;
    }
    case ROUTE_VIBRATOR:
        if (vibrator_play(lo > hi ? lo : hi, ms)) return true;
        LOG("device vibrator: vibrate() threw");
        return false;
    default:
        return false;
    }
}

// Every target; false = one failed and the set must be re-chosen.
bool send(float lo, float hi, uint32_t ms) {
    bool ok = g_nTargets > 0;
    for (int i = 0; i < g_nTargets; ++i)
        if (!send_one(g_targets[i], lo, hi, ms)) ok = false;
    return ok;
}

float unit(float f) {
    if (!(f > 0.0f)) return 0.0f;   // NaN too
    if (f > 1.0f) f = 1.0f;
    return static_cast<float>(static_cast<int>(f * 64.0f + 0.5f)) / 64.0f;
}

}  // namespace

extern "C" int k2qol_have_device_vibrator(void) {
#ifdef __ANDROID__
    return 1;
#else
    return 0;
#endif
}

// CExoInputInternal::SetRumble(nPort 0, fLow, fHigh, nMs), every frame.
extern "C" void k2qol_set_rumble_motors(float fLow, float fHigh, unsigned int nMs) {
    int mode = g_rumble;
    float lo = mode ? unit(fLow) : 0.0f;
    float hi = mode ? unit(fHigh) : 0.0f;
    bool on = lo > 0.0f || hi > 0.0f;
    uint32_t now = SDL_GetTicks();

    // Logs one line per rumble the engine requests, motor or not, to tell "the game never asked"
    // from "the request went nowhere".
    static bool s_reqOn;
    static uint32_t s_reqStart;
    if (on != s_reqOn) {
        s_reqOn = on;
        if (on) {
            s_reqStart = now;
            LOG("engine rumble START low %.2f high %.2f -> %s", lo, hi,
                g_nTargets ? g_targetNames : "(no motor yet)");
        } else {
            LOG("engine rumble STOP after %u ms", now - s_reqStart);
        }
    }

    int nJoy = SDL_NumJoysticks();
    if (mode != g_lastMode || nJoy != g_lastJoyCount || (on && !g_nTargets && now >= g_nextScan)) {
        if (g_sent && g_nTargets) send(0.0f, 0.0f, 0);   // stop what the old routes play
        g_sent = false;
        g_lastMode = mode;
        g_lastJoyCount = nJoy;
        g_nextScan = now + kRescanMs;
        if (mode) choose_target(mode);
        else close_target();
    }
    if (!g_nTargets) return;

    bool changed = lo != g_lo || hi != g_hi;
    if (!changed && !(on && now >= g_rearmAt)) return;
    if (!on && !g_sent) { g_lo = lo; g_hi = hi; return; }   // nothing running, nothing to stop
    uint32_t ms = nMs == 0 || nMs > kPlayMs ? kPlayMs : nMs;
    if (!send(lo, hi, ms)) {
        close_target();
        g_nextScan = 0;       // re-choose on the next frame that wants the motor
        g_lastJoyCount = -1;
        g_sent = false;
        return;
    }
    g_lo = lo;
    g_hi = hi;
    g_sent = on;
    g_rearmAt = now + (ms > kRearmMs ? ms - (kPlayMs - kRearmMs) : ms / 2);
}

// A 0.5 s test pulse on the motor the current mode drives: the page calls it each time the Rumble
// row changes, so the player feels which device the choice reaches. It records the mode, so the
// per-frame path does not see a "mode change" next frame and cancel the pulse.
extern "C" void k2qol_test_rumble(void) {
    const float kTestStrength = 0.75f;
    const uint32_t kTestMs = 500;
    int mode = g_rumble;
    if (g_sent && g_nTargets) send(0.0f, 0.0f, 0);
    g_sent = false;
    g_lo = g_hi = 0.0f;
    g_lastMode = mode;
    g_lastJoyCount = SDL_NumJoysticks();
    g_nextScan = SDL_GetTicks() + kRescanMs;
    if (!mode) {
        close_target();
        return;
    }
    for (int attempt = 0; attempt < 4; ++attempt) {
        choose_target(mode);
        if (!g_nTargets) break;
        if (send(kTestStrength, kTestStrength, kTestMs)) {
            LOG("test rumble: %u ms on %s", kTestMs, g_targetNames);
            return;
        }
        close_target();
    }
    LOG("test rumble: no motor to test for mode %s", kRumbleNames[mode]);
}

#endif  // K2_QOL_CONTROLLER
