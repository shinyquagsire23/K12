// Delivers achievements on the host builds. Achievements::UnlockAchievement
// (src/port/android/AndroidJNI.cpp; KOTOR 1's free UnlockAchievement in
// src/game/swmain/Achievements.cpp) sends its tag here under K2_HOST_BUILD. The engine decides
// when; this decides where to.
//
// KOTOR II's PC builds (Steam app 208580) report "achievement_<n>" through Steamworks:
// ISteamUserStats::SetAchievement, then StoreStats. Here, with no Steamworks SDK, in this order:
//
//   1. steam_api    a steam_api library of this process's architecture next to the executable or in
//                   the game's folder (steam_api64.dll / libsteam_api.dylib / libsteam_api.so), by
//                   its flat C exports, whichever of the SDK generations' names it has.
//   2. steamclient  the running Steam client's own library, which steam_api is a front for:
//                   CreateInterface("SteamClient017"), a pipe and the global user, then its
//                   ISteamUserStats by virtual call. The macOS client's steamclient.dylib is
//                   a universal build; the game's own libsteam_api.dylib is not.
//   3. local        achievements.ini in the player's folder (the shim's fopen puts it there) and a
//                   notice in the game (compat/achievement_toast.cpp: a bark bubble and a message-log
//                   line).
//
// Steam is used only for KOTOR II and only with its app id: K2_STEAM_APPID, else Steam's launch
// environment (SteamAppId / SteamGameId), else a steam_appid.txt next to the executable or in the
// game's folder. K2_STEAM_APPID=0 forces the local backend. KOTOR 1 (app 32370) has no Steam
// achievements and is always local. Whatever the backend, an unlock is also recorded locally; the
// notice is shown only by the local backend (Steam shows its own).
//   K2_STEAM_READONLY=1  connect and read, but never set or store an achievement (testing).
// KOTOR 1's Other Settings page can turn its achievements off (qol/k2qol.cpp `Achievements=0`):
// an unlock is then neither recorded nor announced.
//
// The interfaces, by vtable slot (Itanium and MSVC agree on these: the only overloads, GetStat /
// SetStat, sit in slots 1-4 in either order):
//   ISteamClient017      0 CreateSteamPipe, 1 BReleaseSteamPipe, 2 ConnectToGlobalUser, 4 ReleaseUser,
//                        9 GetISteamUtils, 13 GetISteamUserStats -- the ones a game's own
//                        libsteam_api.dylib calls (SteamAPI_Init: *0x0 / *0x10, its interface
//                        table: *0x48 "SteamUtils007", *0x68 "STEAMUSERSTATS_INTERFACE_VERSION011").
//   ISteamUserStats 011/012  0 RequestCurrentStats, 6 GetAchievement, 7 SetAchievement, 10 StoreStats;
//                        013 dropped RequestCurrentStats, so each of the others is one slot lower.
//   ISteamUtils007       9 GetAppID.
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// Not k2shim.h, whose Win32 typedefs collide with <windows.h>, which the Steam lookups need.
extern "C" {
const unsigned char* k2embedded_find(const char* pszName, size_t* pnSize);   // embedfiles.py
int k2host_achievement_show(const char* pszBubble, const char* pszLog);   // compat/achievement_toast.cpp
int k2host_achievement_language(void);
int k2qol_achievements(void);   // qol/k2qol.cpp (KOTOR 1)
}

#if !defined(K2P_MOBILE_HOST) && !defined(__ANDROID__)
#define K2_ACH_STEAM 1
#endif

namespace {

const unsigned kKotor2AppId = 208580;

enum Backend { kLocal, kSteamApi, kSteamClient };

// ---- the name table (resources/achievements/<game>/achievement_names.txt) -----------------------
struct Names {
    std::string name, desc;
};
// tag -> language -> name and description
typedef std::map<std::string, std::map<std::string, Names>> NameTable;

struct State {
    bool bInit = false;
    int nGame = 0;              // 1 KOTOR, 2 KOTOR II, 0 not told (a mobile host: local only)
    Backend backend = kLocal;
    std::map<std::string, std::string> unlocked;   // tag -> when (UTC, ISO 8601)
    std::vector<std::string> pendingSteam;         // unlocks before Steam's stats arrived
    std::vector<std::pair<std::string, std::string>> notices;   // bubble, log line
    bool bNamesLoaded = false;
    NameTable names;
    SDL_SpinLock lock = 0;
};
State& S()
{
    static State s;
    return s;
}

struct Lock {
    bool bHeld = true;
    Lock() { SDL_AtomicLock(&S().lock); }
    ~Lock() { Release(); }
    void Release()
    {
        if (bHeld) SDL_AtomicUnlock(&S().lock);
        bHeld = false;
    }
};

const char* GameTag() { return S().nGame == 1 ? "k1" : "k2"; }

std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
}

