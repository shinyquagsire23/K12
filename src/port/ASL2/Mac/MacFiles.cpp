// The ASL2 Mac file API, with the Win32 file wrappers (CreateFile, MapViewOfFile, the attribute
// cores) and the small HANDLE_Mac bridge classes. Nothing calls any of them in this build.
//
#include "k2stdint.h"
#include "MacFiles.h"

#include <sys/stat.h>

// The MSVC CRT's spelling of stat, which the port layer provides.
extern "C" int  _stat(const char* pszPath, struct stat* pStat) throw();
// The sibling below uses the 64-bit entry point instead, and the two disagree. Both spellings exist
// in the engine, so neither is the one true stat.
#if defined(K2_HOST_BUILD) && defined(__APPLE__)
// 64-bit hosts have only the 64-bit-inode `struct stat`; `stat64` is the same record under its old
// name. (bionic declares `struct stat64` itself, so the Android host build takes its own.)
struct stat64 : stat {};
extern "C" int stat64(const char* pszPath, struct stat64* pStat);
#endif

// The ASL2 time conversion. The engine mangles it as C++, so it must not be `extern "C"`.
void MacSecondsToFileTime(uint64_t nSeconds, struct _FILETIME* pFileTime) throw();

// Win32's file-time pair and the attribute-data record the info-level row fills.
#include "Win32Find.h"
#include "port/win32const.h"

struct WIN32_FILE_ATTRIBUTE_DATA {
    unsigned int dwFileAttributes;
    struct _FILETIME ftCreationTime;
    struct _FILETIME ftLastAccessTime;
    struct _FILETIME ftLastWriteTime;
    unsigned int nFileSizeHigh;
    unsigned int nFileSizeLow;
};

// An info level other than the only defined one is an error.
typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard = 0,
    GetFileExMaxInfoLevel  = 1
} GET_FILEEX_INFO_LEVELS;

// The Win32 last-error cell comes from the WinThreadsMP header, so it is not re-declared here.

// Win32 attribute bits, as `GetStatAttributes` / `GetFileAttributes_core` return them (the
// directory bit is Win32Find.h's).
enum {
    FILE_ATTRIBUTE_READONLY  = 0x1,
    FILE_ATTRIBUTE_NORMAL    = 0x80,
    FILE_ATTRIBUTE_INVALID   = static_cast<int>(0xFFFFFFFF),
};

// CreateFile's overlapped-IO flag (byte 3 of FileHANDLE::m_nFlags).
enum {
    FILE_FLAG_OVERLAPPED = 0x40000000,
};

// The port's file API.

// Deliberately empty. The shim is a bare return, and the parameter is never read.
void _com_issue_error(int64_t nError)
{
    K2_UNUSED(nError);
}

// The store sequence follows the member-init order.
FileHANDLE::FileHANDLE(int nFD, const uint64_t* pFile, unsigned int nAccess, unsigned int nFlags)
{
    m_nFD = nFD;
    m_pFile = 0;
    if (pFile != 0)
        m_pFile = reinterpret_cast<FILE*>(pFile[0]);
    m_nAccess = nAccess;
    m_nFlags = nFlags;
    m_nFileCursor = 0;
}

FileHANDLE::~FileHANDLE()
{
    if (m_pFile != 0 && fclose(m_pFile) != 0)
        perror("fclose");
}

// Never implemented on this platform; both parameters are unread.
bool FileHANDLE::GetFSRef(FSRef* pFSRef)
{
    K2_UNUSED(pFSRef);
    return false;
}

MacPendingIO::~MacPendingIO()
{
}

// `AddRef` is called on the argument rather than on `this`, which is why the pointer members are
// typed `HANDLE_Mac*`. The same pointer goes into both members, and the destructor reads them
// independently.
FileMappingHANDLE::FileMappingHANDLE(void* pUnk, unsigned int nProtect)
{
    m_pFileHandle = static_cast<FileHANDLE*>(pUnk);
    m_hFile = static_cast<HANDLE_Mac*>(pUnk);
    if (pUnk != 0)
        m_pFileHandle->AddRef();
    m_bMapped = 1;
    m_nProtect = nProtect;
    m_nMappingSize = 0;
    m_nMapFD = -1;
}

