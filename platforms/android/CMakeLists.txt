# Android platform layer and the APK (presets k2-cross-android-touch / k2-cross-android-xbox).
#
# The same engine library (k2_engine), shim and backends as the macOS build, compiled with a current
# NDK for arm64 and linked into libmain.so next to a stock SDL2 (libSDL2.so and its Java). What differs
# from the macOS build:
#   * the entry point is k2_main (main_android.cpp; K2Activity names it to SDLActivity): logcat, the
#     working directory, the OBBs;
#   * GL is gl4es on GLES 2 (gl4es_glue.cpp), and GLU is ours (glu_es.cpp);
#   * the link report runs in --format elf against the NDK sysroot and fails on a duplicate engine
#     definition; the link takes none on the platform side either (no --allow-multiple-definition);
#   * -Bsymbolic: the shim's fopen/stat/access (Aspyr's libc overrides, with the OBB fallback) must
#     win for every caller inside libmain.so, libc++'s ifstream included.
#
#   cmake --build --preset k2-cross-android-touch                      # -> out/k2-cross-android-touch/k2-android-touch.apk
#   cmake --build --preset k2-cross-android-touch --target install-device   # adb install -r -d (+ the OBBs)
#   cmake --build --preset k2-cross-android-touch --target run-device       # launch + follow the logcat
# KOTOR 1 (K2P_GAME=k1) follows the same "<original>.hybrid" scheme: com.aspyr.swkotor.hybrid installs
# beside both Aspyr's com.aspyr.swkotor and the KOTOR II port, replacing neither.
if(K2P_GAME STREQUAL "k1")
  set(_k2a_pkg "com.aspyr.swkotor.hybrid")
  set(_k2a_label "KOTOR (port, ${K2P_UI})")
  set(K2A_ACTIVITY "k2.K1Activity")          # enters k1_main (main_android.cpp), not k2_main
  set(K2A_VERSION_CODE 93)                   # the original's (1.0.43)
  set(K2A_VERSION_NAME "1.0.43-port")
  set(K2A_ORIG_PACKAGE "com.aspyr.swkotor")
  set(K2A_OBB_SRC "${K2_REPO}/decomp_bins/swkotor_obb")
  set(_k2a_apk_stem "k1-android")
else()
  set(_k2a_pkg "com.aspyr.swkotorii.hybrid")
  set(_k2a_label "KOTOR II (port, ${K2P_UI})")
  set(K2A_ACTIVITY "k2.K2Activity")
  set(K2A_VERSION_CODE 229)
  set(K2A_VERSION_NAME "2.0.4-port")
  set(K2A_ORIG_PACKAGE "com.aspyr.swkotorii")
  set(K2A_OBB_SRC "${K2_REPO}/decomp_bins/swkotorii_obb")
  set(_k2a_apk_stem "k2-android")
endif()
set(K2A_PACKAGE "${_k2a_pkg}" CACHE STRING
    "APK package: the hybrid APK's by default, so the two share saves and OBBs")
set(K2A_LABEL "${_k2a_label}" CACHE STRING "launcher label")
set(K2A_BUILD_TOOLS "34.0.0" CACHE STRING "Android SDK build-tools version (aapt2, d8, zipalign, apksigner)")
set(K2A_PLATFORM "android-34" CACHE STRING "Android SDK platform whose android.jar the Java compiles against")
set(K2A_RUN_ENV "K2_STUBS=soft" CACHE STRING "run-device: the k2env intent extra (KEY=VALUE;...)")
# (Android Studio's default SDK: ~/Library/Android/sdk on macOS, ~/Android/Sdk on Linux,
# %LOCALAPPDATA%\Android\Sdk on Windows)
foreach(_sdk "$ENV{ANDROID_HOME}" "$ENV{ANDROID_SDK_ROOT}" "$ENV{HOME}/Library/Android/sdk" "$ENV{HOME}/Android/Sdk"
             "$ENV{LOCALAPPDATA}/Android/Sdk")
  file(TO_CMAKE_PATH "${_sdk}" _sdk)
  if(NOT K2A_SDK AND _sdk AND IS_DIRECTORY "${_sdk}/platforms")
    set(K2A_SDK "${_sdk}" CACHE PATH "Android SDK root")
  endif()
endforeach()
if(NOT K2A_SDK)
  message(FATAL_ERROR "Android SDK not found: set ANDROID_HOME")
endif()

