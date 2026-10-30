// The Aurora debug console, with the command table (ConsoleFunc / ConsoleFuncs_*), the command
// interpreter, and the raw resource reader (AurRes*) that backs `dofile` and friends.
#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurconsole.h"
#include "nullname.h"
#include "Vector.h"
#include "Quaternion.h"
#include "AURORA/aurmodel.h"
#include "aurio.h"
#include "Win32Find.h"

unsigned char returnvalue[1024];

// The per-T funcholder specializations are defined at the bottom of the file; declare them
// before the interpreter's first virtual use instantiates the class.
template <> void funcholder<void (*)()>::Engage(char* args);
template <> void funcholder<void (*)(char*)>::Engage(char* args);
template <> void funcholder<void (*)(int)>::Engage(char* args);
template <> void funcholder<int (*)(char*)>::Engage(char* args);
template <> void funcholder<float (*)(char*)>::Engage(char* args);
template <> void funcholder<char* (*)(char*)>::Engage(char* args);
template <> void funcholder<void (*)(char*, int, Quaternion)>::Engage(char* args);
template <> void funcholder<void (*)(char*, Vector)>::Engage(char* args);
template <> void funcholder<int*>::Engage(char* args);
template <> void funcholder<float*>::Engage(char* args);
template <> void funcholder<Vector*>::Engage(char* args);
template <> void funcholder<Quaternion*>::Engage(char* args);
template <> void funcholder<int*>::PreEngage(char* args);
template <> void funcholder<float*>::PreEngage(char* args);
template <> void funcholder<Vector*>::PreEngage(char* args);
template <> void funcholder<Quaternion*>::PreEngage(char* args);

// strcmp is declared throw(): a potentially-throwing one makes ~ConsoleFunc emit a terminate landing
// pad it does not need.
#include "port/libcbase.h"
#include "port/libcstring.h"

ConsoleFunc* ConsoleFuncs[1024];
int ConsoleFuncs_num;

// Insertion sort into the name-ordered command table. Duplicate names are not rejected: the
// existing entry wins and the newcomer settles after it.
void ConsoleFuncs_Add(ConsoleFunc* pFunc) {
    ConsoleFunc* pCur = pFunc;
    for (int i = 0; i < ConsoleFuncs_num; i++) {
        ConsoleFunc* pEntry = ConsoleFuncs[i];
        if (strcmp(reinterpret_cast<char*>(pCur), reinterpret_cast<char*>(pEntry)) < 0) {
            ConsoleFuncs[i] = pCur;
            pCur = pEntry;
        }
    }
    ConsoleFuncs_num++;
    ConsoleFuncs[ConsoleFuncs_num - 1] = pCur;
}

// Find the entry by name, shift the tail down over it and drop the count.
void ConsoleFuncs_Remove(ConsoleFunc* pFunc) {
    int i;
    for (i = 0; i < ConsoleFuncs_num; i++) {
        if (strcmp(reinterpret_cast<char*>(pFunc), reinterpret_cast<char*>(ConsoleFuncs[i])) == 0) break;
    }
    int nLast = ConsoleFuncs_num - 1;
    if (i < nLast) {
        for (; i < nLast; i++) ConsoleFuncs[i] = ConsoleFuncs[i + 1];
    }
    ConsoleFuncs_num = nLast;
}

// Look a command up by name and hand back its holder. A function-local static caches the index
// of the last hit, so a repeat lookup is O(1); a miss rescans from the start.
funcholder<void (*)()>* FindConCallBack(char* pszName) {
    static int i = -1;
    int nCount = ConsoleFuncs_num;
    if (i >= 0 && i < nCount) {
        ConsoleFunc* pFunc = ConsoleFuncs[i];
        if (strcmp(pszName, reinterpret_cast<char*>(pFunc)) == 0) return pFunc->m_pHolder;
    }
    for (i = 0; i < nCount; i++) {
        ConsoleFunc* pFunc = ConsoleFuncs[i];
        if (strcmp(pszName, reinterpret_cast<char*>(pFunc)) == 0) return pFunc->m_pHolder;
    }
    return 0;
}

