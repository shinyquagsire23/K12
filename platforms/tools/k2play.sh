#!/bin/zsh
# Launch the ASan host build under lldb, driven live through a K2_CTL control file (default
# /tmp/k2/ctl; drive it with platforms/tools/k2ctl.py). Extra args go to lldb before `run`, e.g.
#   platforms/tools/k2play.sh -o "breakpoint set -K false -n CSWCDoor::GetTargetActions -G true -C 'p \$w2'"
# K2_INPUT still works for the boot sequence (e.g. main menu -> Load -> first save):
#   K2_INPUT="1500:click:617,357;1650:click:335,177;1700:click:362,587" platforms/tools/k2play.sh > /tmp/k2/game.log 2>&1 &
# Kill orphans (`pkill -9 -f k2-host-pc-asan/macos/k2$`) after killing lldb: a stopped orphan still
# polls the control file and eats commands.
ROOT=${0:A:h:h:h}
mkdir -p /tmp/k2
cd $ROOT/platforms/out/gamedir-android
rm -f ${K2_CTL:-/tmp/k2/ctl}; rm -f /tmp/k2asan.txt* 2>/dev/null
export K2_DATA_DIR=$PWD K2_STUBS=soft K2_CTL=${K2_CTL:-/tmp/k2/ctl} K2_HIDDEN=${K2_HIDDEN-1} K2_SKIP_MOVIES=${K2_SKIP_MOVIES-1}
export ASAN_OPTIONS=detect_leaks=0:abort_on_error=1:detect_container_overflow=0:detect_odr_violation=0:log_path=/tmp/k2asan.txt
exec lldb -b "$@" -o run -k "thread backtrace -c 40" -k quit -- ../k2-host-pc-asan/macos/k2