// ---- binary KeyValues (Steam's appcache/stats/UserGameStatsSchema_<app>.bin) ---------------------
struct KvNode {
    std::string key, value;
    std::vector<KvNode> kids;
    const KvNode* Find(const char* psz) const
    {
        for (const KvNode& k : kids)
            if (k.key == psz) return &k;
        return 0;
    }
};

bool ParseKv(const unsigned char* p, size_t n, size_t& i, KvNode& node, int nDepth)
{
    if (nDepth > 32) return false;
    while (i < n) {
        unsigned char t = p[i++];
        if (t == 8 || t == 11) return true;
        KvNode kid;
        size_t j = i;
        while (j < n && p[j]) j++;
        if (j >= n) return false;
        kid.key.assign(reinterpret_cast<const char*>(p) + i, j - i);
        i = j + 1;
        switch (t) {
        case 0:
            if (!ParseKv(p, n, i, kid, nDepth + 1)) return false;
            break;
        case 1:
            j = i;
            while (j < n && p[j]) j++;
            if (j >= n) return false;
            kid.value.assign(reinterpret_cast<const char*>(p) + i, j - i);
            i = j + 1;
            break;
        case 2: case 3: case 4: case 6:
            if (i + 4 > n) return false;
            if (t == 2) {
                int32_t v;
                memcpy(&v, p + i, 4);
                kid.value = std::to_string(v);
            }
            i += 4;
            break;
        case 5:   // UTF-16: skipped
            while (i + 1 < n && (p[i] || p[i + 1])) i += 2;
            i += 2;
            break;
        case 7: case 10:
            i += 8;
            break;
        default:
            return false;
        }
        node.kids.push_back(kid);
    }
    return false;
}

std::string HomeDir()
{
    const char* h = getenv("HOME");
    return h ? h : "";
}

#ifdef _WIN32
std::string Utf8(const wchar_t* pw)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, pw, -1, 0, 0, 0, 0);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, pw, -1, &s[0], n, 0, 0);
    return s;
}

std::wstring Wide(const std::string& s)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, 0, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

std::string RegString(const wchar_t* pszKey, const wchar_t* pszValue)
{
    wchar_t buf[1024];
    DWORD n = sizeof buf;
    if (RegGetValueW(HKEY_CURRENT_USER, pszKey, pszValue, RRF_RT_REG_SZ, 0, buf, &n) != ERROR_SUCCESS)
        return std::string();
    return Utf8(buf);
}
#endif

// Where the Steam client keeps its files (appcache/), or "".
std::vector<std::string> SteamRoots()
{
    std::vector<std::string> v;
#if defined(_WIN32)
    std::string s = RegString(L"Software\\Valve\\Steam", L"SteamPath");
    if (!s.empty()) v.push_back(s);
    v.push_back("C:/Program Files (x86)/Steam");
#elif defined(__APPLE__)
    v.push_back(HomeDir() + "/Library/Application Support/Steam");
#else
    v.push_back(HomeDir() + "/.steam/steam");
    v.push_back(HomeDir() + "/.local/share/Steam");
#endif
    return v;
}

bool ReadFile(const std::string& path, std::vector<unsigned char>& data)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    data.clear();
    unsigned char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) data.insert(data.end(), buf, buf + n);
    fclose(f);
    return true;
}

// Loads the player's cached schema (name rows for the tags it has) and its version.
int LoadSteamSchema(NameTable& out)
{
    std::vector<unsigned char> data;
    for (const std::string& root : SteamRoots())
        if (ReadFile(root + "/appcache/stats/UserGameStatsSchema_208580.bin", data)) break;
    if (data.empty()) return -1;
    KvNode root;
    size_t i = 0;
    ParseKv(data.data(), data.size(), i, root, 0);   // a truncated tail still leaves the rows read
    const KvNode* app = root.Find("208580");
    if (!app) return -1;
    const KvNode* stats = app->Find("stats");
    const KvNode* ver = app->Find("version");
    if (stats)
        for (const KvNode& stat : stats->kids)
            if (const KvNode* bits = stat.Find("bits"))
                for (const KvNode& bit : bits->kids) {
                    const KvNode* name = bit.Find("name");
                    const KvNode* disp = bit.Find("display");
                    const KvNode* dn = disp ? disp->Find("name") : 0;
                    const KvNode* dd = disp ? disp->Find("desc") : 0;
                    if (!name || !dn) continue;
                    for (const KvNode& lang : dn->kids) {
                        if (lang.key == "token") continue;
                        Names& n = out[name->value][lang.key];
                        n.name = lang.value;
                        const KvNode* d = dd ? dd->Find(lang.key.c_str()) : 0;
                        n.desc = d ? d->value : "";
                    }
                }
    return ver ? atoi(ver->value.c_str()) : 0;
}

