#ifndef CEXORESMANINTERNAL_H
#define CEXORESMANINTERNAL_H
#include "k2stdint.h"

// Private to CExoResMan.cpp: the Win32-on-Android shims, the bionic struct stat and shortcut views of
// the resource-table types.

#include "exoresman.h"
#include "CExoLinkedList.h"
#include "exolinkedlistinternal.h"
#include "exobase.h"
#include "exostringlist.h"
#include "exolocstring.h"

#include "port/win32api.h"

#include "exofile.h"

// Module-override directory helpers used by AsyncLoad; defined in servercore.
void AddModuleOverrideResourceDirectories();
void RemoveModuleOverrideResourceDirectories();
const char* GetOverridePath(int index, bool flag);

uint64_t AsyncRimLoadThreadProc(void* pParam);
#include "exoresgffread.h"

#include "port/libcstat.h"
// remove stays declared by <stdio.h>; redeclaring it with throw() conflicts with the NDK prototype.

#include "CExoArrayListDtors.h"

#include "exoresfile.h"

#endif
