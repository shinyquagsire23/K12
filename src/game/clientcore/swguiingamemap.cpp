
#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguiingamemap.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "swgui.h"
#include "swguiingame.h"
#include "swguidrawable.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "guiingame.h"
#include "nwscript.h"
#include "exolocstring.h"
#include "exostring.h"
#include "Vector.h"
#include "Quaternion.h"
#include "exoresman.h"
#include "CExoLinkedList.h"
#include "partytables.h"
#include "swparty.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwsmodule.h"
#include "swsareamap.h"
#include "nwscreature.h"
#include "CSWVisibilityNode.h"
#include "nwsobject.h"
#include "nwswaypoint.h"
#include "nwsarea.h"
#include "aurgui.h"
#include "CExoArrayListImpl.h"
#include "gameobjectarray.h"
#include "swguicontrol.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "aurscene.h"

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "factionmanager.h"
#include "tutorial.h"
#include "strref.h"
#include "exostringinline.h"

// The galaxy map's planet slots (CSWPartyTable's planet flags).
enum {
    GALAXY_MAP_PLANET_COUNT = 16,
};


extern int g_nScreenWidth;
#if !KOTOR1 && !defined(K2_UI_TOUCH)
// 708x354 is the port's map pixel space. The map and its markers are drawn at LBL_Map's own
// layout size over it, so they fill the label whichever data supplies map_p: gui.bif's is 588x294
// of the 800x600 GUI, the Android override's 705x352.
static void K2MapLabelMultipliers(const CSWGuiLabel& lblMap, float& fXMult, float& fYMult)
{
    if (lblMap.m_originalExtent.w > 0 && lblMap.m_originalExtent.h > 0)
    {
        fXMult *= static_cast<float>(lblMap.m_originalExtent.w) / 708.0f;
        fYMult *= static_cast<float>(lblMap.m_originalExtent.h) / 354.0f;
    }
}
#endif
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

#if KOTOR1
#ifdef K2_FIX_ENGINE_BUGS
// The map is laid out on 440x256 pixels, while LBL_Map is 704x409, so the map, its fog and its
// markers are drawn scaled up to fill the label.
static float s_fK1MapFit = 1.0f;
#endif

CSWGuiInGameMap::CSWGuiInGameMap(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("map"));
    InitControl(m_lblMap, CExoString("LBL_Map"), 0);

#ifdef K2_FIX_ENGINE_BUGS
    const CSWGuiExtent extentLayout = m_lblMap.m_extent;
    if (extentLayout.w >= 440 && extentLayout.h >= 256)
        s_fK1MapFit = __builtin_fminf(extentLayout.w / 440.0f, extentLayout.h / 256.0f);
    const float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f * s_fK1MapFit;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiExtent extentMap;
    extentMap.w = static_cast<int>(s_fScale * 440.0f);
    extentMap.h = static_cast<int>(s_fScale * 256.0f);
    extentMap.x = (g_SDL_mode.w - extentMap.w) / 2;
    extentMap.y = static_cast<int>(s_fScale * 14.0f + static_cast<float>((g_SDL_mode.h - extentMap.h) / 2));
#ifdef K2_FIX_ENGINE_BUGS
    if (s_fK1MapFit > 1.0f) {
        // centred on the label's own box: the 1024x768 layout is centred on the screen
        const float fLayout = static_cast<float>(g_SDL_mode.h) / 768.0f;
        extentMap.x = static_cast<int>((static_cast<float>(g_SDL_mode.w) - fLayout * 1024.0f) * 0.5f
                            + fLayout * (extentLayout.x + extentLayout.w * 0.5f)) - extentMap.w / 2;
        extentMap.y = static_cast<int>(fLayout * (extentLayout.y + extentLayout.h * 0.5f)) - extentMap.h / 2;
    }
#endif
    m_lblMap.SetExtent(extentMap);

    InitControl(m_lblMapNote,     CExoString("LBL_MapNote"),  1);
    InitControl(m_lblArea,        CExoString("LBL_Area"),     1);
    InitControl(m_lblCompass,     CExoString("LBL_COMPASS"),  1);
    InitControl(m_btnReturn,      CExoString("BTN_RETURN"),   1);
    InitControl(m_btnPartySelect, CExoString("BTN_PRTYSLCT"), 1);
    InitControl(m_btnExit,        CExoString("BTN_EXIT"),     1);
    InitControl(m_btnUp,          CExoString("BTN_UP"),       1);
    InitControl(m_btnDown,        CExoString("BTN_DOWN"),     1);
    StopLoadFromLayout();

    m_image.SetExtent(CSWGuiExtent(0, 0, 512, 256));
#ifdef K2_FIX_ENGINE_BUGS
    m_mapHider.CSWGuiObject::SetExtent(
        CSWGuiExtent(0, 0, static_cast<int>(440.0f * s_fK1MapFit), static_cast<int>(256.0f * s_fK1MapFit)));
#else
    m_mapHider.CSWGuiObject::SetExtent(CSWGuiExtent(0, 0, 440, 256));
#endif
    CSWGuiExtent extentImage = m_image.m_extent;
    extentImage.w = static_cast<int>(extentImage.w * s_fScale);
    extentImage.h = static_cast<int>(extentImage.h * s_fScale);
    m_image.SetExtent(extentImage);
    AddControl(&m_mapHider);

    m_mapHider.m_tail.bVisible = 0;
    ScaleExtentForResolution();
    m_nTransitBlockedStrRef = 0;
    m_nTransitFlags |= TRANSIT_ALLOWED | TRANSIT_RETURN_ENABLED;
    m_btnReturn.m_tail.bFocusOnHover = 0;
    m_btnReturn.AddEvent(INPUTEVENT_GUI_A, this,
                         static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_btnPartySelect.m_tail.bFocusOnHover = 0;
    m_btnPartySelect.AddEvent(INPUTEVENT_GUI_A, this,
                              static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnUp.m_tail.bFocusOnHover = 0;
    m_btnUp.AddEvent(INPUTEVENT_GUI_A, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameMap::OnUpArrowPressed));
    m_btnDown.m_tail.bFocusOnHover = 0;
    m_btnDown.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameMap::OnDownArrowPressed));
    IosAddButtonIcon(&m_btnReturn, 'x');
    IosAddButtonIcon(&m_btnPartySelect, 'a');
    IosAddButtonIcon(&m_btnExit, 'b');
}
#else
CSWGuiInGameMap::CSWGuiInGameMap(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("map_p"), 1, 1, 1);

    // Only LBL_Map passes 0 for its second argument.
    InitControl(m_lblMap,     CExoString("LBL_Map"),     0, 1, 0, 0, 0);
    InitControl(m_lblMapNote, CExoString("LBL_MapNote"), 1, 1, 0, 0, 0);
    InitControl(m_lblArea,    CExoString("LBL_Area"),    1, 1, 0, 0, 0);
    InitControl(m_btnExit,    CExoString("BTN_EXIT"),    1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnExit, 'b');
    InitControl(m_btnUp,      CExoString("BTN_UP"),      1, 1, 0, 0, 0);
    InitControl(m_btnDown,    CExoString("BTN_DOWN"),    1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle,   CExoString("LBL_TITLE"),   1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // 708x354 is the map texture's base aspect; the aspect multipliers scale it and the two
    // offsets centre it inside the letterboxed native rect.
    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
#ifndef K2_UI_TOUCH
        K2MapLabelMultipliers(m_lblMap, fXMult, fYMult);
#endif
    }

    m_image.SetExtent(
        CSWGuiExtent(nXOffset, nYOffset, static_cast<int>(fXMult * 708.0f), static_cast<int>(fYMult * 354.0f)));
    m_mapHider.m_extent =
        CSWGuiExtent(nXOffset, nYOffset, static_cast<int>(fXMult * 708.0f), static_cast<int>(fYMult * 354.0f));
    AddControl(&m_mapHider);

    m_mapHider.m_tail.bVisible = 0;
    m_nTransitBlockedStrRef = 0;
    m_nTransitFlags |= TRANSIT_ALLOWED | TRANSIT_RETURN_ENABLED;
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnUp.m_tail.bFocusOnHover = 0;
    m_btnUp.AddEvent(INPUTEVENT_GUI_A, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameMap::OnUpArrowPressed));
    m_btnUp.m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
    m_btnDown.m_tail.bFocusOnHover = 0;
    m_btnDown.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameMap::OnDownArrowPressed));
    m_btnDown.m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
}
#endif


void CSWGuiInGameMap::OnPanelAdded()
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    float fReach;
    CSWVisibilityNode* pVisible;
    float fNearest;
    CSWSCreature* pMember;
    CSWSArea* pLeaderArea;
    CSWSCreature* pTarget;
    CSWSObject* pJMember;
    unsigned long long nId;

