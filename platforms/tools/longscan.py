#!/usr/bin/env python3
"""longscan.py -- `long` as a fixed-width integer, for the Win64 build (LLP64: long is 32 bits).

The engine was transcribed from an LP64 binary, so every `long` in it is 64 bits wide, and so is
every `long` in the port layer that talks to it (Aspyr's Win32 shim: LPARAM, SIZE_T, GetWindowLong).
On Win64 `long` is 32 bits, and nothing tells the compiler two separately declared sides disagree
(the shim filled a 56-byte MEMORYSTATUS the engine declared as 32). The fixed-width types say what
the binary has on every target: int64_t / uint64_t are `long` / `unsigned long` on arm64 Android,
so the matching build's bytes and mangled names do not change, and on Win64 they stay 64 bits.

    longscan.py --check  [paths...]   list every bare `long` left (the lint; exit 1 if any)
    longscan.py --rewrite [paths...]  rewrite them in place

Rewritten:   long, long int, signed long  -> int64_t;   unsigned long [int], long unsigned -> uint64_t
Kept:        long long, long double, and a line that declares or calls a C library function whose
             own signature has `long` (strtol, ftell, fseek ... LIBC_LONG, named in the code or as an
             asm label): on Win64 those really are 32 bits. A comment or string literal is never
             touched. tools/lint_idiom.py's class W is the same rule, gating tools/match.py.
A file that gains a fixed-width type gains `#include "k2stdint.h"` (src/) or `<stdint.h>` (platforms/).
Default paths: src/ and platforms/' own code (third_party/, generated GL forwarders excepted).
"""
import argparse, os, re, sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
LIBC_LONG = ('strtol', 'strtoul', 'atol', 'labs', 'ldiv', 'ftell', 'fseek', 'lround', 'lroundf', 'lrint',
             'lrintf', 'wcstol', 'wcstoul', '_lseek', '_tell', 'lseek', 'clock', 'random', 'srandom',
             'sysconf', 'a64l', 'l64a', 'ftello', 'fseeko', 'tv_usec', 'tv_nsec', 'tm_gmtoff', 'ulimit',
             'timespec', 'timeval', 'nanosleep')
LIBC_RE = re.compile(r'\b(%s)\b' % '|'.join(LIBC_LONG))
DEFAULT = ['src', 'platforms/shim', 'platforms/compat', 'platforms/console', 'platforms/qol',
           'platforms/k1', 'platforms/pcui', 'platforms/macos', 'platforms/null', 'platforms/ftgl',
           'platforms/video/player', 'platforms/audio', 'platforms/windows']   # android/, ios/, linux/: LP64 only
SKIP = ('third_party', 'include-glibc', 'include-mingw', '/compat/include/', 'longscan.py')
EXTS = ('.cpp', '.h', '.c', '.cc', '.mm', '.inl')

FIXED = re.compile(r'\b(u?int(8|16|32|64)_t|u?intptr_t)\b')
TOKEN = re.compile(r'''//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*'|[A-Za-z_]\w*|\s+|.''', re.S)


def tokens(text):
    return [m.group(0) for m in TOKEN.finditer(text)]


def is_code(t):
    return not (t.startswith('//') or t.startswith('/*') or t.startswith('"') or t.startswith("'") or t.isspace())


