#!/bin/sh
# Fetch llvm-mingw (clang + lld + libc++ for x86_64-w64-mingw32) into platforms/out/_llvm-mingw,
# where windows/mingw.toolchain.cmake finds it. macOS (universal) and Linux (x86_64 / aarch64) hosts;
# on Windows unpack llvm-mingw-<ver>-ucrt-x86_64.zip from the same release page yourself and set
# K2_LLVM_MINGW (or put its bin/ on PATH).
set -eu
VER=${K2_LLVM_MINGW_VERSION:-20261006}
here=$(cd "$(dirname "$0")" && pwd)
dest="$here/../out/_llvm-mingw"
case "$(uname -s)-$(uname -m)" in
  Darwin-*)        pkg="llvm-mingw-$VER-ucrt-macos-universal" ;;
  Linux-x86_64)    pkg="llvm-mingw-$VER-ucrt-ubuntu-22.04-x86_64" ;;
  Linux-aarch64)   pkg="llvm-mingw-$VER-ucrt-ubuntu-22.04-aarch64" ;;
  *) echo "get_llvm_mingw.sh: no llvm-mingw build for $(uname -s)-$(uname -m)" >&2; exit 1 ;;
esac
if [ -d "$dest/$pkg/bin" ]; then echo "$dest/$pkg"; exit 0; fi
mkdir -p "$dest"
curl -fL -o "$dest/$pkg.tar.xz" "https://github.com/mstorsjo/llvm-mingw/releases/download/$VER/$pkg.tar.xz"
tar -C "$dest" -xf "$dest/$pkg.tar.xz"
rm "$dest/$pkg.tar.xz"
echo "$dest/$pkg"
