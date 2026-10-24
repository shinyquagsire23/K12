#!/usr/bin/env python3
"""lint_idiom.py — find non-idiomatic C++ (raw offset math) in src/.

The rule: matched C++ must be real, idiomatic source — `z->width`, not
`*(short*)((char*)z + 0xc)`. Raw offset access means a struct field is still
unmodeled. This tool catalogs the debt and gates new code (untagged findings =
failure).

Pattern classes:
  A  char-cast offset arithmetic:      (char*)p + 0x58   /  (char*)p + 8
     — including the scaled form:      (char*)p + idx * 0x18
     (an index expression in front of the magic number is the same raw
     byte-offset math, just better hidden)
     — and every other spelling of a char VIEW stepped by a literal:
         reinterpret_cast<char*>(p) + 0x58     static_cast<char*>(p) + 10
         (const char*)this + 1                 (uint8_t*)p + 4  /  BYTE*
         ((unsigned char*)p)[0xa8]             reinterpret_cast<char*>(p)[8]
       (a char subscript of an already-named field's own bytes,
       `((unsigned char*)&m_field)[1]`, is not flagged)
  B  deref of raw pointer + offset:    *(int*)(p + 0x438)
                                       *reinterpret_cast<int*>(p + 0x438)
  C  offset math on Unk pointer field: p->m_pUnk1798 + 0xa8
  D  write/read through a _pad name:   pFile->_pad40 = 0
  E  duplicate class definition: a C-prefixed class DEFINED (with body) in a
     file outside its home TU dir (src/<Class>/), when >=2 definitions exist
     engine-wide. These per-TU "local shortcut models" are the sizeof-drift
     regression hazard; consolidate toward one model per class. A deliberate
     site in a sibling's header (helper classes) gets
     '// idiom-exempt: canonical'.
     ALSO E: any other class/struct/union defined at namespace scope in 2+ FILES
     (the Win32/SDL/GL records, ASLgl, AABB, PointerMap ... are per-TU copies);
     its single header definition is the one to keep, every .cpp copy is a
     finding. A file's `#if KOTOR1`/`#else` arms count as one definition, and a
     type inside an anonymous namespace (a distinct type per TU) is not
     reported.
  F  raw vtable-slot dispatch:          (*(VFn13**)pObj)[13](pObj)
     = an unmodeled VIRTUAL METHOD (engine classes: model the virtual; COM-ish
     external interfaces (DirectInput): model the interface struct). Slot [1]
     is not flagged — that's the established deleting-dtor teardown idiom
     (keep it raw; do not declare the dtor virtual).
     ALSO F: any raw cast of `this` to a NON-C-prefixed type:
                                       ((VtblProbeT*)this)->Method(...)
     — the "probe struct" evasion of the slot dispatch above: the slot math is
     just hidden inside the probe type's layout instead of written at the call
     site. Same debt, same fix: declare the virtual on the class's real vtable
     (in its header) and call it by name. (A cast of `this` to a C-prefixed
     engine class is the class-K cast-this-to-base smell instead — reported
     there, not here.) A reviewed, deliberately-kept raw cast of `this` can be
     silenced with `// idiom-exempt: <reason>` (F is not an offset class, so
     the tag is honored).
  G  raw-dispatch / stray function-pointer typedef, THREE sub-shapes:
     (1) function-local typedef in a .cpp (an indented `typedef` inside a
         function body — hoist it to the TU header; compile-time only).
     (2) a fn-ptr typedef at any scope (.h or .cpp) whose name services a
         non-slot-1 raw vtable dispatch anywhere in the same TU directory —
         hoisting a dispatch typedef to the header does not retire it; the
         only real fix is the class-F fix (model the virtual), which retires
         typedef + dispatch together.
     (3) a DEAD fn-ptr typedef — its name appears nowhere else in the TU dir
         (a leftover from an already-rewritten dispatch); just delete it.
     A typedef whose only dispatch uses are the exempt slot-1 deleting-dtor
     idiom is NOT flagged — one file-scope `typedef void (*VFn)(void*);` per
     TU serving the teardown idiom is the sanctioned end state.
  H  m_Unk* field DEFINITION:            int m_Unk08;  /  char* m_pUnk1798;
     A placeholder-named struct member (known offset, unknown purpose). LOWEST
     priority and reported SEPARATELY: it compiles perfectly and is harmless —
     just a naming-debt marker, driven down as we discover how modules interlink
     over time. NOT part of the untagged commit-gate (never blocks a commit) and
     NOT counted in the A-G "idiom debt" headline; it has its own tally. Only
     DECLARATIONS are counted (a `->m_Unk` use / `= m_Unk` read is not H).
  I  memcpy/memset/memclr/memmove whose size argument has no `sizeof`/
     `offsetof`: memset(p, 0, 0x100) — HIGH PRIORITY. A bare byte-count magic
     number is exactly as fragile as a raw struct offset (silently wrong if
     the real struct/array resizes); prefer `sizeof(*p)`/`sizeof(SomeType)`,
     or `offsetof(T, fieldAfter) - offsetof(T, fieldStart)` when the region
     spans several fields up to (but not including) a named boundary field.
     Declarations (`void* memcpy(void*, const void*, unsigned long);`) are
     not flagged. A genuinely runtime-computed length (e.g. `strlen`-derived,
     or read from file/buffer data at runtime) has no compile-time size to
     name — those get `// idiom-exempt: runtime length`.
  J  sizeof(char) / sizeof(unsigned char) / sizeof(signed char) — HIGH PRIORITY.
     Always exactly 1 by the standard; spelling it out is meaningless noise
     (and often a copy-paste leftover from a different type). Just write `1`.
  K  cast-this-to-base:                  ((CSWSObject*)this)->GetArea()
     A cast of `this` to another C-prefixed class pointer — the symptom that
     real C++ inheritance isn't modeled. HIGH PRIORITY. Fires on both the
     method-call form `((CSWSObject*)this)->GetArea()` and the field form
     `((CSWSObject*)this)->m_field`. The fix is to model the actual
     inheritance — `class CDerived : public CBase { ... }` with the base's
     members + (often virtual) methods on a shared base header — so the derived
     method calls the inherited method / reads the inherited field directly with
     no cast. Verify byte-neutrality with a whole-engine build comparison
     (changing a base layout / vptr can shift offsets).
  L  hand-freed local list buffer:       delete[] vecFrames.m_pData;
     A `delete[]` / `operator delete[]` of a non-member (`x.`, not `m_x.`)
     List/CExoArrayList's m_pData/m_pArray. Both containers free their buffer
     in their own destructor, so unless the line after nulls the pointer this
     is a double free at scope exit. HIGH PRIORITY — a correctness finding,
     not style. The fix is to delete the line and let the destructor run
     (scope the local so the buffer is freed where it should be). A reviewed
     site that nulls the pointer right after gets `// idiom-exempt: nulled`.
  M  empty inline-asm barrier:          asm volatile("" : "+r"(idx));
     An empty asm statement (any operand/clobber list) steers the optimiser and
     emits nothing; no original source contained one. It is a byte lever, not a
     transcription, and it can hide a wrong program behind a matched length.
     Only the empty-string form is flagged; `asm("symbol")` label renames are
     not. A barrier proven load-bearing on a matching row (delete it,
     recompile, the row stops matching) gets `// idiom-exempt: <the row it
     holds>`; anything else is `// TODO: idiom` debt or should simply be
     deleted.
  N  scalar-typed view stepped by a literal:  ((int*)m_pUnk0x1c8)[2]
                                       ((short*)&face)[13]   (float*)p + 3
                                       reinterpret_cast<unsigned int*>(p) + 2
     The same raw member offset as class A with the stride hidden in the
     pointee type ([2] of an int* = +8). Only scalar pointees (int/short/
     float/bool/u?intN_t/DWORD/pointer-to-pointer) — a struct-typed step
     (`(Header4PC*)m_pFileData + 1`, `((Vector*)p)[1]`) is typed array or
     header-skip access and is not flagged; index 0 is never flagged. Unlike
     A it is exempt-able, because a genuinely raw byte buffer (a file record,
     a hash digest, a GFF label buffer) has no struct to name:
     `// idiom-exempt: <the buffer and why it stays raw>`.
  (F also covers the pointer-step vtable call `(*(void***)p + 43)(p)`, which
   VTBL_RX's `[43]`-subscript form did not see.)
  W  a bare `long` / `unsigned long`:  unsigned long m_nStrRef;   (unsigned long)pBuf
     The binary is LP64, so its `long` is 64 bits; a Win64 build is LLP64, where
     `long` is 32, and two separately declared sides (the engine's struct, the
     shim that fills it) then disagree with no diagnostic. Spell the width:
     int64_t / uint64_t (src/include/k2stdint.h) are `long` / `unsigned long`
     on the Android build, so the bytes and the mangled names do not change; a
     pointer as an integer is uintptr_t / intptr_t. `long long`, `long double`
     and a line naming a C library function whose own signature has `long`
     (strtol, ftell, fseek, clock ...: platforms/tools/longscan.py LIBC_LONG)
     are not flagged. `platforms/tools/longscan.py --rewrite <file>` fixes a
     file in place.
  Q  a C-style cast:                    (CSWSObject*)pObj   (int)fValue   (char*)"text"
     The engine is written with named casts (tools/castfix.py rewrites every
     C-style cast as the conversion clang resolves it to), so `reinterpret_cast`
     greps out every reinterpretation and `const_cast` every dropped const.
     Write static_cast / reinterpret_cast / const_cast; `(void)x` (discarding a
     value) is not a cast and is not flagged. A cast no single named cast spells
     on every target (pthread_t -> int64_t: an integer on Android, a pointer on
     macOS) gets `// idiom-exempt: <why>`. Classes K and F also see a named
     cast as the C-style one it replaced (`uncast` below), so
     `static_cast<CSWSObject*>(this)->` is still class K (A/B/N have their own
     named-cast patterns). Q alone also covers platforms/'s own C++ (.cpp / .mm,
     and headers no C file includes and with no `__cplusplus` guard);
     third_party/ and the generated *.gen.* files (fix their generators) are
     not ours to lint. `castfix.py --platforms` rewrites them.
  L  an asm label on a declaration:     void f(int) __asm__("_Z1gi");
     Binds a C++ name to a hand-written symbol name, so the code only links
     against one ABI's mangling (the Itanium C++ ABI names of the shipping
     binary) and the declaration's real type is never checked against the
     definition. Declare
     the real entity instead: the member on its class (include its header), a
     function-local static inside the inline function that owns it, a plain
     forwarding function for a renamed import. Never exempt-able. Inline asm
     statements (`asm volatile("" ...)`) are not labels and are not flagged.
  P  a C library prototype outside src/port/libc*.h:
                                       extern "C" size_t strlen(const char* s) throw();
     Every libc function the engine calls is declared once, in port/libcbase.h,
     libcstring.h, libcstdlib.h or libcmath.h (which name and which exception
     specification goes where: libcbase.h's top comment). A per-TU copy is a
     second declaration whose return width or throw() can drift from the
     header's, and differing exception specifications across TUs are
     ill-formed. Include the header instead; a TU that must add an attribute
     for one shape (pure, noduplicate) repeats the header's signature exactly
     and says why with `// idiom-exempt: <reason>`. The name set is read from
     the headers themselves.

WHY A/B/N MATTER BEYOND STYLE: a pun through a hand-typed offset compiles to the
same load as the named member, so a byte-matched row proves nothing about it —
but the offset is unchecked by the type system: it silently reads a different
member (or past the modeled object) if the layout or the constant is wrong, and
it is an aliasing violation the optimiser may exploit in the host, ASan and
KOTOR 1 builds. `aurskin.cpp`'s `*reinterpret_cast<int*>(reinterpret_cast<char*>(
m_pMaterial) + 0x58)` is `m_pMaterial->m_lstUnk50.m_count`.

Severity: A-E, I-L, N > F-G, M > H. A-D (raw offsets), E (duplicate models), and I-J
(memcpy/memset sizing, sizeof(char)) are non-local hazards or correctness
smells — a wrong hand-computed offset/size or a drifted sizeof silently
regresses other functions and TUs, or masks a wrong-type bug. F-G
are self-contained: the slot index / typedef is local to the call site and
that site's own byte-match verifies it, so they're idiom debt but not a
correctness risk. Clean up A-E and I-J first. H is a separate, ungated
placeholder-naming tally — lowest priority of all.

Not flagged (established idioms): (char*)"literal" casts, the vtable-slot-1
deleting-dtor dispatch, *(const float*)&x bit-reinterprets, new char[] buffers.

Tags:
  // TODO: idiom   -- known debt (counted separately; the cleanup backlog)
  // idiom-exempt  — reviewed, deliberately kept (skipped entirely; add a reason)

A raw hex/decimal offset (classes A/B/C) is never exempt-able. `+ 0x30`-
style pointer arithmetic always means a struct field is unmodeled — there is
no legitimate reason to leave it as magic-number pointer math permanently.
`// idiom-exempt` on a line matching A/B/C is ignored (the finding still
fires, and still needs a real fix or at worst a `// TODO: idiom` backlog tag).
The fix is always a real struct field access — `this->m_member` — never a
literal `+ 0x30`. A cast through a base `((OtherClass*)this)->m_member` is
itself a class-K cast-this-to-base finding: prefer modeling real inheritance
so the field is read directly. The cast form is acceptable only as a temporary
step off a worse class-A/B/C raw-offset finding, and still needs its own K
cleanup.
Two shapes, both zero arithmetic at the use site:
  - if the field is safe to model as a genuine typed class member, use
    `&this->m_Member` (ordinary member access), or
  - if it must stay untyped/raw (an inherited field the enclosing per-TU
    model doesn't own, or a real member kept raw to dodge an auto-ctor/dtor
    conflict), give it its own properly-sized, exactly-positioned named
    byte-array field (splitting a too-wide `_pad` array at the boundary if
    needed) and reference that field directly — `this->_rawTag`, not
    `(char*)this + 0x30`. Name it `_raw*`, not `_pad*` (the purpose is known;
    only the C++ type is deliberately unmodeled) so it doesn't itself become a
    class-D "_pad access" finding.
  `offsetof(...)` is reserved for one narrow case: naming the size argument of
  a class-I memcpy/memset/memclr/memmove call (see class I above) when the
  copied region starts at a named field but doesn't align with one single
  field's own address — e.g. `memset((char*)this + offsetof(T, _pad20), 0,
  sizeof(T) - offsetof(T, _pad20))`. It is not a general substitute for
  `&this->m_member` outside that context — if a plain named-field reference
  already gives the exact address you need (the common case), use that
  instead of reaching for `offsetof`. It especially must not be used to grow
  an embedded-by-value struct's modeled `sizeof` past what its own outer
  struct's layout can absorb; that case has no available fix yet and stays a
  plain `// TODO: idiom`.

Usage:
  tools/lint_idiom.py                      table per file + totals; exit 1 if UNTAGGED
  tools/lint_idiom.py src/game/servercore/CSWSStore  LIST that file/dir's findings (path => list mode)
  tools/lint_idiom.py --only EF [path...]  restrict to classes (e.g. E+F only)
  tools/lint_idiom.py --only ABN           every raw-offset spelling (char view, deref, scalar view)
  tools/lint_idiom.py --list               every finding, file:line: [classes] code
  tools/lint_idiom.py --doc                markdown catalog of every finding
  tools/lint_idiom.py --tag [path...]      append '// TODO: idiom' to untagged lines
  tools/lint_idiom.py --selftest           each fixture classifies alike in C-style and named-cast spelling
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"


def platform_files():
    """platforms/'s own C++ for class Q: not third_party/, not generated, not C, and not a header
    a C file includes or one written for C as well (an `#ifdef __cplusplus` guard)"""
    import subprocess
    if (ROOT / ".git").exists():
        out = subprocess.run(["git", "ls-files", "-co", "--exclude-standard", "platforms"], cwd=ROOT,
                             capture_output=True, text=True).stdout.split()
    else:   # a tools/stage_k12.py tree with no repository: its .gitignore keeps out/ and tombstones/
        out = sorted(str(p.relative_to(ROOT)) for p in (ROOT / "platforms").rglob("*") if p.is_file()
                     and p.relative_to(ROOT).parts[1] not in ("out", "tombstones"))
    c_included = {Path(m.group(1)).name for rel in out if rel.endswith(".c")
                  for m in re.finditer(r'#\s*include\s*[<"]([^>"]+)[>"]', (ROOT / rel).read_text(errors="replace"))}
    for rel in out:
        path = ROOT / rel
        if path.suffix not in (".cpp", ".mm", ".h", ".hpp") or "/third_party/" in rel or ".gen." in path.name:
            continue
        if path.suffix in (".h", ".hpp") and (path.name in c_included or
                                              "__cplusplus" in path.read_text(errors="replace")):
            continue
        yield path

NUM = r"(?:0x[0-9a-fA-F]+|\d+)"
# a base expression's tail: member access, calls and subscripts (`m_apPanels[1]`, `GetArea()`,
# `m_a[i].m_p`) -- one nesting level. A/B must not stop at the first `[`, or
# `(char*)m_apScreenPanels[1] + 0x2775` would match nothing.
BASE_TAIL = r"(?:[\w.>-]|\([^()]*\)|\[[^\[\]]*\])*"

# Classes whose match is a raw hex/decimal offset — this tool's policy:
# these can never be silenced by `// idiom-exempt`, only by an actual fix (or,
# failing that, `// TODO: idiom` backlog tagging).
OFFSET_CLASSES = {"A", "B", "C"}

NAMED_CAST_RX = re.compile(r"\b(?:static_cast|reinterpret_cast|const_cast)\s*<")


def _balanced(code, i, open_, close):
    """index just past the bracket closing the one at code[i] (len(code) if unbalanced)"""
    depth = 0
    for j in range(i, len(code)):
        if code[j] in open_:
            depth += 1
        elif code[j] in close:
            depth -= 1
            if depth == 0:
                return j + 1
    return len(code)


def uncast(code):
    """`static_cast<T>(x)` back to the C-style `(T)x` (`(T)(x)` when x is not a simple name or
    member chain), so the patterns written for C-style casts see the named spelling too"""
    out, i = [], 0
    for m in NAMED_CAST_RX.finditer(code):
        if m.start() < i:
            continue
        lt = m.end() - 1
        gt = _balanced(code, lt, "<(", ">)")
        k = gt
        while k < len(code) and code[k].isspace():
            k += 1
        if k >= len(code) or code[k] != "(":
            continue
        end = _balanced(code, k, "(", ")")
        inner = uncast(code[k + 1:end - 1])
        t = code[lt + 1:gt - 1].strip()
        simple = re.fullmatch(r"\s*&?[A-Za-z_][\w:]*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*", inner)
        out.append(code[i:m.start()])
        cast = "(%s)%s" % (t, inner.strip() if simple else "(%s)" % inner)
        # a postfix operator after the named cast applies to the CAST; the C-style spelling needs
        # the group `((T*)this)->f()` to say the same (without it the pattern sees `this->f()`)
        if re.match(r"\s*(?:->|\.(?!\.)|\[)", code[end:]):
            cast = "(%s)" % cast
        out.append(cast)
        i = end
    out.append(code[i:])
    return "".join(out)


# Q: a C-style cast. The cast type: a pointer / reference to anything, or an arithmetic type; the
# '(' is not a call's or a declaration's (no name or closing bracket before it), and the operand
# follows at once. `(void)x` is not a cast.
ARITH_T = (r"(?:(?:const|volatile|unsigned|signed|short|long)\s+)*(?:bool|char|short|int|long|float|double|"
           r"unsigned|signed|wchar_t|size_t|ssize_t|ptrdiff_t|u?intptr_t|u?int(?:8|16|32|64)_t|BYTE|WORD|DWORD|"
           r"UINT|INT|BOOL|ULONG|LONG|USHORT|UCHAR|CHAR|GLenum|GLint|GLuint|GLfloat|GLsizei|GLushort|GLubyte)")
CSTYLE_CAST_RX = re.compile(
    r"(?<![\w)\]>])\(\s*(?:" + ARITH_T + r"(?:\s*const)?|(?:(?:const|volatile|struct|class|enum|unsigned|signed)\s+)*"
    r"[A-Za-z_][\w:]*(?:\s*<[^()]*>)?(?:\s*const)?(?:\s*\(\s*\*\s*\)\s*\([^()]*\))?(?:\s*[*&](?:\s*const)?)+)"
    r"\s*\)\s*(?=[\w(&*!~\"'-]|::)")
CSTYLE_NOT_RX = re.compile(r"\b(?:if|while|for|switch|sizeof|alignof|alignas|decltype|defined|__attribute__|"
                           r"operator|catch|static_assert|__typeof__|typeid|noexcept)\s*$")


def _blank_named_cast_types(code):
    out, i = [], 0
    for m in NAMED_CAST_RX.finditer(code):
        if m.start() < i:
            continue
        gt = _balanced(code, m.end() - 1, "<(", ">)")
        out.append(code[i:m.end()] + " " * (gt - m.end() - 1) + ">")
        i = gt
    out.append(code[i:])
    return "".join(out)


def has_cstyle_cast(code):
    code = _blank_named_cast_types(code)
    for m in CSTYLE_CAST_RX.finditer(code):
        if CSTYLE_NOT_RX.search(code[:m.start()]):
            continue
        return True
    return False

PATTERNS = [
    # A: (char*)expr + numeric-offset (any deref/address-of context); the offset
    # may be scaled: `+ idx * 0x18` / `+ (short)p->idx * 0x18` is the same raw
    # byte-offset math with an index expression in front of the magic number.
    ("A", re.compile(
        r"\(\s*(?:unsigned\s+|signed\s+)?char\s*\*\s*\)\s*"   # (char*) cast
        r"[&(]*[A-Za-z_]" + BASE_TAIL + r"\)*\s*"              # the base expr
        r"[+-]\s*(?:(?:\([^()]*\)\s*)?[\w.>()-]*\s*\*\s*)?" + NUM)),
    # B: *(T*)(ident + numeric-offset)
    ("B", re.compile(
        r"\*\s*\(\s*[A-Za-z_][\w ]*\*+\s*(?:const\s*)?\)\s*"  # *(T*) / *(T**)
        r"\(\s*[A-Za-z_]" + BASE_TAIL + r"\s*\+\s*" + NUM + r"\s*\)")),
    # C: ->m_...Unk... + offset  (pointer field to an unmodeled struct)
    ("C", re.compile(
        r"(?:->|\.)\s*m_p?\w*Unk\w*\s*\+\s*" + NUM)),
    # D: access through a _pad-named field
    ("D", re.compile(
        r"(?:->|\.)\s*_pad\w*")),
    # K: cast of `this` to another C-prefixed class pointer, `(CBase*)this` — a
    # cast-this-to-base smell that means real C++ inheritance isn't modeled. The
    # fix is `class CDerived : public CBase { ... }` so the inherited method/field
    # is reached directly (no cast). HIGH PRIORITY. Catches every
    # form: the direct method call `((CSWSObject*)this)->GetArea()`, the field read
    # `((CSWSObject*)this)->m_field`, and the two-step local `CSWSObject* self =
    # (CSWSObject*)this;`. (A cast the other direction — a base method downcasting
    # its `this` to a derived type — is the same smell and also matches.)
    ("K", re.compile(
        r"\(\s*C[A-Z]\w*\s*\*\s*\)\s*this\b")),
    # L: delete[] of a local (non-`m_`) list's buffer — the dtor frees it again.
    ("L", re.compile(
        r"(?:\bdelete\s*\[\s*\]|\boperator\s+delete\s*\[\s*\]\s*\()\s*\(?\s*"
        r"(?!m_)[A-Za-z_]\w*\s*\.\s*(?:m_pData|m_pArray)\b")),
    # M: an empty inline-asm statement -- an optimiser barrier, not source.
    ("M", re.compile(
        r"\b(?:__)?asm(?:__)?\s*(?:(?:__)?volatile(?:__)?\s*)?\(\s*\"\"")),
]

# F: raw vtable-slot call; slot 1 (deleting dtor) is the established teardown
# idiom and stays unflagged. Two spellings exist in the wild (both flagged):
#   (*(T**)expr)[N](expr)          — the common form
#   ((T*)*(void***)expr)[N](expr)  — an equivalent variant
# and the slot index may be a plain integer or a byte-offset expression like
# [0x1a8 / 8]. group(1) = the fn-ptr typedef name, group(2) = the slot-index
# expression text.
SLOT = r"((?:0x[0-9a-fA-F]+|\d+)(?:\s*/\s*8)?)"
VTBL_RX = re.compile(
    r"\(\s*\*\s*\(\s*([A-Za-z_]\w*)\s*\*\*\s*\)\s*[^)]*\)\s*\[\s*" + SLOT + r"\s*\]\s*\(")
VTBL_RX2 = re.compile(
    r"\(\s*\(\s*([A-Za-z_]\w*)\s*\*\s*\)\s*\*\s*\(\s*void\s*\*+\s*\)\s*[^)]*\)"
    r"\s*\[\s*" + SLOT + r"\s*\]\s*\(")

# F (cont.): ((T*)this) — a raw cast of `this` to a non-C-prefixed per-TU
# "probe" type (e.g. `((StoreVtblProbe*)this)->LoadFromGff(...);`). This is
# the same unmodeled-virtual debt as a raw slot dispatch, with the slot math
# hidden inside the probe type's layout instead of written at the call site —
# model the virtual on the class's real vtable and call it by name. C-prefixed
# targets are excluded here: those are class K (cast-this-to-base) and get
# their own reporting. Unlike A/B/C this is exempt-able: a reviewed,
# deliberately-kept cast takes `// idiom-exempt: <reason>`.
THIS_CAST_RX = re.compile(
    r"\(\s*\(\s*(?:const\s+)?(?!C[A-Z])[^\W]\w*\s*\*+\s*\)\s*this\s*\)"
    # ... or the inner cast of a cast chain, `(CAurObject*)(Gob*)this`
    r"|\)\s*\(\s*(?:const\s+)?(?!C[A-Z])[^\W]\w*\s*\*+\s*\)\s*this\b")


# A/B/F/N (cont.): the C++-cast and typed-stride spellings of raw offset math. The
# A/B regexes above only know the C-style `(char*)p + 0x58`, so the same access
# written `reinterpret_cast<char*>(p) + 0x58`, `((unsigned char*)p)[0xa8]`,
# `(const char*)this + 1` or `((int*)p)[2]` would sail through. A cast operand can
# nest parentheses, so the C++ casts and the `((T*)p)` groups are walked with a
# balanced-paren scan.
CHAR_T = (r"(?:const\s+)?(?:unsigned\s+|signed\s+)?"
          r"(?:char|(?:std::)?u?int8_t|BYTE|(?:std::)?byte)(?:\s+const)?")
PTR_T = (r"(?:const\s+)?(?:(?:unsigned|signed|long|short)\s+)*"
         r"[A-Za-z_][\w:]*(?:\s+const)?(?:\s*\*)+(?:\s*const)?")
CHAR_PTR_RX = re.compile(r"^\s*" + CHAR_T + r"\s*\*\s*$")
# a scalar pointee: stepping one by a literal walks an object in int/float/pointer
# units -- the same offset with the stride hidden in the type. A cast to a struct
# pointer stepped by a literal (`(Header4PC*)m_pFileData + 1`, `((Vector*)p)[1]`) is
# typed array/header-skip access and is not flagged.
SCALAR_PTR_RX = re.compile(
    r"^\s*(?:const\s+)?(?:(?:unsigned|signed|long|short)\s+)*"
    r"(?:int|short|long|float|double|bool|unsigned|signed|u?int(?:16|32|64)_t|"
    r"u?intptr_t|size_t|DWORD|WORD|BOOL|void(?=(?:\s*\*){2}))\b"
    r"(?:\s+const)?\s*\*|(?:\s*\*){2}")
CXX_CAST_RX = re.compile(
    r"\b(?:reinterpret_cast|static_cast)\s*<\s*(" + PTR_T + r")\s*>\s*\(")
# `((T*)expr)` -- the outer paren must not be a call's argument list
C_CAST_GROUP_RX = re.compile(
    r"(?<![\w\])>])\(\s*\(\s*(" + PTR_T + r")\s*\)\s*(?=[A-Za-z_&(])")
# `(T*)base + N` with a plain identifier/member-chain base (no call, no paren sum:
# `(const char*)(data + 9)` re-types a byte-buffer cursor, it is not an offset)
C_CAST_PLUS_RX = re.compile(
    r"\(\s*(" + PTR_T + r")\s*\)\s*&?[A-Za-z_][\w]*(?:\s*\[[^\[\]]*\])*"
    r"(?:\s*(?:->|\.)\s*[A-Za-z_]\w*(?:\s*\[[^\[\]]*\])*)*"
    r"\s*[+-]\s*" + NUM + r"\b(?!\s*[*/\[(])")
# what follows a cast group: a literal step `+ 0x58` / `+ idx * 0x18`, or a literal
# subscript `[0xa8]` (index 0 is not an offset and is not matched)
TAIL_PLUS_RX = re.compile(
    r"\s*[+-]\s*(?:(?:\([^()]*\)\s*)?[\w.>()-]*\s*\*\s*)?" + NUM + r"\b(?!\s*[*/\[(])")
TAIL_INDEX_RX = re.compile(r"\s*\[\s*(?!0+\s*\])(?:0x[0-9a-fA-F]+|\d+)\s*\]")
# B, C++ spelling: *reinterpret_cast<T*>(p + 0x438)
B_CXX_RX = re.compile(
    r"\*\s*reinterpret_cast\s*<\s*" + PTR_T + r"\s*>\s*\(\s*[A-Za-z_]" + BASE_TAIL + r"\s*\+\s*"
    + NUM + r"\s*\)")
# F, the pointer-step spelling of a raw vtable call: `(*(void***)p + 43)(p)` loads
# the vtable and steps it 43 slots (VTBL_RX only knows the `[43]` subscript form)
VTBL_STEP_RX = re.compile(
    r"\*\s*\(\s*[A-Za-z_]\w*(?:\s*\*){3}\s*\)\s*[A-Za-z_][\w.>-]*\s*\+\s*"
    r"(0x[0-9a-fA-F]+|\d+)\s*\)")


def _close(code, i):
    """Index just past the ')' matching the '(' at code[i-1], or -1."""
    depth = 1
    while i < len(code):
        if code[i] == "(":
            depth += 1
        elif code[i] == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return -1


def _step_class(ptr_type, operand, indexed):
    """'A' for a char-typed view stepped by a literal, 'N' for a scalar-typed one,
    None for a struct-typed one (typed array access). A char subscript of an
    already-named field's own bytes (`((char*)&m_field)[1]`) is not flagged."""
    if CHAR_PTR_RX.match(ptr_type):
        if indexed and operand.startswith("&"):
            return None
        return "A"
    if SCALAR_PTR_RX.match(ptr_type):
        return "N"
    return None


