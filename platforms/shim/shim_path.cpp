// Path translation, the user-data layer and the GL entry-point loader.
//
// The desktop builds keep the game's files and the player's apart. The working directory is the
// installed copy, read-only by intent, and a second user-data directory holds what the game writes
// -- saves/, swkotor.ini, temp/, gameinprogress/, the prompt art in override/. A relative engine path
// reads from the user directory when the file is there and from the install when it is not, every
// write goes to the user directory, and a directory listing is the two merged, the user's entry
// winning a name both have. Nothing is written into the install, which on macOS is a signed app
// bundle.
//
// Mounts are further read-only directories, each seen under one relative folder: KOTOR II's Steam
// Workshop items under dlc/mods_english, the mod folder the engine already searches ahead of the
// game's own files (GetOverridePath). They sit between the user directory and the install, the first
// mounted first.
//
// A redirect moves one relative folder somewhere else for reads and writes alike, ahead of every
// other layer: saves/ to the Steam Cloud folder kept beside the game.
#include "k2shim.h"

#include <SDL.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <mutex>
#include <set>
#include <string>

namespace {

std::string s_user;   // absolute; empty leaves everything in the working directory
struct Mount {
    std::string prefix;   // lower case, no trailing '/'
    std::string dir;      // absolute
};
std::vector<Mount> s_mounts;
std::vector<Mount> s_redirects;   // prefix is the folder each replacement takes over

template <class Fn>
Fn RealFn(const char* pszName)
{
    Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, pszName));
    if (!fn) {
        fprintf(stderr, "k2shim: libc %s not found\n", pszName);
        abort();
    }
    return fn;
}

// Calls libc's own entry points, past this file's overrides of the same names.
DIR* RealOpendir(const char* pszPath)
{
    static auto real = RealFn<DIR* (*)(const char*)>("opendir");
    return real(pszPath);
}
struct dirent* RealReaddir(DIR* d)
{
    static auto real = RealFn<struct dirent* (*)(DIR*)>("readdir");
    return real(d);
}
int RealClosedir(DIR* d)
{
    static auto real = RealFn<int (*)(DIR*)>("closedir");
    return real(d);
}
bool RealIsDir(const char* pszPath)
{
    static auto real = RealFn<int (*)(const char*, struct stat*)>("stat");
    struct stat st;
    return real(pszPath, &st) == 0 && S_ISDIR(st.st_mode);
}
int RealMkdir(const char* pszPath, mode_t nMode)
{
    static auto real = RealFn<int (*)(const char*, mode_t)>("mkdir");
    return real(pszPath, nMode);
}

// Whether psz is a Windows drive-letter path ("C:/..."), which is absolute like a leading '/'.
bool DrivePath(const char* psz)
{
#ifdef _WIN32
    return psz && isalpha(static_cast<unsigned char>(psz[0])) && psz[1] == ':';
#else
    (void)psz;
    return false;
#endif
}

// Recognises an engine path the layers apply to -- relative, inside the working directory -- and
// normalises it: backslashes become '/', a leading "./" goes.
bool Relative(const char* pszIn, std::string& rel)
{
    if (!pszIn || !*pszIn || DrivePath(pszIn)) return false;
    rel = pszIn;
    for (char& c : rel)
        if (c == '\\') c = '/';
    while (rel.size() >= 2 && rel[0] == '.' && rel[1] == '/') rel.erase(0, 2);
    while (!rel.empty() && rel[0] == '/' && pszIn[0] != '/') rel.erase(0, 1);
    if (rel.empty() || rel == "." || rel[0] == '/' || rel.compare(0, 2, "..") == 0) return false;
    return true;
}

bool UserRelative(const char* pszIn, std::string& rel) { return !s_user.empty() && Relative(pszIn, rel); }

// Resolves rel inside m, when it lies within the folder m appears as.
bool InMount(const Mount& m, const std::string& rel, std::string& path)
{
    size_t n = m.prefix.size();
    if (rel.size() < n || strncasecmp(rel.c_str(), m.prefix.c_str(), n) != 0) return false;
    if (rel.size() > n && rel[n] != '/') return false;
    path = m.dir + rel.substr(n);
    return true;
}

