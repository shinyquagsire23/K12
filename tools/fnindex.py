#!/usr/bin/env python3
"""fnindex.py — the function TABLE: source file <-> original symbol <-> address.

WHAT THIS REPLACES
------------------
Every src/**/*.cpp used to carry a markup comment per transcribed function:

    // FUNCTION: KOTOR2 0x65c2e8 _ZN21CSWSEffectListHandler17InitializeEffectsEv

That line predates the K1 port and can only ever name ONE of the two images: the address above is
KOTOR2's, and K1 has the same body at a different one — and k1match.py looked the K1 row up by name
anyway, so the address in the comment was information nothing read. Of 15,089 marker sites every
address but 63 is recoverable from the images themselves (name -> address in both .so `.dynsym`s;
neither image carries a .symtab, so the 63 are names the original build kept local and the strip
destroyed).

WHERE THE DATA LIVES NOW
------------------------
  docs/fnindex.tsv    one row per marker SITE (an odd duplicate marker stays two rows):
                        game <TAB> mangled <TAB> src <TAB> line <TAB> addr
                      `addr` is `-` when the .so dynsym can supply it by name — the normal case,
                      read on demand by addr(), so the file cannot drift from the binaries — and the
                      literal address when the image has no name for the row.
                      `line` is the anchor's line number IN THE MARKER-FREE SOURCE, i.e. where the
                      markup comment used to sit; --init computes it as it plans the deletion, so
                      the anchor keeps meaning after the comments are gone.

READERS
-------
`markers(text_or_path, games=(2,))` -> [(addr_hex, mangled)]: the exact shape and order the old
`// FUNCTION: KOTOR2` regex produced. `match.MARKER` IS this function, so every tool that does
`from match import MARKER` needed no edit at all. `finditer(text)` yields marker hits whose
.group(1)/.group(2) are (addr, name) and whose .start()/.end() span the anchor LINE, so a caller
that walks text relative to a marker (a function's comment block, the end of its body) still works.
A `text` argument is matched to its file by content hash — every src file is read once per process.

CLI
---
  tools/fnindex.py --init [--dry-run]   scan src/ and write docs/fnindex.tsv
  tools/fnindex.py --strip [--dry-run]  delete the markup comments the table accounts for
  tools/fnindex.py --check              re-derive every address from the .so; report drift
  tools/fnindex.py --lines              does every row's anchor line name its function?
  tools/fnindex.py --owners             rows the owning TU's object does not define
  tools/fnindex.py --dup                (game, mangled) and address collisions
  tools/fnindex.py --stats              the table's shape
  tools/fnindex.py --reanchor REV [--dry-run]
                                        carry every row's line from REV's table through its file's
                                        edits since REV (a difflib line map, renames followed): run
                                        it after any edit that adds or deletes lines above a row
Run from the repo root.
"""
import hashlib, os, re, sys, threading
from collections import defaultdict, namedtuple

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TSV = os.path.join(REPO, "docs/fnindex.tsv")
SO = {2: "decomp_bins/swkotorii_extracted/lib/arm64-v8a/libkotor2.so",
      1: "decomp_bins/swkotor_extracted/lib/arm64-v8a/libKOTOR.so"}
SRC_EXT = (".cpp", ".h", ".hpp", ".c", ".cc")

# One marker SITE, anchored at the start of the comment. An unanchored match would also catch prose
# that quotes a marker ("the `// FUNCTION: KOTOR2 0x…` marker lives in the .cpp") -- --init reports
# those instead of importing them as rows.
SITE = re.compile(r'^[ \t]*//[ \t]*(?:UI-)?FUNCTION:[ \t]*KOTOR([12])[ \t]+(0x[0-9a-fA-F]+)[ \t]*(\S*)')
LOOSE = re.compile(r'//.*?`?[ \t]*//?[ \t]*FUNCTION:[ \t]*KOTOR([12])[ \t]+(0x[0-9a-fA-F]+)[ \t]*(\S*)')
SYMBOL = re.compile(r'^[A-Za-z_]\w*$')

