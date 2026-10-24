#include "swguiexamine.h"
#include "appmanager.h"
#include "swgui.h"
#include "guiingame.h"
#include "strref.h"

CSWGuiExamine::CSWGuiExamine(CSWGuiManager* pGuiManager)
    : CSWGuiMessageBox(pGuiManager)
{
    m_btnOk.m_text.m_params.SetStrRef(STRREF_CLOSE);
    SetAllowCancel(0);
    m_examineMode = 0;
}

// Any of the four dismissal inputs closes the box; anything else, including a zero value,
// falls through to the base handler.
void CSWGuiExamine::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
        switch (nType)
        {
            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->HideExamineBox();
                return;
        }
    }

    CSWGuiMessageBox::HandleInputEvent(nType, nValue);
}
