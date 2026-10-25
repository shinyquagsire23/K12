#include "k2stdint.h"
#include "port/sdlapi.h"
#include "exotlktable.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "exoresman.h"
#include "swguidrawable.h"
#include "guiingame.h"
#include "CExoArrayListImpl.h"
#include "port/libcmath.h"
#if !KOTOR1  // K2 only: K1 carries no CSWGuiExtent Add specialization
template<> void CExoArrayList<CSWGuiExtent>::Add(CSWGuiExtent v);
#endif

CSWGuiManager* g_pGuiMan;

extern "C" int s_nScrollPulsesShown = 0;

#include "clientexoapp.h"
#include "appmanager.h"

#include "exoinput.h"

// The record CExoInput::GetEvent returns; only the key code is read here.
struct CGuiInputEventRecord {
    unsigned char _pad00[0x20];
    int             m_keyCode;  // scan code; the "BindableKeys" row is this minus
                                // CExoInput::KEYBOARD_START
};

extern CTlkTable* g_pTlkTable;

#include "nwsrules.h"
#include "port/k2qolapi.h"

CSWGuiNavigable::CSWGuiNavigable()
    : m_moveToControls()
{
    m_tail.bFocusOnHover = 1;
}

CSWGuiControl::CSWGuiControl()
    : m_childControls()
{
    // The tail flags are split around the zeroing of the event array.
    m_pContainer = 0;
    m_events.m_pArray = 0;
    m_events.m_count = 0;
    m_events.m_capacity = 0;
    m_tail.bActive = 0;
    m_tail.bVisible = 1;
    m_tail.bEnabled = 1;
    m_tail.nControlId = -1;
    m_tail.m_nClickSound = GUISOUND_CLICKED_DEFAULT;
    m_tail.m_nActivateSound = GUISOUND_ENTERED_DEFAULT;
    m_tail.m_nUserData = 0;
    m_pParent = 0;
    m_nToolTipStrRef = 0;
    m_nBoundInputEventId = 0;
    m_tail.bReserved1 = 0;
    m_tail.bIgnoreMouse = 0;
    m_tail.bToolTipShown = 0;
}


CSWGuiControl* CSWGuiControl::AsSWGuiControl() { return this; }

int CSWGuiControl::GetIdealHeight() { return m_extent.h; }

void CSWGuiControl::HandleLoseMouseFocus() {}

void CSWGuiControl::Draw(float f) { K2_UNUSED(f); }

void CSWGuiControl::DrawDragDrop(int a, int b) { K2_UNUSED(a); K2_UNUSED(b); }

void CSWGuiControl::ReSetFont() {}

CSWGuiControl* CSWGuiControl::AsNavigable() { return 0; }

CSWGuiControl* CSWGuiControl::AsLabel() { return 0; }

CSWGuiControl* CSWGuiControl::AsLabelHilight() { return 0; }

CSWGuiControl* CSWGuiControl::AsButton() { return 0; }

CSWGuiControl* CSWGuiControl::AsButtonToggle() { return 0; }

CSWGuiControl* CSWGuiControl::AsRadioButton() { return 0; }

CSWGuiControl* CSWGuiControl::AsSlider() { return 0; }

CSWGuiControl* CSWGuiControl::AsProgressBar() { return 0; }

CSWGuiControl* CSWGuiControl::AsScrollBar() { return 0; }

CSWGuiControl* CSWGuiControl::AsListBox() { return 0; }

CSWGuiControl* CSWGuiControl::AsSkillFlow() { return 0; }

CSWGuiControl* CSWGuiControl::AsEditBox() { return 0; }

void CSWGuiControl::SetActive(int flag)
{
    m_tail.bActive = flag;
}

void CSWGuiControl::SetEnabled(int flag)
{
    m_tail.bEnabled = flag;
}

void CSWGuiControl::HideToolTip()
{
    g_pGuiMan->RemoveToolTip();
    m_tail.bToolTipShown = 0;
}

int CSWGuiControl::GetIsChild(CSWGuiControl* pControl)
{
    for (int i = 0; i < m_childControls.m_count; i++) {
        if (m_childControls.m_pArray[i] == pControl)
            return 1;
    }
    return 0;
}

CSWGuiControl* CSWGuiControl::GetSelectableParent()
{
    CSWGuiControl* pParent = m_pParent;
    while (pParent && !pParent->GetIsSelectable())
        pParent = pParent->m_pParent;
    return pParent;
}

CSWGuiControl* CSWGuiControl::HitCheckMouse(int x, int y)
{
    if ((m_tail.bVisible && !m_tail.bIgnoreMouse) == 0)
        return 0;
    if (x < m_extent.x || x > m_extent.x + m_extent.w)
        return 0;
    if (y < m_extent.y || y > m_extent.y + m_extent.h)
        return 0;
    return this;
}


void CSWGuiNavigable::SetMoveToControl(int nDir, CSWGuiControl* pControl)
{
    if (nDir >= 0 && nDir <= 3)
        m_moveToControls[nDir] = pControl;
}

CSWGuiControl* CSWGuiNavigable::GetMoveToControl(int nDir)
{
    if (nDir < 0 || nDir > 3)
        return 0;
    return m_moveToControls[nDir];
}

void CSWGuiNavigable::HandleMouseOver(int bMouseIsOver)
{
    if (bMouseIsOver) {
        if (!m_tail.bActive) {
            HandleInputEvent(SWGUIEVENT_FOCUS, 1);
#if !KOTOR1
            g_pGuiMan->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
#endif
        }
        g_pGuiMan->ResetToolTipTimer();
    } else {
        HandleInputEvent(SWGUIEVENT_UNFOCUS, 1);
        if (m_tail.bToolTipShown) {
            g_pGuiMan->RemoveToolTip();
            m_tail.bToolTipShown = 0;
        }
    }
}


void CSWGuiControl::HandleMouseOver(int bMouseIsOver)
{
    if (bMouseIsOver) {
        g_pGuiMan->ResetToolTipTimer();
        return;
    }
    if (m_tail.bToolTipShown) {
        g_pGuiMan->RemoveToolTip();
        m_tail.bToolTipShown = 0;
    }
}

int CSWGuiControl::GetIsSelectable()
{
    CSWGuiObject* pContainer = m_pContainer;
    if (pContainer && pContainer->AsSWGuiListBox()) {
        if (!pContainer->AsSWGuiListBox()->m_tail.bEnabled)
            return 0;
    } else if (!m_tail.bEnabled) {
        return 0;
    }
    return AsNavigable() != 0;
}

void CSWGuiControl::HandleFocusChange(int bGaining)
{
    if (!bGaining || !m_pContainer)
        return;
    CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
    if (!pPanel)
        return;
    if (!m_tail.bActive)
        g_pGuiMan->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    pPanel->SetActiveControl(this, 0);
}

void CSWGuiControl::HandleLMouseDown()
{
    if (!GetIsSelectable()) {
        CSWGuiControl* pParent = m_pParent;
        while (pParent) {
            if (pParent->GetIsSelectable()) {
                pParent->HandleLMouseDown();
                return;
            }
            pParent = pParent->m_pParent;
        }
        g_pGuiMan->CaptureMouse(this, 1);
        return;
    }
    g_pGuiMan->CaptureMouse(this, 1);
}

void CSWGuiListBox::HandleLMouseUp()
{
    m_nRepeatEvent = 0;

    int nIndex;
    CSWGuiControl* pControl = HitCheckMouseLocal(&nIndex);
    if (pControl && m_tail.bEnabled)
    {
        if (m_nPressedIndex == -1)
        {
            if ((bSelectOnLeftClick
                 || (m_aControls.m_count > 0 && m_aControls.m_pArray[0]->AsRadioButton()))
                && (g_pGuiMan->m_nMouseStateFlags & 1)
                && !pControl->AsScrollBar())
                HandleInputEvent(INPUTEVENT_GUI_A, 1);
        }
        else if (m_nPressedIndex == nIndex)
        {
            if (nIndex != m_nSelectedIndex)
            {
                SetSelectedControl(nIndex, 1);
                HandleInputEvent(SWGUIEVENT_SELECTION_CHANGED, 1);
            }
            else if (!bSelectOnLeftClick)
            {
                HandleInputEvent(INPUTEVENT_GUI_A, 1);
            }
        }
    }

    m_nPressedIndex = -1;
    g_pGuiMan->ReleaseMouse(this);
    m_nCapturedScrollZone = 0;
}

void CSWGuiControl::HandleRMouseDown()
{
    if (!GetIsSelectable()) {
        CSWGuiControl* pParent = m_pParent;
        while (pParent) {
            if (pParent->GetIsSelectable()) {
                pParent->HandleRMouseDown();
                return;
            }
            pParent = pParent->m_pParent;
        }
        g_pGuiMan->CaptureMouse(this, 4);
        return;
    }
    g_pGuiMan->CaptureMouse(this, 4);
}

void CSWGuiControl::HandleLMouseUp()
{
    CSWGuiControl* pCaptured = g_pGuiMan->m_pMouseOverControl;
    if (pCaptured) {
        CSWGuiControl* pMatch = pCaptured;
        if (pCaptured != this) {
            pMatch = pCaptured->m_pParent;
            while (pMatch && !pMatch->GetIsSelectable())
                pMatch = pMatch->m_pParent;
        }
        if (pMatch == this) {
            if (g_pGuiMan->m_nMouseStateFlags & 1)
                HandleInputEvent(SWGUIEVENT_DOUBLE_CLICK, 1);
            if (m_tail.bEnabled)
                HandleInputEvent(INPUTEVENT_GUI_A, 1);
        }
    }
    g_pGuiMan->ReleaseMouse(this);
    HandleLoseMouseFocus();
}

void CSWGuiControl::HandleRMouseUp()
{
    CSWGuiControl* pCaptured = g_pGuiMan->m_pMouseOverControl;
    if (pCaptured) {
        CSWGuiControl* pMatch = pCaptured;
        if (pCaptured != this) {
            pMatch = pCaptured->m_pParent;
            while (pMatch && !pMatch->GetIsSelectable())
                pMatch = pMatch->m_pParent;
        }
        if (pMatch == this && m_tail.bEnabled)
            HandleInputEvent(INPUTEVENT_MOUSE_RIGHT, 1);
    }
    g_pGuiMan->ReleaseMouse(this);
    HandleLoseMouseFocus();
}

void CSWGuiControl::AddChildControl(CSWGuiControl* pChild)
{
    m_childControls.Add(pChild);
}

void CSWGuiNavigable::HandleInputEvent(int type, int val)
{
    if (val) {
        CSWGuiControl* pCur = this;
        while (pCur) {
            if (!pCur->AsNavigable())
                break;
            CSWGuiNavigable* pNav = static_cast<CSWGuiNavigable*>(pCur->AsNavigable());
            switch (type) {
            case INPUTEVENT_GUI_DPAD_LEFT: case INPUTEVENT_GUI_KEY_LEFT:
                pCur = pNav->m_moveToControls[1];
                break;
            case INPUTEVENT_GUI_DPAD_RIGHT: case INPUTEVENT_GUI_KEY_RIGHT:
                pCur = pNav->m_moveToControls[3];
                break;
            case INPUTEVENT_GUI_DPAD_UP: case INPUTEVENT_GUI_KEY_UP:
                pCur = pNav->m_moveToControls[0];
                break;
            case INPUTEVENT_GUI_DPAD_DOWN: case INPUTEVENT_GUI_KEY_DOWN:
                pCur = pNav->m_moveToControls[2];
                break;
            }
            if (!pCur)
                break;
            if (pCur == this)
                break;
#if KOTOR1  // K1: the walk stops only on a control that is enabled and visible
            if (pCur->m_tail.bEnabled && pCur->m_tail.bVisible)
                break;
#else
            if (pCur->m_tail.bEnabled)
                break;
#endif
        }
#if KOTOR1  // K1: the landing control is re-tested before it is activated
        if (pCur && m_pContainer && pCur->m_tail.bEnabled && pCur != this && pCur->m_tail.bVisible)
#else
        if (pCur && pCur != this && m_pContainer)
#endif
            m_pContainer->SetActiveControl(pCur, 1);
    }
    CSWGuiControl::HandleInputEvent(type, val);
}

void CSWGuiControl::HandleInputEvent(int type, int val)
{
    if (val) {
        if (type == SWGUIEVENT_FOCUS) {
            SetActive(1);
        } else if (type == SWGUIEVENT_UNFOCUS) {
            if (m_tail.bActive)
                SetActive(0);
        }
    }
    // A slot whose type matches but whose function pointer is null is skipped, not dispatched.
    int nCount = m_events.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_events.m_pArray[i].nEventType == type && !!m_events.m_pArray[i].pFn) {
            m_tail.nCurrentEventType = type;
            m_tail.nCurrentEventArg = val;
            (m_events.m_pArray[i].pObj->*m_events.m_pArray[i].pFn)(this);
            return;
        }
    }
}

int CSWGuiControl::GetEvent(int nEventType, CSWGuiObject** ppObj, void (CSWGuiObject::**ppFn)(CSWGuiControl*))
{
    for (int i = 0; i < m_events.m_count; i++) {
        if (m_events.m_pArray[i].nEventType == nEventType) {
            if (ppObj)
                *ppObj = m_events.m_pArray[i].pObj;
            if (ppFn)
                *ppFn = m_events.m_pArray[i].pFn;
            return 1;
        }
    }
    if (ppObj)
        *ppObj = 0;
    if (ppFn)
        *ppFn = 0;
    return 0;
}

void CSWGuiControl::AddEvent(int nEventType, CSWGuiObject* pObj, void (CSWGuiObject::*pFn)(CSWGuiControl*))
{
    int nFoundIndex = -1;
    int nCount = m_events.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_events.m_pArray[i].nEventType == nEventType) {
            nFoundIndex = i;
            break;
        }
    }
    if (nFoundIndex >= 0) {
        if (pFn) {
            m_events.m_pArray[nFoundIndex].pObj = pObj;
            m_events.m_pArray[nFoundIndex].pFn = pFn;
        } else {
            m_events.DelIndex(nFoundIndex);
        }
        return;
    }
    if (!pFn)
        return;
    CSWGuiControlEvent ev;
    ev.pObj = pObj;
    ev.pFn = pFn;
    ev.nEventType = nEventType;
    m_events.Add(ev);
}

void CSWGuiNavigable::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    int bSuccess;
    CResStruct moveToStruct;
    if (pGFF->GetStructFromStruct(&moveToStruct, &resStruct, const_cast<char*>("MOVETO"))) {
        m_moveToControls[1] = reinterpret_cast<CSWGuiControl*>(static_cast<int64_t>(pGFF->ReadFieldINT(&moveToStruct, const_cast<char*>("LEFT"), bSuccess, 0)));
        m_moveToControls[3] = reinterpret_cast<CSWGuiControl*>(static_cast<int64_t>(pGFF->ReadFieldINT(&moveToStruct, const_cast<char*>("RIGHT"), bSuccess, 0)));
        m_moveToControls[0] = reinterpret_cast<CSWGuiControl*>(static_cast<int64_t>(pGFF->ReadFieldINT(&moveToStruct, const_cast<char*>("UP"), bSuccess, 0)));
        m_moveToControls[2] = reinterpret_cast<CSWGuiControl*>(static_cast<int64_t>(pGFF->ReadFieldINT(&moveToStruct, const_cast<char*>("DOWN"), bSuccess, 0)));
    }
    CSWGuiControl::Load(pGFF, resStruct);
}