// Creates the missing parent directories of path, down from the user directory.
void MakeParents(const std::string& path)
{
    for (size_t i = s_user.size() + 1; (i = path.find('/', i)) != std::string::npos; ++i) {
        std::string dir = path.substr(0, i);
        struct stat st;
        if (lstat(dir.c_str(), &st) != 0) RealMkdir(dir.c_str(), 0755);
    }
}

char* Resolve(const char* pszIn, char* pszOut, size_t nOut);

// Resolves rel through the redirects, if one owns it.
bool Redirected(const std::string& rel, std::string& path)
{
    for (const Mount& m : s_redirects)
        if (InMount(m, rel, path)) return true;
    return false;
}

// Creates the missing parent directories of path, down from root.
void MakeParentsUnder(const std::string& root, const std::string& path)
{
    for (size_t i = root.size() + 1; (i = path.find('/', i)) != std::string::npos; ++i) {
        std::string dir = path.substr(0, i);
        struct stat st;
        if (lstat(dir.c_str(), &st) != 0) RealMkdir(dir.c_str(), 0755);
    }
}

} // namespace

void k2shim_set_user_dir(const char* pszDir)
{
    s_user = pszDir ? pszDir : "";
    while (s_user.size() > 1 && s_user.back() == '/') s_user.pop_back();
    if (!s_user.empty()) {
        RealMkdir(s_user.c_str(), 0755);
        MakeParents(s_user + "/");
    }
}

const char* k2shim_user_dir() { return s_user.c_str(); }

void k2shim_add_mount(const char* pszRelDir, const char* pszAbsDir)
{
    Mount m{ pszRelDir, pszAbsDir };
    for (char& c : m.prefix) c = static_cast<char>(tolower(static_cast<unsigned char>(c == '\\' ? '/' : c)));
    while (!m.prefix.empty() && m.prefix.back() == '/') m.prefix.pop_back();
    while (m.dir.size() > 1 && m.dir.back() == '/') m.dir.pop_back();
    s_mounts.push_back(m);
}

void k2shim_redirect(const char* pszRelDir, const char* pszAbsDir)
{
    Mount m{ pszRelDir, pszAbsDir };
    for (char& c : m.prefix) c = static_cast<char>(tolower(static_cast<unsigned char>(c == '\\' ? '/' : c)));
    while (!m.prefix.empty() && m.prefix.back() == '/') m.prefix.pop_back();
    while (m.dir.size() > 1 && m.dir.back() == '/') m.dir.pop_back();
    s_redirects.push_back(m);
}

char* k2shim_host_path(const char* pszIn, char* pszOut, size_t nOut)
{
    std::string rel;
    if (Relative(pszIn, rel)) {
        std::string redirect;
        if (Redirected(rel, redirect)) return Resolve(redirect.c_str(), pszOut, nOut);
        struct stat st;
        if (!s_user.empty()) {
            Resolve((s_user + "/" + rel).c_str(), pszOut, nOut);
            if (lstat(pszOut, &st) == 0) return pszOut;
        }
        std::string path;
        for (const Mount& m : s_mounts)
            if (InMount(m, rel, path)) {
                Resolve(path.c_str(), pszOut, nOut);
                if (lstat(pszOut, &st) == 0) return pszOut;
            }
    }
    return Resolve(pszIn, pszOut, nOut);
}

char* k2shim_host_path_write(const char* pszIn, char* pszOut, size_t nOut, bool bMakeParents)
{
    std::string rel, redirect;
    if (Relative(pszIn, rel) && Redirected(rel, redirect)) {
        Resolve(redirect.c_str(), pszOut, nOut);
        for (const Mount& m : s_redirects)
            if (bMakeParents && strncmp(pszOut, m.dir.c_str(), m.dir.size()) == 0)
                MakeParentsUnder(m.dir, pszOut);
        return pszOut;
    }
    if (!UserRelative(pszIn, rel)) return Resolve(pszIn, pszOut, nOut);
    Resolve((s_user + "/" + rel).c_str(), pszOut, nOut);
    if (bMakeParents) MakeParents(pszOut);
    return pszOut;
}

