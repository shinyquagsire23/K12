#!/usr/bin/env bash
# build_run_linux.sh — build the platforms/ Linux k2 / k1 (the whole engine from src/) and run it,
# reading the game straight from the Android expansion files (nothing is extracted).
#
#   ./build_run_linux.sh [--k1] [--touch|--xbox] [options] [-- game args]
#
# preset: k2-host-pc / k2-host-touch / k2-host-xbox  KOTOR II, pc (default) / touch / console UI
#         k1-host-pc / k1-host-touch / k1-host-xbox  KOTOR 1 (--k1; pc by default here too)
#
# Game data: the OBBs, from --obb-dir, else $K2L_OBB_DIR, else decomp_bins/swkotorii_obb (K1:
# decomp_bins/swkotor_obb), else ~/k2data/obb2 (K1: ~/k2data/obb1). The writable game directory
# (saves, swkotor2.ini / swkotor.ini, k2shot_*.ppm) is platforms/out/gamedir-linux-k2 (-k1).
# Needs (Debian/Ubuntu): clang lld llvm libc++-dev libc++abi-dev libsdl2-dev libfreetype-dev libgl-dev
#                        libglu1-mesa-dev zlib1g-dev ninja-build cmake python3 librsvg2-bin
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLAT="$REPO/platforms"

GAME=k2 UI=pc DO_BUILD=1 DO_RUN=1 RECONFIGURE=0 TIMEOUT=0 HIDDEN=0 SHOT="" OBB_DIR="${K2L_OBB_DIR:-}"
RUN_ENV=("K2_STUBS=${K2_STUBS:-soft}")
RUN_ARGS=()

die() { echo "error: $*" >&2; exit 1; }
say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }

usage() {
  sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//; $d'
  cat <<'EOF'
Options:
  --k1                KOTOR 1
  --touch | --xbox    the touch or console UI (default: pc)
  --obb-dir DIR       where the main.* / patch.* .obb files are (passed to the game as K2_OBB_DIR)
  --window WxH        the window size (KOTOR II: K2_WINDOW; KOTOR 1: K1_WINDOW)
  --env K=V           one more environment variable for the run (repeatable): K2_FPS=1, K2_SKIP_MOVIES=1 ...
  --hidden            no window (K2_HIDDEN=1); with --shot for an unattended check
  --shot N[,N...]     write those frames to k2shot_<N>.ppm in the game directory
  --timeout SEC       kill the game after SEC seconds
  --xvfb              run under xvfb-run (no display needed; Mesa's llvmpipe renders on the CPU)
  --no-build | --no-run | --reconfigure
  -h, --help          this text
EOF
}

XVFB=0
while [ $# -gt 0 ]; do
  case "$1" in
    --k1)           GAME=k1 ;;
    --touch)        UI=touch ;;
    --xbox)         UI=xbox ;;
    --pc)           UI=pc ;;
    --obb-dir)      OBB_DIR="${2:?--obb-dir needs a directory}"; shift ;;
    --window)       WINDOW="${2:?--window needs WxH}"; shift ;;
    --env)          RUN_ENV+=("${2:?--env needs K=V}"); shift ;;
    --hidden)       HIDDEN=1 ;;
    --shot)         SHOT="${2:?--shot needs frame numbers}"; shift ;;
    --timeout)      TIMEOUT="${2:?--timeout needs seconds}"; shift ;;
    --xvfb)         XVFB=1 ;;
    --no-build)     DO_BUILD=0 ;;
    --no-run)       DO_RUN=0 ;;
    --reconfigure)  RECONFIGURE=1 ;;
    -h|--help)      usage; exit 0 ;;
    --)             shift; RUN_ARGS=("$@"); break ;;
    *)              usage >&2; die "unknown argument: $1" ;;
  esac
  shift
done

if [ "$GAME" = k1 ]; then
  PRESET="k1-host-$UI"
  OBBS=(main.53.com.aspyr.swkotor.obb patch.53.com.aspyr.swkotor.obb)
  for d in "$OBB_DIR" "$REPO/decomp_bins/swkotor_obb" "$HOME/k2data/obb1"; do
    [ -n "$d" ] && [ -f "$d/${OBBS[0]}" ] && { OBB_DIR="$d"; break; }
  done
