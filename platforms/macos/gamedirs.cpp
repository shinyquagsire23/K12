// gamedirs.cpp -- where the desktop builds find the game: its files (chitin.key, data/, modules/ ...)
// and the player's (saves/, swkotor.ini), which an installed copy keeps apart.
//
// In order, the first that answers:
//   K2_DATA_DIR=<dir>    one folder for both;
//   K2_ASSET_DIR=<dir>   the game's files only (a folder with chitin.key); the player's are found as
//                        for an install below, unless K2_USER_DIR names them;
//   the OBBs (K2_OBB_DIR, the older K2_OBB and K1_OBB_DIR): the working directory is the data root;
//   an installed copy, from a fixed list of places and never a search of the disk:
//     * every Steam library: each platform's Steam roots and the libraries their
//       libraryfolders.vdf names, with steamapps/common/<swkotor | Knights of the Old Republic II>
//       and the data folder inside it -- the folder itself, steamassets/ (KOTOR II on Linux) or
//       <app>.app/Contents/Assets or GameData (macOS);
//     * GOG Galaxy's registry entry for the game (Windows);
//     * the disc and store installs' default folders (LucasArts, GOG, Amazon) under Program Files
//       and C:\, and the same seen from Linux and macOS through WSL's /mnt/c and the default Wine
//       prefix, plus Heroic's GOG folder (Linux);
//     * the game's app bundles in /Applications and ~/Applications (macOS), by name;
//     * the Windows build under Wine: the host's Steam installs through Z:, which Wine maps to the
//       host's /, after every Windows place;
//   the working directory, then the executable's own folder (and, in a macOS app bundle, the folder
//   the bundle is in), when it holds chitin.key or the game's OBBs.
//
// A folder counts as the game when it has chitin.key and the voice-over folder only that game has
// (KOTOR 1 streamwaves/, KOTOR II streamvoice/), so neither picks up the other's install.
//
// KOTOR II's Steam Workshop items (TSLRCM and the like) are found beside a Steam install: each
// folder of <library>/steamapps/workshop/content/208580 is laid out as the game's folder is
// (override/, modules/, dialog.tlk, streamvoice/ ...), the shape of the mod folder the engine
// searches ahead of the game (dlc/mods_<language>). main.cpp mounts each there read-only
// (shim/shim_path.cpp), in item-id order. K2_WORKSHOP=0 leaves them out.
//
// A Steam install that keeps a cloudsaves/<account>/ folder beside the game (KOTOR II's Steam Cloud)
// has the save folders there, and that is what Steam syncs, so saves/ is redirected to it for reads
// and writes alike (shim/shim_path.cpp). The account is the one cloudsaves/steam_autocloud.vdf
// names, else the only or newest numeric folder. K2_USER_DIR, or K2_CLOUDSAVES=0, keeps saves/ in
// the player's folder.
//
// The player's folder (K2_USER_DIR overrides it) is the one the installed copy itself uses, so both
// share saves: macOS, ~/Library/Application Support/<the game's name>; KOTOR II on Linux,
// aspyr-media/kotor2 under the data home of the Steam that installed it; the Windows build, its own
// install folder, or the copy Windows' file virtualization made of it in
// %LOCALAPPDATA%\VirtualStore, or, for a host path reached through Wine's Z:, %APPDATA%\<the game's
// name> in the Wine prefix. When that is not the game's folder, the shim reads the game's files from
// the install and writes only to the player's folder.
//
// The OBBs are the Android build's expansion files (main.<v>.<package>.obb and
// patch.<v>.<package>.obb), mounted read-only under the working directory, as on the device
// (shim/shim_obb.cpp; KOTOR 1: k1/k1port.cpp). The variables are the same on every platform and for
// both games:
//   K2_OBB_DIR=<dir>     the folder they are in; the game's own are picked by package name
//                        (com.aspyr.swkotorii / com.aspyr.swkotor), the highest version of each kind
//                        (shim/obbfiles.h), so one folder can hold both games';
//   K2_OBB=<f>;<f>       the older spelling, the files themselves, main first (the later mount wins);
//                        ';' between them on every platform, and ':' too except on Windows, where a
//                        path has one of its own;
//   K1_OBB_DIR=<dir>     KOTOR 1's older name for K2_OBB_DIR.
// With none set, a game folder found below that holds the game's OBBs is read through them. A
// variable that names no OBBs is an error (the message box), not a fall through to the search.
//
// The Windows branches (platforms/windows) use only Win32 and the C runtime, not the POSIX shim.
#include "gamedirs.h"
#include "../shim/obbfiles.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <strings.h>
#include <unistd.h>
#include "../shim/k2shim.h"
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace {

// Platform: existence, listing, absolute paths
std::string Env(const char* pszName)
{
    const char* p = getenv(pszName);
    return p && *p ? p : "";
}

#if defined(_WIN32)
std::string Home() { return Env("USERPROFILE"); }
int StrCaseCmp(const char* a, const char* b) { return strcasecmp(a, b); }

// Windows' file system is already case-blind: the path as written, when it exists.
std::string OnDisk(const std::string& path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES ? path : "";
}

bool IsDir(const std::string& path)
{
    DWORD a = path.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

std::string Absolute(const std::string& path)
{
    char buf[MAX_PATH];
    return _fullpath(buf, path.c_str(), sizeof buf) ? buf : path;
}

void ListDir(const std::string& dir, std::vector<std::string>& names)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "/*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..")) names.push_back(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

std::string ExeDir()
{
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(0, buf, sizeof buf);
    if (n == 0 || n >= sizeof buf) return "";
    std::string p(buf, n);
    size_t slash = p.find_last_of("\\/");
    return slash == std::string::npos ? "" : p.substr(0, slash);
}

std::string CurrentDir()
{
    char buf[MAX_PATH];
    return _getcwd(buf, sizeof buf) ? buf : ".";
}

// A string value from the registry (either view of HKLM, or HKCU), or "".
std::string RegString(HKEY hRoot, const char* pszKey, const char* pszValue)
{
    char buf[MAX_PATH];
    DWORD n = sizeof buf;
    for (DWORD view : { static_cast<DWORD>(RRF_SUBKEY_WOW6432KEY), static_cast<DWORD>(RRF_SUBKEY_WOW6464KEY) }) {
        n = sizeof buf;
        if (RegGetValueA(hRoot, pszKey, pszValue, RRF_RT_REG_SZ | view, 0, buf, &n) == ERROR_SUCCESS)
            return buf;
    }
    return "";
}

// Run under Wine (CrossOver, Proton): ntdll exports wine_get_version, which Windows' does not.
bool UnderWine()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    return ntdll && GetProcAddress(ntdll, "wine_get_version");
}
#else
std::string Home() { return Env("HOME"); }
int StrCaseCmp(const char* a, const char* b) { return strcasecmp(a, b); }

