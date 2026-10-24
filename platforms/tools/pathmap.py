#!/usr/bin/env python3
"""pathmap.py -- which ORIGINAL source file does each of our TUs' code come from?

Writes docs/PATH_MIGRATION_MAP.md, the generated half of docs/PATH_MIGRATION_PLAN.md.

Oracle: the macOS Steam build (decomp_bins/KOTOR2_MacOS/KOTOR2sub) still carries stabs. Every
function it emitted sits under an N_SO naming its source file, e.g.
  /Volumes/.../KOTOR2/Port/CODE/libsource/AURORA/aurmodel.cpp
Each of those functions is looked up in OUR tree (the host engine archive from platforms/, which is
compiled from src/ as-is) to find the TU that defines it today.

Noise filter: only STRONG definitions count on both sides. A weak symbol (inline member, template
instance, implicit special member) is emitted by every object that uses it, so on the Mac side it
would pin a function to whichever file happened to emit it first, and on our side it says nothing
about where the body lives. Needs a finished `cmake --build --preset k2-host-pc`.
"""
import argparse, collections, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, 'hybrid', 'tools'))
import linkreport as LR   # noqa: E402
from ccflags import read_cflags   # noqa: E402
sys.path.insert(0, os.path.join(REPO, 'tools'))
import fnindex      # noqa: E402  the function table
from tuname import tu_name   # noqa: E402

MAC = os.path.join(REPO, 'decomp_bins/KOTOR2_MacOS/KOTOR2sub')
N_STAB, N_TYPE, N_SECT, N_WEAK_DEF = 0xe0, 0x0e, 0x0e, 0x0080
N_SO, N_FUN = 0x64, 0x24


def mac_functions():
    """-> [(original path, name)] for every N_FUN, and the set of weak-defined names."""
    data = open(MAC, 'rb').read()
    base = 0
    if struct.unpack_from('>I', data, 0)[0] == 0xCAFEBABE:
        n = struct.unpack_from('>I', data, 4)[0]
        for i in range(n):
            off = struct.unpack_from('>IIIII', data, 8 + 20 * i)[2]
            if struct.unpack_from('<I', data, off)[0] == 0xFEEDFACF:
                base = off
    ncmds = struct.unpack_from('<I', data, base + 16)[0]
    p = base + 32
    for _ in range(ncmds):
        cmd, sz = struct.unpack_from('<II', data, p)
        if cmd == 2:
            symoff, nsyms, stroff, strsize = struct.unpack_from('<IIII', data, p + 8)
        p += sz
    st = data[base + stroff:base + stroff + strsize]
    funs, weak = [], set()
    cur_dir = cur = ''
    for k in range(nsyms):
        strx, t, _s, desc, _v = struct.unpack_from('<IBBhQ', data, base + symoff + k * 16)
        e = st.find(b'\0', strx)
        nm = st[strx:e].decode('utf8', 'replace')
        if t & N_STAB:
            if t == N_SO:
                if nm.endswith('/'):
                    cur_dir = nm
                elif nm:
                    cur = cur_dir + nm
            elif t == N_FUN and nm:
                funs.append((cur, nm[1:] if nm.startswith('_') else nm))
        elif (t & N_TYPE) == N_SECT and (desc & N_WEAK_DEF):
            weak.add(nm[1:] if nm.startswith('_') else nm)
    return funs, weak


def orig_path(p):
    """'/Volumes/.../Port/Mac/../CODE/libsource/math3d/math3d.cpp' -> 'CODE/libsource/math3d/math3d.cpp'."""
    p = p.split('/Port/', 1)[-1]
    p = os.path.normpath(p)
    return p


def src_index():
    """TU name -> repo-relative .cpp. TU names follow tools/tuname.py (a stem two files share is
    `<dir>_<stem>`)."""
    idx = {}
    for root, _d, files in os.walk(os.path.join(REPO, 'src')):
        for f in files:
            if f.endswith('.cpp'):
                path = os.path.relpath(os.path.join(root, f), REPO)
                idx[tu_name(path)] = path
    return idx


