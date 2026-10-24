# Host-platform build of the engine (run cmake from the repo root; the ports live in platforms/, and
# every build dir is platforms/out/<preset>).
#
#   cmake --preset k2-host-pc            # configure for this computer (platforms/cmake/host-<OS>.cmake)
#   cmake --build --preset k2-host-pc    # compile every src/**/*.cpp, link, write the interface report
#   cmake --preset k2-cross-android-touch && cmake --build --preset k2-cross-android-touch   # an APK
#   cmake --preset k2-cross-windows-pc && cmake --build --preset k2-cross-windows-pc   # Win64, llvm-mingw
# Presets are <k1|k2>-host-<ui> (native: macOS, Linux, Windows with cl.exe) and
# <k1|k2>-cross-<target>-<ui> (windows = llvm-mingw, windows-msvc = clang-cl, windows-cl = cl.exe under Wine, ios,
# ios-sim, android).
#
# The engine is built by the host compiler, without the NDK and the device build's codegen flags, to
# answer two questions: does it compile as ordinary C++ on a desktop target, and what does it need
# from the outside world -- every symbol it references but does not define, classified into engine
# code versus the platform interfaces (GL, SDL/ASL, audio, video, JNI, libc) a port must stub or
# reimplement. K2P_KEEP_GOING_LINK still links an executable, with generated abort stubs for whatever
# is missing.
#
# Every output lands in the binary dir.
cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0141 NEW)   # MSVC: CMAKE_MSVC_DEBUG_INFORMATION_FORMAT (/Zi or /Z7)
project(kotor2_platforms LANGUAGES C CXX ASM)

set(K2_REPO "${CMAKE_CURRENT_SOURCE_DIR}")
set(K2P_DIR "${K2_REPO}/platforms")
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

set(K2P_EXCLUDE_TUS "" CACHE STRING "TU basenames left out of the engine library (;-list)")
option(K2P_KEEP_GOING_LINK "Link the smoke executable with generated abort-stubs for every unresolved symbol" ON)

# ---- the engine's source list + include dirs, straight from tools/cc.sh -------------------------
# platforms/tools/ccflags.py parses cc.sh so the TU set and -I list stay in lockstep with the device
# build. Its per-TU `cflags` are NDK codegen knobs, which are dropped below.
set(_flags_cmake "${CMAKE_BINARY_DIR}/k2flags.cmake")
set(_exclude_args)
foreach(_tu IN LISTS K2P_EXCLUDE_TUS)
  list(APPEND _exclude_args --exclude-tu "${_tu}")
endforeach()
execute_process(COMMAND "${Python3_EXECUTABLE}" "${K2_REPO}/platforms/tools/ccflags.py"
                        --repo "${K2_REPO}" --out "${_flags_cmake}" ${_exclude_args}
                RESULT_VARIABLE _rc OUTPUT_QUIET)
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "ccflags.py failed")
endif()
include("${_flags_cmake}")
file(GLOB_RECURSE _k2_watch CONFIGURE_DEPENDS "${K2_REPO}/src/*.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2_REPO}/tools/cc.sh")
foreach(_s IN LISTS K2_SOURCES)
  set_source_files_properties("${_s}" PROPERTIES COMPILE_FLAGS "")   # drop the NDK per-TU flags
endforeach()
list(LENGTH K2_SOURCES _n)
message(STATUS "k2: ${_n} engine TUs")

# ---- host compile flags -------------------------------------------------------------------------
# The device build's semantic flags that still matter on a host:
#   -fsigned-char   the engine's plain char is signed (the usual host targets default to unsigned)
#   c++11 dialect   the engine predates C++14 (new T[n] clamp); gnu++ for the extensions it uses
# Everything else (fortify, -fno-builtin, stack protector, sections) belongs to the device build.
# ---- K2P_COV: runtime coverage of the engine (host builds) ---------------------------------------
# Off by default: a call per instrumented site, so it is a slower game -- fine for a play session,
# not for frame-rate-sensitive play.
option(K2P_COV "Record runtime coverage of the engine rows (-finstrument-functions)" OFF)
if(K2P_COV)
  add_compile_definitions(K2P_COV=1)
endif()
message(STATUS "k2: K2P_COV ${K2P_COV}")
# ---- K2P_ASAN: AddressSanitizer (presets <k1|k2>-host-pc-asan) -----------------------------------
# clang hosts (macOS, Linux) here: every C and C++ target, asan_ignore.txt's functions excepted, with
# reports at the log_path build_run_macos.sh sets. cl.exe's /fsanitize=address is
# windows/CMakeLists.txt.
option(K2P_ASAN "Build with AddressSanitizer (clang hosts: -fsanitize=address; cl.exe: /fsanitize=address)" OFF)
if(K2P_ASAN AND NOT WIN32)
  set(_k2_asan -fsanitize=address -fno-omit-frame-pointer "-fsanitize-ignorelist=${K2P_DIR}/asan_ignore.txt")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:${_k2_asan}>")
  add_link_options(-fsanitize=address)
  message(STATUS "k2: AddressSanitizer on")
endif()

set(K2_HOST_FLAGS
    -fsigned-char
    # The device build has no fused multiply-adds, while modern clang contracts a*b+c by default,
    # which rounds differently and flips exact ties (a door the player can never approach).
    -ffp-contract=off
    -w                                  # the engine's style draws warnings from current toolchains
    -Wno-error
    -fno-exceptions                      # CRT redeclared with throw() against the SDK's
    -include "${K2P_DIR}/compat/k2host_prelude.h")
