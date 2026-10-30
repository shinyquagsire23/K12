// The pc UI's hardware mouse cursors are the PC games' Win32 cursor resources, read from the .cur
// files the macOS Steam ports ship.
//
// With "EnableHardwareMouse" on, the engine points with LoadCursorA(hInst, id) + SetCursor
// (CExoInputInternal::SetMouseCursor), id indexing g_asMouseCursorNames. The macOS build's LoadCursorA
// turns the id into "<name>.cur" -- KOTOR II's from the bundle's Resources/Cursors, KOTOR's from
// AspyrAssets/global/WinCursor -- and hands the image to SDL_CreateColorCursor. The gui_mp_* textures
// the software cursor draws are not that art: they are DXT-compressed and drawn scaled, and KOTOR's
// are a cyan/green re-tint of the PC's blue set. So the pc UI takes the .cur files when it finds them
// and keeps the software cursor when it does not.
//
// Searched, in order: $K2_CURSORS, Cursors/ or WinCursor/ in the working directory, the folders an
// installed copy keeps beside its data (KOTOR II's resources/cursors), the macOS Steam install, and
// last the Windows game's own executable, whose RT_CURSOR resources are the same images.
//
// The images are 32 pixels, a speck on a 4K screen once the window is DPI aware (Windows window units
// are pixels), so they are scaled by the window's height in its own units over 1080, rounded: a
// 2160-line window gets 64-pixel cursors, a macOS window in points keeps 32.
#include <SDL.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "k2shim.h"

extern char* g_asMouseCursorNames[];