def cast_step_classes(code):
    """Classes for a pointer cast stepped by a literal offset (see the comment above
    CHAR_T): A (char view), N (scalar view), B (`*reinterpret_cast<T*>(p + 0x438)`),
    F (`*(void***)p + 43` vtable step, slot != 1)."""
    found = set()
    if B_CXX_RX.search(code):
        found.add("B")
    if any(int(m.group(1), 0) != 1 for m in VTBL_STEP_RX.finditer(code)):
        found.add("F")
    # reinterpret_cast<T*>(expr) + N  /  reinterpret_cast<T*>(expr)[N]
    for m in CXX_CAST_RX.finditer(code):
        end = _close(code, m.end())
        if end < 0:
            continue
        operand = code[m.end():end - 1].strip()
        if operand.startswith('"'):
            continue
        if TAIL_PLUS_RX.match(code, end):
            cls = _step_class(m.group(1), operand, False)
        elif TAIL_INDEX_RX.match(code, end):
            cls = _step_class(m.group(1), operand, True)
        else:
            continue
        if cls:
            found.add(cls)
    # ((T*)expr)[N]  /  ((T*)expr) + N
    for m in C_CAST_GROUP_RX.finditer(code):
        end = _close(code, m.start() + 1)
        if end < 0:
            continue
        operand = code[m.end():end - 1].strip()
        if TAIL_INDEX_RX.match(code, end):
            cls = _step_class(m.group(1), operand, True)
        elif TAIL_PLUS_RX.match(code, end):
            cls = _step_class(m.group(1), operand, False)
        else:
            continue
        if cls:
            found.add(cls)
    # (T*)base + N -- the plain `(char*)p + N` is class A's own regex; this adds the
    # const/uint8_t/BYTE char spellings and the scalar views. A `*` right before the
    # cast means the step applies to the loaded value, not the cast (VTBL_STEP_RX).
    for m in C_CAST_PLUS_RX.finditer(code):
        if code[:m.start()].rstrip().endswith("*"):
            continue
        cls = _step_class(m.group(1), "", False)
        if cls:
            found.add(cls)
    return found