# The MSVC toolset (the Windows host presets and *-cross-windows-msvc/-cl): the same, spelled for cl
# and clang-cl. Plain char is signed there already, and /vmg keeps one member-pointer representation
# for every class, since the GUI and action-menu handler tables cast between classes' member
# pointers.
if(MSVC)
  set(K2_HOST_FLAGS /w /EHs-c- /vmg "/FI${K2P_DIR}/compat/k2host_prelude.h")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND K2_HOST_FLAGS /clang:-ffp-contract=off)
  else()
    # cl.exe: no contraction under /fp:precise without /fp:contract; /bigobj for the largest TUs.
    list(APPEND K2_HOST_FLAGS /fp:precise /bigobj)
  endif()
endif()

# Instrumented after the base flags, so the coverage flags are the last word on the command line.
if(K2P_COV)
  list(APPEND K2_HOST_FLAGS -finstrument-functions)
endif()

# Three kinds of #ifdef in src/. The first two are off in the device build; the third is the other
# way round: tools/cc.sh defines it, and leaving it undefined here is the point.
#   K2_HOST_BUILD       platform and ABI differences only (bionic struct sizes, missing host
#                       headers); engine behaviour is unchanged
#   K2_FIX_ENGINE_BUGS  fixes for real bugs and undefined behaviour in the engine; off reproduces
#                       the shipped behaviour exactly. Also turns on the narrow K2_FIX_* macros the
#                       hybrid APK enables one by one: K2_FIX_INVENTORY_TAB_ORDER, so the pad walks
#                       the inventory tabs in screen order, and K2_FIX_WRAP_ROUNDING, so WrapStrings
#                       measures glyphs the way Draw advances them and text at a non-integer size --
#                       the Dialog Text option -- wraps inside its box
#   K2_MOBILE_OPT       the mobile port's own cuts are in effect. Defined by tools/cc.sh and
#                       inherited by the hybrid APK, so the device build compiles the shipped
#                       stubs; undefined here, which is how this build gets the cut bodies back:
#                         * PartDanglyMesh::Animate's pendant physics
#                         * the emitters' full birthrates
#                         * KOTOR II's full-size scene buffer (the port drew 7/10 of a 2048+ wide
#                           screen)
#                         * the size-over-distance culls past 15 m (rooms, objects, emitters,
#                           lightsaber blades)
#                         * K1's fog-end far clip and per-area fog-off table
#                       Turn K2P_MOBILE_OPT on to compare against the shipped behaviour, or for
#                       frame rate on a weak GPU.
# One more, orthogonal to the three: which UI the build is (K2P_UI, below). K2_MOBILE_OPT covers
# only the port's performance and content cuts; it says nothing about how the game is driven.
option(K2P_FIX_ENGINE_BUGS "Compile the engine with K2_FIX_ENGINE_BUGS (fixes for original-engine bugs/UB)" ON)
option(K2P_MOBILE_OPT "Compile the engine with K2_MOBILE_OPT (keep the mobile port's cut bodies cut)" OFF)

# ---- which game: K2P_GAME = k2 | k1 -------------------------------------------------------------
# k1 compiles the same tree with KOTOR1 defined, for KOTOR 1's libKOTOR.so instead of KOTOR II's
# libkotor2.so. KOTOR1 goes on the engine's definitions (K2_ENGINE_DEFS, below), so it reaches the
# engine and everything compiled with its flags (the console code, the K1 port layer). The shim is
# not compiled with it: the shim is KOTOR II's port layer (ASL), with plain C names, and K1 reuses
# its OS half through k1/, the port of K1's own libandroid_port.so.
# K1 is C++14, which has no `new T[n]` negative-count clamp.
#   K2P_ORIG_LIBDIR / K2P_ORIG_SO   the shipped Android libraries linkreport classifies against
#   K2P_MAC_ORACLE / K2P_DATA_HOMES  its symbolized macOS build and unexported-data table (K2 only)
set(K2P_GAME "k2" CACHE STRING "Game the build is: k2 (KOTOR II) or k1 (KOTOR, KOTOR1 defined)")
set_property(CACHE K2P_GAME PROPERTY STRINGS k2 k1)
if(K2P_GAME STREQUAL "k1")
  set(K2P_GAME_DEFS KOTOR1=1)
  set(K2P_ENGINE_STD 14)
  set(K2P_ORIG_LIBDIR "${K2_REPO}/decomp_bins/swkotor_extracted/lib/arm64-v8a")
  set(K2P_ORIG_SO "${K2P_ORIG_LIBDIR}/libKOTOR.so")
  set(K2P_MAC_ORACLE "")
  set(K2P_DATA_HOMES "")
  # K2_QOL_IMPROVEMENTS (below) is KOTOR II's; K1 gets only its Controller page (K2_QOL_CONTROLLER).
  set(K2P_QOL_IMPROVEMENTS OFF)
elseif(K2P_GAME STREQUAL "k2")
  set(K2P_GAME_DEFS)
  set(K2P_ENGINE_STD 11)
  set(K2P_ORIG_LIBDIR "${K2_REPO}/decomp_bins/swkotorii_extracted/lib/arm64-v8a")
  set(K2P_ORIG_SO "${K2P_ORIG_LIBDIR}/libkotor2.so")
  set(K2P_MAC_ORACLE "${K2_REPO}/decomp_bins/KOTOR2_MacOS/KOTOR2sub")
  set(K2P_DATA_HOMES "${K2_REPO}/docs/DATA_HOMES.md")
else()
  message(FATAL_ERROR "K2P_GAME must be k2 or k1 (got '${K2P_GAME}')")
endif()
message(STATUS "k2: game ${K2P_GAME}")
# The link report -- what the engine leaves unresolved, classified against the shipped binaries --
# is an analysis, not a link input: it runs as its own target, k2_linkreport, when the binaries are
# in decomp_bins/, and the executable links without it.
if(EXISTS "${K2P_ORIG_SO}")
  set(_k2p_linkreport_default ON)
