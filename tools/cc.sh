#!/usr/bin/env bash
# Compile one KOTOR2 TU with the pinned NDK toolchain into build/<TU>.o
# Usage: tools/cc.sh src/<path>/<TU>.cpp   (run from repo root)
#
# Toolchain: NDK r21d (clang 9.0.8, r365631c2, API 23, arm64-v8a), which reproduces the shipped
# build; r21e is one patch later. NDK_DIR=/path overrides the unpacked toolchain/ copy.
set -euo pipefail

# Prefer the exact r21d if it's been unpacked into toolchain/, else fall back to installed r21e.
if [[ -z "${NDK_DIR:-}" ]]; then
  if [[ -d toolchain/android-ndk-r21d ]]; then
    NDK_DIR="$(cd toolchain/android-ndk-r21d && pwd)"
  else
    NDK_DIR="/Users/maxamillion/Library/Android/sdk/ndk/21.4.7075529"   # r21e fallback
  fi
fi
CXX="$NDK_DIR/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android23-clang++"

SRC="${1:?usage: tools/cc.sh src/<path>/<TU>.cpp}"
STEM="$(basename "${SRC%.cpp}")"
TU="$STEM"
# A stem two src/ files share names its object `<parent dir>_<stem>`; the `cflags/<stem>` file
# keeps the plain stem.
if [[ "$(find src -name "${STEM}.cpp" -not -path '*/.*' | wc -l)" -gt 1 ]]; then
  TU="$(basename "$(dirname "$SRC")")_${STEM}"
fi
OUT="${CC_OUTDIR:-build}/${TU}.o"
# CC_OUTDIR: an alternate object directory.
mkdir -p "${CC_OUTDIR:-build}"
# An IR dump must not invalidate the existing object (see IR_DUMP below).
[[ -n "${IR_DUMP:-}" ]] || rm -f "$OUT"

