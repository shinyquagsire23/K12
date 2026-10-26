#pragma once
#include "k2stdint.h"

class CResGFF;
struct CResStruct;

// A GUI object's screen rectangle, its position and size in the GUI's design coordinates.
struct CSWGuiExtent {  // idiom-exempt: canonical
    int x;
    int y;
    int w;
    int h;

    CSWGuiExtent() { x = 0; y = 0; w = 0; h = 0; }

    CSWGuiExtent(int x_, int y_, int w_, int h_) { x = x_; y = y_; w = w_; h = h_; }

    void Load(CResGFF* pGFF, CResStruct& resStruct);

    // Overlap of the two rectangles.
    static CSWGuiExtent Intersection(const CSWGuiExtent& a, const CSWGuiExtent& b);

    // Smallest rectangle containing both.
    static CSWGuiExtent Union(const CSWGuiExtent& a, const CSWGuiExtent& b);
};

// The abstract interface at the root of the CSWGui* widget hierarchy, with the shared widget slots.
// CSWGuiControl and CSWGuiPanel derive from it and add their own slots, and the screen extents
// live here rather than in the derived classes.

class CSWGuiControl;
class CSWGuiPanel;

class CSWGuiObject {
public:
    // Zeroes both extents in the order the layout was built with; an implicit default constructor
    // would zero them in declaration order instead.
#if KOTOR1
    // K1 has no m_originalExtent: it records the applied scale in a byte instead.
    CSWGuiObject() { m_bScaledForResolution = 0; m_extent = CSWGuiExtent(); }
#else
    CSWGuiObject() { m_originalExtent = CSWGuiExtent(); m_extent = CSWGuiExtent(); }
#endif
    virtual ~CSWGuiObject() {}
#if KOTOR1  // K1 declares it out of line
    virtual void SetExtent(const CSWGuiExtent& extent);
#else
    virtual void SetExtent(const CSWGuiExtent& extent) { m_extent = extent; }
#endif
    virtual void SetActiveControl(CSWGuiControl* pControl, int flag) { K2_UNUSED(pControl); K2_UNUSED(flag); }
    virtual void HandleMouseOver(int bMouseIsOver) { K2_UNUSED(bMouseIsOver); }
    virtual int  HandleMouseCapturedMovement(int a, int b) { K2_UNUSED(a); K2_UNUSED(b); return 0; }
    virtual void HandleLoseMouseFocus() {}
    virtual void HandleLMouseDown() {}
    virtual void HandleLMouseUp() {}
    virtual void HandlsRMouseDown() {}   // sic: engine typo, not "Handle"
    virtual void HandleRMouseUp() {}
#if KOTOR1
    // K1-only: rescale m_extent against the screen height.
    virtual void ScaleExtentForResolution();
    virtual void ScaleExtentForResolution(float fScale);
#endif
    virtual CSWGuiPanel* AsSWGuiPanel() { return 0; }
    virtual CSWGuiControl* AsSWGuiListBox() { return 0; }
    virtual CSWGuiControl* AsSWGuiControl() { return 0; }
#if KOTOR1
    unsigned char m_bScaledForResolution;
    CSWGuiExtent m_extent;
#else
    virtual void SetOriginalExtent(const CSWGuiExtent& extent) { m_originalExtent = extent; }

    CSWGuiExtent m_extent;
    CSWGuiExtent m_originalExtent;
#endif
};

// The GUI manager, which owns the panel and modal stacks, mouse capture, tooltips and the software
// mouse cursor. Partial model; only the members the engine code touches are named.

#include "CExoArrayListDtors.h"
#include "exostring.h"
#include "exoresman.h"

class CAurObject;
class CExoSoundSource;
class CExoString;
class CSWGuiControl;
class CSWGuiScene;
class CSWGuiEditbox;
class CSWGuiLabel;
class CSWGuiObject;
class CSWGuiPanel;
class CSWGuiToolTipPanel;
struct Vector;