void k2shim_layer_dirs(const char* pszDir, std::vector<std::string>& upper, std::string& install)
{
    char buf[1024];
    upper.clear();
    std::string rel, redirect;
    if (Relative(pszDir, rel) && Redirected(rel, redirect)) {
        Resolve(redirect.c_str(), buf, sizeof buf);
        if (RealIsDir(buf)) upper.push_back(buf);
        install.clear();
        return;
    }
    if (Relative(pszDir, rel)) {
        if (!s_user.empty()) {
            Resolve((s_user + "/" + rel).c_str(), buf, sizeof buf);
            if (RealIsDir(buf)) upper.push_back(buf);
        }
        std::string path;
        for (const Mount& m : s_mounts)
            if (InMount(m, rel, path)) {
                Resolve(path.c_str(), buf, sizeof buf);
                if (RealIsDir(buf)) upper.push_back(buf);
            }
    }
    Resolve(pszDir && *pszDir ? pszDir : ".", buf, sizeof buf);
    install = buf[0] ? buf : ".";
    if (!RealIsDir(install.c_str())) install.clear();
}

void k2shim_list_dir(const char* pszHostDir, std::vector<std::string>& names)
{
    if (DIR* d = RealOpendir(pszHostDir && *pszHostDir ? pszHostDir : ".")) {
        while (struct dirent* e = RealReaddir(d))
            if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) names.push_back(e->d_name);
        RealClosedir(d);
    }
}

#ifndef K2P_MOBILE_HOST
// opendir/readdir/closedir for the engine's own listings (KOTOR 1 walks directories itself, KOTOR II
// through FindFirstFileA): a relative directory is found case-blind and, with a user layer or mounts,
// is every layer's copy merged. Absolute paths and every DIR libc made stay libc's.
namespace {
struct MergedDir {
    std::vector<struct dirent> entries;
    size_t next;
};
std::mutex s_dirLock;
std::set<void*> s_merged;

MergedDir* Merged(DIR* d)
{
    std::lock_guard<std::mutex> g(s_dirLock);
    return s_merged.count(d) ? reinterpret_cast<MergedDir*>(d) : 0;
}
} // namespace

extern "C" {

DIR* opendir(const char* pszPath)
{
    if (pszPath[0] == '/' || DrivePath(pszPath)) return RealOpendir(pszPath);
    std::vector<std::string> dirs;
    std::string install;
    k2shim_layer_dirs(pszPath, dirs, install);
    if (!install.empty()) dirs.push_back(install);
    if (dirs.size() < 2) {
        char buf[1024];
        return RealOpendir(!dirs.empty() ? dirs[0].c_str() : Resolve(pszPath, buf, sizeof buf));
    }
    MergedDir* m = new MergedDir();
    m->next = 0;
    std::set<std::string> seen;
    for (const std::string& dir : dirs) {
        DIR* d = RealOpendir(dir.c_str());
        if (!d) continue;
        while (struct dirent* e = RealReaddir(d)) {
            std::string key(e->d_name);
            for (char& c : key) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (seen.insert(key).second) m->entries.push_back(*e);
        }
        RealClosedir(d);
    }
    std::lock_guard<std::mutex> g(s_dirLock);
    s_merged.insert(m);
    return reinterpret_cast<DIR*>(m);
}

struct dirent* readdir(DIR* d)
{
    if (MergedDir* m = Merged(d)) return m->next < m->entries.size() ? &m->entries[m->next++] : 0;
    return RealReaddir(d);
}

#if defined(__GLIBC__)
// The same record on LP64 glibc, under whichever name the caller's headers picked.
struct dirent64* readdir64(DIR* d)
{
    if (Merged(d)) return reinterpret_cast<struct dirent64*>(readdir(d));
    static auto real = RealFn<struct dirent64* (*)(DIR*)>("readdir64");
    return real(d);
}
#endif

int closedir(DIR* d)
{
    if (MergedDir* m = Merged(d)) {
        {
            std::lock_guard<std::mutex> g(s_dirLock);
            s_merged.erase(d);
        }
        delete m;
        return 0;
    }
    return RealClosedir(d);
}

} // extern "C"
#endif

