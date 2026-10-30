// The Win32 message-queue layer of the Mac/Android port.

#ifndef MSG_MAC_H
#define MSG_MAC_H
#include "k2stdint.h"

#include <deque>
#include <future>
#include <mutex>

// The Win32 window handle. It must stay an incomplete type with this name, because the exported
// GetNext and Send take HWND__*, not void*.
struct HWND__;

#include "port/win32api.h"

// One queued message.
struct MSG_Mac_t {
    tagMSG msg;
    bool   m_bPosted;
};

// The port's Win32 message queue, holding a deque of pending window messages, a deque of
// outstanding Send() calls with their completion promises, and the per-message-class counters that
// back the queue-status flags.
class CWin32MSGQueue {
public:
    CWin32MSGQueue();
    ~CWin32MSGQueue();

    void Add(tagMSG const& msg, bool bPosted);
    unsigned int Send(HWND__* hwnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam,
                      bool bUnicode, bool bNoWait);
    void ProcessSendQ();
    unsigned int GetNext(tagMSG* pMsg, HWND__* hwnd, unsigned int nMin, unsigned int nMax,
                         bool bRemove);
    // Steps one of the per-class counters by one and updates the matching flag pair when the
    // counter reaches zero.
    void AdjustStatus(MSG_Mac_t const& sMsg, bool bRemove);

    // One outstanding Send() with its completion promise.
    struct SendMsg {
        void*              hwnd;
        unsigned int       message;
        unsigned long long wParam;
        int64_t            lParam;
        bool               m_bUnicode;
        std::promise<int64_t> m_promise;
    };

private:
    std::deque<MSG_Mac_t> m_queue;
    std::mutex            m_lock;

    unsigned int m_nMsgFlags;
    int          m_nMsgTime;
    int          m_nMsgPt;
    // A pointer-width reserved slot ahead of the counter block; only the constructor touches it.
    void*        m_pReserved1;
    int          m_nKeyCount;
    int          m_nMouseMoveCount;
    int          m_nMouseButtonCount;
    int          m_nPostMessageCount;
    int          m_nTimerCount;
    int          m_nPaintCount;
    int          m_nSendMessageCount;
    int          m_nHotKeyCount;
    int          m_nAllPostMessageCount;

    std::deque<SendMsg> m_sendQueue;
    std::mutex          m_sendLock;

    // Five reserved pointer-width words (0xf0 on 64-bit hosts) that no member function touches, nor
    // the constructor.
    void*               m_aReserved1[5];
    std::promise<int64_t> m_promise;
};

#if !KOTOR1  // K2's queue; K1's library has none, and its tagMSG is the 0x20-byte one
K2_LAYOUT_ASSERT(sizeof(tagMSG) == 0x30, "tagMSG is 0x30 on arm64");
K2_LAYOUT_ASSERT(sizeof(MSG_Mac_t) == 0x38, "MSG_Mac_t is 0x38 (the deque block size 0x49 pins it)");
#endif
K2_LAYOUT_ASSERT(sizeof(CWin32MSGQueue::SendMsg) == 0x30, "SendMsg is 0x30 (block size 0x55)");
#ifndef K2_HOST_BUILD   // the host's std::promise/std::mutex are not bionic's sizes
K2_LAYOUT_ASSERT(sizeof(CWin32MSGQueue) == 0x120, "CWin32MSGQueue is 0x120 (embedded in ThreadHANDLE)");
#endif

namespace MSG_Mac {
    void PurgeHWNDFromQueue(void* hwnd);
    void PurgeTimerFromQueue(void* hwnd, unsigned int nTimerId);
}

extern "C" uint64_t GetWindowLongW(void* hwnd, int nIndex) throw();
extern "C" long long CallWindowProcA(void* pfn, void* hwnd, unsigned int msg,
                                     unsigned long long wParam, long long lParam) throw();
extern "C" long long CallWindowProcW(void* pfn, void* hwnd, unsigned int msg,
                                     unsigned long long wParam, long long lParam) throw();

#endif // MSG_MAC_H
