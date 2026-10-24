#ifndef CSWGUIINGAMEOPTIONS_H
#define CSWGUIINGAMEOPTIONS_H
#include "k2stdint.h"

// The in-game options page, the panel CGuiInGame pops as m_apScreenPanels[7].

#include "swgui.h"

#include "swguicontrol.h"

class CSWGuiManager;

class CSWGuiInGameOptions : public CSWGuiPanel {
public:
    CSWGuiInGameOptions(CSWGuiManager* pGuiManager);

    // The page openers. Each gates on the control's event argument, allocates the page, hands it the
    // shared "bg_replacement" extra border and pushes it with the (3, 1) panel flags.
    void OnLoadGame(CSWGuiControl* pControl);
    void OnSaveGame(CSWGuiControl* pControl);
    void OnGameplay(CSWGuiControl* pControl);
    void OnFeedback(CSWGuiControl* pControl);
    void OnAutopause(CSWGuiControl* pControl);
    void OnGraphics(CSWGuiControl* pControl);
    void OnSound(CSWGuiControl* pControl);
    // OnQuit reuses the panel's shared confirmation box rather than allocating a new one.
    void OnQuit(CSWGuiControl* pControl);

    // The hover-description handler every wall button binds at event 0.
    void OnEnterSlot(CSWGuiControl* pControl);
    // An eight-button wall, unlike CSWGuiOptionsMain's six.
    void SetActiveButton(int nIndex);
    // As the message box's callback, reads the box's dismissal result back and only then ends the
    // game.
    void ReallyQuit(CSWGuiControl* pControl);
    void SetDescription(uint64_t nDescription);

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();

    CSWGuiButton  m_aButtons[8];      // BTN_LOADGAME / BTN_SAVEGAME / BTN_GAMEPLAY / BTN_FEEDBACK /
                                      // BTN_AUTOPAUSE / BTN_GRAPHICS / BTN_SOUND / BTN_QUIT
#if KOTOR1
    // K1 puts this untagged button right after the eight-button wall, ahead of the labels, and has
    // no bar labels.
    CSWGuiButton  m_btnUnbound;
#endif
    CSWGuiLabel   m_lblTitle;         // "LBL_TITLE"
    CSWGuiLabel   m_lblDesc;          // the single row the LB_DESC box carries
    // Four pad-glyph labels in the same family as LBL_TITLE, named after the Switch layout's
    // LBL_A / LBL_SELECT / LBL_START / LBL_EXIT; nothing initialises or reads them.
    CSWGuiLabel   m_lblPadA;
    CSWGuiLabel   m_lblPadSelect;
    CSWGuiLabel   m_lblPadStart;
    CSWGuiLabel   m_lblPadExit;
    // "LBL_RANALOG", the right-stick glyph. SetDescription is its only driver: it sets the visible
    // bit from the LB_DESC box's scroll state.
    CSWGuiLabel   m_lblRandAnalog;
    CSWGuiListBox m_listBoxDesc;      // "LB_DESC"
#if !KOTOR1
    // Built by the constructor and never touched again: no control tag, no event, no reader.
    CSWGuiButton  m_btnUnbound;
#endif
    CSWGuiButton  m_btnExit;          // "BTN_EXIT" (also gets the pad's 'b' glyph)
#if !KOTOR1
    CSWGuiLabel   m_aBarLabels[5];    // "LBL_BAR1" .. "LBL_BAR5"
#endif
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameOptions) == 0x31a0, "CSWGuiInGameOptions size drift");
#endif

#endif // CSWGUIINGAMEOPTIONS_H
