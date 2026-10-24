#!/usr/bin/env bash
# build_run_macos.sh — build the macOS host build (platforms/) and run it on the Android data.
#
#   ./build_run_macos.sh [options] [-- game args...]
#
#   cmake --preset <P>  ->  cmake --build --preset <P>  ->  game dir setup  ->  k2 (k1)
#
# <P> is the UI the build presents (K2P_UI): `k2-host-pc` (PC UI, the default), `k2-host-touch`
# (--touch: the Android touch UI) or `k2-host-xbox` (--xbox: the console UI). Each has its own build
# dir, platforms/out/<P>/, so switching does not rebuild the others.
#
# --k1 builds KOTOR 1 instead (presets k1-host-<ui>: the same tree with KOTOR1 and K1's port layer;
# the UI defaults to touch, as on the device -- --ui pc / --xbox are a first pass). It runs straight
# off K1's two OBBs in decomp_bins/swkotor_obb/ (zips the engine reads in place, so nothing is
# extracted), with platforms/out/gamedir-k1/ as the writable working directory (swkotor.ini, saves/;
# in the touch UI also the dialog font from the APK's assets). In the touch UI the mouse acts as a
# finger, as on the device; --chromebook uses K1's own mouse+keyboard mode (always on in the K1 pc UI).
#
# The game data is the Android OBB content. On first use the two OBBs in decomp_bins/swkotorii_obb/
# are extracted into decomp_bins/swkotorii_android/, and platforms/out/gamedir-android/ is populated
# with symlinks to it plus a private, writable copy of swkotor2.ini (the engine rewrites it). Later
# runs reuse both.
#
# The run uses K2_STUBS=soft (log each unimplemented symbol once and keep going) unless --hard.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORMS="$REPO/platforms"
PRESET=k2-host-pc
UI=""
GAME=k2
WINDOW=""
OBB_DIR="$REPO/decomp_bins/swkotorii_obb"
DATA_DIR="$REPO/decomp_bins/swkotorii_android"
GAME_DIR="$PLATFORMS/out/gamedir-android"

DO_BUILD=1 DO_RUN=1 RECONFIGURE=0 DEBUGGER=0 TIMEOUT=0 MOBILE_OPT=OFF ASAN=0
RUN_ENV=(K2_STUBS=soft)
RUN_ARGS=()

die() { echo "error: $*" >&2; exit 1; }
say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }

