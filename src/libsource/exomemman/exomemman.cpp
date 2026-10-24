// The memory manager's debug bookkeeping and report printing. CExoMemHeap.h holds the heap list
// this file walks.

#include "k2stdint.h"
#include "exomemman.h"
#include "exomemheap.h"
#include <stddef.h>
#include <stdio.h>
#include <time.h>

int g_nTotalNewAllocated;
int g_nTotalContiguousMemory;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"

extern int   g_bProcessModuleMemUsage;
#ifdef K2_HOST_BUILD
extern char* g_szProcessModuleMemName;   // console.cpp's definition
#else
extern char  g_szProcessModuleMemName[];
#endif
extern int   usedtexturememory;
extern int   g_nTotalNewAllocated;
extern int   g_nTotalContiguousMemory;

static char szComment[128];

int s_nLastFreeComment;

// Instantiated on first use; the destructor runs at exit.
CExoMemMan* GetMemMan()
{
    static CExoMemMan memMan;
    return &memMan;
}

int SetFileLine(const char* pszFile, int nLine)
{
    if (GetMemMan()->m_bTrackFileLine)
    {
        GetMemMan()->SetLastFile(pszFile);
        GetMemMan()->SetLastLine(nLine);
    }

    return 0;
}

void CExoMemMan::SetLastFile(const char* pFile)
{
    m_pLastFile = pFile;
}

void CExoMemMan::SetLastLine(int line)
{
    m_nLastLine = line;
}

int SetComment(const char* pszComment)
{
    GetMemMan();

    if (pszComment != 0)
        strncpy(szComment, pszComment, sizeof(szComment));
    else
        szComment[0] = 0;

    return 0;
}

void CExoMemMan::SetLastComment(const char* pszComment)
{
    if (pszComment != 0)
        strncpy(m_szLastComment, pszComment, sizeof(m_szLastComment));
    else
        m_szLastComment[0] = 0;
}

// Formats "file:line" into the comment buffer directly, without going through SetComment().
int SetLineFileComment(const char* pszFile, int nLine)
{
    char szBuffer[1024];

    sprintf(szBuffer, "%s:%d", pszFile, nLine);

    GetMemMan();
    __strncpy_chk2(szComment, szBuffer, sizeof(szComment), sizeof(szComment), sizeof(szBuffer));

    return 0;
}

CExoMemMan::CExoMemMan()
{
    m_pReserved1 = NULL;

    Clear();

    m_aFileNames[0] = "none";
    m_bTrackFileLine = 1;
    m_bMemTrackEnabled = 0;

#if KOTOR1
    m_pReportFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exomemman/exomemman.cpp",
                           "CExoMemMan", 614, "Logs\\swcMemReport.txt", "wt");
#else
    m_pReportFile = fopen("Logs\\swcMemReport.txt", "wt");
#endif
    if (m_pReportFile != NULL)
    {
        time_t now;
        time(&now);
        fprintf(m_pReportFile, ">> File Opened: %s\n\n", ctime(&now));
    }
}

// The zero fill runs one byte past the array, so the last byte lands in m_aTypeStack[0].
void CExoMemMan::Clear()
{
    memset(m_aFileNames, 0, sizeof(m_aFileNames));

    for (int i = 0; i < 40000; i++)
    {
        m_aComments[i][0] = '\0';
    }

    m_pLastFile = NULL;
    m_nLastLine = -1;
    m_szLastComment[0] = '\0';

    memset(m_aReserved2, 0, sizeof(m_aReserved2));
    m_aTypeStack[0] = 0;

    m_nTypeStackDepth = 1;
}

// Interns a file name. The table holds pointers and the match test is pointer identity, not strcmp,
// because every call site passes a __FILE__ literal, so every allocation site in one unit shares
// one address. A free slot is claimed on the way past.
int CExoMemMan::GetFileIndex(char* pszFile)
{
    if (pszFile == NULL)
    {
        return 0;
    }

    for (int i = 0; i < 1000; i++)
    {
        if (m_aFileNames[i] == pszFile)
        {
            return i;
        }
        if (m_aFileNames[i] == NULL)
        {
            m_aFileNames[i] = pszFile;
            return i;
        }
    }

    return -1;
}

CExoMemMan::~CExoMemMan()
{
    fclose(m_pReportFile);
}

void CExoMemMan::Destroy()
{
}

int CExoMemMan::PushType(unsigned int nType)
{
    unsigned int nDepth = m_nTypeStackDepth;

    if (nType <= 0xff && nDepth <= 1023)
    {
        m_nTypeStackDepth = nDepth + 1;
        m_aTypeStack[nDepth] = static_cast<unsigned char>(nType);
        return 1;
    }

    return 0;
}

