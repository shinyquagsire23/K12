#!/usr/bin/env python3
"""Generate platforms/k1/k1gl.gen.cpp: KOTOR 1's GL imports, as forwarders (K2P_GAME=k1).

K1's libKOTOR.so does not call GL itself. It calls `android_port_glX`, Aspyr's GL layer in
libandroid_port.so (on the device a fixed-function emulation over GLES 2), plus a few bare GLES
framebuffer entry points and their OES twins. src/port/k1gl.h declares exactly that import set. Here
each import forwards to the real entry point, looked up once through k2shim_gl_proc (a desktop GL 2.1
compatibility context on macOS, gl4es on Android: both have the GL 1.x/ARB surface natively).

The list comes from src/port/k1gl.h's `extern "C"` block, so it follows the tree. Return types and
parameter types are taken from those declarations as written.

  * An import that is a lone `ret` in the shipped libandroid_port.so (4 bytes: glPushAttrib,
    glPopAttrib, the display-list calls, glPolygonMode, glRasterPos2f, ...) is a no-op here too: the
    shipped game never had them, and the engine relies on that (unbalanced attrib pushes). A 4-byte
    export that is a `b` instead forwards to what it tail-calls (glCompressedTexImage2DARB ->
    glCompressedTexImage2D, glFogi -> glFogx, i.e. glFogi here).
  * The ARB program calls are such no-ops as well: the port replaced ARB vertex AND fragment programs
    with its own GLSL set (APK assets kotor.vert / kotor.frag), keyed by the fragment-program id.
    That layer -- glBindProgramARB, the env parameters, the fog / alpha / program capabilities, the
    skinning streams and the draws that upload its uniforms -- is HAND-written in k1/k1kotorgl.cpp.
  * `android_port_glViewportRaw` is the port's untransformed twin of glViewport: plain glViewport.
  * The OES framebuffer twins forward to the core names.
  * HAND: written in k1/k1port.cpp, because the port did more than forward: glViewport /
    glViewportRaw record the viewport and glGetIntegerv answers GL_VIEWPORT from that record. And the
    GLSL renderer layer above, in k1/k1kotorgl.cpp.
  * The engine's `ASLgl::glX` declarations (k1gl.h) carry the import as an asm label, which on Mach-O
    is a BARE name; linkreport's `alias` class forwards those to the `_android_port_glX` defined here.

    python3 platforms/tools/gen_k1gl.py
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_aslgl import ARRAY_HOOKS   # the client-array shadow's hooks (shim/shim_glarrays.cpp)

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HDR = os.path.join(REPO, 'src', 'port', 'k1gl.h')
PORT_SO = os.path.join(REPO, 'decomp_bins', 'swkotor_extracted', 'lib', 'arm64-v8a', 'libandroid_port.so')
OUT = os.path.join(REPO, 'platforms', 'k1', 'k1gl.gen.cpp')

# the GL name an import forwards to, where it is not the import minus `android_port_`
RENAME = {'android_port_glViewportRaw': 'glViewport'}
# a port tail call whose target desktop GL / gl4es spells differently (glFogx is GLES 1's fixed-point
# glFogi: the engine only passes enum values through it)
TAIL_TARGET = {'android_port_glFogx': 'glFogi'}
# forwarded although the port's own does nothing (see the docstring): the client-attribute stack's
# only callers, the bump passes, pair them and switch the normal and colour arrays back on just before
# the pop, which without the stack left the colour array enabled on a stale pointer
FORWARD = {'android_port_glPushClientAttrib', 'android_port_glPopClientAttrib'}
# defined by hand in k1/k1port.cpp
HAND = {'android_port_glViewport', 'android_port_glViewportRaw', 'android_port_glGetIntegerv',
        # k1/k1kotorgl.cpp
        'android_port_glBindProgramARB', 'android_port_glProgramEnvParameter4fARB',
        'android_port_glProgramEnvParameter4fvARB', 'android_port_glEnable', 'android_port_glDisable',
        'android_port_glIsEnabled', 'android_port_glFogf', 'android_port_glFogfv', 'android_port_glAlphaFunc',
        'android_port_glEnableClientState', 'android_port_glDisableClientState',
        'android_port_glVertexAttribPointerARB', 'android_port_glEnableVertexAttribArrayARB',
        'android_port_glDisableVertexAttribArrayARB', 'android_port_glDrawArrays', 'android_port_glDrawElements',
        'android_port_glBegin'}
# the bare GLES imports k1gl.h declares: the host/gl4es already define the core ones
OES = re.compile(r'^gl\w+OES$')


def nm_sizes(path):
    for nm in ('/opt/homebrew/opt/llvm/bin/llvm-nm', 'llvm-nm'):
        try:
            out = subprocess.run([nm, '-D', '-S', '--defined-only', path], check=True,
                                 capture_output=True, text=True).stdout
            break
        except (OSError, subprocess.CalledProcessError):
            continue
    else:
        sys.exit('gen_k1gl: no llvm-nm')
    sizes = {}
    for line in out.splitlines():
        f = line.split()
        if len(f) == 4:
            sizes[f[3]] = (int(f[0], 16), int(f[1], 16))
    return sizes


def one_insn(path, addr):
    """The single instruction of a 4-byte export: 'ret', or the android_port_glX a `b` tail-calls."""
    for od in ('/opt/homebrew/opt/llvm/bin/llvm-objdump', 'llvm-objdump'):
        try:
            out = subprocess.run([od, '-d', '--no-show-raw-insn', f'--start-address={addr:#x}',
                                  f'--stop-address={addr + 4:#x}', path], check=True,
                                 capture_output=True, text=True).stdout
            break
        except (OSError, subprocess.CalledProcessError):
            continue
    else:
        sys.exit('gen_k1gl: no llvm-objdump')
    m = re.search(r'\bb\s+0x[0-9a-f]+ <(android_port_gl\w+)@plt>', out)
    return m.group(1) if m else 'ret'


def decls():
    text = open(HDR, encoding='utf-8').read()
    block = text[text.index('extern "C" {'):]
    block = block[:block.index('\n}\n')]
    out = []
    for m in re.finditer(r'^([\w\s\*]+?)\b(android_port_gl\w+|gl\w+OES)\(([^)]*)\);', block, re.M):
        ret, name, args = m.group(1).strip(), m.group(2), m.group(3).strip()
        out.append((ret, name, [a.strip() for a in args.split(',')] if args else []))
    return out


def main():
    sizes = nm_sizes(PORT_SO)
    w = []
    w.append('// GENERATED by platforms/tools/gen_k1gl.py from src/port/k1gl.h -- do not edit. See that script.')
    w.append('#include "../shim/k2shim.h"')
    w.append('')
    w.append('extern bool k1gl_tracing;                        // k1port.cpp: the one-frame call trace')
    w.append('void k1gl_trace(const char* pszFormat, ...);')
    w.append('')
    w.append('extern "C" {')
    n_noop = n_out = 0
    for ret, name, args in decls():
        if name in HAND:
            continue
        n_out += 1
        params = ', '.join(f'{t} a{i}' for i, t in enumerate(args))
        call = ', '.join(f'a{i}' for i in range(len(args)))
        if OES.match(name):
            target = name[:-3]
        else:
            target = RENAME.get(name, name[len('android_port_'):])
        void = ret == 'void'
        w.append(f'{ret} {name}({params})')
        w.append('{')
        noop = False
        if name in sizes and sizes[name][1] == 4 and name not in FORWARD:
            tail = one_insn(PORT_SO, sizes[name][0])
            if tail == 'ret':
                noop = True
            else:
                target = TAIL_TARGET.get(tail, tail[len('android_port_'):])
        fmts = []
        for t in args:
            fmts.append('%p' if '*' in t else '%g' if ('float' in t or 'double' in t) else '%u' if 'unsigned' in t else '%d')
        tag = target + (' (no-op)' if noop else '')
        targs = ''.join(f', static_cast<double>(a{i})' if ('float' in t or 'double' in t) and '*' not in t else f', a{i}' for i, t in enumerate(args))
        w.append(f'    if (k1gl_tracing) k1gl_trace("{tag}({", ".join(fmts)})"{targs});')
        if noop:
            n_noop += 1
            w.append('    /* a lone `ret` in libandroid_port.so */')
            if not void:
                w.append('    return 0;')
        else:
            w.append(f'    typedef {ret} (K2_GLAPI* Fn)({", ".join(args)});')
            w.append('    static Fn fn;')
            w.append(f'    if (!fn) fn = (Fn)k2shim_gl_proc("{target}");   /* retried until a context exists */')
            if target.startswith('glDeleteBuffers'):
                w.append(f'    k2shim_gl_unbind_buffers({call});   /* arrays still sourced from them */')
            before_hook, after_hook = ARRAY_HOOKS.get(target, (None, None))
            if before_hook:
                w.append(f'    {before_hook}')
            if void:
                w.append(f'    if (fn) fn({call});')
                if after_hook:
                    w.append(f'    {after_hook}')
                w.append(f'    if (k2shim_gl_checking) k2shim_gl_after("{target}");')
            else:
                w.append(f'    return fn ? fn({call}) : 0;')
        w.append('}')
        w.append('')
    w.append('} // extern "C"')
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, 'w', encoding='utf-8').write('\n'.join(w) + '\n')
    print(f'gen_k1gl: {n_out} imports ({n_noop} no-ops, {len(HAND)} by hand in k1port.cpp / k1kotorgl.cpp) -> {os.path.relpath(OUT, REPO)}')


if __name__ == '__main__':
    main()
