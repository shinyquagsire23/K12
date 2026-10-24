#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "swguimainmenu.h"

// The two element types with a real destructor, so both must be complete here.
template CExoArrayList<CExoString>::~CExoArrayList();
template CExoArrayList<CSWGuiMainMenu::CErrorMessageData>::~CExoArrayList();
