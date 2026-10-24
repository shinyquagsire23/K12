# Win64 with Visual Studio 2022's own cl.exe / link.exe, run under Wine from macOS / Linux (presets
# *-cross-windows-cl-pc): the check that the Windows host presets (cl.exe) build without a
# Windows box. The toolset is msvc-wine's (https://github.com/mstorsjo/msvc-wine) under
# platforms/out/_msvc_wine/msvc17 (windows/get_msvc_wine.sh), whose bin/x64 wrappers run the tools
# through wine and set INCLUDE / LIB themselves.
#
#   -DK2_MSVC_WINE=<dir> or $K2_MSVC_WINE     the msvc-wine install (with bin/x64/cl)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT K2_MSVC_WINE AND DEFINED ENV{K2_MSVC_WINE})
  set(K2_MSVC_WINE "$ENV{K2_MSVC_WINE}")
endif()
if(NOT K2_MSVC_WINE)
  set(K2_MSVC_WINE "${CMAKE_CURRENT_LIST_DIR}/../out/_msvc_wine/msvc17")
endif()
get_filename_component(K2_MSVC_WINE "${K2_MSVC_WINE}" ABSOLUTE)
if(NOT EXISTS "${K2_MSVC_WINE}/bin/x64/cl")
  message(FATAL_ERROR "k2: no msvc-wine toolset at ${K2_MSVC_WINE}: run platforms/windows/get_msvc_wine.sh "
                      "(it asks you to accept Microsoft's license), or pass -DK2_MSVC_WINE=<dir>")
endif()
set(_k2_bin "${K2_MSVC_WINE}/bin/x64")
set(CMAKE_C_COMPILER "${_k2_bin}/cl")
set(CMAKE_CXX_COMPILER "${_k2_bin}/cl")
# lib.exe and link.exe through windows/msvc-wine-rsp.sh: a Windows command line holds 32767
# characters, and Ninja on a macOS / Linux host passes the object lists inline. CMake takes one
# program for each, so the toolset's bin/ gets a two-line script per tool.
foreach(_t lib link)
  file(WRITE "${K2_MSVC_WINE}/bin/k2${_t}.tmp"
       "#!/bin/sh\nexec \"${CMAKE_CURRENT_LIST_DIR}/msvc-wine-rsp.sh\" \"${_k2_bin}/${_t}\" \"$@\"\n")
  file(COPY_FILE "${K2_MSVC_WINE}/bin/k2${_t}.tmp" "${K2_MSVC_WINE}/bin/k2${_t}" ONLY_IF_DIFFERENT)
  file(CHMOD "${K2_MSVC_WINE}/bin/k2${_t}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)
endforeach()
set(CMAKE_LINKER "${K2_MSVC_WINE}/bin/k2link")
set(CMAKE_AR "${K2_MSVC_WINE}/bin/k2lib")
set(CMAKE_RC_COMPILER "${_k2_bin}/rc")
# mt.exe exits 1 without a word under Wine: the manifest step takes llvm-mt (as the clang-cl presets do).
find_program(K2_LLVM_MT llvm-mt HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin REQUIRED)
set(CMAKE_MT "${K2_LLVM_MT}")

# The UCRT headers the root CMakeLists.txt copies (on Windows it finds them through INCLUDE).
file(GLOB _k2_ucrt LIST_DIRECTORIES true "${K2_MSVC_WINE}/kits/10/include/*/ucrt")
list(GET _k2_ucrt -1 K2_MSVC_UCRT)

# /Z7: the debug info in the objects. /Zi's shared PDB needs mspdbsrv, which needs winbind under wine.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
