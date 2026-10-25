// The in-game key mapping page and the list rows it owns.
#pragma once
#include "k2stdint.h"

#ifndef CSWGUIINGAMEOPTKEYMAPPINGS_H
#define CSWGUIINGAMEOPTKEYMAPPINGS_H

// The in-game "Key Mapping" page, pushed by CSWGuiInGameGameplay::OnKeyMap. It owns one row per
// bindable input event, filtered by the current page, and a toolbar of accept, cancel, default plus
// three page-filter buttons.
#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"

class CSWGuiManager;
class CSWGuiKeyMapButton;

class CSWGuiInGameOptKeyMappings : public CSWGuiPanel {
public:
    CSWGuiInGameOptKeyMappings(CSWGuiManager* pGuiManager);

    virtual ~CSWGuiInGameOptKeyMappings();
    virtual void Update(float f);
    virtual void HandleInputEvent(int type, int val);

    // The row factory. nEventClass selects the form GetActionId produces.
    CSWGuiKeyMapButton* CreateOption(int nActionId, CSWGuiButton* pTemplate, int nEventClass);
    void FillListWithEvents();
    void FillListBox(int nPage);
    int  UpdateEvent(int nActionId, int nEventClass, unsigned int nKeyCode,
                     unsigned int nInputClassMask);
    int  IsKeyUsedAlready(unsigned int nKeyCode, int nActionId, int nEventClass,
                          unsigned int nInputClassMask);
    void SetCaptureEvent(CSWGuiKeyMapButton* pButton);
    // Starts or stops raw keyboard capture for a rebind.
    void SetCaptureKeyboardMode(int bCapture);

    void OnDefaultClick(CSWGuiControl* pControl);
    void OnAcceptClick(CSWGuiControl* pControl);
    void OnFilterMove(CSWGuiControl* pControl);
    void FixFilterColors(CSWGuiControl* pControl);
    void OnFilterGame(CSWGuiControl* pControl);
    void OnFilterMini(CSWGuiControl* pControl);

    static const int FILTER_MOVE = 0;
    static const int FILTER_GAME = 1;
    static const int FILTER_MINI = 2;

    CExoArrayList<CSWGuiControl*> m_aOptions;
    CSWGuiLabel  m_lblTitle;
    CSWGuiButton m_btnDefault;
    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnCancel;
    CSWGuiButton m_btnFilterMove;
    CSWGuiButton m_btnFilterGame;
    CSWGuiButton m_btnFilterMini;
    CSWGuiListBox m_listBox;
#if !KOTOR1  // K1 has no LBL_BAR labels
    CSWGuiLabel  m_aBars[3];
#endif
    int m_nFilterMode;
    int m_bCapturing;
    CSWGuiKeyMapButton* m_pCaptureEvent;
};

#endif // CSWGUIINGAMEOPTKEYMAPPINGS_H

#ifndef CSWGUIKEYMAPBUTTON_H
#define CSWGUIKEYMAPBUTTON_H

// One row of the in-game key-mapping list: the action's caption on the left, from the inherited
// button text, plus an embedded button showing the key currently bound to the action.
class CSWGuiControl;
struct CSWGuiExtent;
class CSWGuiInGameOptKeyMappings;
class CExoString;

class CSWGuiKeyMapButton : public CSWGuiButton {
public:
    CSWGuiKeyMapButton(CSWGuiInGameOptKeyMappings* pOwner);

    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual void Draw(float f);
    virtual CSWGuiControl* HitCheckMouse(int x, int y);

    // Formats the action name; the event class selects the form.
    CExoString GetActionId(int nEventClass, int nActionId);
    void SetDescription(const CExoString& sActionName);
    void SetEvents();
    void UpdateColor();
    void ResetToDefault();
    void PostChangeToEvent();
    void SetEditing(int bEditing);
    // Locks the row: it is drawn in the disabled colours and accepts no rebind.
    void SetUnchangable();
    // Returns KEY_INVALID when the key is not bindable, KEY_GOOD when it was applied, and
    // KEY_ALREADYUSED when another action already uses it.
    int  UpdateEvent(unsigned int nKey);
    void onControlEntered(CSWGuiControl* pControl);
    void onControlExit(CSWGuiControl* pControl);
    void onEventClicked(CSWGuiControl* pControl);

    static const int KEY_INVALID = 2;
    static const int KEY_ALREADYUSED = 1;
    static const int KEY_GOOD = 0;
    static const int IC_PC = 1;
    static const int IC_MINIGAME = 2;
    static const int IC_GUI = 4;
    static const int IC_DIALOG = 8;
    static const int IC_FREELOOK = 16;
    static const int IC_MOVIE = 32;

    uint64_t m_nDescStrRef;
    CSWGuiButton m_btnKey;
    CSWGuiInGameOptKeyMappings* m_pOwner;
    int m_nActionId;
    int m_nEventClass;
    int m_bChanged;
    int m_nKeyCode;
    int m_nDefaultKey;
    int m_bDisabled;
    unsigned int m_nInputClassMask;
    int m_nPage;
    int m_nReserved1;
    int m_nSortPos;
};

#include "exoinput.h"
#include "guiingame.h"

inline void CSWGuiInGameOptKeyMappings::SetCaptureKeyboardMode(int bCapture)
{
    m_bCapturing = bCapture;
    g_pExoInput->SetCaptureKeyboardKeys(bCapture);
}

inline void CSWGuiKeyMapButton::SetUnchangable()
{
    m_bDisabled = 1;
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
}

#endif // CSWGUIKEYMAPBUTTON_H