def read_assign(path):
    """docs/path_assign.tsv -> {our TU path: (target path, status, evidence)}. A TU that has already
    moved is found under its target path too, so the table stays valid across the move."""
    out = {}
    if not os.path.exists(path):
        return out
    lines = [l.rstrip('\n') for l in open(path) if l.strip() and not l.startswith('#')]
    for l in lines[1:]:
        tu, target, status, ev = (l.split('\t') + ['', '', ''])[:4]
        cur = tu if os.path.exists(os.path.join(REPO, tu)) else target
        out[cur] = (target, status, ev)
    return out


def shared_member_owners(build, srcidx):
    """The archive names its members by basename only, so `aurmodel.cpp.o` is two objects. For each
    shared stem, read the per-object files CMake leaves under CMakeFiles/ and return
    {symbol: TU name} for their strong definitions."""
    by_lower = {v.lower(): k for k, v in srcidx.items()}
    shared = {os.path.basename(v)[:-4] for k, v in srcidx.items() if k != os.path.basename(v)[:-4]}
    owners = {}
    for root, _d, files in os.walk(os.path.join(build, 'CMakeFiles')):
        for f in files:
            if not (f.endswith('.cpp.o') and f[:-6] in shared):
                continue
            full = os.path.join(root, f)
            rel = full.split(os.sep + 'src' + os.sep, 1)
            tu = by_lower.get(('src/' + rel[1][:-2]).lower()) if len(rel) == 2 else None
            if tu is None:
                continue
            d, _u = LR.scan_archive(full)
            for macho, lst in d.items():
                if any(not w for _o, w in lst):
                    owners[LR.elf_name(macho)[0]] = tu
    return owners


def tu_cflags(path):
    d, tu = os.path.dirname(os.path.join(REPO, path)), os.path.basename(path)[:-4]
    return tuple(read_cflags(os.path.join(d, 'cflags', '_all')) + read_cflags(os.path.join(d, 'cflags', tu)))


def class_headers():
    """class -> original header, from docs/KOTOR2_class_file_index.md (__FILE__ assert strings)."""
    out = {}
    path = os.path.join(REPO, 'docs/KOTOR2_class_file_index.md')
    for line in open(path):
        m = re.match(r'\|\s*\d+\s*\|\s*`([^`]+)`\s*\|\s*`([^`]+)`\s*\|\s*(\w+)', line)
        if m:
            out[m.group(1)] = (m.group(2), m.group(3))
    return out


def target_path(of):
    """'CODE/libsource/AURORA/aurmodel.cpp' -> 'src/libsource/AURORA/aurmodel.cpp' (case kept)."""
    return 'src/' + of.split('CODE/', 1)[1]


def write_tsv(path, per_file, kinds, srcidx, entangled):
    """docs/path_migration.tsv: one row per (our TU, original file) pair.

    Columns: old path, new path, kind, original file, functions.
    * kind `rename` / `move+rename`: new path is the TU's final home -- tools/pathmigrate.py moves it.
    * kind `merge`: every TU of the file lists the same new path; the merge is a phase-2 decision.
    * kind `entangled`: the row says where THESE functions belong; the TU must be split first.
    * kind `entangled-member`: a clean TU of an entangled file; it merges once the split is done.
    A TU that has already moved shows its CURRENT path in the old-path column with old == new, so
    the table is regenerable at any point of the migration. TUs with no macOS counterpart are not
    listed (plan section 5).
    """
    out = os.path.abspath(path)
    if not (out.startswith(os.path.join(REPO, 'docs') + os.sep) and out.endswith('.tsv')):
        sys.exit('pathmap.py: refusing to write %s' % out)
    rows = []
    for of, c in per_file.items():
        for tu, k in c.items():
            kind = kinds[of]
            if kind == 'entangled' and tu not in entangled:
                kind = 'entangled-member'   # clean itself; the file waits on its entangled TUs
            rows.append((srcidx.get(tu, tu), target_path(of), kind, of, k))
    rows.sort(key=lambda r: (r[3], -r[4], r[0]))
    with open(out, 'w') as f:
        f.write('# generated by platforms/tools/pathmap.py -- see docs/PATH_MIGRATION_PLAN.md\n')
        f.write('old\tnew\tkind\toriginal\tfunctions\n')
        for r in rows:
            f.write('%s\t%s\t%s\t%s\t%d\n' % r)


