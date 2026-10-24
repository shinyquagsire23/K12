// K1 only: the SDL2 stream K1 reads models, resources and async file requests through, in place of
// stdio handles.
#ifndef SDLRWOPS_H
#define SDLRWOPS_H
#include "k2stdint.h"

#if KOTOR1
typedef struct SDL_RWops {
    int64_t (*size)(struct SDL_RWops* context);
    int64_t (*seek)(struct SDL_RWops* context, int64_t offset, int whence);
    uint64_t (*read)(struct SDL_RWops* context, void* ptr, uint64_t size,
                          uint64_t maxnum);
    uint64_t (*write)(struct SDL_RWops* context, const void* ptr, uint64_t size,
                           uint64_t num);
    int (*close)(struct SDL_RWops* context);
    unsigned int type;
    union {
        struct {
            int autoclose;
            struct __sFILE* fp;
        } stdio;
    } hidden;
} SDL_RWops;

#define SDL_RWOPS_STDFILE 2

#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2

#define SDL_RWsize(ctx)                 (ctx)->size(ctx)
#define SDL_RWseek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define SDL_RWtell(ctx)                 (ctx)->seek(ctx, 0, RW_SEEK_CUR)
#define SDL_RWread(ctx, ptr, size, n)   (ctx)->read(ctx, ptr, size, n)
#define SDL_RWwrite(ctx, ptr, size, n)  (ctx)->write(ctx, ptr, size, n)
#define SDL_RWclose(ctx)                (ctx)->close(ctx)

extern "C" SDL_RWops* SDL_RWFromFile(const char* file, const char* mode);
#endif

#endif