#if !KOTOR1
    // Re-derive the map's screen rectangle: the panel can be re-added after a resolution change.
    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
#ifndef K2_UI_TOUCH
        K2MapLabelMultipliers(m_lblMap, fXMult, fYMult);
#endif
    }

    m_image.SetExtent(
        CSWGuiExtent(nXOffset, nYOffset, static_cast<int>(fXMult * 708.0f), static_cast<int>(fYMult * 354.0f)));
    m_mapHider.m_extent =
        CSWGuiExtent(nXOffset, nYOffset, static_cast<int>(fXMult * 708.0f), static_cast<int>(fYMult * 354.0f));
#endif

    CExoString sAreaName(g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_areaName);
    m_lblArea.m_text.m_params.SetText(sAreaName);

    // The map's texture is the module's own resref, "lbl_map<modname>", or the fixed placeholder
    // when the party table says the map is disabled or the module has no area map at all.
    CResRef resModId = pServer->GetModule()->m_resRef;
    CExoString sModId;
    CExoString sLayoutName;
    resModId.CopyToString(sModId);
    sLayoutName.Format(const_cast<char*>("lbl_map%s"), sModId.CStr());

    int nNoteCount;
#if KOTOR1
    if (pServer->GetModule()->m_pAreaMap->m_nSize != 0)
#else
    if (pServer->GetPartyTable()->m_disableMap != 0)
    {
        m_image.m_params.SetImage(CResRef("LBL_STATIC"), 0);
        nNoteCount = 0;
    }
    else if (pServer->GetModule()->m_pAreaMap->m_nSize != 0)
#endif
    {
        m_image.m_params.SetImage(CResRef(sLayoutName), 0);
        nNoteCount = 1;
    }
    else
    {
        m_image.m_params.SetImage(CResRef("lbl_mapm28aa"), 0);
        nNoteCount = 0;
    }
    m_image.m_params.SetDrawStyle(0);

    SetMapNote(m_mapHider.InitializeMapNotes(nNoteCount));

    // The X prompt has two states. One flag says the panel still wants to offer the "return to the
    // Ebon Hawk" text, and m_nTransitBlockedStrRef holds the StrRef it quotes.
#if KOTOR1  // K1 suppresses the prompt through the area's Unescapable flag; there is no DisableTransit
    if (static_cast<CSWSArea*>(pServer->GetPlayerCreature()->GetArea())
            ->m_nUnescapable != 0)
#else
    if (static_cast<CSWSArea*>(pServer->GetPlayerCreature()->GetArea())
            ->m_disableTransit != 0)
#endif
    {
        m_nTransitBlockedStrRef = STRREF_FUNCTION_UNAVAILABLE;
        m_nTransitFlags &= ~TRANSIT_ALLOWED;
    }
    else
    {
        m_nTransitBlockedStrRef = 0;
        m_nTransitFlags |= TRANSIT_ALLOWED;
    }

    if (m_nTransitFlags & TRANSIT_ALLOWED)
    {
#if KOTOR1
        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyTogether() == 0)
#else
        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyHealthy() == 0)
#endif
        {
            m_nTransitBlockedStrRef = STRREF_REJOIN_PARTY_FIRST;
            m_nTransitFlags &= ~TRANSIT_ALLOWED;
        }
    }

    if (m_nTransitFlags & TRANSIT_ALLOWED)
    {
        // Any near party member whose spot-check reach covers the party leader is "in sight", so the
        // walk starts from the closest member and scans the area's X-sorted object index outwards.
        CSWSObject* pLeader;
        nId = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
        nId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nId);
        pLeader = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nId);
        fNearest = 3.402823466e+38f;
        for (int i = 0; i < g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount; i++)
        {
            nId = g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[i].id;
            nId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nId);
            pMember = static_cast<CSWSCreature*>(g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nId));
            if (pMember != 0)
            {
                fReach = pMember->m_position.x - static_cast<float>(pMember->GetSpotCheckDistance());
                if (fReach < fNearest)
                    fNearest = pMember->m_position.x - static_cast<float>(pMember->GetSpotCheckDistance());
            }
        }

        int nIndex;
        pLeaderArea = static_cast<CSWSArea*>(pLeader->GetArea());
        pLeaderArea->GetFirstObjectIndiceByX(&nIndex, fNearest);
        while ((m_nTransitFlags & TRANSIT_ALLOWED) && nIndex < pLeaderArea->m_aObjectsByX.m_count)
        {
            pTarget = static_cast<CSWSCreature*>(g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pLeaderArea->m_aObjectsByX.m_pArray[nIndex]));
            if (pTarget != 0)
            {
                for (int j = 0; j < g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
                     j++)
                {
                    nId = g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[j].id;
                    nId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nId);
                    pJMember = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nId);
                    if (pJMember != 0 && pJMember->GetReputation(pTarget->m_objectId, 0) < CFactionManager::REPUTATION_HOSTILE_BELOW)
                    {
                        pVisible =
                            pTarget->GetVisibleListElement(pJMember->m_objectId);
                        if (pVisible != 0 && (pVisible->m_flags & 1) != 0)
                        {
                            m_nTransitFlags &= ~TRANSIT_ALLOWED;
                            m_nTransitBlockedStrRef = STRREF_ENEMIES_NEARBY;
                        }
                    }
                }
            }
            nIndex++;
        }
    }

    m_nTransitFlags = ((m_nTransitFlags & TRANSIT_ALLOWED)
                      ? ((pServer->GetPartyTable()->m_bShowReturn != 0) ? TRANSIT_RETURN_ENABLED : 0)
                      : 0) |
                 (m_nTransitFlags & ~TRANSIT_RETURN_ENABLED);

#if KOTOR1
    // K1: the return and party-select buttons take the party table's caption and are enabled or
    // greyed from two party-table flags.
    m_btnReturn.m_text.m_params.SetStrRef(static_cast<uint64_t>(pServer->GetPartyTable()->m_nReturnStrref));
    m_btnReturn.m_text.m_params.SetColor((m_nTransitFlags & TRANSIT_RETURN_ENABLED) ? CGuiInGame::COLOR_BLUE
                                                          : CGuiInGame::DISABLED_TEXT);
    m_btnReturn.m_border.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>((m_nTransitFlags & TRANSIT_RETURN_ENABLED) ? &CGuiInGame::ENABLED_ICON_COLOR
                                                : &CGuiInGame::DISABLED_ICON_COLOR);
    m_btnPartySelect.m_text.m_params.SetColor((m_nTransitFlags & TRANSIT_ALLOWED) ? CGuiInGame::COLOR_BLUE
                                                               : CGuiInGame::DISABLED_TEXT);
    m_btnPartySelect.m_border.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>((m_nTransitFlags & TRANSIT_ALLOWED) ? &CGuiInGame::ENABLED_ICON_COLOR
                                                : &CGuiInGame::DISABLED_ICON_COLOR);
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bUseSmallFonts)
    {
        m_btnPartySelect.m_text.m_params.SetAlignment(SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE);
        m_btnReturn.m_text.m_params.SetAlignment(SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE);
    }
    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_MAP_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    IosAddButtonIcon(&m_btnReturn, 'x');
#else
    g_pAppManager->m_pClientExoApp->GetClientOptions();
    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_MAP_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
#endif
}

void CSWGuiInGameMap::OnUpArrowPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
}

void CSWGuiInGameMap::OnDownArrowPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
}

void CSWGuiInGameMap::SetMapNote(const CExoString& sMapNote)
{
    CExoString sText;
    if (sMapNote.GetLength() != 0)
    {
        // The "Map Note" caption; the shipped text reads "Map Note: <name>".
        sText = g_pTlkTable->GetSimpleString(STRREF_MAP_NOTE) + CExoString(": ") + sMapNote;
        m_lblMapNote.m_text.m_params.SetText(sText);
    }
    else
    {
        m_lblMapNote.m_text.m_params.SetText(CExoString(""));
    }
}

void CSWGuiInGameMap::OnPanelRemoved()
{
    // CSWGuiMapHider::ClearMapNotes, written out here: the loop body has to stay visible.
    while (m_mapHider.m_mapNotes.m_pInternal->m_pHead != 0)
    {
        unsigned long long* pNoteId =
            static_cast<unsigned long long*>(m_mapHider.m_mapNotes.m_pInternal->m_pHead->m_pData);
        if (pNoteId)
            operator delete(pNoteId);
        m_mapHider.m_mapNotes.m_pInternal->RemoveHead();
    }
    m_mapHider.m_pCurrentMapNote = 0;

    CSWGuiPanel::OnPanelRemoved();
}

