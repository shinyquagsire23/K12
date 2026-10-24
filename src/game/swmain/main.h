#pragma once
#include "k2stdint.h"

extern char g_useModFolder;
extern int  s_numSubscribedMods;

bool IsGameModded();

char const* ClearLogCounters(char* pTag);
char const* DumpLogCounters(char* pTag);

void ChangeResolutionForMovie();
void RestoreResolutionFromMovie();
void HandleLoadError(uint64_t strRef);
