# Windows platform layer and the k2 / k1 executable: presets <k1|k2>-cross-windows-<ui> (llvm-mingw),
# *-cross-windows-msvc-pc / *-cross-windows-cl-pc, and the host presets on a Windows machine (cl.exe).
#
# The Linux build on a PE/COFF host: the same engine library, shim and backends, the same desktop entry
# point and a desktop GL 2.1 compatibility context (opengl32 plus the driver's ICD). SDL2, FreeType and
# zlib are built from source and linked statically.
# What differs from the Linux build:
#   * LLP64: `long` is 32 bits. The engine was written for 32-bit Windows (its `long`s are DWORD /
#     LONG) and the Android port is LP64; compat/include-mingw/ and K2_HOST_BUILD hold the places where
#     the difference shows;
#   * the engine never sees <windows.h>: its Win32 calls go to the shim. The names kernel32, and the
#     CRT for _endthreadex, also define -- the thread, sync, file-find, timer and last-error functions
#     libc++, winpthread and the CRT import for themselves -- are declared once, in src/port/win32api.h,
#     which on Windows puts them in namespace k2win32, so the engine's and the system's are two symbols;
#     the rest the shim answers by their plain names, which kernel32's import library only supplies to
#     nobody;
#   * the link report runs in --format coff against llvm-mingw's import and runtime libraries. The link
#     has no duplicate definitions and takes none.
#
#   cmake --preset k2-cross-windows-pc && cmake --build --preset k2-cross-windows-pc   # -> out/k2-cross-windows-pc/windows/k2.exe
find_package(Threads REQUIRED)
# Windows 10 and later: the registry views the game-folder search reads (RRF_SUBKEY_WOW64*KEY).
add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)

# The C libraries this tree builds itself -- the Bink decoder and player, the audio backend -- see the
# same POSIX surface on the MSVC UCRT (compat/include-msvc/).
if(MSVC)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_k2_posix "${K2P_DIR}/compat/include-msvc")
  else()
    set(_k2_posix ${K2P_MINGW_INCLUDE})   # cl.exe: the overlay chain without #include_next
  endif()
  foreach(_t k2bink k2movie k2_audio)
    if(TARGET ${_t})
      target_include_directories(${_t} BEFORE PRIVATE ${_k2_posix})
    endif()
  endforeach()
endif()
# cl.exe (not clang-cl): the GNU spellings our own code uses (compat/k2_compiler.h, which the engine
# has through k2host_prelude.h). /bigobj: the shim's generated GL tables.
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(K2P_CL_FLAGS "/FI${K2P_DIR}/compat/k2_compiler.h" /bigobj)
  foreach(_t k2bink k2movie k2_audio)
    if(TARGET ${_t})
      target_compile_options(${_t} PRIVATE ${K2P_CL_FLAGS})
    endif()
  endforeach()
endif()

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
  ${K2P_DIR}/windows/win_posix.cpp
  ${K2P_DIR}/windows/win_dlfcn.cpp
  ${K2P_DIR}/windows/win_engine_imports.c
  ${K2P_CONSOLE_SOURCE}
  ${K2P_QOL_SOURCE}
  ${K2P_EMBED_SOURCE}
  ${K2P_K1_PLATFORM_SOURCE}
  $<$<BOOL:${MSVC}>:${K2P_DIR}/windows/win_msvc_posix.cpp>)   # the POSIX calls the UCRT lacks
target_include_directories(k2_platform BEFORE PRIVATE ${K2P_MINGW_INCLUDE})
target_compile_options(k2_platform PRIVATE ${K2P_MINGW_ISYSTEM})   # the engine's view of the CRT
target_include_directories(k2_platform PRIVATE "${K2P_DIR}/compat/include" ${K2_INCLUDE_DIRS})
# The shim includes engine headers (WinThreadsMP.h, ...): same host dialect as k2_engine.
if(MSVC)
  set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "/EHs-c-")
  # shim_crt.cpp / k1shimc.cpp define the CRT names the engine declares itself (src/port/crtapi.h).
  set_property(SOURCE ${K2P_DIR}/shim/shim_crt.cpp ${K2P_DIR}/k1/k1shimc.cpp APPEND PROPERTY
               COMPILE_DEFINITIONS K2_ENGINE_CRT=1)
else()
  set_source_files_properties(${_k2_shim_sources} ${K2P_QOL_SOURCE} PROPERTIES COMPILE_OPTIONS "-fno-exceptions;-fsigned-char")
endif()
set_source_files_properties(${K2P_CONSOLE_SOURCE} PROPERTIES
  COMPILE_OPTIONS "${K2_HOST_FLAGS}" COMPILE_DEFINITIONS "${K2_ENGINE_DEFS}")
# The shim's own uses of the engine headers (MacFiles.h ...) see the CRT's FILE tag too (k2host_prelude.h).
target_compile_definitions(k2_platform PRIVATE K2_HOST_BUILD=1 __sFILE=_iobuf)
target_compile_options(k2_platform PRIVATE ${K2P_CL_FLAGS})
target_link_libraries(k2_platform PUBLIC SDL2::SDL2 k2movie k2_audio Freetype::Freetype ZLIB::ZLIB
                                         opengl32 glu32 Threads::Threads)
