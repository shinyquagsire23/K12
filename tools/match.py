#!/usr/bin/env python3
"""
match.py — reccmp-style, symbol-anchored function comparator for the KOTOR2 AArch64 decomp.

For each marker (address + mangled name) the source file's marker table lists:
  - pull our function's bytes from build/<TU>.o (by the mangled symbol name),
  - pull the original function's bytes from the target .so (by address; size from its dynsym),
  - mask AArch64 PC-relative / relocated operands on both streams so only opcodes + fixed
    operands remain, then byte-compare.
Reports MATCH or DIFF(n) per function.

Masking rationale: our .o has BL/ADRP/ADD targeting reloc'd symbols (imm=0 + a relocation);
the linked .so has the same instructions with real PC-relative immediates. Zeroing the
immediate bit-fields of all PC-relative instruction forms makes the two comparable: a true
code match differs only in those masked bits.

Usage:
  tools/match.py src/<module>/<TU>/<TU>.cpp [--so <path>] [--obj build/<TU>.o] [--dump]
Run from repo root.
"""
import sys, re, struct, argparse, subprocess, bisect
from tuname import tu_name as _tu_name   # build/<TU>.o naming (shared stems)
from elftools.elf.elffile import ELFFile

TARGET_SO = "decomp_bins/swkotorii_extracted/lib/arm64-v8a/libkotor2.so"

# ---- AArch64 immediate masking -------------------------------------------------
# Each entry: (mask that keeps opcode/register bits, i.e. zeroes the PC-rel immediate).
# We detect the form from the 32-bit instruction word and AND it with the keep-mask.
def mask_word(w):
    """Return w with PC-relative / literal immediate fields zeroed, if this is such a form."""
    top6  = (w >> 26) & 0x3f
    top8  = (w >> 24) & 0xff
    top7  = (w >> 25) & 0x7f
    # B (000101 imm26) / BL (100101 imm26)
    if top6 == 0b000101 or top6 == 0b100101:
        return w & ~0x03ffffff                      # zero imm26
    # ADR (0 immlo 10000 immhi) / ADRP (1 immlo 10000 immhi): bit31=op, bits[30:29]=immlo, [23:5]=immhi
    if ((w >> 24) & 0x1f) == 0b10000:
        return w & ~((0x3 << 29) | (0x7ffff << 5))  # zero immlo(30:29)+immhi(23:5)
    # B.cond (0101010 0 imm19 cond) : bits[31:24]=0x54, bit4=0
    if top8 == 0x54 and ((w >> 4) & 1) == 0:
        return w & ~(0x7ffff << 5)                  # zero imm19
    # CBZ/CBNZ (0110101 0/1 ...): sf 011010 op imm19 Rt -> match [30:25]=011010
    if ((w >> 25) & 0x3f) == 0b011010:
        return w & ~(0x7ffff << 5)                  # zero imm19
    # TBZ/TBNZ ([30:25]=011011)
    if ((w >> 25) & 0x3f) == 0b011011:
        return w & ~(0x3fff << 5)                   # zero imm14
    # LDR (literal): [29:24] = 011000 (32b) / 011001? forms; opc[31:30], 011 V 00 imm19 Rt
    #   32-bit LDR-lit=0x18, 64-bit=0x58, others; match [29:27]=011 and [25:24]=00 with bit26=V
    if ((w >> 27) & 0x7) == 0b011 and ((w >> 24) & 0x3) == 0b00 and ((w >> 31) & 1) == 0:
        # this also catches some non-literal forms; only mask when bits[26]=V-flag pattern of LDR-lit.
        # LDR-lit opc in [31:30]: 00,01,10; be conservative and mask imm19.
        return w & ~(0x7ffff << 5)
    return w

