// Win32 files, directories and system information on POSIX.
#include <stdint.h>
#include "k2shim.h"

#include "MacFiles.h"
#include "Win32Find.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <string>
#include <vector>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace {

static_assert(sizeof(WIN32_FIND_DATAA) == 0x140 && offsetof(WIN32_FIND_DATAA, cFileName) == 0x2c,
              "WIN32_FIND_DATAA");

struct FindHandle {
    uint32_t magic;
    std::vector<std::string> names, paths;   // each match and the host path it was found at
    size_t next;
};
const uint32_t kFindMagic = 0x4b32464e;

void TimeToFileTime(time_t t, FILETIME& ft)
{
    uint64_t v = 116444736000000000ull + static_cast<uint64_t>(t) * 10000000ull;
    ft.dwLowDateTime = static_cast<uint32_t>(v);
    ft.dwHighDateTime = static_cast<uint32_t>(v >> 32);
}

// Case-insensitive DOS wildcard match: '*' any run, '?' one character. "*.*" matches everything,
// dotless names included, as on Windows.
bool WildMatch(const char* pat, const char* s)
{
    if (!strcmp(pat, "*.*")) return true;
    while (*pat) {
        if (*pat == '*') {
            while (*pat == '*') ++pat;
            if (!*pat) return true;
            for (; *s; ++s)
                if (WildMatch(pat, s)) return true;
            return false;
        }
        if (!*s) return false;
        if (*pat != '?' && tolower(static_cast<unsigned char>(*pat)) != tolower(static_cast<unsigned char>(*s))) return false;
        ++pat;
        ++s;
    }
    return !*s;
}

bool Fill(FindHandle* h, WIN32_FIND_DATAA* fd)
{
    while (h->next < h->names.size()) {
        const std::string& n = h->names[h->next];
        const std::string& full = h->paths[h->next++];
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        memset(fd, 0, sizeof *fd);
        fd->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0x80;   // or NORMAL
        TimeToFileTime(st.st_ctime, fd->ftCreationTime);
        TimeToFileTime(st.st_atime, fd->ftLastAccessTime);
        TimeToFileTime(st.st_mtime, fd->ftLastWriteTime);
        fd->nFileSizeHigh = static_cast<uint32_t>(static_cast<uint64_t>(st.st_size) >> 32);
        fd->nFileSizeLow = static_cast<uint32_t>(st.st_size);
        snprintf(fd->cFileName, sizeof fd->cFileName, "%s", n.c_str());
        return true;
    }
    return false;
}

std::string HostPath(const char* p)
{
    char buf[1024];
    return k2shim_host_path(p, buf, sizeof buf);
}

bool Fail(int e)
{
    SetLastError(k2shim_errno_to_win32(e));
    return false;
}

} // namespace

K2_WIN32_API_BEGIN

void* FindFirstFileA(const char* pszPattern, WIN32_FIND_DATAA* pData) throw()
{
    std::string pat(pszPattern ? pszPattern : "");
    for (char& c : pat)
        if (c == '\\') c = '/';
    size_t slash = pat.rfind('/');
    std::string dirPart = slash == std::string::npos ? "" : pat.substr(0, slash);
    std::string namePat = slash == std::string::npos ? pat : pat.substr(slash + 1);

    FindHandle* h = new FindHandle;
    h->magic = kFindMagic;
    h->next = 0;
    // The user directory's and the mounts' copies (shim_path.cpp), the working directory's, then what
    // the OBBs add under the same directory: the first entry of a name wins.
    std::vector<std::string> upper;
    std::string install;
    k2shim_layer_dirs(dirPart.empty() ? "." : dirPart.c_str(), upper, install);
    std::string dir = dirPart.empty() ? "" : !install.empty() ? install : HostPath(dirPart.c_str());
    std::vector<std::string> all, at;
    auto add = [&](const std::vector<std::string>& names, const std::string& base) {
        size_t nHave = all.size();
        for (const std::string& n : names) {
            bool dup = false;
            for (size_t i = 0; i < nHave && !dup; ++i) dup = strcasecmp(all[i].c_str(), n.c_str()) == 0;
            if (dup) continue;
            all.push_back(n);
            at.push_back(base.empty() || base == "." ? n : base + "/" + n);
        }
    };
    for (const std::string& dir : upper) {
        std::vector<std::string> names;
        k2shim_list_dir(dir.c_str(), names);
        add(names, dir);
    }
    std::vector<std::string> native, obb;
    k2shim_list_dir(dir.empty() ? "." : dir.c_str(), native);
    add(native, dir);
    k2shim_obb_list(dir.c_str(), obb);
    add(obb, dir);
    for (size_t i = 0; i < all.size(); ++i)
        if (WildMatch(namePat.c_str(), all[i].c_str())) {
            h->names.push_back(all[i]);
            h->paths.push_back(at[i]);
        }
    if (!pData || !Fill(h, pData)) {
        delete h;
        SetLastError(K2_ERROR_FILE_NOT_FOUND);
        return reinterpret_cast<void*>(-1);   // INVALID_HANDLE_VALUE
    }
    return h;
}

