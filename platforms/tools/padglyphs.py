#!/usr/bin/env python3
"""padglyphs.py — build the button-prompt art the Controller options page selects.

The engine already carries two glyph sets and picks between them on one flag
(`IsPsButtonIcons()` @0x468a98, i.e. "is the pad a PlayStation pad"):

    floating icons   cus_button_{a,b,x,y}an   /  cus_button_{a,b,x,y}ps     (94x94)
                     AspyrFloatingButtonIcon::InitAndAdd @0x3debd0
    inline glyphs    diafnt16x16b_an           /  diafnt16x16b_ps, d2x/d3xfont16x16b_ps
                     CAurGUIStringInternal::LoadAlternateFonts @0x72af7c — an atlas that shares
                     the dialog font's 16x16 grid; cells 0x10..0x13 are the A/B/X/Y glyphs that
                     AspyrAddStringIcon prepends to a caption ('a'->0x10 'b'->0x11 'x'->0x12 'y'->0x13)
    console badges   uibut_* (the Xbox layouts' fills: uibut_a, uibut_black, uibut_ltrig, ...)

THE SWITCH SET (KOTOR II) is the vector recreations of the Switch port's own uibut_* textures,
resources/switch_k2_button_*.svg, rendered by rsvg-convert. Letter for letter: the engine's "A" shows
the Switch's A (a controller that sends them the other way round is the page's Swap A/B and X/Y
row, qol/k2qol.cpp). Outputs, all 32-bit uncompressed TGA in bottom-up row order:

    k2nx_<name>.tga                       the fifteen console badges, at the Switch textures' sizes
                                          (NX_UIBUT: uibut_<name> -> art)
    cus_button_{a,b,x,y,l,r}nx.tga        the floating icons, the visible 80 x 80 the others fill
    k2fnt16_nx / k2fnt32_nx / k2fnt64_nx  the PlayStation atlases (diafnt16x16b_ps, d2x/d3xfont16x16b_ps)
                                          with cells 0x10..0x16 redrawn: A/B/X/Y, the d-pad, L / R

The atlases are the only part taken from game data: the Switch dump's loose `override/` TGAs
(decomp_bins/swkotorii_switch/romfs/override), then the Android data's (its loose override/, or the
same files inside the main OBB, --obb-dir: decomp_bins/swkotorii_obb by default, K2P_OBB_DIR in the
build -- so any setup that can play the game builds the art). The 16-px slot starts from the Xbox
dialogfont16x16b; every pad cell is redrawn either way. Without them, or without rsvg-convert, the
generated table has no Switch set and the build still succeeds: the runtime then logs that it has
no Switch art and the Switch choice falls back to the Xbox set.

    python3 platforms/tools/padglyphs.py --out-cpp gen/k2assets.gen.cpp [--png-dir /tmp/glyphs]

THE XBOX AND PLAYSTATION SETS (KOTOR II) are drawn from resources/ too -- xbox_button_* and
ps_button_*, A / B / X / Y and the shoulders -- into the floating icons (cus_button_<k>an / <k>ps,
in front of the game's own) and the atlas copies below; the game's atlases supply only the font
and the d-pad cell.

SHOULDER BUTTONS (the port's, both games): letters 'l' / 'r' of the string-icon helpers draw atlas
cells 0x15 / 0x16 (CELL) and the floating icons take 'l' / 'r' too. The art is resources/'
xbox_button_lb/rb, ps_button_l1/r1, the Switch sets' l/r and the iOS sets' lb/rb, rendered by
rsvg-convert. KOTOR II's own atlases cannot be extended in place, so build() writes copies that
carry them (k2fnt16_an, k2fnt16_ps, k2fnt32_ps, k2fnt64_ps; the APK's loose override/ supplies
diafnt16x16b_an) and qol/k2qol.cpp renames to those.

THE GAMECUBE SET (both games, GC_UIBUT / GC_LETTERS) is resources/gc_button_*, built like the
Switch set under the suffix gc: k2gc_<name>, cus_button_<k>gc, k2fnt{16,32,64}_gc; for K1,
ios_button_<k>gc, k1fnt{16,32}_gc and {black,white,start}button_gc. Its shoulders are the Switch 2
GameCube controller's ZL / Z.

--game k1 builds KOTOR 1's four sets instead (build_k1): the art is the vector recreations in
resources/ (xbox_button_*, ps_button_*, switch_button_*, gc_button_*), rendered at each size it is used at by
rsvg-convert, so nothing is resampled; only K1's glyph atlases, which the art is laid into, come from
its patch OBB (--obb-dir, decomp_bins/swkotor_obb by default). Its Switch set is letter for letter too.

THE RENDERS: every PNG rsvg-convert makes for a build is kept in resources/padglyph_renders.zip
(keyed by the SHA-256 of the SVG text rsvg-convert is fed; the crop boxes by the SVG file's), and a
render is taken from there before rsvg-convert is asked. So a host without librsvg (Windows: no
MSYS) builds the art, and every host lays the same pixels. An SVG edited since the cache was made
needs rsvg-convert, then `padglyphs.py --refresh-cache` and a commit of the zip; without either
that set is dropped with a WARNING.

THE ATLASES ON A HOST WITHOUT THE ANDROID DATA (no decomp_bins/: a Windows host with only the
Steam install): after the dirs and OBBs above, the installed PC game (--game-dir, else found
through Steam: its registry key / libraryfolders.vdf). KOTOR II's Steam override/ has the
PlayStation atlases byte for byte as the OBB, but no diafnt16x16b_an, so k2fnt16_an starts from
its dialogfont16x16b (the Switch dump's). KOTOR 1's PC data has dialogfont16x16b as an
uncompressed TPC in TexturePacks/swpc_tex_gui.erf (the OBB's pixels but for cells 0..3 and the
pad cells 0x10..0x15) and no d2xfont16x16b: the 32-px atlas is then the 16-px one doubled, which
a PC data set never binds (K1LoadIphoneFont drops the 2x slot when d2xfont16x16b is not in the
data).

    python3 platforms/tools/padglyphs.py --check [--game k1]   # what a build would lack (configure)
    python3 platforms/tools/padglyphs.py --refresh-cache        # needs rsvg-convert
"""
import argparse
import glob
import hashlib
import io
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import zipfile
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC_DIR = os.path.join(REPO, 'decomp_bins', 'swkotorii_switch', 'romfs', 'override')
# The Android APK's loose override/ (its Xbox atlas diafnt16x16b_an), searched after SRC_DIR.
APK_DIR = os.path.join(REPO, 'decomp_bins', 'swkotorii_android', 'override')

