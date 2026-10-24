#ifndef AURRES_H
#define AURRES_H

// The console's resource reader (aurconsole.cpp) opens a packaged resource or a file, walks it a
// line or a block at a time, and frees it. The handle is an OpenResource the callers keep opaque.
class OpenResource;

OpenResource*  AurResGet(char* pszName, char* pszExt, int* pnSize, bool bNoFile);
char*          AurResGetNextLine();
unsigned char* AurResGetDataBytes(unsigned int nBytes, void* pRes);
unsigned char* AurResGetData(int nBytes);
void           AurResFreeDataBytes(void* pRes, unsigned char* pBytes);
void           AurResFree(void* pRes, int nSize);
void           AurResFree();
void*          GetResource(void* pRes);
void           AurResRemove(void* pRes);
#if KOTOR1
struct SDL_RWops;
OpenResource*  AurResGet(SDL_RWops* pFile);
#else
#include "port/libcbase.h"
OpenResource*  AurResGet(FILE* pFile);
#endif

#endif // AURRES_H
