#!/bin/sh
# snapshot_windows.sh -- keep the four Windows exes of a test round, so a crash report's
# `k1.exe+0x...` / `k2.exe+0x...` lines can be symbolized after later rebuilds:
#   llvm-symbolizer --obj=out/_snapshots/<round>/<preset>/k2.exe 0x140000000+<offset>
# The exes carry their own DWARF, so the exe is all that is needed.
set -e
cd "$(dirname "$0")/.."
round="$(date +%Y%m%d-%H%M)-$(git rev-parse --short HEAD)$(git diff --quiet HEAD -- ../src . || echo -dirty)"
for p in k2-cross-windows-pc k2-cross-windows-xbox k1-cross-windows-pc k1-cross-windows-xbox; do
    mkdir -p "out/_snapshots/$round/$p"
    cp out/$p/windows/k*.exe "out/_snapshots/$round/$p/"
    rm -f "out/_snapshots/$round/$p/k2smoke.exe"
done
echo "out/_snapshots/$round"