# --game k1 sources: the vector art in resources/ (xbox_button_*, ps_button_*: KOTOR II's Switch
# override icons; switch_button_*: KOTOR 1's Switch override icons, its shoulders and Start among
# them), rendered by rsvg-convert (librsvg; $RSVG_CONVERT overrides the lookup), and KOTOR 1's
# Android patch OBB for the glyph atlases the art is laid into (K1's own font, not in the repo).
RESOURCES = os.path.join(REPO, 'resources')
K1_OBB_DIR = os.path.join(REPO, 'decomp_bins', 'swkotor_obb')
# The rsvg-convert renders; see the module docstring.
RENDER_CACHE = os.path.join(RESOURCES, 'padglyph_renders.zip')
# The repo's copies of the PC games' data, the installed game's stand-in for --refresh-cache.
PC_DATA = {'k2': os.path.join(REPO, 'decomp_bins', 'KOTOR2_Windows'),
           'k1': os.path.join(REPO, 'decomp_bins', 'KOTOR1_Windows')}
# Steam: (app id, default install dir) of each game.
STEAM_APPS = {'k2': (208580, 'Knights of the Old Republic II'), 'k1': (32370, 'swkotor')}
# KOTOR II: the Android OBBs (Aspyr's libObbVfs format), whose override/ holds the same atlases as
# the loose dirs above.
K2_OBB_DIR = os.path.join(REPO, 'decomp_bins', 'swkotorii_obb')

# KOTOR II's console badges the Switch set replaces: uibut_<name> -> resources/switch_k2_button_<art>
# (the Switch port's own uibut_* textures, which these recreate: Black is R, White L, the triggers
# ZL / ZR, Start / Back + / -, 4Way the d-pad, R_PortSel its party-select arrows).
NX_UIBUT = {'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'black': 'r', 'white': 'l', 'ltrig': 'zl',
            'rtrig': 'zr', 'start': 'plus', 'back': 'minus', '4way': 'dpad', 'r_portsel': 'dpad_arrow',
            'lstick': 'lstick', 'rstick': 'rstick', 'rstick_ud': 'rstick_ud'}
# atlas cell of each engine letter (AspyrAddStringIcon's); 'l' / 'r' (the shoulder buttons) are the
# port's, in two cells no shipped atlas draws (0x14 is KOTOR II's d-pad, 0x15 / 0x16 hold placeholder
# art) but that the string code already treats as pad glyphs (0x10..0x16: skipped without a pad, drawn
# white, a size up on pc / xbox).
CELL = {'a': 0x10, 'b': 0x11, 'x': 0x12, 'y': 0x13, 'dpad': 0x14, 'l': 0x15, 'r': 0x16}
# the resources/ art each set draws its shoulders with (no positional swap on the Switch)
SHOULDERS = {'xbox': ('lb', 'rb'), 'ps': ('l1', 'r1'), 'ios': ('lb', 'rb'), 'switch': ('l', 'r'),
             'switch_k2': ('l', 'r'), 'gc': ('zl', 'z')}
# The console sets' art for each atlas / floating-icon letter (the Switch's is letter for letter).
NX_LETTERS = {k: k for k in ('a', 'b', 'x', 'y', 'dpad', 'l', 'r')}
# The GameCube set (both games): resources/gc_button_*, the indigo controller. Letter for letter like
# the Switch's; the shoulders are the Switch 2 GameCube controller's ZL and Z, the triggers its analog
# L / R, and Back, which a GameCube pad has none of, its Home button.
GC_LETTERS = dict(NX_LETTERS, l='zl', r='z')
GC_UIBUT = {'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'black': 'z', 'white': 'zl', 'ltrig': 'l',
            'rtrig': 'r', 'start': 'start', 'back': 'home', '4way': 'dpad', 'r_portsel': 'dpad_arrow',
            'lstick': 'stick', 'rstick': 'cstick', 'rstick_ud': 'cstick_ud'}
GRID = 16   # the dialog font atlas is 16 x 16 cells


class Tga:
    """A type-2 (uncompressed true-colour) 32-bit TGA, pixels kept in FILE row order (BGRA)."""

    def __init__(self, data):
        idlen, cmtype, itype = data[0], data[1], data[2]
        w, h, bpp, desc = struct.unpack_from('<HHBB', data, 12)
        if cmtype != 0 or itype != 2 or bpp != 32:
            raise ValueError('only uncompressed 32-bit TGA is supported (type %d, %d bpp)' % (itype, bpp))
        self.header = bytearray(data[:18 + idlen])
        self.w, self.h, self.desc = w, h, desc
        self.px = bytearray(data[18 + idlen:18 + idlen + w * h * 4])
        self.tail = bytes(data[18 + idlen + w * h * 4:])

    def row_of(self, y):
        """File row index of TOP-DOWN row y."""
        return y if self.desc & 0x20 else self.h - 1 - y

    def get(self, x, y):
        o = (self.row_of(y) * self.w + x) * 4
        b, g, r, a = self.px[o:o + 4]
        return r, g, b, a

    def put(self, x, y, rgba):
        r, g, b, a = rgba
        o = (self.row_of(y) * self.w + x) * 4
        self.px[o:o + 4] = bytes((b, g, r, a))

    def bytes(self):
        return bytes(self.header) + bytes(self.px) + self.tail

    @staticmethod
    def blank(w, h):
        """A transparent w x h image, rows bottom-up like the game's own TGAs."""
        header = bytearray(18)
        header[2] = 2
        struct.pack_into('<HHBB', header, 12, w, h, 32, 8)
        return Tga(bytes(header) + bytes(w * h * 4))