def write_split_tsv(path, rows):
    """docs/path_split.tsv: every mapped function of every ENTANGLED TU, with its original file.

    Columns: TU path, function (ELF mangled name), original file, arm64. arm64 is `major` for the
    TU's majority file, else the verdict of the entangled table: `confirmed` (the .so address lies in
    that file's address run), `k2-cross-android-touch` (it lies in the majority file's run instead: the port may have
    moved it), `unclear`. tools/pathsplit.py reads it.
    """
    out = os.path.abspath(path)
    if not (out.startswith(os.path.join(REPO, 'docs') + os.sep) and out.endswith('.tsv')):
        sys.exit('pathmap.py: refusing to write %s' % out)
    with open(out, 'w') as f:
        f.write('# generated by platforms/tools/pathmap.py -- see docs/PATH_MIGRATION_PLAN.md\n')
        f.write('tu\tfunction\toriginal\tarm64\n')
        for r in sorted(rows):
            f.write('%s\t%s\t%s\t%s\n' % r)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', default=os.path.join(REPO, 'platforms/out/k2-host-pc'))
    ap.add_argument('--out', default=os.path.join(REPO, 'docs/PATH_MIGRATION_MAP.md'))
    ap.add_argument('--tsv', default=os.path.join(REPO, 'docs/path_migration.tsv'),
                    help='machine-readable rename table (see write_tsv)')
    ap.add_argument('--assign', default=os.path.join(REPO, 'docs/path_assign.tsv'),
                    help='hand placements for the TUs the stabs cannot map (phase 5)')
    ap.add_argument('--split-tsv', default=os.path.join(REPO, 'docs/path_split.tsv'),
                    help='function-level split table for the entangled TUs (see write_split_tsv)')
    a = args = ap.parse_args()
    out = os.path.abspath(a.out)
    if not (out.startswith(os.path.join(REPO, 'docs') + os.sep) and out.endswith('.md')):
        sys.exit('pathmap.py: refusing to write %s' % out)

    defs, _u = LR.scan_archive(os.path.join(a.build, 'libk2_engine.a'))
    srcidx = src_index()
    shared = shared_member_owners(a.build, srcidx)
    ours = {}
    for macho, lst in defs.items():
        strong = [o for o, w in lst if not w]
        name = LR.elf_name(macho)[0]
        if name in shared:
            ours[name] = shared[name]
        elif len(strong) == 1:
            ours[name] = re.sub(r'\.cpp\.o$', '', strong[0])

    funs, weak = mac_functions()
    from inventory import orig_symbols
    oaddr = {n: a for n, (a, _sz, t) in orig_symbols().items() if t == 'FUNC'}
    fn_of = collections.defaultdict(list)                      # (original file, TU) -> [names]
    per_file = collections.defaultdict(collections.Counter)   # original file -> our TU -> fn count
    total = collections.Counter()                              # original file -> strong mac fns
    for f, n in funs:
        if '/CODE/' not in f or n in weak:
            continue
        of = orig_path(f)
        total[of] += 1
        if n in ours:
            per_file[of][ours[n]] += 1
            fn_of[(of, ours[n])].append(n)
    # Hand placements (path_assign.tsv) override the stabs for the TUs they name.
    assigned = read_assign(a.assign)
    for tu_path, (target, status, _ev) in assigned.items():
        tu = tu_name(tu_path)
        for of in list(per_file):
            per_file[of].pop(tu, None)
            if not per_file[of]:
                del per_file[of]
        of = 'CODE/' + target[len('src/'):]
        per_file[of][tu] = (len(fnindex.rows_for(tu_path))
                            if os.path.exists(os.path.join(REPO, tu_path)) else 0)
    per_tu = collections.defaultdict(collections.Counter)
    for of, c in per_file.items():
        for tu, k in c.items():
            per_tu[tu][of] += k

    mapped = set(per_tu)
    unmapped = sorted(t for t in srcidx if t not in mapped)
    # a TU is "clean" when all of its mapped functions come from one original file
    home = {tu: c.most_common(1)[0][0] for tu, c in per_tu.items()}
    entangled = {tu for tu, c in per_tu.items() if len(c) > 1}

    kinds = {}
    for of, c in per_file.items():
        tus = list(c)
        if any(t in entangled for t in tus):
            kinds[of] = 'entangled'
        elif len(tus) == 1:
            kinds[of] = 'rename' if os.path.basename(srcidx.get(tus[0], ''))[:-4].lower() == \
                os.path.splitext(os.path.basename(of))[0].lower() else 'move+rename'
        else:
            kinds[of] = 'merge'

    L = []
    w = L.append
    w('# Path migration map (generated)\n')
    w('Generated by `platforms/tools/pathmap.py`; the plan and procedure are in')
    w('`docs/PATH_MIGRATION_PLAN.md`. Regenerate after a build (`cmake --build --preset k2-host-pc` in')
    w('`platforms/`); never quote these numbers.\n')
    w('Oracle: the macOS build\'s stabs name the original source file of every function it emitted.')
    w('Each one is looked up in our tree to find the TU that defines it now. Only STRONG definitions')
    w('count on both sides, so inline and template copies do not smear a function across files.\n')
    kc = collections.Counter(kinds.values())
    w('| | count |')
    w('|---|---|')
    w('| original `CODE/` files in the macOS build | %d |' % len(total))
    w('| ... with code already transcribed | %d |' % len(per_file))
    w('| our TUs (`src/**/*.cpp`) | %d |' % len(srcidx))
    w('| ... mapped to an original file | %d |' % len(mapped))
    w('| ... with code from MORE than one original file (need splitting) | %d |' % len(entangled))
    w('| ... with no macOS counterpart (Android-only, or all inlined on the Mac) | %d |' % len(unmapped))
    for k in ('rename', 'move+rename', 'merge', 'entangled'):
        w('| original files needing: **%s** | %d |' % (k, kc[k]))
    w('')
    w('* **rename**: one TU, already named after the original file; only the directory changes.')
    w('* **move+rename**: one TU holds the whole original file under a class name.')
    w('* **merge**: several TUs, each wholly from this file, become one.')
    w('* **entangled**: at least one of the TUs also holds code from another original file, so it has')
    w('  to be split first. The per-TU table below says which functions go where.\n')

    by_dir = collections.defaultdict(list)
    for of in per_file:
        by_dir[os.path.dirname(of)].append(of)
    w('## By original file\n')
    w('`coverage` = functions of that file our tree defines / its strong functions in the macOS build.')
    w('`cflags` flags a merge group whose TUs carry DIFFERENT per-TU flags (they cannot share one')
    w('object without a codegen decision; see the plan).\n')
    for d in sorted(by_dir):
        w('### `%s/`\n' % d)
        w('| original file | kind | coverage | our TUs (functions) | cflags |')
        w('|---|---|---|---|---|')
        for of in sorted(by_dir[d]):
            c = per_file[of]
            cov = '%d/%d' % (sum(c.values()), total[of])
            parts = []
            for tu, k in c.most_common():
                mark = ' ⚠' if tu in entangled else ''
                parts.append('`%s` %d%s' % (srcidx.get(tu, tu), k, mark))
            flags = {tu_cflags(srcidx[tu]) for tu in c if tu in srcidx}
            cf = 'differ' if len(flags) > 1 else ('`%s`' % ' '.join(next(iter(flags))) if flags and next(iter(flags)) else '')
            w('| `%s` | %s | %s | %s | %s |' % (os.path.basename(of), kinds[of], cov, '<br>'.join(parts), cf))
        w('')

    w('## Entangled TUs -- code from more than one original file\n')
    w('Each has to be split before it moves. The largest share is the natural home; the other')
    w('files\' functions move out to their own original files.\n')
    w('`arm64` checks the minority functions against the Android build: a function counts as')
    w('**confirmed** when its `.so` address lies inside the address run of the other functions the')
    w('stabs give that file, and as **android** when it instead lies inside the run of the TU\'s')
    w('majority file (the Android port may have moved it; verify before splitting).\n')

    def span(of):
        xs = sorted(oaddr[n] for (f, _t), ns in fn_of.items() if f == of for n in ns if n in oaddr)
        if len(xs) < 2:
            return None
        # interquartile-ish core of the run, so a single stray function does not stretch it
        return xs[len(xs) // 10], xs[-1 - len(xs) // 10]

    spans = {of: span(of) for of in per_file}
    split_rows = []
    w('| our TU | original files (functions) | minority functions: arm64 |')
    w('|---|---|---|')
    for tu in sorted(entangled, key=lambda t: (-len(per_tu[t]), t)):
        major = per_tu[tu].most_common(1)[0][0]
        conf = andr = unk = 0
        details = []
        for of, _k in per_tu[tu].most_common()[1:]:
            for n in fn_of[(of, tu)]:
                a = oaddr.get(n)
                sp, sm = spans.get(of), spans.get(major)
                if a is not None and sp and sp[0] <= a <= sp[1]:
                    conf += 1
                elif a is not None and sm and sm[0] <= a <= sm[1]:
                    andr += 1
                    details.append(LR.demangle_all([n]).get(n, n).split('(')[0])
                else:
                    unk += 1
        for n in fn_of[(major, tu)]:
            split_rows.append((srcidx.get(tu, tu), n, major, 'major'))
        for of, _k in per_tu[tu].most_common()[1:]:
            for n in fn_of[(of, tu)]:
                a = oaddr.get(n)
                sp, sm = spans.get(of), spans.get(major)
                v = ('confirmed' if a is not None and sp and sp[0] <= a <= sp[1] else
                     'android' if a is not None and sm and sm[0] <= a <= sm[1] else 'unclear')
                split_rows.append((srcidx.get(tu, tu), n, of, v))
        verdict = 'confirmed %d, android %d, unclear %d' % (conf, andr, unk)
        if details:
            verdict += ' (android: %s)' % ', '.join('`%s`' % d for d in details[:4])
        w('| `%s` | %s | %s |' % (srcidx.get(tu, tu),
                                 ', '.join('`%s` %d' % (of, k) for of, k in per_tu[tu].most_common()),
                                 verdict))
    w('')

    w('## TUs with no macOS counterpart\n')
    w('No strong function of these is in the macOS stabs. They are Android/iOS port code (JNI, cloud,')
    w('iOS widgets, Aspyr glue), files whose every function the Mac build inlined, or TUs holding')
    w('only template instances / data. Place them by hand: `docs/KOTOR2_class_file_index.md` and the')
    w('`__FILE__`-derived `docs/KOTOR2_source_map.md` cover many classes whose code the Mac build')
    w('never emitted.\n')
    ch = class_headers()
    w('| our TU | header-level hint (`KOTOR2_class_file_index.md`) |')
    w('|---|---|')
    for tu in unmapped:
        hint = ch.get(tu)
        w('| `%s` | %s |' % (srcidx[tu], ('`%s` (%s)' % hint) if hint else ''))
    w('')

    w('## Headers\n')
    w('Original class headers, from `__FILE__` assert strings (`docs/KOTOR2_class_file_index.md`).')
    w('An original header usually declares several of our classes, so headers merge too.\n')
    hdr = collections.defaultdict(list)
    for cls, (h, how) in ch.items():
        if h.endswith('.h'):
            hdr[h].append((cls, how))
    w('| original header | our classes (match quality) |')
    w('|---|---|')
    for h in sorted(hdr):
        w('| `%s` | %s |' % (h, ', '.join('`%s` (%s)' % x for x in sorted(hdr[h]))))
    open(out, 'w').write('\n'.join(L) + '\n')
    write_tsv(args.tsv, per_file, kinds, srcidx, entangled)
    write_split_tsv(args.split_tsv, split_rows)
    print('pathmap: %d original files (%s), %d TUs mapped, %d entangled, %d unmapped'
          % (len(per_file), dict(kc), len(mapped), len(entangled), len(unmapped)))


if __name__ == '__main__':
    main()
