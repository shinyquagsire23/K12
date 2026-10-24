# Win64 (x86_64-w64-mingw32) with llvm-mingw: clang + lld + libc++ + the mingw-w64 UCRT runtime.
#
# The <k1|k2>-cross-windows-<ui> presets: the same toolchain on every host, macOS / Linux and Windows
# itself (Visual Studio's "Open Folder" CMake support lists those presets beside the host presets,
# which build with its own cl.exe). clang with libc++
# and the Itanium C++ ABI, as on every other target, so the link report (an analysis, not a link
# input) classifies by the same mangled names. The MSVC ABI builds are the host presets on Windows
# (cl.exe), *-cross-windows-msvc-pc (clang-cl, windows/msvc.toolchain.cmake) and *-cross-windows-cl-pc
# (cl.exe under Wine).
#
# Where llvm-mingw is looked for, first hit wins:
#   -DK2_LLVM_MINGW=<dir> or $K2_LLVM_MINGW    the toolchain's root (the dir holding bin/)
#   platforms/out/_llvm-mingw/llvm-mingw-*     what platforms/windows/get_llvm_mingw.sh unpacks
#   x86_64-w64-mingw32-clang on PATH
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(_k2_triple x86_64-w64-mingw32)

if(NOT K2_LLVM_MINGW AND DEFINED ENV{K2_LLVM_MINGW})
  set(K2_LLVM_MINGW "$ENV{K2_LLVM_MINGW}")
endif()
if(NOT K2_LLVM_MINGW)
  file(GLOB _k2_lm LIST_DIRECTORIES true "${CMAKE_CURRENT_LIST_DIR}/../out/_llvm-mingw/llvm-mingw-*")
  list(SORT _k2_lm)
  list(REVERSE _k2_lm)
  foreach(_d IN LISTS _k2_lm)
    if(IS_DIRECTORY "${_d}/bin")
      set(K2_LLVM_MINGW "${_d}")
      break()
    endif()
  endforeach()
endif()
if(NOT K2_LLVM_MINGW)
  find_program(_k2_clang ${_k2_triple}-clang)
  if(_k2_clang)
    get_filename_component(K2_LLVM_MINGW "${_k2_clang}" DIRECTORY)
    get_filename_component(K2_LLVM_MINGW "${K2_LLVM_MINGW}" DIRECTORY)
  else()
    message(FATAL_ERROR "k2: llvm-mingw not found: run platforms/windows/get_llvm_mingw.sh, or pass "
                        "-DK2_LLVM_MINGW=<dir> / set K2_LLVM_MINGW")
  endif()
endif()
set(K2_LLVM_MINGW "${K2_LLVM_MINGW}" CACHE PATH "llvm-mingw root" FORCE)
# try_compile projects re-read this file: hand them the same toolchain.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES K2_LLVM_MINGW)

if(CMAKE_HOST_WIN32)
  set(_k2_exe ".exe")
else()
  set(_k2_exe "")
endif()
set(_k2_bin "${K2_LLVM_MINGW}/bin")
set(CMAKE_C_COMPILER   "${_k2_bin}/${_k2_triple}-clang${_k2_exe}")
set(CMAKE_CXX_COMPILER "${_k2_bin}/${_k2_triple}-clang++${_k2_exe}")
set(CMAKE_ASM_COMPILER "${_k2_bin}/${_k2_triple}-clang${_k2_exe}")
set(CMAKE_RC_COMPILER  "${_k2_bin}/${_k2_triple}-windres${_k2_exe}")
set(CMAKE_AR           "${_k2_bin}/llvm-ar${_k2_exe}")
set(CMAKE_RANLIB       "${_k2_bin}/llvm-ranlib${_k2_exe}")
set(CMAKE_OBJCOPY      "${_k2_bin}/llvm-objcopy${_k2_exe}")
set(K2_LLVM_BIN "${_k2_bin}")

set(CMAKE_FIND_ROOT_PATH "${K2_LLVM_MINGW}/${_k2_triple}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
