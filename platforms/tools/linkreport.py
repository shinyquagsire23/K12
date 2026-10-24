#!/usr/bin/env python3
"""linkreport.py -- what does the transcribed engine need from the outside world?

Reads every object in the host engine archive (libk2_engine.a), collects the symbols it defines and
the ones it references but does not define, and classifies each unresolved symbol by WHO provides it
in the shipping Android build and WHETHER the host platform can satisfy it:

  host        resolved by the host platform as-is (libSystem, libc++, OpenGL.framework, SDL2 via
              sdl2-compat). Nothing to do.
  engine      defined by the original libkotor2.so -> engine code we have not transcribed yet.
  platform    imported by the original libkotor2.so from another library (SDL2, FMOD, GLES, OBB VFS,
              OpenSL ES, libandroid, ...) and NOT available on the host -> a port must stub or
              reimplement it. Grouped by the providing library.
  alias       a bare (no leading underscore) Mach-O name that only exists because a TU spells an
              `asm("name")` alias for a CRT function (e.g. memset); forwarded to the real symbol.
  unknown     defined nowhere: our declaration's mangled name disagrees with the binary (wrong
              signature/class/const), or the original inlined something we call out of line.

Also reports symbols with more than one STRONG definition across the objects (a real link error).

Outputs (in --out):
  linkreport.md      the human report
  linkreport.json    machine-readable classification
  k2stubs.gen.c      abort stubs (functions) + zero-fill storage (data, sized from the .so) for every
                     engine/platform/unknown symbol, so the smoke executable links
  k2alias.gen.S      tail-call forwarders for the `alias` class

Exit status 1 when an engine DATA object (engine or unknown class) has no definition in src/: every
one is defined since link-clean phase 2, so a new one is an `extern` someone forgot to define, and
its stub would run as zero. K2_ALLOW_DATA_STUBS=1 downgrades that to the stub.

--format elf (the Android build, platforms/android): the objects are ELF, the "host" is the NDK
sysroot + the libraries passed as --host-dylib (.so or .a), and the stubs carry ELF names. Names are
kept internally in the Mach-O spelling (a leading '_') either way, so the classification is shared.

--format coff (the Windows build, platforms/windows): x86-64 COFF, whose C names carry no prefix
either, so it is ELF's path throughout; the host is the import and runtime libraries passed as
--host-dylib, and a call is a REL32 relocation on a call/jmp instead of ARM64's CALL26.
"""
import argparse, collections, json, os, re, subprocess, sys

LLVM_BIN = '/opt/homebrew/opt/llvm/bin'   # --llvm-bin
ELF = False    # --format elf or coff: unprefixed C names
COFF = False   # --format coff