void CSWGuiControl::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_extent.Load(pGFF, resStruct);
#if KOTOR1  // K1 calls the base setter directly (no virtual dispatch)
    CSWGuiObject::SetExtent(m_extent);
#else
    SetExtent(m_extent);
    SetOriginalExtent(m_extent);
#endif

    int bSuccess;
    m_tail.nControlId = pGFF->ReadFieldINT(&resStruct, const_cast<char*>("ID"), bSuccess, -1);
    int nParentId = pGFF->ReadFieldINT(&resStruct, const_cast<char*>("Obj_ParentID"), bSuccess, -1);
    if (nParentId == -1)
        return;

    CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
    if (!pPanel)
        return;
    CSWGuiControl* pParent = pPanel->GetControl(nParentId);
    m_pParent = pParent;
    pParent->AddChildControl(this);
}

void CSWGuiControl::Load(CSWGuiObject* pContainer, CResGFF* pGFF, CResStruct& resStruct, const CExoString& name)
{
    m_pContainer = pContainer;
    CResStruct subStruct;
    if (pGFF->GetStructFromStruct(&subStruct, &resStruct, static_cast<char*>(name.CStr())))
        Load(pGFF, subStruct);
}

void CSWGuiControl::Load(CSWGuiObject* pContainer, CResGFF* pGFF, CResList& resList, const CExoString& name)
{
    m_pContainer = pContainer;
    int nCount = pGFF->GetListCount(&resList);
    CExoString sTag;
    for (int i = 0; i < nCount; i++) {
        CResStruct elemStruct;
        pGFF->GetListElement(&elemStruct, &resList, i);
        int bSuccess;
        sTag = pGFF->ReadFieldCExoString(&elemStruct, const_cast<char*>("TAG"), bSuccess, CExoString(""));
        if (sTag == name) {
            Load(pGFF, elemStruct);
            return;
        }
    }
}

int CSWGuiControl::DisplayToolTip()
{
    if (m_nToolTipStrRef == 0 && m_sToolTipText == "") {
        if (m_pParent)
            return m_pParent->DisplayToolTip();
        return 0;
    }

    CExoString sText;
    sText = (m_nToolTipStrRef != 0) ? g_pTlkTable->GetSimpleString(m_nToolTipStrRef) : m_sToolTipText;

    if (m_nBoundInputEventId != 0) {
        void* pEvent = g_pExoInput->GetEvent(m_nBoundInputEventId, g_pAppManager->m_pClientExoApp->GetInputClass());
        if (pEvent) {
            int nKeyNameStrRef;
            // Gate on the lookup's return value, since on a miss nKeyNameStrRef is left unset.
            if (g_pRules->m_p2DArrays->m_pBindableKey->GetINTEntry(
                    static_cast<CGuiInputEventRecord*>(pEvent)->m_keyCode - CExoInput::KEYBOARD_START, CExoString("KeyNameStrRef"), &nKeyNameStrRef)) {
                sText = sText + CExoString(" : ") + g_pTlkTable->GetSimpleString(nKeyNameStrRef);
            }
        }
    }
    g_pGuiMan->DisplayToolTip(sText);
    m_tail.bToolTipShown = 1;
    return 1;
}

#if !KOTOR1  // K2 only: K1 exports no operator=
CSWGuiControl& CSWGuiControl::operator=(const CSWGuiControl& other)
{
    m_extent = other.m_extent;
    m_originalExtent = other.m_originalExtent;
    m_bCenterOnNativeRect = other.m_bCenterOnNativeRect;
    m_nReserved2 = other.m_nReserved2;
    m_pParent = other.m_pParent;

    m_childControls = other.m_childControls;

    m_nToolTipStrRef = other.m_nToolTipStrRef;
    m_sToolTipText = other.m_sToolTipText;
    m_nBoundInputEventId = other.m_nBoundInputEventId;
    m_nReserved3 = other.m_nReserved3;
    m_pContainer = other.m_pContainer;

    m_events = other.m_events;

    m_tail = other.m_tail;
    return *this;
}
#endif

CSWGuiLabel::CSWGuiLabel()
{
    m_tail.bFocusOnHover = 0;
}

void CSWGuiLabel::SetExtent(const CSWGuiExtent& extent)
{
    // Both stores re-read `extent`: it may alias `m_extent`, so a hoisted copy is not equivalent.
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_border.m_extent = m_extent;
#else
    m_extent = extent;
    m_border.m_extent = extent;
#endif

    CSWGuiExtent innerExtent = m_border.GetInnerExtent();
    m_text.SetExtent(innerExtent);
}

void CSWGuiLabel::Draw(float fDelta)
{
    // The border draws first, under the text.
    m_border.Draw(fDelta);
    m_text.Draw(fDelta);
}

void CSWGuiLabel::SetEnabled(int bEnabled)
{
    m_tail.bEnabled = bEnabled;
    if (bEnabled)
        m_text.m_params.RestoreDefaultColor();
    else
        m_text.m_params.SetColor(CSWGuiManager::DISABLED_TEXT);
}

void CSWGuiLabel::ReSetFont()
{
    m_text.m_params.ReSetFont();
}

void CSWGuiLabel::Initialize(const CSWGuiExtent& extent, CSWGuiLabel* pTemplate, float fParam)
{
    // Forwards to the four-argument overload through the vtable, passing the template's params.
    Initialize(extent, pTemplate->m_text.m_params, pTemplate->m_border.m_params, fParam);
}

void CSWGuiLabel::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& textParams,
                             const CSWGuiBorderParams& borderParams, float fAlpha)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    CSWGuiExtent inner = m_border.GetInnerExtent();
    m_text.Initialize(inner, textParams, fAlpha);
}

void CSWGuiLabel::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_text.Load(pGFF, resStruct, "TEXT");
    m_border.Load(pGFF, resStruct, "BORDER");
    CSWGuiControl::Load(pGFF, resStruct);
}


void CSWGuiLabelHilight::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_border.m_extent = m_extent;
    m_hilightBorder.m_extent = m_extent;
#else
    m_extent = extent;
    m_border.m_extent = extent;
    m_hilightBorder.m_extent = m_extent;
#endif

    CSWGuiExtent inner = CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                   m_hilightBorder.GetInnerExtent());
    m_text.SetExtent(inner);
}

void CSWGuiLabelHilight::Draw(float fDelta)
{
    if (m_tail.bActive)
        m_hilightBorder.Draw(fDelta);
    else
        m_border.Draw(fDelta);
    m_text.Draw(fDelta);
}

#if !KOTOR1
void CSWGuiLabelHilight::SetActive(int bActive)
{
    m_tail.bActive = bActive;
    if (bActive) {
        m_text.m_params.bHilight = 1;
        m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    } else {
        m_text.m_params.bHilight = 0;
        m_text.m_params.RestoreDefaultColor();
    }
}
#endif

void CSWGuiLabelHilight::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_hilightBorder.Load(pGFF, resStruct, "HILIGHT");
    CSWGuiLabel::Load(pGFF, resStruct);
}

void CSWGuiLabelHilight::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& textParams,
                                    const CSWGuiBorderParams& borderParams,
                                    const CSWGuiBorderParams& borderParamsHilight)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    m_hilightBorder.Initialize(m_extent, borderParamsHilight);
    CSWGuiExtent inner = CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                   m_hilightBorder.GetInnerExtent());
    m_text.Initialize(inner, textParams, 1.0f);
}

void CSWGuiLabelHilight::Initialize(const CSWGuiExtent& extent, CSWGuiLabelHilight* pTemplate)
{
    // Forwards to the four-argument overload through the vtable, passing the template's params.
    Initialize(extent, pTemplate->m_text.m_params, pTemplate->m_border.m_params,
               pTemplate->m_hilightBorder.m_params);
}

void CSWGuiButton::SetExtent(const CSWGuiExtent& extent)
{
    // The text gets the intersection of both borders' inner rectangles, so the caption fits
    // whichever border is showing.
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.m_extent = extent;
    m_borderHilight.m_extent = extent;

    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));
}

void CSWGuiButton::Draw(float fDelta)
{
    if (m_tail.bActive)
        m_borderHilight.Draw(fDelta);
    else
        m_border.Draw(fDelta);
    m_text.Draw(fDelta);
}

void CSWGuiButton::SetActive(int bActive)
{
    m_tail.bActive = bActive;
    if (bActive) {
        m_text.m_params.bHilight = 1;
#if KOTOR1
        m_text.m_params.SetColor(CSWGuiManager::COLOR_YELLOW);
#else
        m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    } else {
        m_text.m_params.bHilight = 0;
        m_text.m_params.RestoreDefaultColor();
    }
}

void CSWGuiButton::SetEnabled(int bEnabled)
{
    m_tail.bEnabled = bEnabled;
    if (bEnabled) {
        m_text.m_params.RestoreDefaultColor();
    } else {
        m_text.m_params.SetColor(CSWGuiManager::DISABLED_TEXT);
        // Virtual, since `this` may be a further-derived control.
        SetActive(0);
    }
}

void CSWGuiButton::ReSetFont()
{
    m_text.m_params.ReSetFont();
}

void CSWGuiButton::Initialize(const CSWGuiExtent& extent, CSWGuiButton* pTemplate)
{
    // Forwards to the four-argument overload through the vtable, passing the template's params.
    Initialize(extent, pTemplate->m_text.m_params, pTemplate->m_border.m_params,
               pTemplate->m_borderHilight.m_params);
}

void CSWGuiButton::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& textParams,
                              const CSWGuiBorderParams& borderParams,
                              const CSWGuiBorderParams& borderParamsHilight)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    m_borderHilight.Initialize(m_extent, borderParamsHilight);
    m_border.GetInnerExtent();
    m_borderHilight.GetInnerExtent();
    CSWGuiExtent inner = CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                   m_borderHilight.GetInnerExtent());
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_text);
#endif
    m_text.Initialize(inner, textParams, 1.0f);
}

#if KOTOR1
// K1 only: CSWGuiButtonToggle::HandleInputEvent carries this body
__attribute__((always_inline))
#endif
void CSWGuiButton::HandleInputEvent(int nType, int nVal)
{
    if (nType == INPUTEVENT_GUI_A && nVal != 0 && m_pContainer != 0) {
        if (m_pContainer->AsSWGuiPanel() != 0) {
            CSWGuiManager* pGuiManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
            if (pGuiManager != 0)
                pGuiManager->PlayGuiSound(m_tail.m_nClickSound);
        } else if (m_pContainer->AsSWGuiListBox() != 0) {
            CSWGuiManager* pGuiManager =
                m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;
            if (pGuiManager != 0)
                pGuiManager->PlayGuiSound(m_tail.m_nClickSound);
        }
    }
    CSWGuiNavigable::HandleInputEvent(nType, nVal);
}

void CSWGuiButton::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_border.Load(pGFF, resStruct, "BORDER");
    m_borderHilight.Load(pGFF, resStruct, "HILIGHT");
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_text);
#endif
    m_text.Load(pGFF, resStruct, "TEXT");
    CSWGuiNavigable::Load(pGFF, resStruct);
}


CSWGuiButtonToggle::CSWGuiButtonToggle()
{
    m_nToggleInputEvent = 39;
    bToggledOn = 0;
    m_tail.m_nClickSound = GUISOUND_CHECKBOX_CHECK;
}


void CSWGuiButtonToggle::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& textParams,
                                    const CSWGuiBorderParams& borderParams,
                                    const CSWGuiBorderParams& borderParamsHilight,
                                    const CSWGuiBorderParams& borderParamsToggled,
                                    const CSWGuiBorderParams& borderParamsToggledHilight)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    m_borderHilight.Initialize(m_extent, borderParamsHilight);
    m_borderToggled.Initialize(m_extent, borderParamsToggled);
    m_borderToggledHilight.Initialize(m_extent, borderParamsToggledHilight);
    CSWGuiExtent innerBorder = m_border.GetInnerExtent();
    CSWGuiExtent innerHilight = m_borderHilight.GetInnerExtent();
    CSWGuiExtent innerToggled = m_borderToggled.GetInnerExtent();
    CSWGuiExtent innerToggledHilight = m_borderToggledHilight.GetInnerExtent();
    CSWGuiExtent inner = CSWGuiExtent::Intersection(
        CSWGuiExtent::Intersection(innerBorder, innerHilight),
        CSWGuiExtent::Intersection(innerToggled, innerToggledHilight));
    m_text.Initialize(inner, textParams, 1.0f);
}

void CSWGuiButtonToggle::Initialize(const CSWGuiExtent& extent, CSWGuiButtonToggle* pTemplate)
{
    // Forwards to the six-argument overload through the vtable, passing the template's params.
    Initialize(extent, pTemplate->m_text.m_params, pTemplate->m_border.m_params,
               pTemplate->m_borderHilight.m_params, pTemplate->m_borderToggled.m_params,
               pTemplate->m_borderToggledHilight.m_params);
}

void CSWGuiButtonToggle::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.m_extent = extent;
    m_borderHilight.m_extent = extent;
    m_borderToggled.m_extent = extent;
    m_borderToggledHilight.m_extent = extent;

    CSWGuiExtent innerBorder = m_border.GetInnerExtent();
    CSWGuiExtent innerHilight = m_borderHilight.GetInnerExtent();
    CSWGuiExtent innerToggled = m_borderToggled.GetInnerExtent();
    CSWGuiExtent innerToggledHilight = m_borderToggledHilight.GetInnerExtent();
    CSWGuiExtent::Intersection(
        CSWGuiExtent::Intersection(innerBorder, innerHilight),
        CSWGuiExtent::Intersection(innerToggled, innerToggledHilight));
    m_text.SetExtent(extent);
}

void CSWGuiButtonToggle::Draw(float fDelta)
{
    if (bToggledOn) {
        if (m_tail.bActive)
            m_borderToggledHilight.Draw(fDelta);
        else
            m_borderToggled.Draw(fDelta);
    } else {
        if (m_tail.bActive)
            m_borderHilight.Draw(fDelta);
        else
            m_border.Draw(fDelta);
    }
    m_text.Draw(fDelta);
}

void CSWGuiButtonToggle::HandleInputEvent(int nType, int nVal)
{
    if (nVal != 0 && m_nToggleInputEvent == nType)
        bToggledOn = !bToggledOn;
    CSWGuiButton::HandleInputEvent(nType, nVal);
}

void CSWGuiButtonToggle::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    int bSuccess;
    m_borderToggled.Load(pGFF, resStruct, "SELECTED");
    m_borderToggledHilight.Load(pGFF, resStruct, "HILIGHTSELECTED");
    bToggledOn = pGFF->ReadFieldBYTE(&resStruct, const_cast<char*>("ISSELECTED"), bSuccess, 0);
    CSWGuiButton::Load(pGFF, resStruct);
}

void CSWGuiRadioButton::HandleMouseOver(int bMouseIsOver)
{
    if (bMouseIsOver) {
        g_pGuiMan->ResetToolTipTimer();
        return;
    }
    if (m_tail.bToolTipShown) {
        g_pGuiMan->RemoveToolTip();
        m_tail.bToolTipShown = 0;
    }
}

