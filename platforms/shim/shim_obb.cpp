// shim_obb.cpp -- Aspyr's ASL::FsApi::Obb: the APK expansion files as a read-only layer under the
// working directory.
//
// The Android engine never extracts its OBBs. SDL_main calls ASL::FsApi::Obb::initVfs, which opens
// main.<v>.<pkg>.obb and then patch.<v>.<pkg>.obb and registers every file in each index under its
// relative path, the later mount winning. The libc overrides here then try the native file first and
// fall back to that index, so the writable working directory (the app's external files dir: saves/,
// swkotor2.ini) shadows the read-only OBB.
//
// The format is a zlib'd index at the tail with every entry stored raw; a read is a pread at the
// entry's offset. Every platform mounts the OBBs from K2_OBB_DIR, which obbfiles.h uses to pick the
// game's: Android's obb dir, iOS's Documents, the desktop's environment.
#include <stdint.h>
#include "k2shim.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Entry {
    int fd;
    uint64_t off, size;
};

std::unordered_map<std::string, Entry> g_files;                 // key: lower-case relative path
std::map<std::string, std::map<std::string, std::string>> g_dirs;   // lower dir -> lower name -> name
struct Stream {
    int fd;
    uint64_t off, size, pos;
};
std::mutex g_openLock;
std::map<FILE*, Stream*> g_open;                                 // live OBB streams (GetFileSize)

int Read(void* c, char* buf, int n)
{
    Stream* s = static_cast<Stream*>(c);
    if (n <= 0 || s->pos >= s->size) return 0;
    uint64_t left = s->size - s->pos;
    size_t want = static_cast<uint64_t>(n) < left ? static_cast<size_t>(n) : static_cast<size_t>(left);
    ssize_t got = pread(s->fd, buf, want, static_cast<off_t>(s->off + s->pos));
    if (got < 0) return -1;
    s->pos += static_cast<uint64_t>(got);
    return static_cast<int>(got);
}

#if defined(__GLIBC__)
// glibc has no funopen: the same three behind fopencookie.
ssize_t CookieRead(void* c, char* buf, size_t n) { return Read(c, buf, n > 0x7fffffff ? 0x7fffffff : static_cast<int>(n)); }
int CookieSeek(void* c, off64_t* pOff, int whence)
{
    Stream* s = static_cast<Stream*>(c);
    int64_t base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? static_cast<int64_t>(s->pos) : static_cast<int64_t>(s->size);
    int64_t to = base + static_cast<int64_t>(*pOff);
    if (to < 0) return -1;
    s->pos = static_cast<uint64_t>(to);
    *pOff = to;
    return 0;
}
#else
fpos_t Seek(void* c, fpos_t off, int whence)
{
    Stream* s = static_cast<Stream*>(c);
    int64_t base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? static_cast<int64_t>(s->pos) : static_cast<int64_t>(s->size);
    int64_t to = base + static_cast<int64_t>(off);
    if (to < 0) return -1;
    s->pos = static_cast<uint64_t>(to);
    return static_cast<fpos_t>(to);
}
#endif

int Close(void* c)
{
    {
        std::lock_guard<std::mutex> g(g_openLock);
        for (auto it = g_open.begin(); it != g_open.end(); ++it)
            if (it->second == c) {
                g_open.erase(it);
                break;
            }
    }
    delete static_cast<Stream*>(c);
    return 0;
}

uint64_t U64(const unsigned char* p)
{
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = v << 8 | p[i];
    return v;
}