else
  PRESET="k2-host-$UI"
  for d in "$OBB_DIR" "$REPO/decomp_bins/swkotorii_obb" "$HOME/k2data/obb2"; do
    [ -n "$d" ] && compgen -G "$d/main.*.obb" >/dev/null && { OBB_DIR="$d"; break; }
  done
fi
BUILD_DIR="$PLAT/out/$PRESET"
EXE="$BUILD_DIR/linux/$GAME"   # k2 or k1
GAME_DIR="$PLAT/out/gamedir-linux-$GAME"

# ---- build ---------------------------------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  for tool in cmake ninja clang++; do command -v "$tool" >/dev/null || die "$tool not found (see --help for the packages)"; done
  cd "$REPO"   # CMakeLists.txt / CMakePresets.json are at the root
  # the OBBs also feed the prompt art's glyph atlases (padglyphs.py, K2P_OBB_DIR)
  OBB_ARGS=()
  if [ -n "$OBB_DIR" ]; then
    OBB_ARGS=("-DK2P_OBB_DIR=$OBB_DIR")
    grep -qx "K2P_OBB_DIR:PATH=$OBB_DIR" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null || RECONFIGURE=1
  fi
  if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    say "cmake --preset $PRESET ${OBB_ARGS[*]-}"
    cmake --preset "$PRESET" "${OBB_ARGS[@]+"${OBB_ARGS[@]}"}"
  fi
  say "cmake --build --preset $PRESET"
  cmake --build --preset "$PRESET"
fi
[ -x "$EXE" ] || die "no $GAME at $EXE (build first)"
[ "$DO_RUN" -eq 1 ] || { say "built: $EXE"; exit 0; }

# ---- game data -----------------------------------------------------------------------------------
[ -n "$OBB_DIR" ] && [ -d "$OBB_DIR" ] || die "no OBBs found (pass --obb-dir DIR; see --help)"
mkdir -p "$GAME_DIR"
if [ "$GAME" = k1 ]; then
  for f in "${OBBS[@]}"; do [ -f "$OBB_DIR/$f" ] || die "no $OBB_DIR/$f"; done
  [ -n "${WINDOW:-}" ] && RUN_ENV+=("K1_WINDOW=$WINDOW")
  # the dialog font comes from the original APK's assets; the GLSL shaders are compiled in
  ASSETS="$REPO/decomp_bins/swkotor_extracted/assets"
  mkdir -p "$GAME_DIR/override"
  [ -f "$GAME_DIR/override/iosdialog.otf" ] || cp "$ASSETS/iosdialog.otf" "$GAME_DIR/override/iosdialog.otf" \
    || die "no $ASSETS/iosdialog.otf (the original K1 APK, unzipped)"
else
  [ -n "${WINDOW:-}" ] && RUN_ENV+=("K2_WINDOW=$WINDOW")
fi
RUN_ENV+=("K2_OBB_DIR=$OBB_DIR")   # the game picks its own main.* / patch.* (both games, every platform)
[ "$HIDDEN" -eq 1 ] && RUN_ENV+=("K2_HIDDEN=1")
[ -n "$SHOT" ] && RUN_ENV+=("K2_SHOT=$SHOT")

# ---- run -----------------------------------------------------------------------------------------
cd "$GAME_DIR"
CMD=("$EXE" "${RUN_ARGS[@]+"${RUN_ARGS[@]}"}")
if [ "$XVFB" -eq 1 ]; then
  command -v xvfb-run >/dev/null || die "--xvfb needs xvfb-run (apt install xvfb)"
  CMD=(xvfb-run -a -s "-screen 0 1920x1080x24" "${CMD[@]}")
elif [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && [ "$HIDDEN" -eq 0 ]; then
  echo "warning: no DISPLAY / WAYLAND_DISPLAY: run from the desktop's terminal, or pass --xvfb" >&2
fi
[ "$TIMEOUT" != 0 ] && CMD=(timeout -s KILL "$TIMEOUT" "${CMD[@]}")
say "running $GAME ($PRESET) in $GAME_DIR with the OBBs in $OBB_DIR"
exec env K2_DATA_DIR="$GAME_DIR" "${RUN_ENV[@]}" "${CMD[@]}"
