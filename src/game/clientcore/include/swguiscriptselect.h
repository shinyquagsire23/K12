#pragma once
#include "k2stdint.h"

class CSWGuiManager;
class CSWGuiControl;
class CExoString;

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"

// The DM/QA "which AI script should this creature run?" modal, with one toggle row per row of
// aiscripts.2da, a description box under the list, and Back/Accept buttons. The panel hands the
// chosen row's AI-state id to the selected creature's stats.
class CSWGuiScriptSelect : public CSWGuiPanel {
public:
    // One row of aiscripts.2da.
    struct SCRIPT_INFO {
        uint64_t m_nDescriptionStrRef;
        unsigned int  m_nAIState;
        unsigned int  _pad0c;
    };

    CSWGuiScriptSelect(CSWGuiManager* pGuiManager);

    virtual ~CSWGuiScriptSelect();

    // The first case arm falls through into the second.
    virtual void HandleInputEvent(int nType, int nArg);

    virtual void OnPanelAdded();

    // Builds one toggle row. It clones the template's extent, stamps the row's text with the
    // StrRef, then resizes it to the template's ideal height before binding the two events.
    CSWGuiButtonToggle* CreateOption(uint64_t nStrRef,
                                     void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                     CSWGuiButtonToggle* pTemplate);

    void OnEnter(CSWGuiControl* pControl);

    // Ignores the control argument; uses m_listBox's selected row instead.
    void OnScriptSelected(CSWGuiControl* pControl);

    void SetDescription(unsigned int nStrRef);

    // The invalid-object-id sentinel means "no creature".
    void SetCreatureID(unsigned long long nCreatureId);

    CExoArrayList<SCRIPT_INFO> m_aScripts;
    CSWGuiListBox  m_listBox;
    CSWGuiListBox  m_listBoxDesc;
    CSWGuiLabel    m_lblColour;
    CSWGuiLabel    m_lblTitle;
    CSWGuiButton   m_btnBack;
    CSWGuiButton   m_btnAccept;

    int                _reserved1188;
    unsigned long long m_nCreatureId;
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiScriptSelect) == 0x1198, "CSWGuiScriptSelect size drift");
#endif