namespace {
char* Resolve(const char* pszIn, char* pszOut, size_t nOut)
{
    std::string in(pszIn ? pszIn : "");
    for (char& c : in)
        if (c == '\\') c = '/';
    while (in.size() >= 2 && in[0] == '.' && in[1] == '/') in.erase(0, 2);

    // Fast path: it already exists as written. lstat, not stat: stat is overridden with a call
    // back into here.
    struct stat st;
    if (in.empty() || lstat(in.c_str(), &st) == 0) {
        snprintf(pszOut, nOut, "%s", in.c_str());
        return pszOut;
    }

    std::string built;
    size_t pos = 0;
    if (DrivePath(in.c_str())) {
        built = in.substr(0, 2) + "/";
        pos = in.find_first_not_of('/', 2);
        if (pos == std::string::npos) pos = in.size();
    } else if (in[0] == '/') {
        built = "/";
        pos = in.find_first_not_of('/');
        if (pos == std::string::npos) pos = in.size();
    }
    bool matching = true;   // stops once a component is missing: the rest is a new file's name
    while (pos < in.size()) {
        size_t end = in.find('/', pos);
        if (end == std::string::npos) end = in.size();
        std::string part = in.substr(pos, end - pos);
        if (matching && part != "." && part != "..") {
            DIR* d = RealOpendir(built.empty() ? "." : built.c_str());
            bool found = false;
            if (d) {
                while (struct dirent* e = RealReaddir(d)) {
                    if (strcasecmp(e->d_name, part.c_str()) == 0) {
                        part = e->d_name;
                        found = true;
                        break;
                    }
                }
                RealClosedir(d);
            }
            matching = found;
        }
        if (!built.empty() && built.back() != '/') built += '/';
        built += part;
        pos = in.find_first_not_of('/', end);
        if (pos == std::string::npos) break;
    }
    if (!in.empty() && in.back() == '/' && (built.empty() || built.back() != '/')) built += '/';
    snprintf(pszOut, nOut, "%s", built.c_str());
    return pszOut;
}
} // namespace