// The path's spelling on disk (each component matched case-blind), or "" when it is not there.
std::string OnDisk(const std::string& path)
{
    char buf[1024];
    k2shim_host_path(path.c_str(), buf, sizeof buf);
    struct stat st;
    return stat(buf, &st) == 0 ? buf : "";
}

bool IsDir(const std::string& path)
{
    struct stat st;
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::string Absolute(const std::string& path)
{
    char buf[PATH_MAX];
    return realpath(path.c_str(), buf) ? buf : path;
}

void ListDir(const std::string& dir, std::vector<std::string>& names)
{
    k2shim_list_dir(dir.c_str(), names);
}

std::string ExeDir()
{
    std::string exe;
#if defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t n = sizeof buf;
    if (_NSGetExecutablePath(buf, &n) == 0) exe = Absolute(buf);
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n > 0) exe.assign(buf, static_cast<size_t>(n));
#endif
    size_t slash = exe.find_last_of('/');
    return slash == std::string::npos ? "" : exe.substr(0, slash);
}

std::string CurrentDir()
{
    char buf[PATH_MAX];
    return getcwd(buf, sizeof buf) ? buf : ".";
}
#endif

bool EndsWithNoCase(const std::string& s, const char* pszTail)
{
    size_t n = strlen(pszTail);
    return s.size() >= n && StrCaseCmp(s.c_str() + s.size() - n, pszTail) == 0;
}

