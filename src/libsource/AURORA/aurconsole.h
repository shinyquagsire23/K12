#pragma once
#include "k2stdint.h"

#ifndef FUNCHOLDERBASE_H
#define FUNCHOLDERBASE_H

class funcholderbase {
public:

    virtual ~funcholderbase();
    virtual void Engage(char*);
    virtual void PreEngage(char*);
};

#endif // FUNCHOLDERBASE_H

#ifndef FUNCHOLDER_H
#define FUNCHOLDER_H

extern unsigned char returnvalue[1024];

extern "C" int FormatReturnValue(char* buf, uint64_t size, const char* fmt, ...);

struct Quaternion;
float Yaw(const Quaternion&);
float Pitch(const Quaternion&);
float Roll(Quaternion);
Quaternion YawPitchRoll(float, float, float);

#include "port/libcbase.h"

// Type-erased holder for one console command: the held callable plus the Engage/PreEngage pair the
// console drives.
template <class T>
class funcholder {
public:
    virtual ~funcholder();
    virtual void Engage(char* args);
    virtual void PreEngage(char* args);

    funcholder(T f) : m_func(f) {}

    T m_func;
};

template <class T>
funcholder<T>::~funcholder() {}

template <class T>
void funcholder<T>::PreEngage(char* args) {
    K2_UNUSED(args);
    returnvalue[0] = 0;
}

#endif

#ifndef AURCONSOLE_H
#define AURCONSOLE_H

#include "List.h"

struct Vector;
struct Quaternion;

// One registered console command. The ctor self-registers with the console's sorted command table.
class ConsoleFunc {
public:
    ConsoleFunc(char* pszName, void (*func)());
    ConsoleFunc(char* pszName, void (*func)(char*));
    ConsoleFunc(char* pszName, void (*func)(int));
    ConsoleFunc(char* pszName, int (*func)(char*));
    ConsoleFunc(char* pszName, float (*func)(char*));
    ConsoleFunc(char* pszName, char* (*func)(char*));
    ConsoleFunc(char* pszName, void (*func)(char*, Vector));
    ConsoleFunc(char* pszName, void (*func)(char*, int, Quaternion));
    ConsoleFunc(char* pszName, int* pValue);
    ConsoleFunc(char* pszName, float* pValue);
    ConsoleFunc(char* pszName, Vector* pValue);
    ConsoleFunc(char* pszName, Quaternion* pValue);
    ~ConsoleFunc();

    char m_szName[80];
    funcholder<void (*)()>* m_pHolder;
};

extern ConsoleFunc* ConsoleFuncs[1024];
extern int ConsoleFuncs_num;

void ConsoleFuncs_Add(ConsoleFunc* pFunc);
void ConsoleFuncs_Remove(ConsoleFunc* pFunc);

#if KOTOR1
#include "sdlrwops.h"
#endif

// A resource open through the console's byte reader.
class OpenResource {
public:
    OpenResource(int nSize);
    ~OpenResource();
#if KOTOR1
    void SetupReadBuffer();

    SDL_RWops* m_pFile;
#else

    FILE* m_pFile;
#endif
    int   m_nLine;
    char* m_pData;
    char* m_pCursor;
    int   m_nLength;
    int   m_nSize;
    void* m_pHandle;
};

extern List<OpenResource*> Resources;
extern void (*AurReleaseResource)(void* pHandle, int nSize);
extern unsigned char* (*AurGetResource)(char* pszName, char* pszExt, int* pnSize);
extern bool g_bKeepHistory;

void  HistoryOn();
void  HistoryOff();
#include "aurres.h"

// The console's command-history ring and the cursor its up/down keys walk.
extern List<char*> g_listHistory;
extern int g_nHistoryPosition;

// Interprets one console line and returns the formatted result.
char* FuncInterp(char* pszExpr);
void* doresource(void* pUnk, int nSize);

#endif