// K2_GL_TRACE also logs the state calls that decide where a draw lands: viewport, scissor, enables,
// clears.
namespace {
void (K2_GLAPI* s_pViewport)(int, int, int, int);
void (K2_GLAPI* s_pScissor)(int, int, int, int);
void (K2_GLAPI* s_pEnable)(unsigned int);
void (K2_GLAPI* s_pDisable)(unsigned int);
void (K2_GLAPI* s_pClear)(unsigned int);
void K2_GLAPI TraceViewport(int x, int y, int w, int h)
{
    // K2_GL_VPMAXW=N (a test): a viewport wider than N pixels is narrowed to N.
    static const int s_nMaxW = getenv("K2_GL_VPMAXW") ? atoi(getenv("K2_GL_VPMAXW")) : 0;
    if (s_nMaxW > 0 && w > s_nMaxW && w < 2000) w = s_nMaxW;
    // K2_GL_VPSWAP=x,y,w,h=X,Y,W,H (a test): that exact viewport becomes the other one.
    static int s_anSwap[8];
    static const bool s_bSwap = getenv("K2_GL_VPSWAP") &&
        sscanf(getenv("K2_GL_VPSWAP"), "%d,%d,%d,%d=%d,%d,%d,%d", &s_anSwap[0], &s_anSwap[1], &s_anSwap[2],
               &s_anSwap[3], &s_anSwap[4], &s_anSwap[5], &s_anSwap[6], &s_anSwap[7]) == 8;
    if (s_bSwap && x == s_anSwap[0] && y == s_anSwap[1] && w == s_anSwap[2] && h == s_anSwap[3]) {
        x = s_anSwap[4]; y = s_anSwap[5]; w = s_anSwap[6]; h = s_anSwap[7];
    }
    if (k2shim_gl_tracing) fprintf(stderr, "k2: trace: Viewport %d %d %d %d\n", x, y, w, h);
    s_pViewport(x, y, w, h);
}
void K2_GLAPI TraceScissor(int x, int y, int w, int h)
{
    if (k2shim_gl_tracing) fprintf(stderr, "k2: trace: Scissor %d %d %d %d\n", x, y, w, h);
    s_pScissor(x, y, w, h);
}
void K2_GLAPI TraceEnable(unsigned int e)
{
    if (k2shim_gl_tracing) fprintf(stderr, "k2: trace: Enable 0x%x\n", e);
    s_pEnable(e);
}
void K2_GLAPI TraceDisable(unsigned int e)
{
    if (k2shim_gl_tracing) fprintf(stderr, "k2: trace: Disable 0x%x\n", e);
    s_pDisable(e);
}
void K2_GLAPI TraceClear(unsigned int m)
{
    if (k2shim_gl_tracing) fprintf(stderr, "k2: trace: Clear 0x%x\n", m);
    s_pClear(m);
}
// Logs texture names every frame while tracing, so a name a draw binds can be traced to the object
// that owns it: a GL implementation hands a deleted name out again.
void (K2_GLAPI* s_pGenTextures)(int, unsigned int*);
void (K2_GLAPI* s_pDeleteTextures)(int, const unsigned int*);
void K2_GLAPI TraceGenTextures(int n, unsigned int* p)
{
    s_pGenTextures(n, p);
    const char* pszCaller = "?";
#ifndef _WIN32   // the caller's caller: a frame-pointer walk, which Win64 code does not keep
    Dl_info info;
    if (dladdr(__builtin_return_address(1), &info) && info.dli_sname)
        pszCaller = info.dli_sname;
#endif
    for (int i = 0; i < n; i++)
        fprintf(stderr, "k2: trace: GenTextures %u (%s)\n", p[i], pszCaller);
}
void K2_GLAPI TraceDeleteTextures(int n, const unsigned int* p)
{
    for (int i = 0; i < n; i++)
        fprintf(stderr, "k2: trace: DeleteTextures %u\n", p[i]);
    s_pDeleteTextures(n, p);
}
// Returns the traced wrapper for pszName, or real when it is not traced.
void* TraceProc(const char* pszName, void* real)
{
    static const bool s_bTrace = getenv("K2_GL_TRACE") != 0;
    if (!s_bTrace || !real) return real;
    struct { const char* name; void** slot; void* wrapper; } const k[] = {
        { "glViewport", reinterpret_cast<void**>(&s_pViewport), reinterpret_cast<void*>(TraceViewport) },
        { "glScissor", reinterpret_cast<void**>(&s_pScissor), reinterpret_cast<void*>(TraceScissor) },
        { "glEnable", reinterpret_cast<void**>(&s_pEnable), reinterpret_cast<void*>(TraceEnable) },
        { "glDisable", reinterpret_cast<void**>(&s_pDisable), reinterpret_cast<void*>(TraceDisable) },
        { "glClear", reinterpret_cast<void**>(&s_pClear), reinterpret_cast<void*>(TraceClear) },
        { "glGenTextures", reinterpret_cast<void**>(&s_pGenTextures), reinterpret_cast<void*>(TraceGenTextures) },
        { "glDeleteTextures", reinterpret_cast<void**>(&s_pDeleteTextures), reinterpret_cast<void*>(TraceDeleteTextures) },
    };
    for (const auto& e : k)
        if (strcmp(pszName, e.name) == 0) {
            *e.slot = real;
            return e.wrapper;
        }
    return real;
}
} // namespace

