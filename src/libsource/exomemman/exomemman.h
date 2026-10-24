#pragma once

class CExoMemHeap;
struct __sFILE;

class CExoMemMan {
public:
    CExoMemMan();
    ~CExoMemMan();

    void         Clear();
    void         Destroy();
    void         Update();

    void         SetLastFile(const char* pFile);
    void         SetLastLine(int line);
    void         SetLastComment(const char* pszComment);
    const char*  GetLastFile();
    char*        GetLastComment();
    int          GetLastLine();

    int          GetFileIndex(char* pszFile);
    const char*  GetFileName(int slot);
    int          AddComment(char* pszComment);
    void         RemoveComment(int nIndex);
    char*        GetComment(int nIndex);

    int          PushType(unsigned int nType);
    int          PopType(unsigned int nType);

    CExoMemHeap* GetHeaps();
    void         CheckHeaps();
    void         Heap_Walk(int nToFile);
    void         PrintHeapWalk(int nUnused);

    void         PrintMemTrackStatsOutput();
    void         LogMemTrackOutputAlloc(int a, int b, int nFileIndex, int nCommentIndex);
    void         LogMemTrackOutputFree(int a, int b, int nFileIndex, int nCommentIndex);

    void*         m_pReserved1;
    unsigned char m_aReserved2[0x400];
    unsigned char m_aTypeStack[1024];
    unsigned int  m_nTypeStackDepth;
    char          _pad80c[0x810 - 0x80c];
    const char*   m_pLastFile;
    char          m_szLastComment[128];
    int           m_nLastLine;
    char          _pad89c[0x8a0 - 0x89c];
    const char*   m_aFileNames[1000];
    char          m_aComments[40000][128];
    unsigned int  m_nCommentCount;
    int           m_bTrackFileLine;
    int           m_bMemTrackEnabled;
    int           m_bMemTrackStats;
    __sFILE*      m_pReportFile;
};

CExoMemMan* GetMemMan();

int SetComment(const char* pszComment);
int SetFileLine(const char* pszFile, int nLine);
int SetLineFileComment(const char* pszFile, int nLine);
void PrintBasicMemLine(__sFILE* pFile, int nSize, char* pszName, int nFlag);
