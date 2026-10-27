#ifndef WIN32API_H
#define WIN32API_H
#include "k2stdint.h"

// The Win32 functions the port layer answers (platforms/shim), declared once for every engine TU and
// for the shim that defines them.
//
// KOTOR II calls them by their C names. On Windows those names belong to kernel32 (and _endthreadex
// to the CRT), which libc++, winpthread and the CRT import for themselves, so there the engine's are
// in namespace k2win32: two functions, two names.
//
// KOTOR 1 declares its own with C++ linkage and its own parameter types.
#if defined(_WIN32)
#define K2_WIN32_API_BEGIN namespace k2win32 {
#define K2_WIN32_API_END } using namespace k2win32;
#else
#define K2_WIN32_API_BEGIN extern "C" {
#define K2_WIN32_API_END }
#endif

struct _FILETIME;   // Win32Find.h
struct _WIN32_FIND_DATAA;
struct _SYSTEM_INFO;
struct _STARTUPINFOA;   // exobaseinternal.cpp
struct _PROCESS_INFORMATION;
class FileHANDLE;       // MacFiles.h

struct _SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
};
typedef _SYSTEMTIME SYSTEMTIME;

#if KOTOR1
// K1 keeps Win32's 32-bit layout: eight DWORDs.
struct _MEMORYSTATUS {
    unsigned int dwLength;
    unsigned int dwMemoryLoad;
    unsigned int dwTotalPhys;
    unsigned int dwAvailPhys;
    unsigned int dwTotalPageFile;
    unsigned int dwAvailPageFile;
    unsigned int dwTotalVirtual;
    unsigned int dwAvailVirtual;
};
#else
// The size fields are 8 bytes, as on Win64.
struct _MEMORYSTATUS {
    unsigned int dwLength;
    unsigned int dwMemoryLoad;
    uint64_t dwTotalPhys;
    uint64_t dwAvailPhys;
    uint64_t dwTotalPageFile;
    uint64_t dwAvailPageFile;
    uint64_t dwTotalVirtual;
    uint64_t dwAvailVirtual;
};
#endif
typedef _MEMORYSTATUS MEMORYSTATUS;

// user32's records, laid out as the shim's window layer fills them (shim_window.cpp).
struct HWND__;
struct HDC__;
struct tagPOINT { int x; int y; };   // two 32-bit fields, not longs
typedef tagPOINT POINT;
struct tagRECT { int left; int top; int right; int bottom; };
typedef tagRECT RECT;
struct tagMSG {
    HWND__*      hwnd;
    unsigned int message;
#if KOTOR1  // K1's shim types the two message parameters as 32-bit
    unsigned int wParam;
    unsigned int lParam;
#else
    uint64_t     wParam;
    int64_t      lParam;
#endif
    unsigned int time;
    int          pt_x;
    int          pt_y;
};
typedef tagMSG MSG;
typedef int64_t (*WNDPROC)(HWND__*, unsigned int, unsigned long long, int64_t);
struct tagWNDCLASSA {
    unsigned int style;
    WNDPROC      lpfnWndProc;
    int          cbClsExtra;
    int          cbWndExtra;
    void*        hInstance;
    void*        hIcon;
    void*        hCursor;
    void*        hbrBackground;
    char*        lpszMenuName;
    char*        lpszClassName;
};
typedef tagWNDCLASSA WNDCLASSA;
struct tagWINDOWPOS {
    void*        hwnd;
    void*        hwndInsertAfter;
    RECT         rc;               // Win32's x/y/cx/cy run, copied as one 16-byte block
    unsigned int flags;
};
typedef tagWINDOWPOS WINDOWPOS;
// The pixel-format descriptor, in the field order the pixel-format calls read back.
struct tagPIXELFORMATDESCRIPTOR {
    unsigned short nSize;
    unsigned short nVersion;
    unsigned int   dwFlags;
    unsigned char  iPixelType;
    unsigned char  cColorBits;
    unsigned char  cRedBits;
    unsigned char  cRedShift;
    unsigned char  cGreenBits;
    unsigned char  cGreenShift;
    unsigned char  cBlueBits;
    unsigned char  cBlueShift;
    unsigned char  cAlphaBits;
    unsigned char  cAlphaShift;
    unsigned char  cAccumBits;
    unsigned char  cAccumRedBits;
    unsigned char  cAccumGreenBits;
    unsigned char  cAccumBlueBits;
    unsigned char  cAccumAlphaBits;
    unsigned char  cDepthBits;
    unsigned char  cStencilBits;
    unsigned char  cAuxBuffers;
    unsigned char  iLayerType;
    unsigned char  bReserved;
    unsigned int   dwLayerMask;
    unsigned int   dwVisibleMask;
    unsigned int   dwDamageMask;
};
typedef tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR;
// GetWindowInfo's record: cbSize, then rcWindow, rcClient, dwStyle, dwExStyle, dwWindowStatus,
// cxWindowBorders, cyWindowBorders, atomWindowType, wCreatorVersion. The engine writes only cbSize.
#if !KOTOR1
struct PortDevMode;   // ASL2/Win/PortDevMode.h
#endif
struct WINDOWINFO {
    unsigned int cbSize;
    RECT         rcWindow;
    RECT         rcClient;
    char         _unused[0x3c - 0x24];
};