// the places
bool IsGame(const std::string& dir, bool bKotor1)
{
    return !OnDisk(dir + "/chitin.key").empty() && IsDir(OnDisk(dir + (bKotor1 ? "/streamwaves" : "/streamvoice")));
}

// Returns a folder's OBBs of the game, absolute, main first, or none unless it has the main one
// (KOTOR 1, which waits for both mounts: and the patch).
std::vector<std::string> ObbsIn(const std::string& dir, bool bKotor1)
{
    std::vector<std::string> names, files;
    ListDir(dir, names);
    K2ObbNames obbs = k2_pick_obbs(names, bKotor1);
    if (obbs.main.empty() || (bKotor1 && obbs.patch.empty())) return files;
    files.push_back(Absolute(dir + "/" + obbs.main));
    if (!obbs.patch.empty()) files.push_back(Absolute(dir + "/" + obbs.patch));
    return files;
}

// The variable that names the OBBs (the header), or 0.
const char* ObbVariable(bool bKotor1)
{
    if (!Env("K2_OBB_DIR").empty()) return "K2_OBB_DIR";
    if (!Env("K2_OBB").empty()) return "K2_OBB";
    if (bKotor1 && !Env("K1_OBB_DIR").empty()) return "K1_OBB_DIR";
    return 0;
}

// The OBBs the variable names, or false with the player's message.
bool NamedObbs(const char* pszVar, bool bKotor1, std::vector<std::string>& files, std::string& error)
{
    std::string value = Env(pszVar);
    if (strcmp(pszVar, "K2_OBB") != 0) {
        files = ObbsIn(value, bKotor1);
        const std::string pkg = k2_obb_package(bKotor1);
        if (files.empty())
            error = std::string(pszVar) + "=" + value + " has none of the game's OBBs: it should name the folder "
                    "with main.<version>." + pkg + ".obb" +
                    (bKotor1 ? " and patch.<version>." + pkg + ".obb." : " (and patch.<version>." + pkg + ".obb).");
        return !files.empty();
    }
#ifdef _WIN32
    const char* pszSeps = ";";
#else
    const char* pszSeps = ";:";
#endif
    for (size_t at = 0; at <= value.size();) {
        size_t end = value.find_first_of(pszSeps, at);
        if (end == std::string::npos) end = value.size();
        std::string file = value.substr(at, end - at);
        if (!file.empty()) {
            if (OnDisk(file).empty()) {
                error = "K2_OBB names " + file + ", which is not there.";
                return false;
            }
            files.push_back(Absolute(OnDisk(file)));
        }
        at = end + 1;
    }
    if (bKotor1 && files.size() != 2) {
        error = "K2_OBB should name KOTOR 1's main OBB, then its patch OBB (or use K2_OBB_DIR=<folder>).";
        return false;
    }
    return !files.empty();
}

const char* GameName(bool bKotor1)
{
    return bKotor1 ? "Star Wars: Knights of the Old Republic" : "Star Wars: Knights of the Old Republic II";
}

// One place to look, and where the copy found there keeps the player's files.
struct Candidate {
    std::string dir;
    std::vector<std::string> dataHomes;   // Linux: the data homes of the Steam that installed it
};

struct Steam {
    std::string root;
    std::vector<std::string> dataHomes;
    std::string libPrefix;   // put before a library path the vdf names: "Z:" for a Wine host's Steam
};

