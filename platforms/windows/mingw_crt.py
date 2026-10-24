#!/usr/bin/env python3
"""mingw_crt.py -- copies of mingw-w64's CRT headers as the engine expects to see them.

Written at configure time from the toolchain's own headers (CMakeLists.txt, WIN32), into a directory
searched after libc++'s and before the sysroot's, so libc++'s wrappers #include_next them. Three
edits, each settling a clash the engine's hand-written CRT prototypes have only on this CRT:

  * C++ const overloads. In C++ mingw-w64 declares strchr / strstr / memchr / ... as an extern "C++"
    const/non-const pair. The engine TUs declare the plain C prototypes themselves (`extern "C"
    char* strstr(const char*, const char*)`, as bionic and Darwin spell them), which the pair turns
    into "differ only in their return type" -- the clash compat/include-glibc/string.h settles on
    Linux. Each block

        #ifdef __cplusplus
        extern "C++" { <the const pair> }
        #else
          <the C prototypes>
        #endif

    becomes the C prototypes with C linkage.
  * <io.h> (which <sys/stat.h> includes) loses its #include <string.h>: bionic's and Darwin's
    <sys/stat.h> declare no string functions, and the TUs that include it declare their own.
  * MSVC CRT names the port's shim answers itself (shim/shim_crt.cpp) with the signatures the engine
    declares, e.g. `_strnicmp(const char*, const char*, unsigned int)`, a C++ `sprintf_s(char*,
    unsigned long, ...)` and KOTOR 1's C++ `_strlwr` / `_strupr` / `_itoa` / `_strdate`, the old one-argument mkdir where POSIX's takes a mode, and `_endthreadex` / `_endthread`,
    which the engine's ThreadHANDLE calls as the shim's: the CRT's own declarations are renamed out
    of the way.
"""
import argparse, os, re

OVERLOADS = ('string.h', 'corecrt_memory.h', 'corecrt_wstring.h', 'wchar.h')
NO_STRING_H = ('io.h',)
RENAMES = {
    # The rest are src/port/crtapi.h's: the engine's one declaration of each (nounwind, and KOTOR 1's
    # _strlwr / _strupr / _itoa with C++ linkage), which the CRT's own may not follow.
    'string.h': ('_strnicmp', '_stricmp', '_strdup', '_strlwr', '_strupr'),
    'stdio.h': ('sprintf_s', '_vsnprintf'),
    'stdlib.h': ('_itoa',),
    'math.h': ('_finite',),
    'time.h': ('_strdate',),
    'float.h': ('_finite',),
    # The old one-argument mkdir: the shim's is POSIX's (compat/include-mingw/sys/stat.h).
    'io.h': ('mkdir',),
    'direct.h': ('mkdir',),
    # winpthreads' <pthread.h> includes <process.h>; the engine's thread exit is the shim's
    # (src/port/win32api.h), whose namespace would otherwise make the call ambiguous.
    'process.h': ('_endthreadex', '_endthread'),
}
# wchar.h's inline wmemchr, the C++ const overload again (its C prototype is what BLOCK leaves).
INLINE_WMEMCHR = re.compile(r'#ifdef __cplusplus\n\s*extern "C\+\+"\n#endif\n\s*__CRT_INLINE _CONST_RETURN wchar_t \*__cdecl wmemchr\(.*?\n  \}\n', re.S)
BLOCK = re.compile(r'#ifdef __cplusplus\n\s*extern "C\+\+" \{\n.*?\n\s*\}\n#else\n(.*?)\n#endif', re.S)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sysroot-include', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--ucrt', action='store_true',
                    help="the MSVC UCRT's own headers (the MSVC builds): only the renames; the const overloads "
                         "go with -D_CONST_RETURN= (the C prototypes then return non-const, as the engine's)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    if a.ucrt:
        ucrt_stat(a)
    for h in sorted(set(RENAMES) if a.ucrt else set(OVERLOADS) | set(NO_STRING_H) | set(RENAMES)):
        if a.ucrt and not os.path.exists(os.path.join(a.sysroot_include, h)):
            continue
        text = open(os.path.join(a.sysroot_include, h), encoding='utf-8', errors='surrogateescape').read()
        what = []
        if h in OVERLOADS and not a.ucrt:
            text, n = BLOCK.subn(lambda m: '#ifdef __cplusplus\nextern "C" {\n#endif\n%s\n#ifdef __cplusplus\n}\n#endif'
                                 % m.group(1), text)
            if not n:
                raise SystemExit('mingw_crt.py: no C++ overload block in %s' % h)
            if h == 'wchar.h':
                text, n = INLINE_WMEMCHR.subn('', text)
                if not n:
                    raise SystemExit('mingw_crt.py: no inline wmemchr in wchar.h')
            what.append('the C++ const overloads replaced by the C prototypes')
        if h in NO_STRING_H and not a.ucrt:
            text, n = re.subn(r'^#include <string\.h>\n', '', text, flags=re.M)
            if not n:
                raise SystemExit('mingw_crt.py: %s no longer includes <string.h>' % h)
            what.append('without its #include <string.h>')
        if h in RENAMES and RENAMES[h]:
            names = RENAMES[h]
            if a.ucrt:   # the UCRT declares these exactly as the engine does
                names = tuple(n for n in names if re.search(r'\b%s\b' % n, text) and n not in ('_stricmp', '_strdup'))
            for name in names:
                if not re.search(r'\b%s\b' % name, text):
                    raise SystemExit('mingw_crt.py: %s no longer declares %s' % (h, name))
            # A name the header #undefs around its own declaration (string.h's _strdup) is renamed in
            # the text; the others by a macro around the whole header.
            spelled = [n for n in names if re.search(r'#undef %s\b' % n, text)]
            for n in spelled:
                text = re.sub(r'\b%s\b' % n, '__k2crt' + n, text)
            names = [n for n in names if n not in spelled]
            head = ''.join('#pragma push_macro("%s")\n#define %s __k2crt%s\n' % (n, n, n) for n in names)
            tail = ''.join('\n#pragma pop_macro("%s")' % n for n in names)
            if a.ucrt and names and h not in UCRT_ALWAYS:
                # Only the engine's TUs declare these themselves; the platform code keeps the CRT's.
                # sprintf_s only where an engine TU defines its own (the MSVC STL's <string> calls
                # ::sprintf_s): CMakeLists.txt's K2_ENGINE_SPRINTF_S list.
                head = ''.join('#if defined %s\n#pragma push_macro("%s")\n#define %s __k2crt%s\n#endif\n'
                               % (UCRT_GUARD.get(n, 'K2_ENGINE_CRT'), n, n, n) for n in names)
                tail = ''.join('\n#if defined %s\n#pragma pop_macro("%s")\n#endif' % (UCRT_GUARD.get(n, 'K2_ENGINE_CRT'), n)
                               for n in names)
            text = head + text + tail + '\n'
            names = names + spelled
            what.append('%s renamed' % ', '.join(names))
        write(os.path.join(a.out, h), '/* GENERATED by platforms/windows/mingw_crt.py from the toolchain\'s %s: %s. */\n'
              % (h, '; '.join(what)) + text)