// Win64's sizes (no `long` in them, so LP64 and LLP64 agree): the shim fills these.
static_assert(sizeof(_SYSTEMTIME) == 16 && sizeof(POINT) == 8 && sizeof(RECT) == 16, "Win32's records");
static_assert(sizeof(WNDCLASSA) == 72 && sizeof(WINDOWPOS) == 40 && sizeof(WINDOWINFO) == 60,
              "Win64's window records");
static_assert(sizeof(PIXELFORMATDESCRIPTOR) == 40, "Win32's PIXELFORMATDESCRIPTOR");
#if KOTOR1
static_assert(sizeof(_MEMORYSTATUS) == 32 && sizeof(MSG) == 0x20, "K1's 32-bit MEMORYSTATUS and MSG");
#else
static_assert(sizeof(_MEMORYSTATUS) == 56, "Win64's MEMORYSTATUS");
#endif

#if KOTOR1
struct _devicemodeA;

unsigned int GetTickCount();
void Sleep(int nMilliseconds);
void GetSystemTimeAsFileTime(_FILETIME* pTime);
int FileTimeToSystemTime(const _FILETIME* pFileTime, _SYSTEMTIME* pSystemTime);
int FileTimeToLocalFileTime(const _FILETIME* pFileTime, _FILETIME* pLocal);
void GlobalMemoryStatus(_MEMORYSTATUS* pBuffer);

void* CreateThread(void* pSecAttr, unsigned int nStackSize, int (*pStart)(void*), void* pParam,
                   unsigned int nFlags, unsigned int* pThreadId);
unsigned int ResumeThread(void* hThread);
unsigned int SuspendThread(void* hThread);
int SetThreadPriority(void* hThread, int nPriority);
unsigned int WaitForSingleObject(void* hHandle, unsigned int nMilliseconds);
int CloseHandle(void* hHandle);
void ExitThread(unsigned int nExitCode);
void* GetCurrentProcess();
int SetPriorityClass(void* hProcess, unsigned int dwPriorityClass);

void* FindFirstFileA(const char* pPattern, _WIN32_FIND_DATAA* pData);
int FindNextFileA(void* hFind, _WIN32_FIND_DATAA* pData);
int FindClose(void* hFind);
unsigned int GetLastError();
int DeleteFileA(const char* pPath);
int CreateDirectory(const char* pPath, void* pSecAttr);
int RemoveDirectory(const char* pPath);
unsigned int GetDriveTypeA(const char* pRootPathName);
int CreateProcessA(const char* pApplicationName, char* pCommandLine, void* pProcessAttributes,
                   void* pThreadAttributes, int bInheritHandles, unsigned int dwCreationFlags,
                   void* pEnvironment, const char* pCurrentDirectory, _STARTUPINFOA* pStartupInfo,
                   _PROCESS_INFORMATION* pProcessInformation);
int IsDBCSLeadByte(unsigned char c);

