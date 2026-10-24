#ifndef AURIO_H
#define AURIO_H

#include "List.h"

class MaxTree;
class AsyncEntry;
#include "port/libcbase.h"
#if KOTOR1
#include "sdlrwops.h"
#endif

// The model I/O singleton, which the .mdl/.max readers and writers all go through.
class IODispatcher {
public:
    IODispatcher();
    ~IODispatcher();
    static IODispatcher* GetRef();
    bool Write(MaxTree* pTree, char* pszName);
#if KOTOR1  // K1: the stream flavours take SDL_RWops
    bool Write(MaxTree* pTree, SDL_RWops* pFile, SDL_RWops* pOut);
    MaxTree* ReadSync(char* pszName);
    MaxTree* ReadSync(SDL_RWops* pFile, SDL_RWops* pOut);
#else
    bool Write(MaxTree* pTree, FILE* pFile, FILE* pOut);
    MaxTree* ReadSync(char* pszName);
    MaxTree* ReadSync(FILE* pFile, FILE* pOut);
#endif
    AsyncEntry* PreSpawnAsync(char* pszName);
    MaxTree* RetreiveAsync(void* pHandle);

    List<AsyncEntry*> m_asyncList;
};

#endif
