// The executable is a GUI program (no console window of its own when started from Explorer or a
// shortcut). Started from a console (cmd, PowerShell, a terminal), its stdout / stderr -- the k2: log
// lines, the crash report -- go to that console, unless they were already redirected (a file, a pipe,
// Wine's own terminal). Compiled into the executable itself.
//
// Whatever stdout / stderr reach, a copy also goes to a log file in the player's folder, k2.log (K1:
// k1.log; the previous run's is kept as k2.prev.log), so a run started by Steam or Explorer, with no
// console, still leaves its log and crash report. The lines before the folder is known are held and
// written when it is (k2host_log_open). K2_LOGFILE=0 turns the copy off (an ASan build: off unless K2_LOGFILE=1).
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

extern "C" void k2host_log_flush(void);

namespace {

bool HasHandle(DWORD nStd)
{
    HANDLE h = GetStdHandle(nStd);
    return h && h != INVALID_HANDLE_VALUE && GetFileType(h) != FILE_TYPE_UNKNOWN;
}

struct AttachParentConsole {
    AttachParentConsole()
    {
        if (HasHandle(STD_ERROR_HANDLE) || !AttachConsole(ATTACH_PARENT_PROCESS))
            return;
        freopen("CONOUT$", "w", stderr);
        if (!HasHandle(STD_OUTPUT_HANDLE) || GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR)
            freopen("CONOUT$", "w", stdout);
        setvbuf(stderr, 0, _IONBF, 0);
    }
} s_attach;

// stdout and stderr write into a pipe; a thread passes each chunk on to where stderr went before
// (a console, a file, a pipe, or nowhere) and to the log file. The read end is overlapped: on a
// synchronous handle PeekNamedPipe (k2host_log_flush) queues behind the thread's pending ReadFile,
// which nothing completes once the process stops writing, and the exit hangs there.
struct LogTee {
    HANDLE hRead = 0;
    HANDLE hOut = INVALID_HANDLE_VALUE;    // the original stderr
    HANDLE hFile = INVALID_HANDLE_VALUE;   // the log, once open
    std::string held;                      // the lines before it is
    CRITICAL_SECTION lock;
    volatile LONG nBusy = 0;               // a chunk is being passed on

    LogTee()
    {
        const char* e = getenv("K2_LOGFILE");
        if (e && *e == '0') return;
#ifdef __SANITIZE_ADDRESS__
        // AddressSanitizer's report writer fails on the pipe ("Internal error during continue on
        // error: Fail on write()") and the process dies with the report lost: stderr stays where it
        // was unless K2_LOGFILE=1 asks for the copy.
        if (!e || *e != '1') return;
#endif
        InitializeCriticalSection(&lock);
        char szName[64];
        snprintf(szName, sizeof szName, "\\\\.\\pipe\\k2log-%lu", GetCurrentProcessId());
        hRead = CreateNamedPipeA(szName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                 PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 1 << 16, 0, 0);
        if (hRead == INVALID_HANDLE_VALUE) { hRead = 0; return; }
        HANDLE hWrite = CreateFileA(szName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hWrite == INVALID_HANDLE_VALUE) { CloseHandle(hRead); hRead = 0; return; }
        if (HasHandle(STD_ERROR_HANDLE))
            DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE), GetCurrentProcess(), &hOut, 0,
                            FALSE, DUPLICATE_SAME_ACCESS);
        fflush(stdout);
        fflush(stderr);
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hWrite), _O_TEXT);
        if (fd < 0) return;
        // A GUI program's CRT may have no fd 1 / 2 at all; reopen them on something first.
        if (_fileno(stdout) < 0) freopen("NUL", "w", stdout);
        if (_fileno(stderr) < 0) freopen("NUL", "w", stderr);
        _dup2(fd, _fileno(stdout));
        _dup2(fd, _fileno(stderr));
        SetStdHandle(STD_OUTPUT_HANDLE, hWrite);
        SetStdHandle(STD_ERROR_HANDLE, hWrite);
        setvbuf(stdout, 0, _IONBF, 0);
        setvbuf(stderr, 0, _IONBF, 0);
        CloseHandle(CreateThread(0, 0, Pump, this, 0, 0));
        atexit(k2host_log_flush);
    }

    static DWORD WINAPI Pump(void* p)
    {
        LogTee* t = static_cast<LogTee*>(p);
        char buf[4096];
        DWORD n;
        OVERLAPPED ov = {};
        ov.hEvent = CreateEventA(0, TRUE, FALSE, 0);
        for (;;) {
            if (!ReadFile(t->hRead, buf, sizeof buf, 0, &ov) && GetLastError() != ERROR_IO_PENDING) break;
            if (!GetOverlappedResult(t->hRead, &ov, &n, TRUE) || !n) break;
            InterlockedExchange(&t->nBusy, 1);
            DWORD w;
            if (t->hOut != INVALID_HANDLE_VALUE) WriteFile(t->hOut, buf, n, &w, 0);
            EnterCriticalSection(&t->lock);
            if (t->hFile != INVALID_HANDLE_VALUE)
                WriteFile(t->hFile, buf, n, &w, 0);
            else if (t->held.size() < (1u << 20))
                t->held.append(buf, n);
            LeaveCriticalSection(&t->lock);
            InterlockedExchange(&t->nBusy, 0);
        }
        CloseHandle(ov.hEvent);
        return 0;
    }
} s_tee;

} // namespace

