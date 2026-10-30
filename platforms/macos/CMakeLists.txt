# macOS platform layer + the k2 / k1 executable.
#
# SDL2 API (not SDL3): the engine calls SDL_CondWait, SDL_LockMutex (int-returning) and friends by
# their SDL2 names, so we link the SDL2 ABI. On a current Homebrew that is sdl2-compat: the SDL2 API
# running on the SDL3 runtime.
find_package(SDL2 CONFIG REQUIRED)
find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
find_package(Freetype REQUIRED)   # ftgl/ftgl_host.cpp: the original links FreeType and FTGL statically

# ---- platform layer: everything the engine imports that we answer ourselves --------------------
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
  ${K2P_K1_PLATFORM_SOURCE})
target_include_directories(k2_platform PRIVATE "${K2P_DIR}/compat/include" ${K2_INCLUDE_DIRS})
# The shim includes engine headers (WinThreadsMP.h, ...): same host dialect as k2_engine.
set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "-fno-exceptions;-fsigned-char")
set_source_files_properties(${K2P_CONSOLE_SOURCE} PROPERTIES
  COMPILE_OPTIONS "${K2_HOST_FLAGS}" COMPILE_DEFINITIONS "${K2_ENGINE_DEFS}")
target_compile_definitions(k2_platform PRIVATE K2_HOST_BUILD=1)
# k2movie: ASL::PlayBinkMovieGL.  k2_audio: the FMOD entry points (platforms/audio, K2P_AUDIO).
find_package(ZLIB REQUIRED)       # shim/shim_obb.cpp: the OBB index is zlib'd
target_link_libraries(k2_platform PUBLIC SDL2::SDL2 k2movie k2_audio Freetype::Freetype ZLIB::ZLIB)
# shim/shim_display.cpp asks NSScreen through the Objective-C runtime (AppKit itself comes with SDL).
find_library(K2_OBJC_LIBRARY objc REQUIRED)
target_link_libraries(k2_platform PUBLIC ${K2_OBJC_LIBRARY})
set_target_properties(k2_platform PROPERTIES CXX_STANDARD 11)
if(K2P_K1_PLATFORM_SOURCE)
  k1_platform_sources(k2_platform)   # k1/deps.cmake
endif()
if(TARGET k2_qol_assets)
  add_dependencies(k2_platform k2_qol_assets)
endif()

# ---- link report (K2P_LINKREPORT) ---------------------------------------------------------------
if(K2P_LINKREPORT)   # an analysis of the engine objects; the link does not use it
set(_rep "${CMAKE_BINARY_DIR}/report")
set(_orig_libdir "${K2P_ORIG_LIBDIR}")
set(_ndk_libdir "${K2_REPO}/toolchain/android-ndk-r21d/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/lib/aarch64-linux-android/23")
get_target_property(_sdl_lib SDL2::SDL2 LOCATION)
add_custom_command(
  OUTPUT "${_rep}/linkreport.md" "${_rep}/k2dups.txt"
  COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/tools/linkreport.py"
          --archive "$<TARGET_FILE:k2_engine>"
          --extra-objects "$<TARGET_FILE:k2_platform>" "$<TARGET_FILE:k2movie>" "$<TARGET_FILE:k2bink>"
                          "$<TARGET_FILE:k2_audio>"
          --orig-so "${K2P_ORIG_SO}"
          --apk-libdir "${_orig_libdir}"
          --ndk-libdir "${_ndk_libdir}"
          "--mac-oracle=${K2P_MAC_ORACLE}"
          --host-dylib "${_sdl_lib}"
          --host-dylib "${FREETYPE_LIBRARY}"
          --host-dylib "${ZLIB_LIBRARY}"
          --host-dylib "${K2_OBJC_LIBRARY}"
          --out "${_rep}"
          "--data-homes=${K2P_DATA_HOMES}"
  DEPENDS k2_engine k2_platform k2movie k2bink k2_audio "${K2P_DIR}/tools/linkreport.py" "${K2P_DIR}/tools/elfimage.py"
          ${K2P_DATA_HOMES}
  COMMENT "linkreport: classifying unresolved engine symbols"
  VERBATIM)

add_custom_target(k2_linkreport ALL DEPENDS "${_rep}/linkreport.md")
endif()

add_executable(${K2P_EXE} main.cpp gamedirs.cpp)
if(K2P_GAME STREQUAL "k1")
  target_compile_definitions(${K2P_EXE} PRIVATE K2P_K1=1)   # main.cpp: enter K1's SDL_main
endif()
set_target_properties(${K2P_EXE} PROPERTIES CXX_STANDARD 11)
set_property(TARGET ${K2P_EXE} APPEND PROPERTY LINK_DEPENDS "$<TARGET_FILE:k2_engine>")
target_link_libraries(${K2P_EXE} PRIVATE
  k2_platform
  "-Wl,-force_load,$<TARGET_FILE:k2_engine>"   # the whole engine, not just what main() reaches
  SDL2::SDL2
  ${OPENGL_FRAMEWORK})

# ---- the model viewer (platforms/viewer/viewer.cmake): this executable plus its panel -----------
k2p_add_viewer(${K2P_EXE})