usage() {
  sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//; $d'
  cat <<'EOF'
Options:
  --no-build        skip cmake configure+build, run the k2 / k1 already built
  --no-run          stop after building (and setting up the game dir)
  --k1              KOTOR 1 (preset k1-host-<ui>, touch unless --ui/--xbox; see above)
  --window WxH      K1: the window size, which K1 takes for the whole display (default 1280x720)
  --chromebook      K1: SDL_main's Chromebook mode (mouse + keyboard read directly)
  --ui pc|touch|xbox  which UI build to use (preset k2-host-pc / k2-host-touch / k2-host-xbox); default pc
  --touch           --ui touch: the Android touch UI (the mouse drives the touch hook, see README)
  --xbox            --ui xbox: the console UI (src/ under K2_UI_XBOX; no pad driver on the host yet)
  --mobile-opt      configure with K2P_MOBILE_OPT=ON (the port's cut bodies stay cut; e.g. Switch =
                    --xbox --mobile-opt). Without it the build dir is put back to OFF.
  --reconfigure     re-run `cmake --preset <P>` even if the build dir is configured
  --hard            K2_STUBS unset: abort at the first unimplemented symbol
  --hidden          K2_HIDDEN=1: never show the window (pair with --shot)
  --shot N[,N...]   K2_SHOT: write those frames to <game dir>/k2shot_<N>.ppm
  --gl-check        K2_GL_CHECK=1: log the first GL error per entry point (--gl-trap: stop there)
  --gl-trap         K2_GL_CHECK=trap
  --language N      K2_LANGUAGE (0 English, 3 French, 4 German, 5 Italian, 9 Spanish)
  --timeout SECS    kill the run after SECS (SIGKILL: SDL turns SIGTERM into a quit event)
  --lldb            run under lldb
  --asan            AddressSanitizer build in out/<preset>-asan (configured on first use, with
                    platforms/asan_ignore.txt); reports go to <game dir>/asan.<pid>
  -h, --help        this text

Any other K2_* variable in the environment reaches the game unchanged (e.g. K2_PREF_<key>).
Arguments after `--` are passed to the game.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --k1)            GAME=k1 ;;
    --window)        WINDOW="${2:?--window needs WxH}"; shift ;;
    --window=*)      WINDOW="${1#*=}" ;;
    --chromebook)    RUN_ENV+=(K1_CHROMEBOOK=1) ;;
    --no-build)      DO_BUILD=0 ;;
    --no-run)        DO_RUN=0 ;;
    --ui)            UI="${2:?--ui needs pc, touch or xbox}"; shift ;;
    --ui=*)          UI="${1#*=}" ;;
    --touch)         UI=touch ;;
    --xbox)          UI=xbox ;;
    --mobile-opt)    MOBILE_OPT=ON ;;
    --reconfigure)   RECONFIGURE=1 ;;
    --hard)          RUN_ENV=() ;;
    --hidden)        RUN_ENV+=(K2_HIDDEN=1) ;;
    --shot)          RUN_ENV+=("K2_SHOT=${2:?--shot needs frame numbers}"); shift ;;
    --shot=*)        RUN_ENV+=("K2_SHOT=${1#*=}") ;;
    --gl-check)      RUN_ENV+=(K2_GL_CHECK=1) ;;
    --gl-trap)       RUN_ENV+=(K2_GL_CHECK=trap) ;;
    --language)      RUN_ENV+=("K2_LANGUAGE=${2:?--language needs a number}"); shift ;;
    --language=*)    RUN_ENV+=("K2_LANGUAGE=${1#*=}") ;;
    --timeout)       TIMEOUT="${2:?--timeout needs seconds}"; shift ;;
    --timeout=*)     TIMEOUT="${1#*=}" ;;
    --lldb)          DEBUGGER=1 ;;
    --asan)          ASAN=1 ;;
    -h|--help)       usage; exit 0 ;;
    --)              shift; RUN_ARGS=("$@"); break ;;
    *)               usage >&2; die "unknown argument: $1" ;;
  esac
  shift
done

case "${UI:-pc}" in pc|touch|xbox) ;; *) die "unknown UI '$UI' (pc, touch or xbox)" ;; esac
PRESET="k2-host-${UI:-pc}"
if [ "$GAME" = k1 ]; then
  PRESET="k1-host-${UI:-touch}"   # presets are <k1|k2>-host-<ui>
  OBB_DIR="$REPO/decomp_bins/swkotor_obb"
  GAME_DIR="$PLATFORMS/out/gamedir-k1"
  K1_FONT="$REPO/decomp_bins/swkotor_extracted/assets/iosdialog.otf"
  RUN_ENV+=("K2_OBB_DIR=$OBB_DIR")
  [ -n "$WINDOW" ] && RUN_ENV+=("K1_WINDOW=$WINDOW")
fi
BUILD_DIR="$PLATFORMS/out/$PRESET"
[ "$ASAN" -eq 1 ] && BUILD_DIR="$BUILD_DIR-asan"
EXE="$BUILD_DIR/macos/$GAME"   # k2 or k1

# ---- build --------------------------------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  for tool in cmake ninja; do command -v "$tool" >/dev/null || die "$tool not found (brew install $tool)"; done
  cd "$REPO"   # CMakeLists.txt / CMakePresets.json are at the root
  if [ "$ASAN" -eq 1 ]; then
    # The preset's cache variables in their own build dir (for pc, the <game>-host-pc-asan preset's),
    # plus K2P_ASAN (the sanitizer flags and asan_ignore.txt).
    if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
      say "cmake --preset $PRESET -DK2P_ASAN=ON -> $BUILD_DIR"
      cmake --preset "$PRESET" -B "$BUILD_DIR" -DK2P_MOBILE_OPT="$MOBILE_OPT" -DK2P_ASAN=ON
    fi
    say "cmake --build $BUILD_DIR"
    cmake --build "$BUILD_DIR"
  else
    # K2P_MOBILE_OPT is sticky in the cache, so reconfigure whenever it differs from what was asked.
    if [ -f "$BUILD_DIR/CMakeCache.txt" ] &&
       ! grep -q "^K2P_MOBILE_OPT:BOOL=$MOBILE_OPT\$" "$BUILD_DIR/CMakeCache.txt"; then
      RECONFIGURE=1
    fi
    if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
      say "cmake --preset $PRESET -DK2P_MOBILE_OPT=$MOBILE_OPT"
      cmake --preset "$PRESET" -DK2P_MOBILE_OPT="$MOBILE_OPT"
    fi
    say "cmake --build --preset $PRESET"
    cmake --build --preset "$PRESET"
  fi
