#!/usr/bin/env bash
# build_run_ios.sh — build the platforms/ iOS app (the whole engine from src/, on gl4es + stock SDL2)
# and run it on a connected iPhone / iPad, or on the iOS Simulator with --sim.
#
#   ./build_run_ios.sh [--k1] [--touch|--xbox] [--sim] [options]
#
# preset: k2-cross-ios-pc     KOTOR II, PC UI: a tap is the mouse; an iPad's keyboard and mouse work (default)
#         k2-cross-ios-touch  the touch UI, as in the Android APK (--touch)
#         k2-cross-ios-xbox   the console UI, for a paired game controller (--xbox)
#         k1-cross-ios-pc / k1-cross-ios-touch / k1-cross-ios-xbox   KOTOR 1 (--k1)
#         <game>-cross-ios-sim-<ui>   the same for the Simulator (--sim): k2-cross-ios-sim-pc, ...
#
# Device flow: cmake --preset P -> cmake --build --preset P -> k2ipa.py run-device, which
#   * picks the device: the first paired iPhone / iPad (USB or the network), or --device NAME|UDID;
#   * re-signs the app with an "Apple Development" identity from the keychain and a development
#     provisioning profile from Xcode that covers the device and the bundle id (a team wildcard
#     profile covers any id; --profile FILE picks one);
#   * installs it (xcrun devicectl), copies the OBBs and k2env.txt into its Documents folder --
#     devicectl skips unchanged files, so the 2-4 GB of OBBs only cross once -- and launches it with
#     the console attached (stdout/stderr here; Ctrl-C detaches, the app keeps running).
# Its Documents folder (saves, the ini, k2.log) is also in Finder's device pane and the Files app.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLAT="$REPO/platforms"

K1=0 UI=pc SIM=0 DO_BUILD=1 DO_RUN=1 RECONFIGURE=0 CONSOLE=1
DEVICE="" PROFILE="" BUNDLE_ID=""
RUN_ENV="${K2I_ENV:-K2_STUBS=soft}"

die() { echo "error: $*" >&2; exit 1; }
say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }

usage() {
  sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//; $d'
  cat <<'EOF'
Options:
  --k1                KOTOR 1 (k1-cross-ios-<ui>)
  --touch | --xbox    the touch or console UI (default: pc)
  --sim               build for the Simulator and run there (booting one if none is)
  --device NAME|UDID  which iPhone / iPad (default: the first paired one)
  --profile FILE      the .mobileprovision to sign with (default: picked from Xcode's)
  --bundle-id ID      the app's bundle id (default com.k2port.swkotorii.<ui> / com.k2port.swkotor.<ui>;
                      needed when only an explicit-id profile covers the device)
  --env "K=V;..."     Documents/k2env.txt: K2_* switches (default K2_STUBS=soft, or $K2I_ENV)
  --no-build          use the app already in platforms/out/<preset>/ios/
  --no-run            stop after building
  --no-console        launch and return instead of following the console
  --reconfigure       re-run `cmake --preset <preset>`
  -h, --help          this text
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --k1)           K1=1 ;;
    --touch)        UI=touch ;;
    --xbox)         UI=xbox ;;
    --pc)           UI=pc ;;
    --sim)          SIM=1 ;;
    --device)       DEVICE="${2:?--device needs a name or UDID}"; shift ;;
    --profile)      PROFILE="${2:?--profile needs a file}"; shift ;;
    --bundle-id)    BUNDLE_ID="${2:?--bundle-id needs an id}"; shift ;;
    --env)          RUN_ENV="${2?--env needs a value}"; shift ;;
    --env=*)        RUN_ENV="${1#*=}" ;;
    --no-build)     DO_BUILD=0 ;;
    --no-run)       DO_RUN=0 ;;
    --no-console)   CONSOLE=0 ;;
    --reconfigure)  RECONFIGURE=1 ;;
    -h|--help)      usage; exit 0 ;;
    *)              usage >&2; die "unknown argument: $1" ;;
  esac
  shift
done

# preset names: <k1|k2>-cross-ios-<ui>, -sim after "ios" for the Simulator
IOS=cross-ios; [ "$SIM" = 1 ] && IOS=cross-ios-sim
if [ "$K1" = 1 ]; then
  PRESET="k1-$IOS-$UI" EXE=k1port OBB_SRC="$REPO/decomp_bins/swkotor_obb"
else
  PRESET="k2-$IOS-$UI"
  EXE=k2port OBB_SRC="$REPO/decomp_bins/swkotorii_obb"
fi
BUILD_DIR="$PLAT/out/$PRESET"
APP="$BUILD_DIR/ios/$EXE.app"
command -v xcrun >/dev/null || die "xcrun not found: install Xcode"

# ---- build ---------------------------------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  CFG_ARGS=()
  [ -n "$BUNDLE_ID" ] && CFG_ARGS+=("-DK2I_BUNDLE_ID=$BUNDLE_ID")
  if [ "$RECONFIGURE" -eq 1 ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "${#CFG_ARGS[@]}" -gt 0 ]; then
    say "cmake --preset $PRESET ${CFG_ARGS[*]-}"
    ( cd "$REPO" && cmake --preset "$PRESET" "${CFG_ARGS[@]+"${CFG_ARGS[@]}"}" )
  fi
  say "cmake --build --preset $PRESET"
  ( cd "$REPO" && cmake --build --preset "$PRESET" )
  echo "    link report: $BUILD_DIR/report/linkreport.md"
fi
[ -d "$APP" ] || die "no app at $APP (drop --no-build, or build the '$PRESET' preset first)"
[ "$DO_RUN" -eq 1 ] || { say "done — $APP"; exit 0; }
[ -d "$OBB_SRC" ] || echo "warning: no OBBs at $OBB_SRC; copy them into the app's Documents yourself" >&2

# ---- run -----------------------------------------------------------------------------------------
BID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP/Info.plist")"
if [ "$SIM" = 1 ]; then
  say "Simulator: installing $BID with k2env \"$RUN_ENV\" (Ctrl-C detaches)"
  exec python3 "$PLAT/ios/tools/k2ipa.py" run-sim --app "$APP" --bundle-id "$BID" \
       --obb-dir "$OBB_SRC" --env "$RUN_ENV"
fi
ARGS=(--app "$APP" --obb-dir "$OBB_SRC" --env "$RUN_ENV")
[ -n "$DEVICE" ] && ARGS+=(--device "$DEVICE")
[ -n "$PROFILE" ] && ARGS+=(--profile "$PROFILE")
[ "$CONSOLE" -eq 0 ] && ARGS+=(--no-console)
say "device: signing, installing $BID, copying the OBBs (first time only) and launching"
exec python3 "$PLAT/ios/tools/k2ipa.py" run-device "${ARGS[@]}"
