# Third-party code the Windows build compiles from source (the macOS and Linux hosts take it from the
# system): SDL2, FreeType and zlib, all static, so the executable needs no DLL of ours. GL and GLU are
# Windows' own opengl32 / glu32. The Visual Studio projects the archives ship are deleted on extraction,
# and SDL2 is a separate CMake build (below): Visual Studio's startup-item list then shows k2.exe /
# k1.exe and nothing of the dependencies.
# The archives (pinned URL + SHA-256) are downloaded once into platforms/out/_downloads, shared by every
# Windows preset and checked against the hash before each use. Each build directory extracts and builds
# its own copies, under its own _deps (FetchContent's default): with one tree shared by every build,
# two builds at once, or a build while Visual Studio debugs another, wrote the same k2sdl2-build and
# SDL2.pdb (LNK1201); and the MSVC and mingw builds need different binaries anyway. A build directory
# configured before that cached the shared tree (out/_deps-windows[-msvc]): it is moved back into the
# build directory. A FETCHCONTENT_BASE_DIR given on the command line is kept.
if(FETCHCONTENT_BASE_DIR MATCHES "/out/_deps-windows(-msvc)?/?$")
  set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "" FORCE)
endif()
set(_k2_downloads "${K2P_DIR}/out/_downloads")
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(_k2_strip_vs "${CMAKE_COMMAND}" -P "${K2P_DIR}/cmake/strip_vs_projects.cmake")

FetchContent_Declare(k2sdl2
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
  URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
  DOWNLOAD_DIR "${_k2_downloads}"
  SOURCE_SUBDIR k2-no-cmake    # sources only: built by the ExternalProject below
  PATCH_COMMAND ${_k2_strip_vs})
FetchContent_Declare(k2freetype
  URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-3.tar.gz
  URL_HASH SHA256=bc5c898e4756d373e0d991bab053036c5eb2aa7c0d5c67e8662ddc6da40c4103
  DOWNLOAD_DIR "${_k2_downloads}"
  PATCH_COMMAND ${_k2_strip_vs})
FetchContent_Declare(k2zlib
  URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
  URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
  DOWNLOAD_DIR "${_k2_downloads}"
  SOURCE_SUBDIR k2-no-cmake    # sources only: compiled below
  PATCH_COMMAND ${_k2_strip_vs})

# ---- SDL2, static: k2.exe / k1.exe need no SDL2.dll ------------------------------------------------
# The shim and k1/k1sdl.cpp define some SDL entry points themselves (SDL_RWFromFile, SDL_PollEvent
# ...) and call SDL's behind them (dlsym(RTLD_NEXT), windows/win_dlfcn.cpp): one name, two
# definitions. In the static library SDL's public definition of each is k2sdl_real_<name>
# (windows/sdl_interposed.h, windows/sdl2_static.cmake); /ALTERNATENAME gives <name> to it where the
# game links no definition of its own. SDL2main is not used: main.cpp is main().
# Its own CMake build, with this build's toolchain and CRT (the DLL one, /MD: SDL's default, as here),
# installed into the build dir; this project sees an imported SDL2 (SDL2::SDL2). As a subproject it put
# SDL2's targets into the code model, which Visual Studio lists as startup items.
FetchContent_MakeAvailable(k2sdl2)
set(K2_SDL2_SOURCE_DIR "${k2sdl2_SOURCE_DIR}")
include(ExternalProject)
set(_k2_sdl "${CMAKE_BINARY_DIR}/_deps/k2sdl2-install")
if(MSVC)
  set(_k2_sdl_lib "${_k2_sdl}/lib/SDL2-static.lib")
else()
  set(_k2_sdl_lib "${_k2_sdl}/lib/libSDL2.a")