def slot_value(expr):
    """Evaluate a slot-index expression: '13', '0x68 / 8', '0xd' -> int."""
    expr = expr.replace(" ", "")
    if expr.endswith("/8"):
        return int(expr[:-2], 0) // 8
    return int(expr, 0)


# a function-pointer typedef at any scope: typedef R (*Name)(args);
FNPTR_TYPEDEF_RX = re.compile(
    r"^\s*typedef\s+[^(;]+\(\s*\*\s*([A-Za-z_]\w*)\s*\)\s*\([^;]*\)\s*;")

# H: an m_Unk* field DEFINITION (placeholder-named struct member). Anchored as a
# declaration — a type expression, then the `m_..Unk..` name, then an optional
# array/bitfield, then ';'. The leading-char class excludes `->`/`.`/`=`/`(`, so
# `p->m_Unk08` uses and `x = m_Unk08;` reads do not match (only real declarations).
UNK_DEF_RX = re.compile(
    r"^\s*(?!(?:return|delete|goto|case|throw|co_return|co_yield)\b)"  # not a statement
    r"[\w:][\w:*&<>,\s]*?\bm_\w*Unk\w*\b\s*"
    r"(?:\[[^\]]*\])?\s*(?::\s*\d+\s*)?;")

# I: memcpy/memset/memclr/memmove — flag if the size (last) argument has no `sizeof`.
MEMFUNC_CALL_RX = re.compile(r"(?<!\w)(?:__builtin_)?mem(?:cpy|set|clr|move)\s*\(")