// The "path" values of a libraryfolders.vdf (backslashes arrive doubled).
std::vector<std::string> SteamLibraries(const std::string& root)
{
    std::vector<std::string> out;
    for (const char* rel : { "/steamapps/libraryfolders.vdf", "/config/libraryfolders.vdf" }) {
        FILE* f = fopen((root + rel).c_str(), "rb");
        if (!f) continue;
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            char* p = strstr(line, "\"path\"");
            if (!p) continue;
            p = strchr(p + 6, '"');
            if (!p) continue;
            std::string v;
            for (++p; *p && *p != '"'; ++p) {
                if (*p == '\\' && p[1]) ++p;
                v += *p;
            }
            if (!v.empty()) out.push_back(v);
        }
        fclose(f);
    }
    return out;
}

// Returns the host's Steam roots as the Windows build sees them under Wine through Z: (the host's
// /), the macOS client's for each Z:/Users/<name> and the Linux clients' for each Z:/home/<name>.
std::vector<Steam> WineHostSteamRoots()
{
    std::vector<Steam> roots;
#if defined(_WIN32)
    if (!UnderWine() || !IsDir("Z:/")) return roots;
    std::vector<std::string> users;
    ListDir("Z:/Users", users);
    std::sort(users.begin(), users.end());
    for (const std::string& u : users) {
        std::string home = "Z:/Users/" + u;
        if (u[0] == '.' || !IsDir(home + "/Library")) continue;
        roots.push_back({ home + "/Library/Application Support/Steam", {}, "Z:" });
        roots.push_back({ home + "/Library/Applications/Steam", {}, "Z:" });
    }
    users.clear();
    ListDir("Z:/home", users);
    std::sort(users.begin(), users.end());
    for (const std::string& u : users) {
        std::string home = "Z:/home/" + u;
        if (u[0] == '.' || !IsDir(home)) continue;
        for (const char* rel : { "/.local/share/Steam", "/.steam/steam", "/.steam/root", "/.steam/debian-installation",
                                 "/snap/steam/common/.local/share/Steam",
                                 "/.var/app/com.valvesoftware.Steam/.local/share/Steam",
                                 "/.var/app/com.valvesoftware.Steam/data/Steam" })
            roots.push_back({ home + rel, {}, "Z:" });
    }
#endif
    return roots;
}

std::vector<Steam> SteamRoots()
{
    std::vector<Steam> roots;
#if defined(_WIN32)
    std::string reg = RegString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath");
    if (!reg.empty()) roots.push_back({ reg, {} });
    reg = RegString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", "InstallPath");
    if (!reg.empty()) roots.push_back({ reg, {} });
    for (const char* var : { "ProgramFiles(x86)", "ProgramFiles" })
        if (!Env(var).empty()) roots.push_back({ Env(var) + "/Steam", {} });
#elif defined(__APPLE__)
    roots.push_back({ Home() + "/Library/Application Support/Steam", {} });
    roots.push_back({ Home() + "/Library/Applications/Steam", {} });
#else
    std::string dataHome = !Env("XDG_DATA_HOME").empty() ? Env("XDG_DATA_HOME") : Home() + "/.local/share";
    std::vector<std::string> native = { dataHome, Home() + "/.local/share" };
    roots.push_back({ dataHome + "/Steam", native });
    roots.push_back({ Home() + "/.local/share/Steam", native });
    roots.push_back({ Home() + "/.steam/steam", native });
    roots.push_back({ Home() + "/.steam/root", native });
    roots.push_back({ Home() + "/.steam/debian-installation", native });
    // snap and flatpak keep their own home: the game's files under it, and so its aspyr-media.
    std::string snap = Home() + "/snap/steam/common";
    roots.push_back({ snap + "/.local/share/Steam", { snap + "/.local/share" } });
    std::string flat = Home() + "/.var/app/com.valvesoftware.Steam";
    roots.push_back({ flat + "/.local/share/Steam", { flat + "/data", flat + "/.local/share" } });
    roots.push_back({ flat + "/data/Steam", { flat + "/data", flat + "/.local/share" } });
#endif
    return roots;
}