void LoadNames()
{
    State& s = S();
    if (s.bNamesLoaded) return;
    s.bNamesLoaded = true;
    int nVersion = 0;
    size_t nSize = 0;
    if (const unsigned char* p = k2embedded_find("achievement_names.txt", &nSize)) {
        std::string text(reinterpret_cast<const char*>(p), nSize);
        size_t pos = 0;
        while (pos < text.size()) {
            size_t end = text.find('\n', pos);
            if (end == std::string::npos) end = text.size();
            std::string line = text.substr(pos, end - pos);
            pos = end + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.compare(0, 10, "# version ") == 0) nVersion = atoi(line.c_str() + 10);
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> f;
            size_t a = 0;
            for (;;) {
                size_t b = line.find('\t', a);
                f.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a));
                if (b == std::string::npos) break;
                a = b + 1;
            }
            if (f.size() >= 3) {
                Names& n = s.names[f[0]][f[1]];
                n.name = f[2];
                n.desc = f.size() > 3 ? f[3] : "";
            }
        }
    }
#ifdef K2_ACH_STEAM
    if (s.nGame == 2) {
        // The live schema applies wholesale when it is newer than the table, else only to tags the
        // table lacks.
        NameTable live;
        int nLive = LoadSteamSchema(live);
        for (auto& tag : live)
            if (nLive > nVersion || !s.names.count(tag.first)) s.names[tag.first] = tag.second;
    }
#endif
}

// The player's language as Steam names it, from the game's dialog.tlk.
const char* SteamLanguage()
{
    switch (k2host_achievement_language()) {
    case 1: return "french";
    case 2: return "german";
    case 3: return "italian";
    case 4: return "spanish";
    default: return "english";
    }
}

// "a_mandalorian_affair" -> "A Mandalorian Affair": a tag the table does not have.
std::string Humanise(const std::string& tag)
{
    std::string out;
    bool bStart = true;
    for (char c : tag) {
        if (c == '_') {
            if (!out.empty() && out.back() != ' ') out += ' ';
            bStart = true;
        } else {
            out += bStart && c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
            bStart = false;
        }
    }
    return Trim(out);
}

void Describe(const std::string& tag, std::string& name, std::string& desc)
{
    LoadNames();
    State& s = S();
    auto it = s.names.find(tag);
    if (it != s.names.end()) {
        auto l = it->second.find(SteamLanguage());
        if (l == it->second.end()) l = it->second.find("english");
        if (l == it->second.end()) l = it->second.begin();
        if (l != it->second.end()) {
            name = l->second.name;
            desc = l->second.desc;
            return;
        }
    }
    name = s.nGame == 2 && tag.compare(0, 12, "achievement_") == 0 ? tag : Humanise(tag);
    desc.clear();
}

// ---- achievements.ini ---------------------------------------------------------------------------
// A relative path, since the shim's fopen reads the player's folder first and writes only there.
const char* const kIniName = "achievements.ini";

void LoadIni()
{
    FILE* f = fopen(kIniName, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string l = Trim(line);
        if (l.empty() || l[0] == ';' || l[0] == '#' || l[0] == '[') continue;
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string tag = Trim(l.substr(0, eq));
        if (!tag.empty()) S().unlocked[tag] = Trim(l.substr(eq + 1));
    }
    fclose(f);
}

void SaveIni()
{
    std::string tmp = std::string(kIniName) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) {
        fprintf(stderr, "%s: achievements: cannot write %s\n", GameTag(), kIniName);
        return;
    }
    fprintf(f, "; Achievements this player has unlocked (tag = when, UTC). Kept by the game whether or not\n"
               "; Steam is reporting them; delete a line to have its notice shown again.\n"
               "[achievements]\n");
    for (auto& e : S().unlocked) fprintf(f, "%s=%s\n", e.first.c_str(), e.second.c_str());
    bool bOk = fclose(f) == 0;
    if (!bOk || rename(tmp.c_str(), kIniName) != 0) {
        fprintf(stderr, "%s: achievements: cannot replace %s\n", GameTag(), kIniName);
        remove(tmp.c_str());
    }
}

std::string NowUtc()
{
    time_t t = time(0);
    struct tm tmv;
    gmtime_r(&t, &tmv);
    char buf[32];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return buf;
}

