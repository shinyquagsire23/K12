// CSWGuiBarkBubble, the bark and VO subtitle bubble.


#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguibarkbubble.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "serverexoapp.h"
#include "nwsplayer.h"
#include "gameobjectarray.h"
#include "guiingame.h"
#include "swguimaininterfacepc2.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwcobject.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwsarea.h"
#include "nwtlktable.h"
#include "swguidrawable.h"
#include "exosound.h"
#include "Vector.h"
#include "worldtimer.h"
#include "swgui.h"
#include "exoresman.h"
#if KOTOR1
#include "aurgui.h"
#endif

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "soundprioritygroup.h"

#if KOTOR1
// K1's resolution factor: the screen height (g_SDL_mode +8) over the 768-pixel design height.
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

#ifdef K2_HOST_BUILD
// The desktop's achievement notice (compat/achievement_toast.cpp) asks for the next bark to be a
// corner bubble: at most 2/5 of the screen wide, in the bottom-right corner above the HUD controls
// there. The next bark of the game's own puts the bubble back where its layout says.
static bool s_bK2HostBarkCornerNext;
static bool s_bK2HostBarkInCorner;
void K2HostBarkInCorner() { s_bK2HostBarkCornerNext = true; }

static void K2HostBarkTakeCorner(CSWGuiBarkBubble& bubble)
{
    s_bK2HostBarkInCorner = s_bK2HostBarkCornerNext;
    s_bK2HostBarkCornerNext = false;
    if (!s_bK2HostBarkInCorner)
        return;
    CSWGuiExtent narrow = bubble.m_extent;
    const int nMaxW = bubble.m_pGuiManager->m_nScreenWidth * 2 / 5;
    if (narrow.w > nMaxW) {
        narrow.w = nMaxW;
        bubble.SetExtent(narrow);
    }
}
#endif

CSWGuiBarkBubble::CSWGuiBarkBubble(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
#if KOTOR1  // K1: the layout is "barkbubble"
    StartLoadFromLayout(CResRef("barkbubble"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("barkbubble_p"), 1, 1, 1);
#endif
    InitControl(m_label, "LBL_BARKTEXT", 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1  // K1: the layout rect is rescaled for the resolution first; no x/y override pair
    ScaleExtentForResolution();
#endif
    m_layoutExtent = m_extent;
    m_fBarkTime = 0.0f;
    m_pSoundBark = 0;
    m_nPlayResult = 0;
    m_bIgnoreDistance = 0;
    m_nBarkStartTime = 0;
#if !KOTOR1
    m_nOverrideX = -1;
    m_nOverrideY = -1;
#endif
}

CSWGuiBarkBubble::~CSWGuiBarkBubble()
{
    if (m_pSoundBark)
    {
        m_pSoundBark->Stop(0);
        delete m_pSoundBark;
        m_pSoundBark = 0;
    }
}

#if KOTOR1
// K1: no aspect multipliers and no x/y override. The height pads the text by the frame and 10
// pixels, scaled by the screen height over 768 and rounded up; an iPhone adds 5 more scaled pixels
// and sets the string's font scale to 1.2x that factor. The bark's position is not lifted to the
// walkmesh, and Play is tried once.
void CSWGuiBarkBubble::SetBark(unsigned long long objId, const CExoString& text,
                               const CResRef& resRef, int flag)
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
#ifdef K2_FIX_ENGINE_BUGS
    SetExtent(m_layoutExtent);
#endif
#ifdef K2_HOST_BUILD
    K2HostBarkTakeCorner(*this);
#endif

    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    unsigned long long speakerClientId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
    CGameObject* pSpeaker = g_pAppManager->m_pClientExoApp->GetGameObject(speakerClientId);

    CExoString sSpeech(text);
    CSWTlkTable* pTlk = g_pSWTlkTable;
    pTlk->m_pSpeakerObj = pSpeaker;
    pTlk->m_gender = pTlk->GetObjectGender();
    pTlk->m_bSuppressEnabled &= ~CSWTlkTable::SUPPRESS_ENABLED;
    pTlk->ParseStr(sSpeech);

    m_label.m_text.m_params.SetText(sSpeech.CStr());
    int nBorder = m_label.m_border.m_params.GetBorderDim();
    CSWGuiText* pText = &m_label.m_text;
    int nHeight = pText->GetIdealHeight() + static_cast<int>(__builtin_ceilf(s_fScale * (nBorder * 2 + 10)));
    int nScreenWidth;
    int nScreenHeight;
    if (IosIsIphone(nScreenWidth, nScreenHeight))
    {
        nHeight = static_cast<int>(s_fScale * 5.0f + nHeight);
        if (m_label.m_text.m_pString)
            m_label.m_text.m_pString->SetFontScale(s_fScale * 1.2f);
    }
    CSWGuiExtent extent(m_extent);
    extent.h = nHeight;
    SetExtent(extent);

    m_oidBark = objId;
    m_stateFlags &= ~PANEL_REMOVE_STATE_MASK;
    m_bIgnoreDistance = flag;

    m_fBarkTime = sSpeech.GetLength() * 0.11f + 1.0f;

    if (m_pSoundBark)
    {
        m_pSoundBark->Stop(0);
        delete m_pSoundBark;
        m_pSoundBark = 0;
    }
    m_pSoundBark = new CExoStreamingSoundSource(1);

    if (objId != OBJECT_INVALID)
    {
        CSWCObject* pObject =
            static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(objId)->AsSWCObject());
        Vector vPos = pObject->m_position;
        m_pSoundBark->Set3D(1);
        m_pSoundBark->SetReverbEnabled(0);
        m_pSoundBark->SetPosition(vPos);
    }
    m_pSoundBark->SetPriorityGroup(PRIORITY_GROUP_BARK_BUBBLES);
    m_pSoundBark->SetVolume(CExoSound::CMAX_VOLUME);
    m_nPlayResult = m_pSoundBark->Play(resRef, 8, 1, 0, 0);
}
#else
void CSWGuiBarkBubble::SetBark(unsigned long long objId, const CExoString& text,
                               const CResRef& resRef, int flag)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    SetExtent(m_layoutExtent);