# The three marker sites that name no symbol. The first two are file-local statics that the source
# itself names (the marker's prose says the name is ours); the third is an obsolete duplicate of its
# neighbour: its address is inside the previous function's body, whose own marker is on the next
# line.
LOCAL = {
    ('src/game/servercore/nwscreatureai.cpp', 0x553560): 'SRetPrintf',
    ('src/game/clientcore/clientinput.cpp', 0x22ea38): '_ZL9sprintf_sPcmPKcz',
    ('src/game/clientcore/swguiingameequip.cpp', 0x5224e4): '_ZN17CSWGuiInGameEquip12OnPanelAddedEv',
}
DROP = {('src/game/clientcore/swguiingameequip.cpp', 0x5224e4)}

Row = namedtuple("Row", "game mangled src line addr")


# ---------------------------------------------------------------- the table
# Every cache below is built into a local and published with one assignment. Half-populated global
# caches are the one real hazard here: a thread (progress.py, throwscreen.py and match.py all fan
# out) that enters while another is still filling one sees `is not None` and reads a partial table —
# a silent wrong answer, e.g. "no rows for this file".
_LOCK = threading.RLock()
_ROWS = None


def rows():
    """Every row, in file order (src path, then line)."""
    global _ROWS
    with _LOCK:
        if _ROWS is None:
            out = []
            with open(TSV) as fh:
                for ln in fh:
                    if ln.startswith('#') or not ln.strip():
                        continue
                    g, m, s, l, a = ln.rstrip('\n').split('\t')
                    out.append(Row(int(g[-1]), m or None, s, int(l), None if a == '-' else a))
            _ROWS = out
        return _ROWS


_BY_SRC = None


def by_src():
    global _BY_SRC
    with _LOCK:
        if _BY_SRC is None:
            d = defaultdict(list)
            for r in rows():
                d[r.src].append(r)
            for v in d.values():
                v.sort(key=lambda r: r.line)
            _BY_SRC = d
        return _BY_SRC


_SYMS = {}


def dynsyms(game):
    """{mangled: vaddr} of the image's exported FUNCs — the original's whole name -> address map."""
    with _LOCK:
        if game not in _SYMS:
            from elftools.elf.elffile import ELFFile
            d = {}
            for s in ELFFile(open(os.path.join(REPO, SO[game]), 'rb')).get_section_by_name(
                    '.dynsym').iter_symbols():
                if s['st_info']['type'] == 'STT_FUNC' and s['st_size'] and s['st_shndx'] != 'SHN_UNDEF':
                    d.setdefault(s.name, s['st_value'])
            _SYMS[game] = d
        return _SYMS[game]


def so(game):
    return os.path.join(REPO, SO[game])


def addrof(r):
    """A ROW's address: the binary's, or the table's for a row the binary cannot name. Per row and
    not per name, because a `static` helper appears in several TUs under ONE mangled name (`_ZL…`)
    at one address each, and the table is the only place those addresses exist."""
    if r.addr is not None:
        return int(r.addr, 16)
    return dynsyms(r.game).get(r.mangled)


def addr(game, mangled):
    """An address from (game, mangled) alone — for a caller holding only a name. Ambiguous for a
    stripped local defined in more than one TU; prefer addrof(row)."""
    for r in rows():
        if r.game == game and r.mangled == mangled:
            return addrof(r)
    return dynsyms(game).get(mangled)


# ---------------------------------------------------------------- text -> file
_TEXT = None


def _text_index():
    """{sha1(text): src path}. ~60 call sites hand markers() a file's TEXT (`open(src).read()`),
    which is how it knows which rows the caller means."""
    global _TEXT
    with _LOCK:
        if _TEXT is None:
            idx = {}
            for p in src_files():
                try:
                    with open(os.path.join(REPO, p), encoding='utf-8', errors='replace') as fh:
                        idx[hashlib.sha1(fh.read().encode('utf-8')).hexdigest()] = p
                except OSError:
                    pass
            _TEXT = idx
        return _TEXT