#ifdef K2_ACH_STEAM
// ---- Steam ----------------------------------------------------------------------------------------
bool SteamReadOnly()
{
    const char* p = getenv("K2_STEAM_READONLY");
    return p && *p && *p != '0';
}

unsigned ParseAppId(const char* p)
{
    if (!p) return 0;
    while (*p == ' ' || *p == '\t') p++;
    return static_cast<unsigned>(strtoul(p, 0, 10));
}

std::string ExeDir()
{
    char* p = SDL_GetBasePath();
    std::string s = p ? p : "";
    SDL_free(p);
    if (!s.empty() && s.back() != '/' && s.back() != '\\') s += '/';
    return s;
}

// The app id the process runs as, and where it came from; 0 for none.
unsigned FindAppId(std::string& how)
{
    if (const char* p = getenv("K2_STEAM_APPID")) {
        how = "K2_STEAM_APPID";
        return ParseAppId(p);
    }
    for (const char* var : { "SteamAppId", "SteamGameId" })
        if (unsigned n = ParseAppId(getenv(var))) {
            how = var;
            return n;
        }
    std::string cwd;
    if (char* p = getcwd(0, 0)) {
        cwd = p;
        free(p);
        cwd += '/';
    }
    for (const std::string& dir : { ExeDir(), cwd }) {
        if (dir.empty()) continue;
        std::string path = dir + "steam_appid.txt";
        if (FILE* f = fopen(path.c_str(), "r")) {
            char buf[32] = "";
            if (!fgets(buf, sizeof buf, f)) buf[0] = 0;
            fclose(f);
            how = path;
            return ParseAppId(buf);
        }
    }
    return 0;
}

void* LoadLib(const std::string& path)
{
#ifdef _WIN32
    // The library's own folder comes first for what it imports (steamclient64.dll imports
    // tier0_s64.dll ...).
    return static_cast<void*>(LoadLibraryExW(Wide(path).c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH));
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void CloseLib(void* h)
{
#ifdef _WIN32
    if (h) FreeLibrary(static_cast<HMODULE>(h));
#else
    if (h) dlclose(h);
#endif
}

bool FileExists(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f) fclose(f);
    return f != 0;
}

#ifdef __APPLE__
// Whether a Mach-O (thin or universal) has this process's architecture.
bool MachOHasOurArch(const std::string& path)
{
#if defined(__aarch64__)
    const uint32_t kOurs = 0x0100000c;   // CPU_TYPE_ARM64
#else
    const uint32_t kOurs = 0x01000007;   // CPU_TYPE_X86_64
#endif
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char h[4096];
    size_t n = fread(h, 1, sizeof h, f);
    fclose(f);
    if (n < 8) return false;
    uint32_t be = static_cast<uint32_t>(h[0]) << 24 | h[1] << 16 | h[2] << 8 | h[3];
    uint32_t le = h[0] | h[1] << 8 | h[2] << 16 | static_cast<uint32_t>(h[3]) << 24;
    if (be == 0xcafebabe) {   // FAT_MAGIC, big-endian fat_arch records of 20 bytes
        uint32_t count = static_cast<uint32_t>(h[4]) << 24 | h[5] << 16 | h[6] << 8 | h[7];
        for (uint32_t i = 0; i < count && 8 + i * 20 + 4 <= n; i++) {
            const unsigned char* a = h + 8 + i * 20;
            if ((static_cast<uint32_t>(a[0]) << 24 | a[1] << 16 | a[2] << 8 | a[3]) == kOurs) return true;
        }
        return false;
    }
    if (le == 0xfeedfacf) {   // MH_MAGIC_64
        uint32_t cpu = h[4] | h[5] << 8 | h[6] << 16 | static_cast<uint32_t>(h[7]) << 24;
        return cpu == kOurs;
    }
    return false;
}
#endif

// The interface pointers and calls, whichever backend supplied them.
struct Steam {
    void* hLib = 0;
    void* pStats = 0;           // ISteamUserStats
    int nStatsSlot = 0;         // -1 for 013 (no RequestCurrentStats)
    // steam_api
    void (*RunCallbacks)() = 0;
    void (*Shutdown)() = 0;
    bool (*FlatRequest)(void*) = 0;
    bool (*FlatGet)(void*, const char*, bool*) = 0;
    bool (*FlatSet)(void*, const char*) = 0;
    bool (*FlatStore)(void*) = 0;
    // steamclient
    void* pClient = 0;
    int hPipe = 0, hUser = 0;
    bool (*BGetCallback)(int, void*) = 0;
    void (*FreeLastCallback)(int) = 0;
    unsigned nAppId = 0;
    bool bReady = false;        // the stats arrived
    bool bRequested = false;
    uint32_t nRequestTicks = 0;
};
Steam g_steam;