#ifdef K2_HOST_BUILD
    K2HostBarkTakeCorner(*this);
#endif

    // Resolve the speaker: the server-side player creature, then its CSWSPlayer, then the
    // client-side object.
    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    unsigned long long speakerClientId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
    CGameObject* pSpeaker = g_pAppManager->m_pClientExoApp->GetGameObject(speakerClientId);

    // Speak the bark through the talk table, so its tokens are expanded for that object.
    CExoString sSpeech(text);
    CSWTlkTable* pTlk = g_pSWTlkTable;
    pTlk->m_pSpeakerObj = pSpeaker;
    pTlk->m_gender = pTlk->GetObjectGender();
    pTlk->m_bSuppressEnabled &= ~CSWTlkTable::SUPPRESS_ENABLED;
    pTlk->ParseStr(sSpeech);

    m_label.m_text.m_params.SetText(sSpeech.CStr());
    int nBorder = m_label.m_border.m_params.GetBorderDim();
    CSWGuiText* pText = &m_label.m_text;
    int nHeight = pText->GetIdealHeight() + nBorder * 2 + 10;
    CSWGuiExtent extent(m_extent);
    extent.h = nHeight;
    // The NWScript command's optional x/y override the layout-derived ones when set.
    if (m_nOverrideX != -1)
        extent.x = m_nOverrideX;
    if (m_nOverrideY != -1)
        extent.y = m_nOverrideY;

    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
    extent.x = static_cast<int>(extent.x * fXMult) + nXOffset;
    extent.y = static_cast<int>(extent.y * fYMult) + nYOffset;
    SetExtent(extent);

    m_oidBark = objId;
    m_stateFlags &= ~PANEL_REMOVE_STATE_MASK;
    m_bIgnoreDistance = flag;

    m_fBarkTime = (sSpeech.m_sString ? static_cast<int>(strlen(sSpeech.m_sString)) : 0) * 0.11f + 1.0f;

    if (m_pSoundBark)
    {
        m_pSoundBark->Stop(0);
        delete m_pSoundBark;
        m_pSoundBark = 0;
    }
    m_pSoundBark = new CExoStreamingSoundSource(1);

    // An invalid object id means the bark is played flat instead of positioned.
    if (objId != OBJECT_INVALID)
    {
        CSWCObject* pObject =
            static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(objId)->AsSWCObject());
        Vector vPos = pObject->m_position;

        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0 && pModule->m_pArea != 0)
        {
            CSWSArea* pServerArea = pModule->m_pArea->GetServerArea();
            if (pServerArea != 0)
            {
                float fHeight =
                    g_pAppManager->m_pServerExoApp->ComputeHeight(pServerArea->m_resRef, vPos, 0);
                if (vPos.z - fHeight < 0.5f)
                    vPos.z = fHeight + 0.5f;
            }
        }

        m_pSoundBark->Set3D(1);
        m_pSoundBark->SetReverbEnabled(0);
        m_pSoundBark->SetPosition(vPos);
    }
    m_pSoundBark->SetPriorityGroup(PRIORITY_GROUP_BARK_BUBBLES);
    m_pSoundBark->SetVolume(CExoSound::CMAX_VOLUME);
    m_nPlayResult = m_pSoundBark->Play(resRef, 8, 1, 0, 0);
    if (m_nPlayResult == 0)
        m_nPlayResult = m_pSoundBark->Play(resRef, 4, 1, 0, 0);
}
#endif

