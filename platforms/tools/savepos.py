#!/usr/bin/env python3
"""savepos.py [--row1] [--xp N] [--module NAME] SAVEDIR X Y [Z] -- move the PC of a save: patch the
X/Y/ZPosition floats of SAVEDIR/savegame.sav -> <module>.sav -> module.ifo Mod_PlayerList[0] IN PLACE
(no size change, so both ERF layers stay valid). Prints old -> new.

Use it on a COPY of a save directory (`cp -R "000025 - Game24" "000099 - Test"`): it rewrites the
file. A waypoint from the module's .git (tools/gffdump.py) is a safe spot; an arbitrary point can be
off the walkmesh, and the PC then cannot path anywhere. Made for reproducing a bug at one spot of a
module without walking there (v2084: the Peragus prison room and the MedBay door).

--row1 drives the LOAD LIST to that copy: it stamps savenfo.res's TIMESTAMP (100-ns ticks since
1601) and SAVENUMBER (900), so `K2_INPUT`'s "first row" lands on it (the v2085 recipe, now one
command). Without it a plain copy keeps the source's timestamp and lists as an indistinguishable
twin somewhere in the middle.

--module NAME loads straight into a module the save has already visited (its archive is in
SAVEGAME.sav) by rewriting savenfo.res's LASTMODULE -- the string CSWGuiSaveLoad::LoadGame hands to
SendLoadGameRequest as the module to load. The position patch then targets THAT module's
Mod_PlayerList[0], so `--module 301nar X Y Z` is a one-command teleport into Nar Shaddaa. It warns
when the save carries no archive for the named module (then it falls back to the real one, and
landing in the new module needs the console `warp` path instead of this one)."""
import struct, sys
def erf_entries(d, base=0):
    (nloc, _ls, n, _lo, okeys, ores) = struct.unpack_from('<IIIIII', d, base + 8)
    out = {}
    for i in range(n):
        rr, _id, t, _u = struct.unpack_from('<16sIHH', d, base + okeys + i * 24)
        o, sz = struct.unpack_from('<II', d, base + ores + i * 8)
        out[(rr.rstrip(b'\0').decode().lower(), t)] = (base + o, sz)
    return out
def last_module(sd):
    """The module the PC was in, from savenfo.res's LASTMODULE (lowercased).

    A save contains one 2057 archive PER module loaded at save time (the Ebon Hawk saves in
    platforms/out/gamedir-android/saves carry both 001ebo and 002ebo), and only the PC's own
    module's module.ifo holds the player position -- patching the other one silently does
    nothing. v2200: seen on "000016 - Game15" (001ebo), whose first 2057 entry is 002ebo.
    """
    nfo = open(sd + '/savenfo.res', 'rb').read()
    so, sc, fo, fc, lo, lc, fdo = struct.unpack_from('<12I', nfo, 8)[:7]
    labels = [nfo[lo + i * 16:lo + i * 16 + 16].rstrip(b'\0').decode('latin-1')
              for i in range(lc)]
    # savenfo is FLAT: one struct, whose field COUNT lives in the struct table (sc is 1).
    _ty, _di, nfields = struct.unpack_from('<III', nfo, so)
    for k in range(nfields):
        t, li, v = struct.unpack_from('<III', nfo, fo + k * 12)
        if labels[li] != 'LASTMODULE':
            continue
        n = struct.unpack_from('<I', nfo, fdo + v)[0]
        return nfo[fdo + v + 4:fdo + v + 4 + n].decode('latin-1').lower()
    return None

def gff_fields(d, base, path):
    so, sc, fo, fc, lo, lc, fdo, fdc, fio, fic, lio, lic = struct.unpack_from('<12I', d, base + 8)
    labels = [d[base+lo+i*16:base+lo+i*16+16].rstrip(b'\0').decode('latin-1') for i in range(lc)]
    def sfields(si):
        _ty, di, n = struct.unpack_from('<III', d, base + so + si * 12)
        if n == 0: return []
        if n == 1: return [di]
        return list(struct.unpack_from(f'<{n}I', d, base + fio + di))
    def fld(fi):
        t, li, v = struct.unpack_from('<III', d, base + fo + fi * 12)
        return t, labels[li], v, base + fo + fi * 12 + 8
    # top struct -> Mod_PlayerList -> [0]
    for fi in sfields(0):
        t, name, v, _ = fld(fi)
        if name == 'Mod_PlayerList':
            n = struct.unpack_from('<I', d, base + lio + v)[0]
            s0 = struct.unpack_from('<I', d, base + lio + v + 4)[0]
            return {fld(k)[1]: fld(k) for k in sfields(s0)}
if len(sys.argv) < 4:
    sys.exit(__doc__)
def take_opt(name, cast):
    """Pull `--NAME VALUE` out of argv; returns the value or None."""
    global sys_argv
    if name not in sys_argv:
        return None
    i = sys_argv.index(name)
    v = cast(sys_argv[i + 1])
    del sys_argv[i:i + 2]
    return v