typedef void* (*VFn)();
VFn Slot(void* pObj, int n) { return (*static_cast<VFn**>(pObj))[n]; }

bool StatsRequest()
{
    Steam& st = g_steam;
    if (st.FlatRequest) return st.FlatRequest(st.pStats);
    if (st.FlatGet) return true;   // 013's flat API: the stats are requested at init
    if (st.nStatsSlot < 0) return true;
    return (reinterpret_cast<bool (*)(void*)>(Slot(st.pStats, 0)))(st.pStats);
}

bool StatsGet(const char* pszTag, bool* pbAchieved)
{
    Steam& st = g_steam;
    if (st.FlatGet) return st.FlatGet(st.pStats, pszTag, pbAchieved);
    return (reinterpret_cast<bool (*)(void*, const char*, bool*)>(Slot(st.pStats, 6 + st.nStatsSlot)))(st.pStats, pszTag,
                                                                                     pbAchieved);
}

bool StatsSet(const char* pszTag)
{
    Steam& st = g_steam;
    if (st.FlatSet) return st.FlatSet(st.pStats, pszTag);
    return (reinterpret_cast<bool (*)(void*, const char*)>(Slot(st.pStats, 7 + st.nStatsSlot)))(st.pStats, pszTag);
}

bool StatsStore()
{
    Steam& st = g_steam;
    if (st.FlatStore) return st.FlatStore(st.pStats);
    return (reinterpret_cast<bool (*)(void*)>(Slot(st.pStats, 10 + st.nStatsSlot)))(st.pStats);
}

// bUnload false at process exit: Steam's library is left loaded, as unloading it there can wait on
// its own threads (the process then never ends).
void SteamShutdown(bool bUnload = true)
{
    Steam& st = g_steam;
    if (st.Shutdown) {
        st.Shutdown();
    } else if (st.pClient) {
        if (st.hUser) (reinterpret_cast<void (*)(void*, int, int)>(Slot(st.pClient, 4)))(st.pClient, st.hPipe, st.hUser);
        if (st.hPipe) (reinterpret_cast<bool (*)(void*, int)>(Slot(st.pClient, 1)))(st.pClient, st.hPipe);
    }
    if (bUnload) CloseLib(st.hLib);
    st = Steam();
}

template <class T> T Sym(void* h, const char* psz) { return reinterpret_cast<T>(dlsym(h, psz)); }

// Backend 1, a steam_api library.
bool TrySteamApi(std::string& detail)
{
#if defined(_WIN32)
    const char* pszLib = "steam_api64.dll";
#elif defined(__APPLE__)
    const char* pszLib = "libsteam_api.dylib";
#else
    const char* pszLib = "libsteam_api.so";
#endif
    std::vector<std::string> dirs;
    dirs.push_back(ExeDir());
    dirs.push_back(ExeDir() + "../Frameworks/");
    dirs.push_back(ExeDir() + "../MacOS/");
    if (char* p = getcwd(0, 0)) {
        std::string cwd = std::string(p) + "/";
        free(p);
        dirs.push_back(cwd);
        dirs.push_back(cwd + "../MacOS/");   // a Mac bundle's GameData beside its MacOS
        dirs.push_back(cwd + "../Frameworks/");
    }
    for (const std::string& dir : dirs) {
        std::string path = dir + pszLib;
        if (!FileExists(path)) continue;
#ifdef __APPLE__
        if (!MachOHasOurArch(path)) {
            fprintf(stderr, "%s: achievements: %s is not this build's architecture\n", GameTag(), path.c_str());
            continue;
        }
#endif
        void* h = LoadLib(path);
        if (!h) {
            fprintf(stderr, "%s: achievements: cannot load %s: %s\n", GameTag(), path.c_str(),
                    dlerror() ? dlerror() : "?");
            continue;
        }
        Steam& st = g_steam;
        st.hLib = h;
        bool bInit = false;
        if (auto InitFlat = Sym<int (*)(char*)>(h, "SteamAPI_InitFlat")) {
            static char szErr[1024];
            int r = InitFlat(szErr);
            bInit = r == 0;
            if (!bInit) fprintf(stderr, "%s: achievements: SteamAPI_InitFlat: %d %s\n", GameTag(), r, szErr);
        } else if (auto Init = Sym<bool (*)()>(h, "SteamAPI_Init")) {
            bInit = Init();
            if (!bInit) fprintf(stderr, "%s: achievements: SteamAPI_Init failed\n", GameTag());
        }
        if (!bInit) {
            CloseLib(h);
            st = Steam();
            continue;
        }
        st.Shutdown = Sym<void (*)()>(h, "SteamAPI_Shutdown");
        st.RunCallbacks = Sym<void (*)()>(h, "SteamAPI_RunCallbacks");
        const char* pszAccessor = 0;
        for (const char* psz : { "SteamAPI_SteamUserStats_v012", "SteamAPI_SteamUserStats_v011",
                                 "SteamAPI_SteamUserStats_v013", "SteamUserStats" })
            if (auto Get = Sym<void* (*)()>(h, psz))
                if ((st.pStats = Get())) {
                    pszAccessor = psz;
                    break;
                }
        st.FlatRequest = Sym<bool (*)(void*)>(h, "SteamAPI_ISteamUserStats_RequestCurrentStats");
        st.FlatGet = Sym<bool (*)(void*, const char*, bool*)>(h, "SteamAPI_ISteamUserStats_GetAchievement");
        st.FlatSet = Sym<bool (*)(void*, const char*)>(h, "SteamAPI_ISteamUserStats_SetAchievement");
        st.FlatStore = Sym<bool (*)(void*)>(h, "SteamAPI_ISteamUserStats_StoreStats");
        if (!st.FlatGet || !st.FlatSet || !st.FlatStore) {
            // An SDK without the flat API has SteamUserStats() at 011 or 012 (the same slots).
            st.FlatRequest = 0;
            st.FlatGet = 0;
            st.FlatSet = 0;
            st.FlatStore = 0;
            if (pszAccessor && strstr(pszAccessor, "v013")) st.pStats = 0;
        }
        if (!st.pStats) {
            fprintf(stderr, "%s: achievements: %s has no ISteamUserStats\n", GameTag(), path.c_str());
            SteamShutdown();
            continue;
        }
        detail = "steam_api, " + path;
        return true;
    }
    return false;
}