set_target_properties(k2_platform PROPERTIES CXX_STANDARD 11)
# The movie player draws into the shim's stand-in for the window's framebuffer and presents through
# its copy-and-swap.
target_compile_definitions(k2movie PRIVATE K2M_GL_GETPROC=k2shim_gl_proc_c K2M_GL_SWAP=k2shim_gl_swap_c)
if(K2P_K1_PLATFORM_SOURCE)
  k1_platform_sources(k2_platform)   # k1/deps.cmake
endif()
if(TARGET k2_qol_assets)
  add_dependencies(k2_platform k2_qol_assets)
endif()

# ---- link report (K2P_LINKREPORT) (linkreport.py --format coff) ---------------------------------
if(K2P_LINKREPORT)   # an analysis of the engine objects; the link does not use it
# The host is what the executable links against: llvm-mingw's runtime (UCRT, mingw-w64's CRT glue,
# libc++, winpthreads) and the Windows import libraries, plus the static third-party libraries.
set(_rep "${CMAKE_BINARY_DIR}/report")
set(_host_libs)
set(_sysroot_lib "${K2_LLVM_MINGW}/x86_64-w64-mingw32/lib")
foreach(_l libucrt.a libucrtbase.a libmsvcrt.a libmingw32.a libmingwex.a libc++.a libunwind.a
           libwinpthread.a libkernel32.a libuser32.a libgdi32.a libshell32.a libadvapi32.a libole32.a
           libopengl32.a libglu32.a libwinmm.a libimm32.a libversion.a libsetupapi.a libuuid.a)
  if(EXISTS "${_sysroot_lib}/${_l}")
    list(APPEND _host_libs --host-dylib "${_sysroot_lib}/${_l}")
  endif()
endforeach()
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-libgcc-file-name
                OUTPUT_VARIABLE _rt OUTPUT_STRIP_TRAILING_WHITESPACE)   # compiler-rt builtins
if(EXISTS "${_rt}")
  list(APPEND _host_libs --host-dylib "${_rt}")
endif()
add_custom_command(
  OUTPUT "${_rep}/linkreport.md" "${_rep}/k2dups.txt"
  COMMAND "${Python3_EXECUTABLE}" "${K2P_DIR}/tools/linkreport.py" --format coff
          --llvm-bin "${K2_LLVM_BIN}"
          --archive "$<TARGET_FILE:k2_engine>"
          --extra-objects "$<TARGET_FILE:k2_platform>" "$<TARGET_FILE:k2movie>" "$<TARGET_FILE:k2bink>"
                          "$<TARGET_FILE:k2_audio>"
          --orig-so "${K2P_ORIG_SO}"
          --apk-libdir "${K2P_ORIG_LIBDIR}"
          ${_host_libs}
          --host-dylib "$<TARGET_LINKER_FILE:SDL2>"
          --host-dylib "$<TARGET_FILE:freetype>"
          --host-dylib "$<TARGET_FILE:k2zlib>"
          --out "${_rep}"
          "--data-homes=${K2P_DATA_HOMES}"
  DEPENDS k2_engine k2_platform k2movie k2bink k2_audio SDL2 freetype k2zlib
          "${K2P_DIR}/tools/linkreport.py" "${K2P_DIR}/tools/elfimage.py" ${K2P_DATA_HOMES}
  COMMENT "linkreport: classifying unresolved engine symbols (COFF)"
  VERBATIM)
add_custom_target(k2_linkreport ALL DEPENDS "${_rep}/linkreport.md")
endif()

add_executable(${K2P_EXE} WIN32   # the GUI subsystem: no console window (win_console.cpp)
               ${K2P_DIR}/macos/main.cpp ${K2P_DIR}/macos/gamedirs.cpp
               # in the executable, not k2_platform: nothing references them
               ${K2P_DIR}/windows/win_crash.cpp ${K2P_DIR}/windows/win_console.cpp)