def which_file(arg):
    """The src-relative path a path names, else the file whose content hashes to it (None)."""
    if isinstance(arg, bytes):
        arg = arg.decode('utf-8', 'replace')
    if isinstance(arg, str) and '\n' not in arg and len(arg) < 512:
        cands = [arg] if os.path.isabs(arg) else [arg, os.path.join(REPO, arg)]
        for cand in cands:
            if os.path.isfile(cand):
                rel = os.path.relpath(cand, REPO)
                # a file outside the repo (an overlay tree) has no table rows
                return rel if not rel.startswith('..') else None
        return None
    key = hashlib.sha1(arg.encode('utf-8', 'replace')).hexdigest()
    hit = _text_index().get(key)
    if hit is None:
        # A miss can be a repo being edited underneath us (a session that writes src/ while a
        # screening tool runs): the index holds the file as it was, the caller holds it as it is.
        # Rebuild once from disk and look again before declaring the argument unattributable.
        global _TEXT
        with _LOCK:
            _TEXT = None
        hit = _text_index().get(key)
    return hit


# ---------------------------------------------------------------- the readers
def rows_for(src, games=(2,)):
    """The table's rows for one source file (a path or its text), in file order."""
    path = which_file(src)
    if path is None:
        raise KeyError(
            "fnindex: no src/ file matches this argument — pass a src-relative path, or the exact "
            "text of one (a truncated or edited copy cannot be attributed to a file). "
            "Argument began: %r" % (str(src)[:80],))
    return [r for r in by_src().get(path, ()) if r.game in games]


_WARNED = set()


def _unresolved(r):
    """A row whose address no longer resolves: the .so changed under the table, or the name is a
    typo. It cannot be scored by address, so it is dropped — loudly once, and `--check` lists them
    all. (Before the table existed this was a silent `MISS` in match.py and a silent `continue` in
    progress.py.)"""
    if (r.game, r.mangled) not in _WARNED:
        _WARNED.add((r.game, r.mangled))
        sys.stderr.write('fnindex: KOTOR%d %s (%s:%d) names no symbol in the image — re-run'
                         ' `tools/fnindex.py --check`\n' % (r.game, r.mangled, r.src, r.line))


def markers(src, games=(2,)):
    """[(addr_hex, mangled)] — drop-in for `MARKER.findall(open(src).read())`."""
    out = []
    for r in rows_for(src, games):
        a = addrof(r)
        if a is None:
            _unresolved(r)
            continue
        out.append((hex(a), r.mangled))
    return out


def markers_both(src):
    """Same for the K1-aware tools (k1match, castscreen): both images' rows, as the old KOTOR[12]."""
    return markers(src, games=(1, 2))


class _Hit:
    """A match-like object over the anchor LINE of the (marker-free) text."""

    def __init__(self, groups, a, b, text):
        self._g, self._a, self._b, self._t = groups, a, b, text

    def start(self, g=0):
        return self._a

    def end(self, g=0):
        return self._b

    def span(self, g=0):
        return (self._a, self._b)

    def group(self, g=0):
        return self._t[self._a:self._b] if g == 0 else self._g[g - 1]

    def groups(self):
        return self._g

    def __getitem__(self, g):
        return self.group(g)


def _line_spans(text):
    out, pos = [], 0
    for ln in text.splitlines(keepends=True):
        out.append((pos, pos + len(ln)))
        pos += len(ln)
    return out


def finditer(src, games=(2,)):
    """Marker hits anchored on the row's LINE, so relative text walking still works."""
    spans = None
    for r in rows_for(src, games):
        if addrof(r) is None:
            _unresolved(r)
            continue
        if spans is None:
            spans = _line_spans(src)
        if not (1 <= r.line <= len(spans)):
            continue
        a, b = spans[r.line - 1]
        yield _Hit((hex(addrof(r)), r.mangled), a, b, src)