#if KOTOR1
void CSWGuiInGameMap::Draw(float f)
{
    if ((m_nTransitFlags & TRANSIT_RETURN_ENABLED) == 0)
    {
        m_btnReturn.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnReturn.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
    if ((m_nTransitFlags & TRANSIT_ALLOWED) == 0)
    {
        m_btnPartySelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnPartySelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }

    int x = m_lblMap.m_extent.x;
    int y = m_lblMap.m_extent.y;
    int w = m_lblMap.m_extent.w;
    int h = m_lblMap.m_extent.h;

    CSWGuiPanel::Draw(f);

    AurGUIStartLayer();
    if (AurGUISetupViewport(x, y, w, h, Vector(-1.0f, -1.0f, -1.0f), 0, 1.0f))
    {
        m_image.Draw(f);
        m_mapHider.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
}
#else
void CSWGuiInGameMap::Draw(float f)
{
    // LBL_Map's own rect is the map's screen rectangle.
    int x = m_lblMap.m_extent.x;
    int y = m_lblMap.m_extent.y;
    int h = m_lblMap.m_extent.h;

    CSWGuiPanel::Draw(f);

    if ((m_stateFlags & 1) == 0)
    {
        x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
        y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }

    int w = static_cast<int>((static_cast<float>(g_nScreenWidth) / static_cast<float>(static_cast<unsigned int>(g_nGUIWidth))) *
                  static_cast<float>(m_lblMap.m_originalExtent.w));

    AurGUIStartLayer();
    if (AurGUISetupViewport(x, y, w, h, CSWGuiDrawable::NO_COLOURING, 0, 1.0f))
    {
        m_image.Draw(f);
        m_mapHider.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
}
#endif

CSWGuiControl* CSWGuiInGameMap::HitCheckMouse(int x, int y)
{
    CSWGuiControl* pControl = CSWGuiPanel::HitCheckMouse(x, y);
    if (pControl != 0)
        return pControl;

    if ((m_stateFlags & 1) == 0)
    {
        x -= (m_pGuiManager->m_nScreenWidth - 640) / 2;
        y -= (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }

    int nLeft = m_mapHider.m_extent.x;
    int nLocalX = x - m_lblMap.m_extent.x;
    if (nLocalX >= nLeft && nLeft + m_mapHider.m_extent.w >= nLocalX)
    {
        int nTop = m_mapHider.m_extent.y;
        int nLocalY = y - m_lblMap.m_extent.y;
        if (nLocalY >= nTop && nTop + m_mapHider.m_extent.h >= nLocalY)
        {
            if (m_mapHider.m_noteButton.HitCheckMouse(nLocalX - nLeft, nLocalY - nTop) != 0)
                return &m_mapHider.m_noteButton;

            for (int i = 0; i < m_mapHider.m_aButtons.m_count; i++)
            {
                CSWGuiButton* pButton = m_mapHider.m_aButtons.m_pArray[i];
                if (pButton->HitCheckMouse(nLocalX - nLeft, nLocalY - nTop) != 0)
                    return m_mapHider.m_aButtons.m_pArray[i];
            }
            return 0;
        }
    }
    return 0;
}

void CSWGuiInGameMap::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1: A opens the party selection (or explains in a box why it cannot), X shows the map
    // tutorial or else the return-to-ship prompt; there is no change-character key.
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_A:
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            unsigned char nFlags = m_nTransitFlags;
            CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
            if (nFlags & 1)
            {
                pInGameGui->ShowPartySelection(CExoString(""), 0, -1, -1);
            }
            else
            {
                CSWGuiMessageBox* pBox = pInGameGui->m_pSoloModeQueryBox;
                pBox->SetAllowCancel(0);
                pBox->SetMessage(static_cast<uint64_t>(m_nTransitBlockedStrRef));
                pBox->SetCallback(0, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(0), 0);
                m_pGuiManager->AddPanel(pBox, 1, 1);
            }
            break;
        }
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;
        case INPUTEVENT_GUI_X:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (!g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_RETURN_TO_BASE_BUTTON, OBJECT_INVALID, OBJECT_INVALID, 0))
                OnXButton(0);
            break;
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            SetMapNote(m_mapHider.GetPrevMapNote());
            break;
        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            SetMapNote(m_mapHider.GetNextMapNote());
            break;
        default:
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
#else
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            break;
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            SetMapNote(m_mapHider.GetPrevMapNote());
            break;
        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            SetMapNote(m_mapHider.GetNextMapNote());
            break;
        default:
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
#endif
}

void CSWGuiInGameMap::OnMapNoteClicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CExoString sText;
        CExoLinkedListNode* pNode;
        unsigned long long nNoteId = pControl->m_tail.m_nUserData;

        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nNoteId);
        if (pObject != 0 && pObject->AsSWSWaypoint() != 0)
            pObject->AsSWSWaypoint()->m_mapNote.GetString(
                g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
        SetMapNote(sText);

        // CSWGuiMapHider::SetMapNote, written out here: the loop body has to stay visible.
        pNode = m_mapHider.m_mapNotes.m_pInternal->m_pHead;
        if (pNode != 0)
        {
            for (;;)
            {
                unsigned long long* pNoteId =
                    static_cast<unsigned long long*>(m_mapHider.m_mapNotes.m_pInternal->GetAtPos(pNode));
                if (*pNoteId == nNoteId)
                    break;
                m_mapHider.m_mapNotes.m_pInternal->GetNext(pNode);
                if (pNode == 0)
                    goto done;
            }
            m_mapHider.m_pCurrentMapNote = pNode;
        }
    done:
        CSWGuiInGameMap::Draw(0.001f);
        m_pGuiManager->HandleMouseMove(m_pGuiManager->m_currentMousePos.x,
                                       m_pGuiManager->m_currentMousePos.y);
    }
}

void CSWGuiInGameMap::ReturnToEbonHawk(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_pSoloModeQueryBox->m_bAccepted &&
        g_pAppManager->m_pClientExoApp->DismissInGameGUI() != 0)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 1;
        CExoString sScript("k_sup_gohawk");
        g_pVirtualMachine->RunScript(&sScript, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 0;
    }
}

void CSWGuiInGameMap::OnXButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWGuiMessageBox* pMessageBox;
    if ((m_nTransitFlags & TRANSIT_RETURN_ENABLED) == 0)
    {
        if (m_nTransitBlockedStrRef == 0)
            m_nTransitBlockedStrRef = STRREF_FUNCTION_UNAVAILABLE;
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        // The shared "are you sure?" box's text.
        pBox->SetMessage(static_cast<uint64_t>(m_nTransitBlockedStrRef));
        pBox->SetCallback(0, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(0), 0);
        pMessageBox = pBox;
    }
    else
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage(static_cast<uint64_t>(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nReturnQueryStrref));
        pBox->SetCallback(this,
                          static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameMap::ReturnToEbonHawk), 0);
        pMessageBox = pBox;
    }
    m_pGuiManager->AddPanel(pMessageBox, 1, 1);
}

// The destructor pair is implicit. The class declares none, so the vtable's ODR-use emits both
// from this translation unit.

// The in-game map's map-note widget, a list of visible map notes with a button per note.

















// Keeps the AddUnique helper out of line.
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE


int g_bEnableFogOfWar = 1;

float GetResolutionTextScalar();










int g_bShowFogOfWar = 1;



CSWGuiMapHider::CSWGuiMapHider()
{
    ClearMapNotes();
    m_bMapAvailable = 0;

    m_pBlackDot = NewCAurGUIImage(const_cast<char*>("blackdot"));

    m_pImageBar = new CSWGuiImage();
    m_pImageCircle = new CSWGuiImage();

    if (m_pImageBar)
    {
#if KOTOR1  // K1 scales by the screen height over 768
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = g_SDL_mode.h / 768.0f;
#else
        static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
        float fScalar = s_fScale;
#else
        float fScalar = GetResolutionTextScalar();
#endif
        m_pImageBar->Initialize(CSWGuiExtent(0, 0, static_cast<int>(fScalar * 40.0f), static_cast<int>(fScalar * 40.0f)),
                                CSWGuiImageParams(CResRef("mm_barrow"), 0, 0, 0.0f, SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE, 1.0f,
                                                  Vector(1.0f, 1.0f, 1.0f)));
        m_pImageBar->m_params.bHilight = 1;
    }

    if (m_pImageCircle)
    {
        CSWGuiExtent extent(0, 0, 16, 16);
        m_pImageCircle->Initialize(extent,
                                   CSWGuiImageParams(CResRef("lbl_mapcircle"), 0, 0, 0.0f, SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE,
                                                     1.0f, Vector(1.0f, 1.0f, 1.0f)));
        m_pImageCircle->m_params.m_color = CGuiInGame::COLOR_ORANGE;
    }

    m_noteButton.Initialize(CSWGuiExtent(0, 0, 30, 30), CSWGuiTextParams(),
                            CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"), 2, 0,
                                               0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0),
                            CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"), 2, 0,
                                               0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0));