// Completes on TAB. It completes the last word of the line to the longest common prefix of the
// matches, or appends the single match, and writes the result into the shared return buffer.
char* CommandCompletion(char* pszPartial, List<char*>& matches) {
    matches.m_count = 0;
    reinterpret_cast<char*>(returnvalue)[0] = 0;
    if (!pszPartial) return reinterpret_cast<char*>(returnvalue);
    __strcpy_chk(reinterpret_cast<char*>(returnvalue), pszPartial, 1024);
    int nLen = static_cast<int>(strlen(pszPartial));
    char* pszWord = reinterpret_cast<char*>(returnvalue);
    char* pszStart = pszPartial;
    for (int i = 0; i < nLen; i++) {
        if (pszPartial[i] == ' ') {
            pszWord = reinterpret_cast<char*>(returnvalue) + i + 1;
            pszStart = pszPartial + i + 1;
        }
    }
    for (int i = 0; i < ConsoleFuncs_num; i++) {
        if (strncmp(pszStart, reinterpret_cast<char*>(ConsoleFuncs[i]), strlen(pszStart)) == 0)
            matches.Add(reinterpret_cast<char*>(ConsoleFuncs[i]));
    }
    if (matches.m_count) {
        if (matches.m_count == 1) {
            FormatReturnValue(pszWord, static_cast<uint64_t>(-1), "%s ", matches.m_pData[0]);
        } else {
            for (int i = 0;; i++) {
                unsigned char c = static_cast<unsigned char>(matches.m_pData[0][i]);
                if (c != 0 && matches.m_count > 1) {
                    for (int j = 1; c != 0 && j < matches.m_count; j++) {
                        if (c != static_cast<unsigned char>(matches.m_pData[j][i])) c = 0;
                    }
                }
                pszWord[i] = static_cast<char>(c);
                if (c == 0) break;
            }
        }
    }
    return reinterpret_cast<char*>(returnvalue);
}

// The console's command interpreter does history bookkeeping, in-place bracket expansion, then a
// name/argument split with the argument text handed to the command's holder.
List<char*> g_listHistory;
int g_nHistoryPosition;


char* FuncInterp(char* pszExpr) {
    char szLine[16384];
    char szName[128];
    char szArgs[16384];
    while (*pszExpr == ' ' || *pszExpr == '\t') pszExpr++;
    if (g_bKeepHistory) {
        char* pCopy = new char[strlen(pszExpr) + 1];
        strcpy(pCopy, pszExpr);
        if (g_listHistory.m_count == 0
                || strcmp(pCopy, g_listHistory.m_pData[g_listHistory.m_count - 1]) != 0) {
            g_listHistory.Add(pCopy);
            g_nHistoryPosition = g_listHistory.m_count;
        }
    }
    returnvalue[0] = 0;
    __strcpy_chk(szLine, pszExpr, sizeof(szLine));
    if (szLine[0] == '"') szLine[0] = ' ';
    for (char* p = szLine; *p != 0; p++) {
        if (*p == '"') {
            while (*++p != 0 && *p != '"') {
            }
        } else if (*p == '[') {
            char* pEnd = p + 1;
            int nDepth = 1;
            while (*pEnd != 0) {
                if (*pEnd == '[') nDepth++;
                if (*pEnd == ']') nDepth--;
                if (nDepth == 0) break;
                pEnd++;
            }
            if (nDepth != 0) {
                strcpy(reinterpret_cast<char*>(returnvalue), "missing ']'");
                return reinterpret_cast<char*>(returnvalue);
            }
            *pEnd = 0;
            *p++ = 0;
            FuncInterp(p);
            __strcpy_chk(szArgs, szLine, sizeof(szArgs));
            __strcat_chk(szArgs, reinterpret_cast<char*>(returnvalue), sizeof(szArgs));
            __strcat_chk(szArgs, pEnd + 1, sizeof(szArgs));
            __strcpy_chk(szLine, szArgs, sizeof(szLine));
            p = szLine;
        }
    }
    unsigned char* pszIn = reinterpret_cast<unsigned char*>(szLine);
    while (*pszIn == ' ' || *pszIn == '\t') pszIn++;
    uint64_t i = 0;
    for (;;) {
        switch (pszIn[i]) {
        case 0:
        case ' ':
        case '=':
            goto done;
        default:
#ifdef K2_FIX_ENGINE_BUGS
            if (i == sizeof(szName) - 1)   // a longer name is cut; the rest is argument text
                goto done;
#endif
            szName[i] = static_cast<char>(pszIn[i]);
            i++;
            break;
        }
    }
done:
    szName[i] = 0;
    char* pszArgs = reinterpret_cast<char*>(pszIn) + i;
    while (*pszArgs == '=' || *pszArgs == ' ') pszArgs++;
    funcholder<void (*)()>* pHolder = FindConCallBack(szName);
    if (pHolder) {
        __strcpy_chk(szArgs, pszArgs, sizeof(szArgs));
        char* pszTail = szArgs + __strlen_chk(szArgs, sizeof(szArgs));
        while (pszTail > szArgs) {
            char c = *--pszTail;
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            *pszTail = 0;
        }
        pHolder->Engage(szArgs);
    } else {
        FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "Function %s not found", szName);
    }
    return reinterpret_cast<char*>(returnvalue);
}

