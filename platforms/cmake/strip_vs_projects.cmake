# PATCH_COMMAND for a fetched dependency (windows/deps.cmake, android/deps.cmake, k1/deps.cmake), run
# in its source dir: deletes the Visual Studio solutions and projects it ships (SDL2's VisualC/,
# FreeType's builds/windows/, zlib's contrib/vstudio/, 7-Zip's .dsp). The build never uses them, and
# Visual Studio's "Open Folder" scans every folder under platforms/, out/ included, and lists each
# project it finds as a startup item beside k2.exe / k1.exe. Idempotent.
file(GLOB_RECURSE _k2_vsproj LIST_DIRECTORIES false
     *.sln *.vcxproj *.vcxproj.filters *.vcxproj.user *.vcproj *.csproj *.dsp *.dsw)
if(_k2_vsproj)
  file(REMOVE ${_k2_vsproj})
endif()