void CSWGuiRadioButton::HandleLMouseDown()
{
    if (!m_tail.bEnabled)
        return;
    HandleInputEvent(SWGUIEVENT_FOCUS, 1);
    HandleFocusChange(1);
    g_pGuiMan->CaptureMouse(this, 1);
}

void CSWGuiRadioButton::HandleLMouseUp()
{
    CSWGuiControl* pCaptured = g_pGuiMan->m_pMouseOverControl;
    if (pCaptured) {
        CSWGuiControl* pMatch = pCaptured;
        if (pCaptured != this) {
            pMatch = pCaptured->m_pParent;
            while (pMatch && !pMatch->GetIsSelectable())
                pMatch = pMatch->m_pParent;
        }
        if (pMatch == this) {
            if (g_pGuiMan->m_nMouseStateFlags & 1) {
                HandleInputEvent(INPUTEVENT_GUI_A, 1);
                HandleInputEvent(SWGUIEVENT_DOUBLE_CLICK, 1);
            }
        }
    }
    g_pGuiMan->ReleaseMouse(this);
    HandleLoseMouseFocus();
}

CSWGuiSlider::CSWGuiSlider()
{
    m_nMaxValue = 0;
    m_nCurValue = 0;
    m_nMoveSound = GUISOUND_ENTERED_DEFAULT;
}

void CSWGuiSlider::Draw(float fDelta)
{
    // Each border is named in its own arm so the call stays direct.
    if (m_tail.bActive && m_tail.bEnabled)
        m_borderHilight.Draw(fDelta);
    else
        m_border.Draw(fDelta);
    m_imageThumb.Draw(fDelta);
}

void CSWGuiSlider::SetMaxValue(int nValue)
{
    if (m_nMaxValue == nValue || nValue < 0)
        return;
    m_nMaxValue = nValue;
    SetExtent(m_extent);
}

void CSWGuiSlider::SetCurValue(int nValue)
{
    int nNew;
    if (nValue < 0)
        nNew = 0;
    else if (m_nMaxValue < nValue)
        nNew = m_nMaxValue;
    else
        nNew = nValue;
    m_nCurValue = nNew;
    SetExtent(m_extent);
}

int CSWGuiSlider::HandleMouseCapturedMovement(int x, int y)
{
    K2_UNUSED(x); K2_UNUSED(y);
    // A drag re-runs the hit test and re-seats the thumb; the handler re-reads the mouse itself.
    HandleLMouseDown();
    return 1;
}

void CSWGuiSlider::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_border.m_extent = m_extent;
    m_borderHilight.m_extent = m_extent;
#else
    m_extent = extent;
    m_border.m_extent = extent;
    m_borderHilight.m_extent = m_extent;
#endif

    CSWGuiExtent extThumb = m_imageThumb.GetImageExtent();
#ifndef K2_UI_TOUCH
    // pc / xbox: the thumb's own length is in the layout's pixels, as the trough it slides in was
    // before the layout was scaled to the screen (K1's 640x480 layouts, KOTOR II's g_nGUIHeight
    // lines); at the texture's size it was a sliver beside a trough three times its height.
#if KOTOR1
    float fThumbScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
    float fThumbScale = static_cast<float>(g_nScreenHeight) / static_cast<float>(static_cast<unsigned int>(g_nGUIHeight));
#endif
    extThumb.w = static_cast<int>(static_cast<float>(extThumb.w) * fThumbScale);
    extThumb.h = static_cast<int>(static_cast<float>(extThumb.h) * fThumbScale);
#endif
    if (m_nMaxValue > 0)
    {
        if (m_extent.h > m_extent.w)
        {
            extThumb.w = m_extent.w;
            extThumb.x = m_extent.x;
            if (m_extent.h > extThumb.h)
                extThumb.y = m_extent.y
                             + m_nCurValue * (m_extent.h - extThumb.h) / m_nMaxValue;
            else
            {
                extThumb.h = m_extent.h;
                extThumb.y = 0;
            }
        }
        else
        {
            extThumb.h = m_extent.h;
            extThumb.y = m_extent.y;
            if (m_extent.w > extThumb.w)
                extThumb.x = m_extent.x
                             + m_nCurValue * (m_extent.w - extThumb.w) / m_nMaxValue;
            else
            {
                extThumb.w = m_extent.w;
                extThumb.x = 0;
            }
        }
    }
    else
    {
        // With no range, the thumb fills the trough on the sliding axis and is clamped on the
        // other.
        if (m_extent.h > m_extent.w)
        {
            extThumb.w = m_extent.w;
            extThumb.h = (m_extent.h > extThumb.h) ? extThumb.h : m_extent.h;
        }
        else
        {
            extThumb.w = (m_extent.w > extThumb.w) ? extThumb.w : m_extent.w;
            extThumb.h = m_extent.h;
        }
        extThumb.x = m_extent.x;
        extThumb.y = m_extent.y;
    }

    m_imageThumb.SetExtent(extThumb);
}

void CSWGuiSlider::Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& borderParams,
                              const CSWGuiBorderParams& borderParamsHilight,
                              const CResRef& resRefThumb)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    m_borderHilight.Initialize(m_extent, borderParamsHilight);

    m_imageThumb.Initialize(m_extent,
        CSWGuiImageParams(resRefThumb, 0, 0, 0.0f, 18, 1.0f, Vector{ 1.0f, 1.0f, 1.0f }));

    SetExtent(m_extent);
}

void CSWGuiSlider::Initialize(const CSWGuiExtent& extent, CSWGuiSlider* pTemplate)
{
    // Forwards to the four-argument overload through the vtable, passing the template's params.
    Initialize(extent, pTemplate->m_border.m_params, pTemplate->m_borderHilight.m_params,
               pTemplate->m_imageThumb.m_params.m_resRefImage);
}

int CSWGuiSlider::HitCheckSlider(int x, int y)
{
    CSWGuiExtent extThumb = m_imageThumb.m_extent;
    CSWGuiExtent extInner = m_border.GetInnerExtent();

    if (m_extent.h > m_extent.w)
    {
        if (extThumb.y > y)
        {
            if (extInner.x < x && extInner.y < y && extInner.w + extInner.x > x)
                return 2;
        }
        else if (extThumb.x + extThumb.w >= x && extThumb.x <= x
                 && extThumb.y + extThumb.h >= y)
        {
            return 1;
        }
        if (extThumb.y + extThumb.h < y && extInner.x < x
            && extInner.h + extInner.y > y && extInner.w + extInner.x > x)
            return 3;
    }
    else
    {
        if (extThumb.x > x)
        {
            if (extInner.x < x && extInner.y < y && extInner.h + extInner.y > y)
                return 2;
        }
        else if (extThumb.y <= y && extThumb.x + extThumb.w >= x
                 && extThumb.y + extThumb.h >= y)
        {
            return 1;
        }
        if (extThumb.x + extThumb.w < x && extInner.w + extInner.x > x
            && extInner.y < y && extInner.h + extInner.y > y)
            return 3;
    }
    return 0;
}

void CSWGuiSlider::HandleLMouseDown()
{
    if (!m_tail.bEnabled)
        return;

    int nX, nY;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nX, &nY);
    m_nHitPart = HitCheckSlider(nX, nY);
    if (m_nHitPart == 2)
        HandleInputEvent(SWGUIEVENT_WHEEL_DOWN, 1);
    else if (m_nHitPart == 3)
        HandleInputEvent(SWGUIEVENT_WHEEL_UP, 1);

#if !KOTOR1  // K1: captures the mouse after positioning the thumb
    g_pGuiMan->CaptureMouse(this, 1);
#endif

    CSWGuiExtent extInner = m_border.GetInnerExtent();
    if (nX > extInner.x && nX < extInner.w + extInner.x
        && nY > extInner.y && nY < extInner.h + extInner.y)
    {
        float fRatio;
        if (m_extent.h > m_extent.w)
            fRatio = static_cast<float>(nY - extInner.y) / static_cast<float>(extInner.h);
        else
            fRatio = static_cast<float>(nX - extInner.x) / static_cast<float>(extInner.w);
        if (fRatio > 0.0f)
        {
            fRatio = fRatio * 1.1f - 0.05f;
            if (fRatio < 0.0f)
                fRatio = 0.0f;
            if (fRatio > 1.0f)
                fRatio = 1.0f;
            SetCurValue(static_cast<int>(fRatio * static_cast<float>(m_nMaxValue)));
        }
    }
#if KOTOR1
    g_pGuiMan->CaptureMouse(this, 1);
#endif
}

void CSWGuiSlider::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    int bSuccess;
    m_border.Load(pGFF, resStruct, "BORDER");
    m_borderHilight.Load(pGFF, resStruct, "HILIGHT");
    m_imageThumb.Load(pGFF, resStruct, "THUMB");
    SetMaxValue(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("MAXVALUE"), bSuccess, 0));
    SetCurValue(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("CURVALUE"), bSuccess, 0));
    CSWGuiNavigable::Load(pGFF, resStruct);
}

void CSWGuiSlider::HandleInputEvent(int nType, int nVal)
{
    if (!m_tail.bEnabled)
        return;

#if KOTOR1  // K1: a release goes straight to the base dispatch
    if (nVal == 0)
    {
        CSWGuiNavigable::HandleInputEvent(nType, nVal);
        return;
    }
#endif
    if (nVal != 0)
    {
        if (m_extent.h > m_extent.w)
        {
            switch (nType)
            {
            case INPUTEVENT_GUI_DPAD_UP:
            case INPUTEVENT_GUI_KEY_UP:
            case SWGUIEVENT_WHEEL_DOWN:  // wheel down decrements, like up
            {
                int nOld = m_nCurValue;
                SetCurValue(nOld - 1);
                if (nOld != 0)
                {
                    if (m_pContainer != 0)
                    {
                        if (m_pContainer->AsSWGuiPanel() != 0)
                        {
                            CSWGuiManager* pGuiManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                        else if (m_pContainer->AsSWGuiListBox() != 0)
                        {
                            CSWGuiManager* pGuiManager =
                                m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                    }
                }
                break;
            }
            case INPUTEVENT_GUI_DPAD_DOWN:
            case INPUTEVENT_GUI_KEY_DOWN:
            case SWGUIEVENT_WHEEL_UP:
            {
                int nMax = m_nMaxValue;
                int nOld = m_nCurValue;
                SetCurValue(nOld + 1);
                if (nOld != nMax)
                {
                    if (m_pContainer != 0)
                    {
                        if (m_pContainer->AsSWGuiPanel() != 0)
                        {
                            CSWGuiManager* pGuiManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                        else if (m_pContainer->AsSWGuiListBox() != 0)
                        {
                            CSWGuiManager* pGuiManager =
                                m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                    }
                }
                break;
            }
            }
        }
        else
        {
            switch (nType)
            {
            case INPUTEVENT_GUI_DPAD_LEFT:
            case INPUTEVENT_GUI_KEY_LEFT:
            case SWGUIEVENT_WHEEL_DOWN:
            {
                int nOld = m_nCurValue;
                SetCurValue(nOld - 1);
                if (nOld != 0)
                {
                    if (m_pContainer != 0)
                    {
                        if (m_pContainer->AsSWGuiPanel() != 0)
                        {
                            CSWGuiManager* pGuiManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                        else if (m_pContainer->AsSWGuiListBox() != 0)
                        {
                            CSWGuiManager* pGuiManager =
                                m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                    }
                }
                break;
            }
            case INPUTEVENT_GUI_DPAD_RIGHT:
            case INPUTEVENT_GUI_KEY_RIGHT:
            case SWGUIEVENT_WHEEL_UP:
            {
                int nMax = m_nMaxValue;
                int nOld = m_nCurValue;
                SetCurValue(nOld + 1);
                if (nOld != nMax)
                {
                    if (m_pContainer != 0)
                    {
                        if (m_pContainer->AsSWGuiPanel() != 0)
                        {
                            CSWGuiManager* pGuiManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                        else if (m_pContainer->AsSWGuiListBox() != 0)
                        {
                            CSWGuiManager* pGuiManager =
                                m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;
                            if (pGuiManager != 0)
                                pGuiManager->PlayGuiSound(m_nMoveSound);
                        }
                    }
                }
                break;
            }
            }
        }
    }

    CSWGuiNavigable::HandleInputEvent(nType, nVal);
}

#if !KOTOR1
CSWGuiSlider::~CSWGuiSlider()
{
}
#endif


#if KOTOR1
// K1: resolution rescale in place of K2's aspect multipliers, for all four
// ScaleExtentForResolution bodies. The factor is the screen height over K1's 768-pixel design
// height (g_SDL_mode), computed once per class; x/y truncate and w/h round up.

void CSWGuiObject::ScaleExtentForResolution()
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    ScaleExtentForResolution(s_fScale);
}

void CSWGuiObject::ScaleExtentForResolution(float fScale)
{
    CSWGuiExtent extent = m_extent;
    extent.x = static_cast<int>(extent.x * fScale);
    extent.y = static_cast<int>(extent.y * fScale);
    extent.w = static_cast<int>(ceilf(extent.w * fScale));
    extent.h = static_cast<int>(ceilf(extent.h * fScale));
    SetExtent(extent);
    m_bScaledForResolution = 1;
}

void CSWGuiPanel::ScaleExtentForResolution()
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiPanel::ScaleExtentForResolution(s_fScale);
}

void CSWGuiPanel::ScaleExtentForResolution(float fScale)
{
    CSWGuiObject::ScaleExtentForResolution(fScale);
    for (int i = 0; i < m_controls.m_count; i++)
    {
        CSWGuiControl* pControl = m_controls.m_pArray[i];
#if !defined(K2_UI_TOUCH)
        // pc / xbox: the bar column keeps its layout width, so scale it too.
        if (pControl && pControl->AsListBox()) {
            CSWGuiListBox* pList = static_cast<CSWGuiListBox*>(pControl->AsListBox());
            pList->m_scrollBar.m_extent.w = static_cast<int>(ceilf(pList->m_scrollBar.m_extent.w * fScale));
        }
#endif
#if defined(K2_UI_TOUCH) && defined(K2_FIX_ENGINE_BUGS)
        // The rows a list box builds from its prototype take its height; the prototype is scaled with
        // the list, or the rows stay at layout size while their text grows with the screen and is
        // dropped (the journal's quests, the feedback and resolution lists, AI script select).
        if (pControl && pControl->AsListBox()) {
            CSWGuiListBox* pList = static_cast<CSWGuiListBox*>(pControl->AsListBox());
            if (pList->m_pProtoItem)
                pList->m_pProtoItem->ScaleExtentForResolution(fScale);
        }
#endif
        if (pControl)
            pControl->ScaleExtentForResolution(fScale);
    }
}
#endif


CSWGuiProgressBar::CSWGuiProgressBar()
{
    m_nMaxValue = 0;
    m_nCurValue = 0;
    m_bStartFromLeft = 1;
    m_tail.bFocusOnHover = 0;
}

void CSWGuiProgressBar::Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& borderParams,
                                   const CSWGuiBorderParams& borderParamsProgress)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    m_borderProgress.Initialize(m_extent, borderParamsProgress);
    SetExtent(m_extent);
}

void CSWGuiProgressBar::Initialize(const CSWGuiExtent& extent, CSWGuiProgressBar* pTemplate)
{
    Initialize(extent, pTemplate->m_border.m_params, pTemplate->m_borderProgress.m_params);
}

void CSWGuiProgressBar::SetExtent(const CSWGuiExtent& extent)
{
    CSWGuiExtent extentProgress;
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_border.m_extent = m_extent;
#else
    m_extent = extent;
    m_border.m_extent = extent;
#endif
    extentProgress = m_border.GetInnerExtent();
    if (m_nMaxValue > 0)
    {
        if (m_extent.h > m_extent.w)
        {
            int nHeight = extentProgress.h;
            int nLength = static_cast<int>(static_cast<float>(m_nCurValue) / static_cast<float>(m_nMaxValue) * static_cast<float>(nHeight));
            if (!m_bStartFromLeft)
                extentProgress.y += nHeight - nLength;
            extentProgress.h = nLength;
        }
        else
        {
            int nWidth = extentProgress.w;
            int nLength = static_cast<int>(static_cast<float>(m_nCurValue) / static_cast<float>(m_nMaxValue) * static_cast<float>(nWidth));
            if (!m_bStartFromLeft)
                extentProgress.x += nWidth - nLength;
            extentProgress.w = nLength;
        }
    }
    m_borderProgress.m_extent = extentProgress;
}

void CSWGuiProgressBar::Draw(float fDelta)
{
    m_border.Draw(fDelta);
    m_borderProgress.Draw(fDelta);
}

void CSWGuiProgressBar::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    int bSuccess;
    m_border.Load(pGFF, resStruct, "BORDER");
    m_borderProgress.Load(pGFF, resStruct, "PROGRESS");
    SetMaxValue(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("MAXVALUE"), bSuccess, 0));
    SetCurValue(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("CURVALUE"), bSuccess, 0));
    SetStartFromLeft(pGFF->ReadFieldBYTE(&resStruct, const_cast<char*>("STARTFROMLEFT"), bSuccess, 1));
    CSWGuiControl::Load(pGFF, resStruct);
}