// Split "<name>[=]<args>" and hand the argument text to the command's holder without running
// it, so a data-pointer command prints its current value.
char* FuncPreInterp(char* pszCommand) {
    char szName[128];
    unsigned char* pszIn = reinterpret_cast<unsigned char*>(pszCommand);
    returnvalue[0] = 0;
    int i = 0;
    for (;;) {
        switch (pszIn[i]) {
        case 0:
        case ' ':
        case '=':
            goto done;
        default:
#ifdef K2_FIX_ENGINE_BUGS
            if (i == static_cast<int>(sizeof(szName)) - 1)   // a longer name is cut; the rest is argument text
                goto done;
#endif
            szName[i] = static_cast<char>(pszIn[i]);
            i++;
            break;
        }
    }
done:
    szName[i] = 0;
    char* pszArgs = pszCommand + i;
    while (*pszArgs == '=' || *pszArgs == ' ') pszArgs++;
    funcholder<void (*)()>* pHolder = FindConCallBack(szName);
    if (pHolder) pHolder->PreEngage(pszArgs);
    return reinterpret_cast<char*>(returnvalue);
}

// The twelve constructors name the command and wrap the callable in a funcholder of the right
// signature, then the object registers itself.
ConsoleFunc::ConsoleFunc(char* pszName, void (*func)()) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = new funcholder<void (*)()>(func);
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, void (*func)(char*)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<void (*)(char*)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, void (*func)(int)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<void (*)(int)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, int (*func)(char*)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<int (*)(char*)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, float (*func)(char*)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<float (*)(char*)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, char* (*func)(char*)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<char* (*)(char*)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, void (*func)(char*, Vector)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<void (*)(char*, Vector)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, void (*func)(char*, int, Quaternion)) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<void (*)(char*, int, Quaternion)>(func));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, int* pValue) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<int*>(pValue));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, float* pValue) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<float*>(pValue));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, Vector* pValue) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<Vector*>(pValue));
    ConsoleFuncs_Add(this);
}
ConsoleFunc::ConsoleFunc(char* pszName, Quaternion* pValue) {
    __strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pHolder = reinterpret_cast<funcholder<void (*)()>*>(new funcholder<Quaternion*>(pValue));
    ConsoleFuncs_Add(this);
}

// Leave the table, then delete the holder through its vtable.
ConsoleFunc::~ConsoleFunc() {
    ConsoleFuncs_Remove(this);
    delete m_pHolder;
}

// Unregister one command by name and destroy it, without going through ~ConsoleFunc.
void DeleteFunc(char* pszName) {
    ConsoleFunc* pFunc = 0;
    for (int i = 0; i < ConsoleFuncs_num; i++) {
        if (strcmp(reinterpret_cast<char*>(ConsoleFuncs[i]), pszName) == 0) {
            pFunc = ConsoleFuncs[i];
            break;
        }
    }
    if (!pFunc) return;
    ConsoleFuncs_Remove(pFunc);
    delete pFunc->m_pHolder;
    operator delete(pFunc);
}

