# compat/include-msvc — the POSIX surface the MSVC UCRT lacks (MSVC builds only: the Windows host presets, *-cross-windows-msvc-pc, *-cross-windows-cl-pc)

Searched before `compat/include-mingw/` (whose dirent / dlfcn / statvfs / stdio / stdlib / string
additions are shared) on the MSVC builds. mingw-w64 supplies these POSIX headers and functions itself
(`unistd.h`, `strings.h`, `sys/time.h`, `localtime_r`, `clock_gettime`, `usleep` ...); the UCRT does
not. The bodies are `platforms/windows/win_msvc_posix.cpp`. `docs/MSVC_PORT.md`.