else()
  set(_k2p_linkreport_default OFF)
endif()
option(K2P_LINKREPORT "Run tools/linkreport.py in the build (needs the shipped binaries in decomp_bins/)"
       ${_k2p_linkreport_default})
if(K2P_LINKREPORT AND NOT EXISTS "${K2P_ORIG_SO}")
  message(WARNING "K2P_LINKREPORT: ${K2P_ORIG_SO} is missing; the link report is off")
  set(K2P_LINKREPORT OFF)
endif()
if(NOT K2P_LINKREPORT)
  message(STATUS "k2: link report off (it needs ${K2P_ORIG_SO})")
endif()
# The desktop executable is named for its game: k2 (k2.exe) or k1 (k1.exe).
set(K2P_EXE "${K2P_GAME}")

# ---- the UI the build presents: K2P_UI = pc | touch | xbox ---------------------------------------
# Build-time, one per binary dir (presets k2-host-pc, k2-host-touch, k2-host-xbox). The macros are
# defined for every target, since the engine, the shim and the stubs all branch on them:
#   K2_UI_TOUCH   the Android port's touch UI: GetIosControl's virtual stick feeds walking, the SDL
#                 hook (K2_MOUSE_TOUCH) is on by default, HasTouchScreen() answers 1. tools/cc.sh
#                 defines it, as the shipped library is the touch build, so src/ guards the PC-lineage
#                 replacement for an Android-only body with `#ifndef K2_UI_TOUCH`.
#   K2_UI_XBOX    the console UI (Xbox and Switch: the `_x` layouts and the action-bar driver). Code
#                 that exists only on the console goes under `#ifdef K2_UI_XBOX`; code PC and console
#                 share (the dialog and computer screens, the keyboard and stick walk axis) is
#                 `#ifndef K2_UI_TOUCH`. The hybrid APK compiles src/ with K2_UI_XBOX too
#                 (hybrid/CMakeLists.txt K2H_UI). The console input driver is console/k2console.cpp
#                 (the port of hybrid/runtime/k2console.cpp) over the pad shim/shim_dinput.cpp
#                 exposes, and the name keyboard is console/k2namekeyboard.cpp. The hybrid's
#                 Controller page is K2_QOL_IMPROVEMENTS (below), in every UI.
#   K1_UI_XBOX    KOTOR 1's console UI (K2P_GAME=k1): the PC-lineage code (`#ifndef K2_UI_TOUCH`) and
#                 K1's own pad path, but none of KOTOR II's console code, which is written against
#                 its seven-box `maininterface_x` bar and `_x` layouts that K1's data does not have --
#                 except console/k2namekeyboard.cpp, whose K1 branch builds the keyboard from K1's
#                 own `name` layout.
#   K2_UI_PC      pc, the default: the PC lineage (the macOS Steam build is its reference). Code that
#                 only the PC HUD has (the `mipc28x6_p` menu bar and per-box arrows) goes under
#                 `#ifdef K2_UI_PC`; tools/cc.sh never defines it.
# Platforms combine it with K2P_MOBILE_OPT: Android = touch + mobile opt, Switch = xbox + mobile opt.
set(K2P_UI "pc" CACHE STRING "UI the build presents: pc, touch or xbox")
set_property(CACHE K2P_UI PROPERTY STRINGS pc touch xbox)
if(K2P_UI STREQUAL "touch")
  add_compile_definitions(K2_UI_TOUCH=1)
elseif(K2P_UI STREQUAL "xbox" AND K2P_GAME STREQUAL "k1")
  add_compile_definitions(K1_UI_XBOX=1)
elseif(K2P_UI STREQUAL "xbox")
  add_compile_definitions(K2_UI_XBOX=1)
elseif(K2P_UI STREQUAL "pc")
  add_compile_definitions(K2_UI_PC=1)
else()
  message(FATAL_ERROR "K2P_UI must be pc, touch or xbox (got '${K2P_UI}')")
endif()
message(STATUS "k2: UI ${K2P_UI}, K2_MOBILE_OPT ${K2P_MOBILE_OPT}")

# ---- K2_QOL_IMPROVEMENTS: the hybrid APK's Controller options page, ported ----------------------
# Features the engine did not have, all reached from Main menu -> Options -> Controller:
#   * button-prompt art (Automatic, Xbox, PlayStation, Switch, GameCube)
#   * Disable Right Stick Pitch
#   * rumble (Controller, Device, Off), with rumble.2da loaded as the Switch does
#   * the dialog text size
# Pieces: platforms/qol/ (the page, the settings, the motors) plus `#ifdef K2_QOL_IMPROVEMENTS` hooks
# in src/ (tools/cc.sh and the hybrid never define it, so their objects are unchanged). Defined for
# every target, like the UI macros: the engine, the shim and the platform mains all branch on it.
option(K2P_QOL_IMPROVEMENTS "Compile the Controller options page and its features (K2_QOL_IMPROVEMENTS)" ON)
if(K2P_QOL_IMPROVEMENTS)
  add_compile_definitions(K2_QOL_IMPROVEMENTS=1)
endif()
message(STATUS "k2: K2_QOL_IMPROVEMENTS ${K2P_QOL_IMPROVEMENTS}")
# K2_QOL_CONTROLLER: the part of it KOTOR 1 shares -- the Controller page with the button prompts
# and rumble rows, the prompt art and the motors, but not the text size or the stick pitch. Defined
# wherever K2_QOL_IMPROVEMENTS is, and in every K1 build. K2_QOL_K1 tells qol/k2qol.cpp, compiled
# without KOTOR1, which game's art and settings file it serves.
if(K2P_QOL_IMPROVEMENTS OR K2P_GAME STREQUAL "k1")
  set(K2P_QOL_CONTROLLER ON)
  add_compile_definitions(K2_QOL_CONTROLLER=1)
  if(K2P_GAME STREQUAL "k1")
    add_compile_definitions(K2_QOL_K1=1)
  endif()