#ifndef K2P_MOBILE_HOST
// GL_CLAMP -- the engine's wrap mode for GUI and font textures -- is clamp to the border colour when
// the filter reaches past the edge, which NVIDIA does to the letter. Apple's GL, Mesa and gl4es read it
// as GL_CLAMP_TO_EDGE, which is what the art was made against; on NVIDIA the menu buttons, drawn from
// edge and corner pieces, showed seams.
namespace {
void* GlProc(const char* pszName);
void (*s_pTexParameteri)(unsigned int, unsigned int, int);
void (*s_pTexParameterf)(unsigned int, unsigned int, float);
int EdgeWrap(unsigned int ePname, int nValue)
{
    bool bWrap = ePname == 0x2802 || ePname == 0x2803 || ePname == 0x8072;   // WRAP_S / _T / _R
    return bWrap && nValue == 0x2900 ? 0x812F : nValue;                       // CLAMP -> CLAMP_TO_EDGE
}
void TexParameteri(unsigned int eTarget, unsigned int ePname, int nValue)
{
    s_pTexParameteri(eTarget, ePname, EdgeWrap(ePname, nValue));
}
void TexParameterf(unsigned int eTarget, unsigned int ePname, float fValue)
{
    s_pTexParameterf(eTarget, ePname, static_cast<float>(EdgeWrap(ePname, static_cast<int>(fValue))));
}
// Draws GL_QUAD_STRIP as the GL_TRIANGLE_STRIP of the same vertices, as gl4es does. How a driver
// splits each quad into two triangles is up to it, and the two diagonals disagree: the lightsaber's
// swing trail is non-planar zig-zag quads whose texture coordinates only match along one of them, so
// cutting the other way stretches the glow at the blade's ends across the trail.
void (*s_pBegin)(unsigned int);
void Begin(unsigned int eMode)
{
    s_pBegin(eMode == 0x0008 ? 0x0005 : eMode);   // GL_QUAD_STRIP -> GL_TRIANGLE_STRIP
}
} // namespace

void* k2shim_gl_proc(const char* pszName)
{
    if (strcmp(pszName, "glTexParameteri") == 0) {
        if (!s_pTexParameteri) s_pTexParameteri = reinterpret_cast<void (*)(unsigned int, unsigned int, int)>(GlProc(pszName));
        return s_pTexParameteri ? reinterpret_cast<void*>(TexParameteri) : 0;
    }
    if (strcmp(pszName, "glTexParameterf") == 0) {
        if (!s_pTexParameterf) s_pTexParameterf = reinterpret_cast<void (*)(unsigned int, unsigned int, float)>(GlProc(pszName));
        return s_pTexParameterf ? reinterpret_cast<void*>(TexParameterf) : 0;
    }
    if (strcmp(pszName, "glBegin") == 0) {
        if (!s_pBegin) s_pBegin = reinterpret_cast<void (*)(unsigned int)>(GlProc(pszName));
        return s_pBegin ? reinterpret_cast<void*>(Begin) : 0;
    }
    return TraceProc(pszName, k2shim_glpresent_proc(pszName, GlProc(pszName)));
}

namespace {
void* GlProc(const char* pszName)
#else
void* k2shim_gl_proc(const char* pszName)
#endif
{
    static const char* const kSuffixes[] = { "", "ARB", "EXT" };
    std::string base(pszName);
    // Strips a trailing ARB/EXT, so "glFooARB" also finds "glFoo" and "glFooEXT"
    for (const char* sfx : { "ARB", "EXT" }) {
        size_t n = strlen(sfx);
        if (base.size() > n && base.compare(base.size() - n, n, sfx) == 0) {
            base.resize(base.size() - n);
            break;
        }
    }
#ifdef K2P_MOBILE_HOST
    void* (*lookup)(const char*) = k2android_gl_proc;   // gl4es, never the raw GLES driver
#else
    void* (*lookup)(const char*) = SDL_GL_GetProcAddress;
#endif
#ifdef K2P_MOBILE_HOST
    if (void* p = lookup(pszName)) return TraceProc(pszName, p);
    for (const char* sfx : kSuffixes)
        if (void* p = lookup((base + sfx).c_str())) return TraceProc(pszName, p);
#else
    if (void* p = lookup(pszName)) return p;
    for (const char* sfx : kSuffixes)
        if (void* p = lookup((base + sfx).c_str())) return p;
#endif

    // Logs only once a context exists: before that every lookup fails and is retried.
    if (SDL_GL_GetCurrentContext()) {
        static std::mutex s_lock;
        static std::set<std::string> s_logged;
        std::lock_guard<std::mutex> g(s_lock);
        if (s_logged.insert(pszName).second)
            fprintf(stderr, "k2shim: GL entry point %s not available; calls are ignored\n", pszName);
    }
    return 0;
}
#ifndef K2P_MOBILE_HOST
} // namespace
#endif

// K2_GL_CHECK: logs the first error each entry point raises.
bool k2shim_gl_checking = getenv("K2_GL_CHECK") != 0;
bool k2shim_gl_tracing;
bool k2shim_gl_nocache = getenv("K2_GL_NOCACHE") != 0;   // no uniform cache