// Free-look toggle, called when a panel that frees the mouse goes up. Two overloads rather than a
// default argument: AddPanel calls the two-argument one, RemovePanel the one-argument one.
void EnableMouseCameraRotateToggle(bool bEnable, int nMode);
void EnableMouseCameraRotateToggle(bool bEnable);

bool AllowMouseLookToggle();

extern int g_nScreenWidth;
extern int g_nScreenHeight;

extern char* g_asMouseCursorNames[];

extern unsigned char g_nBitsPerPixel;
extern int g_nDesiredFullScreenRefreshRate;

extern int g_nGUIWidth;
extern int g_nGUIHeight;
extern int g_nUIEdgeOffset;

// Recomputes the letterboxed native resolution globals; SetSize calls it before caching a new
// screen size.
void ScaleNativeResolution();

// Whether the port runs in its tablet layout; the edge inset is forced to 0 when it does.
bool IsTablet();

extern int g_nScaledNativeWidth;
extern int g_nScaledNativeHeight;
extern int g_nScreenLeftOffset;
extern int g_nScreenTopOffset;

// Mouse position, in screen pixels.
struct CSWGuiMousePos {
    int x;
    int y;
};

// The rows of guisounds.2da CSWGuiManager::PlayGuiSound plays (m_paGuiSounds). NONE, in a
// control's click, activate or scroll sound, plays nothing.
enum {
    GUISOUND_NONE                     = -1,
    GUISOUND_CLICKED_DEFAULT          = 0,
    GUISOUND_ENTERED_DEFAULT          = 1,
    GUISOUND_ERROR_DEFAULT            = 2,
    GUISOUND_CHECKBOX_CHECK           = 3,
    GUISOUND_FULLSCREEN_PANEL_ADDED   = 4,
    GUISOUND_FULLSCREEN_PANEL_REMOVED = 5,
    GUISOUND_ACTION_MENU_USE          = 6,
    GUISOUND_ACTION_MENU_SCROLL       = 7,
    GUISOUND_ACTION_MENU_QUEUE        = 8,
    GUISOUND_INVENTORY_ADD            = 9,
    GUISOUND_INVENTORY_SELECT         = 10,
    GUISOUND_INVENTORY_DROP           = 11,
    GUISOUND_LEVEL_UP_NOTIFY          = 12,
    GUISOUND_QUEST_NEW_NOTIFY         = 13,
    GUISOUND_QUEST_COMPLETE_NOTIFY    = 14,
    GUISOUND_PROMPT                   = 15,
    GUISOUND_UPGRADE                  = 16,
    GUISOUND_DARK_SIDE                = 17,
    GUISOUND_LIGHT_SIDE               = 18,
};

class CSWGuiImage;
class CSWGuiManager {
public:
#if KOTOR1
    // K1 has an extra leading byte, so the members below sit higher than in K2.
    unsigned char  m_nReserved1;
    CSWGuiMousePos m_currentMousePos;
    char           _padc[0x10-0xc];
#else
    CSWGuiMousePos m_currentMousePos;
#endif
    CSWGuiControl* m_pMouseOverControl;
    int            m_nCaptureFlags;
    char           _pad14[0x18-0x14];
    CSWGuiObject*  m_pMouseCaptureObject;
    CSWGuiControl* m_pLastMouseOverControl;
    CSWGuiEditbox* m_pFocusedEditbox;
    unsigned char  m_nMouseStateFlags;
    char           _pad31[0x38-0x31];
    CSWGuiScene*   m_pSoftwareMouse;
    CAurObject*    m_pMouseCursorModel;
    CResRef        m_mouseCursorRes;
    int            m_nSoftwareMouseCursor;
    char           _pad5c[0x60-0x5c];
    CSWGuiToolTipPanel* m_pToolTipPanel;
    CSWGuiControl* m_pToolTipControl;
    float          m_fToolTipTimer;
    float          m_fToolTipTimeDelay;
    int            m_bToolTipDisabled;
    char           _pad7c[0x80-0x7c];
    CSWGuiControl* m_pPendingToolTipControl;
    CSWGuiControl* m_pDragDropControl;
    CSWGuiMousePos m_storedMousePos;
    float          m_fDeltaTStore;
    int            m_nRepeatFilterTime;
    int            m_nCurrentInputEvent;
    short          m_nScreenWidth;
    short          m_nScreenHeight;
    unsigned char  m_bSuppressCameraRender;
    unsigned char  m_nCapturedPanelCount;
    unsigned char  m_nRepeatFilterEvent;
    char           _padab[0xb0-0xab];
    CExoString     m_fullScreenBG;
    CSWGuiLabel*   m_pBackgroundLabel;
    CSWGuiPanel*   m_pSafeRegionPanel;
    CSWGuiLabel*   m_pSafeRegionLabel;
    CExoArrayList<CSWGuiPanel*> m_panels;
    CExoArrayList<CSWGuiPanel*> m_modalPanels;
    CExoSoundSource** m_paGuiSounds;
    char           m_nGuiSounds;
    char           _pad101[0x104-0x101];
    int            m_nUIEdgeInset;
    CSWGuiImage*   m_pFrameImage;
#if KOTOR1
    CSWGuiImage*   m_pMirroredFrameImage;
#endif