// The stack is 1-based, so a depth of 1 means empty.
int CExoMemMan::PopType(unsigned int nType)
{
    if (m_nTypeStackDepth >= 2)
    {
        m_nTypeStackDepth--;
        return m_aTypeStack[m_nTypeStackDepth] == nType;
    }

    return 0;
}

// The walk never advances, so the first heap is checked forever. No heap ever registers, so the
// loop is unreachable.
void CExoMemMan::CheckHeaps()
{
    CExoMemHeap* pHeap = GetHeaps();

    while (pHeap != NULL)
    {
        pHeap->CheckHeap();
    }
}

// Ignores `this`: the list head is a static member of CExoMemHeap.
CExoMemHeap* CExoMemMan::GetHeaps()
{
    return CExoMemHeap::m_lHeapsStart;
}

// The third parameter is never read; only the name says what it was for.
void PrintBasicMemLine(FILE* pFile, int nSize, char* pszName, int nFlag)
{
    K2_UNUSED(pszName);
    fprintf(pFile, "%d \t 0x%08X \t %d \t %d ", 0, 0, nSize, nFlag == 0 ? 2 : 1);
    fputc('\n', pFile);
}

// localtime is called once, outside the loop; the fields are re-read per heap. tm_mon is printed
// raw, so every stamped file name carries the month one lower than the real one.
void CExoMemMan::Heap_Walk(int nToFile)
{
    char szFileName[128];

    time_t now;
    time(&now);
    struct tm* pTime = localtime(&now);

    CExoMemHeap* pHeap = GetHeaps();
    while (pHeap != NULL)
    {
        if (nToFile == 0)
        {
            pHeap->Heap_Walk(NULL, NULL);
        }
        else
        {
            sprintf(szFileName, "logs\\HeapContents_%s_%d%02d%02d%02d%02d.txt",
                    pHeap->m_szHeapName, pTime->tm_year + 1900, pTime->tm_mon, pTime->tm_mday,
                    pTime->tm_hour, pTime->tm_min);

            if (g_bProcessModuleMemUsage)
            {
                sprintf(szFileName, "logs\\%010d_%s.txt",
                        g_nTotalNewAllocated + usedtexturememory + g_nTotalContiguousMemory,
                        g_szProcessModuleMemName);
            }

#if KOTOR1
            FILE* pContentsFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exomemman/exomemman.cpp",
                                         "Heap_Walk", 863, szFileName, "wt");
#else
            FILE* pContentsFile = fopen(szFileName, "wt");
#endif

            sprintf(szFileName, "logs\\HeapStats_%s_%d%02d%02d%02d%02d.txt",
                    pHeap->m_szHeapName, pTime->tm_year + 1900, pTime->tm_mon, pTime->tm_mday,
                    pTime->tm_hour, pTime->tm_min);

#if KOTOR1
            FILE* pStatsFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exomemman/exomemman.cpp",
                                      "Heap_Walk", 873, szFileName, "wt");
#else
            FILE* pStatsFile = fopen(szFileName, "wt");
#endif

            if (pContentsFile != NULL && pStatsFile != NULL)
            {
                pHeap->Heap_Walk(pContentsFile, pStatsFile);
                PrintBasicMemLine(pContentsFile, usedtexturememory, pHeap->m_szHeapName, 1);
                fclose(pContentsFile);
                fclose(pStatsFile);
            }
        }

        pHeap = pHeap->m_pNext;
    }
}

const char* CExoMemMan::GetLastFile()
{
    return m_pLastFile;
}

char* CExoMemMan::GetLastComment()
{
    return m_szLastComment;
}

int CExoMemMan::GetLastLine()
{
    return m_nLastLine;
}

// Round-robin scan for a free comment slot. The cursor is a global, so a scan resumes where the
// last one stopped and wraps at most once per outer iteration.
int CExoMemMan::AddComment(char* pszComment)
{
    while (pszComment[0] != '\0')
    {
        if (m_nCommentCount / 64 > 624)
        {
            break;
        }

        while (s_nLastFreeComment < 40000)
        {
            if (m_aComments[s_nLastFreeComment][0] == '\0')
            {
                break;
            }
            s_nLastFreeComment++;
        }

        if (s_nLastFreeComment != 40000)
        {
            K2_ASSUME(s_nLastFreeComment >= 0 && s_nLastFreeComment < 40000);   // only this scan moves it, up from 0
            strncpy(m_aComments[s_nLastFreeComment], pszComment, 128);
            m_aComments[s_nLastFreeComment][127] = '\0';
            m_nCommentCount++;
            return s_nLastFreeComment;
        }

        s_nLastFreeComment = 0;
    }

    return -1;
}

