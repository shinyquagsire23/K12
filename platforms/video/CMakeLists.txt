# platforms/video -- k2bink (Bink 1 decoder, C99) and its host-only test tools.
#
#   cmake -S platforms/video -B platforms/out/video -G Ninja
#   cmake --build platforms/out/video
#   platforms/out/video/binkcheck <movie.bik>...          # headless; ffmpeg as the oracle
#   platforms/out/video/binkplay  <movie.bik>             # SDL2 window + audio
#   platforms/out/video/binkgl    <movie.bik>             # ASL::PlayBinkMovieGL in a GL 2.1 window
#   platforms/out/video/binkgl --verify 30 <movie.bik>    # shader output vs a CPU model, per pixel
#
# Standalone on purpose: it does not configure the engine (the root CMakeLists.txt), so iterating
# on the decoder costs seconds. k2bink has no dependencies; k2movie and the SDL tools need SDL2.
cmake_minimum_required(VERSION 3.21)
project(k2bink LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_library(k2bink STATIC
  k2bink/k2bink.c
  k2bink/k2bink_video.c
  k2bink/k2bink_audio.c
  k2bink/k2bink_tables.c
  k2bink/k2bink_quant.c)
target_include_directories(k2bink PUBLIC k2bink)
if(NOT MSVC)
  target_compile_options(k2bink PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
if(NOT WIN32)
  target_link_libraries(k2bink PRIVATE m)
endif()

# ---- k2movie: the player behind ASL::PlayBinkMovieGL (SDL2 + GL 2.1, GL loaded through SDL) ------
if(TARGET SDL2::SDL2)   # built by the including project (platforms/android/deps.cmake)
  set(SDL2_FOUND ON)
else()
  find_package(SDL2 CONFIG QUIET)
endif()
if(SDL2_FOUND)
  set(K2BINK_SDL2 $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
  enable_language(CXX)
  add_library(k2movie STATIC player/k2movie.c player/asl_playbink.cpp)
  target_include_directories(k2movie PUBLIC player)
  target_link_libraries(k2movie PUBLIC k2bink ${K2BINK_SDL2})
  if(NOT MSVC)
    target_compile_options(k2movie PRIVATE -Wall -Wextra -Wno-unused-parameter)
  endif()
  set_target_properties(k2movie PROPERTIES CXX_STANDARD 11)
  # Windows: its opendir/readdir are the shim's (shim/shim_path.cpp), so it reads their struct dirent.
  if(K2P_MINGW_INCLUDE)
    set_property(SOURCE player/asl_playbink.cpp APPEND PROPERTY INCLUDE_DIRECTORIES ${K2P_MINGW_INCLUDE})
  endif()
  # BinkSubtitles comes from the engine (src/libsource/AURORA/aurbink) and draws through ASL's text
  # path, which the port does not implement yet; no shipped movie has a .sub.txt. The including project
  # supplies the engine's include dirs and objects when it turns this on.
  option(K2P_BINK_SUBTITLES "Draw <movie>.sub.txt subtitles through the engine's BinkSubtitles" OFF)
  if(K2P_BINK_SUBTITLES)
    target_compile_definitions(k2movie PRIVATE K2P_BINK_SUBTITLES=1)
  endif()
else()
  message(STATUS "k2bink: SDL2 not found, skipping k2movie, binkplay and binkgl")
endif()

option(K2BINK_TOOLS "Build binkcheck (headless), binkplay (SDL2) and binkgl (GL player test)" ${PROJECT_IS_TOP_LEVEL})
if(K2BINK_TOOLS)
  add_executable(binkcheck tools/binkcheck.c)
  target_link_libraries(binkcheck PRIVATE k2bink m)

  if(SDL2_FOUND)
    add_executable(binkplay tools/binkplay.c)
    target_link_libraries(binkplay PRIVATE k2bink ${K2BINK_SDL2})

    add_executable(binkgl tools/binkgl.cpp)
    target_link_libraries(binkgl PRIVATE k2movie)
    set_target_properties(binkgl PROPERTIES CXX_STANDARD 11)
  endif()
endif()
