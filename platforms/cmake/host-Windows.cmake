# The host presets (<k1|k2>-host-<ui>, CMakePresets.json) on Windows: Visual Studio's own cl.exe,
# RelWithDebInfo. Run from an x64 developer environment: Visual Studio sets it up itself (the presets'
# architecture x64 / external), a shell needs vcvars64.bat first. No link report: it classifies
# Itanium-mangled names, and cl.exe mangles the MSVC way. The preset names this file as
# host-${hostSystemName}.cmake; it sets no CMAKE_SYSTEM_NAME, so it is not a cross build.
set(CMAKE_C_COMPILER cl.exe)
set(CMAKE_CXX_COMPILER cl.exe)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "")
set(K2P_LINKREPORT OFF CACHE BOOL "")