#if KOTOR1
    m_noteButton.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_noteButton.m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#else
    m_noteButton.m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
    m_noteButton.m_borderHilight.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
#endif
}

void CSWGuiMapHider::ClearMapNotes()
{
    while (m_mapNotes.m_pInternal->m_pHead != 0)
    {
        unsigned long long* pNoteId =
            static_cast<unsigned long long*>(m_mapNotes.m_pInternal->m_pHead->m_pData);
        if (pNoteId)
            operator delete(pNoteId);
        m_mapNotes.m_pInternal->RemoveHead();
    }
    m_pCurrentMapNote = 0;
}

void CSWGuiMapHider::SetMapNote(unsigned long long nNoteId)
{
    CExoLinkedListNode* pNode = m_mapNotes.m_pInternal->m_pHead;
    if (pNode != 0)
    {
        for (;;)
        {
            unsigned long long* pNoteId =
                static_cast<unsigned long long*>(m_mapNotes.m_pInternal->GetAtPos(pNode));
            if (*pNoteId == nNoteId)
                break;
            m_mapNotes.m_pInternal->GetNext(pNode);
            if (pNode == 0)
                return;
        }
        m_pCurrentMapNote = pNode;
    }
}

CSWGuiControl* CSWGuiMapHider::HitCheckMouse(int x, int y)
{
    if (m_extent.x <= x && x <= m_extent.x + m_extent.w && m_extent.y <= y &&
        y <= m_extent.y + m_extent.h)
    {
        // The panel-relative coordinates are computed once and reused by both call sites.
        int nX = x - m_extent.x;
        int nY = y - m_extent.y;

        if (m_noteButton.HitCheckMouse(nX, nY) != 0)
            return &m_noteButton;
        for (int i = 0; i < m_aButtons.m_count; i++)
        {
            if (m_aButtons.m_pArray[i]->HitCheckMouse(nX, nY) != 0)
                return m_aButtons.m_pArray[i];
        }
    }
    return 0;
}

CSWGuiMapHider::~CSWGuiMapHider()
{
    ClearMapNotes();
    if (m_pBlackDot)
    {
        delete m_pBlackDot;
        m_pBlackDot = 0;
    }
    if (m_pImageBar)
    {
        delete m_pImageBar;
        m_pImageBar = 0;
    }
    if (m_pImageCircle)
    {
        delete m_pImageCircle;
        m_pImageCircle = 0;
    }
    for (int i = 0; i < m_aButtons.m_count; i++)
        delete m_aButtons.m_pArray[i];
}

// Steps the cursor to the previous visible map note and returns its localised name. The walk wraps,
// an exhausted GetPrev falling back to the list's tail, and stops on the note the cursor is already
// parked on. A note counts as visible when its waypoint's "MapNoteEnabled" flag is set and its
// world point is either explored or fog of war is off.
CExoString CSWGuiMapHider::GetPrevMapNote()
{
    CSWSWaypoint* pWaypoint;
    CGameObject* pPrev;
    CExoString sResult("");

    CExoLinkedListNode* pNote = m_pCurrentMapNote;
    unsigned long long nObjectId =
        pNote != 0 ? *static_cast<unsigned long long*>(m_mapNotes.m_pInternal->GetAtPos(pNote)) : OBJECT_INVALID;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nObjectId);
    if (pObject != 0 && pObject->AsSWSWaypoint() != 0)
        pObject->AsSWSWaypoint()->m_mapNote.GetString(
            g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, 0);

    if (pNote != 0)
        m_mapNotes.m_pInternal->GetPrev(pNote);
    if (pNote == 0)
        pNote = m_mapNotes.m_pInternal->m_pTail;
    if (pNote == 0)
        return sResult;

    while (pNote != 0 && m_pCurrentMapNote != 0 && pNote != m_pCurrentMapNote)
    {
        unsigned long long nPrevId =
            *static_cast<unsigned long long*>(m_mapNotes.m_pInternal->GetAtPos(pNote));
        pPrev = g_pAppManager->m_pServerExoApp->GetGameObject(nPrevId);
        if (pPrev != 0 && pPrev->AsSWSWaypoint() != 0)
        {
            pWaypoint = pPrev->AsSWSWaypoint();
            if (pWaypoint->m_bMapNoteEnabled != 0)
            {
#if KOTOR1  // K1: no fog-of-war debug switch
                if (g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap
                            ->IsWorldPointExplored(pWaypoint->m_position))
#else
                if (g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap
                            ->IsWorldPointExplored(pWaypoint->m_position) ||
                    g_bShowFogOfWar == 0)
#endif
                {
                    m_pCurrentMapNote = pNote;
                    pWaypoint->m_mapNote.GetString(
                        g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, 0);
                    break;
                }
            }
        }
        m_mapNotes.m_pInternal->GetPrev(pNote);
        if (pNote == 0)
            pNote = m_mapNotes.m_pInternal->m_pTail;
    }
    return sResult;
}

// The forward twin of the above, using GetNext instead of GetPrev; the wrap falls back to the
// list's head.
CExoString CSWGuiMapHider::GetNextMapNote()
{
    CSWSWaypoint* pWaypoint;
    CGameObject* pNext;
    CExoString sResult("");

    CExoLinkedListNode* pNote = m_pCurrentMapNote;
    unsigned long long nObjectId =
        pNote != 0 ? *static_cast<unsigned long long*>(m_mapNotes.m_pInternal->GetAtPos(pNote)) : OBJECT_INVALID;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nObjectId);
    if (pObject != 0 && pObject->AsSWSWaypoint() != 0)
        pObject->AsSWSWaypoint()->m_mapNote.GetString(
            g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, 0);

    if (pNote != 0)
        m_mapNotes.m_pInternal->GetNext(pNote);
    if (pNote == 0)
        pNote = m_mapNotes.m_pInternal->m_pHead;
    if (pNote == 0)
        return sResult;

    while (pNote != 0 && m_pCurrentMapNote != 0 && pNote != m_pCurrentMapNote)
    {
        unsigned long long nNextId =
            *static_cast<unsigned long long*>(m_mapNotes.m_pInternal->GetAtPos(pNote));
        pNext = g_pAppManager->m_pServerExoApp->GetGameObject(nNextId);
        if (pNext != 0 && pNext->AsSWSWaypoint() != 0)
        {
            pWaypoint = pNext->AsSWSWaypoint();
            if (pWaypoint->m_bMapNoteEnabled != 0)
            {
#if KOTOR1  // K1: no fog-of-war debug switch
                if (g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap
                            ->IsWorldPointExplored(pWaypoint->m_position))
#else
                if (g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap
                            ->IsWorldPointExplored(pWaypoint->m_position) ||
                    g_bShowFogOfWar == 0)
#endif
                {
                    m_pCurrentMapNote = pNote;
                    pWaypoint->m_mapNote.GetString(
                        g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, 0);
                    break;
                }
            }
        }
        m_mapNotes.m_pInternal->GetNext(pNote);
        if (pNote == 0)
            pNote = m_mapNotes.m_pInternal->m_pHead;
    }
    return sResult;
}

// Builds the map-note list for the area and returns the localised name of the first note that is
// both enabled and on explored ground, which is the one the cursor parks on. Every usable
// waypoint's object id is boxed in a heap unsigned long long, which ClearMapNotes frees, and
// pushed with AddHead, so the list ends up in reverse area order. The ids come from the area's
// spatial index rather than its membership list.
CExoString CSWGuiMapHider::InitializeMapNotes(int nArea)
{
    unsigned long long* pNoteId;
    m_bMapAvailable = nArea;
    CExoString sResult("");

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();

    for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++)
    {
        unsigned long long nObjectId = pArea->m_aObjectsByX.m_pArray[i];
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(nObjectId, &pObject) == CGameObjectArray::SUCCESS)
        {
            CSWSWaypoint* pWaypoint = pObject->AsSWSWaypoint();
            if (pWaypoint != 0 && pWaypoint->m_bHasMapNote != 0)
            {
                pNoteId = new unsigned long long;
                *pNoteId = nObjectId;
                m_mapNotes.m_pInternal->AddHead(pNoteId);
                if (m_pCurrentMapNote == 0 && pWaypoint->m_bMapNoteEnabled != 0)
                {
                    if (g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap
                            ->IsWorldPointExplored(pWaypoint->m_position) != 0)
                    {
                        m_pCurrentMapNote = m_mapNotes.m_pInternal->m_pHead;
                        pWaypoint->m_mapNote.GetString(
                            g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, 0);
                    }
                }
            }
        }
    }
    return sResult;
}

