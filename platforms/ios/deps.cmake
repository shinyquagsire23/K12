# Third-party code the iOS build needs and the macOS host gets from Homebrew: SDL2, FreeType and
# gl4es, the same pinned sources as android/deps.cmake. Fetched once into platforms/out/_deps-ios,
# shared by every iOS preset.
#
#   SDL2      2.32.10 with the UIScene lifecycle patched in (ios/sdl2_patch.cmake: iOS 27 refuses an
#             app built with its SDK that has none), as a dylib in the app's Frameworks/: KOTOR 1's
#             k1/k1sdl.cpp defines SDL_CreateWindow, SDL_GL_SetAttribute ... in front of SDL's and
#             reaches the real ones through dlsym(RTLD_NEXT), which a static SDL cannot answer (macOS:
#             the same, with Homebrew's dylib).
#   FreeType  the dialogue font (ftgl/ftgl_host.cpp).
#   gl4es     the GL 1.x/ARB surface the shim targets, on the GLES 2 context SDL makes with EAGL
#             (OpenGLES.framework), with the Android build's patches (android/gl4es_patch.cmake).
set(FETCHCONTENT_BASE_DIR "${K2P_DIR}/out/_deps-ios" CACHE PATH "")   # before the include
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(k2sdl2
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
  URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
  PATCH_COMMAND ${CMAKE_COMMAND} -P "${K2P_DIR}/ios/sdl2_patch.cmake")   # the UIScene lifecycle
FetchContent_Declare(k2freetype
  URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-3.tar.gz
  URL_HASH SHA256=bc5c898e4756d373e0d991bab053036c5eb2aa7c0d5c67e8662ddc6da40c4103)
FetchContent_Declare(k2gl4es
  URL https://github.com/ptitSeb/gl4es/archive/a444cc94b17c672c66c3e6ce07428dc603034db1.tar.gz
  URL_HASH SHA256=54dc6e6b5eb04f9a9eb7945ba564b868dd65e7fceeabafe02eaa68cb735f04ce
  SOURCE_SUBDIR k2-no-cmake    # sources only: MakeAvailable finds no CMakeLists.txt to add
  PATCH_COMMAND ${CMAKE_COMMAND} -P "${K2P_DIR}/android/gl4es_patch.cmake")

# ---- SDL2: libSDL2-2.0.dylib, embedded in the app ---------------------------------------------------
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)   # ios/main_ios.cpp calls SDL_UIKitRunApp itself
FetchContent_MakeAvailable(k2sdl2)
set(K2_SDL2_SOURCE_DIR "${k2sdl2_SOURCE_DIR}")
# Loaded from the bundle's Frameworks/ (ios/CMakeLists.txt copies it there).
set_target_properties(SDL2 PROPERTIES INSTALL_NAME_DIR "@rpath" BUILD_WITH_INSTALL_NAME_DIR ON)

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

# ---- gl4es: a static libGL with no EGL/X11 of its own (the compile definitions below are the same
# ones the Android build passes) ---------------------------------------------------------------
FetchContent_MakeAvailable(k2gl4es)
# PATCH_COMMAND runs once, when the sources are fetched; the patch is idempotent, so it is applied
# on every configure too, and an edit to it reaches an existing download.
execute_process(COMMAND ${CMAKE_COMMAND} -P "${K2P_DIR}/android/gl4es_patch.cmake" WORKING_DIRECTORY "${k2gl4es_SOURCE_DIR}"
                COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${K2P_DIR}/android/gl4es_patch.cmake")
file(GLOB _gl4es_src CONFIGURE_DEPENDS "${k2gl4es_SOURCE_DIR}/src/gl/*.c" "${k2gl4es_SOURCE_DIR}/src/gl/wrap/*.c"
                                       "${k2gl4es_SOURCE_DIR}/src/gl/math/*.c")
add_library(k2gl4es STATIC ${_gl4es_src} "${k2gl4es_SOURCE_DIR}/src/glx/hardext.c")
target_include_directories(k2gl4es PUBLIC "${k2gl4es_SOURCE_DIR}/include"
                                   PRIVATE "${k2gl4es_SOURCE_DIR}/src/glx")
target_compile_definitions(k2gl4es PRIVATE NOX11 NOEGL NO_GBM NO_LOADER NO_INIT_CONSTRUCTOR STATICLIB
                                           DEFAULT_ES=2)
target_compile_options(k2gl4es PRIVATE -std=gnu99 -O2 -w)
target_link_libraries(k2gl4es PUBLIC m)
