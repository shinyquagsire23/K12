// The Win32 / ASL / WGL / ASLgl surface Aspyr's port layer provides inside the Android libkotor2.so
// and the macOS KOTOR2sub, reimplemented on SDL2 + POSIX for the host builds.
//
// The engine calls this layer by its Windows names (CreateWindowExA, PeekMessageA, FindFirstFileA,
// wglCreateContext, ...). src/port/win32api.h declares the kernel32/winmm functions -- threads, sync,
// files and directories, time, memory status, last error -- and the shim files that define or call
// them include it; each engine TU still declares the window and GL functions locally, so the records
// below are laid out to match those: LP64, Windows' own field order and widths, the offsets pinned by
// static_asserts. Behaviour follows Aspyr's macOS implementation where it matters.
#ifndef K2SHIM_H
#define K2SHIM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <string>
#include <vector>

#if defined(_WIN32)
#define K2_GLAPI __stdcall
#else
#define K2_GLAPI
#endif

// Win32 scalar types, as the engine declares them on LP64.
typedef uint32_t DWORD;     // `unsigned int` in the engine's prototypes
typedef int32_t  BOOL;
typedef void*    HANDLE;
struct HWND__;
struct HDC__;
struct HINSTANCE__;

// Win32 error codes the shim reports through GetLastError.
enum {
    K2_ERROR_SUCCESS = 0,
    K2_ERROR_FILE_NOT_FOUND = 2,
    K2_ERROR_PATH_NOT_FOUND = 3,
    K2_ERROR_ACCESS_DENIED = 5,
    K2_ERROR_INVALID_HANDLE = 6,
    K2_ERROR_NO_MORE_FILES = 18,
    K2_ERROR_INVALID_PARAMETER = 87,
    K2_ERROR_ALREADY_EXISTS = 183,
};

// errno -> Win32 error, for the file functions.
DWORD k2shim_errno_to_win32(int e);

// Hardware cursors for the pc UI, the bodies of LoadCursorA, SetCursor and ShowCursor.
void* k2shim_cursor_load(uint64_t nId);
void* k2shim_cursor_select(void* h);
void  k2shim_cursor_show(bool bShow);
void  k2shim_cursor_refresh();
// The xbox UIs' pointer: a pad press hides it and parks it off screen until the mouse moves again.
// from_pad returns true when the pointer was showing (the caller then parks the engine's pointer);
// pad_activity says whether an event is a pad press or a stick or trigger pushed past its drift.
union SDL_Event;
bool  k2shim_pointer_from_pad();
void  k2shim_pointer_from_mouse();
bool  k2shim_pad_activity(const SDL_Event* e);
// Where the parked pointer sits, in engine coordinates: off every edge, and not the engine's -1
// "no position".
enum { K2SHIM_POINTER_PARKED = -30000 };

// Describes the desktop the Fullscreen option opens on, as the size in points a fullscreen window
// gets on the main display and the display's pixels per point (1 with K2_HIDPI=0).
void k2shim_fullscreen_points(int* pW, int* pH);
int  k2shim_pixel_scale();
float k2shim_pixel_scalef();   // iOS: UIScreen's nativeScale (2.88 on some iPhones); else as above
// Linux X11: a fullscreen window stays composited (no tearing); a no-op elsewhere.
struct SDL_Window;
void k2shim_keep_composited(SDL_Window* pWindow);
// macOS / Linux: the game's icon on the window and the Dock; a no-op elsewhere.
void k2shim_window_icon(SDL_Window* pWindow);

// Feeds one pumped SDL event to the DirectInput devices.
union SDL_Event;
void k2shim_dinput_event(const SDL_Event* e);

struct SDL_Window;
// A GL entry point by name (then its EXT/ARB/core spellings); NULL, logged once, when missing.
void* k2shim_gl_proc(const char* pszName);
// Windows: the frame is drawn into a framebuffer of the shim's and copied to the window at each swap.
// k2shim_gl_present copies and then swaps with pfnSwap; k2shim_glpresent_proc is the wrapper
// k2shim_gl_proc hands out for the names that see the window's framebuffer.
void  k2shim_gl_present(SDL_Window* pWindow, void (*pfnSwap)(SDL_Window*));
void* k2shim_glpresent_proc(const char* pszName, void* real);

// Engine paths use '\\' and are case-blind; host paths are neither. Converts in place into pszOut and
// returns it: backslashes become '/', a leading ".\\" or "./" is dropped, and each component is
// matched case-insensitively against what exists on disk. A missing tail is kept as written, so a file
// about to be created still gets a sensible name.
char* k2shim_host_path(const char* pszIn, char* pszOut, size_t nOut);

