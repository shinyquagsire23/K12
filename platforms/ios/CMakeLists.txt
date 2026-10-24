# iOS platform layer and the app (presets <k1|k2>-cross-ios-<ui>; <k1|k2>-cross-ios-sim-<ui> = Simulator).
#
# The macOS build for iPhone and iPad: the same engine library, shim and backends, Mach-O and Apple
# clang, so the link report and its checks are macos/'s. What differs from macos/:
#   * the entry point is ios/main_ios.cpp (UIKit runs the app; the working directory is Documents,
#     where the OBBs go: Finder / the Files app);
#   * GL is gl4es on a GLES 2 context (android/gl4es_glue.cpp, android/glu_es.cpp), as on Android:
#     iOS has no desktop GL;
#   * SDL2 is built from source (ios/deps.cmake) as a dylib in the app's Frameworks/;
#   * the window is the screen, at its pixels (shim_window.cpp K2P_IOS, shim_display.cpp
#     k2shim_pixel_scalef).
#
#   cmake --preset k2-cross-ios-sim-pc && cmake --build --preset k2-cross-ios-sim-pc   # -> out/k2-cross-ios-sim-pc/ios/k2port.app
#   cmake --build --preset k2-cross-ios-sim-pc --target run-sim            # install + launch on a booted Simulator
# Device builds (presets without -sim) need a signing identity and a provisioning profile:
#   -DK2I_CODESIGN_IDENTITY="Apple Development: ..." -DK2I_PROVISIONING_PROFILE=<file.mobileprovision>
#   -DK2I_BUNDLE_ID=<an id the profile covers>
if(K2P_GAME STREQUAL "k1")
  set(_k2i_exe "k1port")
  set(_k2i_id "com.k2port.swkotor")
  set(_k2i_name "KOTOR")
else()
  set(_k2i_exe "k2port")
  set(_k2i_id "com.k2port.swkotorii")
  set(_k2i_name "KOTOR II")
endif()
set(K2I_BUNDLE_ID "${_k2i_id}.${K2P_UI}" CACHE STRING "the app's bundle identifier")
set(K2I_DISPLAY_NAME "${_k2i_name} (${K2P_UI})" CACHE STRING "the home screen label")
set(K2I_CODESIGN_IDENTITY "-" CACHE STRING "codesign identity: - (ad hoc: the Simulator) or a developer certificate")
set(K2I_PROVISIONING_PROFILE "" CACHE FILEPATH "device builds: the .mobileprovision embedded in the app")
set(K2I_VERSION "1.0")
set(K2I_BUILD "1")
set(K2I_EXECUTABLE "${_k2i_exe}")
if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
  set(K2I_PLATFORM "iPhoneSimulator")
  set(_k2i_sdk_name iphonesimulator)
else()
  set(K2I_PLATFORM "iPhoneOS")
  set(_k2i_sdk_name iphoneos)
