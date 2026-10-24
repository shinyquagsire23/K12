// The KOTOR II shim's entry points (platforms/shim), re-exported under k2c_ names for k1port.cpp.
// KOTOR 1 declares the same Win32 and CRT names with C++ linkage, and one translation unit cannot
// declare a name both ways, so k1port.cpp defines K1's C++ functions and reaches the shim's through
// here. The thread and CRT functions are the shim's own declarations (port/win32api.h and
// port/crtapi.h, the KOTOR II view); the CRT three are the UCRT's on Windows.
#include <stdint.h>

#include "port/win32api.h"
#include "port/crtapi.h"

extern "C" {
int gluBuild2DMipmaps(unsigned int eTarget, int nComponents, int nWidth, int nHeight, unsigned int eFormat,
                      unsigned int eType, const void* pData);

void k2c_GetSystemTimeAsFileTime(void* pFt) { GetSystemTimeAsFileTime(static_cast<unsigned long long*>(pFt)); }
void* k2c_CreateThread(void* pSecAttr, uint64_t nStackSize, uint64_t (*pfnStart)(void*), void* pParam,
                       uint64_t dwFlags, unsigned int* pThreadId)
{
    return CreateThread(pSecAttr, nStackSize, pfnStart, pParam, dwFlags, pThreadId);
}
uint64_t k2c_ResumeThread(void* hThread) { return ResumeThread(hThread); }
uint64_t k2c_SuspendThread(void* hThread) { return SuspendThread(hThread); }
uint64_t k2c_WaitForSingleObject(void* hHandle, unsigned int nMs) { return WaitForSingleObject(hHandle, nMs); }
int k2c_CloseHandle(void* hObject) { return CloseHandle(hObject); }
char* k2c_strlwr(char* s) { return _strlwr(s); }
char* k2c_strupr(char* s) { return _strupr(s); }
char* k2c_itoa(int value, char* buf, int radix) { return _itoa(value, buf, radix); }
int k2c_gluBuild2DMipmaps(unsigned int eTarget, int nComponents, int nWidth, int nHeight, unsigned int eFormat,
                          unsigned int eType, const void* pData)
{
    return gluBuild2DMipmaps(eTarget, nComponents, nWidth, nHeight, eFormat, eType, pData);
}
} // extern "C"