def line_owner(src, lineno, games=(2,), path=None):
    """The row whose anchor is the last one at or above `lineno` — which function a source line is
    in. Pass `path` when `src` is not that file's text (e.g. a copy with lines blanked out)."""
    best = None
    for r in (rows_for(path, games) if path else rows_for(src, games)):
        if r.line <= lineno:
            best = r
    return best


def anchor_line(path, mangled, games=(1, 2)):
    """The line (in the marker-free file) the symbol's anchor sits on, or None."""
    for r in rows_for(path, games):
        if r.mangled == mangled:
            return r.line
    return None


def anchor_lines(path, games=(2,)):
    """{line: Row} for a file — for a tool that has a source line and wants the function it is in
    (the marker used to BE that line; now the anchor is the line the marker sat on, which is the
    function's own definition line)."""
    return {r.line: r for r in rows_for(path, games)}


def anchor_span(path, mangled, text=None, games=(1, 2)):
    """(start, end) character offsets of the symbol's anchor line in `text` (read from `path` when
    omitted) — the replacement for a caller that used `m.start()`/`m.end()` of the old marker."""
    if text is None:
        text = open(os.path.join(REPO, path), encoding='utf-8', errors='replace').read()
    ln = anchor_line(path, mangled, games)
    if ln is None or not (1 <= ln <= len(_line_spans(text))):
        return None
    return _line_spans(text)[ln - 1]


def body_span(path, mangled, text=None, games=(1, 2)):
    """(start, end) of the symbol's BODY — text after its opening `{` up to the matching `}` — as
    character offsets into `text`. The old marker's `m.end()` was used for this; an anchor LINE is
    not a substitute because the signature is often on the anchor line itself. Best effort: the
    scan skips comments and string/char literals and only accepts a `{` outside parentheses."""
    if text is None:
        text = open(os.path.join(REPO, path), encoding='utf-8', errors='replace').read()
    ln = anchor_line(path, mangled, games)
    if ln is None:
        return None
    spans = _line_spans(text)
    if not (1 <= ln <= len(spans)):
        return None
    i, n, depth = spans[ln - 1][0], len(text), 0
    while i < n:
        c = text[i]
        if c == '/' and text[i:i + 2] == '//':
            j = text.find('\n', i)
            i = n if j < 0 else j + 1
        elif c == '/' and text[i:i + 2] == '/*':
            j = text.find('*/', i)
            i = n if j < 0 else j + 2
        elif c in '"\'':
            q, i = c, i + 1
            while i < n and text[i] != q:
                i += 2 if text[i] == '\\' else 1
            i += 1
        elif c == '(':
            depth += 1
            i += 1
        elif c == ')':
            depth -= 1
            i += 1
        elif c == '{' and depth == 0:
            open_at, d, i = i, 1, i + 1
            while i < n and d:
                c = text[i]
                if c == '/' and text[i:i + 2] == '//':
                    j = text.find('\n', i)
                    i = n if j < 0 else j + 1
                elif c == '/' and text[i:i + 2] == '/*':
                    j = text.find('*/', i)
                    i = n if j < 0 else j + 2
                elif c in '"\'':
                    q, i = c, i + 1
                    while i < n and text[i] != q:
                        i += 2 if text[i] == '\\' else 1
                    i += 1
                else:
                    d += c == '{'
                    d -= c == '}'
                    i += 1
            return (open_at + 1, i - 1)
        elif c == ';' and depth == 0:
            return None                     # a declaration, not a definition
        else:
            i += 1
    return None


class _Shim:
    """MARKER-compatible: .findall/.finditer with the semantics the old regex had."""

    def __init__(self, games=(2,)):
        self.games = games
        self.pattern = (SITE.pattern if games == (1, 2)
                        else r'//\s*FUNCTION:\s*KOTOR2\s+(0x[0-9a-fA-F]+)\s+(\S+)')

    def findall(self, src, *a):
        return markers(src, self.games)

    def finditer(self, src, *a):
        return finditer(src, self.games)

    def search(self, src, *a):
        return next(iter(self.finditer(src)), None)

    def match(self, src, *a):
        return self.search(src)