// The widget's painter, in five stages:
//
// 1. open the map screen's own rectangle as a viewport and pull the aspect multipliers off the GUI
//    manager
// 2. a quad per unexplored fog cell, straight out of the area map's bitfield
// 3. a button per visible map note, created lazily and reused from the previous pass, with the
//    array truncated to the number actually drawn
// 4. the companion blips (slots 1..n) as m_pImageCircle
// 5. slot 0 alone as m_pImageBar inside a second viewport, oriented by
//    GetPartyMemberMapOrientation
//
// The fog walk's cell index runs column-major over (nResX+1) x (nResY+1) cells.
#if KOTOR1
// K1: no aspect multipliers and no text scalar. Everything is laid out on a 440x256 map and scaled
// by the screen height over 768; a note is drawn only when it is enabled and its point is explored,
// whatever the fog-of-war switch says.
void CSWGuiMapHider::Draw(float f)
{
#ifdef K2_FIX_ENGINE_BUGS
    const float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f * s_fK1MapFit;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif

    if (m_bMapAvailable == 0)
        return;

    if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                            Vector(-1.0f, -1.0f, -1.0f), 0, 1.0f) == 0)
        return;

    unsigned int nMaskWords;
    int nResX;
    int nResY;

    CSWSAreaMap* pAreaMap = g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap;
    if (pAreaMap != 0 && pAreaMap->m_nSize != 0)
    {
        unsigned int* pMask = static_cast<unsigned int*>(pAreaMap->GetMapData(nMaskWords, nResX, nResY));
        if (pMask != 0 && g_bShowFogOfWar != 0)
        {
            int nCells = (nResY + 1) * (nResX + 1);
            if (nCells >= 1)
            {
                float fStepX = s_fScale * (440.0f / static_cast<float>(nResX));
                float fStepY = s_fScale * (256.0f / static_cast<float>(nResY));
                float fQuadW = fStepX * 4.0f;
                float fQuadH = fStepY * 4.0f;
                unsigned int n = 0;
                float fX = 0.0f;
                float fY = 0.0f;
                do
                {
                    if (static_cast<int>(n >> 5) > static_cast<int>(nMaskWords))
                        break;
                    if ((pMask[n >> 5] & (1 << (n & 31))) == 0 && g_bEnableFogOfWar != 0)
                    {
                        m_pBlackDot->DrawBuffered((fX - fQuadW * 0.5f) / static_cast<float>(m_extent.w),
                                                  (fY - fQuadH * 0.5f) / static_cast<float>(m_extent.h),
                                                  fQuadW / static_cast<float>(m_extent.w),
                                                  fQuadH / static_cast<float>(m_extent.h), 0, 0.0f,
                                                  Vector(-1.0f, -1.0f, -1.0f), 1.0f);
                    }
                    n++;
                    if (static_cast<int>(n) % (nResX + 1) == 0)
                    {
                        fX = 0.0f;
                        fY = fY + fStepY;
                    }
                    else
                    {
                        fX = fX + fStepX;
                    }
                } while (n != static_cast<unsigned int>(nCells));
            }
        }

        CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);

        int nDrawn = 0;
        CSWGuiExtent extent;
        int nMapX = 0;
        int nMapY = 0;

        CExoLinkedListNode* pNote = m_mapNotes.GetHeadPos();
        while (pNote != 0)
        {
            unsigned long long nNoteId = *m_mapNotes.GetAtPos(pNote);
            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nNoteId);
            if (pObject != 0 && pObject->AsSWSWaypoint() != 0)
            {
                CSWSWaypoint* pWaypoint = pObject->AsSWSWaypoint();
                if (pWaypoint->m_bMapNoteEnabled != 0 &&
                    g_pAppManager->m_pServerExoApp->GetModule()
                            ->m_pAreaMap->IsWorldPointExplored(pWaypoint->m_position) != 0)
                {
                    int nNoteX;
                    int nNoteY = 0;
                    if (g_pAppManager->m_pServerExoApp->GetModule()
                            ->m_pAreaMap->GetMapPixelFromWorldCoord(pWaypoint->m_position,
                                                                    nNoteX, nNoteY) != 0)
                    {
                        if (pNote == m_pCurrentMapNote)
                        {
                            extent.x = static_cast<int>(static_cast<float>(nNoteX - 15) * s_fScale);
                            extent.y = static_cast<int>(static_cast<float>(nNoteY - 15) * s_fScale);
                            extent.w = static_cast<int>(s_fScale * 30.0f);
                            extent.h = extent.w;
                            m_noteButton.m_tail.m_nUserData = nNoteId;
                            m_noteButton.SetExtent(extent);
                            m_noteButton.Draw(f);
                        }
                        else
                        {
                            extent.x = nNoteX - 12;
                            extent.y = nNoteY - 12;
                            extent.w = 24;
                            extent.h = 24;
                            if (static_cast<int>(m_aButtons.m_count) == nDrawn)
                            {
                                m_aButtons.Add(new CSWGuiButton());
                                m_aButtons.m_pArray[nDrawn]->Initialize(
                                    CSWGuiExtent(0, 0, 24, 24), CSWGuiTextParams(),
                                    CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"),
                                                       2, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0),
                                    CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"),
                                                       2, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0));
                                m_aButtons.m_pArray[nDrawn]->m_border.m_params.m_color =
                                    *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
                                m_aButtons.m_pArray[nDrawn]->m_borderHilight.m_params.m_color =
                                    *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
                                m_aButtons.m_pArray[nDrawn]->m_tail.bFocusOnHover = 0;
                                m_aButtons.m_pArray[nDrawn]->AddEvent(
                                    INPUTEVENT_GUI_A, m_pContainer,
                                    static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMap::OnMapNoteClicked));
                            }
                            m_aButtons.m_pArray[nDrawn]->m_tail.m_nUserData = nNoteId;
                            extent.x = static_cast<int>(extent.x * s_fScale);
                            extent.y = static_cast<int>(extent.y * s_fScale);
                            extent.w = static_cast<int>(extent.w * s_fScale);
                            extent.h = static_cast<int>(extent.h * s_fScale);
                            m_aButtons.m_pArray[nDrawn]->SetExtent(extent);
                            m_aButtons.m_pArray[nDrawn]->Draw(f);
                            nDrawn++;
                        }
                    }
                }
            }
            m_mapNotes.GetNext(pNote);
        }

        if (nDrawn < static_cast<int>(m_aButtons.m_count))
            m_aButtons.m_count = nDrawn;

        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        nMapX = 0;
        nMapY = 0;
        unsigned int nMembers = 3;
        if (pParty != 0)
            nMembers = static_cast<unsigned char>(pParty->m_nCharacterCount);
        if (pParty == 0 || nMembers >= 2)
        {
            for (unsigned int i = 1; i != nMembers; i++)
            {
                if (pAreaMap->GetPartyMemberMapLocation(i, nMapX, nMapY) != 0 &&
                    m_pImageCircle != 0)
                {
                    extent.x = static_cast<int>(static_cast<float>(nMapX - 8) * s_fScale);
                    extent.y = static_cast<int>(static_cast<float>(nMapY - 8) * s_fScale);
                    extent.w = static_cast<int>(s_fScale * 16.0f);
                    extent.h = extent.w;
                    m_pImageCircle->SetExtent(extent);
                    m_pImageCircle->Draw(f);
                }
                nMapX = 0;
                nMapY = 0;
            }
        }

        if (m_pImageBar != 0)
        {
            if (pAreaMap->GetPartyMemberMapLocation(0, nMapX, nMapY) != 0)
            {
                extent.x = static_cast<int>(s_fScale * static_cast<float>(nMapX - 20));
                extent.y = static_cast<int>(s_fScale * static_cast<float>(nMapY - 20));
                extent.w = static_cast<int>(s_fScale * 40.0f);
                extent.h = extent.w;
                if (AurGUISetupViewport(extent.x, extent.y, extent.w, extent.h,
                                        CSWGuiDrawable::NO_COLOURING, 0, 1.0f) != 0)
                {
                    CSWGuiImage* pBar = m_pImageBar;
#ifdef K2_FIX_ENGINE_BUGS
                    // The image draws at its own extent, which the constructor sized without the map's
                    // fit, so it sits in the top-left of this viewport.
                    if (pBar->m_extent.w != extent.w)
                        pBar->SetExtent(CSWGuiExtent(0, 0, extent.w, extent.w));
#endif
                    pBar->m_params.m_fRotate = pAreaMap->GetPartyMemberMapOrientation(0);
                    pBar->Draw(f);
                    AurGUICloseViewport();
                }
            }
        }
    }
    AurGUICloseViewport();
}
#else
void CSWGuiMapHider::Draw(float f)
{
    float fScale = GetResolutionTextScalar();
#ifdef K2_QOL_IMPROVEMENTS
    // The party's markers follow the map's screen rectangle, not the integer text scalar.
    float fPartyScale = m_extent.h > 0 ? static_cast<float>(m_extent.h) / 354.0f : fScale;
#else
    float fPartyScale = fScale;
#endif

    if (m_bMapAvailable == 0)
        return;

    if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                            Vector(-1.0f, -1.0f, -1.0f), 0, 1.0f) == 0)
        return;

    unsigned int nMaskWords;
    int nResX;
    int nResY;
    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nOffsetX = 0;
    int nOffsetY = 0;

    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nOffsetX, nOffsetY,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