# J: sizeof(char) / sizeof(unsigned char) / sizeof(signed char) — always 1.
SIZEOF_CHAR_RX = re.compile(r"\bsizeof\s*\(\s*(?:unsigned\s+|signed\s+)?char\s*\)")

# class W: a bare `long` (see the module docstring), string literals blanked first.
# class L: `asm("name")` after a declarator (a `)`, an identifier or `throw()`), not an asm statement.
ASM_LABEL_RX = re.compile(r'(?:[\w)\]])\s*(?:asm|__asm__)\s*\(\s*"')
LONG_RX = re.compile(r"(?<!\blong\s)(?<!\blong\s\s)\blong\b(?!\s+(?:long|double)\b)")
STRING_RX = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
LIBC_LONG_RX = re.compile(r"\b(?:strtol|strtoul|atol|labs|ldiv|ftell|fseek|lround|lroundf|lrint|lrintf|wcstol|"
                          r"wcstoul|_lseek|_tell|lseek|clock|random|srandom|sysconf|ftello|fseeko|tv_usec|"
                          r"tv_nsec|tm_gmtoff|timespec|timeval|nanosleep)\b")

# class P: a declaration of a function port/libc*.h, port/crtapi.h (the MSVC CRT names) or
# port/k2qolapi.h (the QoL layer's hooks) or port/win32api.h (the Win32 calls the shim answers)
# declares, outside those headers. The names come from the headers; a declaration is a
# type-ish prefix (no `=`, `return`, call or member access), the name, a parameter list
# and `;` -- one line.
LIBC_HEADERS = ("libcbase.h", "libcstring.h", "libcstdlib.h", "libcmath.h", "libctime.h", "libcstat.h", "crtapi.h", "k2qolapi.h",
                "win32api.h")
