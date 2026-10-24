# K1 (K2P_GAME=k1): the third-party code K1's port library linked as separate .so files
# (libminiz.so, libLzmaLib.so), fetched once (pinned URL + SHA-256) into platforms/out/_deps-k1 and
# shared by every K1 preset, host and Android alike.
#
#   miniz 3.0.2      the OBB zip reader (k1zip.cpp), and mz_zip_reader_locate_file, which the
#                    engine calls itself (CExoBaseInternal::GetDirectoryList)
#   LZMA SDK 24.09   LzmaDec.c, behind MacDecompress (k1lzma.c): the .bzf BIF resources
#
# Sets K2P_K1_PLATFORM_SOURCE: compiled into each platform's k2_platform, like the shim.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(_k1_deps_dir "${K2P_DIR}/out/_deps-k1")

FetchContent_Declare(k1miniz
  URL https://github.com/richgel999/miniz/releases/download/3.0.2/miniz-3.0.2.zip
  URL_HASH SHA256=ada38db0b703a56d3dd6d57bf84a9c5d664921d870d8fea4db153979fb5332c5
  SOURCE_DIR "${_k1_deps_dir}/miniz-src"
  SUBBUILD_DIR "${_k1_deps_dir}/miniz-subbuild"
  BINARY_DIR "${_k1_deps_dir}/miniz-build"
  SOURCE_SUBDIR k1-no-cmake)    # sources only
FetchContent_Declare(k1lzma
  URL https://github.com/ip7z/7zip/archive/refs/tags/24.09.tar.gz
  URL_HASH SHA256=e4757b307925227724a2044651193664ba3d04e9ac13b8e631f1667896014bbf
  SOURCE_DIR "${_k1_deps_dir}/7zip-src"
  SUBBUILD_DIR "${_k1_deps_dir}/7zip-subbuild"
  BINARY_DIR "${_k1_deps_dir}/7zip-build"
  SOURCE_SUBDIR k1-no-cmake)
FetchContent_MakeAvailable(k1miniz k1lzma)

set(K2P_K1_MINIZ_DIR "${k1miniz_SOURCE_DIR}")
set(K2P_K1_LZMA_DIR "${k1lzma_SOURCE_DIR}/C")
set(K2P_K1_PLATFORM_SOURCE
  "${K2P_DIR}/k1/k1zip.cpp"
  "${K2P_DIR}/k1/k1lzma.c"
  "${K2P_K1_MINIZ_DIR}/miniz.c"
  "${K2P_K1_LZMA_DIR}/LzmaDec.c")

# Source properties are per directory: each platform's CMakeLists calls this on its k2_platform.
function(k1_platform_sources target)
  set(_third "${K2P_K1_MINIZ_DIR}/miniz.c" "${K2P_K1_LZMA_DIR}/LzmaDec.c")
  set_source_files_properties(${_third} PROPERTIES COMPILE_OPTIONS "-w")
  # miniz's large-file path (the main OBB is 2 GB): 64-bit offsets on every target
  set_source_files_properties("${K2P_K1_MINIZ_DIR}/miniz.c" "${K2P_DIR}/k1/k1zip.cpp" PROPERTIES
    COMPILE_DEFINITIONS "_LARGEFILE64_SOURCE=1;_FILE_OFFSET_BITS=64")
  target_include_directories(${target} PRIVATE "${K2P_K1_MINIZ_DIR}" "${K2P_K1_LZMA_DIR}" "${K2P_DIR}/k1")
endfunction()
