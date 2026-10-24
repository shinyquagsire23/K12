#!/usr/bin/env python3
"""inventory.py -- two worklists derived from the host smoke build's link report.

  docs/CLASS_MODEL_MISMATCHES.md   every symbol our code references that the original never defines
                                   under that name, diagnosed against the original's vtables and
                                   exported symbols (inherited-override / signature / wrong class)
  docs/DATA_HOMES.md               every engine global / table / .rodata object our code references
                                   but no TU defines yet, with the TU it most likely belongs in

Needs a finished `cmake --build --preset k2-host-pc` (reads out/k2-host-pc/report/linkreport.json and the
engine archive) plus build/vtables.json (`python3 tools/hoist_vtables.py`).

DATA-HOME ORACLES, strongest first
  stabs     the macOS build keeps N_GSYM / N_STSYM stabs, so every global it emitted carries the
            ORIGINAL source file. That file is mapped to our TU(s) by where the file's own functions
            (its N_FUN entries) are defined in our tree -- the majority TU wins.
  bracket   the ARM64 .data/.bss/.rodata layout follows link order, and within a file roughly
            definition order. The nearest data symbols below and above that one of OUR TUs already
            defines name its neighbours; when both sides agree, that is the file.
  users     the TUs that reference it (a hint only: globals are shared).
"""
import argparse, collections, json, os, re, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import linkreport as LR  # noqa: E402

ORIG_SO = os.path.join(REPO, 'decomp_bins/swkotorii_extracted/lib/arm64-v8a/libkotor2.so')
MAC = os.path.join(REPO, 'decomp_bins/KOTOR2_MacOS/KOTOR2sub')

N_SO, N_FUN, N_GSYM, N_STSYM = 0x64, 0x24, 0x20, 0x26


def safe_write(path, lines):
    """Write a generated doc -- and refuse anything outside docs/ (this tool must never touch src/)."""
    path = os.path.abspath(path)
    if not (path.startswith(os.path.join(REPO, 'docs') + os.sep) and path.endswith('.md')):
        sys.exit('inventory.py: refusing to write %s' % path)
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')


# --- oracles ------------------------------------------------------------------------------------
def mac_stabs():
    """-> (fun_file: name -> file, data_file: name -> file). Names without the Mach-O '_'."""
    data = open(MAC, 'rb').read()
    fun, dat = {}, {}
    for _cpu, base in _slices(data):
        if struct.unpack_from('<I', data, base)[0] != 0xFEEDFACF:
            continue
        ncmds = struct.unpack_from('<I', data, base + 16)[0]
        p, sig = base + 32, None
        for _ in range(ncmds):
            cmd, sz = struct.unpack_from('<II', data, p)
            if cmd == 2:
                sig = struct.unpack_from('<IIII', data, p + 8)
            p += sz
        symoff, nsyms, stroff, strsize = sig
        st = data[base + stroff:base + stroff + strsize]
        cur = ''
        for k in range(nsyms):
            strx, t, _s, _d, _v = struct.unpack_from('<IBBhQ', data, base + symoff + k * 16)
            if t not in (N_SO, N_FUN, N_GSYM, N_STSYM):
                continue
            e = st.find(b'\0', strx)
            nm = st[strx:e].decode('utf8', 'replace')
            if t == N_SO:
                if nm and not nm.endswith('/'):
                    cur = nm
            elif nm:
                nm = nm.split(':')[0]                       # N_STSYM may carry a ':type' suffix
                nm = nm[1:] if nm.startswith('_') else nm
                (fun if t == N_FUN else dat).setdefault(nm, cur)
        break
    return fun, dat


def _slices(data):
    if struct.unpack_from('>I', data, 0)[0] == 0xCAFEBABE:
        n = struct.unpack_from('>I', data, 4)[0]
        return [(0, struct.unpack_from('>IIIII', data, 8 + 20 * i)[2]) for i in range(n)]
    return [(0, 0)]


def orig_sections():
    out = []
    for line in LR.run([LR.tool('llvm-readelf'), '-S', '-W', ORIG_SO]).splitlines():
        m = re.search(r'\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)', line)
        if m:
            name, addr, size = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
            if size:
                out.append((name, addr, addr + size))
    return out