sys_argv = list(sys.argv[1:])
nXp = take_opt('--xp', int)            # absolute Experience for Mod_PlayerList[0]
sModule = take_opt('--module', str)    # load straight into THIS module's archive
argv = [a for a in sys_argv if a != '--row1']
bRow1 = '--row1' in sys_argv
sd, x, y = argv[0], float(argv[1]), float(argv[2])
z = float(argv[3]) if len(argv) > 3 else None
p = sd + '/savegame.sav'
d = bytearray(open(p, 'rb').read())
ents = erf_entries(d)
lm = sModule.lower() if sModule is not None else last_module(sd)
cand = [k for k in ents if k[1] == 2057]
mod = [k for k in cand if k[0] == lm] or cand
if len(cand) > 1 and lm is not None:
    print('module entries', [k[0] for k in cand], '-> patching', mod[0][0])
if sModule is not None and mod[0][0] != lm:
    print('⚠ --module %s: the save carries no archive for it -- the position patch lands in %s;'
          ' set LASTMODULE only if the engine can build the module from kick-off state' % (lm, mod[0][0]))
mod = mod[0]
mo, _ = ents[mod]
inner = erf_entries(d, mo)
io, _ = [v for k, v in inner.items() if k[0] == 'module'][0]
f = gff_fields(d, io, None)
for k, val in (('XPosition', x), ('YPosition', y), ('ZPosition', z)):
    t, name, v, off = f[k]
    old = struct.unpack_from('<f', d, off)[0]
    if val is not None:
        struct.pack_into('<f', d, off, val)
    print(k, old, '->', struct.unpack_from('<f', d, off)[0])

# --xp: the level-up lever. The PC is restored from this Mod_PlayerList entry
# (CSWSPlayer::LoadCharacterFromIFO -> CSWSCreatureStats::ReadStatsFromGff), so raising its
# Experience makes CSWSCreatureStats::CanLevelUp() true on the next load and the Character panel
# grows its level up button -- no debugger and no console needed. (The `pc.utc` copy is a stale
# snapshot and nothing reads it.)
if nXp is not None:
    t, name, v, off = f['Experience']
    if t in (0, 1, 2, 3, 4, 5):
        old = struct.unpack_from('<I', d, off)[0]
        struct.pack_into('<I', d, off, nXp)
        print('Experience', old, '->', struct.unpack_from('<I', d, off)[0])
    else:
        print('Experience: field type %d is not an in-place integer -- NOT patched' % t)
open(p, 'wb').write(d)

def is_inline(t):
    """A GFF field's value lives INLINE in the field record for these types (see
    CResGFF::IsDataInPlace); for every other type nDataOrDataOffset is an offset into the
    FieldData/FieldIndices block."""
    return t in (0, 1, 2, 3, 4, 5)

def savenfo_fields(d2):
    """(fieldTableOffset, fieldDataOffset, labels, fieldCount) of a flat savenfo-style GFF."""
    so, _sc, fo, _fc, lo, lc, fdo = struct.unpack_from('<12I', d2, 8)[:7]
    labels = [d2[lo + i * 16:lo + i * 16 + 16].rstrip(b'\0').decode('latin-1') for i in range(lc)]
    _ty, _di, nfields = struct.unpack_from('<III', d2, so)
    return fo, fdo, labels, nfields

def value_offset(d2, fo, fdo, k):
    """Byte offset of field record `k`'s VALUE: the record's own slot for an in-place type (see
    is_inline), otherwise FieldData at nDataOrDataOffset. Everything that reads or writes a
    savenfo field must go through this -- the two shapes are NOT interchangeable (v2200 assumed
    they were and corrupted the save)."""
    t, _li, v = struct.unpack_from('<III', d2, fo + k * 12)
    return fo + k * 12 + 8 if is_inline(t) else fdo + v

def field_index(d2, fo, labels, nfields, name):
    for k in range(nfields):
        if labels[struct.unpack_from('<III', d2, fo + k * 12)[1]] == name:
            return k
    return None

def read_timestamp(d2):
    """savenfo's TIMESTAMP as 100-ns ticks since 1601 (UTC), or None."""
    fo, fdo, labels, nfields = savenfo_fields(d2)
    k = field_index(d2, fo, labels, nfields, 'TIMESTAMP')
    return None if k is None else struct.unpack_from('<Q', d2, value_offset(d2, fo, fdo, k))[0]

def read_cstring(d2, off):
    """The CExoString stored at `off` (a 4-byte length then the bytes)."""
    n = struct.unpack_from('<I', d2, off)[0]
    return d2[off + 4:off + 4 + n].decode('latin-1')

def write_cstring(d2, fo, fdo, k, s):
    """Write a type-10 field. A shorter-or-equal value goes IN PLACE (NUL padded, so no other
    field's data moves); a longer one is APPENDED and the field retargeted, with the header's
    FieldDataCount bumped to match. The rule is the v2200 SAVENUMBER lesson generalised: know
    where the bytes of THIS field end before writing, and re-read afterwards."""
    b = s.encode('latin-1')
    off = value_offset(d2, fo, fdo, k)
    old = struct.unpack_from('<I', d2, off)[0]
    if len(b) <= old:
        struct.pack_into('<I', d2, off, len(b))
        d2[off + 4:off + 4 + old] = b + b'\0' * (old - len(b))
    else:
        new = len(d2)
        d2 += struct.pack('<I', len(b)) + b
        t, li = struct.unpack_from('<II', d2, fo + k * 12)
        struct.pack_into('<III', d2, fo + k * 12, t, li, new)
        struct.pack_into('<I', d2, 8 + 7 * 4, struct.unpack_from('<I', d2, 8 + 7 * 4)[0] + 4 + len(b))