else()
  set(K2P_QOL_CONTROLLER OFF)
endif()
# The identical-one-shot stacking limit (exosoundsourceinternal.cpp) is game-neutral engine code, so
# KOTOR 1 gets it too, even though the rest of K2_QOL_IMPROVEMENTS (the Controller page) is K2-only.
if(K2P_QOL_IMPROVEMENTS OR K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_SOUND_STACK=1)
endif()
# KOTOR 1: the Upper City cantina / dueling ring's fog (10 / 25 m in the area data) is pushed out to
# 20 / 45 m (Scene::DoFog, aurscene.cpp).
if(K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_CANTINA_FOG=1)
endif()
# KOTOR 1: a looted container's target name reads "<name> (Empty)", as in KOTOR II
# (CSWGuiTargetActionMenu::UpdateNameLabel, swguimaininterfacepc2.cpp).
if(K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_EMPTY_CONTAINERS=1)
endif()
# KOTOR 1: with Frame Buffer Effects on, stealth and Force Push distortion are drawn (the PC's
# RenderForceDistortion as a pass over the scene pbuffer: FrameBufferModificationsEndIos in
# aurglcore.cpp, k1/k1kotorgl.cpp's distortion program) instead of the APK's always-on stealth shell.
if(K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_K1_DISTORTION=1)
endif()
# KOTOR 1: Force Speed's motion blur (the PC's accumulation blur: FrameBufferModificationsEndIos in
# aurglcore.cpp, k1/k1kotorgl.cpp's speed-blur program), which the APK never drew.
if(K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_K1_SPEED_BLUR=1)
endif()
# KOTOR 1: Advanced Graphics' anti-aliasing row, which the APK greyed out because it asked for a
# multisample extension it never used, its AA being the supersampled scene pbuffer. Offers 2x (the
# APK's 1.4 per axis) and 4x (2 per axis), and frees the old pbuffers on a re-init (aurglcore.cpp
# K1SceneBufferScale / K1DeletePBuffer).
if(K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_K1_ANTIALIAS=1)
endif()
# Both games: Advanced Graphics' soft shadows, which both APKs left unavailable, every soft-shadow
# body empty: the PC's image-space pass, the stencil shadows drawn as a mask, knocked down to
# 512x512, blurred and laid over the scene pbuffer (aurglcore.cpp QolSoftShadowsSetUp /
# QolSoftShadowsFinish; k1/k1kotorgl.cpp's copy and blur programs, and for KOTOR II
# src/port/ASL2/OpenGLES/KotorProgram.cpp).
if(K2P_QOL_IMPROVEMENTS OR K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_SOFT_SHADOWS=1)
endif()
# KOTOR II: Advanced Graphics' Frame Buffer Effects, greyed out on the APK, whose post-process always
# ran. Live, and off it leaves out the bloom and the Force Speed blur, as the PC's switch does
# (FrameBufferModificationsEndIos in aurglcore.cpp).
if(K2P_QOL_IMPROVEMENTS AND NOT K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_FRAME_BUFFER_EFFECTS=1)
endif()
# Both games: Advanced Graphics gains rows under V-Sync -- KOTOR II's Force Speed Effects and 60 fps
# framerate lock, which the APK put on the Graphics page, and the framerate lock for K1, which never
# had one (swguioptions.cpp K2QolAddRows; k1/k1sdl.cpp caps the swap).
if(K2P_QOL_IMPROVEMENTS OR K2P_GAME STREQUAL "k1")
  add_compile_definitions(K2_QOL_ADVANCED_ROWS=1)
endif()
# Both games, every host: the framerate lock's wait keeps a fixed schedule instead of sleeping
# SDL_Delay's whole milliseconds since the last frame, and on Android shows each frame at the end of
# its slot (EGL_ANDROID_presentation_time), so 60 fps on a 120 Hz panel is every second vsync
# (shim/shim_time.cpp k2host_pace_frame; KOTOR II's WinMain, K1's SDL_main in MacMain.cpp).
add_compile_definitions(K2_QOL_FRAME_PACING=1)
# Both games, desktop builds: a Fullscreen row on the Graphics page (swguioptions.cpp
# K2QolAddFullscreenRow). KOTOR II switches the host window at once (shim/shim_window.cpp: the
# fullscreen size below a notch, at the display's native resolution); K1 does the same from the main
# menu (swguioptions.cpp K1QolApplyDisplay, k1/k1sdl.cpp), and in a loaded game at the next launch.
# K1's Screen Resolution lists the host's window sizes and applies the same way; its V-Sync row drives
# the swap interval (k1sdl.cpp k2host_set_vsync).
if((K2P_QOL_IMPROVEMENTS OR K2P_GAME STREQUAL "k1") AND NOT ANDROID AND NOT IOS)
  add_compile_definitions(K2_QOL_FULLSCREEN=1)
endif()
# KOTOR 1, xbox UI: OK / Cancel boxes (answered by A / B) open with neither button selected,
# not Cancel (CSWGuiMessageBox::OnPanelAdded, swguiingame.cpp).
if(K2P_GAME STREQUAL "k1" AND K2P_UI STREQUAL "xbox")
  add_compile_definitions(K2_QOL_CONFIRM_PAD=1)
endif()

add_library(k2_engine STATIC ${K2_SOURCES})
set_target_properties(k2_engine PROPERTIES CXX_STANDARD ${K2P_ENGINE_STD} CXX_EXTENSIONS ON)
target_include_directories(k2_engine PRIVATE "${K2P_DIR}/compat/include" ${K2_INCLUDE_DIRS})
target_compile_options(k2_engine PRIVATE ${K2_HOST_FLAGS})
# Android NDK r26+: its libc++ tags placement new / delete with abi_tag too (r25's does not), and the
# engine headers' redeclarations (below) then fail to compile.
if(ANDROID)
  add_compile_definitions(_LIBCPP_NO_ABI_TAG=1)