namespace {

const unsigned kNumCursors = 93;

bool s_bEnabled;
bool s_bShown = true;
bool s_bPointerActive = true;   // false from a pad press until the mouse next moves (xbox UIs)
std::string s_dir;
std::map<std::string, std::string> s_files;   // lower-case file name -> its name on disk
std::vector<uint8_t> s_exe;
SDL_Cursor* s_cursors[kNumCursors];
bool s_bLoaded[kNumCursors];
int s_nScale;
uintptr_t s_current;

std::string Lower(std::string s)
{
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

// Matches a folder only when it holds the default arrow; file names are compared case-blind, since
// KOTOR's set has gui_mp_bashU.cur.
bool ScanDir(const std::string& dir)
{
    DIR* d = opendir(dir.c_str());
    if (!d) return false;
    std::map<std::string, std::string> files;
    while (dirent* e = readdir(d))
        files[Lower(e->d_name)] = e->d_name;
    closedir(d);
    if (!files.count("gui_mp_defaultu.cur")) return false;
    s_dir = dir;
    s_files.swap(files);
    return true;
}

bool ReadWhole(const char* pszPath, std::vector<uint8_t>& b)
{
    FILE* f = fopen(pszPath, "rb");
    if (!f) return false;
    uint8_t buf[65536];
    for (size_t n; (n = fread(buf, 1, sizeof buf, f)) > 0;) b.insert(b.end(), buf, buf + n);
    fclose(f);
    return true;
}

uint32_t Rd16(const std::vector<uint8_t>& b, size_t o) { return b[o] | b[o + 1] << 8; }
uint32_t Rd32(const std::vector<uint8_t>& b, size_t o) { return Rd16(b, o) | Rd16(b, o + 2) << 16; }

// Reads one resource of the executable by numeric type and id, the first language's. The resource
// tree is type -> id -> language -> data entry; a data entry holds an RVA, mapped to the file through
// the section table.
bool ExeResource(unsigned nType, unsigned nId, std::vector<uint8_t>& out)
{
    const std::vector<uint8_t>& b = s_exe;
    if (b.size() < 0x40) return false;
    size_t pe = Rd32(b, 0x3c);
    if (pe + 24 > b.size() || Rd32(b, pe) != 0x4550) return false;
    unsigned nSections = Rd16(b, pe + 6);
    size_t opt = pe + 24, sec = opt + Rd16(b, pe + 20);
    size_t dirs = opt + (Rd16(b, opt) == 0x20b ? 112 : 96);
    if (dirs + 24 > b.size() || sec + nSections * 40 > b.size()) return false;
    uint32_t rsrcRva = Rd32(b, dirs + 16);
    auto fileOff = [&](uint32_t rva, size_t& off) {
        for (unsigned i = 0; i < nSections; i++) {
            size_t h = sec + i * 40;
            uint32_t va = Rd32(b, h + 12), size = Rd32(b, h + 16), raw = Rd32(b, h + 20);
            if (rva >= va && rva < va + size) { off = raw + (rva - va); return off < b.size(); }
        }
        return false;
    };
    size_t root;
    if (!rsrcRva || !fileOff(rsrcRva, root)) return false;
    // Finds numeric id nKey in the directory at dir, root-relative; the language level takes its
    // first entry.
    auto find = [&](size_t dir, long nKey, uint32_t& next) {
        if (root + dir + 16 > b.size()) return false;
        unsigned n = Rd16(b, root + dir + 12) + Rd16(b, root + dir + 14);
        for (unsigned i = 0; i < n; i++) {
            size_t e = root + dir + 16 + i * 8;
            if (e + 8 > b.size()) return false;
            uint32_t name = Rd32(b, e);
            if (nKey < 0 || (!(name & 0x80000000u) && name == static_cast<uint32_t>(nKey))) {
                next = Rd32(b, e + 4);
                return true;
            }
        }
        return false;
    };
    uint32_t t, i, l;
    if (!find(0, nType, t) || !(t & 0x80000000u)) return false;
    if (!find(t & 0x7fffffff, nId, i) || !(i & 0x80000000u)) return false;
    if (!find(i & 0x7fffffff, -1, l) || (l & 0x80000000u)) return false;
    if (root + l + 8 > b.size()) return false;
    size_t off;
    uint32_t nSize = Rd32(b, root + l + 4);
    if (!fileOff(Rd32(b, root + l), off) || off + nSize > b.size()) return false;
    out.assign(b.begin() + off, b.begin() + off + nSize);
    return true;
}

bool FindExe(bool bKotor1)
{
    if (!ReadWhole(bKotor1 ? "swkotor.exe" : "swkotor2.exe", s_exe)) return false;
    std::vector<uint8_t> group;
    if (ExeResource(12, 1, group)) {   // RT_GROUP_CURSOR 1: the default arrow
        s_dir = bKotor1 ? "swkotor.exe" : "swkotor2.exe";
        return true;
    }
    s_exe.clear();
    return false;
}

bool FindDir(bool bKotor1)
{
    if (const char* p = getenv("K2_CURSORS"))
        return ScanDir(p) || FindExe(bKotor1);
    if (ScanDir("Cursors") || ScanDir("WinCursor")) return true;
    if (bKotor1 ? ScanDir("../AspyrAssets/global/WinCursor")
                : ScanDir("../Resources/Cursors") || ScanDir("../resources/cursors"))
        return true;
#if defined(__APPLE__) && !defined(__ANDROID__)
    if (const char* home = getenv("HOME")) {
        std::string steam = std::string(home) + "/Library/Application Support/Steam/steamapps/common/";
        return ScanDir(steam + (bKotor1 ? "swkotor/Knights of the Old Republic.app/Contents/"
                                          "AspyrAssets/global/WinCursor"
                                        : "Knights of the Old Republic II/KOTOR2.app/Contents/"
                                          "Resources/Cursors"))
            || FindExe(bKotor1);
    }
#endif
    return FindExe(bKotor1);
}

// Scales the 32-pixel art to the window's height in its own units over 1080, rounded, at least 1.
// Windows window units are pixels, so there the divisor is 540 and a 4K screen draws the art 4x.
int WindowScale()
{
    SDL_Window* w = SDL_GetMouseFocus();
    if (!w) w = SDL_GL_GetCurrentWindow();
    int nW = 0, nH = 0;
    if (w) SDL_GetWindowSize(w, &nW, &nH);
#ifdef _WIN32
    int n = (nH + 270) / 540;
#else
    int n = (nH + 540) / 1080;
#endif
    return n < 1 ? 1 : n;
}

// Builds a cursor from the DIB at dib -- BITMAPINFOHEADER with a doubled height, palette, XOR rows and
// the 1-bit AND mask, both bottom-up -- at the given hotspot, each image pixel drawn nScale x nScale.
// The PC sets are all 8-bit palettized; 1/4/24/32 are read the same way.
SDL_Cursor* DibCursor(const std::vector<uint8_t>& b, size_t dib, int nHotX, int nHotY, int nScale)
{
    if (dib + 40 > b.size()) return 0;
    size_t hdr = Rd32(b, dib);
    int w = static_cast<int>(Rd32(b, dib + 4)), h = static_cast<int>(Rd32(b, dib + 8)) / 2;
    int bpp = static_cast<int>(Rd16(b, dib + 14));
    if (w <= 0 || h <= 0 || w > 256 || h > 256 || Rd32(b, dib + 16) != 0) return 0;
    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32) return 0;
    size_t nColors = bpp <= 8 ? (Rd32(b, dib + 32) ? Rd32(b, dib + 32) : 1u << bpp) : 0;
    size_t pal = dib + hdr, xr = pal + nColors * 4;
    size_t xorStride = (static_cast<size_t>(w) * bpp + 31) / 32 * 4, andStride = (static_cast<size_t>(w) + 31) / 32 * 4;
    size_t an = xr + xorStride * h;
    if (an + andStride * h > b.size()) return 0;

    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w * nScale, h * nScale, 32,
                                                    SDL_PIXELFORMAT_ARGB8888);
    if (!s) return 0;
    bool bAlpha = false;   // a 32-bit image with any alpha uses it instead of the mask
    if (bpp == 32)
        for (int y = 0; y < h && !bAlpha; y++)
            for (int x = 0; x < w && !bAlpha; x++) bAlpha = b[xr + y * xorStride + x * 4 + 3] != 0;
    for (int y = 0; y < h * nScale; y++) {
        size_t row = xr + static_cast<size_t>(h - 1 - y / nScale) * xorStride;
        size_t mrow = an + static_cast<size_t>(h - 1 - y / nScale) * andStride;
        uint32_t* out = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(s->pixels) + static_cast<size_t>(y) * s->pitch);
        for (int x = 0; x < w * nScale; x++) {
            int nX = x / nScale;
            uint32_t bgr, a = (b[mrow + nX / 8] >> (7 - nX % 8)) & 1 ? 0 : 0xff;
            if (bpp <= 8) {
                unsigned i = (b[row + nX * bpp / 8] >> (8 - bpp - nX * bpp % 8)) & ((1u << bpp) - 1);
                bgr = i < nColors ? Rd32(b, pal + i * 4) & 0xffffff : 0;
            } else {
                const uint8_t* p = &b[row + static_cast<size_t>(nX) * (bpp / 8)];
                bgr = p[0] | p[1] << 8 | p[2] << 16;
                if (bAlpha) a = p[3];
            }
            out[x] = a ? (a << 24 | bgr) : 0;
        }
    }
    SDL_Cursor* c = SDL_CreateColorCursor(s, (nHotX < w ? nHotX : 0) * nScale,
                                          (nHotY < h ? nHotY : 0) * nScale);
    SDL_FreeSurface(s);
    return c;
}

