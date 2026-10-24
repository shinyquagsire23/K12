// CSWGuiQuestItem -- the quest-item list modal, a scrollable item list next to a description box.

#ifndef CSWGUIQUESTITEM_H
#define CSWGUIQUESTITEM_H

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoLinkedList.h"

class CSWGuiManager;
class CSWGuiControl;
class CSWGuiInGameItemEntry;

// A plain panel with three overridden slots, Draw, HandleInputEvent and OnPanelAdded. The two list
// boxes are not interchangeable: m_listBoxDesc holds the single description-label row and is the box
// the arrow events scroll, while m_listBox holds the item rows and is the box whose bShowScrollBar is raised.
class CSWGuiQuestItem : public CSWGuiPanel {
public:

    CSWGuiQuestItem(CSWGuiManager* pGuiManager);
    virtual ~CSWGuiQuestItem();

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    // Fills m_listBox from the player creature's item repository, one entry per plot item.
    void PopulateItemListBox();
    // Handles a hover on an item row: pushes the item's property description into the description
    // box, then sizes and re-adds the label.
    void OnControlEntered(CSWGuiControl* pControl);

    // A one-pointer handle over a heap CExoLinkedListInternal, held by value.
    CExoLinkedList<CSWGuiInGameItemEntry> m_aItemEntries;
    CSWGuiListBox  m_listBoxDesc;
    // A clone of the description box's prototype row, never InitControl'd.
    CSWGuiLabel    m_lblDescription;
    CSWGuiListBox  m_listBox;
    CSWGuiLabel    m_lblTitle;
    CSWGuiButton   m_btnBack;
    CSWGuiControl* m_pSelectedEntry;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiQuestItem) == 0xf20, "CSWGuiQuestItem size drift");
#endif

#endif // CSWGUIQUESTITEM_H