std::string Lower(std::string s)
{
    for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

void AddDirs(const std::string& path)
{
    // "a/b/c.txt" -> g_dirs[""]["a"], g_dirs["a"]["b"], g_dirs["a/b"]["c.txt"]
    size_t start = 0;
    std::string lowerPath = Lower(path);
    while (true) {
        size_t slash = path.find('/', start);
        std::string parent = start ? lowerPath.substr(0, start - 1) : std::string();
        std::string name = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        g_dirs[parent].emplace(Lower(name), name);
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
}

// Turns an engine/host path into the index key, with '/' separators, no "." or empty components,
// relative to the working directory (an absolute path under it is made relative), in lower case.
bool Key(const char* pszPath, std::string& key)
{
    if (!pszPath || g_files.empty()) return false;
    std::string p(pszPath);
    for (char& c : p)
        if (c == '\\') c = '/';
    if (!p.empty() && p[0] == '/') {
        char cwd[1024];
        if (!getcwd(cwd, sizeof cwd)) return false;
        size_t n = strlen(cwd);
        if (p.compare(0, n, cwd) != 0 || (p.size() > n && p[n] != '/')) return false;
        p.erase(0, n);
    }
    key.clear();
    size_t pos = 0;
    while (pos <= p.size()) {
        size_t end = p.find('/', pos);
        if (end == std::string::npos) end = p.size();
        std::string part = p.substr(pos, end - pos);
        if (part == "..") return false;
        if (!part.empty() && part != ".") {
            if (!key.empty()) key += '/';
            key += part;
        }
        pos = end + 1;
    }
    key = Lower(key);
    return true;
}

} // namespace

bool k2shim_obb_mount(const char* pszPath)
{
    int fd = open(pszPath, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "k2: obb %s: cannot open\n", pszPath);
        return false;
    }
    struct stat st;
    unsigned char tail[16];
    if (fstat(fd, &st) != 0 || st.st_size < 16 || pread(fd, tail, 16, st.st_size - 16) != 16) {
        fprintf(stderr, "k2: obb %s: not an OBB\n", pszPath);
        close(fd);
        return false;
    }
    uint64_t begin = U64(tail), usize = U64(tail + 8);
    if (begin >= static_cast<uint64_t>(st.st_size) - 16 || usize > (1u << 30)) {
        fprintf(stderr, "k2: obb %s: bad index\n", pszPath);
        close(fd);
        return false;
    }
    std::vector<unsigned char> packed(static_cast<size_t>(st.st_size - 16 - begin)), index(static_cast<size_t>(usize));
    uLongf outLen = static_cast<uLongf>(usize);
    if (pread(fd, packed.data(), packed.size(), static_cast<off_t>(begin)) != static_cast<ssize_t>(packed.size())
        || uncompress(index.data(), &outLen, packed.data(), static_cast<uLong>(packed.size())) != Z_OK || outLen != usize) {
        fprintf(stderr, "k2: obb %s: index does not inflate\n", pszPath);
        close(fd);
        return false;
    }
    // u64 count; count x { u64 len; char path[len]; u64 offset; u64 usize; u64 csize }
    const unsigned char* p = index.data();
    const unsigned char* end = p + index.size();
    uint64_t n = U64(p), nAdded = 0, nSkipped = 0;
    p += 8;
    for (uint64_t i = 0; i < n && p + 8 <= end; i++) {
        uint64_t len = U64(p);
        p += 8;
        if (p + len + 24 > end) break;
        std::string path(reinterpret_cast<const char*>(p), static_cast<size_t>(len));
        p += len;
        uint64_t off = U64(p), us = U64(p + 8), cs = U64(p + 16);
        p += 24;
        if (us != cs) {   // ChunkCompressed, which KOTOR II's OBBs do not use
            nSkipped++;
            continue;
        }
        g_files[Lower(path)] = Entry{ fd, off, us };
        AddDirs(path);
        nAdded++;
    }
    fprintf(stderr, "k2: obb %s: %llu files%s\n", pszPath, static_cast<unsigned long long>(nAdded),
            nSkipped ? " (chunk-compressed entries skipped)" : "");
    return nAdded != 0;
}

FILE* k2shim_obb_fopen(const char* pszPath, const char* pszMode)
{
    if (!pszMode || strpbrk(pszMode, "wa+")) return 0;   // the OBB is read-only
    std::string key;
    if (!Key(pszPath, key)) return 0;
    auto it = g_files.find(key);
    if (it == g_files.end()) return 0;
    Stream* s = new Stream{ it->second.fd, it->second.off, it->second.size, 0 };
#if defined(_WIN32)
    // The UCRT has no stream of our own making, so the entry is copied into a temporary file, whose
    // size is the file's own (k2shim_obb_size does not need to know it).
    FILE* f = tmpfile();
    char buf[65536];
    for (int got; f && (got = Read(s, buf, sizeof buf)) != 0;)
        if (got < 0 || fwrite(buf, 1, static_cast<size_t>(got), f) != static_cast<size_t>(got)) {
            fclose(f);
            f = 0;
        }
    delete s;
    if (f) rewind(f);
    return f;
#elif defined(__GLIBC__)
    cookie_io_functions_t io = { CookieRead, 0, CookieSeek, Close };
    FILE* f = fopencookie(s, "r", io);
#else
    FILE* f = funopen(s, Read, 0, Seek, Close);
#endif
    if (!f) {
        delete s;
        return 0;
    }
    std::lock_guard<std::mutex> g(g_openLock);
    g_open[f] = s;
    return f;
}

bool k2shim_obb_size(FILE* f, uint64_t* pSize)
{
    std::lock_guard<std::mutex> g(g_openLock);
    auto it = g_open.find(f);
    if (it == g_open.end()) return false;
    *pSize = it->second->size;
    return true;
}

bool k2shim_obb_stat(const char* pszPath, struct stat* pSt)
{
    std::string key;
    if (!Key(pszPath, key)) return false;
    memset(pSt, 0, sizeof *pSt);
    auto it = g_files.find(key);
    if (it != g_files.end()) {
        pSt->st_mode = S_IFREG | 0444;
        pSt->st_size = static_cast<off_t>(it->second.size);
        pSt->st_nlink = 1;
        return true;
    }
    if (g_dirs.count(key)) {
        pSt->st_mode = S_IFDIR | 0555;
        pSt->st_nlink = 2;
        return true;
    }
    return false;
}

void k2shim_obb_list(const char* pszDir, std::vector<std::string>& names)
{
    std::string key;
    if (!Key(pszDir ? pszDir : "", key)) return;
    auto it = g_dirs.find(key);
    if (it == g_dirs.end()) return;
    for (const auto& e : it->second) names.push_back(e.second);
}
