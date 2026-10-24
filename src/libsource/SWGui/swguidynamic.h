// Layout-backed in-game gui; only the `showgui`/`hidegui` commands build one.
#ifndef CSWGUIDYNAMIC_H
#define CSWGUIDYNAMIC_H

#include "swgui.h"

class CSWGuiManager;
class CResRef;

class CSWGuiDynamic : public CSWGuiPanel {
public:

    CSWGuiDynamic(CSWGuiManager* pGuiManager, const CResRef& sLayout);
    virtual ~CSWGuiDynamic();  // slots 0-1 (D2+D0)
};

#endif // CSWGUIDYNAMIC_H