// Closes the mapping handle, then the base destructor runs.
FileMappingHANDLE::~FileMappingHANDLE()
{
    if (m_hFile != 0)
        m_hFile->CloseHandle();
}

// The two class destructors free their own storage, so neither class may be given an in-class
// destructor body.

// The Win32 CreateFileA core.

// Never read by this layer, which has no ACLs, but the tag must exist for CreateFileA's signature.
struct _SECURITY_ATTRIBUTES;

// The mode string is built by appending into an empty `std::string`: the accepted access groups map
// to "r", "w+" or "w", and anything unrecognised fails. The access value is compared as signed,
// which routes the two high-bit constants into the read group.
#include "port/libcbase.h"

HANDLE_Mac* CreateFile_core(const char* pszFileName, unsigned int dwDesiredAccess,
                            unsigned int dwShareMode, struct _SECURITY_ATTRIBUTES* pSecurityAttributes,
                            unsigned int dwCreationDisposition, unsigned int dwFlagsAndAttributes,
                            void* hTemplateFile)
{
    K2_UNUSED(dwShareMode); K2_UNUSED(pSecurityAttributes); K2_UNUSED(dwCreationDisposition); K2_UNUSED(hTemplateFile);
    std::string sMode;
    HANDLE_Mac* pHandle = reinterpret_cast<HANDLE_Mac*>(-1);
    switch (static_cast<int>(dwDesiredAccess)) {
    case static_cast<int>(GENERIC_READ):
    case FILE_READ_DATA:
    case FILE_READ_ATTRIBUTES:
        sMode.append("r", 1);
        break;
    case static_cast<int>(GENERIC_READ | GENERIC_WRITE):
        sMode.append("w+", 2);
        break;
    case FILE_WRITE_DATA:
    case FILE_WRITE_ATTRIBUTES:
    case GENERIC_WRITE:
        sMode.append("w", 1);
        break;
    default:
        return pHandle;
    }
    FILE* pStream = fopen(pszFileName, sMode.c_str());
    if (pStream == 0) {
        pHandle = reinterpret_cast<HANDLE_Mac*>(-1);
        return pHandle;
    }
    int nFD = fileno(pStream);
    pHandle = new FileHANDLE(nFD, reinterpret_cast<const uint64_t*>(&pStream), dwDesiredAccess, dwFlagsAndAttributes);
    return pHandle;
}

// Free functions.

// The Win32 MapViewOfFile core.

// The view registry, a file-scope `std::map` keyed by the view pointer, holding the mapping that
// owns it.
std::map<void*, void*> sFileMappings;

// The ASL2 file-size helper used below is the out-of-scope `GetFileSize` (port/win32api.h), not
// this unit's `GetFileSize_Linux`; the callers pass a null high-dword pointer and use the result 32
// bits wide.
#include "port/win32api.h"