#ifndef K2_UI_TOUCH
        K2MapLabelMultipliers(static_cast<CSWGuiInGameMap*>(m_pContainer)->m_lblMap, fXMult, fYMult);
#endif
    }

    CSWSAreaMap* pAreaMap = g_pAppManager->m_pServerExoApp->GetModule()->m_pAreaMap;
    if (pAreaMap != 0 && pAreaMap->m_nSize != 0)
    {
        unsigned int* pMask = static_cast<unsigned int*>(pAreaMap->GetMapData(nMaskWords, nResX, nResY));
        if (pMask != 0 && g_bShowFogOfWar != 0)
        {
            int nCells = (nResY + 1) * (nResX + 1);
            if (nCells >= 1)
            {
                float fStepX = (fXMult * 708.0f) / static_cast<float>(nResX);
                float fStepY = (fYMult * 354.0f) / static_cast<float>(nResY);
                float fQuadW = fStepX * 4.0f;
                float fQuadH = fStepY * 4.0f;
                unsigned int n = 0;
                float fX = 0.0f;
                float fY = 0.0f;
                do
                {
                    if (static_cast<int>(n >> 5) > static_cast<int>(nMaskWords))
                        break;
                    if ((pMask[n >> 5] & (1 << (n & 31))) == 0 && g_bEnableFogOfWar != 0)
                    {
                        m_pBlackDot->DrawBuffered((fX - fQuadW * 0.5f) / static_cast<float>(m_extent.w),
                                                  (fY - fQuadH * 0.5f) / static_cast<float>(m_extent.h),
                                                  fQuadW / static_cast<float>(m_extent.w),
                                                  fQuadH / static_cast<float>(m_extent.h), 0, 0.0f,
                                                  Vector(-1.0f, -1.0f, -1.0f), 1.0f);
                    }
                    n++;
                    if (static_cast<int>(n) % (nResX + 1) == 0)
                    {
                        fX = 0.0f;
                        fY = fY + fStepY;
                    }
                    else
                    {
                        fX = fX + fStepX;
                    }
                } while (n != static_cast<unsigned int>(nCells));
            }
        }

        CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);

        int nDrawn = 0;
        int nSize30 = static_cast<int>(fScale * 30.0f);
        float fHalf30 = fScale * 30.0f * 0.5f;
        int nSize24 = static_cast<int>(fScale * 24.0f);
        float fHalf24 = fScale * 24.0f * 0.5f;

        // The extent is reused per note, and the coordinate pair is shared with the party section
        // below.
        CSWGuiExtent extent;
        int nMapX = 0;
        int nMapY = 0;

        CExoLinkedListNode* pNote = m_mapNotes.GetHeadPos();
        while (pNote != 0)
        {
            unsigned long long nNoteId = *m_mapNotes.GetAtPos(pNote);
            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nNoteId);
            if (pObject != 0 && pObject->AsSWSWaypoint() != 0)
            {
                CSWSWaypoint* pWaypoint = pObject->AsSWSWaypoint();
                if (pWaypoint->m_bMapNoteEnabled != 0 || g_bShowFogOfWar == 0)
                {
                    if (g_pAppManager->m_pServerExoApp->GetModule()
                            ->m_pAreaMap->IsWorldPointExplored(pWaypoint->m_position) != 0 ||
                        g_bShowFogOfWar == 0)
                    {
                        int nNoteX;
                        int nNoteY = 0;
                        if (g_pAppManager->m_pServerExoApp->GetModule()
                                ->m_pAreaMap->GetMapPixelFromWorldCoord(pWaypoint->m_position,
                                                                        nNoteX, nNoteY) != 0)
                        {
                            nNoteX = static_cast<int>(fXMult * static_cast<float>(nNoteX));
                            nNoteY = static_cast<int>(fYMult * static_cast<float>(nNoteY));

                            if (pNote == m_pCurrentMapNote)
                            {
                                extent.x = static_cast<int>(static_cast<float>(nNoteX) - fHalf30);
                                extent.y = static_cast<int>(static_cast<float>(nNoteY) - fHalf30);
                                extent.w = nSize30;
                                extent.h = nSize30;
                                m_noteButton.m_tail.m_nUserData = nNoteId;
                                m_noteButton.SetExtent(extent);
                                m_noteButton.Draw(f);
                            }
                            else
                            {
                                extent.x = static_cast<int>(static_cast<float>(nNoteX) - fHalf24);
                                extent.y = static_cast<int>(static_cast<float>(nNoteY) - fHalf24);
                                extent.w = nSize24;
                                extent.h = nSize24;
                                nNoteX = static_cast<int>(fXMult * static_cast<float>(nNoteX));
                                nNoteY = static_cast<int>(fYMult * static_cast<float>(nNoteY));

                                if (static_cast<int>(m_aButtons.m_count) == nDrawn)
                                {
                                    m_aButtons.Add(new CSWGuiButton());
                                    m_aButtons.m_pArray[nDrawn]->Initialize(
                                        CSWGuiExtent(0, 0, 24, 24), CSWGuiTextParams(),
                                        CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"),
                                                           2, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0),
                                        CSWGuiBorderParams(CResRef(), CResRef(), CResRef("whitetarget"),
                                                           2, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0));
                                    // The text colour is a Vector, so copy the three floats whole.
                                    m_aButtons.m_pArray[nDrawn]->m_border.m_params.m_color =
                                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_TEXT);
                                    m_aButtons.m_pArray[nDrawn]->m_borderHilight.m_params.m_color =
                                        CGuiInGame::COLOR_NEW_BONUS;
                                    m_aButtons.m_pArray[nDrawn]->m_tail.bFocusOnHover = 0;
                                    m_aButtons.m_pArray[nDrawn]->AddEvent(
                                        INPUTEVENT_GUI_A, m_pContainer,
                                        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameMap::OnMapNoteClicked));
                                }
                                m_aButtons.m_pArray[nDrawn]->m_tail.m_nUserData = nNoteId;
                                m_aButtons.m_pArray[nDrawn]->SetExtent(extent);
                                m_aButtons.m_pArray[nDrawn]->Draw(f);
                                nDrawn++;
                            }
                        }
                    }
                }
            }
            m_mapNotes.GetNext(pNote);
        }

        // The previous pass may have rendered more buttons than this one does.
        if (nDrawn < static_cast<int>(m_aButtons.m_count))
            m_aButtons.m_count = nDrawn;

        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        nMapX = 0;
        nMapY = 0;
        unsigned int nMembers = 3;
        if (pParty != 0)
            nMembers = static_cast<unsigned char>(pParty->m_nCharacterCount);
        if (pParty == 0 || nMembers >= 2)
        {
            int nSize16 = static_cast<int>(fPartyScale * 16.0f);
            float fHalf16 = fPartyScale * 16.0f * 0.5f;
            for (unsigned int i = 1; i != nMembers; i++)
            {
                nMapX = 0;
                nMapY = 0;
                if (pAreaMap->GetPartyMemberMapLocation(i, nMapX, nMapY) != 0)
                {
                    nMapX = static_cast<int>(fXMult * static_cast<float>(nMapX));
                    nMapY = static_cast<int>(fYMult * static_cast<float>(nMapY));
                    if (m_pImageCircle != 0)
                    {
                        extent.x = static_cast<int>(static_cast<float>(nMapX) - fHalf16);
                        extent.y = static_cast<int>(static_cast<float>(nMapY) - fHalf16);
                        extent.w = nSize16;
                        extent.h = nSize16;
                        m_pImageCircle->SetExtent(extent);
                        m_pImageCircle->Draw(f);
                    }
                }
            }
        }

        if (m_pImageBar != 0)
        {
            if (pAreaMap->GetPartyMemberMapLocation(0, nMapX, nMapY) != 0)
            {
                int nSize40 = static_cast<int>(fPartyScale * 40.0f);
                float fHalf40 = fPartyScale * 40.0f * 0.5f;
                nMapX = static_cast<int>(fXMult * static_cast<float>(nMapX));
                nMapY = static_cast<int>(fYMult * static_cast<float>(nMapY));
                extent.x = static_cast<int>(static_cast<float>(nMapX) - fHalf40);
                extent.y = static_cast<int>(static_cast<float>(nMapY) - fHalf40);
                extent.w = nSize40;
                extent.h = nSize40;
                if (AurGUISetupViewport(extent.x, extent.y, extent.w, extent.h,
                                        CSWGuiDrawable::NO_COLOURING, 0, 1.0f) != 0)
                {
                    CSWGuiImage* pBar = m_pImageBar;
#ifdef K2_QOL_IMPROVEMENTS
                    // The image draws at its own extent, 40 x the text scalar.
                    if (pBar->m_extent.w != nSize40)
                        pBar->SetExtent(CSWGuiExtent(0, 0, nSize40, nSize40));
#endif
                    pBar->m_params.m_fRotate = pAreaMap->GetPartyMemberMapOrientation(0);
                    pBar->Draw(f);
                    AurGUICloseViewport();
                }
            }
        }
    }
    AurGUICloseViewport();
}
#endif

