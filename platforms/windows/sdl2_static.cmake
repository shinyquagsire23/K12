# Included by SDL2's own build (CMAKE_PROJECT_INCLUDE, windows/deps.cmake), right after its project():
# SDL_dynapi.c, which defines SDL's public entry points (each a jump to <name>_REAL), names the ones
# the shim defines itself k2sdl_real_<name> (windows/sdl_interposed.h, passed as K2_SDL_INTERPOSED).
set(_k2_defs)
foreach(_n ${K2_SDL_INTERPOSED})
  list(APPEND _k2_defs "${_n}=k2sdl_real_${_n}")
endforeach()
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/dynapi/SDL_dynapi.c" APPEND PROPERTY
             COMPILE_DEFINITIONS ${_k2_defs})

# The DirectInput GUIDs SDL_windowsjoystick.c defines (INITGUID over <dinput.h>) that the shim
# defines too, for the engine (shim_dinput.cpp): SDL's are its own, under its own names.
add_compile_definitions(
  IID_IDirectInput8A=k2sdl_IID_IDirectInput8A IID_IDirectInputDevice8A=k2sdl_IID_IDirectInputDevice8A
  GUID_SysKeyboard=k2sdl_GUID_SysKeyboard GUID_SysMouse=k2sdl_GUID_SysMouse)