    static CExoString FULLSCREEN_BGS[];

    static const Vector DISABLED_TEXT;
#if KOTOR1
    static Vector COLOR_YELLOW;
#endif

    CSWGuiManager();
    ~CSWGuiManager();

    int  GetPosInPanelList(CSWGuiPanel* pPanel);
    int  PanelExists(CSWGuiPanel* pPanel);
    // nFlags: bit 0 pushes the panel onto the modal stack, bit 1 marks it opaque/full-screen.
    void AddPanel(CSWGuiPanel* pPanel, int nFlags, int bPlaySound);
    // Returns 1 when the panel was really removed, 0 on each rejection path.
    int  RemovePanel(CSWGuiPanel* pPanel);
    // The panel z-order quartet; each variant differs only in its destination index.
    void SendPanelToBack(CSWGuiPanel* pPanel);
    void BringPanelToFront(CSWGuiPanel* pPanel);
    void BringPanelForward(CSWGuiPanel* pPanel);
    void SendPanelBack(CSWGuiPanel* pPanel);
    // Returns 1 on a real push, 0 for a null panel or one already on the modal stack.
    int  PushModalPanel(CSWGuiPanel* pPanel);
    // Returns the popped panel, or null when the modal stack was already empty.
    CSWGuiPanel* PopModalPanel();
    void Update(float deltaTime);
    void Draw(float deltaTime);
    // Draws the cached full-screen background label, re-tinting it every frame.
    void DrawPCBG(float fAlpha);
    // Draws the safe-region overlay panel and the label it holds.
    void DrawSafeRegion(float fAlpha);
    void DetermineNeedToDraw();
    // Caches a new physical screen size and re-lays-out everything that depends on it.
    void SetSize(int nWidth, int nHeight);

    void CaptureMouse(CSWGuiObject* pControl, int flag);
    void ReleaseMouse(CSWGuiObject* pControl);
    int  HandleLMouseUp();
    int  HandleLMouseDown(int aMode);
    int  HandleRMouseUp();
    int  HandleRMouseDown();
    // Returns 1 when the software-mouse model was created, 0 when AddModel failed.
    int  ActivateSoftwareMouse();
    void ShowSoftwareMouse();
    void HideSoftwareMouse();
    void DeactivateSoftwareMouse();
    void ClearPanels();
    void HideAllPanels();
    void ShowAllPanels();
    int  IsOnTop(CSWGuiPanel* pPanel);
    // Rescales every panel in both stacks, each background border and each nested panel.
    void ReloadPanels(int nWidth, int nHeight);
    int  HitCheckMouse(int x, int y, CSWGuiPanel** ppPanel, CSWGuiControl** ppControl,
                       int nUnused);
    // Fills the GUI sound table from guisounds.2da; returns 0 if the 2DA fails to load.
    int  LoadGuiSounds();
    void UnloadGuiSounds();
    int  GetPosInModalStack(CSWGuiPanel* pPanel);
    void ChangeToolTipText(const CExoString& text);
    void UpdateMouseOverControl();
    void SetTemporarilyDisableToolTip(int bDisable);
    // Destroys the current tooltip panel and builds a fresh one.
    void ReloadToolTipPanel();
    // Picks the "WxH" name for a resolution off the height alone.
    static CExoString GetScreenResolutionString(unsigned int nWidth, unsigned int nHeight);
    void ResetToolTipTimer();
    void RemoveToolTip();
    void DisplayToolTip(const CExoString& text);
    void PlayGuiSound(char sound);
    void UpdateAllFonts();

