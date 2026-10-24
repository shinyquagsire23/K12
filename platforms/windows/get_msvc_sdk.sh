#!/bin/sh
# Fetch the MSVC CRT + Windows SDK headers and import libraries into platforms/out/_msvc_sdk/splat,
# where windows/msvc.toolchain.cmake (the *-cross-windows-msvc-pc presets, clang-cl cross-compiling from macOS /
# Linux) finds them. Uses xwin (https://github.com/Jake-Shadle/xwin), installed with cargo into
# platforms/out/_msvc_sdk/cargo. On Windows with Visual Studio 2022 none of this is needed: the
# host presets (<k1|k2>-host-<ui>) on Windows use the installed MSVC toolset and SDK.
#
# The packages are Microsoft's, under the Visual Studio license terms, which you have to accept:
#   https://go.microsoft.com/fwlink/?LinkId=2086102
# The script does nothing without --accept-license.
#
#   platforms/windows/get_msvc_sdk.sh --accept-license
set -eu
accept=0
for a in "$@"; do
  case "$a" in
    --accept-license) accept=1 ;;
    *) echo "get_msvc_sdk.sh: unknown argument $a" >&2; exit 2 ;;
  esac
done
if [ "$accept" != 1 ]; then
  cat >&2 <<'EOF'
get_msvc_sdk.sh: the MSVC CRT and Windows SDK are Microsoft's, under the Visual Studio license:
  https://go.microsoft.com/fwlink/?LinkId=2086102
Read it, then run again with --accept-license.
EOF
  exit 1
fi
XWIN_VERSION=${K2_XWIN_VERSION:-0.10.0}
here=$(cd "$(dirname "$0")" && pwd)
dest="$here/../out/_msvc_sdk"
mkdir -p "$dest"
xwin="$dest/cargo/bin/xwin"
if [ ! -x "$xwin" ]; then
  command -v cargo >/dev/null || { echo "get_msvc_sdk.sh: needs cargo (https://rustup.rs)" >&2; exit 1; }
  cargo install --locked --root "$dest/cargo" --version "$XWIN_VERSION" xwin
fi
if [ -d "$dest/splat/crt/include" ]; then echo "$dest/splat"; exit 0; fi
"$xwin" --accept-license --arch x86_64 --cache-dir "$dest/cache" splat --include-debug-libs --output "$dest/splat"
rm -rf "$dest/cache"
echo "$dest/splat"