// End of the listing has to be ERROR_NO_MORE_FILES: the engine's scan loops test exactly that.
int FindNextFileA(void* hFind, WIN32_FIND_DATAA* pData) throw()
{
    FindHandle* h = static_cast<FindHandle*>(hFind);
    if (!h || hFind == reinterpret_cast<void*>(-1) || h->magic != kFindMagic) {
        SetLastError(K2_ERROR_INVALID_HANDLE);
        return 0;
    }
    if (!pData || !Fill(h, pData)) {
        SetLastError(K2_ERROR_NO_MORE_FILES);
        return 0;
    }
    return 1;
}

int FindClose(void* hFind) throw()
{
    FindHandle* h = static_cast<FindHandle*>(hFind);
    if (!h || hFind == reinterpret_cast<void*>(-1) || h->magic != kFindMagic) {   // the engine closes INVALID_HANDLE
        SetLastError(K2_ERROR_INVALID_HANDLE);
        return 0;
    }
    h->magic = 0;
    delete h;
    return 1;
}

int CreateDirectoryA(const char* pszPath, void*)
{
    if (mkdir(HostPath(pszPath).c_str(), 0755) == 0) return 1;
    return Fail(errno);
}

int RemoveDirectoryA(const char* pszPath)
{
    if (rmdir(HostPath(pszPath).c_str()) == 0) return 1;
    return Fail(errno);
}

int DeleteFileA(const char* pszPath)
{
    if (unlink(HostPath(pszPath).c_str()) == 0) return 1;
    return Fail(errno);
}

uint64_t GetCurrentDirectoryA(uint64_t nSize, char* pszBuf)
{
    char cwd[1024];
    if (!getcwd(cwd, sizeof cwd)) return 0;
    size_t n = strlen(cwd);
    if (n + 1 > nSize) return n + 1;
    memcpy(pszBuf, cwd, n + 1);
    return n;
}

int SetCurrentDirectoryA(const char* pszPath) throw()
{
    if (chdir(HostPath(pszPath).c_str()) == 0) return 1;
    return Fail(errno);
}

int GetDiskFreeSpaceExA(const char* pszPath, unsigned long long* pFreeAvail, unsigned long long* pTotal,
                        unsigned long long* pTotalFree)
{
    struct statvfs v;
    std::string p = pszPath ? HostPath(pszPath) : ".";
    if (statvfs(p.empty() ? "." : p.c_str(), &v) != 0 && statvfs(".", &v) != 0) return Fail(errno);
    if (pFreeAvail) *pFreeAvail = static_cast<unsigned long long>(v.f_bavail) * v.f_frsize;
    if (pTotal) *pTotal = static_cast<unsigned long long>(v.f_blocks) * v.f_frsize;
    if (pTotalFree) *pTotalFree = static_cast<unsigned long long>(v.f_bfree) * v.f_frsize;
    return 1;
}

// CheckForCD probes drive letters and acts only on DRIVE_CDROM (5): report "no such root" (1).
unsigned int GetDriveTypeA(const char*) throw() { return 1; }