def run(cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def tool(name):
    for n in (name, name + '.exe'):
        p = os.path.join(LLVM_BIN, n)
        if os.path.exists(p):
            return p
    return name


# --- our objects -------------------------------------------------------------------------------
def scan_archive_elf(path):
    """scan_archive for ELF objects; names get the Mach-O '_' so everything downstream is shared."""
    defs, undefs = collections.defaultdict(list), collections.defaultdict(set)
    for line in run([tool('llvm-nm'), '-A', '-g', path]).splitlines():
        head, _, rest = line.rpartition(': ')
        f = rest.split()
        if len(f) == 2:
            typ, name = f
        elif len(f) == 3:
            typ, name = f[1], f[2]
        else:
            continue
        obj = os.path.basename(head.split(':')[-1] if ':' in head else head)
        if typ == 'U':
            undefs['_' + name].add(obj)
        elif typ in 'wv':
            continue   # a weak REFERENCE (undefined weak): nothing to stub
        else:
            defs['_' + name].append((obj, typ in 'WVC'))
    # a symbol defined by some object is not unresolved because another references it
    return defs, undefs


def scan_archive(path):
    """-> (defs: name -> [(obj, weak)], undefs: name -> set(obj)). Names WITHOUT the Mach-O '_'."""
    if ELF:
        return scan_archive_elf(path)
    defs, undefs = collections.defaultdict(list), collections.defaultdict(set)
    obj = None
    for line in run(['nm', '-m', '-g', path]).splitlines():
        if not line.strip():
            continue
        m = re.match(r'^(?:.*\()?([^\s()]+\.o)\)?:$', line)
        if m:
            obj = m.group(1)
            continue
        # the symbol is the last token: nm puts qualifiers after `external` too ("automatically hidden",
        # "[no dead strip]"), and reading the token after `external` stubbed real inline definitions
        m = re.match(r'^\s*[0-9a-fA-F]*\s*\(([^)]*)\)\s+(.*?)\s*external\b.*?\s(\S+)$', line)
        if not m:
            continue
        sect, attrs, name = m.groups()
        if sect == 'undefined':
            undefs[name].add(obj)
        else:
            defs[name].append((obj, 'weak' in attrs or sect == 'common'))
    return defs, undefs


# --- the original APK --------------------------------------------------------------------------
def dynsyms(so):
    """-> (defined: name -> (type, size), undefined: set(name)) from an ELF .dynsym."""
    out = run([tool('llvm-readelf'), '--dyn-syms', '-W', so])
    defined, undef = {}, set()
    for line in out.splitlines():
        f = line.split()
        if len(f) < 8 or not f[0].rstrip(':').isdigit():
            continue
        _, _, size, typ, bind, _, ndx, name = f[:8]
        name = name.split('@')[0]
        if not name:
            continue
        if ndx == 'UND':
            undef.add(name)
        elif bind in ('GLOBAL', 'WEAK'):
            try:
                sz = int(size, 0) if size.startswith('0x') else int(size)
            except ValueError:
                sz = 0
            defined[name] = (typ, sz)
    return defined, undef


PROVIDER_LABELS = [
    (r'libSDL2', 'SDL2'),
    (r'libfmod', 'FMOD (audio)'),
    (r'libOpenSLES', 'OpenSL ES (audio)'),
    (r'libGLES|libEGL', 'OpenGL ES / EGL'),
    (r'libObbVfs', 'ObbVfs (APK expansion-file VFS)'),
    (r'libLzmaLib|libminiz|libz', 'compression (LZMA / miniz / zlib)'),
    (r'libhidapi', 'hidapi (controllers)'),
    (r'libandroid|liblog|libjnigraphics|libmediandk', 'Android NDK (libandroid / liblog)'),
    (r'libc\+\+', 'libc++ (NDK __ndk1)'),
    (r'libc\.so|libm\.so|libdl', 'libc / libm / libdl (bionic)'),
    (r'libcrashlytics', 'Crashlytics'),
]


def provider_label(lib):
    for pat, lab in PROVIDER_LABELS:
        if re.search(pat, lib):
            return lab
    return lib


def provider_index(libdirs, orig_so=''):
    """name -> providing library basename, over every .so in the given dirs (first wins), the engine
    library itself (--orig-so: libkotor2.so, or K1's libKOTOR.so) excluded."""
    idx = {}
    for d in libdirs:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.endswith('.so') or fn in ('libkotor2.so', os.path.basename(orig_so)):
                continue
            try:
                defined, _ = dynsyms(os.path.join(d, fn))
            except subprocess.CalledProcessError:
                continue
            for n in defined:
                idx.setdefault(n, fn)
    return idx


# --- the host ------------------------------------------------------------------------------------
def tbd_symbols(path):
    syms = set()
    text = open(path, encoding='utf-8').read()
    for tok in re.findall(r"'([^']+)'|([_$][A-Za-z0-9_$.]+)", text):
        s = tok[0] or tok[1]
        if s.startswith('_'):
            syms.add(s[1:])
    # a framework's Objective-C classes are listed by name (`objc-classes: [ AVAudioSession, ... ]`);
    # what an object references is the class and metaclass symbol
    for block in re.findall(r'objc-classes:\s*\[([^\]]*)\]', text):
        for n in re.findall(r'[A-Za-z_][A-Za-z0-9_]*', block):
            syms.add('OBJC_CLASS_$_' + n)
            syms.add('OBJC_METACLASS_$_' + n)
    return syms


def elf_symbols(lib):
    """Defined global names of an ELF .so (its .dynsym; a versioned libfoo.so.N too) or .a."""
    args = ['-D'] if re.search(r'\.so(\.\d+)*$', lib) else []
    out = set()
    for line in run([tool('llvm-nm')] + args + ['-g', '--defined-only', lib]).splitlines():
        f = line.split()
        if len(f) >= 3 and not f[-1].endswith(':'):
            out.add(f[-1].split('@')[0])
    return out


def host_symbols(extra_dylibs, sdk=None, gl_tbd='System/Library/Frameworks/OpenGL.framework/OpenGL.tbd'):
    if ELF:   # the NDK sysroot + the toolchain's runtime + SDL2/FreeType/gl4es, all passed in
        syms, per_lib = set(), {}
        for lib in extra_dylibs:
            if lib and os.path.exists(lib):
                per_lib[os.path.basename(lib)] = elf_symbols(lib)
                syms |= per_lib[os.path.basename(lib)]
        return syms, per_lib
    sdk = sdk or run(['xcrun', '--show-sdk-path']).strip()
    syms = set()
    tbds = [os.path.join(sdk, 'usr/lib', f) for f in ('libSystem.B.tbd', 'libc++.1.tbd', 'libc++abi.tbd')]
    sysdir = os.path.join(sdk, 'usr/lib/system')
    if os.path.isdir(sysdir):   # the iOS SDKs keep libSystem's re-exports inside libSystem.B.tbd
        tbds += [os.path.join(sysdir, f) for f in os.listdir(sysdir) if f.endswith('.tbd')]
    for t in tbds:
        if os.path.exists(t):
            syms |= tbd_symbols(t)
    gl_name = os.path.basename(os.path.dirname(gl_tbd))   # OpenGL.framework / OpenGLES.framework
    per_lib = {gl_name: tbd_symbols(os.path.join(sdk, gl_tbd))}
    syms |= per_lib[gl_name]
    for lib in extra_dylibs:
        if not lib or not os.path.exists(lib):
            continue
        s = set()
        if lib.endswith('.tbd'):   # an SDK stub library (e.g. the SDK's libz)
            s = tbd_symbols(lib)
        else:
            for line in run(['nm', '-gU', lib]).splitlines():
                f = line.split()
                if len(f) >= 3 and f[2].startswith('_'):
                    s.add(f[2][1:])
        per_lib[os.path.basename(lib)] = s
        syms |= s
    return syms, per_lib


# --- demangling ----------------------------------------------------------------------------------
def demangle_all(names):
    names = list(names)
    if not names:
        return {}
    out = subprocess.run([tool('llvm-cxxfilt')], input='\n'.join(names), capture_output=True,
                         text=True, check=True).stdout.splitlines()
    return dict(zip(names, out))


def owner_of(dem):
    """'Foo::Bar(int)' -> 'Foo';  'vtable for Foo' -> 'Foo';  'free_fn(int)' -> '(free functions)'."""
    for pre in ('vtable for ', 'typeinfo for ', 'typeinfo name for ', 'guard variable for ',
                'non-virtual thunk to ', 'virtual thunk to ', 'construction vtable for '):
        if dem.startswith(pre):
            dem = dem[len(pre):]
    head = dem.split('(')[0]
    depth, cut = 0, -1
    for i, ch in enumerate(head):
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        elif ch == ':' and depth == 0 and head[i:i + 2] == '::':
            cut = i
    if cut < 0:
        return '(free functions / globals)'
    return head[:cut].split('::')[0] if not head.startswith('std::') else 'std'


# --- stubs ---------------------------------------------------------------------------------------
HOMES, HOME_ADDRS = {}, []   # filled from --data-homes in main()
DATA_PREFIXES = ('_ZTV', '_ZTI', '_ZTS', '_ZGV', '_ZTT')


def branch_targets(archives):
    """Mach-O names some object in these archives BRANCHES to (ARM64 BR26): the only reliable
    "this is a function" signal for symbols the original .so does not type (hidden, unexported)."""
    out = set()
    for a in archives:
        if not os.path.exists(a):
            continue
        if COFF:
            # A relative relocation carries a call, a jump and a data reference alike: the
            # instruction says which.
            last = ''
            for line in run([tool('llvm-objdump'), '-dr', '--no-show-raw-insn', a]).splitlines():
                f = line.split()
                if len(f) >= 3 and f[1] == 'IMAGE_REL_AMD64_REL32':
                    if last in ('callq', 'call', 'jmp', 'jmpq'):
                        out.add('_' + re.sub(r'[+-]0x[0-9a-f]+$', '', f[2]))
                elif len(f) >= 2 and f[0].endswith(':') and re.match(r'^[0-9a-f]+:$', f[0]):
                    last = f[1]
            continue
        if ELF:
            for line in run([tool('llvm-objdump'), '-r', a]).splitlines():
                f = line.split()
                if len(f) >= 3 and f[1] in ('R_AARCH64_CALL26', 'R_AARCH64_JUMP26'):
                    out.add('_' + re.sub(r'[+-]0x[0-9a-f]+$', '', f[2]))
            continue
        for line in run([tool('llvm-objdump'), '--macho', '-r', a]).splitlines():
            f = line.split()
            if len(f) >= 7 and f[4] == 'BR26':
                out.add(f[-1])
    return out


def data_homes(path):
    """name -> (vaddr, section) for the unexported engine data docs/DATA_HOMES.md places by the
    address in its declaration comment (platforms/tools/inventory.py writes that file)."""
    out = {}
    if not path or not os.path.exists(path):
        return out
    row = re.compile(r'^\| (0x[0-9a-f]+) \| \d+ \| (\.[\w.]+) \| `([^`]+)`')
    for line in open(path, encoding='utf-8'):
        m = row.match(line)
        if m:
            out[m.group(3)] = (int(m.group(1), 16), m.group(2))
    return out


def c_ident(i):
    return 'k2stub_%d' % i


def c_bytes(b):
    return ','.join(str(x) for x in b)


def c_string(b):
    out = []
    for x in b:
        c = chr(x)
        if c in '\\"':
            out.append('\\' + c)
        elif 0x20 <= x < 0x7f:
            out.append(c)
        else:
            out.append('\\%03o' % x)
    return '"' + ''.join(out) + '"'


def lift_data(img, elf_name, size, known):
    """The original's initializer for one stubbed data object, and its pointer fixups.

    -> (bytes, [(offset, ('sym', macho, addend) | ('str', bytes))], [unresolved notes]).
    A pointer slot is zeroed in the image and patched at startup, to the host's definition of the
    symbol it pointed at, or to a copy of the string literal it pointed at. Anything else (a local
    function, an NDK libc++ object) stays NULL and is reported."""
    if img is None:
        return None, [], []
    if elf_name in img.symbols:
        addr, sz, _ = img.symbols[elf_name]
    elif elf_name in HOMES and HOMES[elf_name][1] in ('.data', '.rodata', '.data.rel.ro', '.got'):
        # Unexported: the address comes from the declaration comment; the size is the gap to the
        # next known object (a scalar reads a few bytes of its neighbour, which nothing uses).
        addr, sect = HOMES[elf_name]
        sz = 8 if sect == '.got' else min(img.next_symbol_after(addr, HOME_ADDRS) - addr, 4096)
    else:
        return None, [], []
    data = bytearray(img.read(addr, sz))
    fix, notes = [], []
    for at, target, sym, addend in img.relocs_in(addr, addr + sz):
        off = at - addr
        data[off:off + 8] = bytes(8)
        if sym is not None:
            if '_' + sym in known:
                fix.append((off, ('sym', '_' + sym, addend)))
            elif sym in img.symbols and img.cstring(img.symbols[sym][0] + addend) is not None \
                    and img.section_of(img.symbols[sym][0]) == '.rodata':
                fix.append((off, ('str', img.cstring(img.symbols[sym][0] + addend))))   # e.g. a _ZTS name
            else:
                notes.append('%s+%#x -> %s (not on this host)' % (elf_name, off, sym))
            continue
        hit = img.symbol_at(target)
        if hit and '_' + hit[0] in known:
            fix.append((off, ('sym', '_' + hit[0], hit[1])))
            continue
        text = img.cstring(target)
        if text is not None and img.section_of(target) in ('.rodata', '.data', '.data.rel.ro'):
            fix.append((off, ('str', text)))
            continue
        notes.append('%s+%#x -> %#x in %s (unnamed)' % (elf_name, off, target, img.section_of(target)))
    return bytes(data), fix, notes


def label(name):
    """The assembler name of an internal ('_'-prefixed, Mach-O-spelled) symbol on this target."""
    return name[1:] if ELF else name


def write_stubs(path, entries, img=None, known=frozenset()):
    """entries: list of (macho_name, kind, size, category, display). With img (the original .so),
    data stubs carry the original's initializer instead of zeros; see lift_data."""
    notes_all, fixups, refs, strs = [], [], {}, {}
    with open(path, 'w', encoding='utf-8') as f:
        f.write('// GENERATED by platforms/tools/linkreport.py -- do not edit.\n')
        f.write('// One definition per symbol the engine references and nobody on this host defines.\n')
        f.write('// Functions abort with their name; data carries the original .so\'s initializer (lift_data).\n')
        f.write('#include <stdio.h>\n#include <stdlib.h>\n\n')
        f.write('// K2_STUBS=soft: log each stub once and return 0 instead of aborting (exploration only;\n')
        f.write('// a float return comes back as garbage, a pointer return as NULL).\n')
        f.write('static void k2_stub_hit(const char* what, const char* name, char* seen)\n')
        f.write('{\n    static int soft = -1;\n')
        f.write('    if (soft < 0) { const char* e = getenv("K2_STUBS"); soft = e && e[0] == \'s\'; }\n')
        f.write('    if (!*seen) fprintf(stderr, "k2: unimplemented %s: %s\\n", what, name);\n')
        f.write('    *seen = 1;\n    if (!soft) abort();\n}\n\n')
        for i, (asm, kind, size, cat, name) in enumerate(entries):
            esc = name.replace('\\', '\\\\').replace('"', '\\"')
            if kind == 'data':
                init, fix, notes = lift_data(img, asm[1:], size, known)
                n = max(size, 16, len(init) if init else 0)
                notes_all += notes
                for off, what in fix:
                    fixups.append((c_ident(i), off, what))
                if init and any(init):
                    f.write('__attribute__((aligned(16))) unsigned char %s[%d] __asm__("%s") = {%s};\n'
                            % (c_ident(i), n, label(asm), c_bytes(init)))
                else:
                    f.write('__attribute__((aligned(16))) unsigned char %s[%d] __asm__("%s");\n' % (c_ident(i), n, label(asm)))
            else:
                f.write('long %s(void) __asm__("%s");\n' % (c_ident(i), label(asm)))
                f.write('long %s(void) { static char seen; k2_stub_hit("%s", "%s", &seen); return 0; }\n'
                        % (c_ident(i), cat, esc))
        if fixups:
            f.write('\n// Pointer slots in the lifted data: patched before main().\n')
            for _, _, what in fixups:
                if what[0] == 'sym' and what[1] not in refs:
                    refs[what[1]] = 'k2ref_%d' % len(refs)
                    f.write('extern char %s[] __asm__("%s");\n' % (refs[what[1]], label(what[1])))
                elif what[0] == 'str' and what[1] not in strs:
                    strs[what[1]] = 'k2str_%d' % len(strs)
                    f.write('static const char %s[] = %s;\n' % (strs[what[1]], c_string(what[1])))
            f.write('__attribute__((constructor)) static void k2_data_fixups(void)\n{\n')
            for ident, off, what in fixups:
                if what[0] == 'sym':
                    f.write('    *(const void**)(%s + %d) = %s + %d;\n' % (ident, off, refs[what[1]], what[2]))
                else:
                    f.write('    *(const void**)(%s + %d) = %s;\n' % (ident, off, strs[what[1]]))
            f.write('}\n')
    return notes_all


def write_aliases(path, aliases):
    """aliases: list of (macho_name as referenced, macho_name of the real definition)."""
    with open(path, 'w', encoding='utf-8') as f:
        f.write('// GENERATED by platforms/tools/linkreport.py -- do not edit.\n')
        f.write('// Bare (no leading underscore) Mach-O names minted by engine `asm("name")` aliases,\n')
        f.write('// forwarded to the real C symbol with a tail call.\n')
        f.write('    .text\n')
        for src, dst in sorted(aliases):
            f.write('    .globl %s\n    .p2align 2\n%s:\n' % (src, src))
            f.write('#if defined(__aarch64__)\n    b %s\n#else\n    jmp %s\n#endif\n' % (dst, dst))


# --- classification ------------------------------------------------------------------------------
def elf_name(macho):
    """Mach-O symbol -> the ELF name the NDK build would have used, plus 'was this an asm label'.

    The C/C++ compilers prepend '_' on Mach-O, so '__ZN4Foo3barEv' -> '_ZN4Foo3barEv' and '_strlen'
    -> 'strlen'. An engine `asm("name")` label is emitted VERBATIM, so it arrives without that extra
    underscore: 'memset' (asm("memset")) or '_ZN8GLRender...' (asm("_ZN8GLRender...")). Those are
    the ELF names as spelled, and on Mach-O they no longer meet their definition."""
    if macho.startswith('__'):
        return macho[1:], False
    if re.match(r'^_Z[A-Z0-9_]', macho):
        return macho, True
    if macho.startswith('_'):
        return macho[1:], False
    return macho, True


# A -fsanitize build's compiler-inserted runtime calls (the runtime dylib is not in the SDK .tbds).
SANITIZER_RT = ('__asan', '__ubsan', '__sanitizer', '__tsan')
SHIM_CPP = re.compile(r'^(ASL|Ios|IOS_|ios|Android|RevealAndroid|DOS2MacPath|MSG_Mac|ScaleNativeResolution)')
SHADER_DATA = re.compile(r'(VP|VPARB|VPDeclaration|FragmentShaderID|ShaderID)$')


def shim_group(elf, dem, typ):
    """Aspyr's port layer lives INSIDE the original libkotor2.so. Pick it out of the 'engine' set."""
    if dem.startswith('ASLgl::'):
        return 'ASLgl -- Aspyr GL wrapper (GL 1.x/ARB surface -> GLES)'
    if elf.startswith('wgl'):
        return 'WGL emulation'
    if typ == 'FUNC' and not elf.startswith('_Z'):          # C linkage: the Win32/MSVC-CRT shim
        if elf.startswith('_') or 'wincompatible' in elf:
            return 'MSVC CRT emulation'
        if elf.startswith('ASL'):
            return 'ASL platform layer'
        return 'Win32 API emulation'
    if SHIM_CPP.match(dem) or SHIM_CPP.match(elf):
        return 'ASL platform layer / iOS+Android glue'
    return None


def engine_group(elf, dem, typ):
    if typ == 'FUNC':
        return 'functions'
    if dem.startswith(('vtable for', 'typeinfo')):
        return 'vtables / RTTI (key function not transcribed)'
    if SHADER_DATA.search(elf):
        return 'renderer shader program text / IDs'
    return 'globals, cvars, tables'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--archive', required=True)
    ap.add_argument('--extra-objects', nargs='*', default=[], help='platform-layer objects/archives')
    ap.add_argument('--orig-so', required=True)
    ap.add_argument('--apk-libdir', required=True)
    ap.add_argument('--ndk-libdir', default='')
    ap.add_argument('--mac-oracle', default='', help='symbolized macOS build (KOTOR2sub)')
    ap.add_argument('--host-dylib', action='append', default=[])
    ap.add_argument('--out', required=True)
    ap.add_argument('--data-homes', default='', help='docs/DATA_HOMES.md: addresses of unexported engine data')
    ap.add_argument('--format', choices=['macho', 'elf', 'coff'], default='macho', help='object format of the build')
    ap.add_argument('--llvm-bin', default='', help='where llvm-nm / llvm-objdump / llvm-readelf are (default: Homebrew\'s llvm)')
    ap.add_argument('--sdk', default='', help='macho: the Apple SDK the build links against (default: xcrun\'s macOS SDK)')
    ap.add_argument('--gl-tbd', default='System/Library/Frameworks/OpenGL.framework/OpenGL.tbd',
                    help='macho: the GL framework stub, relative to the SDK (iOS: OpenGLES.framework)')
    a = ap.parse_args()
    global ELF, COFF, LLVM_BIN
    ELF = a.format in ('elf', 'coff')
    COFF = a.format == 'coff'
    if a.llvm_bin:
        LLVM_BIN = a.llvm_bin
    os.makedirs(a.out, exist_ok=True)

    # A static-library rule deletes the archive and then appends every object (`ar qc`). Two builds
    # of one output directory at once both append, and the archive holds every object twice: each
    # symbol then reads as a duplicate definition here, and the link can take a stale copy. Refuse it.
    members = run([tool('llvm-ar') if ELF else 'ar', 't', a.archive]).split()
    twice = sorted(n for n, c in collections.Counter(members).items() if c > 1)
    if len(twice) > len(members) // 4:
        sys.stderr.write('linkreport: %s holds %d of its %d objects more than once (two builds of this '
                         'directory at once?): delete it and build again\n' % (a.archive, len(twice), len(members)))
        sys.exit(1)

    defs, undefs = scan_archive(a.archive)
    plat_defs = set()
    for extra in a.extra_objects:
        if os.path.exists(extra):
            d, u = scan_archive(extra)
            plat_defs |= set(d)
            for n, objs in u.items():
                undefs[n] |= {'[platform] ' + o for o in objs}
    provided = sorted(n for n in undefs if n not in defs and n in plat_defs)
    unresolved = sorted(n for n in undefs if n not in defs and n not in plat_defs)

    orig_def, orig_und = dynsyms(a.orig_so)
    prov = provider_index([a.apk_libdir, a.ndk_libdir], a.orig_so)
    host, host_per_lib = host_symbols(a.host_dylib, a.sdk or None, a.gl_tbd)
    host_macho = {'_' + h for h in host}
    mac_def, mac_und = set(), set()
    if a.mac_oracle and os.path.exists(a.mac_oracle):
        for line in run(['nm', '-arch', 'x86_64', a.mac_oracle]).splitlines():
            f = line.split()
            if len(f) == 3 and f[2].startswith('_'):
                mac_def.add(f[2][1:])
            elif len(f) == 2 and f[0] == 'U':
                mac_und.add(f[1][1:])

    elf = {n: elf_name(n) for n in unresolved + provided}
    dem = demangle_all({elf[n][0] for n in elf})
    D = lambda n: dem.get(elf[n][0], elf[n][0])

    classes, aliases, note = {}, [], {}
    ndk = {}   # host name -> the NDK spelling, when our only difference is libc++'s inline namespace
    for n in unresolved + provided:
        e0 = elf[n][0]
        if 'St3__1' in e0:
            e1 = e0.replace('St3__1', 'St6__ndk1')
            if e1 in orig_def or e1 in orig_und or e1 in prov:
                ndk[n] = e1
    for n in unresolved + provided:
        e, asm = elf[n]
        if not asm and ('_' + e in host_macho or e in host or e == '__dso_handle'
                        or e.startswith(SANITIZER_RT)
                        or e.startswith('objc_msgSend$')):   # host spelling first (libc++ __1);
            # objc_msgSend$<selector>: the selector stubs ld64 synthesizes for Objective-C calls
            classes[n] = ('host', None)
            continue
        e = ndk.get(n, e)
        if asm:
            real = '_' + e                                     # where the definition really is
            if real in defs or real in plat_defs or real in host_macho:
                classes[n] = ('alias', real)
                aliases.append((n, real))
                continue
        typ = orig_def.get(e, (None, 0))[0]
        if '_' + e in host_macho or e in host:
            classes[n] = ('host', None)
        elif e in orig_def:
            g = shim_group(e, D(n), typ)
            classes[n] = ('shim', g) if g else ('engine', engine_group(e, D(n), typ))
        elif e in orig_und or e in prov:
            classes[n] = ('platform', provider_label(prov.get(e, '?')))
        elif e in mac_und:
            classes[n] = ('platform', 'Bink video (macOS build: libBinkMacx64)' if e.startswith('Bink')
                          else 'macOS-build import')
        elif e in mac_def:
            classes[n] = ('unknown', 'internal to the original (defined in the macOS build, not exported on Android)')
        else:
            classes[n] = ('unknown', 'defined nowhere (declaration/mangling disagrees with the binary?)')


    # For "defined nowhere": is the same method (name + parameters) defined on another class?
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
            return None, None
        return head[:cut], head[cut + 2:] + '(' + params
    orig_dem = demangle_all(orig_def)
    member_idx = collections.defaultdict(set)
    for on, od in orig_dem.items():
        cls, mem = split_member(od)
        if cls:
            member_idx[mem].add(cls)
    for n, (c, sub) in list(classes.items()):
        if c == 'unknown' and sub.startswith('defined nowhere'):
            cls, mem = split_member(D(n))
            others = sorted(member_idx.get(mem, set()) - {cls}) if cls else []
            if others:
                classes[n] = ('unknown', 'defined nowhere under this class -- the .so has it on another class')
                note[n] = others

    dups = {n: [o for o, w in lst if not w] for n, lst in defs.items()}
    dups = {n: o for n, o in dups.items() if len(o) > 1}

    prov_classes = {n: classes.pop(n) for n in provided}
    by_cat = collections.defaultdict(list)
    for n, (cat, sub) in classes.items():
        by_cat[cat].append(n)

    def users(n, k=4):
        u = sorted({os.path.splitext(o)[0].replace('.cpp', '').replace('[platform] ', '[p]') for o in undefs[n]})
        return ', '.join(u[:k]) + (' ...' if len(u) > k else '')

    size = lambda n: orig_def.get(elf[n][0], (None, 0))[1]
    lines = []
    w = lines.append
    w('# KOTOR II host link report\n')
    w('Generated by `platforms/tools/linkreport.py`. Every symbol the engine objects reference but do')
    w('not define, classified by who provides it in the shipping Android build and whether this host')
    w('can. `platforms/out/<preset>/report/linkreport.json` has the per-symbol data.\n')
    w('| class | meaning | symbols |')
    w('|---|---|---|')
    w('| engine objects | TUs in the archive | %d |' % len({o for l in defs.values() for o, _ in l}))
    w('| defined | external symbols our objects define | %d |' % len(defs))
    w('| unresolved | referenced, not defined by the engine | %d |' % len(unresolved))
    w('| provided | answered by the platforms/ layer (null backends, compat) | %d |' % len(provided))
    rows = [('host', 'libSystem / libc++ / OpenGL.framework / SDL2 (sdl2-compat) satisfy it'),
            ('engine', 'game code in the original .so, not transcribed yet'),
            ('shim', "Aspyr's port layer inside the original .so (ASL, Win32/WGL/MSVC-CRT emulation)"),
            ('platform', 'a third-party library the original links (FMOD, SDL2, Bink, ...)'),
            ('alias', 'engine asm("...") label; forwarded to the real Mach-O name'),
            ('unknown', 'no oracle defines it under this name')]
    for c, m in rows:
        w('| &nbsp;&nbsp;**%s** | %s | %d |' % (c, m, len(by_cat[c])))
    w('| duplicate strong definitions | the linker rejects these | %d |\n' % len(dups))

    def grouped(cat, title, blurb, sized=False, listing=True, limit=None):
        w('## %s\n' % title)
        if blurb:
            w(blurb + '\n')
        groups = collections.defaultdict(list)
        for n in by_cat[cat]:
            groups[classes[n][1]].append(n)
        for g in sorted(groups, key=lambda g: (-len(groups[g]), g)):
            ns = sorted(groups[g], key=D)
            extra = ', %d bytes in the original' % sum(size(n) for n in ns) if sized else ''
            w('### %s (%d%s)\n' % (g, len(ns), extra))
            if not listing:
                continue
            w('```')
            shown = ns if limit is None else ns[:limit]
            for n in shown:
                w('%-58s %s' % (D(n), users(n)))
            if len(ns) > len(shown):
                w('... %d more (see linkreport.json)' % (len(ns) - len(shown)))
            w('```\n')

    w('## host -- satisfied by this platform\n')
    w('The graphics and windowing API the transcribed code calls DIRECTLY (everything else it needs')
    w('from the host is libc / libm / libc++):\n')
    for lib in sorted(host_per_lib):
        ns = sorted(D(n) for n in by_cat['host'] if elf[n][0] in host_per_lib[lib])
        if ns:
            w('* **%s** (%d): %s' % (lib, len(ns), ', '.join('`%s`' % x for x in ns)))
    w('')

    w('## provided by platforms/ -- answered by the host platform layer\n')
    w('What the null backends and compat shims in `platforms/` stand in for. Each group is an')
    w('interface a real port replaces with a working implementation.\n')
    pg = collections.defaultdict(list)
    for n, (c, sub) in prov_classes.items():
        pg['%s: %s' % (c, sub) if sub else c].append(n)
    for g in sorted(pg, key=lambda g: (-len(pg[g]), g)):
        w('### %s (%d)\n' % (g, len(pg[g])))
        w('```')
        for n in sorted(pg[g], key=D):
            w('%-58s %s' % (D(n), users(n)))
        w('```\n')

    grouped('platform', 'platform -- third-party libraries the original links, still unanswered',
            'Stub or reimplement on the host. (The GL calls do not appear here: every GL entry point the '
            'transcribed code names resolves against OpenGL.framework, and most GL traffic goes through '
            "Aspyr's ASLgl wrapper -- see **shim**.)")
    grouped('shim', "shim -- Aspyr's port layer (inside the original .so)",
            'The Android build did NOT link Win32/WGL/MSVC-CRT: Aspyr emulated them inside libkotor2.so, and '
            'the engine calls them by their Windows names. A desktop port reimplements this layer against '
            'SDL/GL/POSIX (or reuses the macOS build\'s equivalents), so it is platform work, not transcription.',
            sized=True)

    w('## engine -- game code not transcribed yet\n')
    owners = collections.defaultdict(list)
    for n in by_cat['engine']:
        owners[owner_of(D(n))].append(n)
    w('%d symbols, %d bytes in the original .so.\n' % (len(by_cat['engine']), sum(size(n) for n in by_cat['engine'])))
    for g in ('functions', 'vtables / RTTI (key function not transcribed)',
              'renderer shader program text / IDs', 'globals, cvars, tables'):
        ns = [n for n in by_cat['engine'] if classes[n][1] == g]
        w('* **%s**: %d symbols, %d bytes' % (g, len(ns), sum(size(n) for n in ns)))
    w('\n| owner | symbols | bytes (orig) | functions | data |')
    w('|---|---|---|---|---|')
    for o in sorted(owners, key=lambda o: -sum(size(n) for n in owners[o])):
        ns = owners[o]
        fn = sum(1 for n in ns if classes[n][1] == 'functions')
        w('| %s | %d | %d | %d | %d |' % (o, len(ns), sum(size(n) for n in ns), fn, len(ns) - fn))
    w('\n### engine functions\n')
    w('```')
    for n in sorted((n for n in by_cat['engine'] if classes[n][1] == 'functions'), key=lambda n: -size(n)):
        w('%6d  %-58s %s' % (size(n), D(n), users(n, 3)))
    w('```\n')

    w('## unknown -- no oracle defines it under this name\n')
    w('"internal" rows exist in the original but are not exported by the Android .so (anonymous')
    w('namespace, static, or hidden). "nowhere" rows are usually a declaration whose mangling disagrees')
    w('with the binary -- a wrong parameter type or const, a method declared on the wrong class, or')
    w('an override our class model declares that the original class never had (these surface through')
    w('our vtables). Each one is a class-model question worth a look.\n')
    ow = collections.defaultdict(list)
    for n in by_cat['unknown']:
        ow[(classes[n][1], owner_of(D(n)))].append(n)
    for sub in sorted({k[0] for k in ow}):
        w('### %s\n' % sub)
        w('```')
        for key in sorted((k for k in ow if k[0] == sub), key=lambda k: (-len(ow[k]), k[1])):
            for n in sorted(ow[key], key=D):
                extra = ('   [.so: %s]' % ', '.join(note[n][:3])) if n in note else ''
                w('%-66s %s%s' % (D(n), users(n, 3), extra))
        w('```\n')

    if aliases:
        w('## alias -- asm-label names, forwarded\n')
        w('```')
        for src, dst in sorted(aliases):
            w('%s -> %s' % (src, dst))
        w('```\n')

    w('## duplicate strong definitions\n')
    if dups:
        w('```')
        for n in sorted(dups, key=lambda n: dem.get(elf_name(n)[0], n)):
            e = elf_name(n)[0]
            w('%s\n    %s' % (demangle_all([e]).get(e, e), ', '.join(dups[n])))
        w('```')
    else:
        w('None.')
    open(os.path.join(a.out, 'linkreport.md'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')

    json.dump({elf[n][0]: {'macho': n, 'class': c, 'group': s, 'demangled': D(n), 'on_class': note.get(n),
                           'users': sorted(undefs[n]), 'orig': orig_def.get(elf[n][0])}
               for n, (c, s) in classes.items()},
              open(os.path.join(a.out, 'linkreport.json'), 'w', encoding='utf-8'), indent=1, sort_keys=True)

    branched = branch_targets([a.archive] + a.extra_objects)
    entries = []
    for n in unresolved:
        cat, sub = classes[n]
        if cat in ('host', 'alias'):
            continue
        e = elf[n][0]
        typ, sz = orig_def.get(e, (None, 0))
        if typ == 'OBJECT' or e.startswith(DATA_PREFIXES):
            kind = 'data'
        elif typ == 'FUNC':
            kind = 'func'
        elif '(' in D(n):   # a demangled parameter list: a function (maybe only vtable-reached)
            kind = 'func'
        else:   # an unmangled name unknown to the .so: a function only if something branches to it
            kind = 'func' if n in branched else 'data'
        entries.append((n, kind, sz, cat + (':' + sub if sub else ''), D(n)))
    from elfimage import ElfImage
    img = ElfImage(a.orig_so)
    HOMES.update(data_homes(a.data_homes))
    HOME_ADDRS.extend(sorted(v[0] for v in HOMES.values()))
    known = set(defs) | set(plat_defs) | host_macho | {e[0] for e in entries}
    notes = write_stubs(os.path.join(a.out, 'k2stubs.gen.c'), entries, img, known)
    with open(os.path.join(a.out, 'k2data_unresolved.txt'), 'w', encoding='utf-8') as f:
        f.write('# pointer slots in lifted engine data that stay NULL on this host\n')
        f.write('\n'.join(notes) + '\n')
    write_aliases(os.path.join(a.out, 'k2alias.gen.S'), aliases)
    open(os.path.join(a.out, 'k2dups.txt'), 'w', encoding='utf-8').write('\n'.join(sorted(dups)) + '\n')

    print('linkreport: %d unresolved -> ' % len(unresolved) +
          ', '.join('%s %d' % (c, len(by_cat[c])) for c, _ in rows) + '; %d duplicate defs' % len(dups))

    # Every engine data object is defined in src/: a new one is a declaration without a definition,
    # and its stub would be a silent zero, so the link stops here. K2_ALLOW_DATA_STUBS=1 lets an
    # exploratory build through.
    missing = [(n, D) for n, kind, sz, cat, D in entries
               if kind == 'data' and cat.split(':')[0] in ('engine', 'unknown')]
    if missing and os.environ.get('K2_ALLOW_DATA_STUBS') != '1':
        sys.stderr.write('linkreport: %d engine data object(s) declared but defined nowhere in src/ '
                         '(define them: tools/datadefs.py):\n' % len(missing))
        for n, d in missing[:40]:
            sys.stderr.write('    %s\n' % d)
        sys.exit(1)

    # The engine defines everything once. ld64 and Android's lld reject a duplicate themselves; Linux
    # links with --allow-multiple-definition for its platform-side duplicates, so an engine one stops
    # here.
    # COFF is not checked: its inline functions and vtables are COMDAT copies this scan cannot tell
    # from strong definitions. K2_ALLOW_DUPLICATES=1 lets an exploratory build through.
    if dups and not COFF and os.environ.get('K2_ALLOW_DUPLICATES') != '1':
        sys.stderr.write('linkreport: %d engine symbol(s) defined in more than one TU:\n' % len(dups))
        for n in sorted(dups)[:40]:
            sys.stderr.write('    %s    %s\n' % (n, ', '.join(dups[n])))
        sys.exit(1)


if __name__ == '__main__':
    main()