def glyph_box(atlas):
    """(x0, x1, y0, y1) of the atlas's own A glyph (cell 0x10) within its cell."""
    cw, ch = atlas.w // GRID, atlas.h // GRID
    ref = CELL['a']
    rx, ry = (ref % GRID) * cw, (ref // GRID) * ch
    xs = [x for x in range(cw) for y in range(ch) if atlas.get(rx + x, ry + y)[3] > 8]
    ys = [y for y in range(ch) for x in range(cw) if atlas.get(rx + x, ry + y)[3] > 8]
    if not xs:
        return 0, cw - 1, 0, ch - 1
    return min(xs), max(xs), min(ys), max(ys)


def lay_cell(atlas, k, glyph_svg, box=None):
    """Draw glyph_svg into letter k's cell of atlas, as wide as the atlas's own A glyph (cell 0x10,
    or `box`, glyph_box() taken before that cell is redrawn) and centred where it sits, so a
    shoulder glyph lines up with the face buttons beside it."""
    cw, ch = atlas.w // GRID, atlas.h // GRID
    c = CELL[k]
    x0, x1, y0, y1 = box or glyph_box(atlas)
    side = x1 - x0 + 1
    g = glyph_svg.fitted(side, side)
    cx, cy = (c % GRID) * cw, (c // GRID) * ch
    for y in range(ch):
        for x in range(cw):
            atlas.put(cx + x, cy + y, (0, 0, 0, 0))
    ox = x0
    oy = (y0 + y1 + 1) // 2 - side // 2
    for y in range(side):
        for x in range(side):
            ty = oy + y
            if 0 <= ty < ch:
                atlas.put(cx + ox + x, cy + ty, g.get(x, y))


def find_rsvg():
    return os.environ.get('RSVG_CONVERT') or shutil.which('rsvg-convert')


def warn(msg):
    print('padglyphs: WARNING: ' + msg, file=sys.stderr)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


class MissingRender(Exception):
    """A render the cache does not have, on a host without rsvg-convert."""


class Renderer:
    """rsvg-convert's PNGs: from the committed cache first (RENDER_CACHE), else rsvg-convert. Keeps
    what a build used (`used_pngs`, `used_boxes`), which --refresh-cache writes back."""

    def __init__(self, rsvg=None, cache_path=RENDER_CACHE, use_cache=True):
        self.rsvg = rsvg
        self.pngs, self.boxes, self.svgs = {}, {}, {}
        if use_cache and cache_path and os.path.exists(cache_path):
            with zipfile.ZipFile(cache_path) as z:
                for n in z.namelist():
                    if n.endswith('.png'):
                        self.pngs[n[:-4]] = z.read(n)
                self.boxes = json.loads(z.read('boxes.json').decode('utf-8'))
                self.svgs = json.loads(z.read('svgs.json').decode('utf-8'))
        self.used_pngs, self.used_boxes, self.used_svgs = {}, {}, {}
        self.rendered = 0   # renders the cache lacked (made by rsvg-convert)

    def run_rsvg(self, svg, name):
        if not self.rsvg:
            raise MissingRender(name)
        self.rendered += 1
        return subprocess.run([self.rsvg, '--format=png'], input=svg, check=True,
                              stdout=subprocess.PIPE).stdout

    def png(self, svg, name):
        key = sha256(svg)
        data = self.pngs.get(key)
        if data is None:
            data = self.run_rsvg(svg, name)
        self.used_pngs[key] = data
        return data

    def box(self, text, name, compute):
        key = sha256(text.encode('utf-8'))
        box = self.boxes.get(key)
        if box is None:
            if not self.rsvg:
                raise MissingRender(name)
            box = compute()
        self.used_boxes[key] = box
        self.used_svgs[name] = key
        return box

    def write(self, path):
        """The renders this Renderer used, as RENDER_CACHE (byte-stable: sorted, fixed dates)."""
        def entry(name, data, method):
            zi = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            zi.compress_type = method
            zi.external_attr = 0o644 << 16
            z.writestr(zi, data)
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, 'w') as z:
            entry('boxes.json', json.dumps(self.used_boxes, sort_keys=True, indent=0).encode(),
                  zipfile.ZIP_DEFLATED)
            entry('svgs.json', json.dumps(self.used_svgs, sort_keys=True, indent=0).encode(),
                  zipfile.ZIP_DEFLATED)
            for k in sorted(self.used_pngs):
                entry(k + '.png', self.used_pngs[k], zipfile.ZIP_STORED)
        with open(path, 'wb') as f:
            f.write(buf.getvalue())


def steam_game_dirs(game):
    """The installed PC game's directories Steam knows of (its registry key on Windows, the usual
    roots elsewhere; every library in libraryfolders.vdf)."""
    appid, default = STEAM_APPS[game]
    roots = []
    if sys.platform == 'win32':
        try:
            import winreg
            for hive, key, val in ((winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam', 'SteamPath'),
                                   (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Valve\Steam',
                                    'InstallPath')):
                try:
                    with winreg.OpenKey(hive, key) as k:
                        roots.append(winreg.QueryValueEx(k, val)[0])
                except OSError:
                    pass
        except ImportError:
            pass
        roots.append(r'C:\Program Files (x86)\Steam')
    else:
        home = os.path.expanduser('~')
        roots += [os.path.join(home, '.steam', 'steam'), os.path.join(home, '.local', 'share', 'Steam'),
                  os.path.join(home, 'Library', 'Application Support', 'Steam')]
    libs = []
    for r in roots:
        libs.append(r)
        vdf = os.path.join(r, 'steamapps', 'libraryfolders.vdf')
        if os.path.exists(vdf):
            with open(vdf, encoding='utf-8', errors='replace') as f:
                libs += [m.replace('\\\\', '\\') for m in re.findall(r'"path"\s+"([^"]+)"', f.read())]
    dirs = []
    for lib in libs:
        inst = default
        acf = os.path.join(lib, 'steamapps', 'appmanifest_%d.acf' % appid)
        if os.path.exists(acf):
            with open(acf, encoding='utf-8', errors='replace') as f:
                m = re.search(r'"installdir"\s+"([^"]+)"', f.read())
            inst = m.group(1) if m else inst
        d = os.path.abspath(os.path.join(lib, 'steamapps', 'common', inst))
        if os.path.isdir(d) and os.path.normcase(d) not in map(os.path.normcase, dirs):
            dirs.append(d)
    return dirs


def subdir(root, name):
    """root/name in any case (a Windows install's override/ is Override/ in KOTOR 1)."""
    if os.path.isdir(root):
        for n in os.listdir(root):
            if n.lower() == name.lower() and os.path.isdir(os.path.join(root, n)):
                return os.path.join(root, n)
    return None


def erf_resource(path, resref, restype):
    """One resource of an ERF / MOD (the PC games' TexturePacks), or None."""
    with open(path, 'rb') as f:
        head = f.read(160)
        if head[:4] not in (b'ERF ', b'MOD '):
            return None
        _, _, count, _, keys_at, res_at = struct.unpack_from('<6I', head, 8)
        f.seek(keys_at)
        keys = f.read(24 * count)
        for i in range(count):
            rr, _, rt, _ = struct.unpack_from('<16sIHH', keys, 24 * i)
            if rt == restype and rr.rstrip(b'\0').decode('latin-1').lower() == resref:
                f.seek(res_at + 8 * i)
                off, size = struct.unpack('<II', f.read(8))
                f.seek(off)
                return f.read(size)
    return None


def tpc_tga(data):
    """An uncompressed RGBA TPC (rows bottom-up, as a TGA's file order) as a Tga; None otherwise."""
    size, _, w, h, enc, _ = struct.unpack_from('<IfHHBB', data, 0)
    if size or enc != 4 or len(data) < 128 + w * h * 4:
        return None
    px = data[128:128 + w * h * 4]
    t = Tga.blank(w, h)
    t.px[0::4], t.px[1::4], t.px[2::4], t.px[3::4] = px[2::4], px[1::4], px[0::4], px[3::4]
    return t


def doubled(t):
    """t at twice its size, each texel 2 x 2."""
    u = Tga.blank(t.w * 2, t.h * 2)
    for y in range(t.h):
        for x in range(t.w):
            px = t.get(x, y)
            for dy in (0, 1):
                for dx in (0, 1):
                    u.put(2 * x + dx, 2 * y + dy, px)
    return u


def obb_files(obb_dir):
    """{lower-case base name: (obb path, offset, size)} of the raw-stored files in a directory's
    KOTOR II OBBs (main first, then patch: the later wins, as the game mounts them)."""
    sys.path.insert(0, HERE)
    import obbextract
    files = {}
    obbs = sorted(glob.glob(os.path.join(obb_dir, 'main.*.obb'))) + \
        sorted(glob.glob(os.path.join(obb_dir, 'patch.*.obb')))
    for o in obbs:
        with open(o, 'rb') as f:
            for p, off, us, cs in obbextract.read_index(f):
                if us == cs:
                    files[p.replace('\\', '/').rsplit('/', 1)[-1].lower()] = (o, off, us)
    return files


def k2_atlases(src_dirs, obb_dir=None, game_dirs=()):
    """(bases, load, where) of KOTOR II's atlases: bases {slot: file name} (a slot no source has
    is missing), load(name) -> Tga, where(name) -> the source it came from. Searched in src_dirs,
    then the OBBs, then the installed games' override/."""
    obb = obb_files(obb_dir) if obb_dir and os.path.isdir(obb_dir) else {}
    pc = [d for d in (subdir(g, 'override') for g in game_dirs) if d]

    def where(n):
        for d in src_dirs:
            if os.path.exists(os.path.join(d, n)):
                return os.path.join(d, n)
        if n.lower() in obb:
            return obb[n.lower()][0]
        for d in pc:
            if os.path.exists(os.path.join(d, n)):
                return os.path.join(d, n)
        return None

    def load(n):
        p = where(n)
        if p.lower().endswith('.obb'):
            o, off, size = obb[n.lower()]
            with open(o, 'rb') as f:
                f.seek(off)
                return Tga(f.read(size))
        with open(p, 'rb') as f:
            return Tga(f.read())

    # The atlas each slot starts from: 16 / 32 / 64 px (the Switch dump's dialogfont16x16b is the
    # APK's Xbox atlas, diafnt16x16b_an, but for a few font cells; so is the Steam install's).
    bases = {}
    for size, names in (('16an', ('diafnt16x16b_an.tga', 'dialogfont16x16b.tga')),
                        ('16ps', ('diafnt16x16b_ps.tga',)),
                        ('32ps', ('d2xfont16x16b_ps.tga',)),
                        ('64ps', ('d3xfont16x16b_ps.tga',))):
        n = next((n for n in names if where(n)), None)
        if n:
            bases[size] = n
        else:
            warn('no %s in %s, the OBBs in %s or an installed game (%s)'
                 % (' / '.join(names), ', '.join(src_dirs) or '-', obb_dir or '-',
                    ', '.join(game_dirs) or 'none found; --game-dir / K2P_GAME_DATA_DIR'))
    return bases, load, where


def build(src_dirs, resources=None, obb_dir=None, game_dirs=(), renderer=None):
    """{output name: TGA bytes}; a set whose sources are missing is left out (with a WARNING).

    Needs resources/ and its renders (Renderer). The shoulder glyphs ('l' / 'r', cells 0x15 / 0x16)
    of the Xbox and PlayStation sets: the floating icons cus_button_{l,r}{an,ps} and, since the
    game's own atlases cannot be extended in place, copies of them that carry the shoulders, under
    the port's names: k2fnt16_an (diafnt16x16b_an, the Xbox atlas every slot binds), k2fnt16_ps /
    k2fnt32_ps / k2fnt64_ps (diafnt16x16b_ps, d2xfont16x16b_ps, d3xfont16x16b_ps). And the whole
    Switch set (the module docstring)."""
    resources = resources or RESOURCES
    renderer = renderer or Renderer(find_rsvg())
    bases, load, where = k2_atlases(src_dirs, obb_dir, game_dirs)
    for size in sorted(bases):
        print('padglyphs: %s atlas %s from %s' % (size, bases[size], where(bases[size])))

    def svg(name):
        p = os.path.join(resources, name + '.svg')
        return Svg(p, renderer) if os.path.exists(p) else None

    out = {}
    # The Xbox and PlayStation sets: every button the prompts show -- A / B / X / Y and the shoulders
    # -- is the resources/ art, in the floating icons (cus_button_<k>an / <k>ps, over the game's
    # own) and in the atlas copies the texture rename binds (k2fnt*_an / _ps). The atlases give only
    # the font around them and the d-pad cell.
    arts = {(prefix, k): svg('%s_button_%s' % (prefix, art)) for prefix in ('xbox', 'ps')
            for k, art in list(zip('abxy', 'abxy')) + list(zip('lr', SHOULDERS[prefix]))}
    if all(arts.values()):
        try:
            part = {}
            for k in 'abxylr':
                part['cus_button_%san.tga' % k] = arts['xbox', k].as_drawn(94).bytes()
                part['cus_button_%sps.tga' % k] = arts['ps', k].as_drawn(94).bytes()
            for size, name, prefix in (('16an', 'k2fnt16_an', 'xbox'), ('16ps', 'k2fnt16_ps', 'ps'),
                                       ('32ps', 'k2fnt32_ps', 'ps'), ('64ps', 'k2fnt64_ps', 'ps')):
                if size in bases:
                    t = load(bases[size])
                    box = glyph_box(t)
                    for k in 'abxylr':
                        lay_cell(t, k, arts[prefix, k], box)
                    part[name + '.tga'] = t.bytes()
            out.update(part)
        except MissingRender as e:
            warn('%s; no Xbox / PlayStation shoulder set' % e)
    else:
        warn('no xbox_button_* / ps_button_* art in %s; no Xbox / PlayStation set' % resources)

    nx_bases = [('16', bases.get('16ps') or bases.get('16an')), ('32', bases.get('32ps')),
                ('64', bases.get('64ps'))]
    for prefix, suf, uibut, letters, label in (
            ('switch_k2', 'nx', NX_UIBUT, NX_LETTERS, 'Switch'),
            ('gc', 'gc', GC_UIBUT, GC_LETTERS, 'GameCube')):
        art = {a: svg('%s_button_%s' % (prefix, a))
               for a in set(uibut.values()) | set(letters.values())}
        missing = sorted(a for a, v in art.items() if not v)
        if missing or not all(n for _, n in nx_bases):
            warn('no %s_button_%s art or atlas; no %s set'
                 % (prefix, '/'.join(missing) or '*', label))
            continue
        try:
            part = {}
            for name, a in uibut.items():
                part['k2%s_%s.tga' % (suf, name)] = art[a].as_drawn().bytes()
            # The game's floating icons fill 80 x 80 of their 94, at (7, 3).
            for k in 'abxylr':
                t = Tga.blank(94, 94)
                g = art[letters[k]].fitted(80, 80)
                for y in range(80):
                    for x in range(80):
                        t.put(7 + x, 3 + y, g.get(x, y))
                part['cus_button_%s%s.tga' % (k, suf)] = t.bytes()
            for size, n in nx_bases:
                t = load(n)
                box = glyph_box(t)
                for k in ('a', 'b', 'x', 'y', 'dpad', 'l', 'r'):
                    lay_cell(t, k, art[letters[k]], box)
                part['k2fnt%s_%s.tga' % (size, suf)] = t.bytes()
            out.update(part)
        except MissingRender as e:
            warn('%s; no %s set' % (e, label))
    return out


def png_rgba(data):
    """An 8-bit RGBA, non-interlaced PNG (what rsvg-convert writes) -> (w, h, top-down RGBA rows)."""
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('not a PNG')
    o, idat, w = 8, b'', 0
    while o < len(data):
        n, kind = struct.unpack_from('>I4s', data, o)
        body = data[o + 8:o + 8 + n]
        if kind == b'IHDR':
            w, h, depth, ctype, _, _, interlace = struct.unpack('>IIBBBBB', body)
            if (depth, ctype, interlace) != (8, 6, 0):
                raise ValueError('PNG is not 8-bit RGBA non-interlaced')
        elif kind == b'IDAT':
            idat += body
        o += 12 + n
    raw, stride, rows, prev = zlib.decompress(idat), w * 4, [], bytearray(w * 4)
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b = prev[i]
            c = prev[i - 4] if i >= 4 else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line)
        prev = line
    return w, h, rows


class Svg:
    """One resources/ SVG, rendered at any size (Renderer: the cache, else rsvg-convert), optionally
    cropped to its visible part (so art drawn with different margins fills a glyph cell alike)."""

    def __init__(self, path, renderer):
        with open(path, encoding='utf-8') as f:
            self.text = f.read()
        self.name = os.path.basename(path)
        self.renderer = renderer
        root = re.search(r'<svg\b[^>]*>', self.text).group(0)
        vb = re.search(r'viewBox="([^"]+)"', root)
        self.root = root
        self.vb = [float(v) for v in vb.group(1).split()] if vb else [
            0.0, 0.0, float(re.search(r'width="([\d.]+)', root).group(1)),
            float(re.search(r'height="([\d.]+)', root).group(1))]

    def _svg(self, vb, size, height=None):
        """The SVG text rsvg-convert is given for one render (the cache's key)."""
        root = re.sub(r'\s(width|height|viewBox)="[^"]*"', '', self.root)
        root = root[:-1] + ' width="%d" height="%d" viewBox="%s">' % (
            size, height or size, ' '.join('%.4f' % v for v in vb))
        return self.text.replace(self.root, root, 1).encode()

    def _png(self, vb, size, height=None):
        return png_rgba(self.renderer.png(self._svg(vb, size, height), self.name))

    def _render(self, vb, size, height=None):
        w, h, rows = self._png(vb, size, height)
        t = Tga.blank(w, h)
        for y in range(h):
            for x in range(w):
                t.put(x, y, tuple(rows[y][x * 4:x * 4 + 4]))
        return t

    def visible_box(self):
        """The square (in SVG units) around the pixels drawn with any real opacity."""
        if not hasattr(self, '_box'):
            self._box = self.renderer.box(self.text, self.name, self._measure)
        return self._box

    def _measure(self):
        n = 512
        _, _, rows = png_rgba(self.renderer.run_rsvg(self._svg(self.vb, n), self.name))
        xs, ys = [], []
        for y, row in enumerate(rows):
            seen = [x for x in range(n) if row[x * 4 + 3] > 8]
            if seen:
                xs += (seen[0], seen[-1])
                ys.append(y)
        k = self.vb[2] / n
        x0, x1 = self.vb[0] + min(xs) * k, self.vb[0] + (max(xs) + 1) * k
        y0, y1 = self.vb[1] + min(ys) * k, self.vb[1] + (max(ys) + 1) * k
        side = max(x1 - x0, y1 - y0)
        return [(x0 + x1 - side) / 2, (y0 + y1 - side) / 2, side, side]

    def as_drawn(self, size=None):
        """The whole drawing, size x size, or at its own width x height."""
        if size is None:
            return self._render(self.vb, int(round(self.vb[2])), int(round(self.vb[3])))
        return self._render(self.vb, size)

    def fitted(self, size, inner):
        """The visible part inner x inner, centred on a size x size square."""
        x, y, side, _ = self.visible_box()
        pad = side * (size - inner) / inner / 2
        return self._render([x - pad, y - pad, side + 2 * pad, side + 2 * pad], size)


def k1_atlases(obb_dir, game_dirs=()):
    """({'16': TGA bytes, '32': TGA bytes}, where) of KOTOR 1's glyph atlases, dialogfont16x16b and
    d2xfont16x16b: from its patch OBB, else an installed PC game (override/, then the uncompressed
    TPC in TexturePacks/swpc_tex_gui.erf; no 2x atlas there: the 16-px one doubled). ({}, why)
    when neither has them."""
    obbs = sorted(glob.glob(os.path.join(obb_dir, 'patch.*.obb'))) if obb_dir else []
    if obbs:
        with zipfile.ZipFile(obbs[-1]) as z:
            names = {n.lower(): n for n in z.namelist()}
            got = {key: z.read(names[n]) for key, n in (('16', 'override/dialogfont16x16b.tga'),
                                                       ('32', 'override/d2xfont16x16b.tga'))
                   if n in names}
        if len(got) == 2:
            return got, obbs[-1]
    for g in game_dirs:
        found = {}
        for key, res in (('16', 'dialogfont16x16b'), ('32', 'd2xfont16x16b')):
            ov = subdir(g, 'override')
            if ov and os.path.exists(os.path.join(ov, res + '.tga')):
                with open(os.path.join(ov, res + '.tga'), 'rb') as f:
                    found[key] = (f.read(), os.path.join(ov, res + '.tga'))
                continue
            packs = subdir(g, 'TexturePacks')
            erf = packs and os.path.join(packs, 'swpc_tex_gui.erf')
            data = erf and os.path.exists(erf) and erf_resource(erf, res, 3007)
            t = data and tpc_tga(data)
            if t:
                found[key] = (t.bytes(), erf)
        if '16' in found:
            if '32' not in found:
                found['32'] = (doubled(Tga(found['16'][0])).bytes(), found['16'][1] + ' (doubled)')
            return {k: v[0] for k, v in found.items()}, '; '.join(
                '%s px: %s' % (k, found[k][1]) for k in ('16', '32'))
    return {}, ('no KOTOR 1 patch OBB with dialogfont16x16b / d2xfont16x16b (%s) and no installed '
                'PC game with dialogfont16x16b (%s)'
                % (os.path.join(obb_dir or '-', 'patch.*.obb'),
                   ', '.join(game_dirs) or 'none found; --game-dir / K2P_GAME_DATA_DIR'))


def build_k1(resources, obb_dir, game_dirs=(), renderer=None):
    """{output name: TGA bytes} for KOTOR 1: per style (an = Xbox, ps = PlayStation, nx = Switch,
    gc = GameCube) the six floating icons ios_button_<k><style> (a/b/x/y and the shoulders l/r) and the two
    glyph atlases k1fnt16_<style> / k1fnt32_<style> (K1's dialogfont16x16b / d2xfont16x16b with
    cells 0x10..0x13, 0x15, 0x16 redrawn); for the Switch also its shoulder and Start icons for
    the tutorial boxes, under the Xbox names the K1 Switch port filed them by (blackbutton = R,
    whitebutton = L, startbutton = +; the GameCube's Z / ZL / Start in the same slots). A style whose art is missing is left out, and the runtime
    then offers K1's own art in its place. K1's own art gets the shoulders too (below)."""
    renderer = renderer or Renderer(find_rsvg())
    atlases, where = k1_atlases(obb_dir, game_dirs)
    if not atlases:
        warn('%s; no K1 prompt art' % where)
        return {}
    print('padglyphs: K1 atlases from %s' % where)

    def svg(name):
        path = os.path.join(resources, name + '.svg')
        return Svg(path, renderer) if os.path.exists(path) else None

    icons = {}   # style -> {engine letter: Svg}
    for style, prefix, letters in (('an', 'xbox', {k: k for k in 'abxy'}),
                                   ('ps', 'ps', {k: k for k in 'abxy'}),
                                   ('nx', 'switch', {k: k for k in 'abxy'}),
                                   ('gc', 'gc', {k: k for k in 'abxy'})):
        letters = dict(letters, l=SHOULDERS[prefix][0], r=SHOULDERS[prefix][1])
        art = {k: svg('%s_button_%s' % (prefix, letters[k])) for k in letters}
        if all(art.values()):
            icons[style] = art
        else:
            warn('no %s_button_* art in %s; that K1 set is left out' % (prefix, resources))

    out = {}
    for style, art in sorted(icons.items()):
        try:
            part = {}
            for k, v in art.items():
                # The 94 x 94 icons as drawn; the Switch's and GameCube's 128 x 128 ones at their
                # visible size (the others fill 80 of the 94, K1's own 86).
                t = v.as_drawn(94) if v.vb[2:] == [94.0, 94.0] else v.fitted(94, 82)
                part['ios_button_%s%s.tga' % (k, style)] = t.bytes()
            for key, data in atlases.items():
                atlas = Tga(data)
                cw, ch = atlas.w // GRID, atlas.h // GRID
                # K1's own glyphs fill their cell but for a one-texel rim at the top (two at 32 px).
                m = max(1, cw // 16)
                size = cw - m
                for k, v in art.items():
                    c = CELL[k]
                    cx, cy = (c % GRID) * cw, (c // GRID) * ch
                    for y in range(ch):
                        for x in range(cw):
                            atlas.put(cx + x, cy + y, (0, 0, 0, 0))
                    g = v.fitted(size, size)
                    for y in range(size):
                        for x in range(size):
                            atlas.put(cx + m // 2 + x, cy + m + y, g.get(x, y))
                part['k1fnt%s_%s.tga' % (key, style)] = atlas.bytes()
            out.update(part)
        except MissingRender as e:
            warn('%s; no K1 %s set' % (e, style))
            del icons[style]
    # K1's own set (no Controller-page choice, or Automatic without a PlayStation pad) has no
    # shoulder art either: the iOS-style recreations, as ios_button_l / ios_button_r (the names
    # IosFloatingButtonIcon asks for) and in copies of K1's atlases (k1fnt16_io / k1fnt32_io) that
    # keep its own A/B/X/Y.
    own = {k: svg('ios_button_' + art) for k, art in zip('lr', SHOULDERS['ios'])}
    if all(own.values()):
        try:
            part = {}
            for k, v in own.items():
                part['ios_button_%s.tga' % k] = v.as_drawn(94).bytes()
            for key, data in atlases.items():
                atlas = Tga(data)
                for k, v in own.items():
                    lay_cell(atlas, k, v)
                part['k1fnt%s_io.tga' % key] = atlas.bytes()
            out.update(part)
        except MissingRender as e:
            warn('%s; no K1 own-set shoulders' % e)
    for style, prefix, arts in (('nx', 'switch', ('r', 'l', 'plus')), ('gc', 'gc', ('z', 'zl', 'start'))):
        if style not in icons:
            continue
        for name, art in zip(('blackbutton', 'whitebutton', 'startbutton'), arts):
            v = svg('%s_button_%s' % (prefix, art))
            if v:
                try:
                    out['%s_%s.tga' % (name, style)] = v.as_drawn(128).bytes()
                except MissingRender as e:
                    warn('%s; no K1 %s_%s' % (e, name, style))
    return out


def write_coff(blob, symbol, path):
    """An AMD64 COFF object with `blob` in .rdata under the external `symbol` (the cl.exe build:
    cl takes many minutes over tens of MB of string literals and caps one at 64 KB)."""
    import struct
    name = symbol.encode()
    strtab = struct.pack('<I', 4 + len(name) + 1) + name + b'\0'
    raw = 20 + 40
    symtab = raw + len(blob)
    head = struct.pack('<HHIIIHH', 0x8664, 1, 0, symtab, 1, 0, 0)
    # .rdata: initialized data, read-only, 16-byte aligned.
    sect = struct.pack('<8sIIIIIIHHI', b'.rdata', 0, 0, len(blob), raw, 0, 0, 0, 0,
                       0x40000040 | 0x00500000)
    sym = struct.pack('<IIIhHBB', 0, 4, 0, 1, 0, 2, 0)   # long name at string-table offset 4; external
    data = head + sect + blob + sym + strtab
    old = open(path, 'rb').read() if os.path.exists(path) else None
    if old != data:
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, 'wb') as f:
            f.write(data)


def write_cpp(assets, path, obj_path=None):
    lines = ['// GENERATED by platforms/tools/padglyphs.py — do not edit.',
             '// The button-prompt art qol/k2qol.cpp installs into the override folder at startup.',
             '#include <stddef.h>',
             '#include <stdint.h>', '']
    names = sorted(assets)
    if obj_path:
        # The bytes live in a COFF object of their own (write_coff); the table points into it.
        blob = bytearray()
        offsets = []
        for n in names:
            offsets.append(len(blob))
            blob += assets[n]
            blob += b'\0' * (-len(blob) % 16)
        write_coff(bytes(blob), 'k2_asset_blob', obj_path)
        lines.append('extern "C" const uint8_t k2_asset_blob[];')
    else:
        # String literals (octal escapes) rather than brace lists of numbers: the K1 set is ~4 MB, and
        # a literal compiles in a fraction of the time. The array is one byte longer for the NUL.
        for i, n in enumerate(names):
            data = assets[n]
            lines.append('static const char k_asset%d[%d] =' % (i, len(data) + 1))
            for o in range(0, len(data), 64):
                lines.append('  "' + ''.join('\\%03o' % b for b in data[o:o + 64]) + '"')
            lines.append(';')
    lines.append('')
    lines.append('extern "C" {')
    lines.append('struct K2Asset { const char* name; const uint8_t* data; size_t size; };')
    lines.append('extern const K2Asset k2_assets[];')
    lines.append('extern const unsigned k2_asset_count;')
    lines.append('const K2Asset k2_assets[] = {')
    for i, n in enumerate(names):
        if obj_path:
            lines.append('  {"%s", k2_asset_blob + %d, %d},' % (n, offsets[i], len(assets[n])))
        else:
            lines.append('  {"%s", reinterpret_cast<const uint8_t*>(k_asset%d), sizeof k_asset%d - 1},' % (n, i, i))
    if not names:
        lines.append('  {0, 0, 0},')
    lines.append('};')
    lines.append('const unsigned k2_asset_count = %d;' % len(names))
    lines.append('}')
    text = '\n'.join(lines) + '\n'
    old = None
    if os.path.exists(path):
        with open(path, encoding='utf-8') as f:
            old = f.read()
    if old != text:
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)


def check(game, src_dirs, obb_dir, game_dirs):
    """What a build would lack, as WARNING lines (CMake's configure step prints them). Cheap: finds
    the atlases and compares resources/' SVGs with the render cache, renders nothing."""
    rsvg = find_rsvg()
    r = Renderer(rsvg)
    if not r.svgs and not rsvg:
        warn('no render cache (%s) and no rsvg-convert; no prompt art' % RENDER_CACHE)
    elif not rsvg:
        for p in sorted(glob.glob(os.path.join(RESOURCES, '*.svg'))):
            n = os.path.basename(p)
            with open(p, encoding='utf-8') as f:
                key = sha256(f.read().encode('utf-8'))
            if r.svgs.get(n) != key:
                warn('%s is new or changed since %s was made and there is no rsvg-convert: its set is dropped '
                     '(render it where librsvg is: padglyphs.py --refresh-cache)'
                     % (n, os.path.basename(RENDER_CACHE)))
    if game == 'k1':
        atlases, why = k1_atlases(obb_dir, game_dirs)
        if not atlases:
            warn('%s; no K1 prompt art' % why)
        else:
            print('padglyphs: K1 atlases from %s' % why)
    else:
        bases, _, where = k2_atlases(src_dirs, obb_dir, game_dirs)
        for size in sorted(bases):
            print('padglyphs: %s atlas %s from %s' % (size, bases[size], where(bases[size])))


def refresh_cache(path, game_dirs):
    """Re-render every PNG a build can ask for (the Android data's atlases and the PC games' --
    their glyph boxes differ, and so do the renders' sizes) and write them to `path`."""
    rsvg = find_rsvg()
    if not rsvg:
        raise SystemExit('padglyphs: --refresh-cache needs rsvg-convert (brew install librsvg)')
    r = Renderer(rsvg, use_cache=False)
    k2pc = [d for d in game_dirs.get('k2', [PC_DATA['k2']]) if os.path.isdir(d)]
    k1pc = [d for d in game_dirs.get('k1', [PC_DATA['k1']]) if os.path.isdir(d)]
    variants = [('k2 Android data', lambda: build([SRC_DIR, APK_DIR], obb_dir=K2_OBB_DIR, renderer=r)),
                ('k2 PC data', lambda: build([], game_dirs=k2pc, renderer=r)),
                ('k1 Android data', lambda: build_k1(RESOURCES, K1_OBB_DIR, renderer=r)),
                ('k1 PC data', lambda: build_k1(RESOURCES, '', k1pc, renderer=r))]
    for label, run in variants:
        n = len(run())
        print('padglyphs: %s: %d asset(s)' % (label, n))
    # Every SVG's crop box, whether or not this data set fits it (an SVG no set uses yet).
    for p in sorted(glob.glob(os.path.join(RESOURCES, '*.svg'))):
        Svg(p, r).visible_box()
    r.write(path)
    print('padglyphs: %d render(s), %d box(es) -> %s (%d bytes)'
          % (len(r.used_pngs), len(r.used_boxes), path, os.path.getsize(path)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--game', choices=('k2', 'k1'), default='k2')
    ap.add_argument('--src-dir', default=SRC_DIR)
    ap.add_argument('--obb-dir', default='', help='the game\'s Android OBBs (default: decomp_bins/<game>_obb)')
    ap.add_argument('--game-dir', action='append', default=[],
                    help='an installed PC game, the atlases\' last resort (default: Steam\'s)')
    ap.add_argument('--out-cpp')
    ap.add_argument('--out-obj', help='the bytes as an AMD64 COFF object instead of literals (cl.exe)')
    ap.add_argument('--png-dir', help='also write PNG previews (needs Pillow)')
    ap.add_argument('--no-rsvg', action='store_true', help='render from the cache only (test a Windows host)')
    ap.add_argument('--check', action='store_true', help='report what a build would lack, build nothing')
    ap.add_argument('--refresh-cache', action='store_true', help='re-render %s' % RENDER_CACHE)
    a = ap.parse_args()
    if a.no_rsvg:
        os.environ['RSVG_CONVERT'] = ''
        globals()['find_rsvg'] = lambda: None
    game_dirs = [os.path.abspath(d) for d in a.game_dir if d] or steam_game_dirs(a.game)
    obb_dir = a.obb_dir or (K1_OBB_DIR if a.game == 'k1' else K2_OBB_DIR)
    if a.refresh_cache:
        refresh_cache(RENDER_CACHE, {a.game: game_dirs} if a.game_dir else {})
        return
    if a.check:
        check(a.game, [a.src_dir, APK_DIR], obb_dir, game_dirs)
        return
    if not a.out_cpp:
        ap.error('--out-cpp is required')
    r = Renderer(find_rsvg())
    if a.game == 'k1':
        assets = build_k1(RESOURCES, obb_dir, game_dirs, r)
    else:
        assets = build([a.src_dir, APK_DIR], obb_dir=obb_dir, game_dirs=game_dirs, renderer=r)
    if r.rendered:
        warn('%d render(s) are not in %s: run padglyphs.py --refresh-cache and commit it, or a host '
             'without rsvg-convert drops those sets' % (r.rendered, os.path.basename(RENDER_CACHE)))
    write_cpp(assets, a.out_cpp, a.out_obj)
    if a.png_dir and assets:
        from PIL import Image
        os.makedirs(a.png_dir, exist_ok=True)
        for n, data in assets.items():
            Image.open(io.BytesIO(data)).save(os.path.join(a.png_dir, n[:-4] + '.png'))
    print('padglyphs: %d asset(s), %d bytes -> %s'
          % (len(assets), sum(len(v) for v in assets.values()), a.out_cpp))


if __name__ == '__main__':
    main()