// Reports an enabled client array that has no buffer bound and a pointer that looks like a buffer
// offset: the draw reads it as client memory (NVIDIA faults, Apple's GL skips arrays the draw does
// not use).
static void CheckClientArrays(const char* name)
{
    typedef void (K2_GLAPI* GetIntegerv)(unsigned int, int*);
    typedef void (K2_GLAPI* GetPointerv)(unsigned int, void**);
    typedef unsigned char (K2_GLAPI* IsEnabled)(unsigned int);
    typedef void (K2_GLAPI* ClientActive)(unsigned int);
    typedef void (K2_GLAPI* GetAttribiv)(unsigned int, unsigned int, int*);
    typedef void (K2_GLAPI* GetAttribPtr)(unsigned int, unsigned int, void**);
    static GetIntegerv pGetI = reinterpret_cast<GetIntegerv>(k2shim_gl_proc("glGetIntegerv"));
    static GetPointerv pGetP = reinterpret_cast<GetPointerv>(k2shim_gl_proc("glGetPointerv"));
    static IsEnabled pIs = reinterpret_cast<IsEnabled>(k2shim_gl_proc("glIsEnabled"));
    static ClientActive pCAT = reinterpret_cast<ClientActive>(k2shim_gl_proc("glClientActiveTexture"));
    static GetAttribiv pAI = reinterpret_cast<GetAttribiv>(k2shim_gl_proc("glGetVertexAttribiv"));
    static GetAttribPtr pAP = reinterpret_cast<GetAttribPtr>(k2shim_gl_proc("glGetVertexAttribPointerv"));
    static std::set<std::string> reported;
    if (!pGetI || !pGetP || !pIs) return;
    auto bad = [&](const char* what, int unit, void* ptr) {
        char key[160];
        snprintf(key, sizeof key, "%s %s %d", name, what, unit);
        if (reported.insert(key).second)
            fprintf(stderr, "k2: %s with %s array %d enabled, no buffer bound, pointer %p\n",
                    name, what, unit, ptr);
    };
    struct { const char* what; unsigned int cap, binding, pointer; } fixed[] = {
        { "vertex", 0x8074, 0x8896, 0x808E }, { "normal", 0x8075, 0x8897, 0x808F },
        { "color", 0x8076, 0x8898, 0x8090 }, { "secondary color", 0x845E, 0x889C, 0x845D },
        { "fog coord", 0x8457, 0x889D, 0x8456 },
    };
    for (auto& f : fixed) {
        int nBuf = 0; void* ptr = 0;
        if (!pIs(f.cap)) continue;
        pGetI(f.binding, &nBuf);
        pGetP(f.pointer, &ptr);
        if (nBuf == 0 && reinterpret_cast<uintptr_t>(ptr) < 0x1000000) bad(f.what, 0, ptr);
    }
    if (pCAT) {
        int nActive = 0x84C0, nUnits = 0;
        pGetI(0x84E1, &nActive);
        pGetI(0x84E2, &nUnits);   // GL_MAX_TEXTURE_UNITS
        for (int i = 0; i < nUnits && i < 8; i++) {
            pCAT(0x84C0 + i);
            int nBuf = 0; void* ptr = 0;
            if (!pIs(0x8078)) continue;
            pGetI(0x889A, &nBuf);
            pGetP(0x8092, &ptr);
            if (nBuf == 0 && reinterpret_cast<uintptr_t>(ptr) < 0x1000000) bad("texcoord", i, ptr);
        }
        pCAT(nActive);
    }
    if (pAI && pAP)
        for (unsigned int i = 0; i < 16; i++) {
            int nOn = 0, nBuf = 0; void* ptr = 0;
            pAI(i, 0x8622, &nOn);   // GL_VERTEX_ATTRIB_ARRAY_ENABLED
            if (!nOn) continue;
            pAI(i, 0x889F, &nBuf);
            pAP(i, 0x8645, &ptr);
            if (nBuf == 0 && reinterpret_cast<uintptr_t>(ptr) < 0x1000000) bad("generic attrib", static_cast<int>(i), ptr);
        }
}

