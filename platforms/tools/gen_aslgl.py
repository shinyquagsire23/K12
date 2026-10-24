#!/usr/bin/env python3
"""Generate platforms/shim/aslgl.gen.{h,cpp}: the ASLgl:: GL wrappers the engine calls, as forwarders.

On Android, Aspyr's ASLgl namespace translates the engine's GL 1.x/ARB calls down to GLES 2. A
desktop GL 2.1 compatibility context has every one of those entry points natively, so each wrapper
forwards to the real function, loaded once through SDL_GL_GetProcAddress (the name as written, then
its EXT/ARB/core spellings). A function the driver lacks logs once and does nothing.

The signatures come from the engine archive's own UNDEFINED ASLgl references (`nm -u`, demangled),
i.e. exactly what the transcribed engine calls, whether or not this file already defines them (the
link report can't be the source: once this file exists, it no longer lists them). Return types are
not in a mangled name, so the few non-void ones are in RETURNS below; the engine's own per-TU
declarations agree with them.

Every function is emitted twice: `k2gl::name`, the native forwarder (declared in aslgl.gen.h for the
rest of the shim), and `ASLgl::name`, the engine-facing entry point that just calls it. The names in
OVERRIDDEN get no `ASLgl::` wrapper here because shim_kotorgl.cpp defines them: that is Aspyr's own
GL-state layer (fragment-program ids -> GLSL KotorProgram, fog/alpha/light shadows), which on
Android sits between the engine and GL too. EXTRA lists natives only the shim itself calls.

    python3 platforms/tools/gen_aslgl.py [platforms/out/k2-host-pc/libk2_engine.a]
"""
import subprocess
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(REPO, 'platforms', 'shim', 'aslgl.gen.cpp')
OUT_H = os.path.join(REPO, 'platforms', 'shim', 'aslgl.gen.h')

RETURNS = {
    'glGetError': 'unsigned int',
    'glGenLists': 'unsigned int',
    'glGetString': 'unsigned char const*',
    'glIsEnabled': 'unsigned char',
    'glIsTexture': 'unsigned char',
    'glIsProgramARB': 'unsigned char',
    'glCreateShader': 'unsigned int',
    'glCreateProgram': 'unsigned int',
    'glGetAttribLocation': 'int',
    'glGetUniformLocation': 'int',
    'glCheckFramebufferStatus': 'unsigned int',
}

# The shim's hand-written replacements for Aspyr's ASLgl state layer.
OVERRIDDEN = {
    'glAlphaFunc', 'glBegin', 'glBindProgramARB', 'glDeleteProgramsARB', 'glDisable',
    'glDisableClientState', 'glDisableVertexAttribArrayARB', 'glDrawArrays', 'glDrawElements',
    'glEnable', 'glEnableClientState', 'glEnableVertexAttribArrayARB', 'glFogf', 'glFogfv',
    'glGenProgramsARB', 'glGetString', 'glPopClientAttrib', 'glProgramEnvParameter4fARB',
    'glProgramEnvParameter4fvARB', 'glProgramStringARB', 'glPushClientAttrib', 'glShaderSource',
    'glVertexAttribPointerARB',
}

# Empty in Aspyr's ASLgl, so the shipped game never saved or restored GL attribute state, and the
# engine relies on that: RenderSkinnedVP's non-hologram bumpy-shiny pass pushes and never pops.
# Forwarded, the leaked pushes grow gl4es's unbounded stack (a realloc copy per push), and against a
# capped stack the balanced pairs pop stale snapshots once it fills. The ASLgl:: entry points do
# nothing; the k2gl:: natives remain. glPushClientAttrib / glPopClientAttrib are empty there too, but
# their only callers (the two bump passes) pair them and re-enable the normal and colour arrays just
# before the pop, which left the colour array enabled on a stale pointer; shim_kotorgl.cpp keeps
# that stack.
NOOP = {'glPopAttrib', 'glPushAttrib'}