void CSWGuiProgressBar::SetMaxValue(int nValue)
{
    if (m_nMaxValue == nValue || nValue < 0)
        return;
    m_nMaxValue = nValue;
    SetExtent(m_extent);
}

void CSWGuiProgressBar::SetCurValue(int nValue)
{
    int nNew;
    if (nValue < 0)
        nNew = 0;
    else if (m_nMaxValue < nValue)
        nNew = m_nMaxValue;
    else
        nNew = nValue;
    if (m_nCurValue == nNew)
        return;
    m_nCurValue = nNew;
    SetExtent(m_extent);
}

void CSWGuiProgressBar::SetStartFromLeft(int bStart)
{
    if (m_bStartFromLeft == bStart)
        return;
    m_bStartFromLeft = bStart;
    SetExtent(m_extent);
}

void CSWGuiProgressBarReveal::SetExtent(const CSWGuiExtent& extent)
{
    CSWGuiExtent extentProgress = extent;
    extentProgress.x = 0;
    extentProgress.y = 0;
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_extentReveal = m_extent;
    m_border.m_extent = extent;
#else
    m_extent = extent;
    m_extentReveal = extent;
    m_border.m_extent = extent;
#endif
    if (m_nMaxValue > 0)
    {
        if (m_extent.h > m_extent.w)
        {
            int nHeight = m_extentReveal.h;
            int nLength = static_cast<int>(static_cast<float>(nHeight) * (static_cast<float>(m_nCurValue) / static_cast<float>(m_nMaxValue)));
            if (!m_bStartFromLeft)
            {
                m_extentReveal.y += nHeight - nLength;
                extentProgress.y = nLength - nHeight;
            }
            m_extentReveal.h = nLength;
        }
        else
        {
            int nWidth = m_extentReveal.w;
            int nLength = static_cast<int>(static_cast<float>(nWidth) * (static_cast<float>(m_nCurValue) / static_cast<float>(m_nMaxValue)));
            if (!m_bStartFromLeft)
            {
                extentProgress.x = nLength - nWidth;
                m_extentReveal.x += nWidth - nLength;
            }
            m_extentReveal.w = nLength;
        }
    }
    m_borderProgress.m_extent = extentProgress;
}

void CSWGuiProgressBarReveal::Draw(float fDelta)
{
    m_border.Draw(fDelta);
    if (AurGUISetupViewport(m_extentReveal.x, m_extentReveal.y, m_extentReveal.w, m_extentReveal.h,
                            CSWGuiDrawable::NO_COLOURING, false, 1.0f))
    {
        m_borderProgress.Draw(fDelta);
        AurGUICloseViewport();
    }
}

CSWGuiScrollBar::CSWGuiScrollBar()
{
    m_nMaxValue = 0;
    m_nCurValue = 0;
    m_nVisibleValue = 0;
    bDrawMode = 0;
#ifndef K2_UI_TOUCH
    bSlimBar = 0;
#endif
}


int CSWGuiScrollBar::GetTopArrowOn()
{
    if (m_nVisibleValue >= m_nMaxValue)
        return 0;
    return m_nCurValue > 0;
}

int CSWGuiScrollBar::GetBottomArrowOn()
{
    int nLimit = m_nMaxValue;
    if (!bNoSelection && m_nVisibleValue <= 1)
        nLimit = m_nCurValue != m_nMaxValue ? m_nMaxValue + 1 : m_nMaxValue;

    if (m_nVisibleValue >= m_nMaxValue)
        return 0;
    return m_nCurValue + m_nVisibleValue < nLimit;
}

void CSWGuiScrollBar::Draw(float fAlpha)
{
#if !defined(K2_UI_TOUCH)
    // pc / xbox: with bDrawMode clear the trough, thumb and both arrows are drawn; with it set,
    // only the arrows that can scroll.
    if (!bSlimBar)
    {
        int bTop = 1;
        int bBottom = 1;
        if (bDrawMode)
        {
            bTop = GetTopArrowOn();
            bBottom = GetBottomArrowOn();
        }
        else
        {
            m_border.Draw(fAlpha);
            m_imageThumb.Draw(fAlpha);
        }
        CSWGuiExtent extDir = m_imageDir.m_extent;
        int bVertical = m_extent.h > m_extent.w;
        if (bTop)
        {
            if (bVertical)
                extDir.y = m_extent.y;
            else
                extDir.x = m_extent.x;
            m_imageDir.m_params.m_nFlipStyle = 0;
            m_imageDir.SetExtent(extDir);
            m_imageDir.Draw(fAlpha);
        }
        if (bBottom)
        {
            if (bVertical)
                extDir.y = m_extent.y + m_extent.h - extDir.h;
            else
                extDir.x = m_extent.x + m_extent.w - extDir.w;
            m_imageDir.m_params.m_nFlipStyle = bVertical ? 4 : 8;
            m_imageDir.SetExtent(extDir);
            m_imageDir.Draw(fAlpha);
        }
        return;
    }
#endif
    if (bDrawMode)
        return;

    // The thumb is always half-transparent; m_imageDir is never drawn.
    m_imageThumb.m_params.m_fAlpha = 0.5f;
    m_border.Draw(fAlpha);
    m_imageThumb.Draw(fAlpha);
}

void CSWGuiScrollBar::SetMaxValue(int nMaxValue, unsigned char bDisableSelection)
{
    if (m_nMaxValue != nMaxValue && nMaxValue >= 0)
    {
        m_nMaxValue = nMaxValue;
        bNoSelection = bDisableSelection;
        SetExtent(m_extent);
    }
}

void CSWGuiScrollBar::SetCurValue(int nCurValue)
{
    if (nCurValue < 0)
        nCurValue = 0;
    else if (m_nMaxValue < nCurValue)
        nCurValue = m_nMaxValue;

    m_nCurValue = nCurValue;
    SetExtent(m_extent);
}

void CSWGuiScrollBar::SetVisibleValue(int nVisibleValue)
{
    if (m_nVisibleValue != nVisibleValue)
    {
        // A zero visible span is only legal on an empty bar; anything else is clamped up to 1.
        m_nVisibleValue = nVisibleValue;
        if (nVisibleValue < 0 || (nVisibleValue == 0 && m_nMaxValue != 0))
            m_nVisibleValue = 1;
    }
    SetExtent(m_extent);
}

void CSWGuiScrollBar::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    AurMakeCurrent();

    // Only the arrow glyph's position is taken from the image; both dimensions become 8.
    CSWGuiExtent extDir = m_imageDir.GetImageExtent();
    CSWGuiExtent extThumb;
    extDir.w = 8;
    extDir.h = 8;

    // All four border fields are collected in locals first, so both axis arms share one set of
    // stores instead of writing the members separately inside each arm.
    int nBx, nBy, nBw, nBh;
    if (m_extent.h > m_extent.w)
    {
#if !defined(K2_UI_TOUCH)
        // pc / xbox: the arrow glyph is drawn (Draw), one square the bar's width at each end
        if (!bSlimBar)
        {
            extDir.w = extDir.h = m_extent.w;
            if (extDir.h * 2 > m_extent.h)
                extDir.h = m_extent.h / 2;
            extDir.x = m_extent.x;
        }
        else
#endif
        {
            if (m_extent.h <= 15)
                extDir.h = m_extent.h / 2;
            if (m_extent.w >= 9)
                extDir.x = m_extent.x + (m_extent.w - 8) / 2;
            else
            {
                extDir.w = m_extent.w;
                extDir.x = m_extent.x;
            }
        }
        m_imageDir.SetExtent(extDir);

        nBx = m_extent.x;
        nBy = m_extent.y + extDir.h;
        nBw = m_extent.w;
        nBh = m_extent.h - extDir.h * 2;
    }
    else
    {
#if !defined(K2_UI_TOUCH)
        if (!bSlimBar)
        {
            extDir.w = extDir.h = m_extent.h;
            if (extDir.w * 2 > m_extent.w)
                extDir.w = m_extent.w / 2;
            extDir.y = m_extent.y;
        }
        else
#endif
        {
            if (m_extent.w <= 15)
                extDir.w = m_extent.w / 2;
            if (m_extent.h >= 9)
                extDir.y = m_extent.y + (m_extent.h - 8) / 2;
            else
            {
                extDir.h = m_extent.h;
                extDir.y = m_extent.y;
            }
        }
        m_imageDir.SetExtent(extDir);

        nBx = m_extent.x + extDir.w;
        nBy = m_extent.y;
        nBw = m_extent.w - extDir.w * 2;
        nBh = m_extent.h;
    }

    m_border.m_extent.x = nBx;
    m_border.m_extent.y = nBy;
    m_border.m_extent.w = nBw;
    m_border.m_extent.h = nBh;

    CSWGuiExtent extInner = m_border.GetInnerExtent();

    // The two arms are deliberately not mirrors; the vertical one divides the travel by m_nMaxValue
    // - 1, the horizontal one by m_nMaxValue.
    if (m_extent.h > m_extent.w)
    {
        extThumb.x = extInner.x;
        extThumb.w = extInner.w;
        if (m_nMaxValue >= 2)
        {
            int nThumb = static_cast<int>(static_cast<float>(m_nVisibleValue) / static_cast<float>(m_nVisibleValue + m_nMaxValue) * static_cast<float>(extInner.h));
            if (nThumb <= 4)
                nThumb = 4;
            extThumb.h = nThumb;
            extThumb.y = (extInner.h - nThumb) * m_nCurValue / (m_nMaxValue - 1) + extInner.y;
        }
        else
        {
            extThumb.h = extInner.h;
            extThumb.y = extInner.y;
        }
    }
    else
    {
        extThumb.y = extInner.y;
        extThumb.h = extInner.h;
        if (m_nMaxValue != 0)
        {
            int nThumb = static_cast<int>(static_cast<float>(m_nVisibleValue) / static_cast<float>(m_nVisibleValue + m_nMaxValue) * static_cast<float>(extInner.w));
            if (nThumb <= 4)
                nThumb = 4;
            extThumb.w = nThumb;
            extThumb.x = (extInner.w - nThumb) * m_nCurValue / m_nMaxValue + extInner.x;
        }
        else
        {
            extThumb.w = extInner.w;
            extThumb.x = extInner.x;
        }
    }

    m_imageThumb.SetExtent(extThumb);
}

void CSWGuiScrollBar::Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& borderParams,
                                 const CResRef& resRefDir, const CResRef& resRefThumb)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);

    // One params block, re-pointed at each of the two images.
    CSWGuiImageParams params(CResRef(), 0, 0, 0.0f, 18, 1.0f, Vector{ 1.0f, 1.0f, 1.0f });

    params.SetImage(resRefDir, 0);
    m_imageDir.Initialize(m_extent, params);

    params.SetImage(resRefThumb, 0);
    m_imageThumb.Initialize(m_extent, params);

    SetExtent(m_extent);
}

void CSWGuiScrollBar::Initialize(const CSWGuiExtent& extent, CSWGuiScrollBar* pOther)
{
    // Forwards to the four-argument overload through the vtable, passing the other bar's params.
    Initialize(extent, pOther->m_border.m_params, pOther->m_imageDir.m_params.m_resRefImage,
               pOther->m_imageThumb.m_params.m_resRefImage);
}

void CSWGuiScrollBar::Load(CResGFF* pGFF, CResStruct& rStruct)
{
    m_border.Load(pGFF, rStruct, "BORDER");
    m_imageDir.Load(pGFF, rStruct, "DIR");
    m_imageThumb.Load(pGFF, rStruct, "THUMB");

    int bSuccess;
    SetMaxValue(pGFF->ReadFieldINT(&rStruct, const_cast<char*>("MAXVALUE"), bSuccess, 0), 0);
    SetCurValue(pGFF->ReadFieldINT(&rStruct, const_cast<char*>("CURVALUE"), bSuccess, 0));
    SetVisibleValue(pGFF->ReadFieldINT(&rStruct, const_cast<char*>("VISIBLEVALUE"), bSuccess, 0));
    bDrawMode = pGFF->ReadFieldBYTE(&rStruct, const_cast<char*>("DRAWMODE"), bSuccess, 0);

    CSWGuiControl::Load(pGFF, rStruct);
}