void CSWGuiBarkBubble::OnPanelAdded()
{
    m_nBarkStartTime =
        static_cast<int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->GetSnapshotTime() / 1000);
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiBarkBubble::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: the 5-pixel inset is scaled by the resolution factor
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    float fInset = s_fScale * 5.0f;
    CSWGuiExtent inset(static_cast<int>(fInset), static_cast<int>(fInset), static_cast<int>(extent.w - fInset * 2.0f),
                       static_cast<int>(extent.h - fInset * 2.0f));
#else
    CSWGuiExtent inset(5, 5, extent.w - 10, extent.h - 10);
#endif
    m_label.SetExtent(inset);
    CSWGuiPanel::SetExtent(extent);
}

#if !KOTOR1  // K2-only slot
bool CSWGuiBarkBubble::ShouldFreeMouseOnAdd()
{
    return false;
}
#endif

void CSWGuiBarkBubble::Pause()
{
    if (m_fBarkTime > 0.0f || m_pSoundBark)
    {
        if (m_pSoundBark)
            m_pSoundBark->Pause();
#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
        m_stateFlags = nFlags;
    }
}

void CSWGuiBarkBubble::Resume()
{
    if (m_fBarkTime > 0.0f || m_pSoundBark)
    {
        if (m_pSoundBark)
            m_pSoundBark->Resume();
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(this, 4, 1);
    }
}

void CSWGuiBarkBubble::StopBark(int a)
{
    if (a && m_pSoundBark)
    {
        m_pSoundBark->Stop(0);
        delete m_pSoundBark;
        m_pSoundBark = 0;
    }
    m_fBarkTime = 0.0f;
#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
    m_stateFlags = nFlags;
}

// The bubble fades while it is still up, and the panel-state handling below runs only on the
// not-drawn path.
void CSWGuiBarkBubble::Draw(float fAlpha)
{
#if KOTOR1
    // K1: the layout rect at the panel's height, full safe width when the minimap is off and
    // lifted over the action bar in combat -- no combat-pause hide, no x/y override, no tooltip
    // floor, no width or inset clamp.
    CSWGuiExtent extent(m_layoutExtent);
    extent.h = m_extent.h;
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMiniMapVisible())
    {
        int nMarginX;
        int nMarginY;
        m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
        extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;
#if defined(K2_FIX_ENGINE_BUGS) && !defined(K2_UI_TOUCH)
        // pc / xbox: the layout's 4:3 screen less its left gap on both sides, centred; on a 4:3
        // screen that is the clamped full width, on a wider one it is not stretched edge to edge.
        int nFrame = m_pGuiManager->m_nScreenHeight * 4 / 3;
        if (nFrame > extent.w)
            nFrame = extent.w;
        extent.w = nFrame - extent.x * 2;
        extent.x = (m_pGuiManager->m_nScreenWidth - extent.w) / 2;
#elif defined(K2_FIX_ENGINE_BUGS)
        // The width is the screen's but the left edge stays the layout's, kept on screen, as KOTOR
        // II clamps it.
        if (extent.w > m_pGuiManager->m_nScreenWidth - nMarginX * 2 - extent.x)
            extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2 - extent.x;
#endif
    }
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0)
        extent.y += static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetInterfaceOffset());
    SetExtent(extent);
