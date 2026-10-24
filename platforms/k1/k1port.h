// Internal interface of the K1 port layer (K2P_GAME=k1).
//
// k1port.cpp is compiled with the engine's flags because it implements classes the engine declares:
// ObbFile, the FMOD wrapper's globals, the Win32 shim parameter types. The zip reader and the LZMA
// decoder are separate translation units that include miniz.h and the LZMA SDK instead, whose
// anonymous `mz_zip_archive` typedef cannot coexist with the engine's `struct mz_zip_archive`. This
// header is the seam between the two: plain C types only.
#pragma once

#include <stddef.h>

struct SDL_RWops;

#ifdef __cplusplus
extern "C" {
#endif

// One OBB, a zip archive whose entries are stored, never deflated.
// The archive lives in caller-owned storage of k1zip_archive_size() bytes, at the start of ObbFile.
size_t k1zip_archive_size(void);
int    k1zip_open(void* pZip, const char* pszPath);          // nonzero on success
int    k1zip_open_empty(void* pZip);                         // no entries (a directory-backed ObbFile)
void   k1zip_close(void* pZip);
unsigned int k1zip_count(void* pZip);
// One entry, with its full name ("data/2da.bzf", "modules/"), directory flag and time stamp.
int    k1zip_entry(void* pZip, unsigned int nIndex, char* pszName, size_t nName, int* pbDirectory,
                   unsigned long long* pnTime);
int    k1zip_locate(void* pZip, const char* pszName);        // -1 when absent
// A read-only stream over one entry (stored data, read in place); null on failure.
struct SDL_RWops* k1zip_rwops(void* pZip, const char* pszPath, int nIndex);

// Runs the LzmaLib uncompress call for a .bzf resource, which holds five property bytes, then the
// stream.
int k1lzma_uncompress(unsigned char* pDst, size_t* pnDst, const unsigned char* pSrc, size_t* pnSrc,
                      const unsigned char* pProps, size_t nProps);   // 0 == SZ_OK

// Plays one movie from `rw`. Nonzero when it opened.
int k1movie_play(struct SDL_RWops* rw, const char* pszName, int bSound, int nVolume);

// Mounts an archive the way the Android mount calls do, for a platform main that only has a path. A
// directory in place of the .obb serves its loose files (a PC data set such as the Steam install).
void k1port_mount_obb(const char* pszPath, int bPatch);
// Nonzero when the game data has the file (a path inside the archives: "override/x.tga"), in the
// patch or the main OBB or the loose files a directory stands in for.
int k1port_data_has(const char* pszName);

#ifdef __cplusplus
}
#endif