int CSWGuiScrollBar::HitCheckScrollbar(int x, int y)
{
    // Work out which part of the bar the point (x, y) is over: 0 none, 1/2 the two end arrows (the
    // ones GetTopArrowOn/GetBottomArrowOn light up), 3 the thumb, 4/5 the two page zones either
    // side of it. The arrow zones are measured off m_imageDir's extent, the thumb zone off
    // m_imageThumb's, and the two page zones clamp their cross axis against the border's inner
    // extent rather than m_extent.
    //
    // The horizontal left-arrow test spans the whole bar plus a glyph
    // (m_extent.x + m_extent.w + dir.w >= x), so a horizontal bar reports 1 for nearly its whole
    // length. That is the source, not a slip; do not "fix" it.
    int nDirWidth = m_imageDir.m_extent.w;
    int nDirHeight = m_imageDir.m_extent.h;
    int nX = m_extent.x;
    int nY = m_extent.y;
    int nWidth = m_extent.w;
    int nHeight = m_extent.h;
    int nThumbX = m_imageThumb.m_extent.x;
    int nThumbY = m_imageThumb.m_extent.y;
    int nThumbWidth = m_imageThumb.m_extent.w;
    int nThumbHeight = m_imageThumb.m_extent.h;
    CSWGuiExtent inner = m_border.GetInnerExtent();

    if (m_extent.h > m_extent.w)
    {
        if (nThumbY <= y &&
            nThumbY + nThumbHeight >= y &&
            nThumbX <= x &&
            nThumbX + nThumbWidth >= x)
            return 3;

        if (nY + nDirHeight >= y &&
            nY < y &&
            nX < x &&
            nX + nWidth > x)
            return 1;

        if (nY + nHeight > y &&
            nY + nHeight - nDirHeight <= y &&
            nX < x &&
            nX + nWidth > x)
            return 2;

        if (nY + nDirHeight < y &&
            nThumbY > y &&
            inner.x < x &&
            inner.x + inner.w > x)
            return 4;

        if (nY + nHeight - nDirHeight > y &&
            nThumbY + nThumbHeight < y &&
            inner.x < x &&
            inner.x + inner.w > x)
            return 5;
    }
    else
    {
        if (nX < x &&
            nX + nWidth + nDirWidth >= x &&
            nY < y &&
            nY + nHeight > y)
            return 1;

        if (nX + nWidth > x &&
            nX + nWidth - nDirWidth <= x &&
            nY < y &&
            nY + nHeight > y)
            return 2;

        if (nThumbX <= x &&
            nThumbX + nThumbWidth >= x &&
            nThumbY <= y &&
            nThumbY + nThumbHeight >= y)
            return 3;

        if (nX + nDirWidth < x &&
            nThumbX > x &&
            inner.y < y &&
            inner.y + inner.h > y)
            return 4;

        if (nX + nWidth - nDirWidth > x &&
            nThumbX + nThumbWidth < x &&
            inner.y < y &&
            inner.y + inner.h > y)
            return 5;
    }

    return 0;
}


CSWGuiListBox::CSWGuiListBox()
{
    m_nSelectedIndex = -1;
    m_nTopRow = 0;
    m_nOversizedStep = 1;
    bSelectOnLeftClick = 0;
    m_fVisibleLines = 1.0f;
    m_nPadding = 0;
    m_nScrollSound = GUISOUND_NONE;
    m_nPressedIndex = -1;
    m_pProtoItem = 0;
    m_nBottomRow = 0;
    m_nRepeatEvent = 0;
    m_fRepeatTimer = 0.0f;
    m_nDragStartScroll = 0;
    m_pMouseOverControl = 0;
    bOwnsItems = 1;
    bRestoreItemExtent = 0;
    bLayoutDirty = 1;
    bScrollBarOnLeft = 1;
    bShowScrollBar = 1;
    bLooping = 0;
    bUnequalRows = 0;
    bNoSelection = 0;
    bClipPartialRows = 0;
    bPulseArrows = 1;
    bScrollToSelection = 1;
    s_nScrollPulsesShown++;
    m_fScrollHintTime = 0.0f;

    CSWGuiExtent extent;
    CResRef resRefCursor("ios_touch_cursor");
    m_imageArrow.Initialize(extent, CSWGuiImageParams(resRefCursor, 0, 0, 0.0f, 18, 1.0f,
                                   Vector(1.0f, 1.0f, 1.0f)));
}


CSWGuiListBox::~CSWGuiListBox()
{
    if (m_pProtoItem)
    {
        delete m_pProtoItem;
        m_pProtoItem = 0;
    }
    ClearItems();
}

void CSWGuiListBox::ClearItems()
{
    if (bOwnsItems)
    {
        if (m_pMouseOverControl && m_pMouseOverControl->AsScrollBar() == 0)
            m_pMouseOverControl = 0;

        for (int i = 0; i < m_aControls.m_count; i++)
        {
            if (m_aControls.m_pArray[i])
                delete m_aControls.m_pArray[i];
            m_aControls.m_pArray[i] = 0;
        }
    }
    else
    {
        for (int i = 0; i < m_aControls.m_count; i++)
        {
            if (bRestoreItemExtent)
                m_aControls.m_pArray[i]->SetExtent(m_aExtents.m_pArray[i]);
            m_aControls.m_pArray[i]->m_pContainer = 0;
        }
    }

    m_pMouseOverControl = 0;
    m_aControls.m_count = 0;
    m_aExtents.m_count = 0;
    m_nSelectedIndex = -1;
    m_nOversizedStep = 1;
}


CSWGuiControl* CSWGuiListBox::GetControl(int nIndex)
{
    if (nIndex < 0 || nIndex >= m_aControls.m_count)
        return 0;
    return m_aControls.m_pArray[nIndex];
}

CSWGuiControl* CSWGuiListBox::GetSelectedControl()
{
    if (m_nSelectedIndex < 0 || m_nSelectedIndex >= m_aControls.m_count)
        return 0;
    return m_aControls.m_pArray[m_nSelectedIndex];
}


void CSWGuiListBox::SetActiveControl(CSWGuiControl* pControl, int flag)
{
    int nIndex = -1;
    int nCount = m_aControls.m_count;
    if (nCount > 0)
    {
        int64_t i = 0;
        do
        {
            if (m_aControls.m_pArray[i] == pControl)
            {
                nIndex = static_cast<int>(i);
                break;
            }
        } while (++i != nCount);
    }
    SetSelectedControl(nIndex, flag);
}

void CSWGuiListBox::SetSelectedControl(CSWGuiControl* pControl, int flag)
{
    int nIndex = -1;
    int nCount = m_aControls.m_count;
    if (nCount > 0)
    {
        int64_t i = 0;
        do
        {
            if (m_aControls.m_pArray[i] == pControl)
            {
                nIndex = static_cast<int>(i);
                break;
            }
        } while (++i != nCount);
    }
    SetSelectedControl(nIndex, flag);
}

void CSWGuiListBox::SetSelectedControl(int nIndex, int bPlaySound)
{
    if (m_nSelectedIndex != -1)
        m_aControls.m_pArray[m_nSelectedIndex]->HandleInputEvent(SWGUIEVENT_UNFOCUS, 1);

    if (nIndex < 0 || nIndex >= m_aControls.m_count)
    {
        m_nSelectedIndex = -1;
    }
    else
    {
        m_nSelectedIndex = static_cast<short>(nIndex);
        int nSelected = m_nSelectedIndex;
        m_aControls.m_pArray[nSelected]->HandleInputEvent(SWGUIEVENT_FOCUS, 1);
        if (bPlaySound)
        {
            int nStillSelected = m_nSelectedIndex;
            static_cast<CSWGuiPanel*>(m_pContainer)->m_pGuiManager->PlayGuiSound(
                m_aControls.m_pArray[nStillSelected]->m_tail.m_nActivateSound);
        }
    }

    bScrollToSelection = 1;
    OrganizeControls();
    m_scrollBar.SetCurValue(m_nTopRow);
}

void CSWGuiListBox::SetActive(int flag)
{
    m_tail.bActive = flag;
    if (!m_tail.bActive)
        return;

    if (m_nSelectedIndex == -1 && !bNoSelection)
        SetSelectedControl(0, 0);
    else
        OrganizeControls();
}

CSWGuiControl* CSWGuiListBox::HitCheckMouseLocal(int* pnIndex)
{
    int nMouseX;
    int nMouseY;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);

    if (nMouseX >= m_scrollBar.m_extent.x
        && nMouseX <= m_scrollBar.m_extent.x + m_scrollBar.m_extent.w
        && nMouseY >= m_scrollBar.m_extent.y
        && nMouseY <= m_scrollBar.m_extent.y + m_scrollBar.m_extent.h)
    {
        if (pnIndex)
            *pnIndex = -1;
        return &m_scrollBar;
    }

    int nLocalX = nMouseX - m_innerExtent.x;
    int nLocalY = nMouseY - m_innerExtent.y;
    m_nHitLocalX = nLocalX;
    m_nHitLocalY = nLocalY;

    for (int i = 0; i < m_aControls.m_count; i++)
    {
        CSWGuiControl* pHit = m_aControls.m_pArray[i]->HitCheckMouse(nLocalX, nLocalY);
        if (pHit)
        {
            if (pnIndex)
                *pnIndex = i;
            return pHit;
        }
    }
    return 0;
}

int CSWGuiListBox::GetIsSelectable()
{
    if (!m_tail.bEnabled)
        return 0;

    CSWGuiListBox* pListBox = static_cast<CSWGuiListBox*>(AsSWGuiListBox());
    if (!pListBox)
        return 0;

    CSWGuiControl* pControl = pListBox->HitCheckMouseLocal(0);
    if (!pControl)
        return 0;

    return pControl->GetIsSelectable() != 0;
}

int CSWGuiListBox::DisplayToolTip()
{
    CSWGuiControl* pControl = HitCheckMouseLocal(0);
    if (pControl)
        return pControl->DisplayToolTip();
    return 0;
}

// Tracks hover, handing the tool-tip machinery over from the row the mouse just left to the row it
// just entered and arming the hover timer on the new one. The trailing AsScrollBar() is a discarded
// result; both of its edges return, so only the guard's test goes away, not the call.
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
extern bool g_bK2HoverMoved;
#endif
void CSWGuiListBox::HandleMouseMove()
{
    if (!m_tail.bEnabled)
        return;

#ifdef K2_UI_TOUCH
    CSWGuiControl* pControl = HitCheckMouseLocal(0);
#else
    int nIndex;
    CSWGuiControl* pControl = HitCheckMouseLocal(&nIndex);
#endif
    CSWGuiControl* pWasOver = m_pMouseOverControl;
    if (pControl != pWasOver)
    {
        if (pWasOver)
        {
            g_pGuiMan->RemoveToolTip();
            pWasOver->m_tail.bToolTipShown = 0;
        }
        m_pMouseOverControl = pControl;
        if (!pControl)
            return;
        g_pGuiMan->ResetToolTipTimer();
    }
    else if (!pControl)
    {
        return;
    }

    if (pControl->AsScrollBar())
        return;
#ifndef K2_UI_TOUCH
    // PC/console only: hovering moves the selection, which is how a mouse highlights a
    // conversation reply. Not for a click-to-select list or a list of radio buttons.
    if (bSelectOnLeftClick)
        return;
    if (m_aControls.m_count > 0 && m_aControls.m_pArray[0]->AsRadioButton())
        return;
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    if (pControl == pWasOver || !g_bK2HoverMoved)
        return;
#endif
    if (m_nSelectedIndex != nIndex)
        SetSelectedControl(nIndex, 1);
#endif
}

void CSWGuiListBox::HandleLMouseDown()
{
    int nMouseX;
    int nMouseY;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);
    m_nDragStartX = nMouseX;
    m_nDragStartY = nMouseY;

    // Where the drag starts from. A box whose tallest row does not fit scrolls a whole box at a
    // time (m_nOversizedStep); everything else scrolls by the bar's own value.
    if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0 && !bUnequalRows)
        m_nDragStartScroll = m_nOversizedStep;
    else
        m_nDragStartScroll = m_scrollBar.m_nCurValue;

    if (m_fScrollHintTime < 1.0f)
        m_fScrollHintTime += 1.0f;

#if defined(K2_UI_PC)
    // pc: an arrow steps a line and the trough either side of the thumb pages, repeating from Draw
    // while held; the thumb drags.
    m_nCapturedScrollZone = m_scrollBar.HitCheckScrollbar(nMouseX, nMouseY);
    switch (m_nCapturedScrollZone)
    {
    case 1: m_nRepeatEvent = SWGUIEVENT_SCROLL_LINE_UP; break;
    case 2: m_nRepeatEvent = SWGUIEVENT_SCROLL_LINE_DOWN; break;
    case 4: m_nRepeatEvent = SWGUIEVENT_SCROLL_PAGE_UP; break;
    case 5: m_nRepeatEvent = SWGUIEVENT_SCROLL_PAGE_DOWN; break;
    default: m_nRepeatEvent = 0; break;
    }
    if (m_nRepeatEvent != 0 && m_tail.bEnabled)
    {
        m_fRepeatTimer = 0.5f;
        HandleInputEvent(m_nRepeatEvent, 1);
        g_pGuiMan->CaptureMouse(this, 1);
        return;
    }
#endif

    int nIndex;
    HitCheckMouseLocal(&nIndex);

    if (m_tail.bEnabled)
    {
        CSWGuiControl* pCaptured = g_pGuiMan->m_pMouseOverControl;
        if (pCaptured) {
            CSWGuiControl* pMatch = pCaptured;
            if (pCaptured != this)
                pMatch = pCaptured->GetSelectableParent();
            if (pMatch == this && (g_pGuiMan->m_nMouseStateFlags & 1))
                HandleInputEvent(SWGUIEVENT_DOUBLE_CLICK, 1);
        }
        m_nPressedIndex = nIndex;
        HandleInputEvent(SWGUIEVENT_SELECTION_CHANGED, 1);
    }

    g_pGuiMan->CaptureMouse(this, 1);
}


// The Aspyr port shim reports the screen size and whether the port is running phone-sized.
// SetPadding uses it to override a GFF padding of 0 to 6 so the rows do not touch the border, and
// HandleMouseCapturedMovement to pick the touch drag dead zone.
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);

// The touch drag dead zone is 2 px normally, 4 px on a phone. Both are port-layer tunables, each
// referenced once, by HandleMouseCapturedMovement.
#if KOTOR1
// K1 keeps the four touch tunables in this unit's data, in this order.
__attribute__((visibility("hidden"))) int g_nTouchCursorSize = 238;
__attribute__((visibility("hidden"))) int g_nKeyboardTop = 350;
__attribute__((visibility("hidden"))) int g_nTouchDragThresholdIphone = 4;
__attribute__((visibility("hidden"))) int g_nTouchDragThreshold = 2;
#else
__attribute__((visibility("hidden"))) int g_nTouchCursorSize = 238;
__attribute__((visibility("hidden"))) int g_nKeyboardTop = 350;
__attribute__((visibility("hidden"))) int g_nTouchDragThreshold = 2;
__attribute__((visibility("hidden"))) int g_nTouchDragThresholdIphone = 4;
#endif

// The renderer's printf-style trace sink.
extern void AurPrintf(const char* pszFormat, ...);

// The drag half of the click pair. HandleLMouseDown latched the mouse-down point in
// m_nDragStartX/m_nDragStartY and the scroll origin in m_nDragStartScroll; this runs on every mouse move while the
// list box holds the capture and re-derives the scroll position from how far the pointer has
// travelled since. Every exit returns 1: the early-outs mean "not a drag yet", not "not handled".
//
// On touch, the front of this body is port code. GetLocalMouseCoords runs once for the dead zone
// (both axes must clear it) and again for the real work, and the scroll delta is halved so the
// content follows the finger.

int CSWGuiListBox::HandleMouseCapturedMovement(int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    int nMouseX;
    int nMouseY;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);

    int nTravelX = nMouseX - m_nDragStartX;
    if (nTravelX < 0)
        nTravelX = -nTravelX;
    int nTravelY = nMouseY - m_nDragStartY;
    if (nTravelY < 0)
        nTravelY = -nTravelY;

    int nScreenWidth;
    int nScreenHeight;
    int nThreshold = g_nTouchDragThreshold;
    if (IosIsIphone(nScreenWidth, nScreenHeight))
        nThreshold = g_nTouchDragThresholdIphone;
#if defined(K2_UI_PC)
    // pc: an arrow or trough press repeats from Draw; the thumb follows the pointer at once
    if (m_nCapturedScrollZone != 0 && m_nCapturedScrollZone != 3)
        return 1;
    if (m_nCapturedScrollZone == 3)
        nThreshold = 0;