_LIBC_DECL_NAME_RX = re.compile(r"\b([A-Za-z_]\w*)\s*\([^;{()]*(?:\([^()]*\)[^;{()]*)*\)\s*(?:throw\s*\(\s*\)\s*)?"
                                r"(?:__attribute__\s*\(\(.*?\)\)\s*)?;")


def _libc_names(headers=LIBC_HEADERS):
    # Whole statements, so a declaration the header wraps over two lines counts too.
    names = set()
    for h in headers:
        f = ROOT / "src" / "port" / h
        if f.exists():
            code = " ".join(line.split("//")[0] for line in f.read_text().splitlines()
                            if not line.lstrip().startswith("#"))
            names.update(m.group(1) for m in _LIBC_DECL_NAME_RX.finditer(code))
    return names


LIBC_NAMES = _libc_names()
# Win32 names are also class members' names (CNWCTrigger::SetCursor, CExoCriticalSection's Enter/
# LeaveCriticalSection): for a name only win32api.h declares, an indented line that does not start
# with `extern` is a member declaration, not a copy.
WIN32_ONLY_NAMES = _libc_names(("win32api.h",)) - _libc_names(tuple(h for h in LIBC_HEADERS if h != "win32api.h"))
LIBC_PROTO_RX = re.compile(
    r'^\s*(?:extern\s+"C"\s*(?:\{\s*)?)?(?:__attribute__\s*\(\(.*?\)\)\s*)?'
    r"(?:(?:const|unsigned|signed|int|long|short|char|void|bool|float|double|size_t|u?int(?:8|16|32|64)_t|"
    r"wchar_t|FILE|struct\s+\w+|__SIZE_TYPE__|static|inline)\b\s*|\*\s*)+"
    r"(" + "|".join(sorted(LIBC_NAMES, key=len, reverse=True)) + r")\s*\([^;{]*\)\s*"
    r"(?:throw\s*\(\s*\)|noexcept)?\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?;") if LIBC_NAMES else None

TAG = "TODO: idiom"
EXEMPT = "idiom-exempt"


def _split_top_level_args(s):
    """Split a parenthesized call's argument-list text on top-level commas."""
    args, depth, cur = [], 0, []
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            args.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    if cur or not args:
        args.append("".join(cur))
    return args


def has_unsized_memfunc_call(code):
    """True if a mem*() CALL (not a declaration/prototype) on this line has a
    size argument that doesn't mention `sizeof`/`offsetof`. Multi-line calls
    (unbalanced parens on this one line) are skipped — can't safely analyze
    in isolation."""
    for m in MEMFUNC_CALL_RX.finditer(code):
        # A declaration/prototype has the return type `void *` directly before
        # the function name (this codebase's established extern-decl style);
        # a call never does (the name starts the statement or follows an
        # operator/keyword instead).
        if re.search(r"void\s*\*\s*$", code[:m.start()]):
            continue
        depth = 1
        i = m.end()
        while i < len(code) and depth > 0:
            if code[i] == "(":
                depth += 1
            elif code[i] == ")":
                depth -= 1
            i += 1
        if depth != 0:
            continue  # unbalanced on this line -- multi-line call, skip
        args = _split_top_level_args(code[m.end():i - 1])
        if len(args) < 2:
            continue
        if "sizeof" not in args[-1] and "offsetof" not in args[-1]:
            return True
    return False


def strip_comments(line, in_block):
    """Single pass over one line tracking strings, //, and /* */.

    Returns (code, comment, in_block): code = non-comment text, comment =
    everything from the first comment marker on, in_block = carry state.
    """
    code, comment = [], []
    in_str = None
    i, n = 0, len(line)
    while i < n:
        c = line[i]
        if in_block:
            comment.append(c)
            if line.startswith("*/", i):
                comment.append("/")
                in_block = False
                i += 2
                continue
            i += 1
            continue
        if in_str:
            code.append(c)
            if c == "\\" and i + 1 < n:
                code.append(line[i + 1])
                i += 2
                continue
            if c == in_str:
                in_str = None
            i += 1
            continue
        if c in "\"'":
            in_str = c
            code.append(c)
            i += 1
            continue
        if line.startswith("//", i):
            comment.append(line[i:])
            break
        if line.startswith("/*", i):
            in_block = True
            comment.append("/*")
            i += 2
            continue
        code.append(c)
        i += 1
    return "".join(code), "".join(comment), in_block