# win_crash.cpp prints the build id the exe was built from, to pick the matching snapshot when
# symbolizing a crash report.
add_custom_target(k2_build_id
  COMMAND "${CMAKE_COMMAND}" -DOUT=${CMAKE_CURRENT_BINARY_DIR}/gen/build_id.h -DREPO=${K2P_DIR}/..
          -P ${K2P_DIR}/cmake/build_id.cmake
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/gen/build_id.h VERBATIM)
add_dependencies(${K2P_EXE} k2_build_id)
target_include_directories(${K2P_EXE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
if(K2P_GAME STREQUAL "k1")
  target_compile_definitions(${K2P_EXE} PRIVATE K2P_K1=1)   # main.cpp: enter K1's SDL_main
endif()
target_compile_definitions(${K2P_EXE} PRIVATE SDL_MAIN_HANDLED=1)   # main.cpp is main(); K1 defines SDL_main
target_include_directories(${K2P_EXE} BEFORE PRIVATE ${K2P_MINGW_INCLUDE})   # setenv, strlcpy ...
target_compile_options(${K2P_EXE} PRIVATE ${K2P_MINGW_ISYSTEM} ${K2P_CL_FLAGS})
set_target_properties(${K2P_EXE} PROPERTIES CXX_STANDARD 11 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
if(MSVC)
  # The whole engine (static ctors included). The CRT is the DLL one (/MD, the default): the shim
  # defines fopen / stat / ... itself, which a static CRT would define a second time.
  target_link_libraries(${K2P_EXE} PRIVATE k2_engine k2_platform)
  target_link_options(${K2P_EXE} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:k2_engine>")
  # A GUI-subsystem exe starts at WinMain under the MSVC CRT, and the engine defines one: start at
  # main, which finds the game and calls it, as mingw's CRT does.
  target_link_options(${K2P_EXE} PRIVATE "/ENTRY:mainCRTStartup")
else()
  target_link_libraries(${K2P_EXE} PRIVATE
    "-Wl,--whole-archive" k2_engine "-Wl,--no-whole-archive"   # the whole engine, static ctors included
    k2_platform
    -static)                                                   # libc++ / winpthread inside the .exe
endif()
# The game's icon (resources/icons/win32/<game>.ico): Explorer's and, as the first icon resource, the
# window class's SDL registers (the title bar, the taskbar).
enable_language(RC)
set(K2P_ICON "${K2_REPO}/resources/icons/win32/${K2P_GAME}.ico")
configure_file(${K2P_DIR}/windows/k2p_icon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/gen/k2p_icon.rc @ONLY)
target_sources(${K2P_EXE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen/k2p_icon.rc)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/gen/k2p_icon.rc APPEND PROPERTY OBJECT_DEPENDS
             "${K2P_ICON}")

# ---- MSVC diagnostics builds -------------------------------------------------------------------
# K2P_ASAN: AddressSanitizer (/fsanitize=address) over everything this tree compiles (engine, shim,
# the Bink / audio code; SDL2, FreeType and zlib stay uninstrumented). The DLL CRT (/MD) pairs with
# clang_rt.asan_dynamic-x86_64.dll, copied beside the exe from the toolset. The shim's operator new
# is calloc, so ASan sees every engine allocation (but not new/delete mismatches).
# K2P_MSVC_WARN: /W4 /analyze instead of the engine's /w, for a triage build (the log is the output).
option(K2P_ASAN "MSVC: build with AddressSanitizer (/fsanitize=address)" OFF)
option(K2P_MSVC_WARN "MSVC: compile the engine and platform code with /W4 /analyze (triage build)" OFF)
set(_k2_diag_targets k2_engine k2_platform k2movie k2bink k2_audio ${K2P_EXE})
if(K2P_ASAN)
  if(NOT MSVC)
    message(FATAL_ERROR "k2: K2P_ASAN is the MSVC toolset's AddressSanitizer (the Windows host presets k1-/k2-host-pc-asan)")
  endif()
  foreach(_t ${_k2_diag_targets})
    if(TARGET ${_t})
      target_compile_options(${_t} PRIVATE /fsanitize=address)
    endif()
  endforeach()
  target_link_options(${K2P_EXE} PRIVATE /INCREMENTAL:NO)
  get_filename_component(_k2_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  find_file(K2_ASAN_DLL clang_rt.asan_dynamic-x86_64.dll HINTS "${_k2_cl_dir}" NO_DEFAULT_PATH)
  if(NOT K2_ASAN_DLL)
    message(FATAL_ERROR "k2: clang_rt.asan_dynamic-x86_64.dll not found beside ${CMAKE_CXX_COMPILER}")
  endif()
  add_custom_command(TARGET ${K2P_EXE} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${K2_ASAN_DLL}" "$<TARGET_FILE_DIR:${K2P_EXE}>"
    VERBATIM)
  message(STATUS "k2: AddressSanitizer on (${K2_ASAN_DLL})")
endif()
if(K2P_MSVC_WARN)
  if(NOT MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "k2: K2P_MSVC_WARN is for cl.exe (the host presets on Windows)")
  endif()
  foreach(_t ${_k2_diag_targets})
    if(TARGET ${_t})
      get_target_property(_opts ${_t} COMPILE_OPTIONS)
      if(_opts)
        list(REMOVE_ITEM _opts /w)
        set_target_properties(${_t} PROPERTIES COMPILE_OPTIONS "${_opts}")
      endif()
      target_compile_options(${_t} PRIVATE /W4 /analyze /wd4068)
    endif()
  endforeach()
  # the per-source options (shim, QoL page, console) carry the engine flags too
  get_target_property(_srcs k2_platform SOURCES)
  foreach(_s ${_srcs})
    get_source_file_property(_so "${_s}" COMPILE_OPTIONS)
    if(_so)
      list(REMOVE_ITEM _so /w)
      set_source_files_properties("${_s}" PROPERTIES COMPILE_OPTIONS "${_so}")
    endif()
  endforeach()
endif()