// Unregister and destroy every command whose name starts with the prefix; returns the count.
// The scan runs backward, so each removal's tail shift is free.
int DeleteFuncWithPrefix(char* pszPrefix) {
    int nDeleted = 0;
    int i = ConsoleFuncs_num;
    while (i) {
        i--;
#if KOTOR1  // K1 reads the entry once, before the compare
        ConsoleFunc* pFunc = ConsoleFuncs[i];
        if (strncmp(reinterpret_cast<char*>(pFunc), pszPrefix, strlen(pszPrefix)) == 0) {
#else
        if (strncmp(reinterpret_cast<char*>(ConsoleFuncs[i]), pszPrefix, strlen(pszPrefix)) == 0) {
            ConsoleFunc* pFunc = ConsoleFuncs[i];
#endif
            if (pFunc) {
                ConsoleFuncs_Remove(pFunc);
                delete pFunc->m_pHolder;
                operator delete(pFunc);
            }
            nDeleted++;     // a null entry still counts
        }
    }
    return nDeleted;
}

List<OpenResource*> Resources;
bool g_bKeepHistory;


void HistoryOn() {
    g_bKeepHistory = 1;
}

void HistoryOff() {
    g_bKeepHistory = 0;
}

OpenResource::OpenResource(int nSize) {
    m_pFile = 0;
    m_nLine = -1;
    m_pHandle = 0;
    m_nLength = 0;
    m_nSize = nSize > 8192 ? nSize : 8192;
    m_pData = 0;
    m_pCursor = 0;
}

OpenResource::~OpenResource() {
#if KOTOR1  // K1 closes the stream in the destructor
    if (m_pFile) {
        SDL_RWclose(m_pFile);
        m_pFile = 0;
    }
#endif
    if (m_pData) {
        operator delete[](m_pData);
        m_pData = 0;
    }
}

// A null argument means "the most recently opened resource"; a non-null one is validated
// against the table and 0 comes back if it is not there.
void* GetResource(void* pResource) {
    if (!pResource) return Resources.m_pData[Resources.m_count - 1];
    for (int i = 0; i < Resources.m_count; i++)
        if (Resources.m_pData[i] == pResource) return Resources.m_pData[i];
    return 0;
}

// Find by pointer, drop the count and shift the tail down over the hole.
void AurResRemove(void* pResource) {
    for (int i = 0; i < Resources.m_count; i++) {
        if (Resources.m_pData[i] == pResource) {
            Resources.DelIndex(i);
            break;
        }
    }
}

// Close the backing store, unlink the object from the table and delete it.
void AurResFree(void* pRes, int nSize) {
    if (!pRes) return;
    OpenResource* p = static_cast<OpenResource*>(pRes);
    if (p->m_pFile) {
#if KOTOR1
        SDL_RWclose(p->m_pFile);
#else
        fclose(p->m_pFile);
#endif
        p->m_pFile = 0;
    } else {
        AurReleaseResource(p->m_pHandle, nSize);
        p->m_pHandle = 0;
    }
    if (Resources.Contains(p)) Resources.Remove(p);
#if KOTOR1  // K1 deletes the object; its destructor closes the already-cleared slot
    delete p;
#else
    if (p->m_pData) operator delete[](p->m_pData);
    operator delete(p);
#endif
}

void AurResFree() {
    int nLast = Resources.m_count - 1;
    AurResFree(Resources.m_pData[nLast], 0);
}

void AurResFreeDataBytes(void* pRes, unsigned char* pBytes) {
    if (pBytes && static_cast<OpenResource*>(pRes)->m_pFile) operator delete[](pBytes);
}

// Does one bulk read out of the current resource. A file resource allocates a fresh buffer and
// reads into it, a memory resource hands back a window of its blob and advances the cursor.
unsigned char* AurResGetDataBytes(unsigned int nBytes, void* pRes) {
    OpenResource* p = static_cast<OpenResource*>(pRes);
#if KOTOR1
    SDL_RWops* pFile = p->m_pFile;
#else
    FILE* pFile = p->m_pFile;
#endif
    unsigned char* pData;
    if (pFile) {
        pData = new unsigned char[nBytes];
#if KOTOR1
        SDL_RWread(pFile, pData, 1, nBytes);
#else
        fread(pData, 1, nBytes, pFile);
#endif
    } else {
        pData = reinterpret_cast<unsigned char*>(p->m_pCursor);
        p->m_pCursor = reinterpret_cast<char*>(pData) + nBytes;
        p->m_nLength -= nBytes;
    }
    return pData;
}

// Reads nSize bytes out of a resource, filling the line buffer from disk or from the memory
// blob and advancing the cursor.
unsigned char* AurResGetData(int nBytes) {
    if (Resources.m_count == 0) return 0;
    OpenResource* pRes = Resources.m_pData[Resources.m_count - 1];
#ifdef K2_FIX_ENGINE_BUGS
    // The read buffer holds m_nSize bytes (8 KB for a memory blob) and a packed vertex list
    // can be larger: reallocate it to fit the read and its terminator.
    if (nBytes >= pRes->m_nSize) {
        delete[] pRes->m_pData;
        pRes->m_pData = 0;
        pRes->m_nSize = nBytes + 1;
    }
#endif
#if KOTOR1  // K1 reads through SDL_RWread and shares SetupReadBuffer between both arms
    if (pRes->m_pFile) {
        pRes->SetupReadBuffer();
        char* pData = pRes->m_pData;
        SDL_RWread(pRes->m_pFile, pData, 1, nBytes);
    } else {
        char* pCursor = pRes->m_pCursor;
        pRes->SetupReadBuffer();
#else
    if (pRes->m_pFile) {
        if (!pRes->m_pData) {
            pRes->m_pData = new char[pRes->m_nSize];
            pRes->m_pData[0] = 0;
        }
        fread(pRes->m_pData, 1, nBytes, pRes->m_pFile);
    } else {
        char* pCursor = pRes->m_pCursor;
        if (!pRes->m_pData) {
            pRes->m_pData = new char[pRes->m_nSize];
            pRes->m_pData[0] = 0;
        }
#endif
#ifdef K2_FIX_ENGINE_BUGS
        K2_ASSUME(nBytes < pRes->m_nSize);   // the reallocation above
#endif
        int i = 0;
        do {
            pRes->m_pData[i] = pCursor[i];
            i++;
        } while (i < nBytes && i < pRes->m_nLength);
        pRes->m_nLength -= i;
        pRes->m_pCursor = pCursor + i;
        if (pRes->m_nLength < 0) return 0;
#ifdef K2_FIX_ENGINE_BUGS
        K2_ASSUME(i < pRes->m_nSize);   // i <= nBytes
#endif
        pRes->m_pData[i] = 0;
    }
    return reinterpret_cast<unsigned char*>(pRes->m_pData);
}

// The two line readers behind both AurResGetNextLine spellings. Each takes a handle rather
// than an OpenResource: the handle has already been resolved, or is 0 for "the newest".
#if KOTOR1
void OpenResource::SetupReadBuffer() {
    if (!m_pData) {
        m_pData = new char[static_cast<uint64_t>(m_nSize)];
        m_pData[0] = 0;
    }
}

// K1 has no fgets on an SDL_RWops: read a whole buffer, cut it at the first CR or LF and seek
// back.
static char* getnextline_file(void* pResource) {
    OpenResource* p = static_cast<OpenResource*>(GetResource(pResource));
    p->m_nLine++;
    p->SetupReadBuffer();
    int nRead = static_cast<int>(SDL_RWread(p->m_pFile, p->m_pData, 1, p->m_nSize));
    bool bEol = false;
    for (int i = 0; i < nRead; i++) {
        if (p->m_pData[i] == '\r' || p->m_pData[i] == '\n') {
            if (!bEol) p->m_pData[i] = 0;
            bEol = true;
        } else if (bEol) {
#ifdef K2_FIX_ENGINE_BUGS
            SDL_RWseek(p->m_pFile, i - nRead, RW_SEEK_CUR);   // back to the next line's start
#else
            SDL_RWseek(p->m_pFile, nRead - i, p->m_nSize > nRead ? RW_SEEK_END : RW_SEEK_CUR);
#endif
            i = nRead;
            bEol = false;
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    // Bug fix: otherwise falls off the end without returning a value. Returns the line, or 0 at the
    // end of the file.
    return nRead > 0 ? p->m_pData : 0;
#endif
}
#else
static char* getnextline_file(void* pResource) {
    OpenResource* p = static_cast<OpenResource*>(GetResource(pResource));
    p->m_nLine++;
    if (!p->m_pData) {
        p->m_pData = new char[p->m_nSize];
        p->m_pData[0] = 0;
    }
    K2_ASSUME(p->m_nSize >= 4000);   // resource buffers are at least 8 KB
    return fgets(p->m_pData, 4000, p->m_pFile);
}
#endif

// The memory-blob half copies until the buffer is full or a newline lands, terminates the string
// and advances the cursor. 0 means the blob ran out.
static char* getnextline_res(void* pResource) {
    OpenResource* p = static_cast<OpenResource*>(GetResource(pResource));
    if (p->m_nLength < 1) return 0;
    char* pCursor = p->m_pCursor;
#if KOTOR1  // K1 also ends a line at CR, and consumes a CR LF pair whole
    p->SetupReadBuffer();
    int i = 0;
    do {
        p->m_pData[i] = pCursor[i];
        i++;
    } while (i < p->m_nLength && p->m_pData[i - 1] != '\r' && p->m_pData[i - 1] != '\n'
#ifdef K2_FIX_ENGINE_BUGS
             && i < p->m_nSize - 1
#endif
             );
    char* pNext = pCursor + i;
#ifdef K2_FIX_ENGINE_BUGS
    if (p->m_pData[i - 1] == '\r' && i < p->m_nLength && pCursor[i] == '\n') {
#else
    if (p->m_pData[i - 1] == '\r' && pCursor[i] == '\n') {
#endif
        p->m_nLength--;
        pNext++;
    }
    p->m_nLength -= i;
    p->m_pCursor = pNext;
#else
    if (!p->m_pData) {
        p->m_pData = new char[p->m_nSize];
        p->m_pData[0] = 0;
    }
    int i = 0;
    do {
        p->m_pData[i] = pCursor[i];
        i++;
    } while (i < p->m_nLength && p->m_pData[i - 1] != '\n'
#ifdef K2_FIX_ENGINE_BUGS
             && i < p->m_nSize - 1
#endif
             );
    p->m_nLength -= i;
    p->m_pCursor = pCursor + i;
#endif
    if (p->m_nLength < 0) return 0;
#ifdef K2_FIX_ENGINE_BUGS
    K2_ASSUME(i < p->m_nSize);   // the copy loop stops at m_nSize - 1
#endif
    p->m_pData[i] = 0;
    return p->m_pData;
}

// One line out of the newest resource.
char* AurResGetNextLine() {
    if (Resources.m_count == 0) return 0;
    OpenResource* pRes = Resources.m_pData[Resources.m_count - 1];
    if (pRes->m_pFile) return getnextline_file(0);
    return getnextline_res(0);
}

// The same read, out of a caller-named resource; a stale handle yields 0.
char* AurResGetNextLine(void* pResource) {
    OpenResource* pRes = static_cast<OpenResource*>(GetResource(pResource));
    if (!pRes) return 0;
    if (pRes->m_pFile) return getnextline_file(pRes);
    return getnextline_res(pRes);
}

// Opens a resource for `dofile` by two paths. It asks the host for a packaged resource first and
// falls back to a real file when the host has none, or is not installed.
OpenResource* AurResGet(char* pszName, char* pszExt, int* pnSize, bool bNoFile) {
    int nSize;
    if (!pszExt) pszExt = const_cast<char*>("");
    if (AurGetResource) {
        unsigned char* pHandle = AurGetResource(pszName, pszExt, &nSize);
        if (pHandle) {
            OpenResource* pRes = new OpenResource(8192);
            pRes->m_pCursor = reinterpret_cast<char*>(pHandle);
            pRes->m_nLength = nSize < 0 ? ~nSize : nSize;
            pRes->m_pHandle = pHandle;
            if (pnSize) *pnSize = nSize;
            Resources.Add(pRes);
            return pRes;
        }
        if (bNoFile) return 0;
    }

#if KOTOR1  // K1 uses a MAX_PATH-sized path buffer
    char szPath[260];
    FormatReturnValue(szPath, 260, "%s%s%s", pszName, pszExt[0] ? (pszExt[0] == '.' ? "" : ".") : ".", pszExt);
#else
    char szPath[350];
    FormatReturnValue(szPath, 350, "%s%s%s", pszName, pszExt[0] ? (pszExt[0] == '.' ? "" : ".") : ".", pszExt);
#endif
#if KOTOR1
    SDL_RWops* pFile = SDL_RWFromFile(szPath, "rb");
#else
    FILE* pFile = fopen(szPath, "rb");
#endif
    if (!pFile) return 0;

    OpenResource* pRes = new OpenResource(8192);
    pRes->m_pFile = pFile;
    pRes->m_nLine = 0;
    pRes->m_nSize = 500000;
    Resources.Add(pRes);
    if (pnSize) *pnSize = 1;
    return pRes;
}

#if KOTOR1
OpenResource* AurResGet(SDL_RWops* pFile) {
#else
// Wrap an already-open file in an OpenResource and publish it. m_nLine restarts at 0 and
// m_nSize gets the 500000-byte file-resource ceiling.
OpenResource* AurResGet(FILE* pFile) {
#endif
    OpenResource* pRes = new OpenResource(8192);
    pRes->m_pFile = pFile;
    pRes->m_nLine = 0;
    pRes->m_nSize = 500000;
    Resources.Add(pRes);
    return pRes;
}

// The console's file-script command splits the argument into a base name and an extension, opens it
// through AurResGet (host resource first, real file second) and interprets it line by line.
static char szDofileResult[256];      // the console's command-result buffer

char* dofile(char* pszName) {
    char szName[64];
    char szExt[16];

    int i = 0;
    while (pszName[i] != 0 && pszName[i] != '.') {
        szName[i] = pszName[i];
        i++;
    }
    szName[i] = 0;

    int j = 0;
    if (pszName[i] != 0) {
        do {
            szExt[j] = pszName[i + j];
            j++;
        } while (pszName[i + j] != 0);
    }
    szExt[j] = 0;
    char* pszEnd = pszName + i + j;

    OpenResource* pRes = AurResGet(szName, szExt, 0, 0);
    if (!pRes) {
        FormatReturnValue(szDofileResult, 256, "ERROR:  file '%s' doesn't exist", pszEnd);
        return szDofileResult;
    }

    char* pLine = AurResGetNextLine(pRes);
    while (pLine) {
        switch (*pLine) {
        case '\t':
        case ' ':
            pLine++;
            continue;
        case 0:
        case '#':
            break;
        default: {
            int nLen = static_cast<int>(strlen(pLine));
            char* p = pLine + nLen - 1;
            while (p >= pLine) {
                if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
                    *p-- = 0;
                else
                    break;
            }
            FuncInterp(pLine);
            break;
        }
        }
        pLine = AurResGetNextLine(pRes);
    }

    AurResFree(pRes, 0);
    FormatReturnValue(szDofileResult, 256, "Done executing file '%s'", pszEnd);
    return szDofileResult;
}

// The console's doresource command is a stub in this build.
void* doresource(void* pUnk, int nSize) {
    K2_UNUSED(pUnk); K2_UNUSED(nSize);
    return 0;
}

#include "port/crtapi.h"
#include "port/win32api.h"
char* spawn(char* pszCommand);
char* GetToken(char** pszCursor);
Model* FindModel(char* pszName);
// Declared with a 32-bit return: the callers here expect the narrow form. Do not replace it with
// math3d.h's unsigned long version; the MSVC ABI mangles the return type, so there the declaration
// has to match the definition.
#ifdef _MSC_VER
uint64_t FloorLogBase2(uint64_t x);
#else
int FloorLogBase2(uint64_t x);
#endif

// The `gobs` registry is a plain list, walked linearly here.
extern List<Gob*> gobs;

static char szLogBase2Result[32];

// Return the name of the first gob whose model matches the argument. A miss returns the
// NULL_NAME literal, so the interpreter always has a printable word.
char* getgobfrommodel(char* pszArgs) {
    char* pszName = GetToken(&pszArgs);
    char* pszResult = const_cast<char*>(NULL_NAME);
    for (int64_t i = 0; i < gobs.m_count; i++) {
        Gob* pGob = gobs.m_pData[i];
        if (pGob->m_pModel != 0 && _stricmp(pGob->m_pModel->m_szName, pszName) == 0) {
            pszResult = pGob->m_szName;
            break;
        }
    }
    return pszResult;
}

// Write one loaded model back out through the I/O dispatcher.
char* outputmodel(char* pszArgs) {
    char* pszName = GetToken(&pszArgs);
    Model* pModel = FindModel(pszName);
    char* pszResult = const_cast<char*>("FAILURE");
    if (pModel != 0)
        pszResult = IODispatcher::GetRef()->Write(pModel, 0) ? const_cast<char*>("SUCCESS") : pszResult;
    return pszResult;
}

// The dispatcher's synchronous loader; returns the parsed tree or 0.
char* bload(char* pszName) {
    MaxTree* pModel = IODispatcher::GetRef()->ReadSync(pszName);
    return pModel != 0 ? const_cast<char*>("SUCCESS") : const_cast<char*>("FAILURE");
}

// Print the base-2 log of the argument.
char* testlogbase2(char* pszArgs) {
    char* pszNum = GetToken(&pszArgs);
    int nLog = static_cast<int>(FloorLogBase2(atoi(pszNum)));
    FormatReturnValue(szLogBase2Result, 32, "%i\n", nLog);
    return szLogBase2Result;
}

// Walk the directory the first argument globs, strip each hit's extension, run the
// extension-free name as an external command, then move into the second directory, write
// the loaded model out and restore the saved directory for the next hit.
char* converttobinary(char* pszArgs) {
    char szCurDir[260];

    char* pszFirst = GetToken(&pszArgs);
    char* pszSecond = GetToken(&pszArgs);
    GetCurrentDirectoryA(260, szCurDir);
#ifdef K2_FIX_ENGINE_BUGS
    if (pszFirst == 0 || pszSecond == 0)
#else
    if (pszFirst == const_cast<char*>("") || pszSecond == const_cast<char*>(""))
#endif
        return const_cast<char*>("FAILURE");

    IODispatcher::GetRef();
    WIN32_FIND_DATAA findData = {};
    char szFile[260];
    void* hFind = FindFirstFileA(pszFirst, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return const_cast<char*>("Finished");
    }
    for (;;) {
        FormatReturnValue(szFile, 260, "%s", findData.cFileName);
        szFile[__strlen_chk(szFile, 260) - 4] = 0;
        spawn(szFile);
        int i = static_cast<int>(__strlen_chk(szFile - 5, 0));
        for (; i >= 0; i--) {
            if (static_cast<unsigned char>(szFile[i]) == '/' || static_cast<unsigned char>(szFile[i]) == '\\')
                break;
        }
        if (!SetCurrentDirectoryA(pszSecond))
            return const_cast<char*>("FAILURE");
        char* pszCursor = szFile + i + 1;
        Model* pModel = FindModel(GetToken(&pszCursor));
        if (pModel != 0)
            IODispatcher::GetRef()->Write(pModel, 0);
        SetCurrentDirectoryA(szCurDir);
        if (!FindNextFileA(hFind, &findData)) {
            if (GetLastError() != ERROR_NO_MORE_FILES)
                return const_cast<char*>("FAILURE");
            FindClose(hFind);
            return const_cast<char*>("Finished");
        }
    }
}

// funcholder<T> lives in this file for the two members the template instantiates: the deleting
// destructor and PreEngage. Engage is a per-T specialization, defined below.

// Engage(char*): parse the argument string, call the held function and publish its result
// into the shared return buffer.

template <> void funcholder<void (*)()>::Engage(char* args) {
    K2_UNUSED(args);
    m_func();
    returnvalue[0] = 0;
}
template <> void funcholder<void (*)(char*)>::Engage(char* args) {
    m_func(args);
    returnvalue[0] = 0;
}
template <> void funcholder<void (*)(int)>::Engage(char* args) {
    int v;
    static_cast<void>(sscanf(args, "%d", &v));
    m_func(v);
    returnvalue[0] = 0;
}
template <> void funcholder<int (*)(char*)>::Engage(char* args) {
    returnvalue[0] = 0;
    int r = m_func(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%d", r);
}
template <> void funcholder<float (*)(char*)>::Engage(char* args) {
    float r = m_func(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f", r);
}
template <> void funcholder<char* (*)(char*)>::Engage(char* args) {
    char* r = m_func(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%s", r ? r : "");
}

// Parse "name int axis.x axis.y axis.z angle" into a quaternion (identity when the axis is
// zero), call the held callee and echo "OK".
template <> void funcholder<void (*)(char*, int, Quaternion)>::Engage(char* args) {
    char name[64];
    Vector axis = {};
    int n;
    float angle;
    static_cast<void>(sscanf(args, "%s %d %f %f %f %f", name, &n, &axis.x, &axis.y, &axis.z, &angle));
    Quaternion q;
    if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f) {
        q.w = 1.0f;
        q.x = 0.0f;
        q.y = 0.0f;
        q.z = 0.0f;
    } else {
        q = Quaternion(axis, angle);
    }
    m_func(name, n, q);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%s", "OK");
}

template <> void funcholder<void (*)(char*, Vector)>::Engage(char* args) {
    char name[64];
    Vector v = {};
    static_cast<void>(sscanf(args, "%s %f %f %f", name, &v.x, &v.y, &v.z));
    m_func(name, v);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%s", "OK");
}

// The deleting destructor and PreEngage for the function-pointer element types.

template funcholder<void (*)()>::~funcholder();
template void funcholder<void (*)()>::PreEngage(char*);

template funcholder<void (*)(char*)>::~funcholder();
template void funcholder<void (*)(char*)>::PreEngage(char*);

template funcholder<void (*)(int)>::~funcholder();
template void funcholder<void (*)(int)>::PreEngage(char*);

template funcholder<int (*)(char*)>::~funcholder();
template void funcholder<int (*)(char*)>::PreEngage(char*);

template funcholder<float (*)(char*)>::~funcholder();
template void funcholder<float (*)(char*)>::PreEngage(char*);

template funcholder<char* (*)(char*)>::~funcholder();
template void funcholder<char* (*)(char*)>::PreEngage(char*);

template funcholder<void (*)(char*, Vector)>::~funcholder();
template void funcholder<void (*)(char*, Vector)>::PreEngage(char*);

template funcholder<void (*)(char*, int, Quaternion)>::~funcholder();
template void funcholder<void (*)(char*, int, Quaternion)>::PreEngage(char*);

// For the data-pointer element types, a "set variable" Engage that parses a new value into the
// target and echoes it.

template <> void funcholder<int*>::Engage(char* args) {
    returnvalue[0] = 0;
    static_cast<void>(sscanf(args, "%d", m_func));
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%d", *m_func);
}
template <> void funcholder<float*>::Engage(char* args) {
    returnvalue[0] = 0;
    static_cast<void>(sscanf(args, "%f", m_func));
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f", *m_func);
}
template <> void funcholder<Vector*>::Engage(char* args) {
    returnvalue[0] = 0;
    static_cast<void>(sscanf(args, "%f %f %f", &m_func->x, &m_func->y, &m_func->z));
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f %f %f", m_func->x, m_func->y, m_func->z);
}

template <> void funcholder<Quaternion*>::Engage(char* args) {
    returnvalue[0] = 0;
    float yaw = Yaw(*m_func);
    float pitch = Pitch(*m_func);
    float roll = Roll(*m_func);
    static_cast<void>(sscanf(args, "%f %f %f", &yaw, &pitch, &roll));
    *m_func = YawPitchRoll(yaw, pitch, roll);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f %f %f", yaw, pitch, roll);
}

// Data-pointer PreEngage prints the pointed-to value into the return buffer.

template <> void funcholder<int*>::PreEngage(char* args) {
    K2_UNUSED(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%d", *m_func);
}
template <> void funcholder<float*>::PreEngage(char* args) {
    K2_UNUSED(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f", *m_func);
}
template <> void funcholder<Vector*>::PreEngage(char* args) {
    K2_UNUSED(args);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f %f %f", m_func->x, m_func->y, m_func->z);
}
template <> void funcholder<Quaternion*>::PreEngage(char* args) {
    K2_UNUSED(args);
    returnvalue[0] = 0;
    float yaw = Yaw(*m_func);
    float pitch = Pitch(*m_func);
    float roll = Roll(*m_func);
    FormatReturnValue(reinterpret_cast<char*>(returnvalue), 1024, "%f %f %f", yaw, pitch, roll);
}

// The deleting destructor for the data-pointer element types.

template funcholder<int*>::~funcholder();
template funcholder<float*>::~funcholder();
template funcholder<Vector*>::~funcholder();
template funcholder<Quaternion*>::~funcholder();

// The non-template base has a vptr, a virtual destructor and the two virtual entry points, all
// trivial.

funcholderbase::~funcholderbase() {}
void funcholderbase::Engage(char* args) { K2_UNUSED(args); }
void funcholderbase::PreEngage(char* args) { K2_UNUSED(args); }