unsigned int GetFileSize(FileHANDLE* pFile, unsigned int* pnSizeHigh) throw()
{
    uint64_t nObb;
    if (pFile && pFile->m_pFile && k2shim_obb_size(pFile->m_pFile, &nObb)) {   // an OBB stream
        if (pnSizeHigh) *pnSizeHigh = static_cast<unsigned int>(nObb >> 32);
        return static_cast<unsigned int>(nObb);
    }
    struct stat st;
    if (!pFile || fstat(pFile->m_pFile ? fileno(pFile->m_pFile) : pFile->m_nFD, &st) != 0) {
        SetLastError(K2_ERROR_INVALID_HANDLE);
        return 0xffffffffu;
    }
    if (pnSizeHigh) *pnSizeHigh = static_cast<unsigned int>(static_cast<uint64_t>(st.st_size) >> 32);
    return static_cast<unsigned int>(st.st_size);
}

// The launcher/relaunch path; there is nothing to launch on a host build.
int CreateProcessA(char*, char*, void*, void*, int, unsigned int, void*, char*, _STARTUPINFOA*,
                   _PROCESS_INFORMATION*)
{
    SetLastError(K2_ERROR_FILE_NOT_FOUND);
    return 0;
}

int IsBadWritePtr(const void* p, int) { return p == 0; }
int IsDBCSLeadByte(unsigned char) throw() { return 0; }

// SYSTEM_INFO: the engine hashes it as RNG entropy only, so the Win64 layout, filled the way Aspyr's
// macOS build fills it, is all it needs.
namespace {
struct Win64SystemInfo {
    uint32_t dwOemId;
    uint32_t dwPageSize;
    uint64_t lpMinimumApplicationAddress;
    uint64_t lpMaximumApplicationAddress;
    uint64_t dwActiveProcessorMask;
    uint32_t dwNumberOfProcessors;
    uint32_t dwProcessorType;
    uint32_t dwAllocationGranularity;
    uint16_t wProcessorLevel;
    uint16_t wProcessorRevision;
};
static_assert(sizeof(Win64SystemInfo) == 0x30, "Win64's SYSTEM_INFO");
}

void GetSystemInfo(_SYSTEM_INFO* pInfo)
{
    Win64SystemInfo* p = reinterpret_cast<Win64SystemInfo*>(pInfo);   // the engine only forward-declares it
    memset(p, 0, sizeof(*p));
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    p->dwPageSize = 0x1000;
    p->lpMinimumApplicationAddress = 0x10000;
    p->lpMaximumApplicationAddress = 0x7ffeffff;
    p->dwActiveProcessorMask = 1;
    p->dwNumberOfProcessors = static_cast<uint32_t>(ncpu > 0 ? ncpu : 1);
    p->dwProcessorType = 0x24a;
    p->dwAllocationGranularity = 0x1000;
    p->wProcessorLevel = 6;
    p->wProcessorRevision = 0x803;
}

// MEMORYSTATUS: zeros everything past dwLength, as Aspyr's build does. The engine truncates
// dwTotalPhys to int, so a real figure would scramble its memory budget.
static_assert(sizeof(MEMORYSTATUS) == 0x38, "MEMORYSTATUS");
void GlobalMemoryStatus(_MEMORYSTATUS* pStatus)
{
    memset(pStatus, 0, sizeof *pStatus);
    pStatus->dwLength = sizeof *pStatus;
}

// FORMAT_MESSAGE_ALLOCATE_BUFFER: the engine never initializes *ppBuffer and hands it to MessageBoxA
// and LocalFree, so it always receives a freeable string.
unsigned int FormatMessageA(unsigned int, const void*, unsigned int dwMessageId, unsigned int,
                            char** ppBuffer, unsigned int, void*)
{
    char msg[64];
    snprintf(msg, sizeof msg, "Win32 error %u", dwMessageId);
    if (ppBuffer) *ppBuffer = strdup(msg);
    return static_cast<unsigned int>(strlen(msg));
}

void* LocalFree(void* hMem)
{
    free(hMem);
    return 0;
}
K2_WIN32_API_END