// Opens the log in the player's folder, once that is known. The log goes there from now on,
// starting with the lines held.
extern "C" void k2host_log_open(const char* pszDir, const char* pszName)
{
    if (!s_tee.hRead) return;
    std::string path = std::string(pszDir) + "\\" + pszName + ".log";
    std::string prev = std::string(pszDir) + "\\" + pszName + ".prev.log";
    MoveFileExA(path.c_str(), prev.c_str(), MOVEFILE_REPLACE_EXISTING);
    HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "k2: log: cannot write %s (error %lu)\n", path.c_str(), GetLastError());
        return;
    }
    EnterCriticalSection(&s_tee.lock);
    DWORD w;
    WriteFile(h, s_tee.held.data(), static_cast<DWORD>(s_tee.held.size()), &w, 0);
    s_tee.held.clear();
    s_tee.held.shrink_to_fit();
    s_tee.hFile = h;
    LeaveCriticalSection(&s_tee.lock);
    fprintf(stderr, "k2: log: %s\n", path.c_str());
}

// Called when the process is about to end (a crash report was written). Waits, briefly, for the
// pipe to drain into the log, and puts the log on disk.
extern "C" void k2host_log_flush(void)
{
    if (!s_tee.hRead) return;
    fflush(stdout);
    fflush(stderr);
    for (int i = 0; i < 100; ++i) {
        DWORD nAvail = 0;
        if (!PeekNamedPipe(s_tee.hRead, 0, 0, 0, &nAvail, 0) || (nAvail == 0 && !s_tee.nBusy)) break;
        Sleep(10);
    }
    if (s_tee.hFile != INVALID_HANDLE_VALUE) FlushFileBuffers(s_tee.hFile);
}

// Ends the program. The atexit handlers and static destructors run, then the process ends without
// the DLL detach pass (a driver's or Steam's library waiting there on its own threads leaves a
// process with no window running on). A stall in the handlers ends it after a few seconds as well.
extern "C" void k2host_exit(int nCode)
{
    fprintf(stderr, "k2: exiting (%d)\n", nCode);
    static int s_nCode;
    s_nCode = nCode;
    CloseHandle(CreateThread(0, 0, [](void*) -> DWORD {
        Sleep(5000);
        fprintf(stderr, "k2: exit stalled; ending the process\n");
        Sleep(100);
        if (s_tee.hFile != INVALID_HANDLE_VALUE) FlushFileBuffers(s_tee.hFile);
        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(s_nCode));
        return 0;
    }, 0, 0, 0));
    _cexit();
    k2host_log_flush();
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(nCode));
}