def orig_symbols():
    """-> name -> (addr, size, type) for every defined dynsym entry."""
    out = {}
    for line in LR.run([LR.tool('llvm-readelf'), '--dyn-syms', '-W', ORIG_SO]).splitlines():
        f = line.split()
        if len(f) < 8 or not f[0].rstrip(':').isdigit() or f[6] == 'UND':
            continue
        size = int(f[2], 0) if f[2].startswith('0x') else int(f[2])
        out[f[7].split('@')[0]] = (int(f[1], 16), size, f[3])
    return out


def tu_of(obj):
    return re.sub(r'\.cpp\.o$|\.o$', '', obj)


def src_path_index():
    idx = {}
    for root, _dirs, files in os.walk(os.path.join(REPO, 'src')):
        for fn in files:
            if fn.endswith('.cpp'):
                idx[fn[:-4]] = os.path.relpath(os.path.join(root, fn), REPO)
    return idx


# --- class model ---------------------------------------------------------------------------------
def split_member(dm):
    head, _, params = dm.partition('(')
    depth, cut = 0, -1
    for i, ch in enumerate(head):
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        elif depth == 0 and head[i:i + 2] == '::':
            cut = i
    if cut < 0:
        return None, None, None
    return head[:cut], head[cut + 2:], '(' + params


def declaration_site(cls, member):
    """First `member(` in a header named after the class (or any header) -> 'path:line'."""
    leaf = cls.split('::')[-1].split('<')[0]
    pat = re.compile(r'\b' + re.escape(member.split('<')[0]) + r'\s*\(')
    cands = []
    for root, _dirs, files in os.walk(os.path.join(REPO, 'src')):
        for fn in files:
            if fn == leaf + '.h':
                cands.insert(0, os.path.join(root, fn))
    for path in cands:
        try:
            for i, line in enumerate(open(path, errors='replace'), 1):
                if pat.search(line) and not line.lstrip().startswith('//'):
                    return '%s:%d' % (os.path.relpath(path, REPO), i)
        except OSError:
            pass
    return ''


def class_model(rep, vt, orig_dem):
    """-> list of dict rows, one per mismatched symbol."""
    by_class = collections.defaultdict(list)          # orig: class -> [(member, params, dem, mangled)]
    member_idx = collections.defaultdict(set)         # orig: member+params -> {class}
    for mangled, dem in orig_dem.items():
        c, m, p = split_member(dem)
        if c:
            by_class[c].append((m, p, dem, mangled))
            member_idx[m + p].add(c)

    # orig vtables by class name: member -> the demangled target of that slot
    vslots = collections.defaultdict(dict)
    for ztv, v in vt['vtables'].items():
        cls = LR.demangle_all([ztv]).get(ztv, '').replace('vtable for ', '')
        for seg in v.get('segments', []):
            for sl in seg.get('slots', []):
                d = sl.get('demangled') or ''
                c, m, p = split_member(d)
                if m:
                    vslots[cls].setdefault(m + p, d)

    rows = []
    for elf, info in rep.items():
        if info['class'] != 'unknown':
            continue
        dem = info['demangled'] or elf
        c, m, p = split_member(dem)
        users = sorted({tu_of(u.replace('[platform] ', '')) for u in info['users']})
        row = {'sym': elf, 'dem': dem, 'class': c or '(free)', 'member': m, 'users': users,
               'kind': None, 'detail': ''}
        if elf.startswith('_ZTV') or elf.startswith('_ZTI') or elf.startswith('_ZTS'):
            row['kind'] = 'vtable/RTTI the original never emits'
        elif c and m + p in vslots.get(c, {}):
            row['kind'] = 'override the original class does not have'
            row['detail'] = "the original's `%s` vtable slot is `%s`" % (c, vslots[c][m + p])
        elif c and any(mm == m for mm, _pp, _d, _x in by_class.get(c, [])):
            row['kind'] = 'signature mismatch'
            row['detail'] = 'the original has: ' + '; '.join(
                '`%s`' % d for mm, _pp, d, _x in by_class[c] if mm == m)
        elif c and member_idx.get(m + p):
            row['kind'] = 'declared on the wrong class'
            row['detail'] = 'the original defines it on: ' + ', '.join(
                '`%s`' % x for x in sorted(member_idx[m + p])[:5])
        elif 'internal' in (info['group'] or ''):
            row['kind'] = 'internal to the original (not exported on Android)'
            row['detail'] = 'defined in the macOS build; local/anonymous/hidden in the .so'
        elif c:
            row['kind'] = 'member the original never defines'
        else:
            if any(dem.split('(')[0] == od.split('(')[0] for od in orig_dem.values()):
                row['kind'] = 'signature mismatch'
                if '(' not in dem:
                    row['kind'] = 'C linkage, but the original is C++'
                row['detail'] = 'the original has: ' + '; '.join(
                    '`%s`' % od for od in orig_dem.values() if od.split('(')[0] == dem.split('(')[0])
            else:
                row['kind'] = 'free function / global the original never defines'
        if c and m:
            row['decl'] = declaration_site(c, m)
        rows.append(row)
    return rows