def scan_file(path):
    """Yield (lineno, classes, code, tagged) for each finding line."""
    in_block = False
    for lineno, raw in enumerate(path.read_text().splitlines(), 1):
        code, comment, in_block = strip_comments(raw, in_block)
        if not code.strip():
            continue
        # every class reads both spellings: the source as written and its named casts put back in
        # C-style form (`uncast`), so a pattern written for one spelling cannot go blind on the other
        plain = uncast(code) if NAMED_CAST_RX.search(code) else code
        classes = {cls for cls, rx in PATTERNS for c in {code, plain} if rx.search(c)}
        classes |= cast_step_classes(code) | (cast_step_classes(plain) if plain is not code else set())
        if any(slot_value(m.group(2)) != 1
               for rx in (VTBL_RX, VTBL_RX2) for c in (code, plain) for m in rx.finditer(c)):
            classes.add("F")
        if THIS_CAST_RX.search(code) or THIS_CAST_RX.search(plain):
            classes.add("F")
        if has_cstyle_cast(STRING_RX.sub('""', code)):
            classes.add("Q")
        if path.suffix == ".cpp" and re.match(r"\s+typedef\b", code):
            classes.add("G")
        if UNK_DEF_RX.search(code):
            classes.add("H")
        if has_unsized_memfunc_call(code):
            classes.add("I")
        if SIZEOF_CHAR_RX.search(code):
            classes.add("J")
        # (a libc name counts in an asm label too: `zs_ftell(...) asm("ftell")` is ftell)
        bare = STRING_RX.sub('""', code)
        labels = " ".join(re.findall(r'\b(?:asm|__asm__)\s*\(\s*"([^"]*)"', code))
        if LONG_RX.search(bare) and not LIBC_LONG_RX.search(bare + " " + labels):
            classes.add("W")
        if ASM_LABEL_RX.search(code):
            classes.add("L")
        m_p = LIBC_PROTO_RX.search(code) if LIBC_PROTO_RX else None
        if (m_p and not (path.parent.name == "port" and path.name in LIBC_HEADERS)
                and not (m_p.group(1) in WIN32_ONLY_NAMES and code[:1].isspace()
                         and not code.lstrip().startswith("extern"))):
            classes.add("P")
        if not classes:
            continue
        # `// idiom-exempt` silences a finding except a raw-offset class (A/B/C)
        # — those are never exempt-able (see module docstring).
        if EXEMPT in comment and not (classes & (OFFSET_CLASSES | {"L"})):
            continue
        yield lineno, sorted(classes), raw, (TAG in comment)


# a class/struct DEFINITION line (body follows): a C-prefixed engine class at any indent, or any
# other class/struct/union at namespace scope (column 0) -- the Win32/SDL/GL records and helper
# types are per-TU copies too
DEF_RX = re.compile(r"^\s*(?:typedef\s+)?(?:struct|class)\s+(C[A-Z]\w*)\b(.*)$")
DEF_ANY_RX = re.compile(r"^(?:typedef\s+)?(?:struct|class|union)\s+(?:__attribute__\(\([^)]*\)\)\s+)?"
                        r"([A-Za-z_]\w*)\b(.*)$")
ANON_NS_OPEN_RX = re.compile(r"^namespace\s*\{")
NS_CLOSE_RX = re.compile(r"^\}\s*(?://.*)?$")


def scan_defs(path):
    """Yield (lineno, name, raw, tagged, exempt) for class/struct definitions. A type inside an
    anonymous namespace is a distinct type per TU and is not reported."""
    in_block = False
    in_anon = False
    lines = path.read_text().splitlines()
    for lineno, raw in enumerate(lines, 1):
        code, comment, in_block = strip_comments(raw, in_block)
        if ANON_NS_OPEN_RX.match(code):
            in_anon = True
            continue
        if in_anon and NS_CLOSE_RX.match(code):
            in_anon = False
            continue
        m = DEF_RX.match(code) or DEF_ANY_RX.match(code)
        if not m or in_anon:
            continue
        rest = m.group(2).strip()
        # A body (`{`) or a base-clause (`:`) means this is a DEFINITION — including the
        # one-liner form `class CFoo { ... };`, which must be checked before the trailing-`;`
        # test below, since it ends with `;` too and would otherwise look like a forward
        # declaration.
        # The brace may also open the next line.
        if not (rest.startswith("{") or rest.startswith(":") or rest.startswith("final")):
            if rest:
                continue
            nxt = next((l.strip() for l in lines[lineno:lineno + 2] if l.strip()), "")
            if not nxt.startswith("{"):
                continue
        yield lineno, m.group(1), raw, (TAG in comment), (EXEMPT in comment)


_RENAMES = None


def home_dirs(path):
    """The class directories a file belongs to: its parent, except for a .cpp that the path
    migration moved out of its class directory (the rename table, docs/path_renames.tsv), whose
    homes stay the directories it came from -- several for a merged file, the rename target's
    first. Keeps the per-TU groupings below (home class, fn-ptr typedef scope, match.py's per-TU
    view) unchanged by moves and merges."""
    global _RENAMES
    if _RENAMES is None:
        _RENAMES = {}
        f = ROOT / "docs" / "path_renames.tsv"
        if f.is_file():
            for line in f.read_text().splitlines():
                cols = line.split("\t")
                if len(cols) >= 2 and not line.startswith("#") and cols[0] != "old":
                    homes = _RENAMES.setdefault((ROOT / cols[1]).resolve(), [])
                    d = (ROOT / cols[0]).parent.resolve()
                    if d not in homes:
                        homes.append(d)
    rp = Path(path).resolve()
    return _RENAMES.get(rp, [rp.parent])


def home_dir(path):
    return home_dirs(path)[0]


def is_home(path, name):
    """True if this file lives in the class's own TU directory."""
    return any(name in (d, "C" + d, d + "Internal", "C" + d + "Internal")
               for d in (h.name for h in home_dirs(path)))


def collect_dups():
    """Class-E findings: non-home, non-exempt definition sites of classes that
    are defined 2+ times engine-wide. Returns {path: [(lineno, raw, tagged)]},
    plus {name: [all site strings]} for reporting."""
    defs = {}  # name -> [(path, lineno, raw, tagged, exempt)]
    for path in sorted(SRC.rglob("*.h")) + sorted(SRC.rglob("*.cpp")):
        for lineno, name, raw, tagged, exempt in scan_defs(path):
            defs.setdefault(name, []).append((path, lineno, raw, tagged, exempt))
    findings, sites_by_name = {}, {}
    for name, sites in defs.items():
        # a file's #if KOTOR1 / #else arms are one definition per build
        if len({p for p, *_ in sites}) < 2:
            continue
        sites_by_name[name] = [f"{p.relative_to(ROOT)}:{ln}" for p, ln, *_ in sites]
        # a non-C-prefixed type's canonical site is its header definition
        in_header = [p for p, *_ in sites if p.suffix == ".h"]
        for path, lineno, raw, tagged, exempt in sites:
            if exempt or is_home(path, name):
                continue
            if not name.startswith("C") and path.suffix == ".h" and len(in_header) == 1:
                continue
            findings.setdefault(path, []).append((lineno, raw, tagged))
    return findings, sites_by_name


def collect_fnptr_typedefs():
    """Class-G sub-shapes (2)+(3): a fn-ptr typedef AT ANY SCOPE whose name
    services a non-slot-1 raw vtable dispatch anywhere in its TU DIRECTORY
    (hoisting to the header doesn't retire it — only the class-F fix does),
    or a DEAD fn-ptr typedef (name used nowhere else in the dir). A typedef
    whose only dispatch uses are the exempt slot-1 teardown idiom is fine.
    Returns {path: [(lineno, raw, tagged)]}."""
    by_dir = {}
    for path in sorted(SRC.rglob("*.h")) + sorted(SRC.rglob("*.cpp")):
        homes = home_dirs(path)
        # a merged .cpp's uses count in every class dir it came from; its typedefs are reported once
        infos = [by_dir.setdefault(h, {"slots": {}, "uses": {}, "tdefs": []}) for h in homes]
        info = infos[0]
        in_block = False
        for lineno, raw in enumerate(path.read_text().splitlines(), 1):
            code, comment, in_block = strip_comments(raw, in_block)
            if not code.strip():
                continue
            m = FNPTR_TYPEDEF_RX.match(code)
            if m:
                info["tdefs"].append((path, lineno, m.group(1), raw,
                                      TAG in comment, EXEMPT in comment))
                continue  # the definition line itself is not a "use"
            for inf in infos:
                for rx in (VTBL_RX, VTBL_RX2):
                    for dm in rx.finditer(code):
                        inf["slots"].setdefault(dm.group(1), set()).add(slot_value(dm.group(2)))
                for nm in set(re.findall(r"\b[A-Za-z_]\w*\b", code)):
                    inf["uses"][nm] = inf["uses"].get(nm, 0) + 1
    findings = {}
    for info in by_dir.values():
        for path, lineno, name, raw, tagged, exempt in info["tdefs"]:
            if exempt:
                continue
            slots = info["slots"].get(name, set())
            if slots - {1}:
                pass                       # services a real (non-slot-1) raw dispatch
            elif slots:
                continue                   # slot-1-only: sanctioned teardown-idiom typedef
            elif info["uses"].get(name, 0) == 0:
                pass                       # dead — no use anywhere in the TU dir
            else:
                continue                   # a legit type alias in real use
            findings.setdefault(path, []).append((lineno, raw, tagged))
    return findings


