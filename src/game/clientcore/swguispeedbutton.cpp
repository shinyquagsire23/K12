#include "swguispeedbutton.h"

#include "swgui.h"
#include "swguicontrol.h"

// Records which button started the repeat; Draw picks the event id off that bit.
void CSWGuiSpeedButton::HandleLMouseDown()
{
    m_bRepeatLeft |= SPEEDBUTTON_REPEAT_LEFT;
    CSWGuiControl::HandleLMouseDown();
    HandleInputEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, 1);
    m_fTimeLeft = m_fInitialDelay;
}

void CSWGuiSpeedButton::HandleRMouseDown()
{
    m_bRepeatLeft &= ~SPEEDBUTTON_REPEAT_LEFT;
    CSWGuiControl::HandleRMouseDown();
    HandleInputEvent(SWGUIEVENT_SPEED_BUTTON_RIGHT, 1);
    m_fTimeLeft = m_fInitialDelay;
}

// Same repeat loop as CSWGuiListBox::Draw's scroll zones.
void CSWGuiSpeedButton::Draw(float fDelta)
{
    CSWGuiButton::Draw(fDelta);

    if (g_pGuiMan->m_pMouseCaptureObject == this)
    {
        m_fTimeLeft -= fDelta;
        while (m_fTimeLeft < 0.0f)
        {
            m_fTimeLeft += m_fStep;
            HandleInputEvent((m_bRepeatLeft & SPEEDBUTTON_REPEAT_LEFT) ? SWGUIEVENT_SPEED_BUTTON_LEFT
                                                                       : SWGUIEVENT_SPEED_BUTTON_RIGHT,
                             1);
        }
    }
}