KIND_TEXT = {
    'override the original class does not have':
        "Our header declares a virtual override for this class, but the original class inherits "
        "the base implementation. Its vtable slot names the base function. Because our vtable points "
        "at a body that exists nowhere, the fix is to delete the redeclaration; the class then "
        "inherits, exactly like the binary. Check `lint_idiom.py` / RULE 109 before deleting: a few "
        "overrides are deliberately modelled.",
    'signature mismatch':
        "The original defines this function under the same name with different parameter types, "
        "const-ness or return type. Our declaration's mangling does not match, so every call in "
        "the tree names a function that does not exist. `match.py` masks the `bl`, so a MATCH row "
        "can still call the wrong thing.",
    'C linkage, but the original is C++':
        "A TU declares this `extern \"C\"` (or the name is otherwise unmangled), but the original "
        "exports a C++-mangled function with this name. Declare it with C++ linkage and the exact "
        "parameter list shown.",
    'declared on the wrong class':
        "The original has this exact member (name + parameters) on a different class, usually a "
        "base or a sibling. Our tree calls it through the wrong class. Move the declaration, or "
        "qualify the call.",
    'member the original never defines':
        "No oracle has this member at all. Usually the original inlined it everywhere (so the tree "
        "should inline it too: define it in the header), or our model invented it.",
    'free function / global the original never defines':
        "A free function (or a name our tree coined) that the original does not export. Most are "
        "internal to the original (a file `static` / anonymous namespace), so the TU that uses it "
        "must DEFINE it instead of referencing it. Unexported DATA of this kind is listed in "
        "`docs/DATA_HOMES.md`.",
    'internal to the original (not exported on Android)':
        "Exists in the original but with internal linkage. Our TU must define it (file-static or "
        "anonymous namespace) rather than reference it.",
    'vtable/RTTI the original never emits':
        "A vtable or typeinfo our model expects for a class the original never emits one for "
        "(for example, no key function was ever defined).",
}


def write_class_model(rows, path, srcidx):
    kinds = collections.Counter(r['kind'] for r in rows)
    L = []
    w = L.append
    w('# Class-model mismatches (generated)\n')
    w('Generated by `platforms/tools/inventory.py` from the host smoke build')
    w('(`platforms/`, see its README). Each row is a symbol the transcribed code **references**')
    w('that the original `libkotor2.so` does **not** define under that name. It is also absent from')
    w('the macOS build and is not a known import. These are the declarations in `src/` that disagree')
    w('with the binary.\n')
    w('⚠ `match.py` masks call relocations, so none of these moves a MATCH row. They matter for a runnable')
    w('build (each becomes an abort stub, often reached through a vtable) and for model fidelity.')
    w('Regenerate after a class-model change; never quote the counts.\n')
    w('| diagnosis | rows |')
    w('|---|---|')
    for k, n in kinds.most_common():
        w('| %s | %d |' % (k, n))
    w('| **total** | **%d** |\n' % len(rows))
    w('Oracles: the original\'s vtables (`build/vtables.json`), its `.dynsym`, and the symbolized')
    w('macOS build (`decomp_bins/KOTOR2_MacOS/KOTOR2sub`). "decl" is the first matching')
    w('declaration in the class\'s header when one was found; "used by" lists the TUs that reference it.\n')

    for kind, _n in kinds.most_common():
        sub = [r for r in rows if r['kind'] == kind]
        w('## %s (%d)\n' % (kind, len(sub)))
        w(KIND_TEXT.get(kind, '') + '\n')
        byc = collections.defaultdict(list)
        for r in sub:
            byc[r['class']].append(r)
        w('| class | rows |')
        w('|---|---|')
        for c in sorted(byc, key=lambda c: (-len(byc[c]), c)):
            w('| `%s` | %d |' % (c, len(byc[c])))
        w('')
        for c in sorted(byc, key=lambda c: (-len(byc[c]), c)):
            w('### `%s`\n' % c)
            for r in sorted(byc[c], key=lambda r: r['dem']):
                bits = ['`%s`' % r['dem']]
                if r['detail']:
                    bits.append(r['detail'])
                if r.get('decl'):
                    bits.append('decl `%s`' % r['decl'])
                bits.append('used by %s' % ', '.join(r['users'][:4]) + (' ...' if len(r['users']) > 4 else ''))
                w('* ' + ' -- '.join(bits))
            w('')
    safe_write(path, L)