if sModule is not None:
    # --module warps the load itself: CSWGuiSaveLoad::LoadGame passes savenfo's LASTMODULE as the
    # module name of SendLoadGameRequest, and a save carries one 2057 archive per module the player
    # has visited, so rewriting the name (plus that archive's own Mod_PlayerList position, which the
    # branch above already did) lands the PC in that module with no walking, no galaxy map and no
    # console. K2 module names are all 6 chars, so the patch is same-size and shifts nothing.
    pn = sd + '/savenfo.res'
    d2 = bytearray(open(pn, 'rb').read())
    fo2, fdo2, labels2, nf2 = savenfo_fields(d2)
    kM = field_index(d2, fo2, labels2, nf2, 'LASTMODULE')
    if kM is None:
        print('savenfo: no LASTMODULE field -- --module NOT applied')
    else:
        off = value_offset(d2, fo2, fdo2, kM)
        old = read_cstring(d2, off)
        write_cstring(d2, fo2, fdo2, kM, sModule)
        open(pn, 'wb').write(d2)
        print('savenfo: LASTMODULE %r -> %r'
              % (old, read_cstring(open(pn, 'rb').read(), off)))

if bRow1:
    # savenfo.res is a flat GFF (one struct; the field count lives in the struct table, not in the
    # header's StructCount). SAVENUMBER is an inline field (type 4, value in the field record) while
    # TIMESTAMP is not (type 6, value at nDataOrDataOffset): writing both as `fdo + v` leaves
    # SAVENUMBER unchanged and lands the other write in the FieldData block, which overwrote the last
    # four bytes of SAVEGAMENAME ('Current' -> 'Cur\x84\x03\0\0') on '000201 - dev' -- the name the
    # Load Game list shows. Always branch on the field type here.
    import time
    import os
    import glob
    p = sd + '/savenfo.res'
    d2 = bytearray(open(p, 'rb').read())
    fo, fdo, labels, nfields = savenfo_fields(d2)

    # Row 1 is decided by TIMESTAMP, and a device save can carry a timestamp far in the future
    # ('000101 - DroidTest' is 2026-11-14 while the host clock read 2026-10-11), so stamping "now"
    # does not make this save the first row -- the harness then silently loads a different save.
    # Stamp above every sibling save instead (`K2_ROW1_NOW=1` restores the plain "now" stamp).
    ts = int((time.time() + 11644473600) * 10 ** 7)
    if os.environ.get('K2_ROW1_NOW', '') == '':
        newest = None
        for other in glob.glob(os.path.dirname(os.path.abspath(sd)) + '/*/savenfo.res'):
            if os.path.abspath(other) == os.path.abspath(p):
                continue
            try:
                o = read_timestamp(open(other, 'rb').read())
            except Exception:
                continue
            if o is not None and (newest is None or o > newest):
                newest = o
        if newest is not None and newest >= ts:
            ts = newest + 60 * 10 ** 7

    got = 0
    kTS = field_index(d2, fo, labels, nfields, 'TIMESTAMP')
    kNo = field_index(d2, fo, labels, nfields, 'SAVENUMBER')
    if kTS is not None:
        struct.pack_into('<Q', d2, value_offset(d2, fo, fdo, kTS), ts)
        got += 1
    if kNo is not None:
        struct.pack_into('<I', d2, value_offset(d2, fo, fdo, kNo), 900)
        got += 1
    open(p, 'wb').write(d2)

    # verify from the bytes we just wrote, so a silent mis-write cannot pass as "row1 stamp"
    d2 = open(p, 'rb').read()
    fo, fdo, labels, nfields = savenfo_fields(d2)
    seen = {}
    for k in range(nfields):
        name = labels[struct.unpack_from('<III', d2, fo + k * 12)[1]]
        if name == 'TIMESTAMP':
            seen[name] = struct.unpack_from('<Q', d2, value_offset(d2, fo, fdo, k))[0]
        elif name in ('SAVENUMBER', 'TIMEPLAYED'):
            seen[name] = struct.unpack_from('<I', d2, value_offset(d2, fo, fdo, k))[0]
        elif name == 'SAVEGAMENAME':
            o = value_offset(d2, fo, fdo, k)
            n = struct.unpack_from('<I', d2, o)[0]
            seen[name] = d2[o + 4:o + 4 + n].decode('latin-1', 'replace')
    if got == 2:
        print('savenfo: row1 stamp TIMESTAMP=%d SAVENUMBER=%s SAVEGAMENAME=%r'
              % (seen.get('TIMESTAMP', -1), seen.get('SAVENUMBER'), seen.get('SAVEGAMENAME', '?')))
    else:
        print('savenfo: %d/2 fields found -- row1 NOT stamped' % got)