MARKER = _Shim()
MARKER_BOTH = _Shim(games=(1, 2))       # the old `KOTOR([12])` shape: rows of both images


# ---------------------------------------------------------------- the table's source
def src_files():
    out = []
    for root, dirs, files in os.walk(os.path.join(REPO, "src")):
        dirs[:] = [d for d in dirs if not d.startswith('.')]
        for f in sorted(files):
            if f.endswith(SRC_EXT):
                out.append(os.path.relpath(os.path.join(root, f), REPO))
    return sorted(out)


def scan(p):
    """[(game, mangled-or-None, line, addr_hex)] for one file, in line order (anchored sites)."""
    out = []
    with open(os.path.join(REPO, p), encoding='utf-8', errors='replace') as fh:
        for i, line in enumerate(fh, 1):
            m = SITE.match(line)
            if m:
                name = m.group(3)
                out.append((int(m.group(1)), name if SYMBOL.match(name) else None, i,
                            m.group(2).lower()))
    return out


# ---------------------------------------------------------------- CLI
def fde_starts(game):
    from match import fde_starts as _f
    from elftools.elf.elffile import ELFFile
    return _f(ELFFile(open(so(game), 'rb')))


def build_table():
    """The table --init would write, plus the report lines. Returns (rows, report)."""
    rep, out = [], []
    for p in src_files():
        sites = scan(p)
        # A marker quoted inside prose is not a row, even where an unanchored regex would see one;
        # all three live in a header whose functions carry real markers in the .cpp next door.
        for ln, line in enumerate(open(os.path.join(REPO, p), encoding='utf-8', errors='replace'), 1):
            if sites and any(s[2] == ln for s in sites):
                continue
            m = LOOSE.search(line)
            if m:
                rep.append(("quoted", p, ln, m.group(0)[:70]))
        for i, (game, name, line, a) in enumerate(sites):
            anchor = line - i                    # the i sites above it are deleted too
            if name is None:
                name = LOCAL.get((p, int(a, 16)))
                rep.append(("no-name", p, line, a, name or "DROPPED (dup of 0x5224c4)"))
                if name is None:
                    continue
            if (p, int(a, 16)) in DROP:
                continue
            exp = dynsyms(game).get(name)
            if exp is None:
                out.append((game, name, p, anchor, a))
                rep.append(("local", p, line, a, name))
            else:
                out.append((game, name, p, anchor, '-'))
                if hex(exp) != a:
                    rep.append(("STALE-ADDR", p, line, a, name, "binary says %s" % hex(exp)))
    return out, rep


def cmd_init(argv):
    table, rep = build_table()
    have = len(rows()) if os.path.exists(TSV) else 0
    if not have and not os.path.exists(TSV):
        have = -1
    if '--dry-run' not in argv and have > 0 and len(table) * 10 < have:
        print("REFUSING to write: the scan found %d marker sites but the table has %d rows.\n"
              "The markup comments are gone (that is why markers() reads the table), so a scan can\n"
              "no longer reproduce it — regenerate only from a tree that still has the comments, or\n"
              "with --force. To ADD a row, append it to docs/fnindex.tsv by hand and run --check."
              % (len(table), have))
        return 2
    k2 = sum(1 for r in table if r[0] == 2)
    kept = [r for r in table if r[4] != '-']
    print("rows %d  (KOTOR2 %d, KOTOR1 %d) in %d files"
          % (len(table), k2, len(table) - k2, len({r[2] for r in table})))
    print("address from the .so: %d    kept in the table: %d" % (len(table) - len(kept), len(kept)))
    for kind in ("local", "STALE-ADDR", "no-name", "quoted"):
        hits = [r for r in rep if r[0] == kind]
        print("\n-- %s (%d) --" % (kind, len(hits)))
        for r in hits:
            print("   " + "  ".join(str(x) for x in r[1:]))
    if '--dry-run' in argv:
        return 0
    with open(TSV, 'w') as fh:
        fh.write("# docs/fnindex.tsv — the function table (tools/fnindex.py; regenerate with\n"
                 "# `tools/fnindex.py --init`, validate with `--check`). One row per marker site.\n"
                 "#   game    KOTOR2 / KOTOR1\n"
                 "#   mangled the original's symbol name (our own name for a stripped-local row)\n"
                 "#   src     the file that transcribes it\n"
                 "#   line    the anchor's line IN THE MARKER-FREE SOURCE (where the marker sat)\n"
                 "#   addr    `-` = read from that image's .dynsym by name; else the literal\n"
                 "#           address of a row the stripped image has no name for\n"
                 "# game\tmangled\tsrc\tline\taddr\n")
        for game, name, p, anchor, a in table:
            fh.write("KOTOR%d\t%s\t%s\t%d\t%s\n" % (game, name or '', p, anchor, a))
    print("\nwrote %s" % os.path.relpath(TSV, REPO))
    return 0