// Backend 2, the Steam client's steamclient library.
bool TrySteamClient(std::string& detail)
{
    std::vector<std::string> paths;
#if defined(_WIN32)
    std::string reg = RegString(L"Software\\Valve\\Steam\\ActiveProcess", L"SteamClientDll64");
    if (!reg.empty()) paths.push_back(reg);
#elif defined(__APPLE__)
    paths.push_back(HomeDir() + "/Library/Application Support/Steam/Steam.AppBundle/Steam/Contents/MacOS/steamclient.dylib");
#else
    paths.push_back(HomeDir() + "/.steam/sdk64/steamclient.so");
    paths.push_back(HomeDir() + "/.steam/steam/linux64/steamclient.so");
    paths.push_back(HomeDir() + "/.local/share/Steam/linux64/steamclient.so");
#endif
    for (const std::string& path : paths) {
        if (!FileExists(path)) continue;
        void* h = LoadLib(path);
        if (!h) {
            fprintf(stderr, "%s: achievements: cannot load %s: %s\n", GameTag(), path.c_str(),
                    dlerror() ? dlerror() : "?");
            continue;
        }
        Steam& st = g_steam;
        st.hLib = h;
        auto CreateInterface = Sym<void* (*)(const char*, int*)>(h, "CreateInterface");
        st.BGetCallback = Sym<bool (*)(int, void*)>(h, "Steam_BGetCallback");
        st.FreeLastCallback = Sym<void (*)(int)>(h, "Steam_FreeLastCallback");
        const char* pszClient = 0;
        if (CreateInterface)
            for (const char* psz : { "SteamClient017", "SteamClient018", "SteamClient019", "SteamClient020",
                                     "SteamClient021" }) {
                int rc = 0;
                if ((st.pClient = CreateInterface(psz, &rc))) {
                    pszClient = psz;
                    break;
                }
            }
        if (!st.pClient) {
            fprintf(stderr, "%s: achievements: %s: no ISteamClient\n", GameTag(), path.c_str());
            CloseLib(h);
            st = Steam();
            continue;
        }
        st.hPipe = (reinterpret_cast<int (*)(void*)>(Slot(st.pClient, 0)))(st.pClient);
        if (st.hPipe)
            st.hUser = (reinterpret_cast<int (*)(void*, int)>(Slot(st.pClient, 2)))(st.pClient, st.hPipe);
        if (!st.hPipe || !st.hUser) {
            fprintf(stderr, "%s: achievements: Steam is not running or not logged in (%s)\n", GameTag(),
                    st.hPipe ? "no user" : "no pipe");
            SteamShutdown();
            return false;
        }
        typedef void* (*GetStats)(void*, int, int, const char*);
        struct { const char* psz; int nSlot; } versions[] = {
            { "STEAMUSERSTATS_INTERFACE_VERSION012", 0 },
            { "STEAMUSERSTATS_INTERFACE_VERSION011", 0 },
            { "STEAMUSERSTATS_INTERFACE_VERSION013", -1 },
        };
        const char* pszStats = 0;
        for (auto& v : versions)
            if ((st.pStats = (reinterpret_cast<GetStats>(Slot(st.pClient, 13)))(st.pClient, st.hUser, st.hPipe, v.psz))) {
                st.nStatsSlot = v.nSlot;
                pszStats = v.psz;
                break;
            }
        if (!st.pStats) {
            fprintf(stderr, "%s: achievements: %s: no ISteamUserStats\n", GameTag(), path.c_str());
            SteamShutdown();
            continue;
        }
        // The app Steam has put this process in.
        typedef void* (*GetUtils)(void*, int, const char*);
        if (void* pUtils = (reinterpret_cast<GetUtils>(Slot(st.pClient, 9)))(st.pClient, st.hPipe, "SteamUtils007")) {
            unsigned nApp = (reinterpret_cast<unsigned (*)(void*)>(Slot(pUtils, 9)))(pUtils);
            if (nApp != st.nAppId)
                fprintf(stderr, "%s: achievements: Steam reports app %u, not %u\n", GameTag(), nApp, st.nAppId);
        }
        detail = std::string("steamclient, ") + pszClient + " / " + (pszStats + 25);
        return true;
    }
    return false;
}