// The game's folder names under a Windows-style root (C:\, WSL's /mnt/c, a Wine drive_c).
std::vector<std::string> DiscFolders(bool bKotor1)
{
    if (bKotor1)
        return { "Program Files/LucasArts/SWKotOR", "Program Files (x86)/LucasArts/SWKotOR",
                 "Program Files/LucasArts/Star Wars Knights of the Old Republic",
                 "Program Files (x86)/LucasArts/Star Wars Knights of the Old Republic",
                 "GOG Games/Star Wars - KotOR", "Program Files (x86)/GOG Galaxy/Games/Star Wars - KotOR",
                 "Amazon Games/Library/Star Wars - Knights of the Old" };
    return { "Program Files/LucasArts/SWKotOR2", "Program Files (x86)/LucasArts/SWKotOR2",
             "Program Files/LucasArts/Star Wars Knights of the Old Republic II",
             "Program Files (x86)/LucasArts/Star Wars Knights of the Old Republic II",
             "GOG Games/Star Wars - KotOR2", "Program Files (x86)/GOG Galaxy/Games/Star Wars - KotOR2" };
}

// Adds the data folders an install folder may hold: itself, KOTOR II's Linux steamassets/, and each
// app bundle's Contents/Assets or GameData.
void AddInstall(std::vector<Candidate>& out, const std::string& dir, const std::vector<std::string>& homes)
{
    out.push_back({ dir, homes });
    out.push_back({ dir + "/steamassets", homes });
    std::string host = OnDisk(dir);
    if (host.empty()) return;
    std::vector<std::string> names;
    ListDir(host, names);
    for (const std::string& n : names)
        if (EndsWithNoCase(n, ".app")) {
            out.push_back({ dir + "/" + n + "/Contents/Assets", homes });
            out.push_back({ dir + "/" + n + "/Contents/GameData", homes });
        }
}