# AArch64 relocation types whose instruction carries a low-12-bit (page-offset)
# immediate that the linker fills in — form-based masking can't see these because
# the opcode is a plain ADD/LDR/STR. imm12 (bits [21:10]) must be zeroed wherever our
# object has one of these relocations, on both streams. (ADRP-HI21 / CALL26 / JUMP26
# are already covered by the form-based mask_word above.)
LO12_RELOCS = {
    277,  # R_AARCH64_ADD_ABS_LO12_NC
    278,  # R_AARCH64_LDST8_ABS_LO12_NC
    284,  # R_AARCH64_LDST16_ABS_LO12_NC
    285,  # R_AARCH64_LDST32_ABS_LO12_NC
    286,  # R_AARCH64_LDST64_ABS_LO12_NC
    299,  # R_AARCH64_LDST128_ABS_LO12_NC
    312,  # R_AARCH64_LD64_GOT_LO12_NC  (GOT-indirect ldr)
    313,  # R_AARCH64_TLSDESC_LD64_LO12 / low12 variants
}

# ---- Cortex-A53 erratum-843419 veneers -----------------------------------------
# The engine is linked with --fix-cortex-a53-843419 (on by default in the NDK's linker). Where an
# ADRP lands at page offset 0xff8/0xffc and one of the next three instructions writes its
# destination register, the linker moves that one instruction out to a fixup stub past the end of
# the function and leaves a `b` to the stub in its place; the stub runs the moved instruction and
# branches straight back. That is a link-time rewrite of correct object code — exactly the class
# of transform this comparator already normalizes for relocations — so splice the moved
# instruction back in before comparing.
#
# The `<insn>; b back-to-source+4` stub shape alone is not sufficient: ordinary block placement
# reproduces it (a rotated loop's `cbnz` tail, a `mov w8,wzr` tail block), and the naive rule
# would splice those and corrupt the stream. Two extra conditions make it exact: the branch must
# sit at a page offset the erratum can reach (0xffc/0x000/0x004/0x008 — the ADRP is at 0xff8 or
# 0xffc and the patched instruction is within three of it), and the moved instruction must not
# itself be a branch (the erratum only ever moves a load/store).
_VENEER_PAGE_OFFSETS = (0xffc, 0x000, 0x004, 0x008)

def _is_branch(w):
    """True for any AArch64 branch/return: B, BL, B.cond, CBZ/CBNZ, TBZ/TBNZ, BR/BLR/RET."""
    return ((w >> 26) & 0x1f) == 0b00101 \
        or (w & 0xff000010) == 0x54000000 \
        or (w & 0x7e000000) == 0x34000000 \
        or (w & 0x7e000000) == 0x36000000 \
        or (w & 0xfe000000) == 0xd6000000

def unveneer(code, addr, text_lo, text_hi, text_data):
    """Undo linker erratum-843419 fixups in the ORIGINAL byte stream."""
    out = None
    n = len(code) & ~3
    for i in range(0, n, 4):
        va = addr + i
        if (va & 0xfff) not in _VENEER_PAGE_OFFSETS:
            continue
        w = struct.unpack_from("<I", code, i)[0]
        if (w >> 26) != 0b000101:          # unconditional B
            continue
        imm = w & 0x3ffffff
        if imm & (1 << 25):
            imm -= (1 << 26)
        tgt = va + imm * 4
        if addr <= tgt < addr + n:         # intra-function branch: never a veneer
            continue
        if not (text_lo <= tgt < text_hi - 8):
            continue
        moved, back = struct.unpack_from("<II", text_data, tgt - text_lo)
        if _is_branch(moved) or (back >> 26) != 0b000101:
            continue
        bimm = back & 0x3ffffff
        if bimm & (1 << 25):
            bimm -= (1 << 26)
        if tgt + 4 + bimm * 4 != va + 4:
            continue
        if out is None:
            out = bytearray(code)
        struct.pack_into("<I", out, i, moved)
    return bytes(out) if out is not None else code