endif()
# Linux (glibc + libc++): glibc's C++ string.h overloads and stdio guard, and libc++'s ABI tags on
# its placement new, which several engine headers redeclare before <new> (_instances/List.h).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
  set(K2P_GLIBC_INCLUDE "${K2P_DIR}/compat/include-glibc")
  target_include_directories(k2_engine BEFORE PRIVATE "${K2P_GLIBC_INCLUDE}")
  add_compile_definitions(_LIBCPP_NO_ABI_TAG=1)
  # AurLogFile's fopen: on Darwin libc++'s <algorithm> brings <stdio.h> in, on glibc it does not.
  set_property(SOURCE "${K2_REPO}/src/libsource/AURORA/auremitter.cpp" APPEND PROPERTY COMPILE_OPTIONS -include stdio.h)
endif()
# Windows (mingw-w64 UCRT + libc++): compat/include-mingw/ is the CRT as the engine expects to see
# it.
if(WIN32 AND MSVC)
  # The MSVC UCRT: copies of the UCRT headers with the CRT names the engine declares itself renamed
  # out of the way (mingw_crt.py --ucrt), the POSIX surface the UCRT lacks (compat/include-msvc/,
  # bodies windows/win_msvc_posix.cpp) and compat/include-mingw/'s additions. _CONST_RETURN empty:
  # the UCRT's C string prototypes return non-const, as the engine declares them.
  if(K2_MSVC_UCRT)
    set(_k2_ucrt "${K2_MSVC_UCRT}")
  elseif(K2_MSVC_SDK)
    set(_k2_ucrt "${K2_MSVC_SDK}/sdk/include/ucrt")
  else()
    string(REPLACE "\\" "/" _k2_inc_env "$ENV{INCLUDE}")
    foreach(_d IN LISTS _k2_inc_env)
      if(_d MATCHES "/ucrt/?$")
        set(_k2_ucrt "${_d}")
      endif()
    endforeach()
  endif()
  if(NOT EXISTS "${_k2_ucrt}/string.h")
    message(FATAL_ERROR "k2: the UCRT headers were not found (K2_MSVC_SDK, or a VS developer environment's INCLUDE)")
  endif()
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/windows/mingw_crt.py" --ucrt
                          --sysroot-include "${_k2_ucrt}" --out "${CMAKE_BINARY_DIR}/gen/msvc-crt"
                  RESULT_VARIABLE _rc)
  if(_rc)
    message(FATAL_ERROR "mingw_crt.py --ucrt failed")
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2P_DIR}/windows/mingw_crt.py")
  set(K2P_MINGW_INCLUDE "${K2P_DIR}/compat/include-msvc" "${K2P_DIR}/compat/include-mingw"
                        "${CMAKE_BINARY_DIR}/gen/msvc-crt")
  # cl.exe has no #include_next: the same layers, chained by relative includes (windows/cl_overlay.py).
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_k2_layers)
    foreach(_l IN LISTS K2P_MINGW_INCLUDE)
      list(APPEND _k2_layers --layer "${_l}")
    endforeach()
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/windows/cl_overlay.py" ${_k2_layers}
                            --system "${_k2_ucrt}" --out "${CMAKE_BINARY_DIR}/gen/cl-overlay"
                    OUTPUT_VARIABLE _k2_ov OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _rc)
    if(_rc)
      message(FATAL_ERROR "cl_overlay.py failed")
    endif()
    separate_arguments(K2P_MINGW_INCLUDE UNIX_COMMAND "${_k2_ov}")
    file(GLOB_RECURSE _k2_ovdeps CONFIGURE_DEPENDS "${K2P_DIR}/compat/include-msvc/*.h" "${K2P_DIR}/compat/include-mingw/*.h")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2P_DIR}/windows/cl_overlay.py" ${_k2_ovdeps})
  endif()
  set(K2P_MINGW_ISYSTEM)
  target_include_directories(k2_engine BEFORE PRIVATE ${K2P_MINGW_INCLUDE})
  # _ACRTIMP empty: the UCRT's functions are called by their plain names, not through __imp_, so the
  # shim's own fopen / remove / _strnicmp ... (the engine's view of the CRT) are the definitions the
  # link takes, as on mingw; an __imp_ reference would pull the import library's thunk and its
  # second definition.
  # (C and C++ only: llvm-rc reads an empty definition's "" as a second input file.)
  add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:_CONST_RETURN=> $<$<COMPILE_LANGUAGE:C,CXX>:_ACRTIMP=>
                          _CRT_SECURE_NO_WARNINGS
                          _CRT_NONSTDC_NO_WARNINGS
                          __PRFCHWINTRIN_H)   # SDL_endian.h's _m_prefetch stand-in is a builtin now
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/vmg>)
  # The engine's TUs see the CRT names they declare themselves renamed in the copies; sprintf_s only
  # in the TUs that define a file-local one (the MSVC STL's <string> calls ::sprintf_s).
  target_compile_definitions(k2_engine PRIVATE K2_ENGINE_CRT=1)
  foreach(_s IN LISTS K2_SOURCES)
    file(STRINGS "${_s}" _k2_sp REGEX "^static int sprintf_s\\(")
    if(_k2_sp)
      set_property(SOURCE "${_s}" APPEND PROPERTY COMPILE_DEFINITIONS K2_ENGINE_SPRINTF_S=1)
    endif()
  endforeach()
  foreach(_k2_inc IN ITEMS
          "libsource/AURORA/auremitter.cpp:stdio.h,math.h" "libsource/AURORA/aurmodel.cpp:stdio.h"
          "game/clientcore/clientinput.cpp:math.h" "port/android/AndroidJNI.cpp:stdlib.h"
          "game/clientcore/clientexoappinternal.cpp:stdlib.h" "game/clientcore/swguiingame.cpp:math.h"
          "game/servercore/nwvmachinecommands.cpp:math.h" "game/servercore/serverexoappinternal.cpp:stdlib.h")
    string(REPLACE ":" ";" _k2_inc "${_k2_inc}")
    list(GET _k2_inc 0 _k2_tu)
    list(GET _k2_inc 1 _k2_hdrs)
    string(REPLACE "," ";" _k2_hdrs "${_k2_hdrs}")
    foreach(_k2_h IN LISTS _k2_hdrs)
      set_property(SOURCE "${K2_REPO}/src/${_k2_tu}" APPEND PROPERTY COMPILE_OPTIONS "/FI${_k2_h}")
    endforeach()
  endforeach()