// Parameter 1 is really a `FileMappingHANDLE*`: it carries the `FileHANDLE`, the "already mapped"
// byte and the cached mapping size. An already-mapped handle recomputes the size when the requested
// size is zero and returns null; otherwise the view is carved out of a buffer that stores the owner
// in its first eight bytes, the file is seeked to its saved cursor and read, the cursor advances,
// and the view is registered in `sFileMappings`.
void* MapViewOfFile(void* pMappingVoid, unsigned int dwDesiredAccess, unsigned int dwFileOffsetHigh,
                    unsigned int dwFileOffsetLow, unsigned int dwNumberOfBytesToMap)
{
    K2_UNUSED(dwDesiredAccess); K2_UNUSED(dwFileOffsetHigh);
    FileMappingHANDLE* pMapping = static_cast<FileMappingHANDLE*>(pMappingVoid);
    if (pMapping->m_bMapped != 0) {
        if (dwNumberOfBytesToMap == 0)
            dwNumberOfBytesToMap = static_cast<unsigned int>(GetFileSize(pMapping->m_pFileHandle, 0) - dwFileOffsetLow);
        pMapping->m_nMappingSize = dwNumberOfBytesToMap;
        return 0;
    }
    if (dwNumberOfBytesToMap == 0)
        dwNumberOfBytesToMap = static_cast<unsigned int>(GetFileSize(pMapping->m_pFileHandle, 0));
    pMapping->m_nMappingSize = dwNumberOfBytesToMap;
    unsigned char* pBuffer = new unsigned char[dwNumberOfBytesToMap + 8];
    *reinterpret_cast<FileMappingHANDLE**>(pBuffer) = pMapping;
    unsigned char* pView = pBuffer + 8;
    if (pMapping->m_pFileHandle == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        delete[] pBuffer;
        return 0;
    }
    pMapping->m_pFileHandle->m_nFileCursor = 0;
    SetLastError(0);
    FileHANDLE* pFile = pMapping->m_pFileHandle;
    if (pFile == 0 || (pFile->m_abFlags[3] & (FILE_FLAG_OVERLAPPED >> 24)) != 0 || pFile->m_pFile == 0) {
        delete[] pBuffer;
        return 0;
    }
    FILE* pStream = pFile->m_pFile;
    fseek(pStream, static_cast<long>(pFile->m_nFileCursor), 0);
    size_t nRead = static_cast<size_t>(fread(pView, 1, dwNumberOfBytesToMap, pStream));
    if (dwNumberOfBytesToMap != 0 && nRead == 0) {
        delete[] pBuffer;
        return 0;
    }
    pFile->m_nFileCursor += static_cast<unsigned int>(nRead);
    sFileMappings[pView] = pMapping;
    return pView;
}

// Save and restore around a seek to the end; both failure sinks share one exit.
int64_t GetFileSize_Linux(FileHANDLE* pFile)
{
    FILE* pStream = pFile->m_pFile;
    int64_t nSize;
    if (pStream == 0) {
        nSize = -1;
    } else {
        int64_t nOld = ftell(pStream);
        fseek(pStream, 0, 2);
        nSize = ftell(pStream);
        fseek(pStream, static_cast<long>(nOld), 0);
    }
    return nSize;
}

// The size of an open stream; the read position is restored, and any failing call gives -1.
int FileSize(FILE* pFile)
{
    int nPos = ftell(pFile);
    if (nPos == -1 || fseek(pFile, 0, 2) != 0)
        return -1;
    int nSize = ftell(pFile);
    if (nSize == -1)
        return -1;
    return fseek(pFile, nPos, 0) == 0 ? nSize : -1;
}

// Reports a directory, or a normal file, and a failure when the path cannot be stat'ed.
unsigned int GetFileAttributes_core(const FSRef* pFSRef)
{
    struct stat st;
    if (pFSRef != 0) {
        if (_stat(pFSRef->m_sPath.c_str(), &st) != -1)
            return (st.st_mode & S_IFMT) == S_IFDIR ? FILE_ATTRIBUTE_DIRECTORY
                                                    : FILE_ATTRIBUTE_NORMAL;
        SetLastError(2);
    }
    return static_cast<unsigned int>(FILE_ATTRIBUTE_INVALID);
}