def cmd_check(argv):
    bad, local = [], []
    starts = {g: set(fde_starts(g)) for g in (1, 2)}
    for r in rows():
        if r.mangled is None:
            continue
        if r.addr is not None:
            local.append(r)
            if int(r.addr, 16) not in starts[r.game]:
                bad.append(("%s: not an FDE start" % r.addr, r))
            continue
        exp = dynsyms(r.game).get(r.mangled)
        if exp is None:
            bad.append(("names no symbol in the image", r))
    print("rows %d, addresses read from the .so %d, kept in the table %d"
          % (len(rows()), len(rows()) - len(local), len(local)))
    for why, r in bad:
        print("   BAD   %s:%d %s  %s" % (r.src, r.line, r.mangled, why))
    for r in local:
        print("   local %s:%d %s @%s" % (r.src, r.line, r.mangled, r.addr))
    return 1 if bad else 0


def cmd_owners(argv):
    """Rows whose owning TU's object does not define them. INFORMATIONAL: 378 are expected —
    the `_instances/*.cpp` TUs force an instantiation whose body lives in a header, and a weak inline
    is emitted by every TU that odr-uses it while ONE of them owns the row. A row here means the row
    and the object disagree, not that the table is wrong."""
    from match import obj_functions
    from tuname import tu_name
    miss, objs = defaultdict(list), {}
    for r in rows():
        if r.mangled is None or not r.src.endswith('.cpp'):
            continue
        tu = tu_name(r.src)
        if tu not in objs:
            try:
                objs[tu] = obj_functions(os.path.join(REPO, "build", tu + ".o"))
            except Exception:
                objs[tu] = None
        ours = objs[tu]
        if ours is None:
            continue
        if r.mangled not in ours:
            miss[tu].append(r.mangled)
    print("rows whose owning object does not define them: %d in %d TUs"
          % (sum(len(v) for v in miss.values()), len(miss)))
    for tu in sorted(miss):
        print("   %-34s %d" % (tu, len(miss[tu])))
    return 0


def cmd_dup(argv):
    byname, byaddr = defaultdict(list), defaultdict(list)
    for r in rows():
        byname[(r.game, r.mangled)].append(r)
        byaddr[(r.game, addr(r.game, r.mangled))].append(r)
    for (g, m), v in sorted(byname.items(), key=lambda kv: str(kv[0])):
        if len(v) > 1:
            addrs = {addrof(r) for r in v}
            print("name x%d  KOTOR%d %s  %s%s" % (
                len(v), g, m, " ".join("%s:%d" % (r.src, r.line) for r in v),
                "   [DIFFERENT ADDRESSES %s]" % " ".join(sorted(hex(a) for a in addrs))
                if len(addrs) > 1 else ""))
    for (g, a), v in sorted(byaddr.items(), key=lambda kv: (kv[0][0], kv[0][1] or 0)):
        names = {r.mangled for r in v}
        if len(v) > 1 and len(names) > 1:
            print("addr %s  KOTOR%d  %s" % (hex(a), g, " ".join(sorted(str(n) for n in names))))
    return 0