elseif(WIN32)
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/windows/mingw_crt.py"
                          --sysroot-include "${K2_LLVM_MINGW}/x86_64-w64-mingw32/include"
                          --out "${CMAKE_BINARY_DIR}/gen/mingw-crt"
                  RESULT_VARIABLE _rc)
  if(_rc)
    message(FATAL_ERROR "mingw_crt.py failed")
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2P_DIR}/windows/mingw_crt.py")
  # The copies sit between libc++'s headers and the sysroot's: libc++'s <string.h> wrapper must be
  # the one <cstring> finds, and its #include_next reaches the copy. Hence -isystem, libc++ first.
  set(K2P_MINGW_INCLUDE "${K2P_DIR}/compat/include-mingw")
  set(K2P_MINGW_ISYSTEM "SHELL:-isystem \"${K2_LLVM_MINGW}/x86_64-w64-mingw32/include/c++/v1\""
                        "SHELL:-isystem \"${CMAKE_BINARY_DIR}/gen/mingw-crt\"")
  target_include_directories(k2_engine BEFORE PRIVATE ${K2P_MINGW_INCLUDE})
  target_compile_options(k2_engine PRIVATE ${K2P_MINGW_ISYSTEM})
  # TUs that call CRT functions they never declare, which a Darwin / bionic / glibc header they
  # include happens to declare (as auremitter.cpp's fopen on glibc, above).
  foreach(_k2_inc IN ITEMS
          "libsource/AURORA/auremitter.cpp:stdio.h,math.h" "libsource/AURORA/aurmodel.cpp:stdio.h"
          "game/clientcore/clientinput.cpp:math.h" "port/android/AndroidJNI.cpp:stdlib.h"
          "game/clientcore/clientexoappinternal.cpp:stdlib.h" "game/clientcore/swguiingame.cpp:math.h"
          "game/servercore/nwvmachinecommands.cpp:math.h" "game/servercore/serverexoappinternal.cpp:stdlib.h")
    string(REPLACE ":" ";" _k2_inc "${_k2_inc}")
    list(GET _k2_inc 0 _k2_tu)
    list(GET _k2_inc 1 _k2_hdrs)
    string(REPLACE "," ";" _k2_hdrs "${_k2_hdrs}")
    foreach(_k2_h IN LISTS _k2_hdrs)
      set_property(SOURCE "${K2_REPO}/src/${_k2_tu}" APPEND PROPERTY COMPILE_OPTIONS -include ${_k2_h})
    endforeach()
  endforeach()
  add_compile_definitions(_LIBCPP_NO_ABI_TAG=1)
endif()
set(K2_ENGINE_DEFS K2_HOST_BUILD=1
    $<$<BOOL:${K2P_FIX_ENGINE_BUGS}>:K2_FIX_ENGINE_BUGS=1>
    $<$<BOOL:${K2P_FIX_ENGINE_BUGS}>:K2_FIX_INVENTORY_TAB_ORDER=1>
    $<$<BOOL:${K2P_FIX_ENGINE_BUGS}>:K2_FIX_WRAP_ROUNDING=1>
    $<$<BOOL:${K2P_MOBILE_OPT}>:K2_MOBILE_OPT=1>
    ${K2P_GAME_DEFS})
target_compile_definitions(k2_engine PRIVATE ${K2_ENGINE_DEFS})
# iOS forbids system(); the one engine TU that names it (the Android cloud-save JNI, dead on iOS)
# gets a stub instead (ios/ios_nosystem.h, ios/ios_compat.cpp).
if(IOS)
  set_property(SOURCE "${K2_REPO}/src/port/android/AndroidJNI.cpp" APPEND PROPERTY COMPILE_OPTIONS
               -include "${K2P_DIR}/ios/ios_nosystem.h")
endif()
# K1's model parser rejects an orphan node with `throw NULL` (nothing catches it: a terminate, as on
# the device). The only throw in the K1 engine, so only its TU needs exceptions on.
if(K2P_GAME STREQUAL "k1")
  set_property(SOURCE "${K2_REPO}/src/libsource/AURORA/aurmodelparse.cpp" APPEND PROPERTY COMPILE_OPTIONS
               $<IF:$<BOOL:${MSVC}>,/EHsc,-fexceptions>)
  # With exceptions on, glibc's noexcept strcasecmp may not follow the TU's own plain prototype;
  # seen first, a system header's spec is one a later plain redeclaration may leave out.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    set_property(SOURCE "${K2_REPO}/src/libsource/AURORA/aurmodelparse.cpp" APPEND PROPERTY COMPILE_OPTIONS -include strings.h)
  endif()
endif()