#endif

    if (nTravelX < nThreshold || nTravelY < nThreshold)
        return 1;

    if (!(g_pGuiMan->m_nCaptureFlags & 1))
        return 1;

    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);

    if (m_scrollBar.m_extent.h <= m_scrollBar.m_extent.w)
        return 1;

    // Thumb travel is the denominator that turns a pixel drag into a fraction of the range.
#if KOTOR1  // K1: a Chromebook drags the list the other way (pointer, not touch)
    int nHalf = (nMouseY - m_nDragStartY) / 2;
    int nDeltaY = SDL_IsChromebook() ? nHalf : -nHalf;
#if defined(K2_UI_PC)
    if (m_nCapturedScrollZone == 3)
        nDeltaY = nMouseY - m_nDragStartY;
#endif
#else
    int nDeltaY = -((nMouseY - m_nDragStartY) / 2);
#if defined(K2_UI_PC)
    if (m_nCapturedScrollZone == 3)
        nDeltaY = nMouseY - m_nDragStartY;
#endif
#endif
    int nThumbHeight = m_scrollBar.m_imageThumb.m_extent.h;
    int nTrackLength = m_scrollBar.m_border.GetInnerExtent().h - nThumbHeight;

    if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0 && !bUnequalRows)
    {
        // A row taller than the box scrolls a whole box at a time, via m_nOversizedStep.
        m_nOversizedStep = static_cast<short>(static_cast<int>(static_cast<float>(nDeltaY) / static_cast<float>(nTrackLength)
                                 * (GetOversizedStepCount() - 1) + m_nDragStartScroll + 0.5f));
        if (m_nOversizedStep < 1)
            m_nOversizedStep = 1;
        m_nOversizedStep = static_cast<short>((m_nOversizedStep > GetOversizedStepCount())
                         ? GetOversizedStepCount() : m_nOversizedStep);
        OrganizeOversized();
        AurPrintf("pos %d/%d, dY %d (%d-%d)", m_nOversizedStep, GetOversizedStepCount(), nDeltaY,
                  nMouseY, m_nDragStartY);
    }
    else
    {
        // A drag scrolls freely, so whatever the selection was, stop chasing it.
        bScrollToSelection = 0;

        int nTopRow = static_cast<int>(static_cast<float>(nDeltaY) / static_cast<float>(nTrackLength)
                            * (m_scrollBar.m_nMaxValue - 1) + m_nDragStartScroll + 0.5f);
        if (nTopRow < 0)
            nTopRow = 0;
        nTopRow = (m_scrollBar.m_nMaxValue > nTopRow) ? nTopRow
                                                      : m_scrollBar.m_nMaxValue - 1;
        m_nTopRow = static_cast<short>(nTopRow);
        m_scrollBar.SetCurValue(nTopRow);
        OrganizeControls();
    }

    return 1;
}

// Starting size of the touch cursor, read once, here; defined in the port layer.
extern __attribute__((visibility("hidden"))) int g_nTouchCursorSize;

void CSWGuiListBox::Draw(float fDelta)
{
    int nCount = m_aControls.m_count;

    if (bShowScrollBar)
    {
        if (bPulseArrows)
            m_scrollBar.m_imageDir.m_params.m_fAlpha = m_fArrowAlpha;
        m_scrollBar.Draw(fDelta);
    }

    m_border.Draw(fDelta);

    if (AurGUISetupViewport(m_innerExtent.x, m_innerExtent.y, m_innerExtent.w, m_innerExtent.h,
                            m_vColor, false, 1.0f))
    {
        if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0 && !bUnequalRows)
        {
            m_aControls.m_pArray[0]->Draw(fDelta);
        }
        else
        {
            for (int i = 0; i < nCount; i++)
            {
                CSWGuiControl* pControl = m_aControls.m_pArray[i];
                int nTop = pControl->m_extent.y;
                int nBottom = nTop + pControl->m_extent.h;
                if (nBottom > 0 && nTop <= m_innerExtent.h
                    && (!bClipPartialRows || (nTop >= 0 && nBottom < m_innerExtent.h)))
                    pControl->Draw(fDelta);
            }
        }
        AurGUICloseViewport();
    }

    // For auto-repeat, holding an arrow or a page zone keeps raising its input event as long as the
    // pointer has not wandered off the zone it went down in.
    if (g_pGuiMan->m_pMouseCaptureObject == this && m_nRepeatEvent != 0)
    {
        int nMouseX;
        int nMouseY;
        static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);
        if (m_scrollBar.HitCheckScrollbar(nMouseX, nMouseY) == m_nCapturedScrollZone)
        {
            m_fRepeatTimer -= fDelta;
            while (m_fRepeatTimer < 0.0f)
            {
                m_fRepeatTimer += 0.1f;
                HandleInputEvent(m_nRepeatEvent, 1);
            }
        }
    }

#ifdef K2_UI_TOUCH
    // The touch UI's "swipe here" hint, a pulsing `ios_touch_cursor` over a list box that has more
    // rows than it shows. It is port code (the macOS build has no such tail), so pc and xbox skip
    // it.
    int nPulsesShown = s_nScrollPulsesShown;
    if (m_pContainer && m_pContainer->AsSWGuiPanel())
    {
        CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
        if (pPanel->m_pGuiManager->IsOnTop(pPanel)
            && m_fScrollHintTime < ((nPulsesShown > 10) ? 2.0f : 3.0f)
            && nCount > m_nVisibleRows)
        {
            int nX = m_extent.x;
            int nY = m_extent.y;
            int nWidth = m_extent.w;
            int nHeight = m_extent.h;

            // A square centred on the control, never wider than it. The clamp writes back into
            // the static, so it only ever shrinks.
            static int s_nTouchCursorSize = g_nTouchCursorSize;
            if (s_nTouchCursorSize > nWidth)
                s_nTouchCursorSize = nWidth;
            if (s_nTouchCursorSize > nHeight)
                s_nTouchCursorSize = nHeight;

            CSWGuiExtent extentCursor;
            extentCursor.w = s_nTouchCursorSize;
            extentCursor.h = s_nTouchCursorSize;
            extentCursor.x = nX + nWidth / 2 - s_nTouchCursorSize / 2;
            extentCursor.y = nY + nHeight / 2 - s_nTouchCursorSize / 2;
            m_imageArrow.SetExtent(extentCursor);

            m_imageArrow.m_params.m_fAlpha =
                ((nPulsesShown > 10) ? 0.35f : 0.5f) * fabs(sin(m_fScrollHintTime * 3.141));
            m_imageArrow.Draw(fDelta);
            m_fScrollHintTime += fDelta;
        }
    }
#endif
}

void CSWGuiListBox::AddControls(CExoArrayList<CSWGuiControl*>& aControls, int bOwnsControls,
                                int bRestoreExtent, int bUnequal)
{
    int nCount = aControls.m_count;
    ClearItems();

    bOwnsItems = bOwnsControls;
    bRestoreItemExtent = bRestoreExtent;

    int nId = 0;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiControl* pControl = aControls.m_pArray[i];
        if (pControl)
        {
#if !KOTOR1
            pControl->m_bCenterOnNativeRect = m_bCenterOnNativeRect;
#endif

            int nFound = -1;
            for (int64_t j = 0; j < m_aControls.m_count; j++)
            {
                if (m_aControls.m_pArray[j] == pControl)
                {
                    nFound = static_cast<int>(j);
                    break;
                }
            }

            if (nFound == -1)
            {
                pControl->m_pContainer = this;
                pControl->m_tail.nControlId = nId++;
                m_aControls.Add(pControl);
#ifdef K2_QOL_IMPROVEMENTS
                K2QolGrowListRow(this, pControl, nCount);   // before its extent is recorded
#endif
                m_aExtents.Add(pControl->m_extent);
            }
        }
    }

    bUnequalRows = bUnequal;
    bLayoutDirty = 1;
    OrganizeControls();
}

void CSWGuiListBox::AddControls(CSWGuiControl** ppControls, int nCount, int bOwnsControls,
                                int bRestoreExtent, int bUnequal)
{
    ClearItems();

    bOwnsItems = bOwnsControls;
    bRestoreItemExtent = bRestoreExtent;

    int nId = 0;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiControl* pControl = ppControls[i];

        int nFound = -1;
        for (int64_t j = 0; j < m_aControls.m_count; j++)
        {
            if (m_aControls.m_pArray[j] == pControl)
            {
                nFound = static_cast<int>(j);
                break;
            }
        }

        if (nFound == -1)
        {
            pControl->m_pContainer = this;
            pControl->m_tail.nControlId = nId++;
            m_aControls.Add(pControl);
#ifdef K2_QOL_IMPROVEMENTS
            K2QolGrowListRow(this, pControl, nCount);   // before its extent is recorded
#endif
            m_aExtents.Add(pControl->m_extent);
        }
    }

    bUnequalRows = bUnequal;
    bLayoutDirty = 1;
    OrganizeControls();
}

void CSWGuiListBox::HandleInputEvent(int nType, int nVal)
{
    if (nVal)
    {
        int bMoved = 0;
        switch (nType)
        {
        // ---- one row up ----
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            if (m_nSelectedIndex == -1)
            {
                if (bNoSelection)
                {
                    bMoved = (m_nTopRow != 0);
                    m_nTopRow = (m_nTopRow - 1 > 0) ? m_nTopRow - 1 : 0;
                    OrganizeControls();
                }
                else
                {
                    if (m_aControls.m_count == 0)
                        break;
                    SetSelectedControl(0, 0);
                    bMoved = 1;
                }
            }
            else if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                     && !bUnequalRows)
            {
                int nOldStep = m_nOversizedStep;
                bMoved = (nOldStep != 1);
                m_nOversizedStep = static_cast<short>((nOldStep - 1 < 1) ? 1 : nOldStep - 1);
                OrganizeOversized();
            }
            else if (m_nSelectedIndex - 1 >= 0)
            {
                SetSelectedControl(m_nSelectedIndex - 1, 1);
                bMoved = 1;
            }
            else if (bLooping)
            {
                SetSelectedControl(m_aControls.m_count - 1, 1);
                // A held key would otherwise wrap the list round and round.
                if (m_pContainer && m_pContainer->AsSWGuiPanel())
                    g_pExoInput->ClearEventRepeat(
                        m_pContainer->AsSWGuiPanel()->m_pGuiManager->m_nCurrentInputEvent);
                bMoved = 1;
            }
            break;

        // ---- one row down ----
        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            if (m_nSelectedIndex == -1)
            {
                if (bNoSelection)
                {
                    int nMaxTopRow = 0;
                    if (m_aControls.m_count > m_nVisibleRows)
                        nMaxTopRow = m_aControls.m_count - m_nVisibleRows;
                    int nOldTopRow = m_nTopRow;
                    bMoved = (nOldTopRow != nMaxTopRow);
                    m_nTopRow = static_cast<short>((nOldTopRow + 1 > nMaxTopRow) ? nMaxTopRow : nOldTopRow + 1);
                    OrganizeControls();
                }
                else
                {
                    if (m_aControls.m_count == 0)
                        break;
                    SetSelectedControl(0, 1);
                    bMoved = 1;
                }
            }
            else if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                     && !bUnequalRows)
            {
                bMoved = (m_nOversizedStep != GetOversizedStepCount());
                m_nOversizedStep = static_cast<short>((GetOversizedStepCount() >= m_nOversizedStep + 1)
                                 ? m_nOversizedStep + 1 : GetOversizedStepCount());
                OrganizeOversized();
            }
            else if (m_nSelectedIndex < m_aControls.m_count - 1)
            {
                SetSelectedControl(m_nSelectedIndex + 1, 1);
                bMoved = 1;
            }
            else if (bLooping)
            {
                SetSelectedControl(0, 1);
                if (m_pContainer && m_pContainer->AsSWGuiPanel())
                    g_pExoInput->ClearEventRepeat(
                        m_pContainer->AsSWGuiPanel()->m_pGuiManager->m_nCurrentInputEvent);
                bMoved = 1;
            }
            break;

        // ---- scroll bar: line up ----
        case SWGUIEVENT_WHEEL_UP:
        case SWGUIEVENT_SCROLL_LINE_UP:
            if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                && !bUnequalRows)
            {
                m_nOversizedStep = (m_nOversizedStep - 1 < 1) ? 1 : m_nOversizedStep - 1;
                OrganizeOversized();
            }
            else if (m_nTopRow > 0)
            {
                m_nTopRow--;
                bScrollToSelection = 0;
                m_scrollBar.SetCurValue(m_nTopRow);
                OrganizeControls();
            }
            break;

        // ---- scroll bar: line down ----
        case SWGUIEVENT_WHEEL_DOWN:
        case SWGUIEVENT_SCROLL_LINE_DOWN:
            if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                && !bUnequalRows)
            {
                m_nOversizedStep = static_cast<short>((GetOversizedStepCount() >= m_nOversizedStep + 1)
                                 ? m_nOversizedStep + 1 : GetOversizedStepCount());
                OrganizeOversized();
            }
            else if (m_nTopRow < m_scrollBar.m_nMaxValue - 1
                     && m_nBottomRow < m_scrollBar.m_nMaxValue - 1 + m_nVisibleRows)
            {
                m_nTopRow++;
                bScrollToSelection = 0;
                m_scrollBar.SetCurValue(m_nTopRow);
                OrganizeControls();
            }
            break;

        // ---- scroll bar: page up ----
        case SWGUIEVENT_SCROLL_PAGE_UP:
            if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                && !bUnequalRows)
            {
                // A page of an oversized row is m_fVisibleLines whole-box steps.
                m_nOversizedStep = static_cast<short>(m_nOversizedStep - static_cast<int>(m_fVisibleLines));
                if (m_nOversizedStep < 1)
                    m_nOversizedStep = 1;
                OrganizeOversized();
            }
            else if (m_nTopRow > 0)
            {
                m_nTopRow = static_cast<short>(m_nTopRow - m_scrollBar.m_nVisibleValue);
                if (m_nTopRow < 0)
                    m_nTopRow = 0;
                bScrollToSelection = 0;
                m_scrollBar.SetCurValue(m_nTopRow);
                OrganizeControls();
            }
            break;

        // ---- scroll bar: page down ----
        case SWGUIEVENT_SCROLL_PAGE_DOWN:
            if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0
                && !bUnequalRows)
            {
                m_nOversizedStep = static_cast<short>(m_nOversizedStep + static_cast<int>(m_fVisibleLines));
                m_nOversizedStep = static_cast<short>((m_nOversizedStep > GetOversizedStepCount())
                                 ? GetOversizedStepCount() : m_nOversizedStep);
                OrganizeOversized();
            }
            else if (m_nTopRow < m_scrollBar.m_nMaxValue - 1
                     && m_nBottomRow < m_scrollBar.m_nMaxValue - 1 + m_nVisibleRows)
            {
                // On a short last page, never scroll past the last row.
                if (m_scrollBar.m_nMaxValue - (m_nBottomRow + 1) >= m_scrollBar.m_nVisibleValue)
                    m_nTopRow = static_cast<short>(m_nTopRow + m_scrollBar.m_nVisibleValue);
                else if (m_scrollBar.m_nMaxValue - (m_nBottomRow + 1) > 0)
                    m_nTopRow = static_cast<short>(m_nTopRow + (m_scrollBar.m_nMaxValue - (m_nBottomRow + 1)));

                m_nTopRow = static_cast<short>((m_scrollBar.m_nMaxValue > m_nTopRow) ? m_nTopRow
                                                                  : m_scrollBar.m_nMaxValue - 1);
                bScrollToSelection = 0;
                m_scrollBar.SetCurValue(m_nTopRow);
                OrganizeControls();
            }
            break;
        }

        if (bMoved && m_nScrollSound != GUISOUND_NONE && m_pContainer && m_pContainer->AsSWGuiPanel())
            m_pContainer->AsSWGuiPanel()->m_pGuiManager->PlayGuiSound(m_nScrollSound);
    }

    if (m_nSelectedIndex != -1)
        m_aControls.m_pArray[m_nSelectedIndex]->HandleInputEvent(nType, nVal);

    CSWGuiControl::HandleInputEvent(nType, nVal);
}