def mask_bytes(code, addr, relocs=None):
    """Zero PC-relative / relocated immediate fields. `relocs` is a list of
    (func_relative_offset, r_type) from OUR object; the same offsets are masked on
    the linked original, since matched functions are instruction-aligned."""
    out = bytearray(code)
    for i in range(0, len(code) & ~3, 4):
        w = struct.unpack_from("<I", code, i)[0]
        struct.pack_into("<I", out, i, mask_word(w))
    if relocs:
        for off, rtype in relocs:
            if rtype in LO12_RELOCS and 0 <= off <= len(out) - 4:
                w = struct.unpack_from("<I", out, off)[0]
                struct.pack_into("<I", out, off, w & ~(0xfff << 10))  # zero imm12 [21:10]
    return bytes(out)

# ---- ELF helpers ---------------------------------------------------------------
# Engine-scope filter: functions we decidedly will not decompile — third-party libraries and Aspyr
# port shims statically linked into the .so. This tool itself is unaffected (it scores by marker,
# not by scope).
#   Plain-C symbols (no _Z): the engine is C++, so every plain-C defined FUNC is port
#     scaffolding (Win32/MSVC-CRT shims, libc wrappers, SDL_image, Bink, libiconv, FreeType,
#     GL shims, dlmalloc, compiler-rt) — the only in-scope plain-C names are the Aspyr JNI
#     entry points (Java_com_aspyr*), which are port code we might plausibly match someday.
#   Mangled: libc++ (std::__ndk1/_ZSt/__gnu_cxx), shim/third-party namespaces (ASL, Json,
#     TiXml, FMod, OpenGLES, GLSLTranslator, ARBTranslator, NvStrip, FT*/FTGL, Mac*),
#     ASL free functions, D3D-on-GL compat (any name containing D3D / IDirect3D, incl. _ZThn
#     thunks), Win32/Mac shim free functions (_Z<len>Mac/Win/DOS2Mac*), *_Mac shim classes.
#
# GLRender is not a shim: src/libsource/AURORA/GLRender/GLRender.h is the engine's own GL renderer
# (its body comes from the port's aurglcore.cpp), it is included by engine TUs (aurora.cpp,
# aurtexman.cpp, aurmodelrender.cpp, CAurInternal.cpp, CAurGUIStringInternal.cpp), and it operates
# on engine types throughout (CAurTexture, MdlNodeTriMesh, Aurora::AuroraBufferRequestType,
# List<MaxFace>, RenderVertex*). GLSLTranslator and ARBTranslator stay out: they take ASL types
# (`ASLDisplay_t`) and sit in a separate address band with no src-side caller.
_SCOPE_SHIM_NS = re.compile(
    # `FMOD`, not `FMod`: the third-party API namespace is all caps (`_ZN4FMOD...`). A
    # case-sensitive `FMod` alternative would never match it, and would only have caught the
    # engine's own `_ZN15FModAudioSystem*` backend class, keeping that whole TU out of scope.
    r'_ZNK?\d+(ASL|Json|TiXml|FMOD|OpenGLES|GLSLTranslator|ARBTranslator|NvStrip|FT|Mac)')
# `Mac(?!ro)`: `Mac` would also match the console macro recorder's free functions
# (`_Z11MacroRecordPc` and friends), which are engine code. No ASL/Mac shim free function starts
# `Macro`.
_SCOPE_SHIM_FREE = re.compile(r'_Z\d+(ASL|Mac(?!ro)|Win|DOS2Mac)')

