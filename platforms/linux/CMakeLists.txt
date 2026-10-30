# Linux platform layer and the k2 / k1 executable (the host presets <k1|k2>-host-<ui> on a Linux
# machine).
#
# The macOS build on an ELF host: the same engine library, shim and backends, the same desktop entry
# point (macos/main.cpp) and the same desktop GL 2.1 compatibility context (Mesa or the vendor's
# libGL). What differs from macos/:
#   * clang with libc++ (the presets set it): the engine's std:: types are the NDK's libc++ layout,
#     and the shim's libc overrides (fopen, stat, access ... with the OBB fallback) are defined in
#     the executable, so they interpose for every library it loads;
#   * libc++, libc++abi and LLVM's libunwind are linked in (K2P_STATIC_LIBCXX, on by default), so
#     the executable runs where no libc++.so.1 is installed, SteamOS included;
#   * the link report runs in --format elf against the distribution's shared libraries and fails on
#     a duplicate engine definition; the linker allows the platform side's
#     (--allow-multiple-definition; android/ and macos/ link without it);
#   * GLU is the system libGLU (macOS has it inside OpenGL.framework).
#
#   cmake --preset k2-host-pc && cmake --build --preset k2-host-pc   # -> out/k2-host-pc/linux/k2
# Needs (Debian/Ubuntu): clang lld llvm libc++-dev libc++abi-dev libsdl2-dev libfreetype-dev
#                        libgl-dev libglu1-mesa-dev zlib1g-dev ninja-build cmake librsvg2-bin (prompt art)
find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
if(NOT OPENGL_glu_LIBRARY)
  message(FATAL_ERROR "k2: libGLU not found (libglu1-mesa-dev)")
endif()
# SDL2's config may hand back the imported target or only variables (older distributions).
if(TARGET SDL2::SDL2)
  set(_k2_sdl SDL2::SDL2)
else()
  set(_k2_sdl ${SDL2_LIBRARIES})
  include_directories(${SDL2_INCLUDE_DIRS})
endif()

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
  ${K2P_K1_PLATFORM_SOURCE})
target_include_directories(k2_platform BEFORE PRIVATE "${K2P_GLIBC_INCLUDE}")   # the engine's view of glibc
target_include_directories(k2_platform PRIVATE "${K2P_DIR}/compat/include" ${K2_INCLUDE_DIRS})
# The shim includes engine headers (WinThreadsMP.h, ...): same host dialect as k2_engine.
set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "-fno-exceptions;-fsigned-char")
set_source_files_properties(${K2P_CONSOLE_SOURCE} PROPERTIES
  COMPILE_OPTIONS "${K2_HOST_FLAGS}" COMPILE_DEFINITIONS "${K2_ENGINE_DEFS}")
# The shim's own uses of the engine headers (MacFiles.h ...) see glibc's FILE tag too.
target_compile_definitions(k2_platform PRIVATE K2_HOST_BUILD=1 __sFILE=_IO_FILE)
target_link_libraries(k2_platform PUBLIC ${_k2_sdl} k2movie k2_audio Freetype::Freetype ZLIB::ZLIB
                                         OpenGL::GL ${OPENGL_glu_LIBRARY} Threads::Threads ${CMAKE_DL_LIBS})
set_target_properties(k2_platform PROPERTIES CXX_STANDARD 11)
if(K2P_K1_PLATFORM_SOURCE)
  k1_platform_sources(k2_platform)   # k1/deps.cmake
endif()
if(TARGET k2_qol_assets)
  add_dependencies(k2_platform k2_qol_assets)
endif()

option(K2P_STATIC_LIBCXX "link libc++, libc++abi and libunwind into the executable" ON)
if(K2P_STATIC_LIBCXX)
  set(_k2_cxx_runtime libc++.a libc++abi.a libunwind.a)
else()
  set(_k2_cxx_runtime libc++.so.1 libc++abi.so.1 libunwind.so.1)
endif()