// The in-game galaxy map planet picker.


























extern int g_bDo3DGUIs;
extern CTlkTable* g_pTlkTable;

// EFFECTIVE (K1): one stack slot (the SpawnRoom Vector) and the landing-pad block order.
// The planet picker's constructor; the layout lives in the header. The 16 planet buttons are bound
// through the "GUITag" column of the same Planetary 2DA as the icons, and only when the row has
// one, with m_tail.m_nUserData set to the row index. BTN_ACCEPT and BTN_BACK take the virtual
// CSWGuiPanel button handlers; the planet buttons take the plain non-virtual OnPlanetClicked. Only
// the display viewport gets a model ("galaxy", animated "default"); the model viewport gets a room
// only, spawned outside the g_bDo3DGUIs gate.
CSWGuiInGameGalaxyMap::CSWGuiInGameGalaxyMap(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sIcon;
    CExoString sModel;
    CExoString sTag;
    int nName;
    int nDesc;
    m_nDisplayedPlanet = -1;
#if KOTOR1  // K1 loads the "galaxymap" layout, with no bar labels
    StartLoadFromLayout(CResRef("galaxymap"), 1, 1, 0);
#else
    StartLoadFromLayout(CResRef("galaxymap_p"), 1, 1, 0);
#endif
    // LBL_DESC is bound before LBL_PLANETNAME, and the planet model before the display.
    InitControl(m_lblDesc,          CExoString("LBL_DESC"),         1, 1, 0, 0, 0);
    InitControl(m_lblPlanetName,    CExoString("LBL_PLANETNAME"),   1, 1, 0, 0, 0);
    InitControl(m_sceneViewModel,   CExoString("3D_PlanetModel"),   1, 1, 0, 0, 0);
    InitControl(m_sceneViewDisplay, CExoString("3D_PlanetDisplay"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aBars[0],          CExoString("LBL_BAR1"),         1, 1, 0, 0, 0);
    InitControl(m_aBars[1],          CExoString("LBL_BAR2"),         1, 1, 0, 0, 0);
#endif
    InitControl(m_btnAccept,        CExoString("BTN_ACCEPT"),       1, 1, 0, 0, 0);
    InitControl(m_btnBack,          CExoString("BTN_BACK"),         1, 1, 0, 0, 0);

    for (int i = 0; i < 16; i++)
    {
        if (g_pRules->m_p2DArrays->m_pPlanetary->GetCExoStringEntry(
                i, CExoString("GUITag"), &sTag) != 0)
        {
            InitControl(m_aPlanetButtons[i], sTag, 1, 1, 0, 0, 0);
            m_aPlanetButtons[i].m_tail.bFocusOnHover = 0;
            m_aPlanetButtons[i].AddEvent(
                INPUTEVENT_GUI_A, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGalaxyMap::OnPlanetClicked));
            m_aPlanetButtons[i].m_tail.m_nUserData = i;
        }
    }
    StopLoadFromLayout();

#if KOTOR1
    // K1: rescale to the window, then hang the two side panels beside the 4:3 box.
    ScaleExtentForResolution();
    {
        CSWGuiExtent extent;
        extent.w = static_cast<int>(__builtin_ceilf(static_cast<float>(g_SDL_mode.h) * 204.0f / 512.0f));
        extent.h = g_SDL_mode.h;
        extent.x = static_cast<int>((static_cast<float>(g_SDL_mode.w) - __builtin_ceilf(static_cast<float>(g_SDL_mode.h) * 4.0f / 3.0f)) * 0.5f -
                         static_cast<float>(extent.w));
        extent.y = 0;
        m_imgSideLeft.Initialize(extent, CSWGuiImageParams(CResRef("and_sm_left"), 0, 0, 0.0f, SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE,
                                                           1.0f, Vector(1.0f, 1.0f, 1.0f)));
        extent.x = g_SDL_mode.w - extent.x - extent.w;
        m_imgSideRight.Initialize(extent, CSWGuiImageParams(CResRef("and_sm_right"), 0, 0, 0.0f, SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE,
                                                            1.0f, Vector(1.0f, 1.0f, 1.0f)));
    }
#endif

    // BTN_BACK is bound first, then BTN_ACCEPT.
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(
        INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(
        INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));

    if (g_bDo3DGUIs != 0)
    {
        CSWGuiScene* pScene = &m_sceneViewDisplay.m_scene;
        CAurCamera* pCamera = pScene->m_pCamera;
        pScene->m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0),
                                    Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        CAurObject* pModel = pScene->AddModel(CExoString("galaxy"), -1);
        if (pModel != 0)
        {
            pModel->PlayAnimation(const_cast<char*>("default"), 1.0f, 0, 0.0f);
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 0);
            pCamera->SetViewAngle(22.726f);
        }
    }
    // The model viewport gets a room only, outside the gate above.
    m_sceneViewModel.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0),
                                                 Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

    int i;
    for (i = 0; i < 16; i++)
    {
        g_pRules->m_p2DArrays->m_pPlanetary->GetCExoStringEntry(i, CExoString("Icon"), &sIcon);
        g_pRules->m_p2DArrays->m_pPlanetary->GetCExoStringEntry(i, CExoString("Model"), &sModel);
        g_pRules->m_p2DArrays->m_pPlanetary->GetINTEntry(i, CExoString("Name"), &nName);
        g_pRules->m_p2DArrays->m_pPlanetary->GetINTEntry(i, CExoString("Description"), &nDesc);
        m_aPlanetButtons[i].m_border.m_params.SetFillImage(CResRef(sIcon), 0);
        m_aPlanetButtons[i].m_borderHilight.m_params.SetFillImage(CResRef(sIcon), 0);
        // Through a pointer: SetActive must go through the vtable here.
        CSWGuiButton* pButton = &m_aPlanetButtons[i];
        pButton->SetActive(0);
        m_aPlanetInfo[i].m_nNameStrRef = nName;
        m_aPlanetInfo[i].m_nDescStrRef = nDesc;
        m_aPlanetInfo[i].m_resRefModel = sModel;
    }

    m_nFullScreenBGIndex = 3;
#if !KOTOR1
    m_nOldSelectedPlanet = -1;
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
#endif
}

// The base handler runs first, and an empty val returns immediately, the reverse of
// CSWGuiInGameMap::HandleInputEvent. Only the accept arm hides the galaxy map GUI, and the "you have
// not chosen a planet" message fires when the selection is unchanged from the one the panel opened
// with.
void CSWGuiInGameGalaxyMap::HandleInputEvent(int type, int val)
{
    CSWGuiPanel::HandleInputEvent(type, val);
    if (val == 0)
        return;

    switch (type)
    {
#if KOTOR1  // K1 runs the travel script for any selection; no already-here or locked-out boxes
    case INPUTEVENT_GUI_A:
    case INPUTEVENT_GUI_START:
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        if (g_pVirtualMachine != 0)
        {
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 1;
            CExoString sScript("k_sup_galaxymap");
            g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 0;
        }
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideGalaxyMapGui();
        break;
#else
    case INPUTEVENT_GUI_A:
    case INPUTEVENT_GUI_START:
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        {
            CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            if (pPartyTable->m_selectedPlanet == m_nOldSelectedPlanet)
            {
                CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                pBox->SetAllowCancel(0);
                pBox->SetCallback(0, 0, 0);
                pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pBox->SetMessage(STRREF_ALREADY_AT_LOCATION);
                g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
                break;
            }
            if (pPartyTable->GetPlanetSelectable(pPartyTable->m_selectedPlanet) != 0)
            {
                if (g_pVirtualMachine != 0)
                {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 1;
                    CExoString sScript("k_sup_galaxymap");
                    g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 0;
                }
            }
            else
            {
                if (pPartyTable->m_selectedPlanet == -1)
                    break;
                int nLockedOutReason = -1;
                if (g_pRules->m_p2DArrays->m_pPlanetary->GetINTEntry(
                        pPartyTable->m_selectedPlanet, CExoString("LockedOutReason"),
                        &nLockedOutReason) == 0)
                    break;
                CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                pBox->SetAllowCancel(0);
                pBox->SetCallback(0, 0, 0);
                pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pBox->SetMessage(static_cast<uint64_t>(nLockedOutReason));
                g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
                break;
            }
        }
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideGalaxyMapGui();
        break;
#endif
    case INPUTEVENT_GUI_DPAD_LEFT:
    case INPUTEVENT_GUI_DPAD_UP:
    case INPUTEVENT_GUI_KEY_UP:
    case INPUTEVENT_GUI_KEY_LEFT:
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        PrevPlanet();
        break;
    case INPUTEVENT_GUI_DPAD_RIGHT:
    case INPUTEVENT_GUI_DPAD_DOWN:
    case INPUTEVENT_GUI_KEY_DOWN:
    case INPUTEVENT_GUI_KEY_RIGHT:
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        NextPlanet();
        break;
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_BACK:
    case INPUTEVENT_GUI:
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideGalaxyMapGui();
        break;
    }
}