def fde_starts(elf):
    """Sorted function start addresses from `.eh_frame_hdr`'s binary-search FDE table.

    The function-boundary oracle for internal-linkage rows: an anonymous-namespace or
    file-`static` function has no `.dynsym` row (the .so ships no `.symtab`), so nothing else can
    name or size it. Every function with unwind info appears here, which under `-fexceptions` is
    all of them.

    The reader is `.eh_frame_hdr`'s own: a 4-byte header (version / eh_frame_ptr encoding /
    count encoding / table encoding), then `count` entries of `(4-byte sdata relative to this
    table, 4-byte fde address)` — the FDE addresses are not needed, only the starts.
    """
    hdr = elf.get_section_by_name('.eh_frame_hdr')
    if hdr is None:
        return []
    D = hdr.data()
    va = hdr['sh_addr']
    if len(D) < 12:
        return []

    def rd(enc, p):
        f = enc & 0x0f
        if f == 0x0b:
            v, = struct.unpack_from('<i', D, p); n = 4
        elif f == 0x03:
            v, = struct.unpack_from('<I', D, p); n = 4
        elif f == 0x04:
            v, = struct.unpack_from('<Q', D, p); n = 8
        elif f == 0x0c:
            v, = struct.unpack_from('<q', D, p); n = 8
        else:
            raise ValueError(hex(enc))
        if enc & 0x70 == 0x10:
            v += va
        return v, p + n

    p = 4
    _, p = rd(D[1], p)
    cnt, p = rd(D[2], p)
    out = []
    for _ in range(cnt):
        a, = struct.unpack_from('<i', D, p)
        out.append(va + a)
        p += 8
    out.sort()
    return out


def in_engine_scope(name):
    """True if a .so dynsym FUNC belongs to the decompilable engine (see rules above)."""
    if not name.startswith('_Z'):
        return name.startswith('Java_com_aspyr')
    if name.startswith(('_ZNSt6__ndk1', '_ZNKSt6__ndk1', '_ZSt',
                        '_ZN9__gnu_cxx', '_ZNK9__gnu_cxx')):
        return False
    if _SCOPE_SHIM_NS.match(name) or _SCOPE_SHIM_FREE.match(name):
        return False
    # The 'D3D' substring is a coarse net for the whole D3D9-compat layer (D3DShader_*, D3DUtils,
    # HLSLUtils, ASLFX*, ID3DX*_Mac, _D3DFORMAT/_D3DLOCKED_RECT-parameterised ASL functions,
    # MacGetD3DAdapter*) — deliberately coarse, because tightening it to a token boundary lets a
    # large slice of that shim back into scope. It has exactly one false positive: CExoSound's
    # 2D/3D-positional-audio probe (`Set2D3DBias`/`Get2D3DBias`/`Test2D3DBias`) contains the
    # literal substring "D3D".
    if ('D3D' in name and '2D3DBias' not in name) or 'IDirect3D' in name:
        return False
    # `_Mac` drops the whole Mac port-shim layer (HANDLE_Mac, MacFiles, ASLScopeLock, MSG_Mac, ...),
    # but it is a substring test and one in-scope class has a `_Mac`-named parameter type:
    # `CWin32MSGQueue::AdjustStatus(MSG_Mac_t const&, bool)` mangles to `...RK9MSG_Mac_tb`, so the
    # type name alone would reject it while all seven of its siblings in the same class count. This
    # carve-out is worth exactly one row, and nothing else.
    if '_Mac' in name and not name.startswith('_ZN14CWin32MSGQueue'):
        return False
    return True

def fde_only_functions(so_path, om):
    """addr -> size for FUNC starts that `.eh_frame_hdr` knows and the dynsym does not.

    These are the internal-linkage rows (`static` / anonymous-namespace functions). They are
    deliberately not folded into `orig_functions`' map: that map is the engine denominator, and
    widening it would pull in every statically-linked third-party function the FDE table also
    covers — the dynsym is what makes `in_engine_scope` decidable at all. They are tallied on
    their own line instead, like the out-of-scope rows.
    """
    f = open(so_path, "rb"); elf = ELFFile(f)
    text = elf.get_section_by_name(".text")
    lo, hi = text["sh_addr"], text["sh_addr"] + text["sh_size"]
    starts = [a for a in fde_starts(elf) if lo <= a < hi]
    out = {}
    for i, a in enumerate(starts):
        if a in om:
            continue
        end = starts[i + 1] if i + 1 < len(starts) else hi
        if end > a:
            out[a] = end - a
    return out