void CSWGuiListBox::Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& borderParams,
                               int nPadding)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
    CSWGuiExtent extentBorder = m_extent;
#else
    m_extent = extent;

    CSWGuiExtent extentBorder = extent;
#endif
    if (bScrollBarOnLeft)
        extentBorder.x += m_scrollBar.m_extent.w;
    extentBorder.w -= m_scrollBar.m_extent.w;
    m_border.Initialize(extentBorder, borderParams);
    m_innerExtent = m_border.GetInnerExtent();

    SetPadding(nPadding);
}

void CSWGuiListBox::SetPadding(int nPadding)
{
    int nScreenWidth;
    int nScreenHeight;
    bool bIsIphone = IosIsIphone(nScreenWidth, nScreenHeight);

    if (m_nPadding > m_innerExtent.w / 2 || m_nPadding > m_innerExtent.h / 2)
        return;

#if KOTOR1 && !defined(K2_UI_TOUCH)
    // pc / xbox: the 6-unit gap is the phone default; the PC lineage keeps the layout's padding.
    (void)bIsIphone;
    m_nPadding = static_cast<unsigned char>(nPadding);
#else
    m_nPadding = static_cast<unsigned char>((nPadding == 0 && bIsIphone) ? 6 : nPadding);
#endif
    bLayoutDirty = 1;
    OrganizeControls();
}

void CSWGuiListBox::Initialize(const CSWGuiExtent& extent, CSWGuiListBox* pTemplate)
{
    Initialize(extent, pTemplate->m_border.m_params, pTemplate->m_nPadding);
    // Called through a pointer on purpose. Called on the member directly, the derived override runs
    // instead of the base one. Not every call in this class is written this way -
    // SetScrollBarOnLeft's SetExtent really is spelled on the member - so read each call site
    // before changing it.
    CSWGuiScrollBar* pScrollBar = &m_scrollBar;
    pScrollBar->Initialize(pTemplate->m_scrollBar.m_extent,
                           pTemplate->m_scrollBar.m_border.m_params,
                           pTemplate->m_scrollBar.m_imageDir.m_params.m_resRefImage,
                           pTemplate->m_scrollBar.m_imageThumb.m_params.m_resRefImage);
}

void CSWGuiListBox::InitializeScrollBar(CSWGuiExtent& extent, const CSWGuiBorderParams& borderParams,
                                        const CResRef& resRefDir, const CResRef& resRefThumb,
                                        int bOnLeft)
{
    bScrollBarOnLeft = bOnLeft;

    if (extent.w > m_extent.w)
        extent.w = m_extent.w;
    extent.y = m_extent.y;
    extent.h = m_extent.h;
    extent.x = bScrollBarOnLeft ? m_extent.x : m_extent.x + m_extent.w - extent.w;
#if KOTOR1  // K1: the base setter is out of line
    m_scrollBar.CSWGuiObject::SetExtent(extent);
#else
    m_scrollBar.m_extent = extent;
#endif
    m_scrollBar.m_border.Initialize(m_scrollBar.m_extent, borderParams);

    CResRef resRefNone;
    CSWGuiImageParams paramsImage(resRefNone, 0, 0, 0.0f, 18, 1.0f, Vector(1.0f, 1.0f, 1.0f));
    paramsImage.SetImage(resRefDir, 0);
    m_scrollBar.m_imageDir.Initialize(m_scrollBar.m_extent, paramsImage);
    paramsImage.SetImage(resRefThumb, 0);
    m_scrollBar.m_imageThumb.Initialize(m_scrollBar.m_extent, paramsImage);
    // Through a pointer again, and this one is a real virtual dispatch, unlike the direct call the
    // same member takes in SetScrollBarOnLeft. See the note on Initialize(extent, CSWGuiListBox*).
    CSWGuiScrollBar* pScrollBar = &m_scrollBar;
    pScrollBar->SetExtent(m_scrollBar.m_extent);

    int nVisible = m_nVisibleRows > 1 ? m_nVisibleRows : 1;
    int nCount = m_aControls.m_count;
#if KOTOR1
    int nOver = nCount - nVisible;
    m_scrollBar.SetMaxValue((nOver > 0 ? nOver : 0) + 1, bNoSelection);
#else
    m_scrollBar.SetMaxValue(nCount > nVisible ? nCount - nVisible + 1 : 1, bNoSelection);
#endif

    SetExtent(m_extent);
}

void CSWGuiListBox::SetScrollBarOnLeft(int bOnLeft)
{
    if (bScrollBarOnLeft == static_cast<unsigned int>(bOnLeft))
        return;
    bScrollBarOnLeft = bOnLeft;

    if (m_extent.w == 0)
        return;

    int nWidth = m_scrollBar.m_extent.w;
    if (m_extent.w < nWidth)
        nWidth = m_extent.w;

    CSWGuiExtent extent;
    extent.y = m_extent.y;
    extent.w = nWidth;
    extent.h = m_extent.h;
    extent.x = m_extent.x + (bScrollBarOnLeft ? 0 : m_extent.w - nWidth);
    m_scrollBar.SetExtent(extent);

    SetExtent(m_extent);
}

void CSWGuiListBox::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_border.Load(pGFF, resStruct, CExoString("BORDER"));
    m_scrollBar.CSWGuiControl::Load(this, pGFF, resStruct, CExoString("SCROLLBAR"));

    int bSuccess;
    SetScrollBarOnLeft(pGFF->ReadFieldBYTE(&resStruct, const_cast<char*>("LEFTSCROLLBAR"), bSuccess, 1));
#if KOTOR1 && !defined(K2_UI_TOUCH)
    extern int K1ScaleLoadedPadding(int nPadding, CResGFF* pGFF);
    // The frame's DIMENSION is the inset the rows sit inside, in layout units as the padding is.
    m_border.m_params.m_nBorderDim = K1ScaleLoadedPadding(m_border.m_params.m_nBorderDim, pGFF);
    SetPadding(K1ScaleLoadedPadding(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("PADDING"), bSuccess, 0),
                                    pGFF));
#else
    SetPadding(pGFF->ReadFieldINT(&resStruct, const_cast<char*>("PADDING"), bSuccess, 0));
#endif

    m_vColor = pGFF->ReadFieldVector(&resStruct, const_cast<char*>("COLOR"), bSuccess, Vector(0.0f, 0.0f, 0.0f));

    bLooping = pGFF->ReadFieldBYTE(&resStruct, const_cast<char*>("LOOPING"), bSuccess, 0);

    CSWGuiNavigable::Load(pGFF, resStruct);
    LoadProtoItem(pGFF, resStruct);
}

void CSWGuiListBox::LoadProtoItem(CResGFF* pGFF, CResStruct& resStruct)
{
    CResStruct protoStruct;
    if (!pGFF->GetStructFromStruct(&protoStruct, &resStruct, const_cast<char*>("PROTOITEM")))
        return;

    int bSuccess;
    m_nProtoItemType = pGFF->ReadFieldINT(&protoStruct, const_cast<char*>("CONTROLTYPE"), bSuccess, 0);
    switch (m_nProtoItemType) {
    case SWGUICONTROL_LABEL:
        m_pProtoItem = new CSWGuiLabel;
        break;
    case SWGUICONTROL_LABEL_HILIGHT:
        m_pProtoItem = new CSWGuiLabelHilight();
        break;
    case SWGUICONTROL_BUTTON:
        m_pProtoItem = new CSWGuiButton();
        break;
    case SWGUICONTROL_BUTTON_TOGGLE:
        m_pProtoItem = new CSWGuiButtonToggle;
        break;
    case SWGUICONTROL_SLIDER:
        m_pProtoItem = new CSWGuiSlider;
        break;
    }

#if KOTOR1 && !defined(K2_UI_TOUCH)
    extern int g_nK1LayoutScaleSuspend;
    g_nK1LayoutScaleSuspend++;
#endif
    if (m_pProtoItem)
        m_pProtoItem->Load(this, pGFF, resStruct, CExoString("PROTOITEM"));
#if KOTOR1 && !defined(K2_UI_TOUCH)
    g_nK1LayoutScaleSuspend--;
#endif
}

void CSWGuiListBox::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line; the scroll-bar width is read after it
    CSWGuiObject::SetExtent(extent);
    int nScrollBarWidth = m_scrollBar.m_extent.w;
#else
    int nScrollBarWidth = m_scrollBar.m_extent.w;
    m_extent = extent;
#endif

    m_border.m_extent.x = m_extent.x + (bScrollBarOnLeft ? nScrollBarWidth : 0);
    m_border.m_extent.y = m_extent.y;
    m_border.m_extent.w = m_extent.w - nScrollBarWidth;
    m_border.m_extent.h = m_extent.h;

    m_innerExtent = m_border.GetInnerExtent();

    if (m_extent.w != 0)
    {
        CSWGuiExtent scrollBarExtent;
        scrollBarExtent.w = m_extent.w < nScrollBarWidth ? m_extent.w : nScrollBarWidth;
        scrollBarExtent.y = m_extent.y;
        scrollBarExtent.h = m_extent.h;
        scrollBarExtent.x = m_extent.x + (bScrollBarOnLeft ? 0 : m_extent.w - scrollBarExtent.w);
        m_scrollBar.SetExtent(scrollBarExtent);
    }

    bLayoutDirty = 1;
    OrganizeControls();
}

void CSWGuiListBox::SetScrollBarExtent(int nWidth, int bReapplyExtent)
{
    CSWGuiExtent scrollBarExtent;
    if (m_extent.w == 0)
        return;

    scrollBarExtent.w = m_extent.w < nWidth ? m_extent.w : nWidth;
    scrollBarExtent.y = m_extent.y;
    scrollBarExtent.h = m_extent.h;
    scrollBarExtent.x = m_extent.x + (bScrollBarOnLeft ? 0 : m_extent.w - scrollBarExtent.w);
    m_scrollBar.SetExtent(scrollBarExtent);

    if (bReapplyExtent)
        SetExtent(m_extent);
}

void CSWGuiListBox::ReSetFont()
{
    for (int i = 0; i < m_aControls.m_count; i++)
        m_aControls.m_pArray[i]->ReSetFont();
    if (m_pProtoItem)
        m_pProtoItem->ReSetFont();
}

// The three arrow-pulse statics. Class-scoped, so the pulse is shared by every list box on screen
// rather than running per instance.
float CSWGuiListBox::m_fArrowAlpha = 1.0f;
float CSWGuiListBox::m_fArrowAlphaPulseTime = 0.75f;
unsigned char CSWGuiListBox::m_bArrowAlphaPulseUp;

void CSWGuiListBox::UpdateArrowPulsing(float fElapsed)
{
    m_fArrowAlphaPulseTime -= fElapsed;
    if (m_fArrowAlphaPulseTime <= 0.0f)
    {
        m_bArrowAlphaPulseUp = (m_bArrowAlphaPulseUp == 0);
        m_fArrowAlphaPulseTime = 0.75f;
    }

    float fAlpha = (m_bArrowAlphaPulseUp == 0) ? 1.0f : 0.2f;
    float fRange = (m_bArrowAlphaPulseUp == 0) ? -0.8f : 0.8f;
    m_fArrowAlpha = fAlpha + fRange * ((0.75f - m_fArrowAlphaPulseTime) / 0.75f);
}

void CSWGuiListBox::OrganizeOversized()
{
    CSWGuiExtent extent;
    extent.x = m_nPadding;
    extent.h = m_nRowHeight;
    extent.w = m_innerExtent.w - 2 * m_nPadding;

    int nSteps = GetOversizedStepCount();

    int nCurValue;
    if (nSteps == m_nOversizedStep)
    {
        // As the last step, line the control's bottom edge up with the inner rectangle's.
        extent.y = m_innerExtent.h - m_nRowHeight - m_nPadding;
        nCurValue = m_nRowHeight;
    }
    else
    {
        extent.y = m_nPadding
                 + static_cast<int>(static_cast<float>(-((m_nOversizedStep - 1) * m_innerExtent.h)) / m_fVisibleLines);
        if (m_nOversizedStep == 1)
            nCurValue = 0;
        else
            nCurValue = static_cast<int>(static_cast<float>(m_nOversizedStep - 1) / static_cast<float>(nSteps - 1)
                              * static_cast<float>(m_nRowHeight - m_innerExtent.h));
    }
    m_scrollBar.SetCurValue(nCurValue);

    m_aControls.m_pArray[0]->SetExtent(extent);
}

