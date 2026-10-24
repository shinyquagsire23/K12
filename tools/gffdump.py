#!/usr/bin/env python3
"""gffdump.py -- list / dump resources from a KOTOR RIM or ERF/MOD, decoding GFF ones.

When a scripted or data-driven behaviour silently does nothing on the host, read the DATA before
the code: is the door really KeyRequired, which script does the dialog fire, what does the .utp
say.  (v1997 found `a_to_002ebo.ncs` this way; v1998 the 001EBO engine room door's KeyRequired.)

Usage:
    python3 tools/gffdump.py MODULE.rim                    # list resources (resref.type, size)
    python3 tools/gffdump.py MODULE.rim RESREF.EXT         # dump one GFF (top struct, lists nested)
    python3 tools/gffdump.py MODULE.rim '*.utd' -f KeyRequired,Locked,Tag,LocName
                                                           # one line per matching GFF, chosen fields
"""
import fnmatch
import struct
import sys

TYPES = {2002: 'mdl', 2009: 'nss', 2010: 'ncs', 2012: 'are', 2014: 'ifo', 2023: 'git', 2025: 'uti',
         2027: 'utc', 2029: 'dlg', 2032: 'utt', 2035: 'uts', 2040: 'ute', 2042: 'utd', 2044: 'utp',
         2047: 'gui', 2051: 'utm', 2052: 'dwk', 2053: 'pwk', 2058: 'utw', 3000: 'lyt', 3001: 'vis',
         3007: 'tpc', 3008: 'mdx', 2017: '2da', 2016: 'wok', 2007: 'lip', 4: 'wav', 2037: 'jrl',
         2045: 'stg' if False else 'fac', 2030: 'itp', 2038: 'utg' if False else 'fac'}


def read_archive(path):
    d = open(path, 'rb').read()
    sig = d[:4]
    out = {}
    if sig == b'RIM ':
        n, off = struct.unpack_from('<II', d, 12)
        for i in range(n):
            rr, t, _id, o, sz = struct.unpack_from('<16sIIII', d, off + i * 32)
            out[rr.rstrip(b'\0').decode().lower() + '.' + TYPES.get(t, str(t))] = d[o:o + sz]
    else:                                         # ERF / MOD
        (nloc, _ls, n, _lo, okeys, ores) = struct.unpack_from('<IIIIII', d, 8)
        for i in range(n):
            rr, _id, t, _u = struct.unpack_from('<16sIHH', d, okeys + i * 24)
            o, sz = struct.unpack_from('<II', d, ores + i * 8)
            out[rr.rstrip(b'\0').decode().lower() + '.' + TYPES.get(t, str(t))] = d[o:o + sz]
    return out


def gff(d):
    (so, sc, fo, fc, lo, lc, fdo, fdc, fio, fic, lio, lic) = struct.unpack_from('<12I', d, 8)
    labels = [d[lo + i * 16:lo + i * 16 + 16].rstrip(b'\0').decode('latin-1') for i in range(lc)]

    def field(i):
        t, li, v = struct.unpack_from('<III', d, fo + i * 12)
        name = labels[li]
        if t in (0, 1, 2, 3, 4, 5):                      # byte char word short dword int
            fmt = {0: 'B', 1: 'b', 2: 'H', 3: 'h', 4: 'I', 5: 'i'}[t]
            return name, struct.unpack(fmt, struct.pack('<I', v)[:struct.calcsize(fmt)])[0]
        if t == 8:
            return name, struct.unpack('<f', struct.pack('<I', v))[0]
        if t in (6, 7, 9):
            return name, struct.unpack_from('<' + ('Q' if t == 6 else 'q' if t == 7 else 'd'), d, fdo + v)[0]
        if t == 10:                                      # CExoString
            n = struct.unpack_from('<I', d, fdo + v)[0]
            return name, d[fdo + v + 4:fdo + v + 4 + n].decode('latin-1')
        if t == 11:                                      # ResRef
            n = d[fdo + v]
            return name, d[fdo + v + 1:fdo + v + 1 + n].decode('latin-1')
        if t == 12:                                      # LocString
            _tot, strref, cnt = struct.unpack_from('<IiI', d, fdo + v)
            s = ''
            if cnt:
                _lid, ln = struct.unpack_from('<II', d, fdo + v + 12)
                s = d[fdo + v + 20:fdo + v + 20 + ln].decode('latin-1')
            return name, f'strref {strref} {s!r}'
        if t == 14:
            return name, struct_(v)
        if t == 15:
            n = struct.unpack_from('<I', d, lio + v)[0]
            return name, [struct_(struct.unpack_from('<I', d, lio + v + 4 + 4 * k)[0]) for k in range(n)]
        if t == 17:
            return name, struct.unpack_from('<3f', d, fdo + v)
        if t == 16:
            return name, struct.unpack_from('<4f', d, fdo + v)
        return name, f'<type {t}>'

    def struct_(i):
        _ty, di, n = struct.unpack_from('<III', d, so + i * 12)
        if n == 0:                                       # empty struct (DataOrDataOffset = -1)
            return {}
        if n == 1:
            return dict([field(di)])
        idx = struct.unpack_from(f'<{n}I', d, fio + di)
        return dict(field(k) for k in idx)

    return struct_(0)


def show(v, ind=0):
    pad = '  ' * ind
    for k, x in v.items():
        if isinstance(x, list):
            print(f'{pad}{k}: [{len(x)}]')
            for n, e in enumerate(x[:40]):
                print(f'{pad}  [{n}]')
                show(e, ind + 2)
        elif isinstance(x, dict):
            print(f'{pad}{k}:')
            show(x, ind + 1)
        else:
            print(f'{pad}{k} = {x!r}')


def main():
    a = sys.argv[1:]
    res = read_archive(a[0])
    if len(a) == 1:
        for k, v in sorted(res.items()):
            print(f'{k:24s} {len(v)}')
        return
    fields = None
    if '-f' in a:
        fields = a[a.index('-f') + 1].split(',')
    for k in sorted(res):
        if not fnmatch.fnmatch(k, a[1].lower()):
            continue
        g = gff(res[k])
        if fields:
            print(k, ' '.join(f'{f}={g.get(f)!r}' for f in fields))
        else:
            print('==', k)
            show(g)


if __name__ == '__main__':
    main()
