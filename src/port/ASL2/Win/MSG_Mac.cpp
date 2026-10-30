#include "k2stdint.h"
#include "MSG_Mac.h"

#include "port/win32const.h"
#include <atomic>
#include <chrono>

// The thread object's message queue, defined here so this file need not pull in WinThreadsMP.h.
namespace ASL {
    struct ASLThread {
        unsigned char _opaque[0xa0];
        CWin32MSGQueue m_msgQueue;
    };
    ASLThread* GetCurrentThreadHANDLE() throw();

    namespace Internal {
        std::atomic<unsigned char> mWinMainRunning;
    }
}

// m_nMsgFlags holds each GetQueueStatus bit twice: in the low word while the queue holds such a
// message, and in the high word when one arrived since the last GetNext.
enum : unsigned int {
    QS_NEW_WORD = 0xffff0000,
    QS_KEY_PAIR = QS_KEY << 16 | QS_KEY,
    QS_MOUSEMOVE_PAIR = QS_MOUSEMOVE << 16 | QS_MOUSEMOVE,
    QS_MOUSEBUTTON_PAIR = QS_MOUSEBUTTON << 16 | QS_MOUSEBUTTON,
    QS_TIMER_PAIR = QS_TIMER << 16 | QS_TIMER,
    QS_HOTKEY_PAIR = QS_HOTKEY << 16 | QS_HOTKEY,
    QS_POSTED_PAIR = (QS_POSTMESSAGE | QS_ALLPOSTMESSAGE) << 16 | QS_POSTMESSAGE | QS_ALLPOSTMESSAGE,
};

// Everything in the constructor is member construction and zero-initialisation. The reserved
// block at 0xf0 is deliberately left alone.
CWin32MSGQueue::CWin32MSGQueue()
{
    m_nMsgFlags = 0;
    m_nMsgTime = 0;
    m_nMsgPt = 0;
    m_pReserved1 = 0;
    m_nKeyCount = 0;
    m_nMouseMoveCount = 0;
    m_nMouseButtonCount = 0;
    m_nPostMessageCount = 0;
    m_nTimerCount = 0;
    m_nPaintCount = 0;
    m_nSendMessageCount = 0;
    m_nAllPostMessageCount = 0;
}

// The body is nothing but the implicit member teardown, in reverse declaration order.
CWin32MSGQueue::~CWin32MSGQueue()
{
}

// Appends to the message deque and updates the class counter and flag bit.
void CWin32MSGQueue::Add(tagMSG const& msg, bool bPosted)
{
    MSG_Mac_t sMsg;
    sMsg.msg = msg;
    sMsg.m_bPosted = bPosted;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        AdjustStatus(sMsg, false);
        m_queue.push_back(sMsg);
    }
}

// Makes up to four passes over the deque, one per message class in the requested range.
unsigned int CWin32MSGQueue::GetNext(tagMSG* pMsg, HWND__* hwnd, unsigned int nMin,
                                     unsigned int nMax, bool bRemove)
{
    K2_UNUSED(hwnd);

    ProcessSendQ();

    if (m_queue.empty())
        return 0;

    unsigned int nMask = QS_NEW_WORD & ~(QS_POSTED_PAIR & QS_NEW_WORD);
    unsigned int nFilterMax = 0xffff;
    if (nMin != 0 || nMax != 0)
    {
        nMask = QS_NEW_WORD & ~(QS_POSTMESSAGE << 16);
        nFilterMax = nMax;
    }
    if (nMin == 0)
        nMin = 1;

    m_nMsgFlags &= nMask;

    unsigned int nResult;
    std::deque<MSG_Mac_t>::iterator it;

    m_lock.lock();

    for (it = m_queue.begin(); it != m_queue.end(); ++it)
    {
        if (it->msg.message <= nFilterMax && it->msg.message >= nMin && it->m_bPosted)
            goto Found;
    }
    for (it = m_queue.begin(); it != m_queue.end(); ++it)
    {
        if (it->msg.message >= nMin && it->msg.message <= nFilterMax &&
            it->msg.message != WM_QUIT && it->msg.message != WM_TIMER)
            goto Found;
    }
    for (it = m_queue.begin(); it != m_queue.end(); ++it)
    {
        if (nFilterMax < WM_TIMER)
            continue;
        if (nMin > WM_TIMER)
            continue;
        if (it->msg.message == WM_TIMER)
            goto Found;
    }
    for (it = m_queue.begin(); it != m_queue.end(); ++it)
    {
        if (it->msg.message == WM_QUIT)
        {
            *pMsg = it->msg;
            if (bRemove)
            {
                AdjustStatus(*it, true);
                m_queue.erase(it);
            }
            goto Publish;
        }
    }

    m_lock.unlock();
    nResult = 0;
    goto Done;

Found:
    *pMsg = it->msg;
    if (bRemove)
    {
        AdjustStatus(*it, true);
        m_queue.erase(it);
    }

Publish:
    nResult = 1;
    m_lock.unlock();

    m_nMsgTime = pMsg->time;
    m_nMsgPt = static_cast<int>(static_cast<unsigned short>(pMsg->pt_x) | (static_cast<unsigned int>(pMsg->pt_y) << 16));

Done:
    ProcessSendQ();
    return nResult;
}