int EnumDisplaySettingsA(const char* pDevice, unsigned int nMode, _devicemodeA* pDevMode);
int ChangeDisplaySettingsA(_devicemodeA* pDevMode, unsigned int nFlags);
int DestroyWindow(void* hWnd);
void* LoadCursorA(void* hInstance, const char* pName);
int DestroyCursor(void* hCursor);
void* SetCursor(void* hCursor);
int ShowCursor(int bShow);
int SetCursorPos(int x, int y);
struct tagCURSORINFO {
    unsigned int cbSize;
    unsigned int flags;  // bit 0 is CURSOR_SHOWING
    void*        hCursor;
    int          ptScreenPos[2];
};
static_assert(sizeof(tagCURSORINFO) == 24, "Win64's CURSORINFO");
int GetCursorInfo(tagCURSORINFO* pInfo);
// KOTOR 1's library calls the window functions with C++ linkage and 32-bit message parameters;
// platforms/k1/k1port.cpp forwards them to the shim's (k2shim:: below), the one implementation both
// games run.
int DefWindowProcA(void* hWnd, unsigned int nMsg, unsigned int wParam, int lParam);
int PostMessageA(void* hWnd, unsigned int nMsg, unsigned int wParam, int lParam);
void PostQuitMessage(int nExitCode);
HDC__* GetDC(void* hWnd);
void* SetCapture(void* hWnd);
int ClientToScreen(void* hWnd, tagPOINT* pPoint);

// The tree's call shapes, forwarded to K1's signatures.
inline void GetSystemTimeAsFileTime(unsigned long long* pTime)
{
    GetSystemTimeAsFileTime(reinterpret_cast<_FILETIME*>(pTime));
}
inline void* LoadCursorA(void* hInstance, uint64_t nCursor)
{
    return LoadCursorA(hInstance, reinterpret_cast<const char*>(nCursor));
}
inline void* SetCursor_Win32(void* hCursor)
{
    return SetCursor(hCursor);
}
inline int ShowCursor_Win32(int bShow)
{
    return ShowCursor(bShow);
}
inline void* GetCurrentProcess_Win32()
{
    return GetCurrentProcess();
}
// K1's thread procs return a 64-bit DWORD and CreateThread takes a 32-bit one: the routine is passed
// as is.
inline void* CreateThread(void* pSecAttr, uint64_t nStackSize, uint64_t (*pStart)(void*),
                          void* pParam, uint64_t nFlags, unsigned int* pThreadId)
{
    return CreateThread(pSecAttr, static_cast<unsigned int>(nStackSize), reinterpret_cast<int (*)(void*)>(reinterpret_cast<void (*)()>(pStart)), pParam,
                        static_cast<unsigned int>(nFlags), pThreadId);
}
#else
K2_WIN32_API_BEGIN
unsigned int GetTickCount();
void Sleep(unsigned int dwMilliseconds);
void GetSystemTimeAsFileTime(unsigned long long* pFileTime);
int FileTimeToSystemTime(const _FILETIME* pFileTime, _SYSTEMTIME* pSystemTime);

void* CreateThread(void* pSecAttr, uint64_t nStackSize, uint64_t (*pStartAddr)(void*), void* pParam,
                   uint64_t dwFlags, unsigned int* pThreadId);
uint64_t ResumeThread(void* hThread);
uint64_t SuspendThread(void* hThread);
int SetThreadPriority(void* hThread, int nPriority);
unsigned int WaitForSingleObject(void* hHandle, unsigned int dwMilliseconds);
int CloseHandle(void* hObject);

void* FindFirstFileA(const char* pPattern, _WIN32_FIND_DATAA* pData);
int FindNextFileA(void* hFind, _WIN32_FIND_DATAA* pData);
int FindClose(void* hFind);
unsigned int GetLastError();
int DeleteFileA(const char* pPath);
unsigned int GetDriveTypeA(const char* pRootPathName) throw();
int CreateProcessA(char* pApplicationName, char* pCommandLine, void* pProcessAttributes,
                   void* pThreadAttributes, int bInheritHandles, unsigned int dwCreationFlags,
                   void* pEnvironment, char* pCurrentDirectory, _STARTUPINFOA* pStartupInfo,
                   _PROCESS_INFORMATION* pProcessInformation);
int IsDBCSLeadByte(unsigned char c) throw();

int FileTimeToLocalFileTime(const _FILETIME* pFileTime, _FILETIME* pLocal);
void GlobalMemoryStatus(_MEMORYSTATUS* pBuffer);

