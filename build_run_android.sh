#!/usr/bin/env bash
# build_run_android.sh — build the platforms/ Android APK (the whole engine from src/, on stock SDL2)
# and run it on a device or the emulator.
#
#   ./build_run_android.sh [k2-cross-android-touch|k2-cross-android-xbox|k1-cross-android-touch|k1-cross-android-xbox] [options]
#
# preset: k2-cross-android-touch  the touch UI, as in the shipped APK (default)
#         k2-cross-android-xbox   the console UI, for a game controller (also --xbox)
#         k1-cross-android-touch  KOTOR 1 (also --k1): the same tree with KOTOR1, touch UI. It installs as
#                           com.aspyr.swkotor.hybrid -- BESIDE Aspyr's com.aspyr.swkotor and the KOTOR
#                           II port, replacing neither -- with K1's OBBs copied from the installed
#                           original (or pushed from decomp_bins/swkotor_obb/).
#         k1-cross-android-xbox   KOTOR 1, console UI (also --k1 --xbox; first pass)
# Presets are named <k1|k2>-cross-android-<ui>.
#
# It installs as the HYBRID's package (com.aspyr.swkotorii.hybrid), so it replaces that APK and keeps
# its data: the saves, swkotor2.ini and the OBBs are shared, and ./build_run_hybrid_emu.sh puts the
# hybrid back the same way. Flow:
#   cmake --preset P -> cmake --build --preset P -> device.py install -> device.py run (logcat)
# With no device attached it boots the k2hybrid AVD (the one with the OBBs), like
# build_run_hybrid_emu.sh; it never wipes its data.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLAT="$REPO/platforms"

PRESET=""
AVD="${K2H_AVD:-k2hybrid}"
DO_BUILD=1 DO_INSTALL=1 DO_RUN=1 RECONFIGURE=0 HEADLESS=0 K1=0 XBOX=0
RUN_ENV="${K2A_ENV:-K2_STUBS=soft}"
BOOT_TIMEOUT="${K2H_BOOT_TIMEOUT:-420}"

die() { echo "error: $*" >&2; exit 1; }
say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }

usage() {
  sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//; $d'
  cat <<'EOF'
Options:
  --k1              KOTOR 1 (k1-cross-android-touch; with --xbox, k1-cross-android-xbox)
  --xbox            the console UI (k2-cross-android-xbox, or k1-cross-android-xbox with --k1)
  --env "K=V;..."   the k2env intent extra: K2_* switches for this launch (default K2_STUBS=soft,
                    or $K2A_ENV); <files>/k2env.txt on the device is read as well
  --avd NAME        AVD to boot when no device is attached (default: k2hybrid, or $K2H_AVD)
  --no-build        skip cmake configure+build, use the APK already in platforms/out/<preset>/
  --no-install      skip adb install + OBB staging
  --no-run          stop after installing; do not launch or follow logcat
  --reconfigure     re-run `cmake --preset <preset>` even if the build dir is configured
  --headless        start the emulator with -no-window
  -h, --help        this text
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    k[12]-cross-android-touch|k[12]-cross-android-xbox) [ -z "$PRESET" ] || die "preset given twice ($PRESET, $1)"; PRESET="$1" ;;
    --k1)           K1=1 ;;
    --xbox)         XBOX=1 ;;
    --env)          RUN_ENV="${2?--env needs a value}"; shift ;;
    --env=*)        RUN_ENV="${1#*=}" ;;
    --avd)          AVD="${2:?--avd needs a name}"; shift ;;
    --no-build)     DO_BUILD=0 ;;
    --no-install)   DO_INSTALL=0 ;;
    --no-run)       DO_RUN=0 ;;
    --reconfigure)  RECONFIGURE=1 ;;
    --headless)     HEADLESS=1 ;;
    -h|--help)      usage; exit 0 ;;
    *)              usage >&2; die "unknown argument: $1" ;;
  esac
  shift
done
if [ "$K1$XBOX" != 00 ]; then
  WANT=k2-cross-android-touch; [ "$K1" = 1 ] && WANT=k1-cross-android-touch
  [ "$XBOX" = 1 ] && { [ "$K1" = 1 ] && WANT=k1-cross-android-xbox || WANT=k2-cross-android-xbox; }
  [ -z "$PRESET" ] || [ "$PRESET" = "$WANT" ] || die "preset given twice ($PRESET, $WANT)"
  PRESET="$WANT"