void CSWGuiListBox::OrganizeControls()
{
    int nCount = m_aControls.m_count;

    if (nCount == 0)
    {
        if (bLayoutDirty)
        {
            m_scrollBar.SetMaxValue(1, 0);
            m_scrollBar.SetCurValue(1);
            m_scrollBar.SetVisibleValue(1);
            bLayoutDirty = 0;
        }
        return;
    }

    if (bUnequalRows)
    {
        OrganizeUnequal();
        return;
    }

    int nRowPitch;
    if (bLayoutDirty)
    {
        // Re-apply every row's stored extent and remeasure the tallest one.
        m_nRowHeight = 0;
        for (int i = 0; i < nCount; i++)
        {
            m_aControls.m_pArray[i]->SetExtent(m_aExtents.m_pArray[i]);
#ifdef K2_FIX_ENGINE_BUGS
            // A text row sized to its text at another width (an unscaled prototype's) is re-sized
            // at the width it is laid out at below, or it keeps the narrow wrap's extra lines.
            CSWGuiControl* pLabel = m_aControls.m_pArray[i]->AsLabel();
            CSWGuiExtent extentRow = m_aExtents.m_pArray[i];
            if (pLabel && extentRow.w != m_innerExtent.w - 2 * m_nPadding &&
                static_cast<CSWGuiLabel*>(pLabel)->m_text.GetIdealHeight() == extentRow.h)
            {
                extentRow.w = m_innerExtent.w - 2 * m_nPadding;
                pLabel->SetExtent(extentRow);
                extentRow.h = static_cast<CSWGuiLabel*>(pLabel)->m_text.GetIdealHeight();
                pLabel->SetExtent(extentRow);
            }
#endif
            int nIdeal = m_aControls.m_pArray[i]->GetIdealHeight();
            if (nIdeal > m_nRowHeight)
                m_nRowHeight = nIdeal;
        }

        m_nTopRow = 0;
        if (m_nRowHeight == 0)
        {
            m_scrollBar.SetMaxValue(1, 0);
            m_scrollBar.SetCurValue(1);
            m_scrollBar.SetVisibleValue(1);
            return;
        }

        nRowPitch = m_nRowHeight + m_nPadding;
        m_nVisibleRows = static_cast<short>(m_innerExtent.h / nRowPitch);
        if (m_nVisibleRows == 0)
        {
            // Not even one row fits, so the bar scrolls pixels, not rows.
            m_scrollBar.SetMaxValue(m_nRowHeight - m_innerExtent.h + 1, bNoSelection);
            m_scrollBar.SetVisibleValue(m_innerExtent.h);
        }
        else
        {
            int nHidden = nCount - m_nVisibleRows;
            int bStretch = bNoSelection;
            m_scrollBar.SetMaxValue(nHidden > 0 ? nHidden + 1 : 1, static_cast<unsigned char>(bStretch));
            if (bStretch)
                m_scrollBar.SetVisibleValue(1);
            else
                m_scrollBar.SetVisibleValue(m_nVisibleRows > nCount ? nCount : m_nVisibleRows);
        }

        // If a single row already overflows the box, re-derive how many text lines the box holds
        // from the first row's font, which OrganizeOversized scrolls by.
        if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0 && !bUnequalRows)
        {
            CSWGuiControl* pLabel = m_aControls.m_pArray[0]->AsLabel();
            if (pLabel != 0)
            {
                int nFontHeight = static_cast<CSWGuiLabel*>(pLabel)->m_text.GetFontHeight();
                if (nFontHeight != 0)
                    m_fVisibleLines = static_cast<float>(m_innerExtent.h) / static_cast<float>(nFontHeight);
            }
        }

        bLayoutDirty = 0;
    }
    else
    {
        nRowPitch = m_nRowHeight + m_nPadding;
    }

    if (m_nRowHeight + m_nPadding > m_innerExtent.h && m_aControls.m_count > 0 && !bUnequalRows)
    {
        OrganizeOversized();
        return;
    }

    // Scroll the selection back into view.
    if (bScrollToSelection && m_nSelectedIndex != -1)
    {
        if (m_nSelectedIndex < m_nTopRow)
            m_nTopRow = m_nSelectedIndex;
        else if (m_nSelectedIndex >= m_nTopRow + m_nVisibleRows)
        {
            int nTop = m_nSelectedIndex - m_nVisibleRows + 1;
            m_nTopRow = static_cast<short>(nTop >= 0 ? nTop : 0);
        }
    }

    m_scrollBar.SetCurValue(m_nTopRow);

    CSWGuiExtent extent;
    extent.x = m_nPadding;
    extent.h = m_nRowHeight;
    extent.w = m_innerExtent.w - 2 * m_nPadding;

    int y = m_nPadding - m_nTopRow * nRowPitch;

    // Rows scrolled off the top are laid out at the plain pitch, above the inner rectangle.
    for (int i = 0; i < m_nTopRow; i++)
    {
        extent.y = y;
        m_aControls.m_pArray[i]->SetExtent(extent);
        y += nRowPitch;
    }

    int nEnd = m_nVisibleRows + m_nTopRow;
    if (nEnd > nCount)
        nEnd = nCount;

    // The visible rows share out the leftover height; each gets `nExtra` more, and the first
    // `nSlack % m_nVisibleRows` of them get one pixel more still.
    if (m_nTopRow < nEnd)
    {
        int nSlack = m_innerExtent.h - m_nPadding - m_nVisibleRows * nRowPitch;
        int nExtra = nSlack / m_nVisibleRows;
        int nRemainder = nSlack - nExtra * m_nVisibleRows;
        int nHeight = m_nRowHeight + nExtra;

        for (int i = m_nTopRow; i < nEnd; i++)
        {
            extent.h = nHeight;
            extent.y = y;
            if (nRemainder != 0)
            {
                extent.h = nHeight + 1;
                nRemainder--;
                y++;
            }
            m_aControls.m_pArray[i]->SetExtent(extent);
            y += nHeight + m_nPadding;
        }
    }

    // Rows below the window go back to the plain pitch and the plain height.
    extent.h = m_nRowHeight;
    for (int i = m_nVisibleRows + m_nTopRow; i < nCount; i++)
    {
        extent.y = y;
        m_aControls.m_pArray[i]->SetExtent(extent);
        y += nRowPitch;
    }
}

void CSWGuiListBox::OrganizeUnequal()
{
    int nCount = m_aControls.m_count;
    CExoString sUnused;

    if (bLayoutDirty)
    {
        // The remeasure skips the last row, so the one-row case needs its own arm.
        if (nCount == 1)
        {
            m_nRowHeight = m_aControls.m_pArray[0]->GetIdealHeight();
        }
        else
        {
            m_nRowHeight = 0;
            for (int i = 0; i < nCount - 1; i++)
            {
                int nIdeal = m_aControls.m_pArray[i]->GetIdealHeight();
                if (nIdeal > m_nRowHeight)
                    m_nRowHeight = nIdeal;
            }
        }

        m_nTopRow = 0;
        m_nBottomRow = -1;
        bAnchorTop = 1;
        if (m_nRowHeight == 0)
            return;
        bLayoutDirty = 0;
    }

    // Re-anchor on the selection when it has fallen outside the window: above it pins the top,
    // below it pins the bottom.
    if (bScrollToSelection && m_nSelectedIndex != -1)
    {
        if (m_nTopRow != -1 && m_nSelectedIndex < m_nTopRow)
        {
            m_nTopRow = m_nSelectedIndex;
            m_nBottomRow = -1;
            bAnchorTop = 1;
        }
        else if (m_nBottomRow != -1 && m_nSelectedIndex > m_nBottomRow)
        {
            m_nTopRow = -1;
            m_nBottomRow = m_nSelectedIndex;
            bAnchorTop = 0;
        }
    }

    CSWGuiExtent extent;
    extent.x = m_nPadding;
    extent.w = m_innerExtent.w - 2 * m_nPadding;

    int nAnchor;
    int y;
    if (bAnchorTop)
    {
        nAnchor = m_nTopRow;
        y = m_nPadding;
    }
    else
    {
        nAnchor = m_nBottomRow;
        y = m_innerExtent.h - m_aControls.m_pArray[nAnchor]->m_extent.h - m_nPadding;
    }

    extent.h = m_aControls.m_pArray[nAnchor]->m_extent.h;
    extent.y = y;
    m_aControls.m_pArray[nAnchor]->SetExtent(extent);

    int nAbove = nAnchor;
    while (--nAbove >= 0)
    {
        extent.h = m_aControls.m_pArray[nAbove]->m_extent.h;
        y = y - extent.h - m_nPadding;
        extent.y = y;
        m_aControls.m_pArray[nAbove]->SetExtent(extent);
    }

    // Downwards from the anchor, re-reading the anchor's extent after its own SetExtent ran.
    if (nAnchor + 1 < nCount)
    {
        CSWGuiControl* pAnchor = m_aControls.m_pArray[nAnchor];
        y = pAnchor->m_extent.h + pAnchor->m_extent.y + m_nPadding;
        for (int i = nAnchor + 1; i < nCount; i++)
        {
            int nHeight = m_aControls.m_pArray[i]->m_extent.h;
            int nPadding = m_nPadding;
            extent.h = nHeight;
            extent.y = y;
            m_aControls.m_pArray[i]->SetExtent(extent);
            y += nHeight + nPadding;
        }
    }

    // Now that everything is placed, find the other end of the window by scanning away from the
    // anchor for the first row that no longer fits.
    if (bAnchorTop)
    {
        int i;
        for (i = m_nTopRow + 1; i < nCount; i++)
        {
            CSWGuiControl* pControl = m_aControls.m_pArray[i];
            if (pControl->m_extent.y + pControl->m_extent.h > m_innerExtent.h)
                break;
        }
        // The loop-exhausted edge stores nCount - 1 directly instead of i - 1.
        if (i >= nCount)
        {
            m_nBottomRow = static_cast<short>(nCount - 1);
        }
        else
        {
            m_nBottomRow = static_cast<short>(i - 1);
            if (m_nBottomRow == -1)
                m_nBottomRow = static_cast<short>(nCount - 1);
        }
    }
    else
    {
        int i;
        for (i = m_nBottomRow; i > 0; i--)
        {
            if (m_aControls.m_pArray[i - 1]->m_extent.y < 0)
                break;
        }
        if (i > 0)
            m_nTopRow = static_cast<short>(i);
        if (m_nTopRow == -1)
            m_nTopRow = 0;
        bAnchorTop = 1;
    }

    m_nVisibleRows = m_nBottomRow - m_nTopRow + 1;

    int nHidden = nCount - m_nVisibleRows;
    m_scrollBar.SetMaxValue(nHidden > 0 ? nHidden + 1 : 1, bNoSelection);
    m_scrollBar.SetVisibleValue(m_nBottomRow - m_nTopRow + 1);
    m_scrollBar.SetCurValue(m_nTopRow);
}

CSWGuiEditbox::CSWGuiEditbox()
{
    m_tail.bFocusOnHover = 1;
}


// The mac/ios soft keyboard, declared here as in CSWGuiSaveLoad.cpp.
void ASLPlat_ShowVirtualKeyboard(const char* sText, unsigned int nMaxLength);
bool ASLPlat_IsVirtualKeyboardVisible();  // read by Draw

// The IosIsIphone shim and the port's keyboard-inset tunable, as declared in CSWGuiControl.cpp.
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
extern __attribute__((visibility("hidden"))) int g_nKeyboardTop;

// libSDL2 entry points used by the soft-keyboard teardown in HandleFocusChange.

#if !KOTOR1
CSWGuiEditbox::~CSWGuiEditbox()
{
}
#endif

int CSWGuiEditbox::GetIsSelectable()
{
    return m_tail.bEnabled;
}

void CSWGuiEditbox::SetEnabled(int flag)
{
    m_tail.bEnabled = flag;
    if (flag)
        m_editText.m_params.RestoreDefaultColor();
    else
        m_editText.m_params.SetColor(CSWGuiManager::DISABLED_TEXT);
}

void CSWGuiEditbox::HandleLMouseUp()
{
    g_pGuiMan->ReleaseMouse(this);
    HandleLoseMouseFocus();
}

void CSWGuiEditbox::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line, so the copies after it re-read the member
    CSWGuiObject::SetExtent(extent);
    m_border.m_extent = m_extent;
#else
    m_extent = extent;
    m_border.m_extent = extent;
#endif

    CSWGuiExtent innerExtent = m_border.GetInnerExtent();
    m_editText.SetExtent(innerExtent);
}

void CSWGuiEditbox::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& textParams,
                               const CSWGuiBorderParams& borderParams)
{
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
    m_border.Initialize(m_extent, borderParams);
    CSWGuiExtent inner = m_border.GetInnerExtent();
    m_editText.Initialize(inner, textParams, 1.0f);
}

void CSWGuiEditbox::HandleLMouseDown()
{
    if (!m_tail.bEnabled)
        return;

    HandleInputEvent(SWGUIEVENT_FOCUS, 1);
    HandleFocusChange(1);
    ASLPlat_ShowVirtualKeyboard(m_editText.m_sText.CStr(), m_editText.m_nMaxLength);
    g_pGuiMan->CaptureMouse(this, 1);
}

void CSWGuiEditbox::HandleFocusChange(int bGaining)
{
    if (bGaining)
    {
        g_pExoInput->KeyboardModeOn();
        g_pGuiMan->m_pFocusedEditbox = this;
        m_editText.SetCaretVisible(1);

        if (!m_pContainer)
            return;
        CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
        if (!pPanel)
            return;
        if (!m_tail.bActive)
            g_pGuiMan->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        pPanel->SetActiveControl(this, 0);
    }
    else
    {
        g_pExoInput->KeyboardModeOff();
        g_pGuiMan->m_pFocusedEditbox = 0;
        m_editText.SetCaretVisible(0);
#if !KOTOR1  // K1 leaves SDL text input alone
        if (SDL_IsTextInputActive())
            SDL_StopTextInput();
#endif
    }
}

void CSWGuiEditbox::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    m_editText.Load(pGFF, resStruct, "TEXT");
    m_border.Load(pGFF, resStruct, "BORDER");
    CSWGuiControl::Load(pGFF, resStruct);
}

void CSWGuiEditbox::SetFocus()
{
    HandleFocusChange(1);
}

void CSWGuiEditbox::ReSetFont()
{
    m_editText.m_params.ReSetFont();
}

void CSWGuiEditbox::HandleKeyPress(int nKey)
{
    switch (nKey) {
    case EDITKEY_BACKSPACE:
        m_editText.RemoveLastChar();
        break;
    case EDITKEY_LINEFEED:
    case EDITKEY_RETURN:
        if (m_pContainer != 0) {
            CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
            if (pPanel != 0)
                pPanel->HandleInputEvent(INPUTEVENT_GUI_A, 1);
        }
        break;
    case EDITKEY_ESCAPE:
        if (m_pContainer != 0) {
            CSWGuiPanel* pPanel = m_pContainer->AsSWGuiPanel();
            if (pPanel != 0)
                pPanel->HandleInputEvent(INPUTEVENT_GUI_B, 1);
        }
        break;
    case EDITKEY_LEFT:
        HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, 1);
        break;
    case EDITKEY_UP:
        HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        break;
    case EDITKEY_RIGHT:
        HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, 1);
        break;
    case EDITKEY_DOWN:
        HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        break;
    default:
        if (nKey == EDITKEY_DELETE) {
            m_editText.RemoveLastChar();
        } else if (nKey >= EDITKEY_FIRST_PRINTABLE) {
            switch (nKey) {
            case '/':
            case '\\':
            case '_':
                break;
            default: {
                static char sChar[2];
                sChar[0] = static_cast<char>(nKey);
                sChar[1] = 0;
                m_editText.AddNewChar(sChar);
                break;
            }
            }
        }
        break;
    }
}

void CSWGuiEditbox::Draw(float fAlpha)
{
    int nScreenWidth;
    int nScreenHeight;
    CSWGuiExtent extent = m_border.m_extent;
    CResRef savedFillImage;

    int nInset = 0;
    if (ASLPlat_IsVirtualKeyboardVisible())
    {
        if (IosIsIphone(nScreenWidth, nScreenHeight) && nScreenWidth <= 960)
        {
            nInset = extent.y + extent.h - g_nKeyboardTop;
            if (nInset < 0)
            {
                nInset = 0;
            }
            else
            {
                extent = m_border.m_extent;
                extent.y -= nInset;
                m_border.m_extent = extent;

                extent = m_editText.m_extent;
                extent.y -= nInset;
                m_editText.SetExtent(extent);

                savedFillImage = m_border.m_params.m_fillImage;
                m_border.m_params.SetFillImage(CResRef("ios_edit_box"), 0);
            }
        }
    }

    m_border.Draw(fAlpha);
    m_editText.Draw(fAlpha);

    if (nInset)
    {
        m_border.m_params.SetFillImage(savedFillImage, 0);

        extent = m_border.m_extent;
        extent.y += nInset;
        m_border.m_extent = extent;

        extent = m_editText.m_extent;
        extent.y += nInset;
        m_editText.SetExtent(extent);
    }
}


void CSWGui3DSceneView::SetExtent(const CSWGuiExtent& extent) {
    m_scene.m_extent = extent;
#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
}

void CSWGui3DSceneView::Draw(float f)
{
    m_scene.Draw(f);
}