# The UCRT names the shim answers for every TU, not just the engine's (the POSIX mkdir, the thread exit).
UCRT_ALWAYS = ('direct.h', 'process.h')
UCRT_GUARD = {'sprintf_s': 'K2_ENGINE_SPRINTF_S'}
# The guard line differs by SDK (10.0.22621: `!__STDC__`; 10.0.26100: `_CRT_INTERNAL_NONSTDC_NAMES`); it is kept.
UCRT_STAT_BLOCK = re.compile(r'(#if !defined RC_INVOKED && !defined __midl && [^\n]*!defined _CRT_NO_TIME_T\n)'
                             r'\s*#ifdef _USE_32BIT_TIME_T\n.*?\n#endif\n', re.S)


def ucrt_stat(a):
    """sys/stat.h: the UCRT defines POSIX's stat / fstat as static inline wrappers of _stat64i32; the shim
    defines them itself (shim/shim_stdio.cpp, over windows/win_dlfcn.cpp's real calls), so the copy
    declares them instead."""
    src = os.path.join(a.sysroot_include, 'sys', 'stat.h')
    text = open(src, encoding='utf-8', errors='surrogateescape').read()
    text, n = UCRT_STAT_BLOCK.subn(r'\1'
                                   'int __cdecl fstat(int _FileHandle, struct stat* _Stat);\n'
                                   'int __cdecl stat(char const* _FileName, struct stat* _Stat);\n#endif\n', text)
    if not n:
        raise SystemExit('mingw_crt.py: no inline stat / fstat block in the UCRT sys/stat.h')
    os.makedirs(os.path.join(a.out, 'sys'), exist_ok=True)
    write(os.path.join(a.out, 'sys', 'stat.h'), '/* GENERATED by platforms/windows/mingw_crt.py from the UCRT sys/stat.h: '
          'stat / fstat declared, not inline. */\n' + text)


def write(path, text):
    if not os.path.exists(path) or open(path, encoding='utf-8', errors='surrogateescape').read() != text:
        open(path, 'w', encoding='utf-8', errors='surrogateescape').write(text)


if __name__ == '__main__':
    main()
