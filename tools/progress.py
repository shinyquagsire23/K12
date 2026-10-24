#!/usr/bin/env python3
"""
progress.py — completion dashboard for the KOTOR2 byte-match decomp.

Compiles every src/**/*.cpp (via tools/cc.sh), runs the matcher, and reports:
  - exact-matched functions and their total byte size,
  - the % against the engine's matchable function bytes (defined .text FUNCs in the .so that
    are in engine scope — third-party libs + port shims excluded via in_engine_scope).
One number to track. Run from repo root.

⏱ IT IS A PIPELINE, NOT THREE PHASES (v1063): the compiles run `jobs`-at-a-time in a thread pool
(default one per core; `-j N` / PROGRESS_JOBS=N overrides), each TU is parsed and byte-compared the
moment its object lands, and the whole-tree lint is computed in a background thread during the
subprocess waits. 84 s → ~14 s wall on a 10-core box (13.8-14.8 s across runs) with byte-identical output; what remains is
the clang compiles themselves (~126 s CPU, 10-13 s wall at 10 workers — oversubscribing measured
slightly worse). See docs/TOOLS.md for the profile.
"""
import subprocess, glob, sys, os, re, concurrent.futures, threading
from tuname import tu_name as _tu_name   # build/<TU>.o naming (shared stems)
sys.path.insert(0, "tools")
from match import orig_functions, obj_functions, mask_bytes, MARKER, in_engine_scope, \
    fde_only_functions

SO = "decomp_bins/swkotorii_extracted/lib/arm64-v8a/libkotor2.so"


def n_workers():
    """How many cc.sh runs to keep in flight. `-j N` (or PROGRESS_JOBS=N) overrides the
    default of one per core. Every compile is a subprocess wait, so this is the whole cost of
    the sweep — 291 sequential clang runs is ~78 s, ~11 s at 10 in flight."""
    for i, a in enumerate(sys.argv):
        if a == "-j" and i + 1 < len(sys.argv):
            return max(1, int(sys.argv[i + 1]))
        if a.startswith("-j") and a[2:].isdigit():
            return max(1, int(a[2:]))
    return max(1, int(os.environ.get("PROGRESS_JOBS") or (os.cpu_count() or 1)))


def build_one(src):
    r = subprocess.run(["tools/cc.sh", src], capture_output=True, text=True)
    return r.returncode, r.stderr


ADDR_TU = {}   # addr -> the TU whose marker scored it (PROGRESS_DUMP)


def score_tu(src, om, getb, matched_addrs, transcribed_addrs, out_of_scope, fde_rows,
             fde_matched):
    """Score one TU: parse its object, then byte-compare every marker against the .so."""
    tu = _tu_name(src)
    ours = obj_functions(f"build/{tu}.o")
    for addr_s, mangled in MARKER.findall(open(src).read()):
        addr = int(addr_s, 16)
        if addr not in om:
            # An internal-linkage row, with no `.dynsym` entry: its size comes from `.eh_frame_hdr`
            # and it is tallied on its own line rather than into `transcribed`. The denominator is
            # dynsym-derived, so counting this row in the numerator would break the
            # numerator-subsets-the-denominator invariant.
            sz = fde_rows.get(addr)
            if sz is None:
                continue                     # no sized engine symbol at all — nothing to compare
            if mangled in ours:
                mb, mrel = ours[mangled]
                if len(mb) == sz and mask_bytes(getb(addr, sz), addr, mrel) == mask_bytes(mb, 0, mrel):
                    fde_matched[addr] = sz
            continue
        name, sz = om[addr]
        # The numerator has to respect the denominator's scope: `total_engine_bytes` excludes
        # out-of-scope FUNCs, and without this check a marker for one still lands in
        # `transcribed_addrs` and is measured against a total that excludes its bytes. The scope
        # filter is on the original symbol's name, the same string the denominator classifies, and
        # such rows are tallied separately so the work stays visible instead of vanishing.
        if not in_engine_scope(name):
            out_of_scope[addr] = sz; continue
        transcribed_addrs[addr] = sz
        ADDR_TU.setdefault(addr, src)
        if mangled not in ours: continue     # transcribed but didn't compile to that symbol
        mb, mrel = ours[mangled]
        oM = mask_bytes(getb(addr, sz), addr, mrel)
        mM = mask_bytes(mb, 0, mrel)
        if len(oM) == len(mM) and oM == mM:
            matched_addrs[addr] = sz


