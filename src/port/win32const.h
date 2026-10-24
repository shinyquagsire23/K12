#ifndef WIN32CONST_H
#define WIN32CONST_H

// The Win32 constants the engine passes to and gets back from the port's Win32 shim
// (port/win32api.h), as winuser.h, winbase.h and winerror.h name them. The engine never includes
// those headers. Values above INT_MAX sit in the `unsigned int` enums, so each name keeps the type
// its literal had.

// Window messages.
enum {
    WM_DESTROY = 0x0002,
    WM_SIZE = 0x0005,
    WM_ACTIVATE = 0x0006,
    WM_SETFOCUS = 0x0007,
    WM_KILLFOCUS = 0x0008,
    WM_PAINT = 0x000F,
    WM_CLOSE = 0x0010,
    WM_QUIT = 0x0012,
    WM_ACTIVATEAPP = 0x001C,
    WM_CANCELMODE = 0x001F,
    WM_SETCURSOR = 0x0020,
    WM_WINDOWPOSCHANGING = 0x0046,
    WM_WINDOWPOSCHANGED = 0x0047,
    WM_KEYDOWN = 0x0100,
    WM_KEYUP = 0x0101,
    WM_CHAR = 0x0102,
    WM_SYSKEYDOWN = 0x0104,
    WM_SYSKEYUP = 0x0105,
    WM_SYSCHAR = 0x0106,
    WM_SYSCOMMAND = 0x0112,
    WM_TIMER = 0x0113,
    WM_MOUSEMOVE = 0x0200,
    WM_LBUTTONDOWN = 0x0201,
    WM_LBUTTONUP = 0x0202,
    WM_LBUTTONDBLCLK = 0x0203,
    WM_RBUTTONDOWN = 0x0204,
    WM_RBUTTONUP = 0x0205,
    WM_RBUTTONDBLCLK = 0x0206,
    WM_MBUTTONDOWN = 0x0207,
    WM_MBUTTONUP = 0x0208,
    WM_MBUTTONDBLCLK = 0x0209,
    WM_MOUSEWHEEL = 0x020A,
    WM_XBUTTONDOWN = 0x020B,
    WM_XBUTTONUP = 0x020C,
    WM_XBUTTONDBLCLK = 0x020D,
    WM_HOTKEY = 0x0312,
    WM_USER = 0x0400,
};

// GetQueueStatus's flags: the low word says what the queue holds, the high word what arrived since
// the last call.
enum {
    QS_KEY = 0x0001,
    QS_MOUSEMOVE = 0x0002,
    QS_MOUSEBUTTON = 0x0004,
    QS_POSTMESSAGE = 0x0008,
    QS_TIMER = 0x0010,
    QS_HOTKEY = 0x0080,
    QS_SENDMESSAGE = 0x0040,
    QS_ALLPOSTMESSAGE = 0x0100,
};

// Virtual-key codes.
enum {
    VK_BACK = 0x08,
    VK_RETURN = 0x0D,
    VK_SHIFT = 0x10,
    VK_CONTROL = 0x11,
    VK_MENU = 0x12,
    VK_ESCAPE = 0x1B,
    VK_LEFT = 0x25,
    VK_UP = 0x26,
    VK_RIGHT = 0x27,
    VK_DOWN = 0x28,
    VK_F4 = 0x73,
    VK_F10 = 0x79,
};

// A key message's lParam: ALT was held (KF_ALTDOWN in the high word).
enum { KEYDATA_ALTDOWN = 0x20000000 };

// WM_SYSCOMMAND's commands; the low four bits of wParam are the system's own.
enum {
    SC_SIZE = 0xF000,
    SC_SCREENSAVE = 0xF140,
    SC_MONITORPOWER = 0xF170,
};
enum : unsigned int { SC_COMMAND_MASK = 0xfffffff0 };

// WM_SIZE's wParam and WM_SETCURSOR's hit-test code.
enum { SIZE_MAXIMIZED = 2, HTCLIENT = 1 };

// Window classes and styles.
enum {
    CS_VREDRAW = 0x0001,
    CS_HREDRAW = 0x0002,
    CS_DBLCLKS = 0x0008,
    WS_EX_TOPMOST = 0x00000008,
    WS_EX_APPWINDOW = 0x00040000,
    WS_MINIMIZEBOX = 0x00020000,
    WS_SYSMENU = 0x00080000,
    WS_CAPTION = 0x00C00000,
    WS_VISIBLE = 0x10000000,
    WS_CLIPCHILDREN = 0x02000000,
};
enum : unsigned int { WS_POPUP = 0x80000000 };

// ShowWindow, SetWindowPos, GetStockObject, LoadCursor, PatBlt.
enum {
    SW_SHOWMAXIMIZED = 3,
    SW_MINIMIZE = 6,
    SWP_NOSIZE = 0x0001,
    SWP_FRAMECHANGED = 0x0020,
    SWP_NOCOPYBITS = 0x0100,
    BLACK_BRUSH = 4,
    IDC_ARROW = 32512,
    BLACKNESS = 0x00000042,
};

// MessageBox, FormatMessage.
enum {
    MB_ICONHAND = 0x00000010,
    MB_ICONINFORMATION = 0x00000040,
    MB_SYSTEMMODAL = 0x00001000,
    FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100,
    FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200,
    FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000,
    LANG_USER_DEFAULT = 0x0400,   // MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
};

// Process priority classes.
enum {
    NORMAL_PRIORITY_CLASS = 0x00000020,
    HIGH_PRIORITY_CLASS = 0x00000080,
    REALTIME_PRIORITY_CLASS = 0x00000100,
};

// Waits, threads and file access.
enum {
    WAIT_OBJECT_0 = 0,
    WAIT_TIMEOUT = 0x00000102,
    STILL_ACTIVE = 0x00000103,
    FILE_READ_DATA = 0x0001,
    FILE_WRITE_DATA = 0x0002,
    FILE_READ_ATTRIBUTES = 0x0080,
    FILE_WRITE_ATTRIBUTES = 0x0100,
    GENERIC_WRITE = 0x40000000,
};
enum : unsigned int {
    INFINITE = 0xffffffff,
    WAIT_FAILED = 0xffffffff,
    GENERIC_READ = 0x80000000,
};

// GetLastError codes.
enum {
    ERROR_GEN_FAILURE = 31,
    ERROR_NOT_SUPPORTED = 50,
    ERROR_INVALID_PARAMETER = 87,
    ERROR_NOT_OWNER = 288,
    ERROR_DEVICE_NOT_CONNECTED = 1167,
};

#endif