# The client-array shadow: state hooks run after the native call, draw hooks before it. RA is the
# forwarder's return address, i.e. the engine caller of an ASLgl:: wrapper.
RA = '__builtin_return_address(0)'
ARRAY_HOOKS = {
    'glEnableClientState': (None, 'k2shim_glarr_enable(a0, true);'),
    'glDisableClientState': (None, 'k2shim_glarr_enable(a0, false);'),
    'glClientActiveTexture': (None, 'k2shim_glarr_client_active(a0);'),
    'glClientActiveTextureARB': (None, 'k2shim_glarr_client_active(a0);'),
    'glEnableVertexAttribArray': (None, 'k2shim_glarr_attrib_enable(a0, true);'),
    'glEnableVertexAttribArrayARB': (None, 'k2shim_glarr_attrib_enable(a0, true);'),
    'glDisableVertexAttribArray': (None, 'k2shim_glarr_attrib_enable(a0, false);'),
    'glDisableVertexAttribArrayARB': (None, 'k2shim_glarr_attrib_enable(a0, false);'),
    'glBindBuffer': (None, 'k2shim_glarr_bind(a0, a1);'),
    'glBindBufferARB': (None, 'k2shim_glarr_bind(a0, a1);'),
    'glVertexPointer': (None, f'k2shim_glarr_pointer(0x8074, a0, a1, a2, a3, {RA});'),
    'glNormalPointer': (None, f'k2shim_glarr_pointer(0x8075, 3, a0, a1, a2, {RA});'),
    'glColorPointer': (None, f'k2shim_glarr_pointer(0x8076, a0, a1, a2, a3, {RA});'),
    'glTexCoordPointer': (None, f'k2shim_glarr_pointer(0x8078, a0, a1, a2, a3, {RA});'),
    'glVertexAttribPointer': (None, f'k2shim_glarr_attrib_pointer(a0, a1, a2, a4, a5, {RA});'),
    'glVertexAttribPointerARB': (None, f'k2shim_glarr_attrib_pointer(a0, a1, a2, a4, a5, {RA});'),
    'glPushClientAttrib': ('k2shim_glarr_push();', None),
    'glPopClientAttrib': (None, 'k2shim_glarr_pop();'),
    'glDrawArrays': (f'k2shim_glarr_draw("glDrawArrays", a0, a2, {RA});', None),
    'glDrawElements': (f'k2shim_glarr_draw("glDrawElements", a0, a1, {RA});', None),
    'glDrawRangeElements': (f'k2shim_glarr_draw("glDrawRangeElements", a0, a3, {RA});', None),
}

# Natives the shim calls that the engine itself never does: (name, [arg types]).
EXTRA = [
    ('glGetFloatv', ['unsigned int', 'float*']),
    ('glGetTexLevelParameteriv', ['unsigned int', 'int', 'unsigned int', 'int*']),
]


def fixed_width(t):
    """The engine spells its 64-bit GL sizes int64_t (platforms/tools/longscan.py), which demangles as
    `long` or `long long` depending on the host; the generated file names the width either way."""
    t = re.sub(r'\bunsigned long( long)?\b', 'uint64_t', t)
    return re.sub(r'\blong( long)?\b', 'int64_t', t)