def find_decls(names):
    """name -> (path:line, text) of its first non-comment declaration in src/ (one grep)."""
    if not names:
        return {}
    pat = '(' + '|'.join(re.escape(n) for n in names) + ')'
    out = subprocess.run(['git', 'grep', '-n', '-w', '-E', pat, '--', 'src'], cwd=REPO,
                         capture_output=True, text=True).stdout
    found = {}
    for line in out.splitlines():
        path, ln, text = line.split(':', 2)
        code = text.split('//')[0]
        if not code.strip() or 'extern' not in code:
            continue
        for n in names:
            if n not in found and re.search(r'\b%s\b' % re.escape(n), code):
                found[n] = ('%s:%s' % (path, ln), text.strip())
    return found


# --- data homes ----------------------------------------------------------------------------------
def data_homes(rep, defs, orig, sections, fun_file, dat_file, hidden=()):
    # our definitions: name -> TU (strong ones only; a weak template copy says nothing)
    our_def = {}
    for macho, lst in defs.items():
        strong = [o for o, weak in lst if not weak]
        if len(strong) == 1:
            e, _asm = LR.elf_name(macho)
            our_def[e] = tu_of(strong[0])

    # mac file -> our TU, by where that file's functions live in our tree
    file_tus = collections.defaultdict(collections.Counter)
    for fn, f in fun_file.items():
        if fn in our_def:
            file_tus[f][our_def[fn]] += 1

    def mac_to_tu(f):
        c = file_tus.get(f)
        if not c:
            return None, 0, 0
        tu, n = c.most_common(1)[0]
        return tu, n, sum(c.values())

    def section_of(addr):
        for name, lo, hi in sections:
            if lo <= addr < hi:
                return name
        return '?'

    # bracket index: per section, sorted (addr, name) of orig data symbols
    per_sec = collections.defaultdict(list)
    for n, (addr, size, typ) in orig.items():
        if typ in ('OBJECT', 'TLS') or (typ == 'NOTYPE' and size):
            per_sec[section_of(addr)].append((addr, n))
    for v in per_sec.values():
        v.sort()

    def bracket(name, addr=None):
        addr = orig[name][0] if addr is None else addr
        lst = per_sec[section_of(addr)]
        i = next((k for k, (a, n) in enumerate(lst) if n == name), None)
        if i is None:                                   # not in the dynsym: place by address
            import bisect
            j = bisect.bisect_left(lst, (addr, ''))
            lst = lst[:j] + [(addr, name)] + lst[j:]
            i = j
        lo = next(((lst[k][1], our_def[lst[k][1]]) for k in range(i - 1, -1, -1) if lst[k][1] in our_def), None)
        hi = next(((lst[k][1], our_def[lst[k][1]]) for k in range(i + 1, len(lst)) if lst[k][1] in our_def), None)
        return lo, hi

    pos = {}                                           # TU -> first .text address (link order)
    for n, tu in our_def.items():
        o = orig.get(n)
        if o and o[2] == 'FUNC':
            pos[tu] = min(pos.get(tu, 1 << 62), o[0])

    def in_window(lo, hi, cands):
        if not (lo and hi) or lo[1] not in pos or hi[1] not in pos:
            return []
        a, b = sorted((pos[lo[1]], pos[hi[1]]))
        return [c for c in cands if c in pos and a <= pos[c] <= b]

    # class -> the TU that defines most of its member functions (our tree is one class per TU)
    ddem = LR.demangle_all(our_def)
    class_tus = collections.defaultdict(collections.Counter)
    defined = set()                                    # classes with any strong definition of ours
    for n, tu in our_def.items():
        c, _m, _p = split_member(ddem.get(n, ''))
        if c:
            defined.add(c)
        if c and orig.get(n, (0, 0, ''))[2] == 'FUNC':
            class_tus[c][tu] += 1

    # a class dir `<leaf>/<leaf>.h` whose class no TU of ours defines anything of. Decided from the
    # definition data, not from the dir lacking a .cpp: in the flat layout (`src/<path>/<tu>.cpp`,
    # headers left in their class dirs) every class dir lacks one.
    headers_only = {}
    for root, _dirs, files in os.walk(os.path.join(REPO, 'src')):
        leaf = os.path.basename(root)
        if leaf + '.h' in files and leaf not in defined:
            headers_only[leaf] = os.path.relpath(os.path.join(root, leaf + '.h'), REPO)

    def class_home(dem):
        for pre in ('vtable for ', 'typeinfo for ', 'typeinfo name for ', 'VTT for '):
            if dem.startswith(pre):
                c = dem[len(pre):]
                break
        else:
            c, _m, _p = split_member(dem + '(')
        if c and class_tus.get(c):
            return class_tus[c].most_common(1)[0][0]
        if c and c in headers_only:
            return '(no TU yet: class header `%s`)' % headers_only[c]
        return None

    rows = []
    for elf, info in rep.items():
        if info['class'] != 'engine' or not info['orig'] or info['orig'][0] == 'FUNC':
            continue
        if elf not in orig:
            continue
        addr, size, _t = orig[elf]
        mf = dat_file.get(elf)
        mtu, mn, mtot = mac_to_tu(mf) if mf else (None, 0, 0)
        lo, hi = bracket(elf)
        users = sorted({tu_of(u.replace('[platform] ', '')) for u in info['users']})
        cands = list(file_tus[mf]) if mf and mf in file_tus else []
        home = class_home(info['demangled'] or elf)
        if home:
            verdict, conf = home, 'class home' + ('+stabs' if home in cands else '')
        elif mtu and len(cands) > 1:                     # the original file is split across our TUs
            win = in_window(lo, hi, cands)
            both = [u for u in users if u in cands]
            if lo and hi and lo[1] == hi[1] and lo[1] in cands:
                verdict, conf = lo[1], 'stabs+bracket'
            elif len(win) == 1:
                verdict, conf = win[0], 'stabs+window'
            elif len(both) == 1:
                verdict, conf = both[0], 'stabs+user'
            else:
                verdict, conf = mtu, 'stabs (split file, majority)'
        elif mtu:
            verdict, conf = mtu, 'stabs'
            if lo and hi and lo[1] == hi[1] == mtu:
                conf = 'stabs+bracket'
        elif lo and hi and lo[1] == hi[1]:
            verdict, conf = lo[1], 'bracket'
        elif mf:
            verdict, conf = '(no TU yet: original file `%s`)' % mf, 'stabs-file'
        elif len(users) == 1:
            verdict, conf = users[0], 'single user'
        else:
            verdict, conf = '?', 'none'
        rows.append({'sym': elf, 'dem': info['demangled'] or elf, 'group': info['group'], 'addr': addr,
                     'size': size, 'section': section_of(addr), 'mac_file': mf,
                     'mac_tu': mtu, 'mac_votes': '%d/%d' % (mn, mtot) if mtu else '',
                     'lo': lo, 'hi': hi, 'users': users, 'verdict': verdict, 'conf': conf})

    # Data our tree named but the original does not export (file-statics, hidden .rodata constants).
    # The declaration comment usually carries the original address; place it by its neighbours.
    for h in hidden:
        addr = h['addr']
        users = h['users']
        if addr is not None and section_of(addr) != '?':
            lo, hi = bracket(h['sym'], addr)
            sec = section_of(addr)
        else:
            lo = hi = None
            sec = '?'
        if lo and hi and lo[1] == hi[1]:
            verdict, conf = lo[1], 'bracket (unexported)'
        else:
            verdict, conf = (users[0] if len(users) == 1 else '?'), 'unexported: declaring TU'
        rows.append({'sym': h['sym'], 'dem': h['sym'], 'group': 'unexported', 'addr': addr or 0,
                     'size': 0, 'section': sec, 'mac_file': None, 'mac_tu': None, 'mac_votes': '',
                     'lo': lo, 'hi': hi, 'users': users, 'verdict': verdict, 'conf': conf,
                     'decl': h['decl']})

    # Second pass: rows the stabs placed become neighbours too, so a weakly-placed row sitting
    # between two stabs-placed rows (or a stabs row and one of our definitions) inherits their TU.
    placed = {r['sym']: r['verdict'] for r in rows
              if r['conf'] in ('stabs', 'stabs+bracket', 'stabs+window', 'stabs+user')}
    saved = dict(our_def)
    our_def.update(placed)
    for r in rows:
        if r['conf'] in ('single user', 'none'):
            lo, hi = bracket(r['sym'])
            if lo and hi and lo[1] == hi[1]:
                r['lo'], r['hi'], r['verdict'], r['conf'] = lo, hi, lo[1], 'bracket (inferred)'
        if r['conf'] == 'unexported: declaring TU' and r['addr']:
            lo, hi = bracket(r['sym'], r['addr'])
            if lo and hi:
                r['lo'], r['hi'] = lo, hi
                if lo[1] == hi[1]:
                    r['verdict'], r['conf'] = lo[1], 'bracket (unexported)'
    our_def.clear()
    our_def.update(saved)

    # Link-order window: .data/.bss follow the same object order as .text. With the two neighbours
    # in different TUs, the object's file sits between them in link order -- so a candidate (a user,
    # or the declaring TU) whose code lies inside that window is corroborated.
    for r in rows:
        if r['conf'] not in ('single user', 'none', 'unexported: declaring TU') or not (r['lo'] and r['hi']):
            continue
        a, b = pos.get(r['lo'][1]), pos.get(r['hi'][1])
        if a is None or b is None:
            continue
        lo_p, hi_p = min(a, b), max(a, b)
        window = sorted((p, t) for t, p in pos.items() if lo_p <= p <= hi_p)
        r['window'] = len(window)
        inside = [u for u in r['users'] if u in pos and lo_p <= pos[u] <= hi_p]
        if len(inside) == 1:
            r['verdict'], r['conf'] = inside[0], 'link-order window'
    return rows


