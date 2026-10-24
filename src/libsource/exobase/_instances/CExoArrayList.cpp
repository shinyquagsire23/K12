#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "exolocstring.h"
#include "exoresman.h"
#include "Vector.h"
#include "swroomsurfacemesh.h"
#include "swgui.h"
#include "exobaseinternal.h"
#include "nwccreaturestats.h"
#include "nwscreaturestats.h"
#include "nwsvartable.h"
#include "nwsjournal.h"
#include "ClientExoAppInternal.h"
#include "nwjournal.h"


// Every element type the rest of the engine instantiates.
template class CExoArrayList<unsigned long long>;

template class CExoArrayList<unsigned int>;

template class CExoArrayList<int>;

template class CExoArrayList<float>;

template class CExoArrayList<unsigned short>;

template class CExoArrayList<CExoString>;

template class CExoArrayList<CResRef>;

template class CExoArrayList<CExoLocString>;

template void CExoArrayList<CSWRoomSurfaceMeshAABBNode>::Allocate(int);

template void CExoArrayList<Vector>::Insert(Vector, int);

template void CExoArrayList<CFileInfo>::Allocate(int);
template void CExoArrayList<CFileInfo>::Insert(CFileInfo, int);

template void CExoArrayList<CSWGuiExtent>::Add(CSWGuiExtent);

template void CExoArrayList<CSWCStats_SpellLikeAbility>::Allocate(int);
template void CExoArrayList<CSWSStats_SpellLikeAbility>::Allocate(int);

template void CExoArrayList<CSWSScriptVar>::Allocate(int);

template void CExoArrayList<SJournalEntry>::Allocate(int);

template void CExoArrayList<CExoArrayList<CExoString> >::Allocate(int);

template void CExoArrayList<CGuiStringStruct>::Allocate(int);

template void CExoArrayList<CGuiStringStruct>::Insert(CGuiStringStruct, int);

template void CExoArrayList<CSWCJournalPendingUpdate>::Allocate(int);

template void CExoArrayList<CWorldJournalEntry>::Allocate(int);