    // Snapshots the mouse position, but only when the client option is on.
    void StoreMouseForMouseTeleportingToButtons();
    void RestoreOldMousePosition();
    // The same restore, gated on the client option.
    void RestoreMouseForMouseTeleportingToButtons();
    void MoveMouseToPosition(int x, int y);
    void HandleMouseMove(int x, int y);
    // Replays the last mouse position, so hover state follows a panel change.
    void UpdateMouseMove() { HandleMouseMove(m_currentMousePos.x, m_currentMousePos.y); }
    void HandleInputEvent(int a, int b);
    // Selects a software-mouse cursor. nCursor indexes g_asMouseCursorNames into
    // m_mouseCursorRes and is cached in m_nSoftwareMouseCursor; nX is the cursor-shown state;
    // bRound rounds an even cursor index before the lookup.
    void SetSoftwareMouseCursor(int nCursor, int nX, int bRound);
    int  HandleZMouse(int zDelta);

    void StoreCurrentMousePosition();
    void GetSafeMargin(int& xMargin, int& yMargin);

    int  HandleKeyPress(int nKey);
    // Appends the client-options font-size suffix to a font name in place.
    void GetUpdatedFontName(char* szFontName);
    void GenerateAspectMultipliers(int bUseNativeRect, float& fXMult, float& fYMult,
                                   int& nXOffset, int& nYOffset,
                                   float fBaseWidth, float fBaseHeight) const;
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiManager) == 0x120 && __builtin_offsetof(CSWGuiManager, m_nScreenWidth) == 0xac &&
              __builtin_offsetof(CSWGuiManager, m_panels) == 0xe0, "CSWGuiManager K1 layout (ctor 0x6266e0)");
#endif

// A GUI panel, a screen or dialog holding controls.

#include "Vector.h"
#include "resgff.h"

class CRes;
class CResGFF;
class CResRef;
class CSWGuiBorder;
class CSWGuiControl;
struct CSWGuiExtent;
class CSWGuiManager;

// The Aurora GUI layer API Draw brackets its control walk with. AurGUISetupViewport reports
// whether the rectangle survived clipping, and the viewport is closed only when it did.
void AurGUIStartLayer();
void AurGUIStopLayer();
int  AurGUISetupViewport(int x, int y, int w, int h, const Vector& v, bool b, float f);
void AurGUICloseViewport();

// CSWGuiPanel::m_stateFlags. A panel's extent is in screen pixels (NATIVE_COORDS) or in the 640x480
// frame centred on the screen; CENTER_X and CENTER_Y centre that frame on one axis. SHOWN puts the
// panel in the manager's draw pass and DRAW is what that pass leaves set on the panels it draws.
// FULLSCREEN covers the screen: it suppresses the camera render and supplies the full-screen
// background (KOTOR 1 also centres the panel on it). The removal request, which CSWGuiManager
// carries out after the frame, removes the panel or removes and deletes it; DEFERRED leaves the
// request to the draw and input sweeps. KOTOR 1 keeps the bits from CENTER_X up one higher, with
// NOT_CAPTURING (left out of m_nCapturedPanelCount) in the gap.
enum {
    PANEL_STATE_NATIVE_COORDS = 0x01,
    PANEL_STATE_SHOWN = 0x04,
    PANEL_STATE_FULLSCREEN = 0x08,
#if KOTOR1
    PANEL_STATE_NOT_CAPTURING = 0x10,
    PANEL_STATE_CENTER_X = 0x20,
    PANEL_STATE_CENTER_Y = 0x40,
    PANEL_STATE_DRAW = 0x80,
    PANEL_REMOVE_DEFERRED = 0x100,
    PANEL_REMOVE = 0x200,
    PANEL_REMOVE_AND_DELETE = 0x400,
#else
    PANEL_STATE_CENTER_X = 0x10,
    PANEL_STATE_CENTER_Y = 0x20,
    PANEL_STATE_DRAW = 0x40,
    PANEL_REMOVE_DEFERRED = 0x80,
    PANEL_REMOVE = 0x100,
    PANEL_REMOVE_AND_DELETE = 0x200,
#endif
    PANEL_REMOVE_MASK = PANEL_REMOVE | PANEL_REMOVE_AND_DELETE,
    PANEL_REMOVE_STATE_MASK = PANEL_REMOVE_DEFERRED | PANEL_REMOVE_MASK,
};