// Reads a .cur: an ICONDIR and one ICONDIRENTRY, the hotspot in the planes/bpp slots, then the DIB at
// the entry's offset.
SDL_Cursor* LoadCur(const std::string& path, int nScale)
{
    std::vector<uint8_t> b;
    if (!ReadWhole(path.c_str(), b) || b.size() < 22 || Rd16(b, 2) != 2 || Rd16(b, 4) < 1) return 0;
    return DibCursor(b, Rd32(b, 18), static_cast<int>(Rd16(b, 10)), static_cast<int>(Rd16(b, 12)), nScale);
}

// Reads the executable's RT_GROUP_CURSOR nGroup -- a header and 14-byte entries of width, height,
// planes, bit count, size and RT_CURSOR id, the deepest image winning -- then that RT_CURSOR: the
// hotspot as two words, then the DIB.
SDL_Cursor* LoadExeCursor(unsigned nGroup, int nScale)
{
    std::vector<uint8_t> g, c;
    if (!ExeResource(12, nGroup, g) || g.size() < 6) return 0;
    unsigned nBest = 0, nBestBits = 0;
    for (unsigned i = 0, n = Rd16(g, 4); i < n && 6 + i * 14 + 14 <= g.size(); i++) {
        unsigned nBits = Rd16(g, 6 + i * 14 + 6);
        if (nBits >= nBestBits) { nBestBits = nBits; nBest = Rd16(g, 6 + i * 14 + 12); }
    }
    if (!nBest || !ExeResource(1, nBest, c) || c.size() < 44) return 0;
    return DibCursor(c, 4, static_cast<int>(Rd16(c, 0)), static_cast<int>(Rd16(c, 2)), nScale);
}

