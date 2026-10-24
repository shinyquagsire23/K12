# platforms/audio -- the FMOD backend the engine links, and its headless decoder test.
#
#   -DK2P_AUDIO=miniaudio   the real one: platforms/audio/fmod_miniaudio.cpp on vendored miniaudio
#   -DK2P_AUDIO=null        the silent one: platforms/null/fmod_null.cpp (FMOD lifetimes, no sound)
#
# Both build the same 29 entry points against the engine's own src/port/fmod/FModAPI.h, so the two
# are interchangeable at link time and an A/B is a reconfigure.
#
#   platforms/out/k2-host-pc/audio/fmodcheck -v <gamedir>/streammusic
#   platforms/out/k2-host-pc/audio/fmodcheck --bif <gamedir>/data/sounds.bif
set(K2P_AUDIO "miniaudio" CACHE STRING "FMOD backend: miniaudio (audible) or null (silent)")
set_property(CACHE K2P_AUDIO PROPERTY STRINGS miniaudio null)

if(K2P_AUDIO STREQUAL "miniaudio")
  add_library(k2_audio STATIC
    "${K2P_DIR}/third_party/miniaudio/k2ma.c"
    k2wavsniff.c
    fmod_miniaudio.cpp)
  target_include_directories(k2_audio
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}" "${K2P_DIR}/third_party/miniaudio"
    PRIVATE ${K2_INCLUDE_DIRS})
  set_target_properties(k2_audio PROPERTIES CXX_STANDARD 11 C_STANDARD 99)
  if(NOT MSVC)
    target_compile_options(k2_audio PRIVATE -Wall -Wextra -Wno-unused-parameter)
  endif()
  if(IOS)
    # miniaudio's iOS backend talks to AVAudioSession: its implementation TU is Objective-C there.
    set_source_files_properties("${K2P_DIR}/third_party/miniaudio/k2ma.c" PROPERTIES COMPILE_OPTIONS "-xobjective-c")
    target_link_libraries(k2_audio PUBLIC
      "-framework CoreFoundation" "-framework AVFoundation" "-framework AudioToolbox" "-framework Foundation")
  elseif(APPLE)
    # miniaudio's CoreAudio backend. AudioToolbox pulls in AudioUnit on every macOS we target.
    target_link_libraries(k2_audio PUBLIC
      "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
  endif()
  find_package(Threads REQUIRED)
  target_link_libraries(k2_audio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

  option(K2P_AUDIO_TOOLS "Build fmodcheck, the headless sniff+decode sweep" ON)
  if(K2P_AUDIO_TOOLS)
    add_executable(fmodcheck tools/fmodcheck.c)
    target_link_libraries(fmodcheck PRIVATE k2_audio)
    set_target_properties(fmodcheck PROPERTIES C_STANDARD 99)
  endif()
else()
  add_library(k2_audio STATIC "${K2P_DIR}/null/fmod_null.cpp")
  target_include_directories(k2_audio PRIVATE ${K2_INCLUDE_DIRS})
  set_target_properties(k2_audio PROPERTIES CXX_STANDARD 11)
endif()
target_compile_definitions(k2_audio PRIVATE K2_HOST_BUILD=1)
message(STATUS "k2: audio backend = ${K2P_AUDIO}")
