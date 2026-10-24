// The POSIX dynamic-loader calls the shim makes, on Windows (windows/win_dlfcn.cpp).
//
//   dlopen / dlsym      LoadLibrary / GetProcAddress; RTLD_DEFAULT searches the executable and
//                       the libraries it loaded.
//   dlsym(RTLD_NEXT)    the CRT function behind one of the shim's overrides of the same name
//                       (fopen, stat, opendir ...: shim_stdio.cpp, shim_path.cpp), which on
//                       Windows is a table of the CRT's own entry points, not a search.
//   dladdr              the module an address is in; no symbol names.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 0
#define RTLD_DEFAULT ((void*)0)
#define RTLD_NEXT ((void*)-1)
typedef struct {
    const char* dli_fname;
    void* dli_fbase;
    const char* dli_sname;
    void* dli_saddr;
} Dl_info;
void* dlopen(const char* pszFile, int nMode);
void* dlsym(void* pHandle, const char* pszName);
int dlclose(void* pHandle);
const char* dlerror(void);
int dladdr(const void* pAddr, Dl_info* pInfo);
#ifdef __cplusplus
}
#endif
