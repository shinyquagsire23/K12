#!/usr/bin/env python3
"""ccflags.py — emit the matching build's compile flags as a CMake include file.

`tools/cc.sh` is the single source of truth for how an engine TU is compiled (flags + the per-TU
`cflags/_all` / `cflags/<TU>` override files). The hybrid build must compile the SAME objects, so this
script PARSES cc.sh instead of copying its flag list; a flag landed there reaches the hybrid build on
the next configure. It never runs cc.sh (that would write into build/).

Usage (from CMake at configure time):
  ccflags.py --repo <root> --out <file.cmake> [--overlay <dir>]
Emits:
  set(K2_BASE_FLAGS "<flags>")                 # everything between "$CXX" and -c, minus -I
  set(K2_INCLUDE_DIRS "<abs>;<abs>;...")        # the -I list, absolutized
  set(K2_SOURCES "<abs .cpp>;...")
  set(K2_CFLAG_FILES "<abs cflags files>;...")  # for CMAKE_CONFIGURE_DEPENDS
  set_source_files_properties(<src> PROPERTIES COMPILE_FLAGS "<per-TU flags>")

THE OVERLAY (`--overlay hybrid/overlay`, a mirror of src/'s directory layout)
---------------------------------------------------------------------------
A file at <overlay>/<same relative path as under src/> REPLACES the src/ file of that path for the
hybrid build only. Nothing under src/ is read for that TU, and the matching build (tools/cc.sh,
tools/progress.py, build/) never looks at the overlay at all.

That separation is the point: experiments that would change a byte-matched row — a debug hook, a
swapped GUI resref, an extra control binding — live in the overlay, so they cannot regress
`progress.py` and cannot collide with someone else transcribing under src/.

Overlay roots are prepended to the include path, so an overlaid HEADER also wins; `cflags/_all` /
`cflags/<TU>` are taken from the overlay directory when present, else from the src/ one. Every
substitution is printed at configure time, because a silently-overlaid TU would be very confusing
to debug.
"""
import argparse, os, re, shlex, sys


def _split(block):
    """Bash-ish word split of a cc.sh fragment: backtick comment splices, # lines, continuations."""
    block = re.sub(r'`[^`]*`', ' ', block, flags=re.S)        # backtick comment splices
    block = '\n'.join(l for l in block.splitlines() if not re.match(r'\s*#', l))
    block = re.sub(r'\\(\n|$)', ' ', block)   # line continuations (the last one loses its \n above)
    return shlex.split(block)


def base_flags(cc_sh_text):
    """Return the literal argument list cc.sh passes to $CXX (minus -c/-o and the ${...} splices).

    cc.sh collects its flags in a `FLAGS=( ... )` array and then compiles with
        "$CXX" "${FLAGS[@]}" ${TU_CFLAGS} ${EXTRA_CFLAGS:-} -c "$SRC" -o "$OUT"
    Both halves are read: the array for the flags, the compile line so that a literal flag spelled
    there (rather than in the array) cannot be silently dropped from the hybrid build.
    """
    m = re.search(r'^FLAGS=\(\n(.*?)^\)\s*$', cc_sh_text, re.S | re.M)
    if not m:
        sys.exit('ccflags.py: could not find the FLAGS=( ... ) array in tools/cc.sh')
    flags = [x for x in _split(m.group(1)) if not x.startswith('${')]

    inv = re.search(r'^"\$CXX"([^\n]*?)-c "\$SRC"', cc_sh_text, re.M)
    if inv is None:
        sys.exit('ccflags.py: could not find the \'"$CXX" ... -c "$SRC"\' line in tools/cc.sh')
    for x in _split(inv.group(1)):                            # literals spelled on the command line
        if x != '${FLAGS[@]}' and not x.startswith('${'):
            flags.append(x)
    return flags


def read_cflags(path):
    """Same semantics as cc.sh's read_cflags: drop lines starting with '#', join the rest."""
    if not os.path.isfile(path):
        return []
    out = []
    for line in open(path, encoding='utf-8'):
        if re.match(r'^\s*#', line):
            continue
        out.append(line.rstrip('\n'))
    # cc.sh does `tr '\n' ' '` then word-splits the unquoted expansion.
    return ' '.join(out).split()


