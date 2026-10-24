// KOTOR 1's OBB reader, the zip half (K2P_GAME=k1; k1port.h).
//
// K1's expansion files are plain zips with every entry stored (main.53 / patch.53 report "stor"
// for all 16,549 entries). The engine's ObbFile calls mz_zip_reader_locate_file on the archive at
// the start of an ObbFile, so the archive here is a real mz_zip_archive in that storage, and an
// entry's stream reads its bytes in place with pread (a deflated entry, should one appear, is
// inflated to the heap instead).
#include "k1port.h"

#include <SDL.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "miniz.h"

namespace {

struct Zip {
    mz_zip_archive zip;   // the engine hands ObbFile::GetZip() to mz_zip_reader_locate_file
    int fd;               // the same file, for the streams' preads
};
static_assert(sizeof(Zip) <= 0xa0, "fits ObbFile's archive storage (k1obb.h)");

struct Stream {
    int fd;               // -1: pMem holds the (inflated) bytes
    uint64_t off, size, pos;
    unsigned char* pMem;
};

Stream* Of(SDL_RWops* rw) { return static_cast<Stream*>(rw->hidden.unknown.data1); }

Sint64 Size(SDL_RWops* rw) { return static_cast<Sint64>(Of(rw)->size); }

Sint64 Seek(SDL_RWops* rw, Sint64 off, int whence)
{
    Stream* s = Of(rw);
    int64_t base = whence == RW_SEEK_SET ? 0 : whence == RW_SEEK_CUR ? static_cast<int64_t>(s->pos) : static_cast<int64_t>(s->size);
    int64_t to = base + off;
    // ObbFile::Entry::seek refuses a position past the end (-1, stream unmoved), and K1 relies on
    // it: IosLocalizedStrings' line reader only stops when that seek fails.
    if (to < 0 || static_cast<uint64_t>(to) > s->size) return -1;
    s->pos = static_cast<uint64_t>(to);
    return to;
}

size_t Read(SDL_RWops* rw, void* pDst, size_t nSize, size_t nCount)
{
    Stream* s = Of(rw);
    if (nSize == 0 || nCount == 0 || s->pos >= s->size) return 0;
    uint64_t want = static_cast<uint64_t>(nSize) * nCount, left = s->size - s->pos;
    if (want > left) want = left - left % nSize;
    if (want == 0) return 0;
    if (s->pMem) {
        memcpy(pDst, s->pMem + s->pos, static_cast<size_t>(want));
    } else {
        ssize_t got = pread(s->fd, pDst, static_cast<size_t>(want), static_cast<off_t>(s->off + s->pos));
        if (got <= 0) return 0;
        want = static_cast<uint64_t>(got);
    }
    s->pos += want;
    return static_cast<size_t>(want / nSize);
}

size_t Write(SDL_RWops*, const void*, size_t, size_t) { return 0; }

int Close(SDL_RWops* rw)
{
    Stream* s = Of(rw);
    if (s->pMem) mz_free(s->pMem);
    delete s;
    SDL_FreeRW(rw);
    return 0;
}

SDL_RWops* Make(Stream* s)
{
    SDL_RWops* rw = SDL_AllocRW();
    if (!rw) {
        delete s;
        return 0;
    }
    rw->size = Size;
    rw->seek = Seek;
    rw->read = Read;
    rw->write = Write;
    rw->close = Close;
    rw->type = SDL_RWOPS_UNKNOWN;
    rw->hidden.unknown.data1 = s;
    return rw;
}

} // namespace

extern "C" size_t k1zip_archive_size(void) { return sizeof(Zip); }

extern "C" int k1zip_open(void* p, const char* pszPath)
{
    Zip* z = static_cast<Zip*>(p);
    memset(z, 0, sizeof *z);
    z->fd = open(pszPath, O_RDONLY);
    if (z->fd < 0) return 0;
    if (!mz_zip_reader_init_file(&z->zip, pszPath, 0)) {
        fprintf(stderr, "k1: %s: not a zip (%s)\n", pszPath,
                mz_zip_get_error_string(mz_zip_get_last_error(&z->zip)));
        close(z->fd);
        z->fd = -1;
        return 0;
    }
    return 1;
}

// An archive with no entries (a bare end-of-central-directory record), for an ObbFile that serves a
// directory instead (k1port.cpp): the engine's own mz_zip_reader_locate_file calls just miss.
extern "C" int k1zip_open_empty(void* p)
{
    static const unsigned char s_eocd[22] = { 'P', 'K', 5, 6 };
    Zip* z = static_cast<Zip*>(p);
    memset(z, 0, sizeof *z);
    z->fd = -1;
    return mz_zip_reader_init_mem(&z->zip, s_eocd, sizeof s_eocd, 0);
}

extern "C" void k1zip_close(void* p)
{
    Zip* z = static_cast<Zip*>(p);
    mz_zip_reader_end(&z->zip);
    if (z->fd >= 0) close(z->fd);
    z->fd = -1;
}

extern "C" unsigned int k1zip_count(void* p) { return mz_zip_reader_get_num_files(&static_cast<Zip*>(p)->zip); }

extern "C" int k1zip_entry(void* p, unsigned int n, char* pszName, size_t nName, int* pbDirectory,
                           unsigned long long* pnTime)
{
    Zip* z = static_cast<Zip*>(p);
    mz_zip_archive_file_stat st;
    if (!mz_zip_reader_file_stat(&z->zip, n, &st)) return 0;
    snprintf(pszName, nName, "%s", st.m_filename);
    *pbDirectory = st.m_is_directory != 0;
    *pnTime = static_cast<unsigned long long>(st.m_time);
    return 1;
}

extern "C" int k1zip_locate(void* p, const char* pszName)
{
    return mz_zip_reader_locate_file(&static_cast<Zip*>(p)->zip, pszName, 0, 0);
}

extern "C" SDL_RWops* k1zip_rwops(void* p, const char* pszPath, int n)
{
    Zip* z = static_cast<Zip*>(p);
    mz_zip_archive_file_stat st;
    if (n < 0 || !mz_zip_reader_file_stat(&z->zip, static_cast<mz_uint>(n), &st) || st.m_is_directory) return 0;
    Stream* s = new Stream();
    s->size = st.m_uncomp_size;
    if (st.m_method == 0) {
        // the data follows the local header: 30 bytes, then its own name and extra field
        unsigned char hdr[30];
        if (pread(z->fd, hdr, sizeof hdr, static_cast<off_t>(st.m_local_header_ofs)) != static_cast<ssize_t>(sizeof hdr)
            || hdr[0] != 'P' || hdr[1] != 'K' || hdr[2] != 3 || hdr[3] != 4) {
            fprintf(stderr, "k1: OBB entry %s: bad local header\n", pszPath);
            delete s;
            return 0;
        }
        s->fd = z->fd;
        s->off = st.m_local_header_ofs + 30 + (hdr[26] | hdr[27] << 8) + (hdr[28] | hdr[29] << 8);
    } else {
        size_t n2 = 0;
        s->fd = -1;
        s->pMem = static_cast<unsigned char*>(mz_zip_reader_extract_to_heap(&z->zip, static_cast<mz_uint>(n), &n2, 0));
        if (!s->pMem) {
            delete s;
            return 0;
        }
        s->size = n2;
    }
    return Make(s);
}