#else
    // 13 is the combat-pause mode that hides the bubble outright.
    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_combatPauseMode == 13)
        return;

    // The rect starts as the layout's (m_layoutExtent) with the panel's current height, then clamped.
    CSWGuiExtent extent(m_layoutExtent);
    extent.h = m_extent.h;

    int nMarginX;
    int nMarginY;
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMiniMapVisible())
    {
        m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
        extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;
    }
    if (m_nOverrideX != -1)
        extent.x = m_nOverrideX;
    if (m_nOverrideY != -1)
        extent.y = m_nOverrideY;

    // The bubble is lifted above the action bar in combat.
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0)
        extent.y += static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetInterfaceOffset());
    int nY = extent.y;

    // The bottom limit is the action-queue tooltip's bottom edge plus the target-action menu's own
    // cached offset.
    CSWGuiMainInterface* pInterface =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
#ifdef K2_UI_XBOX
    // The console bubble sits under the target banner, not under the screen. The maininterface_x
    // layout has no LBL_CMBTMSGBG, so with no fallback the floor came out at the surface height and
    // every bark was drawn off the bottom of the screen; the fallback is the target banner plate's
    // bottom edge plus a 20-pixel gap. A mobile layout that binds LBL_CMBTMSGBG keeps the Android
    // behaviour.
    if (pInterface->m_lblCombatMsgBg.m_extent.w > 0) {
        int nLimit = pInterface->m_lblCombatMsgBg.m_extent.y + pInterface->m_lblCombatMsgBg.m_extent.h;
        nLimit += pInterface->m_targetActionMenu.m_aViewport[3];
        if (nY <= nLimit)
            extent.y = nLimit + 20;
    } else if (pInterface->m_targetActionMenu.m_lblNameBg.m_extent.w > 0) {
        const CSWGuiExtent& plate = pInterface->m_targetActionMenu.m_lblNameBg.m_extent;
        int nLimit = plate.y + plate.h;
        if (nY <= nLimit)
            extent.y = nLimit + 20;
    }
#else
    int nLimit = pInterface->m_lblCombatMsgBg.m_extent.y + pInterface->m_lblCombatMsgBg.m_extent.h;
    nLimit += g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_targetActionMenu
                  .m_aViewport[3];
    if (nY <= nLimit)
        extent.y = nLimit + 20;
#endif

    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
    int nMaxW = m_pGuiManager->m_nScreenWidth - nMarginX * 2 - extent.x;
    if (nMaxW < extent.w)
        extent.w = nMaxW;

    // Keep the bubble inside the screen's horizontal safe-area insets.
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        CSWGuiManager* pGuiMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        float fInset = static_cast<float>(static_cast<unsigned int>(pGuiMgr->m_nUIEdgeInset));
        float fX = static_cast<float>(extent.x);
        float fDelta;
        if (fX < fInset)
            fDelta = fInset - fX;
        else if (static_cast<float>(pGuiMgr->m_nScreenWidth) - fInset < static_cast<float>(extent.x + extent.w))
            fDelta = -(fInset - static_cast<float>(pGuiMgr->m_nScreenWidth - (extent.x + extent.w)));
        else
            fDelta = 0.0f;
        extent.x = static_cast<int>(fDelta + fX);
    }
    SetExtent(extent);
#endif
#ifdef K2_QOL_IMPROVEMENTS
    {
        // SetBark sized the bubble at the layout's width; it draws at this one. Re-fit the text as
        // it wraps here, so the height always holds it (centred text taller than its rect loses its
        // first lines), and keep the bubble on screen.
        int nPad = m_label.m_border.m_params.GetBorderDim() * 2 + 10;
        int nHeight = K2QolFitText(m_label.m_text,
                                   m_pGuiManager->m_nScreenHeight - nMarginY - extent.y - nPad)
                    + nPad;
        if (nHeight != extent.h) {
            extent.h = nHeight;
            SetExtent(extent);
        }
    }
#endif

