// The SDL2 entry points the shim and k1/k1sdl.cpp define themselves, reaching SDL's own behind them
// with dlsym(RTLD_NEXT, name) (windows/win_dlfcn.cpp). SDL2 is linked statically on Windows
// (windows/deps.cmake): its public definition of each of these is named k2sdl_real_<name> (its
// SDL_dynapi.c is compiled with <name>=k2sdl_real_<name>; SDL's own calls go to <name>_REAL, as in
// SDL2.dll), and the link maps <name> to it (/ALTERNATENAME) where no definition of ours is linked
// (KOTOR II has only SDL_RWFromFile). A name missing here is a duplicate-symbol link error.
// One K2SDL_INTERPOSED(name) per line: deps.cmake reads this file.
K2SDL_INTERPOSED(SDL_CreateWindow)
K2SDL_INTERPOSED(SDL_GL_CreateContext)
K2SDL_INTERPOSED(SDL_GL_SetAttribute)
K2SDL_INTERPOSED(SDL_GL_SetSwapInterval)
K2SDL_INTERPOSED(SDL_GL_SwapWindow)
K2SDL_INTERPOSED(SDL_GetDisplayMode)
K2SDL_INTERPOSED(SDL_JoystickOpen)
K2SDL_INTERPOSED(SDL_PollEvent)
K2SDL_INTERPOSED(SDL_RWFromFile)