def _leaf(mangled):
    """The bare identifier a row's definition must contain: `_ZN21CClass17MethodEv` -> `Method`,
    `_ZL9snprintffPcmPKcz` -> `snprintff`, `Java_com_aspyr_a_b` -> the last `_`-separated part is NOT
    an identifier, so those return None and are skipped."""
    m = re.match(r'_Z(?:[NLKVR]*)(\d+)', mangled)
    if not m:
        return mangled if re.match(r'^[A-Za-z_]\w*$', mangled) and not mangled.startswith('_Z') else None
    i, last = m.start(1), None
    while i < len(mangled) and mangled[i].isdigit():
        j = i
        while j < len(mangled) and mangled[j].isdigit():
            j += 1
        k = int(mangled[i:j])
        last = mangled[j:j + k] or last
        i = j + k
        if mangled[i:i + 2] in ('Th', '_Z') or mangled[i:i + 1] in ('I', 'B', 'S'):
            break
    return last if last and re.match(r'^[A-Za-z_]\w*$', last) else None


def cmd_lines(argv):
    """Does every row's ANCHOR LINE name its function? The anchor is the slot the markup comment
    occupied, so it is the definition line (or the first note line when notes followed the marker).
    A row that fails here has a stale `line` — the usual cause is a function that MOVED between files
    or statements inserted above it without a re-init."""
    bad, skipped, cache = [], 0, {}
    for r in rows():
        leaf = _leaf(r.mangled or '')
        if leaf is None:
            skipped += 1
            continue
        if r.src not in cache:
            try:
                cache[r.src] = open(os.path.join(REPO, r.src), encoding='utf-8',
                                    errors='replace').read().splitlines()
            except OSError:
                cache[r.src] = None
        lines = cache[r.src]
        if lines is None:
            bad.append((r, 'missing file'))
            continue
        if r.line < 1 or r.line > len(lines):
            bad.append((r, 'line %d past EOF (%d lines)' % (r.line, len(lines))))
            continue
        # The anchor is the slot the markup comment occupied, and this tree writes the notes below
        # the marker (the marker is the first line of the block), so the definition is somewhere
        # after it: search forward to the next anchor / 80 lines for `leaf(`.
        nxt = next((x.line for x in by_src()[r.src] if x.line > r.line), len(lines) + 1)
        blank = not lines[r.line - 1].strip()
        end = min(nxt - 1, len(lines))
        hit = any(re.search(r'\b%s\s*[(:<]' % re.escape(leaf), lines[t]) for t in range(r.line - 1, end))
        if not hit:
            bad.append((r, 'no `%s` in lines %d-%d%s (anchor: %r)'
                        % (leaf, r.line, end, '; marker-only row' if blank or end == r.line - 1 else '',
                           lines[r.line - 1].strip()[:46])))
    print("rows checked %d, skipped (no bare identifier: Java_/SDL_main/…) %d, STALE %d"
          % (len(rows()), skipped, len(bad)))
    for r, why in bad[:60]:
        print("   %-58s:%-6d %s" % (r.src, r.line, why))
    return 1 if bad else 0


def cmd_stats(argv):
    c = defaultdict(int)
    for r in rows():
        c['rows'] += 1
        c['game%d' % r.game] += 1
        c['no-name'] += not r.mangled
        c['addr-in-table'] += r.addr is not None
    c['files'] = len(by_src())
    for k in sorted(c):
        print("%-14s %d" % (k, c[k]))
    return 0


