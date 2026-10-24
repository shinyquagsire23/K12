#include "CExoArrayListDtors.h"
#include "swgui.h"
#include "swguiingameabilities.h"
#include "swguiscriptselect.h"

struct SWMGExplodeBullet;
class CSWVisibilityNode;
class CAllowedSoundProvider;
class CSWGuiButton;
class CEncounterListEntry;
class CSWMGGunBank;
class CLoopingVisualEffect;
class CExoString;
class CSWCMapPin;
class CGuiInGameFloatyText;
struct SWMGTrackInfo;
class CSWCPlaceableObjectLight;
class CAurObject;
class CSWGuiStoreItemEntry;
class CGameEffect;
class CSWGuiControl;
class CSWSExpression;
template <class T> class SafePointer;

#include "swmgtrackfollower.h"

// The trivially-destructible element types.
template CExoArrayList<unsigned int>::~CExoArrayList();
template CExoArrayList<int>::~CExoArrayList();
template CExoArrayList<float>::~CExoArrayList();
template CExoArrayList<unsigned long long>::~CExoArrayList();
template CExoArrayList<CSWGuiExtent>::~CExoArrayList();
template CExoArrayList<CSWGuiSkillStruct>::~CExoArrayList();
template CExoArrayList<SWMGFollowerModel>::~CExoArrayList();
template CExoArrayList<CSWGuiScriptSelect::SCRIPT_INFO>::~CExoArrayList();

template CExoArrayList<SWMGExplodeBullet*>::~CExoArrayList();
template CExoArrayList<CSWVisibilityNode*>::~CExoArrayList();
template CExoArrayList<CAllowedSoundProvider*>::~CExoArrayList();
template CExoArrayList<CSWGuiButton*>::~CExoArrayList();
template CExoArrayList<CEncounterListEntry*>::~CExoArrayList();
template CExoArrayList<CSWMGGunBank*>::~CExoArrayList();
template CExoArrayList<CLoopingVisualEffect*>::~CExoArrayList();
template CExoArrayList<SafePointer<CAurObject>*>::~CExoArrayList();
template CExoArrayList<CExoString*>::~CExoArrayList();
template CExoArrayList<CSWCMapPin*>::~CExoArrayList();
template CExoArrayList<CGuiInGameFloatyText*>::~CExoArrayList();
template CExoArrayList<SWMGTrackInfo*>::~CExoArrayList();
template CExoArrayList<CSWCPlaceableObjectLight*>::~CExoArrayList();
template CExoArrayList<CAurObject*>::~CExoArrayList();
template CExoArrayList<CSWGuiStoreItemEntry*>::~CExoArrayList();
template CExoArrayList<CGameEffect*>::~CExoArrayList();
template CExoArrayList<CSWGuiControl*>::~CExoArrayList();
template CExoArrayList<CSWSExpression*>::~CExoArrayList();