// Counts each message class in one counter and its own bit pair in the non-empty flag word.
void CWin32MSGQueue::AdjustStatus(MSG_Mac_t const& sMsg, bool bRemove)
{
    int nDelta = bRemove ? -1 : 1;

    if (sMsg.m_bPosted)
    {
        m_nPostMessageCount += nDelta;
        if (bRemove)
        {
            if (m_nPostMessageCount == 0)
                m_nMsgFlags &= ~QS_POSTED_PAIR;
        }
        else
            m_nMsgFlags |= QS_POSTED_PAIR;
    }

    switch (sMsg.msg.message)
    {
    case WM_MOUSEMOVE:
        m_nMouseMoveCount += nDelta;
        if (bRemove)
        {
            if (m_nMouseMoveCount == 0)
                m_nMsgFlags &= ~QS_MOUSEMOVE_PAIR;
        }
        else
            m_nMsgFlags |= QS_MOUSEMOVE_PAIR;
        break;
    case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
        m_nMouseButtonCount += nDelta;
        if (bRemove)
        {
            if (m_nMouseButtonCount == 0)
                m_nMsgFlags &= ~QS_MOUSEBUTTON_PAIR;
        }
        else
            m_nMsgFlags |= QS_MOUSEBUTTON_PAIR;
        break;
    case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
        m_nKeyCount += nDelta;
        if (bRemove)
        {
            if (m_nKeyCount == 0)
                m_nMsgFlags &= ~QS_KEY_PAIR;
        }
        else
            m_nMsgFlags |= QS_KEY_PAIR;
        break;
    case WM_TIMER:
        m_nTimerCount += nDelta;
        if (bRemove)
        {
            if (m_nTimerCount == 0)
                m_nMsgFlags &= ~QS_TIMER_PAIR;
        }
        else
            m_nMsgFlags |= QS_TIMER_PAIR;
        break;
    case WM_HOTKEY:
        m_nHotKeyCount += nDelta;
        if (bRemove)
        {
            if (m_nHotKeyCount == 0)
                m_nMsgFlags &= ~QS_HOTKEY_PAIR;
        }
        else
            m_nMsgFlags |= QS_HOTKEY_PAIR;
        break;
    }
}

// Posts one message with a completion channel and, unless the request is fire-and-forget, pumps
// the queue until the promise is satisfied.
unsigned int CWin32MSGQueue::Send(HWND__* hwnd, unsigned int nMsg, unsigned long long wParam,
                                  int64_t lParam, bool bUnicode, bool bNoWait)
{
    SendMsg sMsg;
    std::future<int64_t> future = sMsg.m_promise.get_future();
    sMsg.hwnd = hwnd;
    sMsg.message = nMsg;
    sMsg.wParam = wParam;
    sMsg.lParam = lParam;
    sMsg.m_bUnicode = bUnicode;

    {
        std::lock_guard<std::mutex> lock(m_sendLock);
        m_sendQueue.push_back(std::move(sMsg));
    }

    if (bNoWait)
        return 1;

    int64_t nResult = 0;
    ASL::ASLThread* pThread = ASL::GetCurrentThreadHANDLE();
    for (;;)
    {
        if (pThread != 0)
            pThread->m_msgQueue.ProcessSendQ();
        if ((ASL::Internal::mWinMainRunning & 1) == 0)
            break;
        std::chrono::steady_clock::time_point tTimeout =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(8);
        if (future.wait_until(tTimeout) != std::future_status::timeout)
            break;
    }
    if (ASL::Internal::mWinMainRunning & 1)
        nResult = future.get();
    return static_cast<unsigned int>(nResult);
}

// Runs the window procedure each queued message names under m_sendLock, then satisfies its promise.
void CWin32MSGQueue::ProcessSendQ()
{
    while (m_sendQueue.size() != 0)
    {
        SendMsg sMsg = [&]() -> SendMsg
        {
            std::lock_guard<std::mutex> lock(m_sendLock);
            SendMsg sFront = std::move(m_sendQueue.front());
            m_sendQueue.pop_front();
            return sFront;
        }();

        if (sMsg.m_bUnicode)
        {
#ifdef K2_FIX_ENGINE_BUGS
            sMsg.m_promise.set_value(CallWindowProcW(reinterpret_cast<void*>(GetWindowLongW(sMsg.hwnd, -4)),
                                                     sMsg.hwnd, sMsg.message, sMsg.wParam,
                                                     sMsg.lParam));
#else
            sMsg.m_promise.set_value(CallWindowProcW(reinterpret_cast<void*>(static_cast<int>(GetWindowLongW(sMsg.hwnd, -4))),
                                                     sMsg.hwnd, sMsg.message, sMsg.wParam,
                                                     sMsg.lParam));
#endif
        }
        else
        {
            sMsg.m_promise.set_value(CallWindowProcA(reinterpret_cast<void*>(GetWindowLongA(static_cast<HWND__*>(sMsg.hwnd), -4)),
                                                     sMsg.hwnd, sMsg.message, sMsg.wParam,
                                                     sMsg.lParam));
        }
    }
}