# Per-library flags: the tools/libflags.tsv row whose prefix is longest over the source's src/ path
# (a k1/k2 row beats a `*` row of the same length), appended to FLAGS below and before
# EXTRA_CFLAGS. A source compiled outside src/ takes the flags of CC_FLAGS_AS (a src/ path), else of
# the src/ file with its name, plus any `cflags/_all` and `cflags/<stem>` files in its own directory.
GAME=k2
[[ " ${EXTRA_CFLAGS:-} " == *" -DKOTOR1 "* ]] && GAME=k1
REL="${CC_FLAGS_AS:-${SRC#./}}"   # CC_FLAGS_AS=<src/ path>: a copy's flags are that file's
if [[ "$REL" != src/* ]]; then
  # the src/ file with this name; of several, the one whose directory has the copy's directory name
  REL="$(find src -name "${STEM}.cpp" -not -path '*/.*' | sort | awk -v d="/$(basename "$(dirname "$SRC")")/" \
    'NR == 1 { f = $0 } index($0, d) && !m { m = $0 } END { print m ? m : f }')"
fi
TU_CFLAGS="$(awk -F'\t' -v p="$REL" -v g="$GAME" '
  /^#/ || NF < 3 { next }
  ($1 == "*" || $1 == g) && index(p, $2) == 1 {
    n = length($2) * 2 + ($1 != "*")
    if (n > best) { best = n; f = $3 }
  }
  END { print f }' tools/libflags.tsv) "
read_cflags() {
  # `|| true`: a file of comments only leaves grep with no output and exit 1 (set -e would kill us).
  [[ -f "$1" ]] && grep -v '^[[:space:]]*#' "$1" | tr '\n' ' ' || true
}
if [[ "${SRC#./}" != src/* ]]; then
  TU_CFLAGS="$TU_CFLAGS$(read_cflags "$(dirname "$SRC")/cflags/_all")$(read_cflags "$(dirname "$SRC")/cflags/$STEM")"
fi
if [[ -n "${CF_MAP:-}" ]]; then   # CF_MAP: `<path prefix> <flags...>` lines, longest prefix wins
  TU_CFLAGS=""; best=0
  while read -r pfx rest; do
    [[ -z "$pfx" || "$pfx" == '#'* ]] && continue
    if [[ "$pfx" == k1:* ]]; then [[ "$GAME" == k1 ]] || continue; pfx="${pfx#k1:}"; fi
    if [[ "$pfx" == k2:* ]]; then [[ "$GAME" == k2 ]] || continue; pfx="${pfx#k2:}"; fi
    if [[ "$REL" == "$pfx"* && ${#pfx} -ge $best ]]; then best=${#pfx}; TU_CFLAGS="$rest "; fi
  done < "$CF_MAP"
fi
# KOTOR 1 is C++14: no `new T[n]` negative-count clamp (see the -std note below).
if [[ "$GAME" == k1 ]]; then
  TU_CFLAGS="$TU_CFLAGS -std=c++14"
fi

# Release-shaped flags: -fPIC (shared lib), -O2, exceptions + RTTI on (the build has .eh_frame and
# typeinfo symbols), one section per function and per datum.
# The -O level is a guess; revisit it if the codegen diverges (try -O3/-Oz).
# -fno-builtin: required. The engine treats libc functions (strlen/strcpy/memcpy/...) as fully
#   opaque calls — no builtin folding (strlen(s)!=0 -> s[0]!=0) and no pure-call CSE (two strlen
#   calls stay two). Declare CRT prototypes yourself (extern "C", no purity attributes): pulling
#   them from the NDK <string.h> re-adds __pure__ and defeats -fno-builtin for that call.
# -fsigned-char: required. Android's default plain `char` is unsigned; the engine's is signed, as on
#   Windows. Arguments that carry a plain `char` are sign-extended by the generated code, which is
#   impossible under the unsigned default without a cast the sources do not have. No source relies
#   on the opposite: engine-wide, it cost no other TU a single instruction.
# -fstack-protector-strong: the NDK arm64 default. The engine emits stack-canary prologues on
#   functions with local char arrays (e.g. CExoString::StripNonAlphaNumeric's char[128]); this flag
#   reproduces them. Leaf and accessor functions with no stack buffers are unaffected.
# -D_FORTIFY_SOURCE=2: the engine was built with fortify on. Turns bionic's string.h/stdio.h calls
#   into the __*_chk variants and makes the variadic sprintf inline out-of-line as a specialized
#   wrapper (needed for CExoString(int)). It does not perturb functions whose destination is a heap
#   buffer: __bos is -1 there, so fortify is a no-op.
# -funwind-tables: required. The NDK's aarch64-linux-android driver does not default this on for
#   C++ with -fexceptions (only the sync tables EH strictly needs), so without it clang emits no
#   .cfi for the frame-pointer setup, and the scheduler may then sink the frame-pointer add away
#   from the stp it belongs to. Do not drop it: -fno-unwind-tables and
#   -fno-asynchronous-unwind-tables both reproduce that sunk-prologue shape.
# -g: both shipped images were built with debug info and stripped. DBG_VALUEs are not free at
#   -O2/-O3: one between `add x0,x0,#1` and `ldrb w8,[x0]` stops the load/store optimizer forming
#   the pre-indexed `ldrb w8,[x0,#1]!`, and debug values move pre-RA scheduling. Measured 2026-10-26
#   tree-wide: K1 +16 / -2, K2 +2 / -1, and the losses were source spellings (a `float*` local
#   where the original kept the value; a hand-expanded `dot`), MATCH in both games once respelled.
#   So under -g a NAMED LOCAL is evidence: an extra or missing variable can now cost bytes.
FLAGS=(
  -fPIC -O2 -fexceptions -frtti -g \
  -funwind-tables \
  -ffunction-sections -fdata-sections \
  -fno-addrsig \
  -fno-builtin \
  -fsigned-char \
  -fstack-protector-strong \
  -D_FORTIFY_SOURCE=2 \
  `# The build is pre-C++14. clang emits the negative-count clamp for "new T[n]" with a 1-byte T
   # under -std=c++98 and -std=c++11 but not under -std=c++14, where a negative array size is
   # simply left to the sign extension to make huge. CSWMessage::ExtendWriteBuffer and its inliners
   # all carry that clamp, and no C++14 source spelling can produce it: the frontend never emits
   # the select. Measured engine-wide, -std=c++11 changes nothing outside CSWMessage. This
   # construct does not tell c++98 from c++11; c++11 is the later of the two that reproduces it.` \
  -std=c++11 \
  -DNDEBUG \
  `# K2_MOBILE_OPT: the mobile/console port's own cuts are in effect. The Android/Switch build has
   # no body for a handful of functions the PC-lineage (macOS) build still has. src/ carries each
   # restored body behind an #ifndef of this, so defining it here keeps the matching build
   # compiling the shipped shapes. Byte-neutral by construction: the guarded code is simply not
   # compiled.
   # The hybrid APK inherits it through hybrid/tools/ccflags.py (it must behave like the shipped
   # build); platforms/ builds its own flag list and leaves it undefined, which is how the host
   # build gets the real bodies back.` \
  -DK2_MOBILE_OPT=1 \
  `# K2_UI_TOUCH: the shipped build presents the touch UI. src/ forks a handful of bodies on which
   # UI the build presents (touch / pc / xbox -- CMakeLists.txt K2P_UI): the Android-only
   # touch body under #ifdef K2_UI_TOUCH, the PC-lineage one (shared with the console) in the #else.
   # Independent of K2_MOBILE_OPT, which is only the port's cut bodies. Byte-neutral. The hybrid
   # APK inherits it too -- its console UI is hybrid/overlay/.` \
  -DK2_UI_TOUCH=1 \
  -I src \
  `# directories that keep a class's headers beside its .cpp` \
  -I src/game/servercore -I src/libsource/AURORA -I src/libsource/namegen \
  -I src/libsource/math3d \
  `# include/ roots: headers live in their own directory and are included by basename, as the
   # shipped build did` \
  -I src/include \
  -I src/game/clientcore/include \
  -I src/game/servercore/include \
  -I src/libsource/exobase/include \
  -I src/libsource/exoinput/include \
  -I src/libsource/exosound/include \
  -I src/libsource/exoresman/include \
  -I src/libsource/nwscript/include \
  -I src/libsource \
  -I src/game/swmain \
  -I src/port/ASL2/OpenGLES \
  -I src/port/ASL2/Win \
  -I src/port/ASL2/Mac \
  -I src/libsource/SWGui \
  -I src/port/ios \
  -I src/libsource/aurorainterface \
  -I src/libsource/AURORA/_instances \
  -I src/port/AURORA \
  -I src/libsource/exobase/_instances \
  -I src/libsource/exomemman \
  -I src/libsource/exomovieplayer \
  -I src/libsource/exoresman/_instances \
  -I src/port/fmod \
  -I src/port/android \
  -I src/port/Mac \
)

# IR_DUMP=1 dumps the frontend IR (no optimization passes) instead of an object: every
# `landingpad` block, its `unwind label %N` predecessors and the alloca a `llvm.lifetime.end` in
# its body names. It reuses this file's flag set (the per-TU `cflags` included) on purpose: an
# approximation of the command line can mint or lose landing pads. Output: $IR_OUT, /tmp/<TU>.ll.
if [[ -n "${IR_DUMP:-}" ]]; then
  IR_OUT="${IR_OUT:-/tmp/${TU}.ll}"
  # IR_OPTIMIZED=1: the -O2 IR the backend sees, with -g locations.
  IR_PASSES=(-Xclang -disable-llvm-passes)
  [[ -n "${IR_OPTIMIZED:-}" ]] && IR_PASSES=(-g)
  "$CXX" "${FLAGS[@]}" ${TU_CFLAGS} ${EXTRA_CFLAGS:-} \
    "${IR_PASSES[@]}" -S -emit-llvm -o "$IR_OUT" "$SRC"
  echo "wrote $IR_OUT ($(wc -c < "$IR_OUT") bytes, frontend IR)  [NDK: $(basename "$NDK_DIR")]${TU_CFLAGS:+  [per-TU cflags: $TU_CFLAGS]}"
  exit 0
fi

"$CXX" "${FLAGS[@]}" ${TU_CFLAGS} ${EXTRA_CFLAGS:-} -c "$SRC" -o "$OUT"

echo "built $OUT ($(wc -c < "$OUT") bytes)  [NDK: $(basename "$NDK_DIR")]${TU_CFLAGS:+  [per-TU cflags: $TU_CFLAGS]}"
