#!/bin/sh
# Fetch Visual Studio 2022's MSVC toolset (cl.exe, link.exe, the CRT, STL and ASan runtime) and the
# Windows SDK into platforms/out/_msvc_wine/msvc17, with wrappers that run them under Wine, so the
# k2-cross-windows-cl-pc presets (windows/msvc-wine.toolchain.cmake) build with the real cl.exe from
# macOS / Linux. Uses msvc-wine (https://github.com/mstorsjo/msvc-wine): its vsdownload.py and
# install.sh. Needs wine64 (or wine) on PATH, python3, msitools (msiextract) and git. On Windows with
# Visual Studio 2022 none of this is needed: the host presets (<k1|k2>-host-<ui>) use the installed toolset.
#
# The packages are Microsoft's, under the Visual Studio license terms, which you have to accept:
#   https://go.microsoft.com/fwlink/?LinkId=2086102
# The script does nothing without --accept-license.
#
#   platforms/windows/get_msvc_wine.sh --accept-license
set -eu
accept=0
for a in "$@"; do
  case "$a" in
    --accept-license) accept=1 ;;
    *) echo "get_msvc_wine.sh: unknown argument $a" >&2; exit 2 ;;
  esac
done
if [ "$accept" != 1 ]; then
  cat >&2 <<'MSG'
get_msvc_wine.sh: the MSVC toolset and Windows SDK are Microsoft's, under the Visual Studio license:
  https://go.microsoft.com/fwlink/?LinkId=2086102
Read it, then run again with --accept-license.
MSG
  exit 1
fi
here=$(cd "$(dirname "$0")" && pwd)
dest="$here/../out/_msvc_wine"
mkdir -p "$dest"
if [ -x "$dest/msvc17/bin/x64/cl" ]; then echo "$dest/msvc17"; exit 0; fi
command -v msiextract >/dev/null || { echo "get_msvc_wine.sh: needs msitools (brew install msitools)" >&2; exit 1; }
command -v wine64 >/dev/null || command -v wine >/dev/null || { echo "get_msvc_wine.sh: needs wine" >&2; exit 1; }
[ -d "$dest/src" ] || git clone --depth 1 https://github.com/mstorsjo/msvc-wine.git "$dest/src"
# VS 2022 (major 17), the x64 toolset only.
python3 "$dest/src/vsdownload.py" --major 17 --accept-license --architecture x64 --host-arch x64 \
  --dest "$dest/msvc17"
"$dest/src/install.sh" "$dest/msvc17"
echo "$dest/msvc17"