fi
PRESET="${PRESET:-k2-cross-android-touch}"
BUILD_DIR="$PLAT/out/$PRESET"
UI=touch; case "$PRESET" in *xbox) UI=xbox ;; esac
STEM=k2-android ACTIVITY=k2.K2Activity INSTALL_ARGS=()
if [ "${PRESET#k1-}" != "$PRESET" ]; then
  STEM=k1-android ACTIVITY=k2.K1Activity
  INSTALL_ARGS=(--orig-package com.aspyr.swkotor --obb-src "$REPO/decomp_bins/swkotor_obb")
fi
APK="$BUILD_DIR/$STEM-$UI.apk"

SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
[ -d "$SDK" ] || die "Android SDK not found at $SDK (set ANDROID_HOME)"
ADB="$SDK/platform-tools/adb"
EMULATOR="$SDK/emulator/emulator"
export PATH="$SDK/platform-tools:$SDK/emulator:$PATH"   # device.py shells out to a bare `adb`

# ---- a device: the attached one, else boot the AVD (it boots while the build runs) ----------------
SERIAL="$("$ADB" devices | awk 'NR > 1 && $2 == "device" {print $1; exit}')"
if [ -z "$SERIAL" ] && [ "$DO_INSTALL$DO_RUN" != "00" ]; then
  [ -x "$EMULATOR" ] || die "no device attached and no emulator at $EMULATOR"
  EMU_LOG="$PLAT/out/emulator-$AVD.log"
  mkdir -p "$PLAT/out"
  ARGS=(-avd "$AVD" -no-boot-anim)
  [ "$HEADLESS" -eq 1 ] && ARGS+=(-no-window)
  say "no device attached: starting emulator $AVD (log: $EMU_LOG)"
  nohup "$EMULATOR" "${ARGS[@]}" >"$EMU_LOG" 2>&1 &
  for _ in $(seq 1 60); do
    SERIAL="$("$ADB" devices | awk '/^emulator-[0-9]+\t/ {print $1; exit}')"
    [ -n "$SERIAL" ] && break
    sleep 1
  done
  [ -n "$SERIAL" ] || die "the emulator never appeared in \`adb devices\`; see $EMU_LOG"
fi
[ -n "$SERIAL" ] && export ANDROID_SERIAL="$SERIAL"

# ---- build ---------------------------------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    say "cmake --preset $PRESET"
    ( cd "$REPO" && cmake --preset "$PRESET" )
  fi
  say "cmake --build --preset $PRESET"
  ( cd "$REPO" && cmake --build --preset "$PRESET" )
  echo "    link report: $BUILD_DIR/report/linkreport.md"
fi
[ -f "$APK" ] || die "no APK at $APK (drop --no-build, or build the '$PRESET' preset first)"
PKG="$(sed -n 's/^K2A_PACKAGE:STRING=//p' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | head -1)"
if [ -z "$PKG" ]; then
  PKG=com.aspyr.swkotorii.hybrid; [ "${PRESET#k1-}" != "$PRESET" ] && PKG=com.aspyr.swkotor.hybrid
fi

if [ "$DO_INSTALL$DO_RUN" != "00" ]; then
  say "waiting for $SERIAL (timeout ${BOOT_TIMEOUT}s)"
  "$ADB" wait-for-device
  deadline=$(( $(date +%s) + BOOT_TIMEOUT ))
  until [ "$("$ADB" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r\n')" = "1" ]; do
    [ "$(date +%s)" -lt "$deadline" ] || die "$SERIAL did not finish booting in ${BOOT_TIMEOUT}s"
    sleep 2
  done
fi

# ---- install (APK + OBBs: device.py copies or pushes them only when missing) ----------------------
if [ "$DO_INSTALL" -eq 1 ]; then
  say "installing $APK as $PKG"
  python3 "$REPO/platforms/tools/device.py" install --apk "$APK" "--package=$PKG" "${INSTALL_ARGS[@]+"${INSTALL_ARGS[@]}"}"
fi

# ---- launch + logcat -----------------------------------------------------------------------------
if [ "$DO_RUN" -eq 1 ]; then
  say "launching $PKG with k2env \"$RUN_ENV\" (Ctrl-C stops the logcat; the app keeps running)"
  exec python3 "$REPO/platforms/tools/device.py" run "--package=$PKG" --activity "$ACTIVITY" \
       --extra "k2env=$RUN_ENV"
fi
say "done — $APK is installed as $PKG"