void ExitThread(unsigned int nExitCode) __attribute__((noreturn));
void* GetCurrentProcess_Win32();
int SetPriorityClass(void* hProcess, unsigned int dwPriorityClass);
K2_WIN32_API_END
#endif

K2_WIN32_API_BEGIN
void SetLastError(unsigned int dwErrCode) throw();

void* CreateSemaphoreA(void* pSec, int nInitial, int nMax, void* pName) throw();
void InitializeCriticalSection(void* pSection);
void DeleteCriticalSection(void* pSection);
void EnterCriticalSection(void* pSection);
void LeaveCriticalSection(void* pSection);
// Never returns; a thread procedure ends in this call, with no epilogue after it.
void _endthreadex(unsigned int nReturnCode) throw() __attribute__((noreturn));
void _endthread() throw();
unsigned int SleepEx(unsigned int dwMilliseconds, int bAlertable) throw();
// The SList pair, no-op stubs in the port layer.
int InitializeSListHead(void* pListHead) throw();
void* InterlockedPopEntrySList(void* pListHead) throw();

int QueryPerformanceFrequency(long long* pFrequency);
int QueryPerformanceCounter(long long* pCount);
void GetSystemInfo(_SYSTEM_INFO* pInfo);
unsigned int timeGetTime();
void GetSystemTime(_SYSTEMTIME* pSystemTime);
void GetLocalTime(_SYSTEMTIME* pSystemTime);
int SystemTimeToFileTime(const _SYSTEMTIME* pSystemTime, unsigned long long* pFileTime);
int CompareFileTime(const _FILETIME* pFileTime1, const _FILETIME* pFileTime2);

int GetDiskFreeSpaceExA(const char* pPath, unsigned long long* pFreeBytesAvailable,
                        unsigned long long* pTotalBytes, unsigned long long* pTotalFreeBytes);
int CreateDirectoryA(const char* pPath, void* pSecAttr);
int RemoveDirectoryA(const char* pPath);
uint64_t GetCurrentDirectoryA(uint64_t nSize, char* pszBuf);
int SetCurrentDirectoryA(const char* pszPath) throw();
// The second parameter is Win32's optional high-dword out-pointer.
unsigned int GetFileSize(FileHANDLE* pFile, unsigned int* pnSizeHigh) throw();
int IsBadWritePtr(const void* p, int nBytes);

void* GetModuleHandleA(const char* pszModule);
unsigned int FormatMessageA(unsigned int dwFlags, const void* pSource, unsigned int dwMessageId,
                            unsigned int dwLanguageId, char** ppBuffer, unsigned int nSize, void* pArguments);
void* LocalFree(void* hMem);
K2_WIN32_API_END

// user32, gdi32 and opengl32: the window, message, cursor and context calls, which the shim answers
// with C linkage on every target (platforms/shim/shim_window.cpp).
#if !KOTOR1
// The message pump fills KOTOR II's MSG. KOTOR 1's is half that size and it has no Win32 message
// loop (SDL's pump in MacMain.cpp), so KOTOR 1 never reaches these.
static_assert(sizeof(MSG) == 0x30, "the shim's MSG");
extern "C" {
int PeekMessageA(MSG* pMsg, HWND__* hWnd, unsigned int wMsgFilterMin, unsigned int wMsgFilterMax,
                 unsigned int wRemoveMsg);
int GetMessageA(MSG* pMsg, HWND__* hWnd, unsigned int wMsgFilterMin, unsigned int wMsgFilterMax);
int TranslateMessage(const MSG* pMsg);
int64_t DispatchMessageA(const MSG* pMsg);
}
#endif

// The ones KOTOR 1 declares under its own C++ signatures (above), which k1port.cpp forwards to
// these: KOTOR II calls them by their C names.
namespace k2shim {
extern "C" {
int64_t DefWindowProcA(HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam);
int PostMessageA(HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam);
void PostQuitMessage(int nExitCode);
int DestroyWindow(HWND__* hWnd);
HDC__* GetDC(HWND__* hWnd);
HWND__* SetCapture(HWND__* hWnd);
int ClientToScreen(HWND__* hWnd, POINT* pPoint);
int SetCursorPos(int x, int y);
void* LoadCursorA(void* hInstance, uint64_t nCursor);   // MAKEINTRESOURCE's integer
int DestroyCursor(void* hCursor);
int ShowCursor_Win32(int bShow);
void* SetCursor_Win32(void* hCursor);
#if !KOTOR1
// This is the engine's own enumerator (ASL2/Win/PortDevMode.h has the record), not Win32's: the
// mode index is the second argument and the device string the first.
int EnumDisplaySettingsA(const char* pDevice, unsigned int iModeNum, PortDevMode* pDevMode);
int ChangeDisplaySettingsA(PortDevMode* pDevMode, unsigned int nFlags);
#endif
}
}
#if !KOTOR1
using namespace k2shim;
#endif

