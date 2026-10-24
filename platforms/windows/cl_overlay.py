#!/usr/bin/env python3
"""cl_overlay.py -- the compat header overlays without #include_next, for cl.exe (docs/MSVC_PORT.md).

The MSVC builds put header layers in front of the UCRT: compat/include-msvc/, compat/include-mingw/
and mingw_crt.py --ucrt's renamed copies (gen/msvc-crt). A layer's header adds to the next one's
with `#include_next <h>`, which clang-cl understands and cl.exe does not (C1021). This copies each
layer to <out>/L<i>/ with every `#include_next <h>` replaced by a quoted, relative include of the
next layer's h (or, past the last layer, of a copy of the system header, from --system), so the
search order -- L0, L1, ... -- and the chains are the same as before.

    cl_overlay.py --layer compat/include-msvc --layer compat/include-mingw --layer gen/msvc-crt \\
                  --system <ucrt include dir> --out gen/cl-overlay
"""
import argparse
import os
import re

NEXT = re.compile(r'^(\s*)#\s*include_next\s*<([^>]+)>', re.M)


def files(root):
    out = []
    for d, _, names in os.walk(root):
        for n in names:
            if n.endswith('.h'):
                out.append(os.path.relpath(os.path.join(d, n), root).replace(os.sep, '/'))
    return sorted(out)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if not os.path.exists(path) or open(path, newline='', encoding='utf-8', errors='surrogateescape').read() != text:
        open(path, 'w', newline='', encoding='utf-8', errors='surrogateescape').write(text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--layer', action='append', required=True)
    ap.add_argument('--system', action='append', default=[])
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    layers = [files(l) for l in a.layer]
    nsys = len(a.layer)   # the system copies' layer

    def target(h, after):
        """The index of the first layer past `after` with h (the system copies' last), or None."""
        for i in range(after + 1, len(a.layer)):
            if h in layers[i]:
                return i
        for s in a.system:
            if os.path.exists(os.path.join(s, h)):
                write(os.path.join(a.out, 'L%d' % nsys, h), open(os.path.join(s, h), newline='', encoding='utf-8', errors='surrogateescape').read())
                return nsys
        return None   # a header no search reaches (include-mingw's unistd.h behind include-msvc's)

    for i, root in enumerate(a.layer):
        for h in layers[i]:
            text = open(os.path.join(root, h), newline='', encoding='utf-8', errors='surrogateescape').read()

            def repl(m, i=i, h=h):
                j = target(m.group(2), i)
                if j is None:
                    return '%s#error "cl_overlay.py: no <%s> after this layer"' % (m.group(1), m.group(2))
                rel = os.path.relpath(os.path.join(a.out, 'L%d' % j, m.group(2)),
                                      os.path.dirname(os.path.join(a.out, 'L%d' % i, h))).replace(os.sep, '/')
                return '%s#include "%s"' % (m.group(1), rel.replace(os.sep, '/'))
            write(os.path.join(a.out, 'L%d' % i, h), NEXT.sub(repl, text))
    print(' '.join(os.path.join(a.out, 'L%d' % i).replace(os.sep, '/') for i in range(len(a.layer))))


if __name__ == '__main__':
    main()
