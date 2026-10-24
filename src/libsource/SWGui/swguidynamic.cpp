#include "swguidynamic.h"

#include "swguicontrol.h"
#include "resgff.h"
#include "exoresman.h"
#include "exostring.h"

CSWGuiDynamic::CSWGuiDynamic(CSWGuiManager* pGuiManager, const CResRef& sLayout)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(sLayout, 1, 1, 1);

    CResGFF* pGFF = m_pLayoutRes;
    CExoString sTag;
    int nCount = pGFF->GetListCount(&m_layoutList);
    for (int i = 0; i < nCount; i++)
    {
        int bSuccess;
        CResStruct sStruct;
        pGFF->GetListElement(&sStruct, &m_layoutList, i);

        unsigned int nControlType =
            pGFF->ReadFieldDWORD(&sStruct, const_cast<char*>("CONTROLTYPE"), bSuccess, 0);
        sTag = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("TAG"), bSuccess, CExoString(""));

        CSWGuiControl* pControl;
        switch (nControlType)
        {
        case SWGUICONTROL_LABEL:         pControl = new CSWGuiLabel; break;
        case SWGUICONTROL_LABEL_HILIGHT: pControl = new CSWGuiLabelHilight(); break;
        case SWGUICONTROL_BUTTON:        pControl = new CSWGuiButton(); break;
        case SWGUICONTROL_BUTTON_TOGGLE: pControl = new CSWGuiButtonToggle; break;
        case SWGUICONTROL_SLIDER:        pControl = new CSWGuiSlider; break;
        case SWGUICONTROL_SCROLLBAR:     pControl = new CSWGuiScrollBar; break;
        case SWGUICONTROL_PROGRESSBAR:   pControl = new CSWGuiProgressBar; break;
        case SWGUICONTROL_LISTBOX:       pControl = new CSWGuiListBox; break;
        default: continue;
        }

        InitControl(*pControl, sTag, 1, 1, 0, 0, 0);
    }

    StopLoadFromLayout();
#if KOTOR1  // K1 scales the panel to the screen
    ScaleExtentForResolution();
#endif
}

CSWGuiDynamic::~CSWGuiDynamic()
{
    int nCount = m_controls.m_count;
    for (int i = 0; i < nCount; i++)
    {
        if (m_controls.m_pArray[i] != 0)
            delete m_controls.m_pArray[i];
    }

    m_controls.Clear();
}