def scan(text):
    """-> (new text, [(line, old, new)]). Lines naming a LIBC_LONG function keep their longs."""
    toks = tokens(text)
    code = [i for i, t in enumerate(toks) if is_code(t)]
    pos_in_code = {i: k for k, i in enumerate(code)}
    lines, line = [], 1
    for t in toks:
        lines.append(line)
        line += t.count('\n')
    src_lines = text.split('\n')
    out = list(toks)
    hits = []
    consumed = set()
    for k, i in enumerate(code):
        if toks[i] != 'long' or i in consumed:
            continue
        prev = toks[code[k - 1]] if k > 0 else ''
        nxt = toks[code[k + 1]] if k + 1 < len(code) else ''
        if nxt in ('long', 'double') or prev == 'long':
            continue
        line_text = src_lines[lines[i] - 1]
        # a libc name in the code, or as an asm label (`zs_ftell(...) asm("ftell")`), not in a string
        labels = ' '.join(re.findall(r'\b(?:asm|__asm__)\s*\(\s*"([^"]*)"', line_text))
        if LIBC_RE.search(re.sub(r'"(?:\\.|[^"\\])*"', '""', line_text) + ' ' + labels):
            continue
        start, end, unsigned = i, i, False
        if prev in ('unsigned', 'signed'):
            start = code[k - 1]
            unsigned = prev == 'unsigned'
        if nxt == 'unsigned':           # long unsigned [int]
            unsigned = True
            end = code[k + 1]
            if k + 2 < len(code) and toks[code[k + 2]] == 'int':
                end = code[k + 2]
        elif nxt == 'int':
            end = code[k + 1]
        new = 'uint64_t' if unsigned else 'int64_t'
        old = ''.join(toks[start:end + 1])
        out[start] = new
        for j in range(start + 1, end + 1):
            out[j] = ''
            consumed.add(j)
        hits.append((lines[i], old, new))
    return ''.join(out), hits


def add_include(path, text):
    rel = os.path.relpath(path, REPO)
    inc = '#include "k2stdint.h"' if rel.startswith('src' + os.sep) else '#include <stdint.h>'
    if inc in text or (inc.startswith('#include <') and re.search(r'#include\s*<(c?stdint|inttypes)\.?h?>', text)):
        return text
    if path.endswith('k2stdint.h'):
        return text
    lines = text.split('\n')
    # before the first directive or declaration, past the leading comment block; past an include
    # guard (or #pragma once) when that is the first directive
    at = 0
    while at < len(lines) and (lines[at].startswith('//') or not lines[at].strip()):
        at += 1
    if at < len(lines) and lines[at].startswith('#pragma once'):
        at += 1
    elif at + 1 < len(lines) and lines[at].startswith('#ifndef') and lines[at + 1].startswith('#define'):
        at += 2
    lines.insert(at, inc)
    return '\n'.join(lines)


def files(paths):
    for p in paths:
        p = os.path.join(REPO, p)
        if os.path.isfile(p):
            yield p
            continue
        for root, _, names in os.walk(p):
            for n in sorted(names):
                f = os.path.join(root, n)
                if n.endswith(EXTS) and not any(s in f for s in SKIP) and '.gen.' not in n:
                    yield f


def main():
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--check', action='store_true')
    g.add_argument('--rewrite', action='store_true')
    ap.add_argument('paths', nargs='*')
    a = ap.parse_args()
    total, nfiles = 0, 0
    for f in files(a.paths or DEFAULT):
        text = open(f, encoding='utf-8', errors='surrogateescape').read()
        new, hits = scan(text)
        if not hits:
            if a.rewrite and FIXED.search(text) and add_include(f, text) != text:
                open(f, 'w', encoding='utf-8', errors='surrogateescape').write(add_include(f, text))
            continue
        total += len(hits)
        nfiles += 1
        rel = os.path.relpath(f, REPO)
        if a.check:
            for ln, old, rep in hits[:5]:
                print('%s:%d: bare `%s` (-> %s)' % (rel, ln, old, rep))
            if len(hits) > 5:
                print('%s: ... %d more' % (rel, len(hits) - 5))
        else:
            open(f, 'w', encoding='utf-8', errors='surrogateescape').write(add_include(f, new))
    print('longscan: %d bare long(s) in %d file(s)%s' % (total, nfiles, '' if a.check else ' rewritten'),
          file=sys.stderr)
    return 1 if a.check and total else 0


if __name__ == '__main__':
    sys.exit(main())
