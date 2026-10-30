#!/usr/bin/env bash
# build_run_viewer.sh — build the model viewer (platforms/viewer/: k2view / k1view) and run it.
#
#   ./build_run_viewer.sh [--k1] [options] [-- viewer args...]
#
#   cmake --preset <game>-host-pc  ->  cmake --build --preset <game>-host-pc --target <game>view  ->  run
#
# The viewer is the game's own executable with the intro, menus and game loop swapped for one model in
# a room, a coloured light circling it and a Dear ImGui panel (the model, its animations, its
# reflection map, the light, the backdrop, the renderer's switches). It builds in the pc preset's dir
# beside the game, so a game build there already did most of the work.
#
# The game's files are found as the game finds them (an installed Steam / GOG / disc copy, else the
# working directory), or --asset-dir; the player's folder only gives it swkotor.ini's graphics
# settings. KOTOR 1 defaults to the PC data in decomp_bins/KOTOR1_Windows when it is there.
#
# Mouse: left drag orbits, right (or Shift + left) drag pans, the wheel zooms; WASD moves the camera
# (Q / E down and up, Shift faster) unless a panel field has the keyboard; Tab hides the panel.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORMS="$REPO/platforms"

GAME=k2 DO_BUILD=1 DO_RUN=1 RECONFIGURE=0 DEBUGGER=0
ASSET_DIR="" USER_DIR=""
RUN_ENV=()
RUN_ARGS=()

die() { echo "error: $*" >&2; exit 1; }
say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }

usage() {
  sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//; $d'
  cat <<'EOF'
Options:
  --k1              KOTOR 1 (k1view, preset k1-host-pc); default KOTOR II (k2view, k2-host-pc)
  --model RESREF    the model to start with (K2VIEW_MODEL; default lqa_dewback / c_hutt)
  --room LAYOUT     the backdrop, an area's .lyt (K2VIEW_ROOM; "none" for none;
                    default mqatester / 000test, BioWare's test rooms)
  --anim NAME       the animation to start with (K2VIEW_ANIM)
  --asset-dir DIR   the game's files (K2_ASSET_DIR: the folder with chitin.key)
  --user-dir DIR    the player's folder (K2_USER_DIR: swkotor.ini / swkotor2.ini)
  --ctl FILE        K2_CTL: scripted actions, e.g. `echo shot:NAME > FILE` for a screenshot
  --env K=V         one more environment variable for the run (repeatable)
  --no-build        skip cmake configure+build, run the viewer already built
  --no-run          stop after building
  --reconfigure     re-run `cmake --preset <P>` even if the build dir is configured
  --lldb            run under lldb
  -h, --help        this text

Arguments after `--` are passed to the viewer.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --k1)            GAME=k1 ;;
    --model)         RUN_ENV+=("K2VIEW_MODEL=${2:?--model needs a resref}"); shift ;;
    --model=*)       RUN_ENV+=("K2VIEW_MODEL=${1#*=}") ;;
    --room)          RUN_ENV+=("K2VIEW_ROOM=${2:?--room needs a layout}"); shift ;;
    --room=*)        RUN_ENV+=("K2VIEW_ROOM=${1#*=}") ;;
    --anim)          RUN_ENV+=("K2VIEW_ANIM=${2:?--anim needs a name}"); shift ;;
    --anim=*)        RUN_ENV+=("K2VIEW_ANIM=${1#*=}") ;;
    --asset-dir)     ASSET_DIR="${2:?--asset-dir needs a folder}"; shift ;;
    --asset-dir=*)   ASSET_DIR="${1#*=}" ;;
    --user-dir)      USER_DIR="${2:?--user-dir needs a folder}"; shift ;;
    --user-dir=*)    USER_DIR="${1#*=}" ;;
    --ctl)           RUN_ENV+=("K2_CTL=${2:?--ctl needs a file}"); shift ;;
    --ctl=*)         RUN_ENV+=("K2_CTL=${1#*=}") ;;
    --env)           RUN_ENV+=("${2:?--env needs K=V}"); shift ;;
    --env=*)         RUN_ENV+=("${1#*=}") ;;
    --no-build)      DO_BUILD=0 ;;
    --no-run)        DO_RUN=0 ;;
    --reconfigure)   RECONFIGURE=1 ;;
    --lldb)          DEBUGGER=1 ;;
    -h|--help)       usage; exit 0 ;;
    --)              shift; RUN_ARGS=("$@"); break ;;
    *)               usage >&2; die "unknown argument: $1" ;;
  esac
  shift
done

PRESET="$GAME-host-pc"
TARGET="${GAME}view"
BUILD_DIR="$PLATFORMS/out/$PRESET"
case "$(uname -s)" in
  Darwin) EXE="$BUILD_DIR/macos/$TARGET" ;;
  Linux)  EXE="$BUILD_DIR/linux/$TARGET" ;;
  *)      die "the desktop presets build on macOS and Linux; on Windows build the $TARGET target of the $PRESET preset" ;;
esac

# ---- build --------------------------------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  for tool in cmake ninja; do command -v "$tool" >/dev/null || die "$tool not found"; done
  cd "$REPO"   # CMakeLists.txt / CMakePresets.json are at the root
  if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    say "cmake --preset $PRESET"
    cmake --preset "$PRESET"
  fi
  say "cmake --build --preset $PRESET --target $TARGET"
  cmake --build --preset "$PRESET" --target "$TARGET"
fi
[ -x "$EXE" ] || die "no $TARGET at $EXE (build first)"
[ "$DO_RUN" -eq 1 ] || { say "built $EXE"; exit 0; }

# ---- run ----------------------------------------------------------------------------------------
if [ -z "$ASSET_DIR" ] && [ "$GAME" = k1 ] && [ -z "${K2_ASSET_DIR:-}${K2_DATA_DIR:-}" ] &&
   [ -f "$REPO/decomp_bins/KOTOR1_Windows/chitin.key" ]; then
  ASSET_DIR="$REPO/decomp_bins/KOTOR1_Windows"
fi
[ -n "$ASSET_DIR" ] && RUN_ENV+=("K2_ASSET_DIR=$(cd "$ASSET_DIR" && pwd)")
[ -n "$USER_DIR" ] && { mkdir -p "$USER_DIR"; RUN_ENV+=("K2_USER_DIR=$(cd "$USER_DIR" && pwd)"); }

CMD=("$EXE" "${RUN_ARGS[@]+"${RUN_ARGS[@]}"}")
[ "$DEBUGGER" -eq 1 ] && CMD=(lldb -- "${CMD[@]}")
say "running $TARGET ($PRESET)"
exec env "${RUN_ENV[@]+"${RUN_ENV[@]}"}" "${CMD[@]}"
