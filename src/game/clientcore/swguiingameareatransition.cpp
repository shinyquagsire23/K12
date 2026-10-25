#include "k2stdint.h"
#include "swguiingameareatransition.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "swparty.h"
#include "nwctrigger.h"
#include "nwcdoor.h"
#include "port/k1gui.h"
#if KOTOR1
#include "aurgui.h"
#endif

float GetResolutionTextScalar();
#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exostringinline.h"

#if KOTOR1  // K1: the "areatransition" layout, scaled by the panel itself
CSWGuiInGameAreaTransition::CSWGuiInGameAreaTransition(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_position = Vector(0.0f, 0.0f, 0.0f);

    StartLoadFromLayout(CResRef("areatransition"), 1, 1, 1);
    InitControl(m_lblTextBG, CExoString("LBL_TEXTBG"), 1, 1, 0, 0, 0);
    InitControl(m_lblIcon, CExoString("LBL_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_lblDescription, CExoString("LBL_DESCRIPTION"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();
#ifndef K2_UI_TOUCH
    // pc / xbox: the banner is centred across the screen (the layout's x is a 4:3 screen's centre)
    {
        CSWGuiExtent extent = m_extent;
        extent.x = (pGuiManager->m_nScreenWidth - extent.w) / 2;
        SetExtent(extent);
    }
#endif

    m_objId = OBJECT_INVALID;
    m_nBannerY = m_extent.y;
}
#else
CSWGuiInGameAreaTransition::CSWGuiInGameAreaTransition(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_position = Vector(0.0f, 0.0f, 0.0f);
    m_vTextBGColor = Vector(0.0f, 0.0f, 0.0f);
    m_vIconColor = Vector(0.0f, 0.0f, 0.0f);
    m_vDescColor = Vector(0.0f, 0.0f, 0.0f);

    StartLoadFromLayout(CResRef("areatrans_p"), 1, 1, 1);
    InitControl(m_lblTextBG, CExoString("LBL_TEXTBG"), 1, 1, 0, 0, 0);
    InitControl(m_lblIcon, CExoString("LBL_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_lblDescription, CExoString("LBL_DESCRIPTION"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // Remembers the layout's own colours so Draw can put them back when the party is healthy: the first
    // two are border colours (a CSWGuiColor, hence the cast), the third is LBL_DESCRIPTION's text colour,
    // already a Vector.
    m_vTextBGColor = *reinterpret_cast<Vector*>(&m_lblTextBG.m_border.m_params.m_color);
    m_vIconColor = *reinterpret_cast<Vector*>(&m_lblIcon.m_border.m_params.m_color);
    m_vDescColor = m_lblDescription.m_text.m_params.m_color;

    m_objId = OBJECT_INVALID;

    float fScale = GetResolutionTextScalar();
    CSWGuiExtent extent = m_extent;

    // w and h are assigned as an adjacent pair on purpose.
    extent.w = static_cast<int>(fScale * 400.0f);
    extent.h = static_cast<int>(fScale * 82.0f);
    int nHalfWidth = extent.w / 2;
    extent.x =
        static_cast<int>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth) / 2 - nHalfWidth;
    SetExtent(extent);

    extent.w = static_cast<int>(fScale * 50.0f);
    extent.h = static_cast<int>(fScale * 50.0f);
    extent.x = nHalfWidth - extent.w / 2;
    extent.y = static_cast<int>(fScale * 27.0f);
    m_lblIcon.SetExtent(extent);

    extent.w = static_cast<int>(fScale * 348.0f);
    extent.h = static_cast<int>(fScale * 20.0f);
    extent.x = nHalfWidth - extent.w / 2;
    extent.y = static_cast<int>(fScale * 6.0f);
    m_lblDescription.SetExtent(extent);

    extent.x = 0;
    extent.y = 0;
    extent.w = static_cast<int>(fScale * 400.0f);
    extent.h = static_cast<int>(fScale * 32.0f);
    m_lblTextBG.SetExtent(extent);

    // The banner's resting y, taken off the layout's own extent.
    m_nBannerY = m_extent.y;
}
#endif

void CSWGuiInGameAreaTransition::SetTransitionObject(unsigned long long objId, const Vector& vPos)
{
    if (m_objId != objId)
    {
        // Fetched once, then narrowed through the vtable's own AsSWCObject slot rather than a cast.
        CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pObject != 0)
        {
            CSWCObject* pSWCObject = pObject->AsSWCObject();
            if (pSWCObject != 0)
            {
                // Only trigger and door objects are handled here. The door arm is written first
                // on purpose.
                CExoString sName;
                if (pSWCObject->m_objectType == GAME_OBJECT_TYPE_DOOR)
                {
                    // The door arm copies through a temporary; the trigger arm assigns the member
                    // directly.
                    sName = CExoString(pSWCObject->AsSWCDoor()->m_transitionDestination);
                }
                else if (pSWCObject->m_objectType == GAME_OBJECT_TYPE_TRIGGER)
                {
                    sName = pSWCObject->AsSWCTrigger()->m_transitionDestination;
                }

                // A transition destination is "<tag>-<something>"; the banner shows the tail.
                int nDash = sName.Find(CExoString("-"), 0);
                if (nDash != -1)
                {
                    int nLength = sName.GetLength();
                    sName = sName.Right(nLength - nDash - 2);
                }
                m_lblDescription.m_text.m_params.SetText(sName);
#ifdef K2_QOL_IMPROVEMENTS
                K2QolLayoutAreaTransition(*this);
#endif
            }
        }
        m_objId = objId;
    }
    m_position = vPos;
}

#if KOTOR1  // K1: party-together test, fixed palette, resolution text scale
void CSWGuiInGameAreaTransition::Draw(float f)
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
    int nBarkY;
    CSWGuiExtent extent = m_extent;
    extent.y = (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetBarkBubbleVisible(nBarkY) == 0)
                   ? m_nBannerY
                   : nBarkY;
    SetExtent(extent);

    if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyTogether() == 0)
    {
        *reinterpret_cast<Vector*>(&m_lblTextBG.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::DISABLED_ICON_COLOR);
        *reinterpret_cast<Vector*>(&m_lblIcon.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::DISABLED_ICON_COLOR);
        m_lblDescription.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
    }
    else
    {
        *reinterpret_cast<Vector*>(&m_lblTextBG.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::ENABLED_ICON_COLOR);
        *reinterpret_cast<Vector*>(&m_lblIcon.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::ENABLED_ICON_COLOR);
        m_lblDescription.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
    }
    if (m_lblDescription.m_text.m_pString)
        m_lblDescription.m_text.m_pString->SetFontScale(s_fScale);

    CSWGuiPanel::Draw(f);
}
#else
void CSWGuiInGameAreaTransition::Draw(float f)
{
    int nBarkY;   // declared before extent: the slot order matters
    CSWGuiExtent extent = m_extent;
    extent.y = (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetBarkBubbleVisible(nBarkY) == 0)
                   ? m_nBannerY
                   : nBarkY;
    SetExtent(extent);

    if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyHealthy() == 0)
    {
        // The party is in trouble, so paint both labels with the disabled icon tint and grey the
        // caption out.
        *reinterpret_cast<Vector*>(&m_lblTextBG.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::DISABLED_ICON_COLOR);
        *reinterpret_cast<Vector*>(&m_lblIcon.m_border.m_params.m_color) =
            *static_cast<const Vector*>(&CGuiInGame::DISABLED_ICON_COLOR);
        m_lblDescription.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
    }
    else
    {
        *reinterpret_cast<Vector*>(&m_lblTextBG.m_border.m_params.m_color) = m_vTextBGColor;
        *reinterpret_cast<Vector*>(&m_lblIcon.m_border.m_params.m_color) = m_vIconColor;
        m_lblDescription.m_text.m_params.SetColor(m_vDescColor);
    }

    CSWGuiPanel::Draw(f);
}
#endif