// Runs the callbacks, through the steam_api library's dispatch or by draining the pipe's queue
// here.
void SteamPump()
{
    Steam& st = g_steam;
    if (st.RunCallbacks) st.RunCallbacks();
    if (st.BGetCallback && st.FreeLastCallback && st.hPipe) {
        struct { int hUser; int iCallback; unsigned char* pParam; int cubParam; int pad; } msg;
        for (int i = 0; i < 64 && st.BGetCallback(st.hPipe, &msg); i++) {
            if ((msg.iCallback == 1101 || msg.iCallback == 1102) && msg.pParam && msg.cubParam >= 12) {
                int nResult;   // UserStatsReceived_t / UserStatsStored_t: { uint64 game; EResult; ... }
                memcpy(&nResult, msg.pParam + 8, 4);
                if (nResult != 1)
                    fprintf(stderr, "%s: achievements: Steam %s: result %d\n", GameTag(),
                            msg.iCallback == 1101 ? "stats request" : "store", nResult);
            }
            st.FreeLastCallback(st.hPipe);
        }
    }
}

Backend StartSteam(std::string& detail)
{
    std::string how;
    unsigned nAppId = FindAppId(how);
    if (nAppId == 0) {
        detail = how.empty() ? "no Steam launch" : how + " is 0";
        return kLocal;
    }
    if (nAppId != kKotor2AppId) {
        detail = "app " + std::to_string(nAppId) + " from " + how + " is not KOTOR II's";
        return kLocal;
    }
    // Steam associates the process with the app through these (a Steam launch already set them).
    std::string id = std::to_string(nAppId);
    setenv("SteamAppId", id.c_str(), 1);
    setenv("SteamGameId", id.c_str(), 1);
    g_steam.nAppId = nAppId;
    Backend b = kLocal;
    if (TrySteamApi(detail)) b = kSteamApi;
    else if (TrySteamClient(detail)) b = kSteamClient;
    if (b == kLocal) {
        detail = "app " + id + " from " + how + ", but Steam not reachable";
        return kLocal;
    }
    g_steam.nAppId = nAppId;
    detail += ", app " + id;
    if (!StatsRequest()) {
        fprintf(stderr, "%s: achievements: steam (%s): RequestCurrentStats failed\n", GameTag(), detail.c_str());
        SteamShutdown();
        detail = "the Steam stats request failed";
        return kLocal;
    }
    g_steam.bRequested = true;
    g_steam.nRequestTicks = SDL_GetTicks();
    return b;
}

void SendToSteam(const std::string& tag)
{
    if (SteamReadOnly()) {
        fprintf(stderr, "%s: achievements: steam: read-only, %s not set\n", GameTag(), tag.c_str());
        return;
    }
    bool bSet = StatsSet(tag.c_str());
    bool bStored = bSet && StatsStore();
    fprintf(stderr, "%s: achievements: steam: %s %s\n", GameTag(), tag.c_str(),
            bStored ? "set and stored" : bSet ? "set, StoreStats failed" : "SetAchievement failed");
}