# ---- platform layer: the shim, compat, FTGL, null Bink, and the Android glue -------------------
file(GLOB _k2_shim_sources CONFIGURE_DEPENDS ${K2P_DIR}/shim/*.cpp)
add_library(k2_platform STATIC
  ${K2P_DIR}/compat/android_compat.cpp
  ${_k2_shim_sources}
  ${K2P_DIR}/null/bink_null.cpp
  ${K2P_DIR}/compat/placement_new.cpp
  ${K2P_DIR}/compat/engine_gaps.cpp
  ${K2P_DIR}/compat/engine_fix_log.cpp
  ${K2P_DIR}/compat/k2cov.cpp
  ${K2P_CONSOLE_SOURCE}
  ${K2P_QOL_SOURCE}
  ${K2P_EMBED_SOURCE}
  ${K2P_K1_PLATFORM_SOURCE}
  gl4es_glue.cpp
  glu_es.cpp)
target_include_directories(k2_platform PRIVATE "${K2P_DIR}/compat/include" "${K2P_DIR}/shim" ${K2_INCLUDE_DIRS})
set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "-fno-exceptions;-fsigned-char")
set_source_files_properties(${K2P_CONSOLE_SOURCE} PROPERTIES
  COMPILE_OPTIONS "${K2_HOST_FLAGS}" COMPILE_DEFINITIONS "${K2_ENGINE_DEFS}")
target_compile_definitions(k2_platform PRIVATE K2_HOST_BUILD=1)
target_link_libraries(k2_platform PUBLIC SDL2::SDL2 k2movie k2_audio Freetype::Freetype k2gl4es z log android)
set_target_properties(k2_platform PROPERTIES CXX_STANDARD 11)
if(K2P_K1_PLATFORM_SOURCE)
  k1_platform_sources(k2_platform)   # k1/deps.cmake
endif()
if(TARGET k2_qol_assets)
  add_dependencies(k2_platform k2_qol_assets)
endif()
# The movie player's GL goes through gl4es too (k2movie.c).
target_compile_definitions(k2movie PRIVATE K2M_GL_GETPROC=gl4es_GetProcAddress)

# ---- link report (K2P_LINKREPORT) (linkreport.py --format elf) ----------------------------------
if(K2P_LINKREPORT)   # an analysis of the engine objects; the link does not use it
set(_rep "${CMAKE_BINARY_DIR}/report")
set(_orig_libdir "${K2P_ORIG_LIBDIR}")
set(_ndk_api_libdir "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/${ANDROID_PLATFORM_LEVEL}")
set(_ndk_libdir "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android")
# What the NDK answers: the API level's system libraries, the static C++ runtime, compiler-rt.
set(_host_libs)
foreach(_l libc.so libm.so libdl.so liblog.so libz.so libandroid.so libGLESv2.so libEGL.so libOpenSLES.so)
  list(APPEND _host_libs --host-dylib "${_ndk_api_libdir}/${_l}")
endforeach()
# The startup object every shared library links: bionic's atexit lives there, not in libc.so.
list(APPEND _host_libs --host-dylib "${_ndk_api_libdir}/crtbegin_so.o")
foreach(_l libc++_static.a libc++abi.a)
  list(APPEND _host_libs --host-dylib "${_ndk_libdir}/${_l}")
endforeach()
# (r25 keeps them under lib64/, r26 under lib/; libunwind.a is in an aarch64/ subdir)
file(GLOB_RECURSE _rt "${ANDROID_TOOLCHAIN_ROOT}/lib*/libclang_rt.builtins-aarch64-android.a")
file(GLOB_RECURSE _unwind "${ANDROID_TOOLCHAIN_ROOT}/lib*/libunwind.a")
list(FILTER _unwind INCLUDE REGEX "/aarch64/")
list(APPEND _rt ${_unwind})
if(NOT _rt)
  message(FATAL_ERROR "k2: no compiler-rt builtins / libunwind under ${ANDROID_TOOLCHAIN_ROOT}")
endif()
foreach(_l IN LISTS _rt)
  list(APPEND _host_libs --host-dylib "${_l}")
endforeach()
add_custom_command(
  OUTPUT "${_rep}/linkreport.md" "${_rep}/k2dups.txt"
  COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/tools/linkreport.py" --format elf
          --archive "$<TARGET_FILE:k2_engine>"
          --extra-objects "$<TARGET_FILE:k2_platform>" "$<TARGET_FILE:k2movie>" "$<TARGET_FILE:k2bink>"
                          "$<TARGET_FILE:k2_audio>"
          --orig-so "${K2P_ORIG_SO}"
          --apk-libdir "${_orig_libdir}"
          --ndk-libdir "${_ndk_api_libdir}"
          "--mac-oracle=${K2P_MAC_ORACLE}"
          ${_host_libs}
          --host-dylib "$<TARGET_FILE:SDL2::SDL2>"
          --host-dylib "$<TARGET_FILE:freetype>"
          --host-dylib "$<TARGET_FILE:k2gl4es>"
          --out "${_rep}"
          "--data-homes=${K2P_DATA_HOMES}"
  DEPENDS k2_engine k2_platform k2movie k2bink k2_audio SDL2::SDL2 freetype k2gl4es
          "${K2P_DIR}/tools/linkreport.py" "${K2P_DIR}/tools/elfimage.py" ${K2P_DATA_HOMES}
  COMMENT "linkreport: classifying unresolved engine symbols (ELF)"
  VERBATIM)
add_custom_target(k2_linkreport ALL DEPENDS "${_rep}/linkreport.md")
endif()

# ---- libmain.so: what SDLActivity loads after libSDL2.so and calls k2_main in ------------------
add_library(main SHARED main_android.cpp)
set_target_properties(main PROPERTIES CXX_STANDARD 11)
if(K2P_GAME STREQUAL "k1")
  target_compile_definitions(main PRIVATE K2P_K1=1)   # main_android.cpp: k1_main, K1's SDL_main
endif()
target_link_libraries(main PRIVATE
  "-Wl,--whole-archive" k2_engine "-Wl,--no-whole-archive"   # the whole engine, static ctors included
  k2_platform
  "-Wl,-Bsymbolic"                                            # the shim's libc overrides win inside
  "-Wl,--no-undefined")

# ---- the APK ----------------------------------------------------------------------------------
set(_manifest "${CMAKE_BINARY_DIR}/AndroidManifest.xml")
configure_file(AndroidManifest.xml.in "${_manifest}" @ONLY)
set(_apk "${CMAKE_BINARY_DIR}/${_k2a_apk_stem}-${K2P_UI}.apk")
# KOTOR 1's dialog font, iosdialog.otf -- the FTGL face of the touch UI's widgets, and of the movie
# subtitles with K2P_BINK_SUBTITLES -- ships in the original APK's assets/, not in its OBBs; KOTOR II's
# main OBB carries the same file as override/iosdialog.otf. k1_main copies it to override/. It is
# Aspyr's data, so it is taken from the unzipped APK, not the repo. The GLSL renderer's shaders are
# compiled into the library (resources/shaders, K2P_EMBED_SOURCE: CompileShader falls back to them).
set(_k2a_assets)
if(K2P_GAME STREQUAL "k1")
  set(_k2a_font "${K2_REPO}/decomp_bins/swkotor_extracted/assets/iosdialog.otf")
  if(EXISTS "${_k2a_font}")
    list(APPEND _k2a_assets --asset "${_k2a_font}")
  else()
    message(WARNING "k2: no ${_k2a_font} (the original K1 APK, unzipped): the touch UI's widget text "
                    "(and K2P_BINK_SUBTITLES' movie subtitles) will not draw")
  endif()
endif()
file(GLOB _sdl_java CONFIGURE_DEPENDS "${K2_SDL2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/*.java")
file(GLOB _k2_java CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/java/k2/*.java")
# the launcher icons (resources/icons/<game>_icon_1024.png, put in per-density folders)
file(GLOB_RECURSE _k2a_icons CONFIGURE_DEPENDS "${K2_REPO}/resources/icons/android/${K2P_GAME}/*")
add_custom_command(
  OUTPUT "${_apk}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/k2apk.py"
          --sdk "${K2A_SDK}" --build-tools "${K2A_BUILD_TOOLS}" --platform "${K2A_PLATFORM}"
          --manifest "${_manifest}"
          --java "${K2_SDL2_SOURCE_DIR}/android-project/app/src/main/java"
          --java "${CMAKE_CURRENT_SOURCE_DIR}/java"
          --lib "$<TARGET_FILE:main>" --lib "$<TARGET_FILE:SDL2::SDL2>"
          ${_k2a_assets}
          --res "${K2_REPO}/resources/icons/android/${K2P_GAME}"
          --work "${CMAKE_BINARY_DIR}/apk" --out "${_apk}"
  DEPENDS main SDL2::SDL2 "${_manifest}" ${_sdl_java} ${_k2_java} ${_k2a_icons} tools/k2apk.py
  COMMENT "k2apk: packaging ${_apk}"
  VERBATIM)
add_custom_target(apk ALL DEPENDS "${_apk}")

# ---- device targets (tools/device.py: adb install -r -d, the OBBs, launch + logcat) ---------------
set(_device "${K2_REPO}/platforms/tools/device.py")
add_custom_target(install-device
  COMMAND "${Python3_EXECUTABLE}" "${_device}" install --apk "${_apk}" --package "${K2A_PACKAGE}"
          --orig-package "${K2A_ORIG_PACKAGE}" --obb-src "${K2A_OBB_SRC}"
  DEPENDS apk USES_TERMINAL VERBATIM)
add_custom_target(run-device
  COMMAND "${Python3_EXECUTABLE}" "${_device}" run --package "${K2A_PACKAGE}" --activity ${K2A_ACTIVITY}
          --extra "k2env=${K2A_RUN_ENV}"
  USES_TERMINAL VERBATIM)
