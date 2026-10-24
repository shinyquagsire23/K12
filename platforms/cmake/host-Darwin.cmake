# The host presets (<k1|k2>-host-<ui>, CMakePresets.json) on macOS: Apple's clang from the active
# Xcode / Command Line Tools, Debug. The preset names this file as host-${hostSystemName}.cmake so one
# preset name builds natively on every desktop OS; it sets no CMAKE_SYSTEM_NAME, so it is not a
# cross build.
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")