extern "C" {
int64_t SendMessageA(HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam);
void* LoadIconA(void* hInstance, uint64_t nIcon);   // the name is MAKEINTRESOURCE's integer
unsigned short RegisterClassA(const WNDCLASSA* pWndClass);
HWND__* CreateWindowExA(unsigned int dwExStyle, const char* pszClassName, const char* pszWindowName,
                        unsigned int dwStyle, int x, int y, int nWidth, int nHeight, HWND__* hWndParent,
                        void* hMenu, void* hInstance, void* pParam);
int ShowWindow_Win32(HWND__* hWnd, int nCmdShow);
int SetWindowPos(HWND__* hWnd, HWND__* hWndInsertAfter, int x, int y, int cx, int cy, unsigned int uFlags);
int SetWindowTextA(HWND__* hWnd, const char* pszString);
int AdjustWindowRect(RECT* pRect, unsigned int dwStyle, int bMenu);
int GetClientRect(HWND__* hWnd, RECT* pRect);
int GetWindowInfo(HWND__* hWnd, WINDOWINFO* pInfo);
int64_t GetWindowLongA(HWND__* hWnd, int nIndex) throw();
int SetWindowLongA(HWND__* hWnd, int nIndex, unsigned int dwNewLong);
int MessageBoxA(HWND__* hWnd, const char* pszText, const char* pszCaption, unsigned int uType);
void* BeginPaint(HWND__* hWnd, void* pPaint);
int EndPaint(HWND__* hWnd, const void* pPaint);
void* GetStockObject(int nObject);
int PatBlt(void* hDC, int x, int y, int nWidth, int nHeight, unsigned int dwRop);

int GetCursorPos(POINT* pPoint);
int ScreenToClient(HWND__* hWnd, POINT* pPoint);
int PtInRect_Win32(const RECT* pRect, POINT pt);   // the point goes by value
int ReleaseCapture();
short GetAsyncKeyState(int nVirtKey) throw();
int GetKeyState(int nVirtKey);   // the engine tests bit 15 of an int

int ReleaseDC(HWND__* hWnd, void* hDC);
int ChoosePixelFormat(void* hDC, const PIXELFORMATDESCRIPTOR* pPFD);
int SetPixelFormat(void* hDC, int iPixelFormat, const PIXELFORMATDESCRIPTOR* pPFD);
int DescribePixelFormat(void* hDC, int iPixelFormat, unsigned int nBytes, PIXELFORMATDESCRIPTOR* pPFD);
int SwapBuffers(void* hDC);

void* wglCreateContext(void* hDC);
int wglDeleteContext(void* hGLRC);
int wglMakeCurrent(void* hDC, void* hGLRC);
void* wglGetCurrentContext();
void* wglGetCurrentDC();
int wglShareLists(void* hGLRC1, void* hGLRC2);
int wglSwapIntervalEXT(int nInterval);
int wglChoosePixelFormatARB(void* hDC, const int* pAttribIList, const float* pAttribFList, unsigned int nMaxFormats,
                            int* pFormats, unsigned int* pnNumFormats);
int wglMakeContextCurrentARB(void* hDrawDC, void* hReadDC, void* hGLRC);
void* wglCreatePBufferARB(void* hDC, int iPixelFormat, int iWidth, int iHeight, const int* pAttribList);
void* wglGetPBufferDCARB(void* hPBuffer);
int wglReleasePBufferDCARB(void* hPBuffer, void* hDC);
int wglDestroyPBufferARB(void* hPBuffer);
int wglBindTexImageARB(void* hPBuffer, int iBuffer);
}

#endif // WIN32API_H