void CSWGuiInGameGalaxyMap::Draw(float f)
{
#if KOTOR1  // K1 draws the side panels on a full-window tinted layer first
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h, Vector(0.455f, 0.6f, 0.294f), 0, 1.0f))
    {
        m_imgSideLeft.Draw(f);
        m_imgSideRight.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
#endif
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable->m_selectedPlanet != -1)
    {
        CSWGuiButton* pButton = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
        pButton->SetActive(1);
    }
    CSWGuiPanel::Draw(f);
}

void CSWGuiInGameGalaxyMap::OnPlanetClicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nSelected = pPartyTable->m_selectedPlanet;
    if (nSelected != -1)
    {
        if (pControl->m_tail.m_nUserData == static_cast<unsigned long long>(nSelected))
            return;
        // Through a pointer: SetActive must go through the vtable here.
        CSWGuiButton* pButton = &m_aPlanetButtons[nSelected];
        pButton->SetActive(0);
    }
    pPartyTable->SetSelectedPlanet(static_cast<int>(pControl->m_tail.m_nUserData));
    DisplayPlanet(pPartyTable->m_selectedPlanet, 1);
}

void CSWGuiInGameGalaxyMap::PrevPlanet()
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable->m_selectedPlanet == -1)
        return;

    CSWGuiButton* pButton = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
    pButton->SetActive(0);

    int nIndex = pPartyTable->m_selectedPlanet;
    for (;;)
    {
        int nNext = nIndex - 1;
        nIndex = (nIndex < 1) ? GALAXY_MAP_PLANET_COUNT - 1 : nNext;
#if KOTOR1  // K1 also skips planets that are shown but not selectable
        if (pPartyTable->GetPlanetAvailable(nIndex) != 0 &&
            pPartyTable->GetPlanetSelectable(nIndex) != 0)
            break;
#else
        if (pPartyTable->GetPlanetAvailable(nIndex) != 0)
            break;
#endif
        if (nIndex - 1 == pPartyTable->m_selectedPlanet)
            goto found;
    }
    pPartyTable->SetSelectedPlanet(nIndex);
found:
    {
        CSWGuiButton* pNew = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
        pNew->SetActive(1);
    }
    DisplayPlanet(pPartyTable->m_selectedPlanet, 1);
}

void CSWGuiInGameGalaxyMap::NextPlanet()
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable->m_selectedPlanet == -1)
        return;

    CSWGuiButton* pButton = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
    pButton->SetActive(0);

    int nIndex = pPartyTable->m_selectedPlanet;
    for (;;)
    {
        int nNext = nIndex + 1;
        nIndex = (nIndex >= GALAXY_MAP_PLANET_COUNT - 1) ? 0 : nNext;
#if KOTOR1  // K1 also skips planets that are shown but not selectable
        if (pPartyTable->GetPlanetAvailable(nIndex) != 0 &&
            pPartyTable->GetPlanetSelectable(nIndex) != 0)
            break;
#else
        if (pPartyTable->GetPlanetAvailable(nIndex) != 0)
            break;
#endif
        if (nIndex + 1 == pPartyTable->m_selectedPlanet)
            goto found;
    }
    pPartyTable->SetSelectedPlanet(nIndex);
found:
    {
        CSWGuiButton* pNew = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
        pNew->SetActive(1);
    }
    DisplayPlanet(pPartyTable->m_selectedPlanet, 1);
}

void CSWGuiInGameGalaxyMap::OnPanelAdded()
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
#if KOTOR1
    if (pPartyTable->m_selectedPlanet == -1)
#else
    m_nOldSelectedPlanet = pPartyTable->m_selectedPlanet;
    if (m_nOldSelectedPlanet == -1)
#endif
    {
        for (unsigned int i = 0; i < GALAXY_MAP_PLANET_COUNT; i++)
        {
            if (pPartyTable->GetPlanetAvailable(i) != 0 &&
                pPartyTable->GetPlanetSelectable(i) != 0)
            {
                pPartyTable->SetSelectedPlanet(i);
                break;
            }
        }
    }

    int nPlanet;
    if (pPartyTable->m_selectedPlanet == -1)
    {
        nPlanet = -1;
    }
    else
    {
        CSWGuiButton* pButton = &m_aPlanetButtons[pPartyTable->m_selectedPlanet];
        pButton->SetActive(1);
        nPlanet = pPartyTable->m_selectedPlanet;
    }
    DisplayPlanet(nPlanet, 1);

    for (int i = 0; i < GALAXY_MAP_PLANET_COUNT; i++)
    {
        CSWGuiButton* pButton = &m_aPlanetButtons[i];
        pButton->m_tail.bVisible = pPartyTable->GetPlanetAvailable(i) & 1;
    }

    CSWGuiPanel::OnPanelAdded();
}

// Loads one Planetary 2DA row into the panel: the two TLK captions into LBL_DESC and
// LBL_PLANETNAME, then the row's Model resref into the planet display viewport.
void CSWGuiInGameGalaxyMap::DisplayPlanet(int nPlanet, int bFlag)
{
    CExoString sName;
    CExoString sDesc;
    if (nPlanet != -1 && m_nDisplayedPlanet != nPlanet)
    {
        if (m_aPlanetInfo[nPlanet].m_nNameStrRef != 0)
            sName = g_pTlkTable->GetSimpleString(m_aPlanetInfo[nPlanet].m_nNameStrRef);
        if (m_aPlanetInfo[nPlanet].m_nDescStrRef != 0)
            sDesc = g_pTlkTable->GetSimpleString(m_aPlanetInfo[nPlanet].m_nDescStrRef);
        CSWGuiScene* pScene = &m_sceneViewModel.m_scene;
        m_lblDesc.m_text.m_params.SetText(sDesc);
        m_lblPlanetName.m_text.m_params.SetText(sName);
#ifdef K2_QOL_IMPROVEMENTS
        // Both captions are set at the Text Size, each clipped to its own box. The planet name's
        // one-line strip sits directly on LBL_DESC, so neither box can grow.
        K2QolFitText(m_lblDesc.m_text, m_lblDesc.m_text.m_extent.h);
        K2QolFitText(m_lblPlanetName.m_text, m_lblPlanetName.m_text.m_extent.h);
#endif

        // The guard is the model list's count, so no GetModel call is needed.
        if (pScene->m_aModels.m_count != 0)
            pScene->RemoveModel(0, 1);

        if (m_aPlanetInfo[nPlanet].m_resRefModel.IsValid())
        {
            CExoString sModel;
            m_aPlanetInfo[nPlanet].m_resRefModel.CopyToString(sModel);
            if (g_bDo3DGUIs != 0)
            {
                CAurCamera* pCamera = pScene->m_pCamera;
                CAurObject* pModel = pScene->AddModel(sModel, -1);
                if (pModel != 0)
                {
                    if (bFlag != 0)
                    {
                        pModel->PlayAnimation(const_cast<char*>("zoomin"), 1.0f, AnimRun::kAnimOnce, 0.0f);
                        pModel->PlayAnimation(const_cast<char*>("rotate"), 1.0f, AnimRun::kAnimQueue, 0.0f);
                    }
                    pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 0);
                    pCamera->SetViewAngle(22.726f);
                }
            }
        }
        m_nDisplayedPlanet = nPlanet;
    }
}

void CSWGuiInGameGalaxyMap::OnPanelRemoved()
{
    for (int i = 0; i < GALAXY_MAP_PLANET_COUNT; i++)
    {
        CSWGuiButton* pButton = &m_aPlanetButtons[i];
        pButton->SetActive(0);
    }
    m_nDisplayedPlanet = -1;
    CSWGuiPanel::OnPanelRemoved();
}

// The destructor pair is implicit. The class declares none, so the vtable's ODR-use emits both
// from this TU.
