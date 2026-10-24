#!/bin/sh
# llvm-rc for the clang-cl cross build (*-cross-windows-msvc-pc) (windows/msvc.toolchain.cmake): it parses its arguments
# cl-style, so an input path under /Users reads as the /U option. CMake puts the input last; `--`
# before it keeps it a file name.
rc="$1"; shift
n=$#
i=0
for a in "$@"; do
  i=$((i + 1))
  if [ "$i" -eq "$n" ]; then set -- "$@" -- "$a"; else set -- "$@" "$a"; fi
done
shift "$n"
exec "$rc" "$@"
