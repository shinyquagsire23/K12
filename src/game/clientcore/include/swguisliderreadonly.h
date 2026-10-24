// A slider the player cannot drag: a left mouse-down does not move it.

#ifndef CSWGUISLIDERREADONLY_H
#define CSWGUISLIDERREADONLY_H

#include "swguicontrol.h"

class CSWGuiSliderReadOnly : public CSWGuiSlider {
public:

    virtual void HandleLMouseDown();
};

#endif
