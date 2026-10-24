#include "CExoLinkedList.h"

// The wrapper owns a CExoLinkedListInternal*, so the destructor never touches T and a forward
// declaration is all each element type needs.
class CAurObject;
class CAuroraModel;
class CERFKey;
class CERFRes;
class CERFString;
class CExoKeyTable;
class CExoSoundSourceInternal;
class CExoStreamingSoundSourceInternal;
class CExoString;
class CProjectileBehavior;
class CRIMRes;
class CRes;
class CSWBehaviorAttachSound;
class CSWCVisualEffectOnObject;
class CSWGuiInGameItemEntry;
class CSWSDialogPlayer;
class CSWSObjectActionNode;
class CSWSPlayerLUOInventoryItem;
struct CFileSection;
struct ExoLocString_st;
struct NWAreaExpansion_st;
struct NWModuleCutScene_st;
struct NWModuleExpansion_st;
struct NWPlayerListItem_st;

template class CExoLinkedList<CExoString>;
template class CExoLinkedList<unsigned long long>;
template class CExoLinkedList<CAurObject>;
template class CExoLinkedList<CProjectileBehavior>;
template class CExoLinkedList<CSWBehaviorAttachSound>;
template class CExoLinkedList<CSWCVisualEffectOnObject>;
template class CExoLinkedList<CSWGuiInGameItemEntry>;
template class CExoLinkedList<CSWSPlayerLUOInventoryItem>;
template class CExoLinkedList<NWPlayerListItem_st>;
template class CExoLinkedList<NWModuleCutScene_st>;
template class CExoLinkedList<NWModuleExpansion_st>;
template class CExoLinkedList<CSWSObjectActionNode>;
template class CExoLinkedList<NWAreaExpansion_st>;
template class CExoLinkedList<CSWSDialogPlayer>;
template class CExoLinkedList<CAuroraModel>;
template class CExoLinkedList<CFileSection>;
template class CExoLinkedList<ExoLocString_st>;
template class CExoLinkedList<CERFRes>;
template class CExoLinkedList<CERFKey>;
template class CExoLinkedList<CERFString>;
template class CExoLinkedList<CRes>;
template class CExoLinkedList<CExoKeyTable>;
template class CExoLinkedList<CRIMRes>;
template class CExoLinkedList<CExoSoundSourceInternal>;
template class CExoLinkedList<CExoStreamingSoundSourceInternal>;
