#pragma once

struct __sFILE;

// The abstract base of the exobase allocator's heaps. Every heap links itself into one process-wide
// singly-linked list at construction, and CExoMemMan's report functions walk that list.
class CExoMemHeap {
public:
    virtual ~CExoMemHeap();
    virtual void ReservedSlot1() = 0;
    virtual void ReservedSlot2() = 0;
    virtual void ReservedSlot3() = 0;
    // Called once per heap by CExoMemMan::Heap_Walk, with a pair of freshly opened report files or
    // with two nulls when asked for the silent walk.
    virtual void Heap_Walk(__sFILE* pContentsFile, __sFILE* pStatsFile) = 0;
    // Called once per heap by CExoMemMan::CheckHeaps.
    virtual void CheckHeap() = 0;

    // Static: both take the heap explicitly.
    static void AddHeap(CExoMemHeap* pHeap);
    static void RemoveHeap(CExoMemHeap* pHeap);

    void SetHeapName(const char* pszName);

    static CExoMemHeap* m_lHeapsStart;

    CExoMemHeap* m_pNext;

    int m_nTrackPeakBlocks;
    int m_nTrackLiveBlocks;
    int m_nTrackTotalBlocks;
    int m_nTrackPeakBytes;
    int m_nTrackTotalBytes;
    int m_nTrackRequestedBytes;
    int m_nTrackLiveBytes;
    int m_nUsedSmallest;
    int m_nUsedLargest;
    int m_nUsedAverage;
    int m_nFreeSmallest;
    int m_nFreeLargest;
    int m_nFreeAverage;
    int m_nBlocksTotal;
    int m_nBlocksFree;
    int m_nBlocksUsed;
    char m_szHeapName[128];
    char m_szNameSuffix[128];
};
