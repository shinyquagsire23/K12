# Third-party code the Android build needs and the host gets from Homebrew: SDL2, FreeType and gl4es.
# Fetched once (pinned URL + SHA-256) into platforms/out/_deps-android, shared by every Android
# preset, so a second preset or a clean build dir does not download again.
#
#   SDL2 2.32.10  stock, both halves: libSDL2.so and the org.libsdl.app Java classes from the same
#                 tarball (android/CMakeLists.txt compiles them). The engine's Java bridge goes to the
#                 stub JNIEnv in compat/android_compat.cpp, not to Java.
#   FreeType      the dialogue font (ftgl/ftgl_host.cpp), as on the host.
#   gl4es         the GL 1.x/ARB surface the shim targets (immediate mode, the matrix stacks, client
#                 arrays, GLSL 1.20 built-ins) on GLES 2. Aspyr's Android ASLgl did the same job with
#                 gles2-bc plus its own glBegin emulation.
set(FETCHCONTENT_BASE_DIR "${K2P_DIR}/out/_deps-android" CACHE PATH "")   # before the include
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# cmake/strip_vs_projects.cmake: the archives' Visual Studio projects would show up as startup items
# in Visual Studio's "Open Folder" view of platforms/ (on a Windows host).
set(_k2_strip_vs "${CMAKE_COMMAND}" -P "${K2P_DIR}/cmake/strip_vs_projects.cmake")
FetchContent_Declare(k2sdl2
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
  URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
  PATCH_COMMAND ${_k2_strip_vs})
FetchContent_Declare(k2freetype
  URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-3.tar.gz
  URL_HASH SHA256=bc5c898e4756d373e0d991bab053036c5eb2aa7c0d5c67e8662ddc6da40c4103
  PATCH_COMMAND ${_k2_strip_vs})
FetchContent_Declare(k2gl4es
  URL https://github.com/ptitSeb/gl4es/archive/a444cc94b17c672c66c3e6ce07428dc603034db1.tar.gz
  URL_HASH SHA256=54dc6e6b5eb04f9a9eb7945ba564b868dd65e7fceeabafe02eaa68cb735f04ce
  SOURCE_SUBDIR k2-no-cmake    # sources only: MakeAvailable finds no CMakeLists.txt to add
  PATCH_COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/gl4es_patch.cmake" COMMAND ${_k2_strip_vs})

# ---- SDL2: libSDL2.so, which SDLActivity loads by name -------------------------------------------
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(k2sdl2)
set(K2_SDL2_SOURCE_DIR "${k2sdl2_SOURCE_DIR}")

# ---- FreeType, static, no optional codecs (the font is a plain OTF) -------------------------------
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(k2freetype)
set_target_properties(freetype PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(NOT TARGET Freetype::Freetype)
  add_library(Freetype::Freetype ALIAS freetype)
endif()

# ---- gl4es: a static libGL with no EGL/X11 of its own --------------------------------------------
# Its CMakeLists writes into CMAKE_SOURCE_DIR, so it is compiled here from its source list instead
# of add_subdirectory'd. NOEGL + NO_LOADER: every GLES entry point comes from the getprocaddress
# the shim installs (SDL_GL_GetProcAddress); NO_INIT_CONSTRUCTOR: the shim calls initialize_gl4es()
# once the first context is current. DEFAULT_ES=2: SDL creates a GLES 2 context.
FetchContent_MakeAvailable(k2gl4es)
# PATCH_COMMAND runs once, when the sources are fetched; the patch is idempotent, so it is applied
# on every configure too, and an edit to it reaches an existing download.
execute_process(COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/gl4es_patch.cmake" WORKING_DIRECTORY "${k2gl4es_SOURCE_DIR}"
                COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/gl4es_patch.cmake")
# The list is gl4es's own src/CMakeLists.txt GL_SRC for a non-Linux target: gl/, gl/wrap/, gl/math/
# and glx/hardext.c (the hardware-extension probe; no GLX).
file(GLOB _gl4es_src CONFIGURE_DEPENDS "${k2gl4es_SOURCE_DIR}/src/gl/*.c" "${k2gl4es_SOURCE_DIR}/src/gl/wrap/*.c"
                                       "${k2gl4es_SOURCE_DIR}/src/gl/math/*.c")
add_library(k2gl4es STATIC ${_gl4es_src} "${k2gl4es_SOURCE_DIR}/src/glx/hardext.c")
target_include_directories(k2gl4es PUBLIC "${k2gl4es_SOURCE_DIR}/include"
                                   PRIVATE "${k2gl4es_SOURCE_DIR}/src/glx")
target_compile_definitions(k2gl4es PRIVATE NOX11 NOEGL NO_GBM NO_LOADER NO_INIT_CONSTRUCTOR STATICLIB
                                           DEFAULT_ES=2 USE_ANDROID_LOG)
target_compile_options(k2gl4es PRIVATE -std=gnu99 -funwind-tables -O2 -w
                                       -include "${k2gl4es_SOURCE_DIR}/include/android_debug.h")
set_target_properties(k2gl4es PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(k2gl4es PUBLIC log m)
