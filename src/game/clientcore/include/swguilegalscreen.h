// The legal-notice panel shown at intro.

#ifndef CSWGUILEGALSCREEN_H
#define CSWGUILEGALSCREEN_H

#include "swgui.h"

class CResRef;
class CSWGuiManager;

class CSWGuiLegalScreen : public CSWGuiPanel {
public:
    CSWGuiLegalScreen(CSWGuiManager* pManager, unsigned int nArg);
    int  m_fadeBusy;
    int  m_nArg;
    virtual ~CSWGuiLegalScreen();
    void SetImage(const CResRef& r);
};

#endif
