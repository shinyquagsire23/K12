#!/usr/bin/env bash
# msvc-wine-rsp.sh <msvc-wine tool wrapper> args... -- run lib.exe / link.exe under Wine with the
# arguments in a response file (windows/msvc-wine.toolchain.cmake). A Windows command line holds 32767
# characters and Ninja on a macOS / Linux host passes a static library's or an executable's object
# list inline; the response file has no such limit. Absolute paths are rewritten to Wine's Z: drive,
# as msvc-wine's own wrapper does for the command line (it cannot see inside a response file).
tool=$1
shift
rsp="k2rsp.$$.rsp"
: > "$rsp"
for a; do
  case "$a" in
    /*) d=$(dirname "$a"); [ "$d" != / ] && [ -d "$d" ] && a="z:$a" ;;
    [-/][A-Za-z]*:/*) p=${a#*:}; d=$(dirname "$p"); [ "$d" != / ] && [ -d "$d" ] && a="${a%%:*}:z:$p" ;;
  esac
  a=${a//\"/\\\"}
  printf '"%s"\n' "$a" >> "$rsp"
done
# UTF-16LE with a BOM, which lib.exe and link.exe read unambiguously.
{ printf '\377\376'; iconv -f UTF-8 -t UTF-16LE < "$rsp"; } > "$rsp.u16" && mv "$rsp.u16" "$rsp"
"$tool" "@$rsp"
rc=$?
rm -f "$rsp"
exit $rc