// Returns the cursor for engine id nId. Both tables name one cursor the sets lack, id 6
// "gui_mp_invalid", which the KOTOR II plist maps to gui_mp_invalidd.cur, the down half of the 5/6
// pair. The executables number their groups as this table does except that the two pickup cursors
// (59, 60) come last, after the run and walk arrows.
SDL_Cursor* Cursor(unsigned nId)
{
    if (nId == 0 || nId >= kNumCursors) return 0;
    int nScale = WindowScale();
    if (nScale != s_nScale) {
        for (unsigned i = 0; i < kNumCursors; i++) {
            if (s_cursors[i]) SDL_FreeCursor(s_cursors[i]);
            s_cursors[i] = 0;
            s_bLoaded[i] = false;
        }
        s_nScale = nScale;
    }
    if (!s_bLoaded[nId]) {
        s_bLoaded[nId] = true;
        const char* pszName = g_asMouseCursorNames[nId];
        std::string name = Lower(pszName ? pszName : "");
        auto it = s_files.find(name + ".cur");
        if (it == s_files.end()) it = s_files.find(name + "d.cur");
        if (!s_exe.empty())
            s_cursors[nId] = LoadExeCursor(nId >= 61 ? nId - 2 : nId >= 59 ? nId + 32 : nId, nScale);
        else if (!name.empty() && it != s_files.end())
            s_cursors[nId] = LoadCur(s_dir + "/" + it->second, nScale);
        if (!s_cursors[nId])
            fprintf(stderr, "k2shim: no cursor %u (%s)\n", nId, name.c_str());
    }
    return s_cursors[nId];
}

void Apply()
{
    if (!s_bEnabled) {
        SDL_ShowCursor(SDL_DISABLE);   // the engine draws its own (software) cursor
        return;
    }
    if (SDL_Cursor* c = Cursor(static_cast<unsigned>(s_current & 0xffff)))
        SDL_SetCursor(c);
    SDL_ShowCursor(s_bShown && s_bPointerActive ? SDL_ENABLE : SDL_DISABLE);
}

} // namespace

// Enables hardware cursors when the .cur set is there; returns the mode to use.
extern "C" int k2shim_cursor_enable(int bWanted, int bKotor1)
{
    static int s_nFound = -1;
    if (s_nFound < 0) {
        s_nFound = FindDir(bKotor1 != 0);
        fprintf(stderr, "k2shim: hardware cursors %s%s\n", s_nFound ? "from " : "not found",
                s_dir.c_str());
    }
    s_bEnabled = bWanted && s_nFound;
    Apply();
    return s_bEnabled;
}

// Hands out the token LoadCursorA returns. The engine destroys the handle SetCursor returned whenever
// it differs from the new one, so every load is a distinct token with the id in its low 16 bits.
void* k2shim_cursor_load(uint64_t nId)
{
    static uintptr_t s_next = 0x10000;
    return reinterpret_cast<void*>(s_next++ << 16 | (nId & 0xffff));
}

void* k2shim_cursor_select(void* h)
{
    uintptr_t prev = s_current;
    s_current = reinterpret_cast<uintptr_t>(h);
    // The engine selects its cursor again on every WM_SETCURSOR, one a mouse move; setting the
    // same one again costs macOS a cursor-rect rebuild and a window-server round trip each time.
    if (s_bEnabled && h && (prev & 0xffff) != (s_current & 0xffff)) Apply();
    return reinterpret_cast<void*>(prev);
}

void k2shim_cursor_show(bool bShow)
{
    s_bShown = bShow;
    Apply();
}

void k2shim_cursor_refresh() { Apply(); }

bool k2shim_pointer_from_pad()
{
    if (!s_bPointerActive) return false;
    s_bPointerActive = false;
    Apply();
    return true;
}

void k2shim_pointer_from_mouse()
{
    if (s_bPointerActive) return;
    s_bPointerActive = true;
    Apply();
}

bool k2shim_pad_activity(const SDL_Event* e)
{
    const int kDeadZone = 12000;   // a resting stick's drift stays below this
    switch (e->type) {
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_JOYBUTTONDOWN:
        return true;
    case SDL_JOYHATMOTION:
        return e->jhat.value != SDL_HAT_CENTERED;
    case SDL_CONTROLLERAXISMOTION:
        return e->caxis.value > kDeadZone || e->caxis.value < -kDeadZone;
    case SDL_JOYAXISMOTION:
        return e->jaxis.value > kDeadZone || e->jaxis.value < -kDeadZone;
    default:
        return false;
    }
}