def cmake_quote(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--exclude-tu', action='append', default=[],
                    help='TU basename to leave out of the hybrid build (repeatable)')
    ap.add_argument('--overlay', default=None,
                    help='directory mirroring src/; a file there replaces the src/ file of the '
                         'same relative path (hybrid build only)')
    ap.add_argument('--ui', choices=('touch', 'pc', 'xbox'), default='touch',
                    help="which UI the build presents. cc.sh compiles the shipped TOUCH build "
                         "(-DK2_UI_TOUCH=1); 'xbox' swaps that for -DK2_UI_XBOX=1 and 'pc' for -DK2_UI_PC=1 "
                         "(CMakeLists.txt K2P_UI documents the three)")
    a = ap.parse_args()
    repo = os.path.abspath(a.repo)
    src_root = os.path.join(repo, 'src')
    overlay = os.path.abspath(a.overlay) if a.overlay and os.path.isdir(a.overlay) else None

    def overlaid(path):
        """The overlay's replacement for a src/ path, or the path itself."""
        if not overlay:
            return path
        cand = os.path.join(overlay, os.path.relpath(path, src_root))
        return cand if os.path.isfile(cand) else path

    args = base_flags(open(os.path.join(repo, 'tools', 'cc.sh'), encoding='utf-8').read())
    incs, flags = [], []
    it = iter(args)
    for x in it:
        if x == '-I':
            incs.append(os.path.join(repo, next(it)))
        elif x.startswith('-I'):
            incs.append(os.path.join(repo, x[2:]))
        else:
            flags.append(x)
    if '-DK2_UI_TOUCH=1' not in flags:
        sys.exit('ccflags.py: tools/cc.sh no longer passes -DK2_UI_TOUCH=1; update the --ui swap')
    ui_define = {'touch': ['-DK2_UI_TOUCH=1'], 'pc': ['-DK2_UI_PC=1'], 'xbox': ['-DK2_UI_XBOX=1']}[a.ui]
    i = flags.index('-DK2_UI_TOUCH=1')
    flags[i:i + 1] = ui_define

    # An overlaid header must win over the src/ one, so every -I gains its overlay twin in front.
    if overlay:
        mirrored = []
        for inc in incs:
            inc = os.path.abspath(inc)
            if inc == src_root:
                mirrored.append(overlay)
            elif inc.startswith(src_root + os.sep):
                # Emitted whether or not the directory exists yet: clang ignores a missing -I, and
                # this way creating an overlay subdir does not need a fresh configure to be seen.
                mirrored.append(os.path.join(overlay, os.path.relpath(inc, src_root)))
        incs = mirrored + incs

    sources, cflag_files, props, swapped = [], [], [], []
    for root, dirs, files in os.walk(src_root):
        dirs[:] = sorted(d for d in dirs if not d.startswith('.'))
        for fn in sorted(files):
            p = os.path.join(root, fn)
            if os.path.basename(root) == 'cflags':
                cflag_files.append(p)
            if not fn.endswith('.cpp'):
                continue
            tu = fn[:-4]
            if tu in a.exclude_tu:
                continue
            use = overlaid(p)
            if use != p:
                swapped.append(os.path.relpath(p, src_root))
            sources.append(use)
            # Per-TU flags follow the file that is actually compiled.
            fdir = os.path.dirname(use)
            extra = read_cflags(os.path.join(fdir, 'cflags', '_all')) + \
                read_cflags(os.path.join(fdir, 'cflags', tu))
            if not extra and use != p:      # overlay dir carries no cflags: inherit src/'s
                extra = read_cflags(os.path.join(root, 'cflags', '_all')) + \
                    read_cflags(os.path.join(root, 'cflags', tu))
            if extra:
                # COMPILE_FLAGS (a plain string), not COMPILE_OPTIONS: CMake de-duplicates option
                # lists, which would silently eat the second `-mllvm` of a pair.
                props.append((use, ' '.join(shlex.quote(e) for e in extra)))

    # Overlay-only TUs (no src/ counterpart) still get compiled.
    if overlay:
        for root, dirs, files in os.walk(overlay):
            dirs[:] = sorted(d for d in dirs if not d.startswith('.'))
            for fn in sorted(files):
                if os.path.basename(root) == 'cflags':
                    cflag_files.append(os.path.join(root, fn))
                if not fn.endswith('.cpp'):
                    continue
                p = os.path.join(root, fn)
                if p in sources or fn[:-4] in a.exclude_tu:
                    continue
                if not os.path.isfile(os.path.join(src_root, os.path.relpath(p, overlay))):
                    sources.append(p)
                    swapped.append('(new) ' + os.path.relpath(p, overlay))

    # CMake wants forward slashes; on a Windows host os.path.join produces backslashes.
    fwd = lambda ps: [x.replace(os.sep, '/') for x in ps]
    incs, sources, cflag_files = fwd(incs), fwd(sources), fwd(cflag_files)
    props = [(p.replace(os.sep, '/'), opt) for p, opt in props]

    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, 'w', encoding='utf-8') as f:
        f.write('# GENERATED by platforms/tools/ccflags.py from tools/cc.sh — do not edit\n')
        f.write('set(K2_BASE_FLAGS %s)\n' % cmake_quote(' '.join(flags)))
        f.write('set(K2_INCLUDE_DIRS %s)\n' % cmake_quote(';'.join(incs)))
        f.write('set(K2_SOURCES %s)\n' % cmake_quote(';'.join(sources)))
        f.write('set(K2_CFLAG_FILES %s)\n' % cmake_quote(';'.join(cflag_files)))
        for p, opt in props:
            f.write('set_source_files_properties(%s PROPERTIES COMPILE_FLAGS %s)\n'
                    % (cmake_quote(p), cmake_quote(opt)))
    print('ccflags: %d TUs (%d with per-TU flags), base: %s'
          % (len(sources), len(props), ' '.join(flags)))
    if swapped:
        print('ccflags: OVERLAY active (%s) — %d file(s) replaced:' % (overlay, len(swapped)))
        for rel in swapped:
            print('    %s' % rel)


if __name__ == '__main__':
    main()