# ---- link report (K2P_LINKREPORT) --------------------------------------------------------------
if(K2P_LINKREPORT)   # an analysis of the engine objects; the link does not use it
# The host is what the executable links against: the C/C++ runtimes the compiler would pick and the
# libraries above, each asked of the compiler so the distribution's multiarch layout does not matter.
set(_rep "${CMAKE_BINARY_DIR}/report")
set(_host_libs)
# libc_nonshared.a: glibc's atexit, stat64 ... live there, not in libc.so.6.
foreach(_l libc.so.6 libc_nonshared.a libm.so.6 ${_k2_cxx_runtime} libgcc_s.so.1 libgcc.a)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=${_l}
                  OUTPUT_VARIABLE _p OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(IS_ABSOLUTE "${_p}" AND EXISTS "${_p}")
    list(APPEND _host_libs --host-dylib "${_p}")
  endif()
endforeach()
foreach(_l IN ITEMS ${OPENGL_opengl_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${FREETYPE_LIBRARY}
                    ${ZLIB_LIBRARY})
  if(EXISTS "${_l}")
    get_filename_component(_real "${_l}" REALPATH)   # the .so link is a symlink to the versioned file
    list(APPEND _host_libs --host-dylib "${_real}")
  endif()
endforeach()
if(TARGET SDL2::SDL2)
  set(_sdl_lib "$<TARGET_FILE:SDL2::SDL2>")
else()
  find_library(_sdl_lib SDL2 REQUIRED)
endif()
add_custom_command(
  OUTPUT "${_rep}/linkreport.md" "${_rep}/k2dups.txt"
  COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/tools/linkreport.py" --format elf
          --archive "$<TARGET_FILE:k2_engine>"
          --extra-objects "$<TARGET_FILE:k2_platform>" "$<TARGET_FILE:k2movie>" "$<TARGET_FILE:k2bink>"
                          "$<TARGET_FILE:k2_audio>"
          --orig-so "${K2P_ORIG_SO}"
          --apk-libdir "${K2P_ORIG_LIBDIR}"
          ${_host_libs}
          --host-dylib "${_sdl_lib}"
          --out "${_rep}"
          "--data-homes=${K2P_DATA_HOMES}"
  DEPENDS k2_engine k2_platform k2movie k2bink k2_audio "${K2P_DIR}/tools/linkreport.py" "${K2P_DIR}/tools/elfimage.py"
          ${K2P_DATA_HOMES}
  COMMENT "linkreport: classifying unresolved engine symbols (ELF)"
  VERBATIM)
add_custom_target(k2_linkreport ALL DEPENDS "${_rep}/linkreport.md")
endif()

add_executable(${K2P_EXE} ${K2P_DIR}/macos/main.cpp ${K2P_DIR}/macos/gamedirs.cpp)
if(K2P_GAME STREQUAL "k1")
  target_compile_definitions(${K2P_EXE} PRIVATE K2P_K1=1)   # main.cpp: enter K1's SDL_main
endif()
set_target_properties(${K2P_EXE} PROPERTIES CXX_STANDARD 11 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${K2P_EXE} PRIVATE
  "-Wl,--whole-archive" k2_engine "-Wl,--no-whole-archive"   # the whole engine, static ctors included
  k2_platform
  "-Wl,--allow-multiple-definition")                          # platform-side duplicates (above)
if(K2P_STATIC_LIBCXX)
  # -static-libstdc++ makes clang's own -lc++ a -Bstatic one; Debian's libc++.a leaves libc++abi
  # and the unwinder out (its libc++.so is the script INPUT(libc++.so.1 -lc++abi -lunwind)).
  target_link_options(${K2P_EXE} PRIVATE -static-libstdc++)
  target_link_libraries(${K2P_EXE} PRIVATE -l:libc++abi.a -l:libunwind.a)
endif()

# ---- the model viewer (platforms/viewer/viewer.cmake): this executable plus its panel -----------
k2p_add_viewer(${K2P_EXE})