def build_and_score(srcs, om_getter, fde_getter, matched_addrs, transcribed_addrs, out_of_scope,
                    fde_rows, fde_matched, jobs):
    """PIPELINE, not two phases. The 291 compiles (~13 s wall) and the ELF parsing + byte
    comparing (~6 s) are both serial-bound in different resources, so each TU is scored the
    moment its object lands instead of after every compile has finished. `om_getter` is called
    while the first compiles are already in flight (parsing the .so is another ~1.5 s that costs
    nothing there). Returns [(src, stderr)] for the TUs whose build failed.

    Threads, not processes: every compile is just a subprocess wait, so the GIL is released and
    the work really overlaps. ⚠ Parallel-safe only because TU NAMES are unique engine-wide — the
    invariant build/<TU>.o rests on. A stem two src/ files share (the original layout has
    `AURORA/aurmodel.cpp` and `aurorainterface/aurmodel.cpp`) is named `<dir>_<stem>` by
    tools/tuname.py and tools/cc.sh, so the two compiles never race for one object.
    """
    fails = []
    ex = concurrent.futures.ThreadPoolExecutor(max_workers=jobs)
    try:
        futs = {ex.submit(build_one, s): s for s in srcs}
        om, getb = om_getter()
        fde_rows.update(fde_getter(om))
        for fut in concurrent.futures.as_completed(futs):
            src = futs[fut]; rc, stderr = fut.result()
            if rc != 0:
                fails.append((src, stderr)); continue
            score_tu(src, om, getb, matched_addrs, transcribed_addrs, out_of_scope, fde_rows,
                     fde_matched)
    finally:
        ex.shutdown()
    # sorted → deterministic failure report regardless of completion order
    return sorted(fails), om


def lint_in_background(out):
    """idiom debt + m_Unk tallies, computed in a THREAD while the compiles run. `lint_idiom.collect`
    is ~4 s of pure-CPU Python with no dependency on any object file, and the compile stage is
    almost entirely subprocess WAITING — so it costs nothing here and would otherwise be the last
    serial 4 s of the run. Results land in `out['rows']` (or `out['err']`)."""
    def run():
        try:
            import pathlib, importlib.util
            spec = importlib.util.spec_from_file_location(
                "lint_idiom", pathlib.Path(__file__).parent / "lint_idiom.py")
            li = importlib.util.module_from_spec(spec); spec.loader.exec_module(li)
            out["rows"] = [r for v in li.collect().values() for r in v]
        except Exception as e:      # lint failure must not take the sweep down with it
            out["err"] = e
    t = threading.Thread(target=run, daemon=True)
    t.start()
    return t


def rawoff_in_background(out):
    """tools/rawoffast.py's counted raw-offset sites (the AST census; tags allow file formats),
    run as a subprocess beside the compiles. The count must stay 0."""
    def run():
        try:
            import pathlib, re as _re
            tool = pathlib.Path(__file__).parent / "rawoffast.py"
            if not tool.exists():   # tools/stage_k12.py does not ship the AST census
                out["absent"] = True
                return
            r = subprocess.run([sys.executable, str(tool)],
                               capture_output=True, text=True)
            m = _re.search(r"counted (\d+)", r.stdout)
            out["n"] = int(m.group(1)) if m else None
            out["lines"] = [l for l in r.stdout.splitlines() if not l.startswith("counted")][:5]
        except Exception as e:
            out["err"] = e
    t = threading.Thread(target=run, daemon=True)
    t.start()
    return t


def total_engine_bytes(om):
    """Engine denominator: defined .text FUNCs, minus the won't-decompile buckets
    (third-party libs + Aspyr port shims — see match.py's in_engine_scope)."""
    scoped = [(a, ns) for a, ns in om.items() if in_engine_scope(ns[0])]
    excl_b = sum(sz for a, (n, sz) in om.items() if not in_engine_scope(n))
    return (sum(sz for _, (_, sz) in scoped), len(scoped),
            sum(sz for _, (_, sz) in om.items()), len(om), excl_b)