void CExoMemMan::RemoveComment(int nIndex)
{
    if (nIndex == -1)
    {
        return;
    }
    if (m_aComments[nIndex][0] == '\0')
    {
        return;
    }

    m_aComments[nIndex][0] = '\0';
    m_nCommentCount--;
}

const char* CExoMemMan::GetFileName(int slot)
{
    return m_aFileNames[slot];
}

// The -1 sentinel means "no comment", which prints as a dash.
char* CExoMemMan::GetComment(int nIndex)
{
    if (nIndex == -1)
    {
        return const_cast<char*>("-");
    }
    return m_aComments[nIndex];
}

// The formatted lines are discarded; only the formatting calls matter here.
void CExoMemMan::PrintMemTrackStatsOutput()
{
    char szBuffer[512];

    if (m_bMemTrackStats)
    {
        CExoMemHeap* pHeap = GetMemMan()->GetHeaps();
        while (pHeap != NULL)
        {
            int nTrackPeakBlocks = pHeap->m_nTrackPeakBlocks;
            int nTrackLiveBlocks = pHeap->m_nTrackLiveBlocks;
            int nTrackTotalBlocks = pHeap->m_nTrackTotalBlocks;
            int nTrackPeakBytes = pHeap->m_nTrackPeakBytes;
            int nTrackTotalBytes = pHeap->m_nTrackTotalBytes;
            int nTrackRequestedBytes = pHeap->m_nTrackRequestedBytes;
            int nTrackLiveBytes = pHeap->m_nTrackLiveBytes;

            sprintf(szBuffer, "Heap: %s", pHeap->m_szHeapName);
            sprintf(szBuffer, "Blocks: %21d | %10d / %10d", nTrackTotalBlocks, nTrackLiveBlocks, nTrackPeakBlocks);
            sprintf(szBuffer, "Bytes: %10d(%10d) | %10d / %10d",
                    nTrackTotalBytes, nTrackRequestedBytes, nTrackLiveBytes, nTrackPeakBytes);

            pHeap = pHeap->m_pNext;
        }
    }
}

void CExoMemMan::LogMemTrackOutputAlloc(int a, int b, int nFileIndex, int nCommentIndex)
{
    if (m_bMemTrackEnabled && m_pReportFile != NULL)
    {
        time_t now;
        time(&now);
        fprintf(m_pReportFile, " %d | %d | %s | %s | %s", a, b, GetFileName(nFileIndex),
                GetComment(nCommentIndex), ctime(&now));
    }
}

// Identical to the alloc twin apart from the leading character of the format string.
void CExoMemMan::LogMemTrackOutputFree(int a, int b, int nFileIndex, int nCommentIndex)
{
    if (m_bMemTrackEnabled && m_pReportFile != NULL)
    {
        time_t now;
        time(&now);
        fprintf(m_pReportFile, "-%d | %d | %s | %s | %s", a, b, GetFileName(nFileIndex),
                GetComment(nCommentIndex), ctime(&now));
    }
}

void CExoMemMan::Update()
{
}

// The int parameter is never read, and the six formatted lines are discarded.
void CExoMemMan::PrintHeapWalk(int nUnused)
{
    K2_UNUSED(nUnused);
    char szBuffer[256];

    CExoMemHeap* pHeap = GetMemMan()->GetHeaps();
    while (pHeap != NULL)
    {
        int nUsedSmallest = pHeap->m_nUsedSmallest;
        int nUsedLargest = pHeap->m_nUsedLargest;
        int nUsedAverage = pHeap->m_nUsedAverage;
        int nFreeSmallest = pHeap->m_nFreeSmallest;
        int nFreeLargest = pHeap->m_nFreeLargest;
        int nFreeAverage = pHeap->m_nFreeAverage;
        int nBlocksTotal = pHeap->m_nBlocksTotal;
        int nBlocksFree = pHeap->m_nBlocksFree;
        int nBlocksUsed = pHeap->m_nBlocksUsed;

        sprintf(szBuffer, "Heap: %s", pHeap->m_szHeapName);
        sprintf(szBuffer, "         %s        %s         %s", "Total", "Used", "Free");
        sprintf(szBuffer, "Blocks: [%10d] [%10d] [%10d]", nBlocksTotal, nBlocksUsed, nBlocksFree);
        sprintf(szBuffer, "         %s      %s     %s", "Largest", "Smallest", "Average");
        sprintf(szBuffer, "Used  : [%10d] [%10d] [%10d]", nUsedLargest, nUsedSmallest,
                nUsedAverage);
        sprintf(szBuffer, "Free  : [%10d] [%10d] [%10d]", nFreeLargest, nFreeSmallest,
                nFreeAverage);

        pHeap = pHeap->m_pNext;
    }
}