void k2shim_gl_unbind_buffers(int n, const unsigned int* ids)
{
    typedef void (K2_GLAPI* GetIntegerv)(unsigned int, int*);
    typedef unsigned char (K2_GLAPI* IsEnabled)(unsigned int);
    typedef void (K2_GLAPI* Cap)(unsigned int);
    typedef void (K2_GLAPI* GetAttribiv)(unsigned int, unsigned int, int*);
    static GetIntegerv pGetI = reinterpret_cast<GetIntegerv>(k2shim_gl_proc("glGetIntegerv"));
    static IsEnabled pIs = reinterpret_cast<IsEnabled>(k2shim_gl_proc("glIsEnabled"));
    static Cap pDisable = reinterpret_cast<Cap>(k2shim_gl_proc("glDisableClientState"));
    static Cap pCAT = reinterpret_cast<Cap>(k2shim_gl_proc("glClientActiveTexture"));
    static Cap pDisableAttrib = reinterpret_cast<Cap>(k2shim_gl_proc("glDisableVertexAttribArray"));
    static GetAttribiv pAI = reinterpret_cast<GetAttribiv>(k2shim_gl_proc("glGetVertexAttribiv"));
    if (!ids || n <= 0) return;
    k2shim_glarr_buffers_deleted(n, ids);
    if (!pGetI || !pIs) return;
    auto deleted = [&](int nBuf) {
        for (int i = 0; i < n; i++)
            if (nBuf != 0 && static_cast<unsigned int>(nBuf) == ids[i]) return true;
        return false;
    };
    auto check = [&](unsigned int cap, unsigned int binding) {
        int nBuf = 0;
        if (!pIs(cap)) return;
        pGetI(binding, &nBuf);
        if (deleted(nBuf) && pDisable) pDisable(cap);
    };
    check(0x8074, 0x8896);   // vertex
    check(0x8075, 0x8897);   // normal
    check(0x8076, 0x8898);   // colour
    check(0x845E, 0x889C);   // secondary colour
    check(0x8457, 0x889D);   // fog coordinate
    if (pCAT) {
        int nActive = 0x84C0, nUnits = 0;
        pGetI(0x84E1, &nActive);   // GL_CLIENT_ACTIVE_TEXTURE
        pGetI(0x84E2, &nUnits);    // GL_MAX_TEXTURE_UNITS
        for (int i = 0; i < nUnits && i < 8; i++) {
            pCAT(0x84C0 + i);
            check(0x8078, 0x889A);   // texture coordinates
        }
        pCAT(nActive);
    }
    if (pAI && pDisableAttrib)
        for (unsigned int i = 0; i < 16; i++) {
            int nOn = 0, nBuf = 0;
            pAI(i, 0x8622, &nOn);   // GL_VERTEX_ATTRIB_ARRAY_ENABLED
            if (!nOn) continue;
            pAI(i, 0x889F, &nBuf);  // GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
            if (deleted(nBuf)) pDisableAttrib(i);
        }
}

void k2shim_gl_after(const char* name)
{
    static bool bInBegin;
    static std::set<std::string> reported;
    if (strcmp(name, "glBegin") == 0) { bInBegin = true; return; }
    if (strcmp(name, "glEnd") == 0) bInBegin = false;
    if (bInBegin) return;   // glGetError is itself an error between glBegin and glEnd
    if (strncmp(name, "glDraw", 6) == 0 && strcmp(name, "glDrawBuffer") != 0)
        CheckClientArrays(name);
    typedef unsigned int (K2_GLAPI* GetError)(void);
    static GetError pfn;
    if (!pfn) pfn = reinterpret_cast<GetError>(k2shim_gl_proc("glGetError"));
    unsigned int e = pfn ? pfn() : 0;
    if (e != 0 && reported.insert(name).second) {
        fprintf(stderr, "k2: GL error 0x%x after %s\n", e, name);
        static const bool bTrap = strcmp(getenv("K2_GL_CHECK"), "trap") == 0;
        if (bTrap)
            __builtin_debugtrap();   // K2_GL_CHECK=trap stops in the debugger at the first error
    }
}