class CSWGuiPanel : public CSWGuiObject {
public:
    CSWGuiPanel(CSWGuiManager* pGuiManager);
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 30 slots
    virtual ~CSWGuiPanel();  // slots 0-1 (D2+D0)
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual void SetActiveControl(CSWGuiControl* pControl, int flag);
    virtual CSWGuiPanel* AsSWGuiPanel();
    virtual void Draw(float f);
    virtual void Update(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual CSWGuiControl* HitCheckMouse(int x, int y);
    virtual CSWGuiControl* GetActiveControl();
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();
#if !KOTOR1
    virtual bool ShouldFreeMouseOnAdd();
#endif
    virtual void OnAButtonPressed(CSWGuiControl* pControl);
    virtual void OnBButtonPressed(CSWGuiControl* pControl);
    virtual void OnXButtonPressed(CSWGuiControl* pControl);
    virtual void OnYButtonPressed(CSWGuiControl* pControl);
    virtual void OnBlackButtonPressed(CSWGuiControl* pControl);
    virtual CExoString GetFullScreenBG();
    virtual void ResetFont();
    // </VTABLE-GEN>

#if KOTOR1
    // K1 has no m_bCenterOnNativeRect, no owned m_pNextInChain and no m_bDrawOffScreen/m_bIsMainInterface.
    int            m_bAllowFontReset;
#else
    int            m_bCenterOnNativeRect;
    char           _pad2c[0x30 - 0x2c];
    CSWGuiPanel*   m_pNextInChain;
    int            m_bAllowFontReset;
    char           _pad3c[0x40-0x3c];
#endif
    CSWGuiManager* m_pGuiManager;
    CSWGuiControl* m_pActiveControl;
    CExoArrayList<CSWGuiControl*> m_controls;
    CResGFF*       m_pLayoutRes;
    CResList       m_layoutList;
    unsigned short m_stateFlags;
#if !KOTOR1
    char           _pad7e[0x80-0x7e];
    int            m_bDrawOffScreen;
#endif
    unsigned char  m_nFullScreenBGIndex;
#if KOTOR1
    char           _pad5f[1];
#else
    char           _pad85[0x88-0x85];
#endif
    float          m_fAlpha;
    Vector         m_vColor;
    CSWGuiBorder*  m_pBackground;
    char           m_nOpenSoundIndex;
#if KOTOR1
    // K1 has no explicit tail pad: a derived panel's first byte member sits in the tail.
#else
    char           _pada1[0xa4-0xa1];
    int            m_bIsMainInterface;
#endif

    // Builds the panel's controls from a layout resource; Start does the load, Stop finalizes it.
#if KOTOR1
    // K1 takes one argument; the four-argument form is a forwarder so shared call sites keep
    // K2's spelling.
    void StartLoadFromLayout(const CResRef& resRef);
    void StartLoadFromLayout(const CResRef& resRef, int, int, int) { StartLoadFromLayout(resRef); }
#else
    void StartLoadFromLayout(const CResRef& resRef, int a, int b, int c);
#endif
    void StopLoadFromLayout();
    // Binds a widget built by the active layout load to one of the panel's member controls, so
    // it must run between StartLoadFromLayout and StopLoadFromLayout.
    // K1 takes three arguments; the trailing four are Aspyr scale/offset additions, kept as a
    // forwarder so the shared call sites compile unchanged.
#if KOTOR1
    void InitControl(CSWGuiControl& control, const CExoString& sControlName, int bAddToTable);
    void InitControl(CSWGuiControl& control, const CExoString& sControlName,
                     int bAddToTable, int, int, int, int)
    {
        InitControl(control, sControlName, bAddToTable);
    }
#else
    void InitControl(CSWGuiControl& control, const CExoString& sControlName,
                     int a, int b, int c, int d, int e);
#endif
    // The overload without a layout tag; an empty control name means the widget was built by hand.
#if !KOTOR1
    void InitControl(CSWGuiControl& control, int bAddToTable, int bScale,
                     const CExoString& sControlName, int bUnused);
#endif
#if KOTOR1
    // K1-only: rescale the panel's own extent, then pass the factor down to every control.
    virtual void ScaleExtentForResolution();
    virtual void ScaleExtentForResolution(float fScale);
#endif

    // The panel's extent shifted by the letterboxing offset the manager's screen size implies.
    void GetExtentAccountingForPanelOffset(CSWGuiExtent* pExtent);

    // The mouse position relative to the panel's own top-left corner.
    void GetLocalMouseCoords(int* pX, int* pY);

    void ConnectControlMoves();
    void AddControl(CSWGuiControl* pControl);
    void ClearControls();
    // Unlinks a control from the list and clears the control's own m_pContainer back-pointer.
    void RemoveControl(CSWGuiControl* pControl);

    // Control z-order. "Front" is the end of m_controls.
    void BringControlToFront(CSWGuiControl* pControl);
    void SendControlToBack(CSWGuiControl* pControl);
    void BringControlForward(CSWGuiControl* pControl);
    void SendControlBack(CSWGuiControl* pControl);

    void SetBackground(const CResRef& resRef);
    void SetVisible(int flag);
    CSWGuiControl* GetControl(int nId);
#if !KOTOR1
    void AddExtraBorderPanel(CSWGuiManager* pGuiManager, CExoString name);
#endif
    void CenterPanel();

    // Stamps remove-and-delete on the panel and every panel chained after it; bDeferred leaves
    // the request to the draw and input sweeps.
    void SetToBeDeleted(int bDeferred)
    {
#if KOTOR1
        // K1 has no panel chain: one stamp.
        m_stateFlags = static_cast<unsigned short>(((m_stateFlags & ~PANEL_REMOVE_STATE_MASK)
                                                    | (bDeferred & 1) * PANEL_REMOVE_DEFERRED)
                                                   + PANEL_REMOVE_AND_DELETE);
#else
        CSWGuiPanel* pPanel = this;
        do
        {
            pPanel->m_stateFlags = static_cast<unsigned short>(((pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK)
                                                                | (bDeferred & 1) * PANEL_REMOVE_DEFERRED)
                                                               + PANEL_REMOVE_AND_DELETE);
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif
    }

    // Stamps a plain remove on this panel alone, unless a remove-and-delete is already pending;
    // bDeferred leaves the request to the draw and input sweeps.
    void SetToBeRemoved(int bDeferred)
    {
        unsigned short nFlags = static_cast<unsigned short>((m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                                                            | (bDeferred & 1) * PANEL_REMOVE_DEFERRED);
        if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = static_cast<unsigned short>((nFlags & ~PANEL_REMOVE_MASK) + PANEL_REMOVE);
        m_stateFlags = nFlags;
    }
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiPanel) == 0x80, "CSWGuiPanel K1 size");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiPanel, m_fAlpha) == 0x60 && __builtin_offsetof(CSWGuiPanel, m_nOpenSoundIndex) == 0x78,
              "CSWGuiPanel K1 layout (ctor 0x6245fc)");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiPanel) == 0xa8, "CSWGuiPanel size drift");
#endif