fi
[ -x "$EXE" ] || die "no $GAME at $EXE (build first)"

# ---- game data ----------------------------------------------------------------------------------
if [ "$GAME" = k1 ]; then
  for f in main.53.com.aspyr.swkotor.obb patch.53.com.aspyr.swkotor.obb; do
    [ -f "$OBB_DIR/$f" ] || die "no $OBB_DIR/$f (adb pull it from /sdcard/Android/obb/com.aspyr.swkotor/)"
  done
  mkdir -p "$GAME_DIR/override"
  # The touch UI's widgets draw with the APK's TrueType face (the device installs it the same way).
  # The port's GLSL shaders are compiled in (resources/shaders), so nothing else comes from the APK.
  if [ "${UI:-touch}" = touch ] && [ ! -f "$GAME_DIR/override/iosdialog.otf" ]; then
    [ -f "$K1_FONT" ] || die "no $K1_FONT (the original K1 APK, unzipped)"
    cp "$K1_FONT" "$GAME_DIR/override/iosdialog.otf"
  fi
elif [ ! -f "$DATA_DIR/chitin.key" ]; then
  shopt -s nullglob
  obbs=("$OBB_DIR"/*.obb)
  shopt -u nullglob
  [ "${#obbs[@]}" -gt 0 ] || die "no extracted data in $DATA_DIR and no OBBs in $OBB_DIR"
  say "extracting ${#obbs[@]} OBB(s) -> $DATA_DIR"
  python3 "$PLATFORMS/tools/obbextract.py" "${obbs[@]}" -o "$DATA_DIR"
fi

# Symlink every top-level entry (new ones too), but keep the private swkotor2.ini.
if [ "$GAME" = k2 ]; then
  mkdir -p "$GAME_DIR"
  for e in "$DATA_DIR"/*; do
    name="${e##*/}"
    [ "$name" = swkotor2.ini ] && continue
    [ -e "$GAME_DIR/$name" ] && [ ! -L "$GAME_DIR/$name" ] && continue   # a real file the engine wrote
    ln -sfn "$e" "$GAME_DIR/$name"
  done
  [ -f "$GAME_DIR/swkotor2.ini" ] || cp "$DATA_DIR/swkotor2.ini" "$GAME_DIR/swkotor2.ini"
fi

[ "$DO_RUN" -eq 1 ] || { say "built; game dir ready at $GAME_DIR"; exit 0; }

# ---- run ----------------------------------------------------------------------------------------
cd "$GAME_DIR"
CMD=("$EXE" "${RUN_ARGS[@]+"${RUN_ARGS[@]}"}")
if [ "$DEBUGGER" -eq 1 ]; then
  CMD=(lldb -- "${CMD[@]}")
fi
if [ "$TIMEOUT" != 0 ]; then
  command -v timeout >/dev/null || die "--timeout needs GNU timeout (brew install coreutils)"
  CMD=(timeout -s KILL "$TIMEOUT" "${CMD[@]}")
fi
[ "$ASAN" -eq 1 ] && RUN_ENV+=("ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1:detect_container_overflow=0:detect_odr_violation=0:log_path=$GAME_DIR/asan}")
say "running $GAME ($PRESET$([ "$ASAN" -eq 1 ] && echo ", ASan")) in $GAME_DIR"
exec env K2_DATA_DIR="$GAME_DIR" "${RUN_ENV[@]+"${RUN_ENV[@]}"}" "${CMD[@]}"