def collect():
    findings = {}  # path -> list of (lineno, classes, raw, tagged)
    for path in sorted(SRC.rglob("*.cpp")) + sorted(SRC.rglob("*.h")):
        rows = list(scan_file(path))
        if rows:
            findings[path] = rows
    for path in platform_files():
        rows = [(lineno, ["Q"], raw, tagged) for lineno, classes, raw, tagged in scan_file(path) if "Q" in classes]
        if rows:
            findings[path] = rows
    dup_findings, _ = collect_dups()
    for path, rows in dup_findings.items():
        merged = findings.setdefault(path, [])
        merged += [(lineno, ["E"], raw, tagged) for lineno, raw, tagged in rows]
        merged.sort(key=lambda r: r[0])
    for path, rows in collect_fnptr_typedefs().items():
        merged = findings.setdefault(path, [])
        have = {r[0]: r for r in merged}
        for lineno, raw, tagged in rows:
            if lineno in have:              # already flagged (e.g. per-line G) — merge class
                r = have[lineno]
                if "G" not in r[1]:
                    merged[merged.index(r)] = (r[0], sorted(set(r[1]) | {"G"}), r[2], r[3])
            else:
                merged.append((lineno, ["G"], raw, tagged))
        merged.sort(key=lambda r: r[0])
    return findings


def collect_orphan_tags(findings):
    """Lines carrying '// TODO: idiom' on real CODE with no finding — stale
    leftovers from an already-fixed site (they hide real debt movement).
    Pure-comment lines (prose that mentions the tag) are not counted."""
    flagged = {(p, r[0]) for p, rows in findings.items() for r in rows}
    orphans = []
    for path in sorted(SRC.rglob("*.cpp")) + sorted(SRC.rglob("*.h")):
        in_block = False
        for lineno, raw in enumerate(path.read_text().splitlines(), 1):
            code, comment, in_block = strip_comments(raw, in_block)
            if TAG in comment and code.strip() and (path, lineno) not in flagged:
                orphans.append((path, lineno, raw))
    return orphans


def suggest(classes):
    """A one-line fix hint for classes where the tool can suggest something
    concrete (raw-offset A/B/C; the two new high-priority I/J classes)."""
    s = set(classes)
    if s & OFFSET_CLASSES:
        return ("suggest: &this->m_Member (real typed member), or give the raw "
                "region its own exactly-positioned named `_raw*` field and "
                "reference it directly -- not offsetof() (reserved for memcpy/"
                "memset size args only)")
    if "N" in s:
        return ("suggest: name the member the scalar index lands on (index x sizeof(T) = "
                "the byte offset); a genuinely raw byte buffer takes "
                "`// idiom-exempt: <buffer, why raw>`")
    if "I" in s:
        return "suggest: sizeof(*dst)/sizeof(SomeType) instead of the bare byte count"
    if "J" in s:
        return "suggest: just write 1 (sizeof(char) is always 1)"
    if "L" in s:
        return ("suggest: delete the line -- List/CExoArrayList's destructor frees the "
                "buffer at scope exit; scope the local where the orig frees it")
    if "M" in s:
        return ("suggest: delete it and recompile; if a MATCH row breaks, find the source "
                "shape it stands in for, else tag `// idiom-exempt: <row>`")
    if "K" in s:
        return ("suggest: model real inheritance `class CDerived : public CBase` "
                "and drop the cast (call the inherited method / read the field directly)")
    if "F" in s:
        return ("suggest: resolve the class's _ZTV relocations and model the vtable as "
                "declared-never-defined virtuals, then call p->Method() —"
                "byte-identical to the raw dispatch / ((ProbeT*)this) cast")
    if "G" in s:
        return ("suggest: if it services a raw dispatch, the class-F fix retires both; "
                "if dead, delete it; a plain local alias hoists to the TU header")
    return None


def filter_findings(findings, paths, only):
    """Restrict to files under any of `paths` and to finding classes in `only`."""
    if paths:
        wanted = [Path(p).resolve() for p in paths]
        findings = {f: rows for f, rows in findings.items()
                    if any(f.resolve() == w or w in f.resolve().parents
                           for w in wanted)}
    if only:
        keep = set(only.upper())
        findings = {f: r for f, rows in findings.items()
                    if (r := [row for row in rows if keep & set(row[1])])}
    return findings


# Every raw-offset / cast-smell class must give the same verdict for a line in C-style spelling
# and in its named-cast spelling: a pattern written for one spelling must not go blind on the
# other (a subscripted base such as `m_apScreenPanels[1] + 0x2775` was invisible in both). Each
# row: (expected classes, C-style line, named-cast line). `--selftest` exits 1 on any mismatch.
SPELLING_FIXTURES = [
    ("A", "unsigned char* p = (unsigned char*)((char*)m_apScreenPanels[1] + 0x2775);",
          "unsigned char* p = reinterpret_cast<unsigned char*>(reinterpret_cast<char*>(m_apScreenPanels[1]) + 0x2775);"),
    ("A", "return *(signed char*)((char*)m_apScreenPanels[0] + 0x56e1);",
          "return *reinterpret_cast<signed char*>(reinterpret_cast<char*>(m_apScreenPanels[0]) + 0x56e1);"),
    ("A", "x = *(int*)((char*)GetThing() + 0x10);",
          "x = *reinterpret_cast<int*>(reinterpret_cast<char*>(GetThing()) + 0x10);"),
    ("A", "x = *(int*)((char*)m_a[i].m_p + 0x10);",
          "x = *reinterpret_cast<int*>(reinterpret_cast<char*>(m_a[i].m_p) + 0x10);"),
    ("A", "char* q = (char*)pVoid + 0x30;",
          "char* q = static_cast<char*>(pVoid) + 0x30;"),
    ("B", "x = *(int*)(m_pBytes[2] + 0x10);",
          "x = *reinterpret_cast<int*>(m_pBytes[2] + 0x10);"),
    ("N", "x = ((int*)p)[2];",
          "x = reinterpret_cast<int*>(p)[2];"),
    ("K", "((CSWSObject*)this)->GetArea();",
          "static_cast<CSWSObject*>(this)->GetArea();"),
    ("F", "return (CAurObject*)(Gob*)this;",
          "return reinterpret_cast<CAurObject*>(static_cast<Gob*>(this));"),
    ("F", "((FooProbe*)this)->Load();",
          "reinterpret_cast<FooProbe*>(this)->Load();"),
    ("F", "(*(VFn**)p)[7](p);",
          "(*reinterpret_cast<VFn**>(p))[7](p);"),
    ("", "x = (int)fValue + 3;",
         "x = static_cast<int>(fValue) + 3;"),
    ("", "f((char*)p, 0x10);",
         "f(reinterpret_cast<char*>(p), 0x10);"),
]


def selftest():
    import tempfile
    bad = 0
    for want, cstyle, named in SPELLING_FIXTURES:
        got = []
        for line in (cstyle, named):
            with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as fh:
                fh.write(line + "\n")
            cls = set()
            for _, c, _, _ in scan_file(Path(fh.name)):
                cls |= set(c)
            Path(fh.name).unlink()
            got.append("".join(sorted(cls - {"Q"})))
        ok = got[0] == got[1] == want
        bad += not ok
        print(f"{'PASS' if ok else 'FAIL'} want [{want}] c-style [{got[0]}] named [{got[1]}]  {named}")
    print(f"{len(SPELLING_FIXTURES) - bad}/{len(SPELLING_FIXTURES)} fixtures agree in both spellings")
    return 1 if bad else 0