def orig_functions(so_path):
    """addr -> (name, size) from the target .so dynsym (defined FUNCs)."""
    f = open(so_path, "rb"); elf = ELFFile(f)
    text = elf.get_section_by_name(".text")
    tbase, tdata = text["sh_addr"], text.data()
    m = {}
    dyn = elf.get_section_by_name(".dynsym")
    for s in dyn.iter_symbols():
        if s["st_info"]["type"] != "STT_FUNC": continue
        if s["st_size"] == 0 or s["st_shndx"] == "SHN_UNDEF": continue
        m[s["st_value"]] = (s.name, s["st_size"])
    thi = tbase + len(tdata)
    def getbytes(addr, size):
        off = addr - tbase
        return unveneer(tdata[off:off+size], addr, tbase, thi, tdata)
    return m, getbytes

def obj_functions(obj_path):
    """mangled name -> (bytes, relocs) from our compiled object (.text.<name> sections
    via symtab). relocs is a list of (func_relative_offset, r_type) for that function."""
    from elftools.elf.relocation import RelocationSection
    f = open(obj_path, "rb"); elf = ELFFile(f)
    symtab = elf.get_section_by_name(".symtab")
    # section index -> list of (section_relative_offset, r_type)
    rela_by_sec = {}
    for sec in elf.iter_sections():
        if isinstance(sec, RelocationSection):
            tgt = sec["sh_info"]
            lst = rela_by_sec.setdefault(tgt, [])
            for r in sec.iter_relocations():
                lst.append((r["r_offset"], r["r_info_type"]))
    out = {}
    for s in symtab.iter_symbols():
        if s["st_info"]["type"] != "STT_FUNC" or s["st_size"] == 0: continue
        shndx = s["st_shndx"]
        if not isinstance(shndx, int): continue
        sec = elf.get_section(shndx)
        data = sec.data()
        off = s["st_value"]           # offset within its own section
        fn_relocs = [(ro - off, rt) for (ro, rt) in rela_by_sec.get(shndx, [])
                     if off <= ro < off + s["st_size"]]
        out[s.name] = (bytes(data[off:off+s["st_size"]]), fn_relocs)
    return out