def main():
    srcs = sorted(glob.glob("src/**/*.cpp", recursive=True))
    # transcribed = we wrote source for it (a marker resolving to a real sized engine FUNC),
    #   whether or not it byte-matches. matched = the subset that is byte-exact.
    #
    # Counts distinct addresses, not markers. `total_engine_bytes` builds the denominator from `om`,
    # which is keyed by address, so an address with several dynsym aliases -- the usual C1/C2 or
    # D1/D2 pair emitted at one address, or `CSWGuiTestPattern`'s vtable slot 0 aliasing
    # `_ZN11CSWGuiPanelD2Ev` -- contributes one function and its size once. Scoring every marker is
    # still correct; only the tally dedupes, and a marker whose twin already matched adds no
    # coverage.
    matched_addrs = {}       # addr -> size, for markers that byte-matched
    transcribed_addrs = {}   # addr -> size, for markers that resolved to a sized engine FUNC
    out_of_scope = {}        # addr -> size, for in-src markers the engine scope EXCLUDES
    fde_rows = {}            # addr -> size, for internal-linkage rows (FDE-only, see match.py)
    fde_matched = {}         # addr -> size, the byte-exact subset of the above
    lint_out = {}
    lint_thread = lint_in_background(lint_out)          # overlaps the whole compile stage
    rawoff_out = {}
    rawoff_thread = rawoff_in_background(rawoff_out)    # ~70 s of clang parses, same overlap
    fails, om = build_and_score(srcs, lambda: orig_functions(SO),
                                lambda om: fde_only_functions(SO, om),
                                matched_addrs, transcribed_addrs, out_of_scope, fde_rows,
                                fde_matched, n_workers())
    for src, stderr in fails:
        print(f"  [build FAIL] {src}\n{stderr[-400:]}")
    if os.environ.get("PROGRESS_DUMP"):   # one `addr exact|transcribed` row per address, for A/B diffs
        with open(os.environ["PROGRESS_DUMP"], "w") as f:
            for a in sorted(transcribed_addrs):
                f.write(f"{a:#x} {'exact' if a in matched_addrs else 'transcribed'} {ADDR_TU.get(a, '?')}\n")
    matched = len(matched_addrs);         matched_bytes = sum(matched_addrs.values())
    transcribed = len(transcribed_addrs); transcribed_bytes = sum(transcribed_addrs.values())
    tot_bytes, tot_funcs, raw_bytes, raw_funcs, excl_bytes = total_engine_bytes(om)
    pct = lambda a, b: (100.0 * a / b) if b else 0.0
    print(f"engine          : {tot_funcs:,} funcs / {tot_bytes:,} bytes"
          f"  (of {raw_funcs:,}/{raw_bytes:,} raw dynsym — excludes {excl_bytes:,} B of"
          " third-party/shim, see match.py in_engine_scope)")
    print(f"transcribed     : {transcribed} funcs / {transcribed_bytes:,} bytes"
          f"  ({pct(transcribed_bytes, tot_bytes):.4f}% of engine)")
    print(f"exact-matched   : {matched} funcs / {matched_bytes:,} bytes"
          f"  ({pct(matched_bytes, tot_bytes):.4f}% of engine)  <- COVERAGE")
    print(f"match rate      : {matched}/{transcribed} funcs"
          f" ({pct(matched, transcribed):.1f}%),"
          f" {matched_bytes:,}/{transcribed_bytes:,} bytes ({pct(matched_bytes, transcribed_bytes):.1f}%)"
          "  <- of what we transcribed")
    if out_of_scope:
        print(f"out-of-scope    : {len(out_of_scope)} markers / {sum(out_of_scope.values()):,} bytes"
              " inscribed in src/ but OUTSIDE the engine scope (port shim) — scored, NOT counted"
              " in the numerator; see match.py in_engine_scope")
    # Markers that resolved to an internal-linkage function, with no `.dynsym` row. Sized from
    # `.eh_frame_hdr` and reported on their own line: the engine denominator is dynsym-derived, so
    # folding them into `transcribed` would let the numerator exceed its own universe. `fde_rows` is
    # every such row in the image; only the ones src/ has markers for get printed.
    fde_count = len(fde_matched)
    if fde_count:
        print(f"fde-only        : {fde_count} rows / {sum(fde_matched.values()):,} bytes byte-exact"
              " — INTERNAL-LINKAGE FUNCs the dynsym cannot name (static / anonymous namespace),"
              " sized from .eh_frame_hdr; NOT counted above"
              "  <- tools/match.py fde_only_functions")
    lint_thread.join()
    rawoff_thread.join()
    if rawoff_out.get("absent"):
        print("raw offsets     : not run (tools/rawoffast.py is not in this tree)")
    elif rawoff_out.get("n") is not None:
        print(f"raw offsets     : {rawoff_out['n']} counted  <- tools/rawoffast.py; must be 0"
              f" (// raw-offset: <why> allows a file-format buffer)")
        for l in rawoff_out.get("lines", []):
            print("                  " + l)
    else:
        print(f"raw offsets     : census failed ({rawoff_out.get('err')})")
    if "rows" in lint_out:
        rows = lint_out["rows"]
        # H (m_Unk placeholder fields) is an ungated, separate tally: keep it out of the A-G "idiom
        # debt" headline and the untagged gate.
        h_total = sum(1 for r in rows if "H" in r[1])
        core = len(rows) - h_total
        untag = sum(1 for r in rows if not r[3] and (set(r[1]) - {"H"}))
        print(f"idiom debt      : {core} non-idiomatic findings"
              f" ({untag} untagged)  <- tools/lint_idiom.py; drive to 0")
        print(f"m_Unk fields    : {h_total} placeholder-named fields"
              f"  <- lowest priority; rename as module interlinks surface")
    else:
        print(f"idiom debt      : lint failed ({lint_out.get('err')})")

if __name__ == "__main__":
    main()