endif()
file(STRINGS "${K2P_DIR}/windows/sdl_interposed.h" _k2_sdl_interposed REGEX "^K2SDL_INTERPOSED\\(")
string(REGEX REPLACE "K2SDL_INTERPOSED\\(([A-Za-z0-9_]+)\\)" "\\1" _k2_sdl_interposed "${_k2_sdl_interposed}")
string(REPLACE ";" "|" _k2_sdl_interposed_arg "${_k2_sdl_interposed}")
set(_k2_sdl_args
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DCMAKE_INSTALL_PREFIX=${_k2_sdl}"
  "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
  -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF -DSDL2_DISABLE_SDL2MAIN=ON
  -DSDL2_DISABLE_UNINSTALL=ON "-DSDL_CMAKE_DEBUG_POSTFIX="
  -DSDL_LIBC=ON    # MSVC: SDL defaults to no C runtime, which clang-cl's libcalls need
  "-DCMAKE_PROJECT_INCLUDE=${K2P_DIR}/windows/sdl2_static.cmake"
  "-DK2_SDL_INTERPOSED=${_k2_sdl_interposed_arg}")
if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND _k2_sdl_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()
foreach(_v K2_LLVM_MINGW K2_MSVC_SDK K2_HOST_LLVM K2_MSVC_WINE)   # what the toolchain files look for
  if(${_v})
    list(APPEND _k2_sdl_args "-D${_v}=${${_v}}")
  endif()
endforeach()
ExternalProject_Add(k2sdl2_build
  SOURCE_DIR "${k2sdl2_SOURCE_DIR}"
  BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/k2sdl2-ep-build"
  INSTALL_DIR "${_k2_sdl}"
  LIST_SEPARATOR "|"
  CMAKE_ARGS ${_k2_sdl_args}
  BUILD_BYPRODUCTS "${_k2_sdl_lib}"
  LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON LOG_OUTPUT_ON_FAILURE ON)
ExternalProject_Add_StepDependencies(k2sdl2_build configure "${K2P_DIR}/windows/sdl2_static.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2P_DIR}/windows/sdl_interposed.h")
file(MAKE_DIRECTORY "${_k2_sdl}/include/SDL2")   # an imported target's include dirs must exist
add_library(SDL2 STATIC IMPORTED GLOBAL)

# The Win32 functions SDL calls that the shim also defines, for the engine, by the same C name
# (src/port/win32api.h's user32 / gdi32 / ole32 names: GetClientRect, SwapBuffers ...; on MSVC also a
# CRT one, shim_crt.cpp's _strnicmp). SDL calls them through their import slots (__imp_<name>), but
# user32.lib's member for a name defines the slot AND a <name> thunk: a second <name>. These import
# libraries, searched before the system's, give SDL the slots alone, since DATA imports have no thunk.
# A duplicate symbol between the shim and lib<dll>.a / <dll>.lib in a link is a name to add here.
set(_k2_sdl_imports_USER32
  CallWindowProcW ClientToScreen CreateWindowExA DestroyWindow GetAsyncKeyState GetClientRect
  GetCursorPos GetDC GetKeyState GetWindowLongW MessageBoxA ReleaseCapture ReleaseDC ScreenToClient
  SetCapture SetCursorPos SetWindowPos SystemParametersInfoA TranslateMessage)
set(_k2_sdl_imports_GDI32 ChoosePixelFormat DescribePixelFormat GetPixelFormat SetPixelFormat SwapBuffers)
set(_k2_sdl_imports_ole32 CoUninitialize)
set(_k2_sdl_dlls USER32 GDI32 ole32)
if(MSVC)   # and the CRT names shim_crt.cpp defines for the engine (src/port/crtapi.h)
  set(_k2_sdl_imports_api-ms-win-crt-string-l1-1-0 _strnicmp)
  list(APPEND _k2_sdl_dlls api-ms-win-crt-string-l1-1-0)
endif()
set(_k2_sdl_implibs)
if(NOT MSVC)
  get_filename_component(_k2_cc_dir "${CMAKE_C_COMPILER}" DIRECTORY)
  find_program(K2_DLLTOOL NAMES llvm-dlltool x86_64-w64-mingw32-dlltool HINTS "${_k2_cc_dir}" REQUIRED)