// In the user-data layer, with a user directory set, a relative path reads from it when the file is
// there and from the working directory -- the game's install -- when not, and is written only there
// (k2shim_host_path_write, whose bMakeParents creates the directories above it). Listings are the two
// merged. With no user directory, which is the default and always the case on Android and iOS, both
// are the working directory.
void  k2shim_set_user_dir(const char* pszAbsDir);
const char* k2shim_user_dir();   // "" when unset
char* k2shim_host_path_write(const char* pszIn, char* pszOut, size_t nOut, bool bMakeParents);
// A read-only directory seen under a relative folder, between the user directory and the working
// directory (KOTOR II's Steam Workshop items under dlc/mods_english); the first mounted is searched
// first.
void  k2shim_add_mount(const char* pszRelDir, const char* pszAbsDir);
// One relative folder moved elsewhere for reads and writes, ahead of every other layer (saves/ to the
// install's Steam Cloud folder).
void  k2shim_redirect(const char* pszRelDir, const char* pszAbsDir);
// Resolves a relative directory to the host paths of its copies that exist: the user directory's and
// the mounts' in `upper`, in search order, and the working directory's in `install` ("" when missing).
// k2shim_list_dir lists one directory's entries, straight from libc.
void  k2shim_layer_dirs(const char* pszDir, std::vector<std::string>& upper, std::string& install);
void  k2shim_list_dir(const char* pszHostDir, std::vector<std::string>& names);

// The OBB layer serves the expansion files, read-only, under the working directory. Each call
// answers only when the native file system did not, since the callers try that first.
struct stat;
bool  k2shim_obb_mount(const char* pszObbPath);                    // index one .obb; later mounts win
FILE* k2shim_obb_fopen(const char* pszPath, const char* pszMode);  // read modes only
bool  k2shim_obb_stat(const char* pszPath, struct stat* pSt);
bool  k2shim_obb_size(FILE* f, uint64_t* pSize);                   // an OBB stream's length
void  k2shim_obb_list(const char* pszDir, std::vector<std::string>& names);   // a directory's entries

// Answers the bytes of the file named pszName among those compiled into the binary from the shaders
// and the achievements art (case-blind; NUL-terminated past *pnSize), else NULL.
extern "C" const unsigned char* k2embedded_find(const char* pszName, size_t* pnSize);

// What Achievements::UnlockAchievement calls under K2_HOST_BUILD -- Steam, or the local record and its
// notice -- and the K2_INPUT / K2_CTL `achieve:TAG` action.
extern "C" void k2host_unlock_achievement(const char* pszTag);

#ifdef K2P_MOBILE_HOST
// Android and iOS: the engine's GL 1.x surface is gl4es on the GLES 2 context SDL creates.
void  k2android_gl_context_created();       // once, after the first context is current
void  k2android_gl_pre_swap();              // before every SDL_GL_SwapWindow
void  k2android_gl_post_swap();             // after it
void* k2android_gl_proc(const char* pszName);
#endif

// K2_GL_CHECK=1: the generated ASLgl forwarders call k2shim_gl_after(name) after every GL call and
// log the first error each entry point raises (skipping glBegin/glEnd interiors).
extern bool k2shim_gl_checking;
// K2_GL_TRACE=<frame>: true while that frame (counted in SwapBuffers calls) is being drawn.
extern bool k2shim_gl_tracing;
void k2shim_gl_after(const char* name);
// Before glDeleteBuffers, disables every client array and generic attribute still sourced from one
// of the buffers. GL rebinds such an array to buffer 0, so its offset turns into a client pointer,
// and NVIDIA's driver reads every enabled array, used by the draw or not.
void k2shim_gl_unbind_buffers(int n, const unsigned int* ids);
// The generated forwarders keep a client-array shadow, so an array still enabled on memory that
// operator delete frees is disabled before the next draw, and the Windows crash handler prints the
// last draws and the enabled arrays.
extern void* k2shim_glarr_caller;   // the engine caller of the draw about to be made (ASLgl draws)
void k2shim_glarr_enable(unsigned int cap, bool on);
void k2shim_glarr_client_active(unsigned int unit);
void k2shim_glarr_attrib_enable(unsigned int index, bool on);
void k2shim_glarr_attribs_off();   // a KotorProgram's generic arrays off
void k2shim_glarr_bind(unsigned int target, unsigned int buffer);
void k2shim_glarr_buffers_deleted(int n, const unsigned int* ids);
void k2shim_glarr_pointer(unsigned int cap, int size, unsigned int type, int stride, const void* p, void* caller);
void k2shim_glarr_attrib_pointer(unsigned int index, int size, unsigned int type, int stride, const void* p,
                                 void* caller);
void k2shim_glarr_push();
void k2shim_glarr_pop();
void k2shim_glarr_draw(const char* name, unsigned int mode, int count, void* caller);
bool k2shim_glarr_watching(const void* p);
void k2shim_glarr_freed(const void* p, size_t n);
void k2shim_glarr_dump(uintptr_t fault);

#endif
