// The Win32 directory-scan surface the port shim exposes, the find-data structs plus the four
// find-file entry points.

#ifndef WIN32FIND_H
#define WIN32FIND_H

struct _FILETIME {
    unsigned int dwLowDateTime;
    unsigned int dwHighDateTime;
};
typedef _FILETIME FILETIME;

struct _WIN32_FIND_DATAA {
    unsigned int dwFileAttributes;
    FILETIME     ftCreationTime;
    FILETIME     ftLastAccessTime;
    FILETIME     ftLastWriteTime;
    unsigned int nFileSizeHigh;
    unsigned int nFileSizeLow;
    unsigned int dwReserved0;
    unsigned int dwReserved1;
    char         cFileName[260];
    char         cAlternateFileName[14];
};
typedef _WIN32_FIND_DATAA WIN32_FIND_DATAA;
static_assert(sizeof(FILETIME) == 8 && sizeof(WIN32_FIND_DATAA) == 0x140, "Win32's find records");

enum {
    ERROR_NO_MORE_FILES = 18,
    FILE_ATTRIBUTE_DIRECTORY = 0x10,
};
#define INVALID_HANDLE_VALUE (reinterpret_cast<void*>(-1))

#include "port/win32api.h"

#endif
