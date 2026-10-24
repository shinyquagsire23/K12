"""tuname.py -- the ONE rule that names a TU and its object file (path migration).

A TU is named after its `.cpp` stem, and its object is `build/<TU>.o`. Until the path migration
every stem was unique engine-wide, and many tools still rely on that. The original source layout
has a few same-named files in different directories (`AURORA/aurmodel.cpp` and
`aurorainterface/aurmodel.cpp`), so for a stem that more than one `src/**/*.cpp` shares, the TU
name becomes `<parent dir>_<stem>`, e.g. `AURORA_aurmodel`. Every other TU keeps its plain stem,
so every existing `build/<TU>.o`, `--tu` argument and doc reference is unchanged.

`tools/cc.sh` applies the same rule in bash; keep the two in sync.
"""
import functools, os

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))


@functools.lru_cache(maxsize=1)
def _stems():
    """stem -> [repo-relative .cpp paths under src/]"""
    out = {}
    for root, dirs, files in os.walk(os.path.join(REPO, 'src')):
        dirs[:] = [d for d in dirs if not d.startswith('.')]
        for f in files:
            if f.endswith('.cpp'):
                out.setdefault(f[:-4], []).append(os.path.relpath(os.path.join(root, f), REPO))
    return out


def tu_name(src):
    """`src/libsource/AURORA/aurmodel.cpp` -> `AURORA_aurmodel` (shared stem) or the plain stem."""
    stem = os.path.splitext(os.path.basename(src))[0]
    if len(_stems().get(stem, ())) > 1:
        return os.path.basename(os.path.dirname(os.path.abspath(src))) + '_' + stem
    return stem


def obj_path(src):
    """build/<TU>.o for a source path (repo-relative, like every caller's `build/...`)."""
    return f"build/{tu_name(src)}.o"


def src_for_tu(tu):
    """Inverse of tu_name: a TU name (plain stem or `<dir>_<stem>`) -> its repo-relative .cpp, or
    None. A bare shared stem is ambiguous and returns None."""
    paths = _stems().get(tu)
    if paths and len(paths) == 1:
        return paths[0]
    if '_' in tu:
        d, _, stem = tu.partition('_')
        for p in _stems().get(stem, ()):
            if os.path.basename(os.path.dirname(p)) == d:
                return p
    return None