# The console action-bar driver (K2P_UI=xbox; a stub elsewhere). It is platform code but reads and
# drives the engine through src/'s headers, so it is compiled with the engine's flags; each platform
# adds it to its k2_platform, where the link report sees it, and applies these properties. Its OS half
# is shim/shim_console.cpp.
set(K2P_CONSOLE_SOURCE "${K2P_DIR}/console/k2console.cpp" "${K2P_DIR}/console/k2namekeyboard.cpp"
    "${K2P_DIR}/pcui/k2pcstrings.cpp"   # K2_UI_PC's keyboard-and-mouse strings (empty elsewhere)
    "${K2P_DIR}/compat/engine_instances.cpp"   # template members an optimized build may not emit
    "${K2P_DIR}/compat/achievement_toast.cpp"   # the unlock notice (shim/shim_achievements.cpp's local backend)
    "${K2P_DIR}/ftgl/ftgl_host.cpp")   # the dialogue font: draws through the game's own GL layer (ASLgl / K1's)

# K2_QOL_IMPROVEMENTS (above). The page is engine code: it includes src/'s headers, so it joins
# K2P_CONSOLE_SOURCE and takes the engine's flags. The settings, the texture rename and the rumble
# output talk to SDL and are compiled like the shim. The Switch prompt art is rendered from
# resources/switch_k2_button_* by platforms/tools/padglyphs.py, its atlases from the local Switch
# dump's or the APK's fonts; without them there is no Switch set, and "Switch" falls back to the Xbox
# art.
# KOTOR 1 (padglyphs.py --game k1): the Xbox, PlayStation and Switch art is the vector art in
# resources/ (rendered by rsvg-convert), laid into K1's own glyph atlases (its patch OBB), and a
# missing source drops that set. The renders come from resources/padglyph_renders.zip first, so a
# host without librsvg (Windows) builds the same pixels; a host without the OBBs takes the atlases
# from the installed PC game (K2P_GAME_DATA_DIR, else Steam's). Configure runs `padglyphs.py --check`
# and turns what it would lack into a warning.
set(K2P_QOL_SOURCE)
# The game's Android OBBs, where padglyphs.py finds the glyph atlases it lays the prompt art into
# (any setup that runs the game has them; the default is the repo's decomp_bins copy).
if(K2P_GAME STREQUAL "k1")
  set(K2P_OBB_DIR "${K2_REPO}/decomp_bins/swkotor_obb" CACHE PATH "the game's Android OBBs (prompt art atlases)")
else()
  set(K2P_OBB_DIR "${K2_REPO}/decomp_bins/swkotorii_obb" CACHE PATH "the game's Android OBBs (prompt art atlases)")
endif()
set(K2P_GAME_DATA_DIR "" CACHE PATH
    "the installed PC game, the prompt art atlases' last resort (empty: found through Steam)")
if(K2P_QOL_CONTROLLER)
  list(APPEND K2P_CONSOLE_SOURCE "${K2P_DIR}/qol/k2qolpage.cpp")
  set(_k2_assets "${CMAKE_BINARY_DIR}/gen/k2assets.gen.cpp")
  file(GLOB _k2_glyph_svgs CONFIGURE_DEPENDS "${K2_REPO}/resources/*.svg")
  set(_k2_glyph_args --game "${K2P_GAME}" --obb-dir "${K2P_OBB_DIR}")
  if(K2P_GAME_DATA_DIR)
    list(APPEND _k2_glyph_args --game-dir "${K2P_GAME_DATA_DIR}")
  endif()
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${K2_REPO}/platforms/tools/padglyphs.py" --check
                          ${_k2_glyph_args}
                  OUTPUT_VARIABLE _k2_glyph_found ERROR_VARIABLE _k2_glyph_lacks
                  RESULT_VARIABLE _k2_glyph_rc)
  if(NOT _k2_glyph_rc EQUAL 0 OR _k2_glyph_lacks)
    message(WARNING "The button-prompt art (platforms/tools/padglyphs.py) will be incomplete; the "
                    "build runs on, and the game falls back to its own prompts for what is missing:\n"
                    "${_k2_glyph_lacks}")
  else()
    string(STRIP "${_k2_glyph_found}" _k2_glyph_found)
    message(STATUS "${_k2_glyph_found}")
  endif()
  # cl.exe: the ~20 MB of art as a COFF object (padglyphs.py --out-obj), not as string literals.
  set(_k2_assets_obj)
  set(_k2_assets_objarg)
  if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_k2_assets_obj "${CMAKE_BINARY_DIR}/gen/k2assets.gen.obj")
    set(_k2_assets_objarg --out-obj "${_k2_assets_obj}")
  endif()
  add_custom_command(
    OUTPUT "${_k2_assets}" ${_k2_assets_obj}
    COMMAND "${Python3_EXECUTABLE}" "${K2_REPO}/platforms/tools/padglyphs.py" ${_k2_glyph_args}
            --out-cpp "${_k2_assets}" ${_k2_assets_objarg}
    DEPENDS "${K2_REPO}/platforms/tools/padglyphs.py" ${_k2_glyph_svgs}
            "${K2_REPO}/resources/padglyph_renders.zip"
    COMMENT "padglyphs: the button-prompt art"
    VERBATIM)
  # The rule lives in this directory and k2_platform in a subdirectory, which only sees the file
  # through this target (each platform's CMakeLists adds the dependency).
  add_custom_target(k2_qol_assets DEPENDS "${_k2_assets}" ${_k2_assets_obj})
  set_source_files_properties("${_k2_assets}" ${_k2_assets_obj} PROPERTIES GENERATED TRUE)
  if(_k2_assets_obj)
    set_source_files_properties(${_k2_assets_obj} PROPERTIES EXTERNAL_OBJECT TRUE)
  endif()
  set(K2P_QOL_SOURCE "${K2P_DIR}/qol/k2qol.cpp" "${_k2_assets}" ${_k2_assets_obj})
