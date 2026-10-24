# Win64 with the MSVC C++ ABI, cross-compiled by clang-cl from macOS / Linux (presets *-cross-windows-msvc-pc);
# clang-cl + lld-link + llvm-lib/rc/mt against the MSVC CRT, STL and Windows SDK
# that windows/get_msvc_sdk.sh splats into platforms/out/_msvc_sdk/splat (xwin). On Windows the
# host presets (<k1|k2>-host-<ui>, cmake/host-Windows.cmake) use Visual Studio's own cl.exe and need no toolchain file.
#
# Where things are looked for, first hit wins:
#   -DK2_MSVC_SDK=<dir> or $K2_MSVC_SDK      the splat root (crt/ and sdk/)
#   platforms/out/_msvc_sdk/splat
#   -DK2_HOST_LLVM=<dir> or $K2_HOST_LLVM    a dir with clang-cl (Homebrew's llvm/bin, then PATH)
#   lld-link: the same dir, else llvm-mingw's bin/ (out/_llvm-mingw), else PATH
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT K2_MSVC_SDK AND DEFINED ENV{K2_MSVC_SDK})
  set(K2_MSVC_SDK "$ENV{K2_MSVC_SDK}")
endif()
if(NOT K2_MSVC_SDK AND IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../out/_msvc_sdk/splat/crt/include")
  set(K2_MSVC_SDK "${CMAKE_CURRENT_LIST_DIR}/../out/_msvc_sdk/splat")
endif()
if(NOT K2_MSVC_SDK)
  message(FATAL_ERROR "k2: no MSVC CRT / Windows SDK: run platforms/windows/get_msvc_sdk.sh (it asks you to "
                      "accept Microsoft's license), or pass -DK2_MSVC_SDK=<splat dir>")
endif()
get_filename_component(K2_MSVC_SDK "${K2_MSVC_SDK}" ABSOLUTE)

if(NOT K2_HOST_LLVM AND DEFINED ENV{K2_HOST_LLVM})
  set(K2_HOST_LLVM "$ENV{K2_HOST_LLVM}")
endif()
set(_k2_hints ${K2_HOST_LLVM} /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
file(GLOB _k2_lm LIST_DIRECTORIES true "${CMAKE_CURRENT_LIST_DIR}/../out/_llvm-mingw/llvm-mingw-*/bin")
find_program(K2_CLANG_CL clang-cl HINTS ${_k2_hints})
if(NOT K2_CLANG_CL)   # Homebrew's llvm 22 ships clang without the clang-cl name: clang picks cl mode by its name
  find_program(_k2_clang clang HINTS ${_k2_hints} NO_DEFAULT_PATH REQUIRED)
  set(_k2_cl "${CMAKE_CURRENT_LIST_DIR}/../out/_clang-cl/clang-cl")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../out/_clang-cl")
  file(CREATE_LINK "${_k2_clang}" "${_k2_cl}" SYMBOLIC)
  set(K2_CLANG_CL "${_k2_cl}" CACHE FILEPATH "" FORCE)
endif()
find_program(K2_LLD_LINK lld-link HINTS ${_k2_hints} ${_k2_lm} REQUIRED)
find_program(K2_LLVM_LIB llvm-lib HINTS ${_k2_hints} ${_k2_lm} REQUIRED)
find_program(K2_LLVM_RC llvm-rc HINTS ${_k2_hints} ${_k2_lm} REQUIRED)
find_program(K2_LLVM_MT llvm-mt HINTS ${_k2_hints} ${_k2_lm})

set(CMAKE_C_COMPILER "${K2_CLANG_CL}")
set(CMAKE_CXX_COMPILER "${K2_CLANG_CL}")
set(CMAKE_LINKER "${K2_LLD_LINK}")
set(CMAKE_AR "${K2_LLVM_LIB}")
set(CMAKE_RC_COMPILER "${K2_LLVM_RC}")
if(K2_LLVM_MT)
  set(CMAKE_MT "${K2_LLVM_MT}")
endif()
# llvm-rc reads an input path under /Users as its /U option: windows/llvm-rc-cross.sh puts `--` before
# it. CMake recognises llvm-rc by its file name, so the wrapper is an `llvm-rc` of its own.
if(NOT CMAKE_HOST_WIN32)
  set(_k2_rcw "${K2_MSVC_SDK}/../bin/llvm-rc")
  file(WRITE "${_k2_rcw}.tmp" "#!/bin/sh\nexec \"${CMAKE_CURRENT_LIST_DIR}/llvm-rc-cross.sh\" \"${K2_LLVM_RC}\" \"$@\"\n")
  file(COPY_FILE "${_k2_rcw}.tmp" "${_k2_rcw}" ONLY_IF_DIFFERENT)
  file(CHMOD "${_k2_rcw}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
  get_filename_component(_k2_rcw "${_k2_rcw}" ABSOLUTE)
  set(CMAKE_RC_COMPILER "${_k2_rcw}")
endif()
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

# The splat's headers as system headers, its libraries on the link path.
set(_k2_inc crt/include sdk/include/ucrt sdk/include/um sdk/include/shared sdk/include/winrt)
set(_k2_flags "")
foreach(_d IN LISTS _k2_inc)
  string(APPEND _k2_flags " /imsvc \"${K2_MSVC_SDK}/${_d}\"")
endforeach()
set(CMAKE_C_FLAGS_INIT "${_k2_flags}")
set(_k2_rc "")
foreach(_d IN LISTS _k2_inc)
  string(APPEND _k2_rc " -I${K2_MSVC_SDK}/${_d}")
endforeach()
set(CMAKE_RC_FLAGS_INIT "${_k2_rc}")
set(CMAKE_CXX_FLAGS_INIT "${_k2_flags}")
set(_k2_libs crt/lib/x86_64 sdk/lib/um/x86_64 sdk/lib/ucrt/x86_64)
set(_k2_link "")
foreach(_d IN LISTS _k2_libs)
  string(APPEND _k2_link " /libpath:\"${K2_MSVC_SDK}/${_d}\"")
endforeach()
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_k2_link}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_k2_link}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_k2_link}")

set(CMAKE_FIND_ROOT_PATH "${K2_MSVC_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