# The marker table (tools/fnindex.py): MARKER answers `MARKER.findall(open(src).read())` with
# (address, mangled name) pairs, so the address comes from the original .so rather than from a
# comment in our source. MARKER_BOTH is the K1-aware shape the port tools use.
from fnindex import MARKER, MARKER_BOTH   # noqa: F401  (re-exported for the K1 port tools)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--so", default=TARGET_SO)
    ap.add_argument("--obj", default=None)
    ap.add_argument("--dump", action="store_true")
    a = ap.parse_args()

    tu = _tu_name(a.src)
    obj = a.obj or f"build/{tu}.o"
    markers = MARKER.findall(open(a.src).read())
    if not markers:
        print("no // FUNCTION: KOTOR2 markers found in", a.src); return 1

    orig_map, orig_bytes = orig_functions(a.so)
    ours = obj_functions(obj)

    # Structural lint debt (classes A-F: unmodeled fields / duplicate classes / raw vtable
    # dispatch) means the TU's struct model is still wrong somewhere, so per-function byte
    # distances are noise that invites grinding the wrong thing — suppress them (MATCH/DIFF
    # status still prints) until the model debt is cleared.
    lint_rows = _lint_rows(a.src)
    suppress = any(set(r[2]) & SUPPRESS_CLASSES for r in lint_rows)
    if suppress:
        print("  ⚠ distances suppressed: this TU carries A/B/C/D/E/F lint findings (an"
              " incomplete struct/vtable model) — clear them to re-enable DIFF counts,"
              " LENGTH deltas and --dump.")

    # A stripped file-local static has no .dynsym entry (and the .so carries no .symtab), so it
    # cannot be looked up by address. Its exact size comes from the original's own .eh_frame_hdr FDE
    # table, which is a complete function-boundary oracle: every function with unwind info appears
    # there, which under -fexceptions is all of them. Synthesizing addr..next dynsym FUNC instead is
    # meaningless for the tightly packed internal-linkage rows (a family of anonymous-namespace
    # helpers can have no dynsym row and no marker, so its first row's synthesized span would run
    # far past its real end). The dynsym synthesis remains as the fallback for an address that is
    # not an FDE start.
    text = a.so and None  # placeholder: filled below
    _so_file = open(a.so, "rb"); _so_elf = ELFFile(_so_file)
    _text = _so_elf.get_section_by_name(".text")
    _text_lo, _text_hi = _text["sh_addr"], _text["sh_addr"] + _text["sh_size"]
    _text_data = _text.data()
    _fdes = fde_starts(_so_elf)
    npass = 0
    for addr_s, mangled in markers:
        addr = int(addr_s, 16)
        if addr not in orig_map:
            osize = None
            fde = False
            if _fdes:
                i = bisect.bisect_left(_fdes, addr)
                if i < len(_fdes) and _fdes[i] == addr:
                    osize = (_fdes[i + 1] if i + 1 < len(_fdes) else _text_hi) - addr
                    fde = True
            if osize is None and _text_lo <= addr < _text_hi:
                nxt = min((k for k in orig_map if k > addr), default=_text_hi)
                osize = nxt - addr
            if osize is None:
                print(f"  ???  0x{addr:x} {mangled}: not a defined FUNC in the .so"); continue
            orig_map[addr] = (mangled + (" [fde]" if fde else " [synthesized]"), osize)
            if fde:
                print(f"  ! 0x{addr:x} {mangled}: not in dynsym — FDE size {osize}")
            else:
                print(f"  ! 0x{addr:x} {mangled}: not in dynsym — synthesized size {osize}"
                      " (no FDE start here either)")
        oname, osize = orig_map[addr]
        ob = orig_bytes(addr, osize)
        if mangled not in ours:
            print(f"  MISS  0x{addr:x} {mangled}: not emitted in {obj} (name mismatch?)"); continue
        mb, mrel = ours[mangled]
        oM, mM = mask_bytes(ob, addr, mrel), mask_bytes(mb, 0, mrel)
        if len(oM) != len(mM):
            if suppress:
                print(f"  DIFF  0x{addr:x} {mangled}: LENGTH MISMATCH")
            else:
                print(f"  DIFF  0x{addr:x} {mangled}: LENGTH {len(mM)} vs orig {len(oM)}")
                if a.dump: _dump(ob, mb, addr, mrel)
            continue
        ndiff = sum(1 for x, y in zip(oM, mM) if x != y)
        if ndiff == 0:
            print(f"  MATCH 0x{addr:x} {mangled}  ({osize}B)"); npass += 1
        elif suppress:
            print(f"  DIFF(?) 0x{addr:x} {mangled}")
        else:
            print(f"  DIFF({ndiff}) 0x{addr:x} {mangled}  ({osize}B)")
            if a.dump: _dump(ob, mb, addr, mrel)
    print(f"--- {npass}/{len(markers)} match ---")
    nbad = _lint(a.src, rows=lint_rows)
    return 0 if (npass == len(markers) and nbad == 0) else 1

# Lint classes that gate distance output (see main): every one is a "the model is still
# wrong" class — A/B raw offsets, C Unk-ptr arithmetic, D _pad writes, E duplicate class
# definitions, F raw vtable dispatch. G/I/J/K are style/size-arg debt and H is the ungated
# m_Unk tally; none of those invalidate distances.
SUPPRESS_CLASSES = set("ABCDEF")