def write_data_homes(rows, path, srcidx):
    conf = collections.Counter(r['conf'] for r in rows)
    L = []
    w = L.append
    w('# Data homes -- where the remaining globals and .rodata belong (generated)\n')
    w('Generated by `platforms/tools/inventory.py`. Each row is an engine data object: a global,')
    w('cvar, table, shader text, vtable or typeinfo. The transcribed code **references** it, the original')
    w('`libkotor2.so` **exports** it, and no TU in `src/` defines it yet. These are exactly the')
    w('zero-filled data stubs the host smoke build has to invent. Most of them are also why a')
    w('`extern` in `src/` has no definition anywhere.\n')
    w('Total: **%d objects, %d bytes** in the original.\n' % (len(rows), sum(r['size'] for r in rows)))
    w('| evidence | rows | meaning |')
    w('|---|---|---|')
    meaning = {
        'class home+stabs': 'a vtable/typeinfo/static member: the TU defining most of the class\'s methods, which is also one of the stabs file\'s TUs',
        'class home': 'as above, where the stabs disagree or say nothing (our tree keeps one class per TU; the original file may have held several)',
        'stabs+bracket': 'the macOS stabs name the file AND both ARM64 neighbours already live in that TU',
        'stabs': 'the macOS stabs name the original file, and that file maps to exactly one of our TUs',
        'stabs+window': 'the stabs file is split across several of our TUs; exactly one of them lies between the ARM64 neighbours in link order',
        'stabs+user': 'the stabs file is split across several of our TUs; exactly one of them references the object',
        'stabs (split file, majority)': 'the stabs file is split and nothing narrows it: the TU holding most of that file\'s functions (check the votes)',
        'bracket': 'no stabs, but the nearest ARM64 data neighbours on both sides are defined by the same TU',
        'bracket (inferred)': 'as bracket, where a neighbour is itself a stabs-placed row from this list',
        'stabs-file': 'stabs name a file whose functions are not in our tree yet: the TU does not exist',
        'link-order window': 'neighbours are two different TUs; exactly one referencing TU has its code between them in link order',
        'bracket (unexported)': 'a name our tree coined for data the original does not export; placed by its address (from the declaration comment) between two neighbours of one TU',
        'unexported: declaring TU': 'as above, but the neighbours disagree or no address is recorded: the declaring TU is the best guess',
        'single user': 'no layout evidence; only one TU references it (weak)',
        'none': 'unresolved: neighbours disagree and no stabs',
    }
    for k in ('class home+stabs', 'class home', 'stabs+bracket', 'stabs', 'stabs+window', 'stabs+user', 'stabs (split file, majority)', 'bracket', 'bracket (inferred)', 'stabs-file',
              'link-order window', 'bracket (unexported)', 'unexported: declaring TU', 'single user', 'none'):
        if conf[k]:
            w('| %s | %d | %s |' % (k, conf[k], meaning[k]))
    w('')
    w('How to use: put the definition in the verdict TU, in ARM64 address order among that TU\'s own')
    w('data. The address column is the original\'s. Initializers come from `tools/rodata.py`')
    w('(`.rodata`/`.data`), and a `.bss` object is zero-initialized. "votes" is how many of the mac')
    w('file\'s functions our tree already defines in that TU. `lo`/`hi` are the nearest data symbols')
    w('below/above that OUR tree defines, with their TU.\n')

    byv = collections.defaultdict(list)
    for r in rows:
        byv[r['verdict']].append(r)
    w('## Summary by destination TU\n')
    w('| destination | objects | bytes | evidence |')
    w('|---|---|---|---|')
    for v in sorted(byv, key=lambda v: (-sum(r['size'] for r in byv[v]), v)):
        ev = collections.Counter(r['conf'] for r in byv[v])
        name = '`%s`' % (srcidx.get(v) or v)
        w('| %s | %d | %d | %s |' % (name, len(byv[v]), sum(r['size'] for r in byv[v]),
                                    ', '.join('%s %d' % kv for kv in ev.most_common())))
    w('')
    w('## Per destination\n')
    for v in sorted(byv, key=lambda v: (-sum(r['size'] for r in byv[v]), v)):
        w('### %s\n' % (srcidx.get(v) or v))
        w('| addr | size | section | symbol | evidence | mac file (votes) | lo / hi neighbour TU | users |')
        w('|---|---|---|---|---|---|---|---|')
        for r in sorted(byv[v], key=lambda r: r['addr']):
            nb = '%s / %s' % (r['lo'][1] if r['lo'] else '-', r['hi'][1] if r['hi'] else '-')
            mf = ('%s (%s)' % (r['mac_file'], r['mac_votes'])) if r['mac_file'] else '-'
            us = ', '.join(r['users'][:3]) + (' ...' if len(r['users']) > 3 else '')
            sym = '`%s`' % r['dem'] + (' (decl `%s`)' % r['decl'] if r.get('decl') else '')
            w('| 0x%x | %d | %s | %s | %s | %s | %s | %s |' % (r['addr'], r['size'], r['section'],
                                                             sym, r['conf'], mf, nb, us))
        w('')
    safe_write(path, L)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', default=os.path.join(REPO, 'platforms/out/k2-host-pc'))
    ap.add_argument('--docs', default=os.path.join(REPO, 'docs'))
    a = ap.parse_args()
    rep = json.load(open(os.path.join(a.build, 'report/linkreport.json')))
    defs, _u = LR.scan_archive(os.path.join(a.build, 'libk2_engine.a'))
    vt = json.load(open(os.path.join(REPO, 'build/vtables.json')))
    orig = orig_symbols()
    orig_dem = LR.demangle_all(orig)
    fun_file, dat_file = mac_stabs()
    srcidx = src_path_index()

    cm = class_model(rep, vt, orig_dem)
    never = [r for r in cm if r['kind'] == 'free function / global the original never defines']
    decls = find_decls([r['sym'] for r in never if re.match(r'^[A-Za-z_]\w*$', r['sym'])])
    hidden = []
    for r in never:
        d = decls.get(r['sym'])
        if d and not re.search(r'\b%s\s*\(' % re.escape(r['sym']), d[1].split('//')[0]):
            m = re.search(r'0x([0-9a-f]{5,})', d[1])
            hidden.append({'sym': r['sym'], 'users': r['users'], 'decl': d[0],
                           'addr': int(m.group(1), 16) if m else None})
            r['kind'] = None                         # data: reported in DATA_HOMES.md instead
        elif d:
            r['decl'] = d[0]
    cm = [r for r in cm if r['kind']]
    write_class_model(cm, os.path.join(a.docs, 'CLASS_MODEL_MISMATCHES.md'), srcidx)
    dh = data_homes(rep, defs, orig, orig_sections(), fun_file, dat_file, hidden)
    write_data_homes(dh, os.path.join(a.docs, 'DATA_HOMES.md'), srcidx)
    print('inventory: %d class-model rows %s' % (len(cm), dict(collections.Counter(r['kind'] for r in cm))))
    print('inventory: %d data objects %s' % (len(dh), dict(collections.Counter(r['conf'] for r in dh))))


if __name__ == '__main__':
    main()