def main():
    import argparse
    ap = argparse.ArgumentParser(
        description="non-idiomatic C++ lint (see module docstring for classes)")
    ap.add_argument("paths", nargs="*",
                    help="restrict to these files/dirs (e.g. src/game/servercore/CSWSStore); "
                         "with a path and no mode flag, findings are LISTED")
    ap.add_argument("--list", action="store_true", dest="list_mode",
                    help="print every finding as file:line: [classes] code")
    ap.add_argument("--doc", action="store_true",
                    help="markdown catalog (for docs/IDIOM_CLEANUP.md)")
    ap.add_argument("--tag", action="store_true",
                    help="append '// TODO: idiom' to untagged finding lines")
    ap.add_argument("--only", metavar="CLASSES", default="",
                    help="restrict to finding classes, e.g. --only EF")
    ap.add_argument("--orphans", action="store_true",
                    help="list stale '// TODO: idiom' tags on lines with no finding")
    ap.add_argument("--selftest", action="store_true",
                    help="every class gives the same verdict in C-style and named-cast spelling")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    mode = ("--tag" if a.tag else "--doc" if a.doc else
            "--list" if a.list_mode or a.paths or a.only else "")

    all_findings = collect()
    if a.orphans:
        orphans = collect_orphan_tags(all_findings)
        for path, lineno, raw in orphans:
            print(f"  {path.relative_to(ROOT)}:{lineno}: {raw.strip()}")
        print(f"\n{len(orphans)} orphan '// {TAG}' tags (code lines with no finding "
              f"— stale; remove the tag, it hides real debt movement)")
        return 0

    findings = filter_findings(all_findings, a.paths, a.only)
    total = sum(len(v) for v in findings.values())
    # H (m_Unk placeholder fields) is a separate, ungated tally — excluded from the
    # A-L "idiom debt" headline and from the untagged commit-gate.
    h_total = sum(1 for v in findings.values() for r in v if "H" in r[1])
    core_total = total - h_total
    untagged = sum(1 for v in findings.values() for r in v
                   if not r[3] and (set(r[1]) - {"H"}))

    if mode == "--tag":
        n = 0
        for path, rows in findings.items():
            lines = path.read_text().splitlines(keepends=True)
            for lineno, classes, _raw, tagged in rows:
                if tagged or not (set(classes) - {"H"}):  # skip tagged + ungated H
                    continue
                idx = lineno - 1
                body = lines[idx].rstrip("\n")
                lines[idx] = body + "  // " + TAG + "\n"
                n += 1
            path.write_text("".join(lines))
        print(f"tagged {n} lines (of {total} findings)")
        return 0

    if mode == "--doc":
        print("# Non-idiomatic C++ cleanup catalog")
        print()
        print("Generated by `tools/lint_idiom.py --doc`. Raw offset access =")
        print("an unmodeled struct field; each fix is: model the field (src/ header")
        print("+ Ghidra struct), rewrite the access, confirm `tools/progress.py`")
        print("shows no regression. Classes: A=(char*)+off (incl. scaled `+ idx*0x18`)")
        print("B=*(T*)(p+off)")
        print("C=Unk-ptr+off  D=_pad access  E=duplicate class definition")
        print("(consolidate to the canonical model; see the section below)")
        print("F=raw vtable-slot dispatch (non-slot-1 = an unmodeled virtual),")
        print("or a raw cast of `this` to a non-C-prefixed probe type ((T*)this)->M()")
        print("(C-prefixed this-casts are class K instead; F is exempt-able via")
        print("`// idiom-exempt: <reason>` for a reviewed, deliberate cast)")
        print("G=function-local typedef in a .cpp (hoist to the TU header)")
        print("H=m_Unk* field DEFINITION (placeholder name — LOWEST priority,")
        print("ungated, tallied separately; renamed as module interlinks surface).")
        print("I=memcpy/memset/memclr/memmove with no `sizeof` in the size arg")
        print("(HIGH PRIORITY — prefer sizeof(*dst)/sizeof(T) over a bare byte count).")
        print("J=sizeof(char) (HIGH PRIORITY — always 1; just write 1).")
        print("K=cast-this-to-base `((CBase*)this)->` (HIGH PRIORITY — model real")
        print("inheritance `class CDerived : public CBase` and drop the cast).")
        print("L=asm label on a declaration (never exempt-able: declare the real entity).")
        print("N=scalar-typed view stepped by a literal `((int*)p)[2]` / `(float*)p + 3`")
        print("(the class-A offset with the stride hidden in the type; exempt-able for")
        print("a genuinely raw byte buffer).")
        print("Q=a C-style cast (write static_cast / reinterpret_cast / const_cast;")
        print("tools/castfix.py rewrites them).")
        print()
        print("**Severity: A-E and I-J first** — those are non-local hazards or")
        print("correctness smells (a wrong offset/size or drifted sizeof silently")
        print("regresses OTHER TUs, or masks a wrong-type bug). F-G are")
        print("self-contained (the site's own byte-match verifies the slot);")
        print("idiom debt only, clean up last or opportunistically. H is a")
        print("separate ungated naming tally — lowest priority of all.")
        print()
        print("⚠ **A/B/C (raw offset) findings can NEVER be `// idiom-exempt`'d** —")
        print("only fixed (`&this->m_Member`, or a properly-sized/positioned named")
        print("`_raw*` field for a region that must stay untyped — see the module")
        print("docstring; `offsetof` is reserved for memcpy/memset size args, not")
        print("a general substitute) or, failing that, tagged `// TODO: idiom`.")
        print()
        print(f"**{core_total} idiom findings (A-N) + {h_total} m_Unk placeholder "
              f"fields (H) across {len(findings)} files** ({untagged} untagged).")
        print()
        _, sites_by_name = collect_dups()
        if sites_by_name:
            print(f"## Duplicate class definitions ({len(sites_by_name)} names)")
            print()
            print("Consolidation targets — each class should end with ONE")
            print("canonical model (in its home TU dir, or a site marked")
            print("`// idiom-exempt: canonical`); all other definitions are")
            print("class-E findings in the per-file lists below.")
            print()
            for name, sites in sorted(sites_by_name.items(),
                                      key=lambda kv: -len(kv[1])):
                print(f"- **{name}** ({len(sites)}): " +
                      ", ".join(f"`{s}`" for s in sites))
            print()
        for path, rows in sorted(findings.items(), key=lambda kv: -len(kv[1])):
            rel = path.relative_to(ROOT)
            print(f"## {rel} ({len(rows)})")
            print()
            for lineno, classes, raw, _tagged in rows:
                snippet = raw.strip()
                if len(snippet) > 100:
                    snippet = snippet[:97] + "..."
                hint = suggest(classes)
                hint_s = f" — {hint}" if hint else ""
                print(f"- `{rel}:{lineno}` [{','.join(classes)}] `{snippet}`{hint_s}")
            print()
        return 0

    if mode == "--list":
        for path, rows in sorted(findings.items()):
            rel = path.relative_to(ROOT)
            for lineno, classes, raw, tagged in rows:
                mark = "!" if (not tagged and set(classes) - {"H"}) else " "
                hint = suggest(classes)
                hint_s = f"  — {hint}" if hint else ""
                print(f"{mark} {rel}:{lineno}: [{','.join(classes)}] {raw.strip()}{hint_s}")
        print(f"\n{core_total} idiom findings (A-N), {untagged} untagged ('!'); "
              f"{h_total} m_Unk placeholder fields (H, ungated)")
        return 1 if untagged else 0

    # default: per-file table (H = the ungated m_Unk placeholder tally, last column)
    print(f"{'file':<80} {'A':>4} {'B':>4} {'C':>4} {'D':>4} {'E':>4} {'F':>4} {'G':>4} {'I':>4} {'J':>4} {'K':>4} {'L':>4} {'M':>4} {'N':>4} {'P':>4} {'Q':>4} {'H':>5} {'untag':>6}")
    for path, rows in sorted(findings.items(), key=lambda kv: -len(kv[1])):
        rel = str(path.relative_to(ROOT))
        by = {c: 0 for c in "ABCDEFGHIJKLMNPQW"}
        un = 0
        for _lineno, classes, _raw, tagged in rows:
            for c in classes:
                by[c] += 1
            if not tagged and (set(classes) - {"H"}):
                un += 1
        print(f"{rel:<80} {by['A']:>4} {by['B']:>4} {by['C']:>4} {by['D']:>4}"
              f" {by['E']:>4} {by['F']:>4} {by['G']:>4} {by['I']:>4} {by['J']:>4}"
              f" {by['K']:>4} {by['L']:>4} {by['M']:>4} {by['N']:>4} {by['P']:>4} {by['Q']:>4} {by['H']:>5} {un:>6}")
    n_orphans = len(collect_orphan_tags(all_findings))
    orphan_s = (f"; {n_orphans} orphan tags (--orphans to list)" if n_orphans else "")
    print(f"\nTOTAL: {core_total} idiom debt (A-N) in {len(findings)} files; "
          f"{untagged} UNTAGGED (must be 0 before commit); "
          f"{h_total} m_Unk placeholder fields (H, ungated, lowest priority)"
          f"{orphan_s}")
    return 1 if untagged else 0


if __name__ == "__main__":
    sys.exit(main())