// Every installed copy's data folder to try, in order. `searched` gets every place and `brief` the
// short form for the message box (the Steam libraries, then one line for the fixed folders).
std::vector<Candidate> Candidates(bool bKotor1, std::vector<std::string>& searched, std::vector<std::string>& brief)
{
    std::vector<Candidate> out;
    const char* steamName = bKotor1 ? "swkotor" : "Knights of the Old Republic II";
    std::set<std::string> seen;
    auto addSteam = [&](const std::vector<Steam>& roots) {
        for (const Steam& s : roots) {
            if (!seen.insert(Absolute(s.root)).second) continue;
            searched.push_back(s.root + "/steamapps/common/" + steamName);
            if (!IsDir(s.root)) continue;
            brief.push_back(s.root + "/steamapps/common/" + steamName);
            AddInstall(out, s.root + "/steamapps/common/" + steamName, s.dataHomes);
            for (std::string l : SteamLibraries(s.root)) {
                if (!s.libPrefix.empty() && l[0] == '/') l = s.libPrefix + l;
                if (!IsDir(l) || !seen.insert(Absolute(l)).second) continue;
                searched.push_back(l + "/steamapps/common/" + steamName);
                brief.push_back(l + "/steamapps/common/" + steamName);
                AddInstall(out, l + "/steamapps/common/" + steamName, s.dataHomes);
            }
        }
    };
    addSteam(SteamRoots());
#if defined(_WIN32)
    // GOG Galaxy's game ids: 1207666283 KOTOR, 1421404581 KOTOR II.
    std::string gog = RegString(HKEY_LOCAL_MACHINE, bKotor1 ? "SOFTWARE\\GOG.com\\Games\\1207666283"
                                                            : "SOFTWARE\\GOG.com\\Games\\1421404581", "path");
    if (!gog.empty()) {
        searched.push_back(gog);
        brief.push_back(gog);
        AddInstall(out, gog, {});
    }
#endif
    if (brief.empty()) brief.push_back("Steam (none installed)");
    std::vector<std::string> drives;
#if defined(_WIN32)
    drives.push_back(!Env("SystemDrive").empty() ? Env("SystemDrive") : "C:");
#else
#if !defined(__APPLE__)
    drives.push_back("/mnt/c");                        // WSL
#endif
    drives.push_back(Home() + "/.wine/drive_c");       // the default Wine prefix
#endif
    std::vector<std::string> discs;
    for (const std::string& d : drives)
        for (const std::string& f : DiscFolders(bKotor1)) discs.push_back(d + "/" + f);
#if !defined(_WIN32) && !defined(__APPLE__)
    discs.push_back(Home() + (bKotor1 ? "/Games/Heroic/Star Wars - KotOR" : "/Games/Heroic/Star Wars - KotOR2"));
#endif
    for (const std::string& d : discs) {
        searched.push_back(d);
        AddInstall(out, d, {});
    }
#if defined(_WIN32)
    brief.push_back("the LucasArts, GOG and Amazon folders on " + drives[0]);
#elif defined(__APPLE__)
    brief.push_back("the LucasArts, GOG and Amazon folders in ~/.wine");
#else
    brief.push_back("the LucasArts, GOG and Amazon folders in ~/.wine, WSL's /mnt/c and ~/Games/Heroic");
#endif
#if defined(__APPLE__)
    // The bundles are told apart by name. KOTOR's Mac App Store build keeps the data in
    // Contents/Assets, KOTOR II's in Contents/GameData, and TransGaming's KOTOR II a Windows
    // install inside its own C: drive.
    brief.push_back("the game's app in /Applications and ~/Applications");
    for (const std::string& apps : { std::string("/Applications"), Home() + "/Applications" }) {
        searched.push_back(apps + "/<the game>.app");
        std::vector<std::string> names;
        ListDir(apps, names);
        for (const std::string& n : names) {
            std::string lower(n);
            for (char& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (!EndsWithNoCase(n, ".app")) continue;
            if (lower.find("knights") == std::string::npos && lower.find("kotor") == std::string::npos) continue;
            std::string app = apps + "/" + n + "/Contents";
            out.push_back({ app + "/Assets", {} });
            out.push_back({ app + "/GameData", {} });
            out.push_back({ app + (bKotor1 ? "/Resources/transgaming/c_drive/Program Files/SWKotOR"
                                           : "/Resources/transgaming/c_drive/Program Files/SWKotOR2"), {} });
        }
    }
#endif
    // Last, under Wine: the host's own Steam installs, through Z:.
    std::vector<Steam> host = WineHostSteamRoots();
    size_t nBrief = brief.size();
    addSteam(host);
    if (brief.size() == nBrief && !host.empty()) brief.push_back("the host's Steam through Z: (none installed)");
    return out;
}

// Where the copy whose data folder is `dir` keeps the player's files.
std::string UserDirFor(const std::string& dir, const std::vector<std::string>& dataHomes, bool bKotor1)
{
#if defined(_WIN32)
    // A disc install under Program Files that the game wrote to as a standard user: Windows moved
    // those writes to %LOCALAPPDATA%\VirtualStore\<the path without its drive>.
    std::string local = Env("LOCALAPPDATA");
    if (!local.empty() && dir.size() > 3 && dir[1] == ':') {
        std::string store = local + "\\VirtualStore" + dir.substr(2);
        if (IsDir(store + "/saves")) return store;
    }
    // A host's macOS app bundle or KOTOR II's Linux steamassets/, seen through Wine's Z:: not the
    // bundle, and not the host build's own player folder either, but the Wine prefix's %APPDATA%.
    std::string fwd(dir);
    for (char& c : fwd)
        if (c == '\\') c = '/';
    if (!Env("APPDATA").empty() && fwd.size() > 2 && (fwd[0] == 'Z' || fwd[0] == 'z') && fwd[1] == ':' &&
        (fwd.find(".app/Contents/") != std::string::npos || EndsWithNoCase(fwd, "/steamassets")))
        return Env("APPDATA") + "/" + (bKotor1 ? "Knights of the Old Republic" : "Star Wars Knights of the Old Republic II");
    (void)dataHomes;
    return dir;
#else
    if (dir.find("/transgaming/") == std::string::npos && dir.find(".app/Contents/") != std::string::npos)
        return Home() + "/Library/Application Support/" +
               (bKotor1 ? "Knights of the Old Republic" : "Star Wars Knights of the Old Republic II");
    if (!bKotor1 && EndsWithNoCase(dir, "/steamassets")) {
        std::vector<std::string> homes = dataHomes;
        if (homes.empty()) homes.push_back(!Env("XDG_DATA_HOME").empty() ? Env("XDG_DATA_HOME") : Home() + "/.local/share");
        for (const std::string& h : homes)
            if (IsDir(h + "/aspyr-media/kotor2")) return h + "/aspyr-media/kotor2";
        return homes[0] + "/aspyr-media/kotor2";
    }
    return dir;
#endif
}

// The executable's folder, and, inside a macOS bundle (<x>.app/Contents/MacOS), the bundle's folder.
std::vector<std::string> ExeDirs()
{
    std::vector<std::string> dirs;
    std::string exe = ExeDir();
    if (exe.empty()) return dirs;
    dirs.push_back(exe);
    if (EndsWithNoCase(exe, ".app/Contents/MacOS")) {
        std::string app = exe.substr(0, exe.size() - strlen("/Contents/MacOS"));
        size_t slash = app.find_last_of('/');
        if (slash != std::string::npos) dirs.push_back(app.substr(0, slash));
    }
    return dirs;
}

// KOTOR II's Workshop items for the install whose data folder is `dir`: the item folders of the Steam
// library it is in, by id.
std::vector<std::string> WorkshopItems(std::string dir, bool bKotor1)
{
    std::vector<std::string> items;
    if (bKotor1 || Env("K2_WORKSHOP") == "0") return items;
    for (char& c : dir)
        if (c == '\\') c = '/';
    std::string lower(dir);
    for (char& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    size_t cut = lower.find("/steamapps/common/");
    if (cut == std::string::npos) return items;
    std::string root = dir.substr(0, cut) + "/steamapps/workshop/content/208580";
    std::vector<std::string> names;
    ListDir(root, names);
    std::vector<unsigned long long> ids;
    for (const std::string& n : names)
        if (!n.empty() && n.find_first_not_of("0123456789") == std::string::npos && IsDir(root + "/" + n))
            ids.push_back(strtoull(n.c_str(), 0, 10));
    std::sort(ids.begin(), ids.end());
    for (unsigned long long id : ids) items.push_back(root + "/" + std::to_string(id));
    return items;
}

// The Steam Cloud save folder of the install whose data folder is `dir`, or "".
std::string CloudSaves(std::string dir)
{
    if (Env("K2_CLOUDSAVES") == "0") return "";
    for (char& c : dir)
        if (c == '\\') c = '/';
    std::string lower(dir);
    for (char& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    size_t cut = lower.find("/steamapps/common/");
    if (cut == std::string::npos) return "";
    size_t end = dir.find('/', cut + 18);
    std::string cloud = OnDisk(dir.substr(0, end) + "/cloudsaves");
    if (cloud.empty() || !IsDir(cloud)) return "";
    std::string account;
    if (FILE* f = fopen((cloud + "/steam_autocloud.vdf").c_str(), "rb")) {
        char line[512];
        while (fgets(line, sizeof line, f))
            if (char* p = strstr(line, "\"accountid\"")) {
                for (p += 11; *p && !isdigit(static_cast<unsigned char>(*p)); ++p) {}
                while (isdigit(static_cast<unsigned char>(*p))) account += *p++;
                break;
            }
        fclose(f);
    }
    if (!account.empty() && IsDir(cloud + "/" + account)) return cloud + "/" + account;
    std::vector<std::string> names;
    ListDir(cloud, names);
    std::string best;
    long long bestTime = -1;
    for (const std::string& n : names) {
        if (n.empty() || n.find_first_not_of("0123456789") != std::string::npos) continue;
        struct stat st;
        if (stat((cloud + "/" + n).c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (static_cast<long long>(st.st_mtime) > bestTime) {
            bestTime = static_cast<long long>(st.st_mtime);
            best = cloud + "/" + n;
        }
    }
    return best;
}

} // namespace

bool k2_find_game_dirs(bool bKotor1, K2GameDirs& out, std::string& error)
{
    std::string user = Env("K2_USER_DIR");
    std::string here = CurrentDir();
    auto done = [&](const std::string& assets, const std::string& defUser, const char* how) {
        out.assets = Absolute(assets);
        out.user = user.empty() ? defUser : user;
        if (IsDir(out.user)) out.user = Absolute(out.user);
        out.how = how;
        out.mods = WorkshopItems(out.assets, bKotor1);
        if (user.empty() && strcmp(how, "K2_DATA_DIR") != 0) out.saves = CloudSaves(out.assets);
        return true;
    };

    // The OBBs a variable names are mounted whichever folder is the data root (K2_DATA_DIR with them:
    // the writable folder the ini and saves go to).
    const char* pszObbVar = ObbVariable(bKotor1);
    if (pszObbVar && !NamedObbs(pszObbVar, bKotor1, out.obbs, error)) return false;

    if (!Env("K2_DATA_DIR").empty()) return done(Env("K2_DATA_DIR"), Env("K2_DATA_DIR"), "K2_DATA_DIR");
    if (!Env("K2_ASSET_DIR").empty()) {
        std::string dir = Env("K2_ASSET_DIR");
        if (OnDisk(dir + "/chitin.key").empty()) {
            error = "K2_ASSET_DIR=" + dir + " has no chitin.key: it should name the folder of the game's "
                    "files (the one with chitin.key, data/ and modules/).";
            return false;
        }
        return done(dir, UserDirFor(Absolute(dir), {}, bKotor1), "K2_ASSET_DIR");
    }
    if (pszObbVar) return done(here, here, pszObbVar);

    std::vector<std::string> searched, brief;
    for (const Candidate& c : Candidates(bKotor1, searched, brief)) {
        if (!IsGame(c.dir, bKotor1)) continue;
        std::string dir = OnDisk(c.dir);
        return done(dir, UserDirFor(Absolute(dir), c.dataHomes, bKotor1), "an installed copy");
    }
    std::vector<std::pair<std::string, const char*>> local = { { here, "the working directory" } };
    for (const std::string& d : ExeDirs()) local.push_back({ d, "the executable's folder" });
    for (const auto& l : local) {
        out.obbs = ObbsIn(l.first, bKotor1);
        if (!OnDisk(l.first + "/chitin.key").empty() || !out.obbs.empty())
            return done(l.first, l.first, l.second);
        searched.push_back(l.first + " (" + l.second + ")");
        brief.push_back(l.first + " (" + l.second + ")");
    }

    fprintf(stderr, "k2: no game files in any of:\n");
    for (const std::string& where : searched) fprintf(stderr, "  %s\n", where.c_str());
    error = std::string(GameName(bKotor1)) + ": the game's files were not found.\n\n"
            "Install the game (Steam, GOG or disc), or set K2_ASSET_DIR to the folder that holds its "
            "chitin.key (K2_USER_DIR: where saves go), or K2_DATA_DIR to one folder for both, or "
            "K2_OBB_DIR to the folder with the Android version's OBBs, or run from the game's folder."
            "\n\nLooked in:";
    for (const std::string& where : brief) error += "\n" + where;
    return false;
}