def _lint_rows(src):
    """Collect the TU's lint findings (same view _lint prints): the TU .cpp + sibling
    headers, plus this dir's slice of the engine-wide class-E duplicate scan, minus the
    ungated class-H tally. Returns rows of (path, lineno, classes, raw, tagged)."""
    import os, pathlib, importlib.util
    spec = importlib.util.spec_from_file_location(
        "lint_idiom", pathlib.Path(__file__).parent / "lint_idiom.py")
    li = importlib.util.module_from_spec(spec); spec.loader.exec_module(li)
    p = pathlib.Path(src)
    rows = []  # (path, lineno, classes, raw, tagged)
    homes = li.home_dirs(p)   # the class dir(s), also for a .cpp the path migration moved/merged
    for f in [p] + sorted(pathlib.Path(os.path.relpath(h)) for d in homes for h in d.glob("*.h")):
        rows += [(f,) + r for r in li.scan_file(f)]
    dup_findings, _ = li.collect_dups()  # class E needs the engine-wide view
    for f, drows in dup_findings.items():
        if li.home_dir(f) in homes:
            rows += [(f, lineno, ["E"], raw, tagged)
                     for lineno, raw, tagged in drows]
    # class H (m_Unk placeholder-field definitions) is an ungated, whole-engine
    # tally — not a per-TU gate concern; drop it here so it neither floods the
    # display nor blocks the run (see lint_idiom's H notes).
    return [r for r in rows if set(r[2]) != {"H"}]

def _lint(src, cap=5, rows=None):
    """Idiom lint on the TU (+ sibling headers): raw offset access = an
    unmodeled struct field. Prints at most `cap` findings; returns the number
    of UNTAGGED findings (new debt — fails the run)."""
    if rows is None:
        rows = _lint_rows(src)
    import pathlib
    p = pathlib.Path(src)
    if not rows:
        return 0
    untagged = [r for r in rows if not r[4]]
    print(f"  ⚠ NON-IDIOMATIC C++: {len(rows)} findings "
          f"({len(untagged)} untagged) — raw `*(T*)((char*)p + 0xNN)` = an"
          " unmodeled struct field (model it; codegen is identical); [E] ="
          " duplicate class definition (extend the canonical model, don't"
          " re-declare a local copy — sizeof-drift hazard); [F] = raw"
          " vtable-slot call `(*(VFn**)p)[N](p)` = an unmodeled virtual"
          " method (slot 1/deleting-dtor excepted).")
    shown = (untagged + [r for r in rows if r[4]])[:cap]
    for f, lineno, classes, raw, tagged in shown:
        mark = "known" if tagged else "NEW"
        print(f"      {f}:{lineno}: [{','.join(classes)},{mark}] {raw.strip()[:90]}")
    if len(rows) > cap:
        print(f"      … and {len(rows) - cap} more:"
              f" tools/lint_idiom.py {p}")
    if untagged:
        print("      ➜ untagged findings FAIL the run: model the struct field"
              " now, or tag known debt `// TODO: idiom` /"
              " a reviewed deliberate keep `// idiom-exempt`.")
    return len(untagged)

def _dump(ob, mb, addr=None, rel=None):
    """Side-by-side raw disassembly. `<<` = a real difference (survives masking, i.e. it is part of
    DIFF(n)); `~` = the raw words differ but the masked words agree, so the scorer discards it (a
    relocation / PC-relative display artifact, not a finding)."""
    from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN
    md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
    o = list(md.disasm(ob, 0)); m = list(md.disasm(mb, 0))
    n = max(len(o), len(m))
    if addr is not None:
        om, mm = mask_bytes(ob, addr, rel), mask_bytes(mb, 0, rel)
    else:
        om = mm = None
    print("      orig".ljust(40), "| ours")
    for i in range(n):
        L = f"{o[i].mnemonic} {o[i].op_str}" if i < len(o) else ""
        R = f"{m[i].mnemonic} {m[i].op_str}" if i < len(m) else ""
        if L == R:
            flag = ""
        elif om is not None and om[i*4:i*4+4] == mm[i*4:i*4+4]:
            flag = "  ~"                      # masked away: not scored
        else:
            flag = "  <<"
        print(f"      {L:34s} | {R}{flag}")

if __name__ == "__main__":
    sys.exit(main())
