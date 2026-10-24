"""Usage: python3 platforms/tools/savefix_dupclass.py <in SAVEGAME.sav> <out SAVEGAME.sav>
Drop duplicate-class ClassList entries from every UTC in a KotOR2 SAVEGAME.sav (in place, size-neutral):
the list count is cut to the first occurrence of each class id when duplicates sit at the tail."""
import struct, sys
sys.path.insert(0, __import__("os").path.join(__import__("os").path.dirname(__import__("os").path.abspath(__file__)), "..", "..", "tools"))
import gffdump
src, dst = sys.argv[1], sys.argv[2]
d = bytearray(open(src, 'rb').read())
(nloc, _ls, n, _lo, okeys, ores) = struct.unpack_from('<IIIIII', d, 8)
fixed = []
for i in range(n):
    rr, _id, t, _u = struct.unpack_from('<16sIHH', d, okeys + i * 24)
    if t != 2027: continue
    o, sz = struct.unpack_from('<II', d, ores + i * 8)
    g = d[o:o + sz]
    (so, sc, fo, fc, lo, lc, fdo, fdc, fio, fic, lio, lic) = struct.unpack_from('<12I', g, 8)
    labels = [bytes(g[lo + k * 16:lo + k * 16 + 16]).rstrip(b'\0').decode('latin-1') for k in range(lc)]
    for f in range(fc):
        ft, li, v = struct.unpack_from('<III', g, fo + f * 12)
        if ft != 15 or labels[li] != 'ClassList': continue
        cnt = struct.unpack_from('<I', g, lio + v)[0]
        idx = struct.unpack_from('<%dI' % cnt, g, lio + v + 4)
        parsed = gffdump.gff(bytes(g))
        classes = [c.get('Class') for c in parsed.get('ClassList', [])]
        keep = []
        for c in classes:
            if c in keep: break
            keep.append(c)
        if len(keep) < cnt and len(set(classes)) == len(keep):
            struct.pack_into('<I', d, o + lio + v, len(keep))
            fixed.append('%s: ClassList %s -> %s' % (rr.rstrip(b'\0').decode(), classes, classes[:len(keep)]))
open(dst, 'wb').write(d)
print('\n'.join(fixed) or 'nothing to fix')
res = gffdump.read_archive(dst)
for k in sorted(r for r in res if r.endswith('.utc')):
    g = gffdump.gff(res[k]); print('  verify', k, [(c.get('Class'), c.get('ClassLevel')) for c in g.get('ClassList', [])])