def split_args(s):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
            continue
        depth += ch in '(<'
        depth -= ch in ')>'
        cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def main():
    archive = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        REPO, 'platforms', 'out', 'k2-host-pc', 'libk2_engine.a')
    names = subprocess.run(['nm', '-u', archive], capture_output=True, text=True).stdout.split()
    names = sorted({n for n in names if n.startswith('__ZN5ASLgl')})
    dem = subprocess.run(['c++filt', '-_'], input='\n'.join(names), capture_output=True,
                         text=True).stdout.splitlines()
    funcs = []
    for d in dem:
        m = re.match(r'^ASLgl::(\w+)\((.*)\)$', d)
        if m:
            funcs.append((m.group(1), [fixed_width(a) for a in split_args(m.group(2)) if a != 'void']))
    have = {n for n, _ in funcs}
    shim_only = {n for n, _ in EXTRA} - have
    funcs += [(n, a) for n, a in EXTRA if n in shim_only]
    funcs.sort()
    missing = OVERRIDDEN - {n for n, _ in funcs}
    if missing:
        sys.exit(f'OVERRIDDEN names the engine does not call: {sorted(missing)}')

    hdr = [
        '// GENERATED by platforms/tools/gen_aslgl.py -- do not edit. See that script for what and why.',
        '#ifndef ASLGL_GEN_H',
        '#define ASLGL_GEN_H',
        '#include <stdint.h>',
        '',
        '// The native GL entry points, loaded by name from the current context.',
        'namespace k2gl {',
    ]
    lines = [
        '// GENERATED by platforms/tools/gen_aslgl.py -- do not edit. See that script for what and why.',
        '#include "k2shim.h"',
        '#include "aslgl.gen.h"',
        '',
        'namespace k2gl {',
    ]
    wrappers = ['namespace ASLgl {']
    for name, args in funcs:
        ret = RETURNS.get(name, 'void')
        params = ', '.join(f'{t} a{i}' for i, t in enumerate(args))
        types = ', '.join(args) or 'void'
        call = ', '.join(f'a{i}' for i in range(len(args)))
        body = (f'    typedef {ret} (K2_GLAPI* Fn)({types});\n'
                f'    static Fn fn;\n'
                f'    if (!fn) fn = (Fn)k2shim_gl_proc("{name}");   /* retried until a context exists */\n')
        # k2shim_gl_after: K2_GL_CHECK=1 reports the first GL error each entry point raises
        # (a no-op branch otherwise); glGetError itself is excluded so the engine's own reads see it.
        after = '' if name == 'glGetError' else f'    if (k2shim_gl_checking) k2shim_gl_after("{name}");\n'
        if name.startswith('glDeleteBuffers'):
            body += f'    k2shim_gl_unbind_buffers({call});   /* arrays still sourced from them */\n'
        before_hook, after_hook = ARRAY_HOOKS.get(name, (None, None))
        if before_hook:
            body += f'    {before_hook}\n'
        if after_hook:
            after = f'    {after_hook}\n' + after
        if ret == 'void':
            body += f'    if (fn) fn({call});\n' + after
        else:
            body += (f'    {ret} r = fn ? fn({call}) : static_cast<{ret}>(0);\n' + after + '    return r;\n')
        lines.append(f'{ret} {name}({params})\n{{\n{body}}}\n')
        hdr.append(f'{ret} {name}({", ".join(args)});')
        if name not in OVERRIDDEN and name not in shim_only:
            ret_kw = '' if ret == 'void' else 'return '
            if name in NOOP:
                wrappers.append(f'{ret} {name}({params}) {{}}   // empty in Aspyr\'s ASLgl (NOOP)')
            else:
                if RA in ''.join(h or '' for h in ARRAY_HOOKS.get(name, ())):
                    # the shadow records the engine caller, not this wrapper
                    wrappers.append(f'{ret} {name}({params}) {{ k2shim_glarr_caller = {RA}; '
                                    f'{ret_kw}k2gl::{name}({call}); }}')
                else:
                    wrappers.append(f'{ret} {name}({params}) {{ {ret_kw}k2gl::{name}({call}); }}')
    lines.append('} // namespace k2gl\n')
    lines += wrappers
    lines.append('} // namespace ASLgl')
    hdr += ['} // namespace k2gl', '', '#endif']
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    with open(OUT_H, 'w', encoding='utf-8') as f:
        f.write('\n'.join(hdr) + '\n')
    print(f'{OUT}: {len(funcs)} natives, {len(wrappers) - 1} ASLgl wrappers')


if __name__ == '__main__':
    main()