// Polls once a frame until the stats arrive, since GetAchievement answers only once they have.
void SteamPollReady()
{
    State& s = S();
    Steam& st = g_steam;
    if (st.bReady) return;
    bool bAchieved = false;
    if (StatsGet("achievement_0", &bAchieved)) {
        st.bReady = true;
        int nUnlocked = 0;
        for (int i = 0; i < 57; i++) {
            char tag[32];
            snprintf(tag, sizeof tag, "achievement_%d", i);
            bool b = false;
            if (StatsGet(tag, &b) && b) nUnlocked++;
        }
        fprintf(stderr, "%s: achievements: steam stats received: achievement_0 %s, %d of 57 unlocked\n",
                GameTag(), bAchieved ? "unlocked" : "locked", nUnlocked);
        std::vector<std::string> pending;
        pending.swap(s.pendingSteam);
        for (const std::string& tag : pending) SendToSteam(tag);
        return;
    }
    if (SDL_GetTicks() - st.nRequestTicks > 15000) {
        fprintf(stderr, "%s: achievements: steam stats never arrived; achievements: local\n", GameTag());
        SteamShutdown();
        s.backend = kLocal;
        // What waited for Steam is recorded already; show it now.
        std::vector<std::string> pending;
        pending.swap(s.pendingSteam);
        for (const std::string& tag : pending) {
            std::string name, desc;
            Describe(tag, name, desc);
            s.notices.push_back(std::make_pair("Achievement unlocked: " + name,
                                               "Achievement unlocked: " + name + (desc.empty() ? "" : " - " + desc)));
        }
    }
}
#endif   // K2_ACH_STEAM

void Init(int nGame)
{
    State& s = S();
    if (s.bInit) return;
    s.bInit = true;
    s.nGame = nGame;
    LoadIni();
    std::string detail;
#ifdef K2_ACH_STEAM
    if (nGame == 2) {
        s.backend = StartSteam(detail);
        atexit([] {
            if (S().backend != kLocal) SteamShutdown(false);
        });
    } else if (nGame == 1) {
        detail = "KOTOR has no Steam achievements";
    }
#endif
    if (s.backend == kLocal)
        fprintf(stderr, "%s: achievements: local (%s%s%zu unlocked in %s)\n", GameTag(), detail.c_str(),
                detail.empty() ? "" : "; ", s.unlocked.size(), kIniName);
    else
        fprintf(stderr, "%s: achievements: steam (%s)\n", GameTag(), detail.c_str());
}

} // namespace

extern "C" {

// From the desktop entry point (macos/main.cpp) once the game's and player's folders are set:
// nGame 1 for KOTOR, 2 for KOTOR II. A host that never calls it gets the local backend.
void k2host_achievements_init(int nGame)
{
    Lock lock;
    Init(nGame);
}

void k2host_unlock_achievement(const char* pszTag)
{
    if (!pszTag || !*pszTag) return;
    Lock lock;
    State& s = S();
    Init(0);
#ifdef K2_QOL_K1
    if (!k2qol_achievements()) {
        fprintf(stderr, "%s: achievements: off (Other Settings), %s not unlocked\n", GameTag(), pszTag);
        return;
    }
#endif
    std::string tag = pszTag;
    bool bNew = !s.unlocked.count(tag);
    if (bNew) {
        s.unlocked[tag] = NowUtc();
        SaveIni();
    }
#ifdef K2_ACH_STEAM
    if (s.backend != kLocal) {
        // Steam dedups; a tag it has had since the last local session is still sent.
        if (g_steam.bReady) SendToSteam(tag);
        else s.pendingSteam.push_back(tag);
        return;
    }
#endif
    if (!bNew) return;
    std::string name, desc;
    Describe(tag, name, desc);
    fprintf(stderr, "%s: achievements: unlocked %s (%s)\n", GameTag(), tag.c_str(), name.c_str());
    s.notices.push_back(std::make_pair("Achievement unlocked: " + name,
                                       "Achievement unlocked: " + name + (desc.empty() ? "" : " - " + desc)));
}

// Once a frame from the main loop (shim_time.cpp k2host_pace_frame).
void k2host_achievements_frame(void)
{
    State& s = S();
    if (!s.bInit) return;
    Lock lock;
#ifdef K2_ACH_STEAM
    if (s.backend != kLocal) {
        SteamPump();
        SteamPollReady();
    }
#endif
    if (s.notices.empty()) return;
    std::pair<std::string, std::string> notice = s.notices.front();
    // Unlocked while the engine shows it, because what it calls may itself unlock (the talk table).
    lock.Release();
    if (!k2host_achievement_show(notice.first.c_str(), notice.second.c_str())) return;
    Lock relock;
    if (!s.notices.empty() && s.notices.front() == notice) s.notices.erase(s.notices.begin());
}

} // extern "C"