#ifdef K2_HOST_BUILD
    if (s_bK2HostBarkInCorner) {
        int nCornerX;
        int nCornerY;
        m_pGuiManager->GetSafeMargin(nCornerX, nCornerY);
        const int nScreenW = m_pGuiManager->m_nScreenWidth;
        const int nScreenH = m_pGuiManager->m_nScreenHeight;
        const int nGap = nScreenH / 48;
        CSWGuiExtent corner = m_extent;
        if (corner.w > nScreenW * 2 / 5)
            corner.w = nScreenW * 2 / 5;
        corner.x = nScreenW - nCornerX - nGap - corner.w;
#ifdef K2_QOL_IMPROVEMENTS
        SetExtent(corner);
        corner.h = K2QolFitText(m_label.m_text, nScreenH / 3) + m_label.m_border.m_params.GetBorderDim() * 2 + 10;
#endif
        // Above the lowest HUD controls under the bubble's columns in the bottom half (K1's party
        // slots, K2's portraits).
        int nFloor = nScreenH - nCornerY;
        CSWGuiMainInterface* pHud = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
        for (int i = 0; pHud && i < pHud->m_controls.m_count; i++) {
            const CSWGuiControl* pControl = pHud->m_controls.m_pArray[i];
            if (!pControl || !pControl->m_tail.bVisible)
                continue;
            const CSWGuiExtent& e = pControl->m_extent;
            if (e.w <= 0 || e.h <= 0 || e.h > nScreenH / 3 || e.y < nScreenH / 2)
                continue;
            if (e.x + e.w <= corner.x || e.x >= corner.x + corner.w)
                continue;
            if (e.y < nFloor)
                nFloor = e.y;
        }
        corner.y = nFloor - nGap - corner.h;
        // The combat lift below is taken back off after the draw.
        if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0)
            corner.y += static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetInterfaceOffset());
        SetExtent(corner);
    }
#endif

    // The bubble is drawn only while its object is close enough to the party leader (36 world units
    // squared), unless the SetBark flag says to ignore the distance.
    bool bShow = true;
    if (m_oidBark != OBJECT_INVALID)
    {
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader != 0)
        {
            Vector vLeaderPos = pLeader->m_position;
            CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidBark);
            if (pGameObject == 0 || pGameObject->AsSWCObject() == 0)
            {
                bShow = false;
            }
            else if (!m_bIgnoreDistance)
            {
                Vector vPos = static_cast<CSWCObject*>(pGameObject->AsSWCObject())->m_position;
                float fDx = vLeaderPos.x - vPos.x;
                float fDy = vLeaderPos.y - vPos.y;
                float fDz = vLeaderPos.z - vPos.z;
                bShow = fDx * fDx + fDy * fDy + fDz * fDz < 36.0f;
            }
        }
    }

    bool bDraw = false;
    if (m_pSoundBark != 0)
    {
#if KOTOR1  // K1: the sound pointer is re-tested before GetPlayFailed (a null one hides the bubble)
        if (m_pSoundBark->IsPlaying())
            bDraw = bShow;
        else if (m_pSoundBark != 0)
        {
            if (m_pSoundBark->GetPlayFailed() == 0 && m_nPlayResult != 0)
            {
                if (m_pSoundBark != 0)
                {
                    m_pSoundBark->Stop(0);
                    delete m_pSoundBark;
                    m_pSoundBark = 0;
                }
            }
            else
                bDraw = bShow && m_fBarkTime > 0.0f;
        }
#else
        if (m_pSoundBark->IsPlaying())
            bDraw = bShow;
        else if (m_pSoundBark->GetPlayFailed() == 0 && m_nPlayResult != 0)
        {
            if (m_pSoundBark != 0)
            {
                m_pSoundBark->Stop(0);
                delete m_pSoundBark;
                m_pSoundBark = 0;
            }
        }
        else
            bDraw = bShow && m_fBarkTime > 0.0f;
#endif
    }

    if (bDraw)
    {
        CSWGuiPanel::Draw(fAlpha);
        float fDelta = m_pGuiManager->m_fDeltaTStore;
        if (fDelta < 1.5f)
            m_fBarkTime -= fDelta;
    }
    else
    {
#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
        m_stateFlags = nFlags;
    }

    // The bubble tracks the action bar's vertical offset while it is up.
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0)
#if KOTOR1  // K1: through the base class's SetExtent, not a raw field write
    {
        CSWGuiExtent rect = m_extent;
        rect.y -= static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetInterfaceOffset());
        CSWGuiObject::SetExtent(rect);
    }
#else
        m_extent.y -= static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetInterfaceOffset());
#endif
}