endif()
execute_process(COMMAND xcrun --sdk ${_k2i_sdk_name} --show-sdk-path OUTPUT_VARIABLE _k2i_sdk
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# ---- platform layer: the engine's imports that we answer ourselves ------------------------------
file(GLOB _k2_shim_sources CONFIGURE_DEPENDS ${K2P_DIR}/shim/*.cpp)   # the Win32/ASL/WGL/ASLgl port layer
add_library(k2_platform STATIC
  ${K2P_DIR}/compat/android_compat.cpp
  ${_k2_shim_sources}
  ${K2P_DIR}/null/bink_null.cpp
  ${K2P_DIR}/compat/bionic_compat.c
  ${K2P_DIR}/compat/placement_new.cpp
  ${K2P_DIR}/compat/engine_gaps.cpp
  ${K2P_DIR}/compat/engine_fix_log.cpp
  ${K2P_DIR}/compat/k2cov.cpp
  ${K2P_CONSOLE_SOURCE}
  ${K2P_QOL_SOURCE}
  ${K2P_EMBED_SOURCE}
  ${K2P_K1_PLATFORM_SOURCE}
  ${K2P_DIR}/android/gl4es_glue.cpp
  ${K2P_DIR}/android/glu_es.cpp
  ios_compat.cpp
  ios_gpucapture.mm)   # a Metal GPU frame capture on demand (shim_window.cpp, K2_GL_TRACE=file)
target_include_directories(k2_platform PRIVATE "${K2P_DIR}/compat/include" "${K2P_DIR}/shim" ${K2_INCLUDE_DIRS})
set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "-fno-exceptions;-fsigned-char")
set_source_files_properties(${K2P_CONSOLE_SOURCE} PROPERTIES
  COMPILE_OPTIONS "${K2_HOST_FLAGS}" COMPILE_DEFINITIONS "${K2_ENGINE_DEFS}")
target_compile_definitions(k2_platform PRIVATE K2_HOST_BUILD=1)
target_link_libraries(k2_platform PUBLIC SDL2::SDL2 k2movie k2_audio freetype k2gl4es z objc
  "-framework Foundation" "-framework UIKit" "-framework OpenGLES" "-framework QuartzCore")
set_target_properties(k2_platform PROPERTIES CXX_STANDARD 11)
if(K2P_K1_PLATFORM_SOURCE)
  k1_platform_sources(k2_platform)   # k1/deps.cmake
endif()
if(TARGET k2_qol_assets)
  add_dependencies(k2_platform k2_qol_assets)
endif()
# The movie player's GL goes through gl4es too (k2movie.c), and compiles against its GL headers.
target_compile_definitions(k2movie PRIVATE K2M_GL_GETPROC=gl4es_GetProcAddress K2M_GL_SWAP=k2ios_gl_swap)
target_link_libraries(k2movie PRIVATE k2gl4es)

# ---- link report (K2P_LINKREPORT), against the iOS SDK -----------------------------------------
if(K2P_LINKREPORT)   # an analysis of the engine objects; the link does not use it
set(_rep "${CMAKE_BINARY_DIR}/report")
# The frameworks the platform layer links (miniaudio's iOS backend calls AVAudioSession and
# AudioUnit directly; macOS's dlopens them), so their symbols are host, not stubs.
set(_k2i_fw_tbds)
foreach(_fw Foundation CoreFoundation UIKit QuartzCore AVFoundation AVFAudio AudioToolbox CoreAudio)
  if(EXISTS "${_k2i_sdk}/System/Library/Frameworks/${_fw}.framework/${_fw}.tbd")
    list(APPEND _k2i_fw_tbds --host-dylib "${_k2i_sdk}/System/Library/Frameworks/${_fw}.framework/${_fw}.tbd")
  endif()
endforeach()
set(_ndk_libdir "${K2_REPO}/toolchain/android-ndk-r21d/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/aarch64-linux-android/23")
add_custom_command(
  OUTPUT "${_rep}/linkreport.md" "${_rep}/k2dups.txt"
  COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/tools/linkreport.py"
          --archive "$<TARGET_FILE:k2_engine>"
          --extra-objects "$<TARGET_FILE:k2_platform>" "$<TARGET_FILE:k2movie>" "$<TARGET_FILE:k2bink>"
                          "$<TARGET_FILE:k2_audio>"
          --orig-so "${K2P_ORIG_SO}"
          --apk-libdir "${K2P_ORIG_LIBDIR}"
          --ndk-libdir "${_ndk_libdir}"
          "--mac-oracle=${K2P_MAC_ORACLE}"
          --sdk "${_k2i_sdk}"
          --gl-tbd System/Library/Frameworks/OpenGLES.framework/OpenGLES.tbd
          --host-dylib "$<TARGET_FILE:SDL2::SDL2>"
          --host-dylib "$<TARGET_FILE:freetype>"
          --host-dylib "$<TARGET_FILE:k2gl4es>"
          ${_k2i_fw_tbds}
          --host-dylib "${_k2i_sdk}/usr/lib/libz.tbd"
          --host-dylib "${_k2i_sdk}/usr/lib/libobjc.tbd"
          --out "${_rep}"
          "--data-homes=${K2P_DATA_HOMES}"
  DEPENDS k2_engine k2_platform k2movie k2bink k2_audio SDL2::SDL2 freetype k2gl4es
          "${K2P_DIR}/tools/linkreport.py" "${K2P_DIR}/tools/elfimage.py" ${K2P_DATA_HOMES}
  COMMENT "linkreport: classifying unresolved engine symbols (iOS)"
  VERBATIM)

add_custom_target(k2_linkreport ALL DEPENDS "${_rep}/linkreport.md")
endif()

# ---- the app ----------------------------------------------------------------------------------
set(_plist "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
configure_file(Info.plist.in "${_plist}" @ONLY)
add_executable(k2ios MACOSX_BUNDLE main_ios.cpp)
if(K2P_GAME STREQUAL "k1")
  target_compile_definitions(k2ios PRIVATE K2P_K1=1)   # main_ios.cpp: K1's SDL_main
endif()
set_target_properties(k2ios PROPERTIES
  OUTPUT_NAME "${K2I_EXECUTABLE}"
  CXX_STANDARD 11
  MACOSX_BUNDLE_INFO_PLIST "${_plist}"
  INSTALL_RPATH "@executable_path/Frameworks"
  BUILD_WITH_INSTALL_RPATH ON
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
set_property(TARGET k2ios APPEND PROPERTY LINK_DEPENDS "$<TARGET_FILE:k2_engine>")
target_link_libraries(k2ios PRIVATE
  k2_platform
  "-Wl,-force_load,$<TARGET_FILE:k2_engine>"   # the whole engine, not just what main() reaches
  SDL2::SDL2)

# Bundle contents: SDL2 in Frameworks/; KOTOR 1's dialog font, iosdialog.otf (the FTGL face of the
# touch UI's widgets and of the movie subtitles), which the APK carried as an asset rather than in
# its OBBs (main_ios.cpp copies it to override/; SDL_RWFromFile looks in the bundle first). It is
# Aspyr's data, so it is taken from the unzipped APK, not the repo. The GLSL renderer's shaders are
# compiled in (resources/shaders, K2P_EMBED_SOURCE).
set(_app "$<TARGET_BUNDLE_DIR:k2ios>")
set(_k2i_assets)
if(K2P_GAME STREQUAL "k1")
  set(_k2i_font "${K2_REPO}/decomp_bins/swkotor_extracted/assets/iosdialog.otf")
  if(EXISTS "${_k2i_font}")
    list(APPEND _k2i_assets "${_k2i_font}")
  else()
    message(WARNING "k2: no ${_k2i_font} (the original K1 APK, unzipped): the touch UI's widget text "
                    "(and K2P_BINK_SUBTITLES' movie subtitles) will not draw")
  endif()
endif()
set(_sign_args --identity "${K2I_CODESIGN_IDENTITY}"
    --icon "${K2_REPO}/resources/icons/${K2P_GAME}_icon_1024.png"
    --platform ${_k2i_sdk_name} --min-os ${CMAKE_OSX_DEPLOYMENT_TARGET})
foreach(_a IN LISTS _k2i_assets)
  list(APPEND _sign_args --asset "${_a}")
endforeach()
if(K2I_PROVISIONING_PROFILE)
  list(APPEND _sign_args --profile "${K2I_PROVISIONING_PROFILE}")
endif()
add_custom_command(TARGET k2ios POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_app}/Frameworks"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_SONAME_FILE:SDL2::SDL2>" "${_app}/Frameworks/"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/k2ipa.py" sign --app "${_app}"
          ${_sign_args}
  COMMENT "k2ipa: bundling and signing ${K2I_EXECUTABLE}.app"
  VERBATIM COMMAND_EXPAND_LISTS)

# ---- Simulator / device targets (ios/tools/k2ipa.py) ----------------------------------------------
if(K2P_GAME STREQUAL "k1")
  set(K2I_OBB_SRC "${K2_REPO}/decomp_bins/swkotor_obb" CACHE PATH "run-sim: the OBBs copied into the app's Documents")
else()
  set(K2I_OBB_SRC "${K2_REPO}/decomp_bins/swkotorii_obb" CACHE PATH "run-sim: the OBBs copied into the app's Documents")
endif()
set(K2I_RUN_ENV "K2_STUBS=soft" CACHE STRING "run-sim: Documents/k2env.txt (KEY=VALUE;...)")
add_custom_target(run-sim
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/k2ipa.py" run-sim --app "${_app}"
          --bundle-id "${K2I_BUNDLE_ID}" --obb-dir "${K2I_OBB_SRC}" --env "${K2I_RUN_ENV}"
  DEPENDS k2ios USES_TERMINAL VERBATIM)
add_custom_target(ipa
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/k2ipa.py" ipa --app "${_app}"
          --out "${CMAKE_BINARY_DIR}/${K2I_EXECUTABLE}-${K2P_UI}.ipa"
  DEPENDS k2ios VERBATIM)
