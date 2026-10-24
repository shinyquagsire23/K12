# The host presets (<k1|k2>-host-<ui>, CMakePresets.json) on Linux: clang with libc++ and lld, Debug.
# libc++ because the engine's std:: types are the NDK's libc++ layout (linux/CMakeLists.txt). The
# preset names this file as host-${hostSystemName}.cmake; it sets no CMAKE_SYSTEM_NAME, so it is not
# a cross build.
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++ -fuse-ld=lld")
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")