// Fills a WIN32_FILE_ATTRIBUTE_DATA. All four failure exits share one epilogue. The attribute word
// is a nested ternary over the mode's type and owner-write bits, and the creation time comes from
// the change time rather than the modification time.
int GetFileAttributesEx_core(const FSRef* pFSRef, _GET_FILEEX_INFO_LEVELS eInfoLevel, void* pInfo)
{
    if (eInfoLevel != 0) {
        SetLastError(ERROR_GEN_FAILURE);
        return 0;
    }
    if (pInfo == 0) {
        SetLastError(ERROR_GEN_FAILURE);
        return 0;
    }
    struct stat64 st;
    if (stat64(pFSRef->m_sPath.c_str(), &st) != 0) {
        SetLastError(ERROR_GEN_FAILURE);
        return 0;
    }
    WIN32_FILE_ATTRIBUTE_DATA data;
    // S_IFDIR shifted right ten bits lands on FILE_ATTRIBUTE_DIRECTORY.
    data.dwFileAttributes = (st.st_mode & (S_IFREG | S_IRWXU)) != 0
                          ? FILE_ATTRIBUTE_NORMAL
                          : ((st.st_mode & S_IWUSR) == 0 ? (((st.st_mode >> 10) & FILE_ATTRIBUTE_DIRECTORY) | FILE_ATTRIBUTE_READONLY)
                                                       : ((st.st_mode >> 10) & FILE_ATTRIBUTE_DIRECTORY));
    MacSecondsToFileTime(st.st_ctime, &data.ftCreationTime);
    MacSecondsToFileTime(st.st_atime, &data.ftLastAccessTime);
    MacSecondsToFileTime(st.st_mtime, &data.ftLastWriteTime);
    data.nFileSizeHigh = static_cast<unsigned int>(static_cast<unsigned long long>(st.st_size) >> 32);
    data.nFileSizeLow = static_cast<unsigned int>(st.st_size);
    *static_cast<WIN32_FILE_ATTRIBUTE_DATA*>(pInfo) = data;
    SetLastError(0);
    return 1;
}

// Reports a directory or a normal file.
unsigned int GetStatAttributes(struct stat* pStat)
{
    return (pStat->st_mode & S_IFMT) == S_IFDIR ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

// Unimplemented stubs; none of the parameters is read.
int SetFileAttributes_core(const FSRef* pFSRef, unsigned int nAttributes)
{
    K2_UNUSED(pFSRef); K2_UNUSED(nAttributes);
    return 0;
}

int PathIsDirectoryEmpty_core(const FSRef* pFSRef, bool bPathIsRooted)
{
    K2_UNUSED(pFSRef); K2_UNUSED(bPathIsRooted);
    return 0;
}

// XInput's DirectSound device-id probe.

// The null device id is 24 bytes wide here, not the 16 of a Win32 id, because every use copies that
// much, so modelling it at 16 would truncate. The extra 8 bytes are zeros like the rest of the
// object.
struct _GUID {
    unsigned int Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
    unsigned int m_anReserved1[2];
};

_GUID GUID_NULL;

// The two file-scope statics the probe below reaches, the mutex and the four-entry slot table. The
// hidden visibility is required, since with default visibility both would be reached through the
// GOT instead of directly.
namespace ASL {
    namespace XInput {
        struct ControllerSlot {
            void* m_pDevice;
            unsigned char m_abReserved1[0x10];
        };
        __attribute__((visibility("hidden"))) std::mutex m_Lock;
        __attribute__((visibility("hidden"))) ControllerSlot m_aSlots[4];
    }
}

// Probes the slot under the lock. With no device in the slot it reports ERROR_DEVICE_NOT_CONNECTED
// and writes nothing; otherwise it stamps both out-GUIDs with `GUID_NULL`, each behind its own null
// test, and reports 0. The zero assignment between the two writes is load-bearing.
unsigned int XInputGetDSoundAudioDeviceGuids(unsigned int uIndex, _GUID* pRenderGuid,
                                             _GUID* pCaptureGuid)
{
    ASL::XInput::m_Lock.lock();
    unsigned int nResult;
    if (ASL::XInput::m_aSlots[uIndex].m_pDevice == 0) {
        nResult = ERROR_DEVICE_NOT_CONNECTED;
    } else {
        if (pRenderGuid != 0)
            *pRenderGuid = GUID_NULL;
        nResult = 0;
        if (pCaptureGuid != 0)
            *pCaptureGuid = GUID_NULL;
    }
    ASL::XInput::m_Lock.unlock();
    return nResult;
}
