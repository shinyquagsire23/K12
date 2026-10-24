// CSWGuiInGameAutoPause -- the "Auto-Pause" page of the in-game options menu. Its single layout
// resref is "optautopause_p".

#ifndef CSWGUIINGAMEAUTOPAUSE_H
#define CSWGUIINGAMEAUTOPAUSE_H
#include "k2stdint.h"

#include "swgui.h"

#include "swguicontrol.h"
#include "swguioptions.h"

// Owns the six auto-pause checkboxes, the description list box with its prototype label, the title
// label, the Back and Default buttons, and the four bar labels the options screens keep.
class CSWGuiInGameAutoPause : public CSWGuiPanel {
public:
    CSWGuiInGameAutoPause(CSWGuiManager* pGuiManager);
    virtual ~CSWGuiInGameAutoPause();

    // None of these are virtual.
    void OnEnterSlot(CSWGuiControl* pControl);
    void OnDefault(CSWGuiControl* pControl);
    // Re-syncs every row from CClientOptions.
    void SetupOptions();
    void SaveOptions();
    void ClosePanel();
    void SetDescription(uint64_t nDescription);

    virtual void HandleInputEvent(int type, int val);
#if !KOTOR1
    virtual bool ShouldFreeMouseOnAdd();
#endif

    enum {
        kCbEndRound = 0, kCbEnemySighted, kCbMineSighted,
        kCbPartyKilled, kCbActionMenu, kCbTriggers
    };
    CSWGuiOptionsCheckbox m_aCheckbox[6];
    // The list box's prototype row, never InitControl'd.
    CSWGuiLabel           m_labelDesc;
    CSWGuiLabel           m_labelTitle;
    CSWGuiButton          m_btnBack;
    CSWGuiButton          m_btnDefault;
#if !KOTOR1
    // K1 has no bar labels: its list box follows the default button directly.
    CSWGuiLabel           m_aLabelBar[4];
#endif
    CSWGuiListBox         m_listBoxDetails;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameAutoPause) == 0x2970, "CSWGuiInGameAutoPause size drift");
#endif

#endif // CSWGUIINGAMEAUTOPAUSE_H