endif()

# ---- files compiled into the binary (resources/, platforms/tools/embedfiles.py) -----------------
# The GLSL shaders the port's renderer opens by name in the working directory (the device: the OBB
# or the APK's assets). A PC data set (the Steam installs) has none, so they are the last resort of
# the shim's fopen (KOTOR II's KotorProgram) and of k1kotorgl.cpp's CompileShader. The two games
# ship different revisions; resources/shaders/common holds the files both have byte for byte.
# Generated at configure time (CMAKE_CONFIGURE_DEPENDS re-runs it when a shader changes), so every
# platform's k2_platform takes K2P_EMBED_SOURCE without a target dependency.
set(_k2_embedded "${CMAKE_BINARY_DIR}/gen/k2embedded.gen.cpp")
# Also the game's achievement names (resources/achievements/<game>, platforms/tools/achievement_names.py),
# which shim/shim_achievements.cpp's unlock notice reads.
set(_k2_embed_dirs "${K2_REPO}/resources/shaders/common" "${K2_REPO}/resources/shaders/${K2P_GAME}"
    "${K2_REPO}/resources/achievements/${K2P_GAME}")
# And on macOS / Linux the window icon (resources/icons/window/<game>, shim_display.cpp).
if(NOT WIN32 AND NOT ANDROID AND NOT IOS)
  list(APPEND _k2_embed_dirs "${K2_REPO}/resources/icons/window/${K2P_GAME}")
endif()
list(TRANSFORM _k2_embed_dirs APPEND "/*" OUTPUT_VARIABLE _k2_embed_globs)
file(GLOB _k2_embed_files CONFIGURE_DEPENDS ${_k2_embed_globs})
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${K2_REPO}/platforms/tools/embedfiles.py" --out "${_k2_embedded}"
          ${_k2_embed_dirs}
  RESULT_VARIABLE _k2_embed_result)
if(_k2_embed_result)
  message(FATAL_ERROR "embedfiles.py failed (${_k2_embed_result})")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             "${K2_REPO}/platforms/tools/embedfiles.py" ${_k2_embed_files})
set(K2P_EMBED_SOURCE "${_k2_embedded}")

# ---- K1: the port of KOTOR 1's libandroid_port.so ------------------------------------------------
# What libKOTOR.so imported from the shipped port library: the C++-linkage Win32 shims, the
# android_port_glX GL entry points, the zip OBB reader, MacDecompress, the Ios* input globals.
# k1port.cpp and k1gl.gen.cpp include engine headers, so they ride with the console sources (the
# engine's flags and KOTOR1); K2P_K1_PLATFORM_SOURCE (k1/deps.cmake: miniz, LzmaDec, the SDL front)
# is compiled like the shim. Each platform adds it to k2_platform.
set(K2P_K1_PLATFORM_SOURCE)
if(K2P_GAME STREQUAL "k1")
  include(${K2P_DIR}/k1/deps.cmake)
  list(APPEND K2P_CONSOLE_SOURCE "${K2P_DIR}/k1/k1port.cpp" "${K2P_DIR}/k1/k1gl.gen.cpp")
  list(APPEND K2P_K1_PLATFORM_SOURCE "${K2P_DIR}/k1/k1sdl.cpp" "${K2P_DIR}/k1/k1kotorgl.cpp"
       "${K2P_DIR}/k1/k1movie.cpp" "${K2P_DIR}/k1/k1shimc.cpp" "${K2P_DIR}/k1/k1crt.cpp")
endif()

# ---- K2P_MOBILE_HOST: the host the build runs on, desktop (macOS, Linux) or mobile (Android, iOS) ---
# K2P_MOBILE_HOST: a phone's or tablet's OS -- one fullscreen window SDL sizes, GLES 2 with gl4es in
# front of it (android/gl4es_glue.cpp), no desktop window modes. Distinguish it from K2P_MOBILE_OPT
# (the shipped port's content cuts) and K2P_UI (how the game is driven). K2P_IOS: iOS on top.
if(ANDROID OR IOS)
  add_compile_definitions(K2P_MOBILE_HOST=1)
endif()
if(IOS)
  add_compile_definitions(K2P_IOS=1)
endif()

# ---- Android / iOS: SDL2 / FreeType / gl4es from source (the host takes them from Homebrew) -------
if(ANDROID)
  include(${K2P_DIR}/android/deps.cmake)
  set(K2P_AUDIO_TOOLS OFF CACHE BOOL "" FORCE)   # fmodcheck is a host tool
elseif(IOS)
  include(${K2P_DIR}/ios/deps.cmake)
  set(K2P_AUDIO_TOOLS OFF CACHE BOOL "" FORCE)
elseif(WIN32)
  include(${K2P_DIR}/windows/deps.cmake)
  set(K2P_AUDIO_TOOLS OFF CACHE BOOL "" FORCE)
endif()

# ---- sound: the FMOD backend behind src/port/fmod's FModAPI.h (audio/CMakeLists.txt) -----------
add_subdirectory(platforms/audio audio)

# ---- movies: k2bink + the k2movie player behind ASL::PlayBinkMovieGL (video/CMakeLists.txt) -----
set(K2BINK_TOOLS OFF CACHE BOOL "" FORCE)   # binkcheck/binkplay/binkgl: build platforms/video standalone
add_subdirectory(platforms/video video)

# ---- platform layer and the host executable -----------------------------------------------------
if(ANDROID)
  add_subdirectory(platforms/android android)
elseif(IOS)
  add_subdirectory(platforms/ios ios)
elseif(APPLE)
  add_subdirectory(platforms/macos macos)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(platforms/linux linux)
elseif(WIN32)
  add_subdirectory(platforms/windows windows)
endif()