def cmd_strip(argv):
    dry = '--dry-run' in argv
    changed = removed = 0
    for p in src_files():
        sites = scan(p)
        want = [r for r in by_src().get(p, ())] if os.path.exists(TSV) else []
        if not sites:
            if want:
                print("   SKIP %s: %d rows in the table but no marker sites left (already stripped?)"
                      % (p, len(want)))
            continue
        for i, (game, name, line, a) in enumerate(sites):
            if name is None and LOCAL.get((p, int(a, 16))) is None:
                sites[i] = (game, None, line, a)     # dropped row: remove the comment anyway
        lines = open(os.path.join(REPO, p), encoding='utf-8', errors='replace').readlines()
        kill = {ln for (g, n, ln, a) in sites}
        out = [l for i, l in enumerate(lines, 1) if i not in kill]
        if len(out) == len(lines):
            continue
        if dry:
            print("   would remove %d lines from %s" % (len(lines) - len(out), p))
        else:
            with open(os.path.join(REPO, p), 'w', encoding='utf-8') as fh:
                fh.writelines(out)
        changed += 1
        removed += len(lines) - len(out)
    print("%s %d lines from %d files" % ("would remove" if dry else "removed", removed, changed))
    return 0


def cmd_reanchor(argv):
    """Line anchors are positions in the TEXT, so an edit that adds or removes lines above a row
    moves it (the decomment sweep after v2236 left 13,121 of 15,084 rows STALE). Each row's line in
    REV's table is mapped through difflib's opcodes between the file at REV and now: a line in an
    unchanged run moves with it, a line inside a replaced run goes to the replacement's start."""
    import difflib, collections, subprocess
    if not argv or argv[0].startswith('-'):
        print("usage: fnindex.py --reanchor REV [--dry-run]")
        return 2
    rev, dry = argv[0], '--dry-run' in argv

    def git(*a):
        return subprocess.run(['git', *a], cwd=REPO, capture_output=True, text=True).stdout
    renames = {}
    for l in git('diff', '-M', '--name-status', rev, '--', 'src').split('\n'):
        f = l.split('\t')
        if f[0].startswith('R'):
            renames[f[2]] = f[1]
    old_rows = collections.defaultdict(list)
    for l in git('show', rev + ':' + os.path.relpath(TSV, REPO)).split('\n'):
        f = l.split('\t')
        if len(f) == 5 and not l.startswith('#'):
            old_rows[(f[0], f[1], f[2])].append(int(f[3]))
    text = open(TSV, encoding='utf-8').read().split('\n')
    maps, used, c = {}, collections.Counter(), collections.Counter()
    for i, l in enumerate(text):
        f = l.split('\t')
        if len(f) != 5 or l.startswith('#'):
            continue
        key = (f[0], f[1], renames.get(f[2], f[2]))
        if used[key] >= len(old_rows.get(key, ())):
            c['new since REV (kept)'] += 1
            continue
        line = old_rows[key][used[key]]
        used[key] += 1
        if f[2] not in maps:
            a = git('show', '%s:%s' % (rev, key[2])).split('\n')
            b = open(os.path.join(REPO, f[2]), encoding='utf-8', errors='replace').read().split('\n')
            m = {}
            for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
                for k in range(i1, i2):
                    m[k + 1] = j1 + (k - i1) + 1 if tag == 'equal' else j1 + 1
            maps[f[2]] = (m, len(b))
        m, n = maps[f[2]]
        new = str(m.get(line, min(line, n + 1)))
        c['moved' if new != f[3] else 'unchanged'] += 1
        f[3] = new
        text[i] = '\t'.join(f)
    print(', '.join('%s %d' % kv for kv in sorted(c.items())))
    if not dry:
        with open(TSV, 'w', encoding='utf-8') as fh:
            fh.write('\n'.join(text))
    return 0


def main():
    argv = sys.argv[1:]
    if not argv or argv[0] in ('-h', '--help'):
        print(__doc__)
        return 0
    cmd, rest = argv[0], argv[1:]
    for name, fn in (('--init', cmd_init), ('--strip', cmd_strip), ('--check', cmd_check),
                     ('--lines', cmd_lines), ('--owners', cmd_owners), ('--dup', cmd_dup),
                     ('--stats', cmd_stats), ('--reanchor', cmd_reanchor)):
        if cmd == name:
            return fn(rest)
    print("unknown command %r" % cmd)
    return 2


if __name__ == '__main__':
    sys.exit(main())