endif()
foreach(_dll ${_k2_sdl_dlls})
  set(_def "${CMAKE_BINARY_DIR}/_deps/k2sdl_${_dll}.def")
  set(_text "LIBRARY ${_dll}.dll\nEXPORTS\n")
  foreach(_n ${_k2_sdl_imports_${_dll}})
    string(APPEND _text "  ${_n} DATA\n")
  endforeach()
  file(CONFIGURE OUTPUT "${_def}" CONTENT "${_text}" @ONLY)
  if(MSVC)
    set(_lib "${CMAKE_BINARY_DIR}/_deps/k2sdl_${_dll}.lib")
    set(_cmd "${CMAKE_AR}" /nologo /machine:x64 "/def:${_def}" "/out:${_lib}")
  else()
    set(_lib "${CMAKE_BINARY_DIR}/_deps/libk2sdl_${_dll}.a")
    set(_cmd "${K2_DLLTOOL}" -m i386:x86-64 -d "${_def}" -l "${_lib}")
  endif()
  if(NOT EXISTS "${_lib}" OR "${_def}" IS_NEWER_THAN "${_lib}")
    execute_process(COMMAND ${_cmd} RESULT_VARIABLE _rc)
    if(_rc)
      message(FATAL_ERROR "k2: building ${_lib} failed (${_rc}): ${_cmd}")
    endif()
  endif()
  list(APPEND _k2_sdl_implibs "${_lib}")
endforeach()
# The Win32 libraries SDL2's CMakeLists links a static SDL2 with (its EXTRA_LIBS on Windows; not
# dinput8: SDL makes its DirectInput object with CoCreateInstance and defines the GUIDs itself).
set(_k2_sdl_syslibs ${_k2_sdl_implibs} kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
                    advapi32 setupapi shell32)
set(_k2_sdl_alt)
foreach(_n ${_k2_sdl_interposed})
  if(MSVC)
    list(APPEND _k2_sdl_alt "/ALTERNATENAME:${_n}=k2sdl_real_${_n}")
  else()
    list(APPEND _k2_sdl_alt "-Wl,--Xlink=-alternatename:${_n}=k2sdl_real_${_n}")
  endif()
endforeach()
set_target_properties(SDL2 PROPERTIES
  IMPORTED_LOCATION "${_k2_sdl_lib}"
  INTERFACE_INCLUDE_DIRECTORIES "${_k2_sdl}/include;${_k2_sdl}/include/SDL2"
  INTERFACE_LINK_LIBRARIES "${_k2_sdl_syslibs}"
  INTERFACE_LINK_OPTIONS "${_k2_sdl_alt}")
add_dependencies(SDL2 k2sdl2_build)
add_library(SDL2::SDL2 ALIAS SDL2)

# ---- FreeType, static, no optional codecs (the font is a plain OTF) -------------------------------
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(k2freetype)
if(NOT TARGET Freetype::Freetype)
  add_library(Freetype::Freetype ALIAS freetype)
endif()

# ---- zlib (shim/shim_obb.cpp: the OBB index is zlib'd) ---------------------------------------------
FetchContent_MakeAvailable(k2zlib)
set(_z "${k2zlib_SOURCE_DIR}")
add_library(k2zlib STATIC
  ${_z}/adler32.c ${_z}/compress.c ${_z}/crc32.c ${_z}/deflate.c ${_z}/gzclose.c ${_z}/gzlib.c
  ${_z}/gzread.c ${_z}/gzwrite.c ${_z}/infback.c ${_z}/inffast.c ${_z}/inflate.c ${_z}/inftrees.c
  ${_z}/trees.c ${_z}/uncompr.c ${_z}/zutil.c)
target_include_directories(k2zlib PUBLIC "${_z}")
target_compile_options(k2zlib PRIVATE $<IF:$<BOOL:${MSVC}>,/w,-w>)
add_library(ZLIB::ZLIB ALIAS k2zlib)
