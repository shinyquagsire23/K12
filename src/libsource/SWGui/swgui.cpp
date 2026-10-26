// The GUI manager, its panels and the GUI rectangle type. All four classes share one translation
// unit because ~CSWGuiPanel calls CSWGuiManager::PanelExists and GetPosInPanelList.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "port/Mac/MacMain.h"
#include "swgui.h"

#include "exosound.h"
#include "twodimarray.h"
#include "swguiingame.h"
#include "exoresman.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "guiingame.h"
#include "swguimaininterfacepc2.h"
#include "ClientExoAppInternal.h"
#include "exoinput.h"
#include "AURORA/aurmodel.h"
#include "aurgui.h"
#include "aurcamera.h"
#include "exobase.h"
#include "exotimersinternal.h"

#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "restype.h"
#include "soundprioritygroup.h"

char* g_asMouseCursorNames[] = {
    const_cast<char*>(""), const_cast<char*>("gui_mp_defaultu"), const_cast<char*>("gui_mp_defaultd"), const_cast<char*>("gui_mp_walku"), const_cast<char*>("gui_mp_walkd"), const_cast<char*>("gui_mp_invalidu"),
    const_cast<char*>("gui_mp_invalid"), const_cast<char*>("gui_mp_bashu"), const_cast<char*>("gui_mp_bashd"), const_cast<char*>("gui_mp_bashup"), const_cast<char*>("gui_mp_bashdp"),
    const_cast<char*>("gui_mp_talku"), const_cast<char*>("gui_mp_talkd"), const_cast<char*>("gui_mp_notalku"), const_cast<char*>("gui_mp_notalkd"), const_cast<char*>("gui_mp_followu"),
    const_cast<char*>("gui_mp_followd"), const_cast<char*>("gui_mp_examineu"), const_cast<char*>("gui_mp_examined"), const_cast<char*>("gui_mp_noexamu"), const_cast<char*>("gui_mp_noexamd"),
    const_cast<char*>("gui_mp_transu"), const_cast<char*>("gui_mp_transd"), const_cast<char*>("gui_mp_dooru"), const_cast<char*>("gui_mp_doord"), const_cast<char*>("gui_mp_useu"), const_cast<char*>("gui_mp_used"),
    const_cast<char*>("gui_mp_useup"), const_cast<char*>("gui_mp_usedp"), const_cast<char*>("gui_mp_magicu"), const_cast<char*>("gui_mp_magicd"), const_cast<char*>("gui_mp_nomagicu"),
    const_cast<char*>("gui_mp_nomagicd"), const_cast<char*>("gui_mp_dismineu"), const_cast<char*>("gui_mp_dismined"), const_cast<char*>("gui_mp_dismineup"), const_cast<char*>("gui_mp_disminedp"),
    const_cast<char*>("gui_mp_recmineu"), const_cast<char*>("gui_mp_recmined"), const_cast<char*>("gui_mp_recmineup"), const_cast<char*>("gui_mp_recminedp"), const_cast<char*>("gui_mp_locku"),
    const_cast<char*>("gui_mp_lockd"), const_cast<char*>("gui_mp_doorup"), const_cast<char*>("gui_mp_doordp"), const_cast<char*>("gui_mp_selectu"), const_cast<char*>("gui_mp_selectd"),
    const_cast<char*>("gui_mp_createu"), const_cast<char*>("gui_mp_created"), const_cast<char*>("gui_mp_nocreatu"), const_cast<char*>("gui_mp_nocreatd"), const_cast<char*>("gui_mp_killu"),
    const_cast<char*>("gui_mp_killd"), const_cast<char*>("gui_mp_nokillu"), const_cast<char*>("gui_mp_nokilld"), const_cast<char*>("gui_mp_healu"), const_cast<char*>("gui_mp_heald"),
    const_cast<char*>("gui_mp_nohealu"), const_cast<char*>("gui_mp_noheald"), const_cast<char*>("gui_mp_pickupu"), const_cast<char*>("gui_mp_pickupd"), const_cast<char*>("gui_mp_arrun00"),
    const_cast<char*>("gui_mp_arrun01"), const_cast<char*>("gui_mp_arrun02"), const_cast<char*>("gui_mp_arrun03"), const_cast<char*>("gui_mp_arrun04"), const_cast<char*>("gui_mp_arrun05"),
    const_cast<char*>("gui_mp_arrun06"), const_cast<char*>("gui_mp_arrun07"), const_cast<char*>("gui_mp_arrun08"), const_cast<char*>("gui_mp_arrun09"), const_cast<char*>("gui_mp_arrun10"),
    const_cast<char*>("gui_mp_arrun11"), const_cast<char*>("gui_mp_arrun12"), const_cast<char*>("gui_mp_arrun13"), const_cast<char*>("gui_mp_arrun14"), const_cast<char*>("gui_mp_arrun15"),
    const_cast<char*>("gui_mp_arwalk00"), const_cast<char*>("gui_mp_arwalk01"), const_cast<char*>("gui_mp_arwalk02"), const_cast<char*>("gui_mp_arwalk03"), const_cast<char*>("gui_mp_arwalk04"),
    const_cast<char*>("gui_mp_arwalk05"), const_cast<char*>("gui_mp_arwalk06"), const_cast<char*>("gui_mp_arwalk07"), const_cast<char*>("gui_mp_arwalk08"), const_cast<char*>("gui_mp_arwalk09"),
    const_cast<char*>("gui_mp_arwalk10"), const_cast<char*>("gui_mp_arwalk11"), const_cast<char*>("gui_mp_arwalk12"), const_cast<char*>("gui_mp_arwalk13"), const_cast<char*>("gui_mp_arwalk14"),
    const_cast<char*>("gui_mp_arwalk15")
};

int g_nGUIWidth = 800;
int g_nGUIHeight = 600;
int g_nUIEdgeOffset = 80;

#if !KOTOR1
int g_scaleMipc;
#endif
int g_bDrawSafeRegion;


#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/k2qolapi.h"
#include "port/aslglapi.h"

CExoString CSWGuiManager::FULLSCREEN_BGS[5] = {
    "back", "store", "pazaak", "map", "comp",
};
#if KOTOR1  // K1's disabled-text colour differs, and COLOR_YELLOW exists only there
const Vector CSWGuiManager::DISABLED_TEXT(0.0f, 0.33f, 0.49f);
Vector CSWGuiManager::COLOR_YELLOW(0.98f, 1.0f, 0.0f);
#else
const Vector CSWGuiManager::DISABLED_TEXT(0.13f, 0.9f, 0.7f);
#endif

extern int g_scaleMipc;

extern int  g_bDrawSafeRegion;
int GetMiniGamePlayingType();

CSWGuiPanel::CSWGuiPanel(CSWGuiManager* pGuiManager)
{
#if KOTOR1
    m_vColor = Vector();
    m_pBackground = new CSWGuiBorder();
    m_fAlpha = 1.0f;
    m_nOpenSoundIndex = static_cast<char>(-1);
    m_pGuiManager = pGuiManager;
    m_pActiveControl = 0;
    m_pLayoutRes = 0;
    m_nFullScreenBGIndex = 0;
    m_stateFlags = (m_stateFlags & 0xf818) | (PANEL_STATE_SHOWN | PANEL_STATE_DRAW);
    m_bAllowFontReset = 1;
#else
    m_bDrawOffScreen = 0;
    m_vColor = Vector();
    m_bIsMainInterface = 0;
    m_pBackground = new CSWGuiBorder();
    m_fAlpha = 1.0f;
    m_nOpenSoundIndex = static_cast<char>(-1);
    m_bAllowFontReset = 1;
    m_pGuiManager = pGuiManager;
    m_pActiveControl = 0;
    m_pLayoutRes = 0;
    m_nFullScreenBGIndex = 0;
    m_stateFlags = (m_stateFlags & 0xfc08) | (PANEL_STATE_NATIVE_COORDS | PANEL_STATE_SHOWN | PANEL_STATE_DRAW);
    m_pNextInChain = 0;
#endif
}

#if KOTOR1
void CSWGuiObject::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;
}
#endif

// EFFECTIVE: one member address is formed earlier in the orig (an add moves, 4 B).
CSWGuiPanel::~CSWGuiPanel()
{
    delete m_pBackground;
    m_pBackground = 0;

    StopLoadFromLayout();

    delete m_pLayoutRes;
    m_pLayoutRes = 0;

    if (m_pGuiManager && m_pGuiManager->PanelExists(this))
        m_pGuiManager->RemovePanel(this);

#if !KOTOR1
    if (m_pNextInChain) {
        delete m_pNextInChain;
        m_pNextInChain = 0;
    }
#endif
}

int CSWGuiManager::GetPosInPanelList(CSWGuiPanel* pPanel)
{
    int nCount = m_panels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* p = m_panels.m_pArray[i];
        if (p && p == pPanel)
            return i;
    }
    return -1;
}

int CSWGuiManager::PanelExists(CSWGuiPanel* pPanel)
{
    return GetPosInPanelList(pPanel) != -1;
}

#if KOTOR1
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#include <stdio.h>
#ifndef K2_UI_TOUCH
// The K1 PC/Xbox build reads the PC layouts (authored for 1024x768) and scales every extent as it
// loads; the scale is keyed by the layout's CResGFF so a panel built while another is loading keeps
// its own factor.
struct K1LayoutScale { CResGFF* pGFF; float fScale; };
static K1LayoutScale s_aK1LayoutScale[16];

static float K1LayoutDesignWidth(const CExoString& sName)
{
    static const struct { const char* pszSuffix; float fWidth; } s_aSizes[] = {
        {"16x12", 1600.0f}, {"12x10", 1280.0f}, {"12x9", 1280.0f}, {"10x8", 1024.0f},
        {"10x7", 1024.0f}, {"8x6", 800.0f},
    };
    CExoString sLower = sName.LowerCase();
    const char* psz = sLower.CStr();
    uint64_t n = strlen(psz);
    for (unsigned int i = 0; i < sizeof(s_aSizes) / sizeof(s_aSizes[0]); i++) {
        uint64_t nSuffix = strlen(s_aSizes[i].pszSuffix);
        if (n >= nSuffix && strcmp(psz + n - nSuffix, s_aSizes[i].pszSuffix) == 0)
            return s_aSizes[i].fWidth;
    }
    return 640.0f;
}

static void K1LayoutScaleSet(CResGFF* pGFF, float fScale)
{
    for (unsigned int i = 0; i < 16; i++)
        if (s_aK1LayoutScale[i].pGFF == pGFF || s_aK1LayoutScale[i].pGFF == 0) {
            s_aK1LayoutScale[i].pGFF = fScale == 1.0f ? 0 : pGFF;
            s_aK1LayoutScale[i].fScale = fScale;
            return;
        }
}

int g_nK1LayoutScaleSuspend = 0;

void K1ScaleLoadedExtent(CSWGuiExtent& extent, CResGFF* pGFF)
{
    if (g_nK1LayoutScaleSuspend)
        return;
    for (unsigned int i = 0; i < 16; i++)
        if (s_aK1LayoutScale[i].pGFF == pGFF && pGFF != 0) {
            float f = s_aK1LayoutScale[i].fScale;
            int nRight = static_cast<int>(static_cast<float>(extent.x + extent.w) * f + 0.5f);
            int nBottom = static_cast<int>(static_cast<float>(extent.y + extent.h) * f + 0.5f);
            extent.x = static_cast<int>(static_cast<float>(extent.x) * f + 0.5f);
            extent.y = static_cast<int>(static_cast<float>(extent.y) * f + 0.5f);
            extent.w = nRight - extent.x;
            extent.h = nBottom - extent.y;
            return;
        }
}

static void K1LayoutScaleRegister(CResGFF* pGFF, const CResRef& resRef)
{
    unsigned int nSource = 0;
    float fScale = 1.0f;
    if (g_pExoResMan->Exists(resRef, RESTYPE_GUI, &nSource) && nSource != KEYTABLE_SOURCE_DIRECTORY) {
        CExoString sName;
        resRef.CopyToString(sName);
        // The Pazaak screens have no touch re-layout: every UI loads the archive's 640x480
        // layouts, and their K1 bodies scale them to the window themselves.
        CExoString sLower = sName.LowerCase();
        if (strstr(sLower.CStr(), "pazaak") != sLower.CStr())
            fScale = 1024.0f / K1LayoutDesignWidth(sName);
    }
    K1LayoutScaleSet(pGFF, fScale);
}
#endif
void CSWGuiPanel::StartLoadFromLayout(const CResRef& resRef)
{
    if (!(m_stateFlags & 2))
    {
        CExoString sName;
        resRef.CopyToString(sName);

#ifdef K2_UI_TOUCH
        int nScreenWidth;
        int nScreenHeight;
        if (IosIsIphone(nScreenWidth, nScreenHeight))
#else
        // pc / xbox: the x-prefixed touch layouts are never loaded.
        if (false)
#endif
        {
            char szName[256];
            sprintf(szName, "x%s", sName.CStr());
            CResRef iphoneRes(szName);
            if (g_pExoResMan->Exists(iphoneRes, RESTYPE_GUI))
                m_pLayoutRes = new CResGFF(RESTYPE_GUI, const_cast<char*>("GUI "), iphoneRes);
        }

        CResGFF* pResGFF = m_pLayoutRes;
        if (!pResGFF)
        {
            pResGFF = new CResGFF(RESTYPE_GUI, const_cast<char*>("GUI "), resRef);
            m_pLayoutRes = pResGFF;
        }
#ifndef K2_UI_TOUCH
        K1LayoutScaleRegister(pResGFF, resRef);
#endif

        if (pResGFF->Demand())
        {
            CResStruct resStruct;
            int bSuccess;
            pResGFF->GetTopLevelStruct(&resStruct);
            m_extent.Load(pResGFF, resStruct);
            Vector vColor(-1.0f, -1.0f, -1.0f);
            m_vColor = pResGFF->ReadFieldVector(&resStruct, const_cast<char*>("COLOR"), bSuccess, vColor);

            CResStruct borderStruct;
            if (pResGFF->GetStructFromStruct(&borderStruct, &resStruct, const_cast<char*>("BORDER")))
            {
                m_pBackground->Load(pResGFF, resStruct, CExoString("BORDER"));
                CSWGuiExtent borderExtent;
                borderExtent.x = 0;
                borderExtent.y = 0;
                borderExtent.w = m_extent.w;
                borderExtent.h = m_extent.h;
                m_pBackground->SetExtent(borderExtent);
            }
            else
            {
                m_pBackground->m_params.SetFillImage(
                    pResGFF->ReadFieldCResRef(&resStruct, const_cast<char*>("BACKGROUND"), bSuccess, CResRef("")), 0);
            }

            float fAlpha = pResGFF->ReadFieldFLOAT(&resStruct, const_cast<char*>("ALPHA"), bSuccess, 1.0f);
            fAlpha = fAlpha < 1.0f ? fAlpha : 1.0f;
            fAlpha = fAlpha > 0.0f ? fAlpha : 0.0f;
            m_fAlpha = fAlpha;

            pResGFF->GetList(&m_layoutList, &resStruct, const_cast<char*>("CONTROLS"));
            m_stateFlags |= 2;
        }
    }
    m_stateFlags |= 1;
}
#else
void CSWGuiPanel::StartLoadFromLayout(const CResRef& resRef, int bScaleOrigin, int bScaleSize,
                                      int bApplyOffset)
{
    if (m_stateFlags & 2)
        return;

    m_bCenterOnNativeRect = bApplyOffset;

    // A local, not the member: the extent load takes a pointer that may alias the member.
    CResGFF* pResGFF = new CResGFF(RESTYPE_GUI, const_cast<char*>("GUI "), resRef);
    m_pLayoutRes = pResGFF;
    if (pResGFF->Demand())
    {
        CResStruct resStruct;
        int bSuccess;
        pResGFF->GetTopLevelStruct(&resStruct);
        m_extent.Load(pResGFF, resStruct);
        Vector vColor(-1.0f, -1.0f, -1.0f);
        m_vColor = pResGFF->ReadFieldVector(&resStruct, const_cast<char*>("COLOR"), bSuccess, vColor);

        // Existence test only: the border sub-struct is looked up in the parent struct itself.
        CResStruct borderStruct;
        if (pResGFF->GetStructFromStruct(&borderStruct, &resStruct, const_cast<char*>("BORDER")))
        {
            m_pBackground->Load(pResGFF, resStruct, CExoString("BORDER"));
            CSWGuiExtent borderExtent;
            borderExtent.x = 0;
            borderExtent.y = 0;
            borderExtent.w = m_extent.w;
            borderExtent.h = m_extent.h;
            m_pBackground->SetExtent(borderExtent);
        }
        else
        {
            m_pBackground->m_params.SetFillImage(
                pResGFF->ReadFieldCResRef(&resStruct, const_cast<char*>("BACKGROUND"), bSuccess, CResRef("")), 0);
        }

        float fAlpha = pResGFF->ReadFieldFLOAT(&resStruct, const_cast<char*>("ALPHA"), bSuccess, 1.0f);
        fAlpha = fAlpha < 1.0f ? fAlpha : 1.0f;
        fAlpha = fAlpha > 0.0f ? fAlpha : 0.0f;
        m_fAlpha = fAlpha;

        pResGFF->GetList(&m_layoutList, &resStruct, const_cast<char*>("CONTROLS"));
        m_stateFlags |= 2;
    }

    CSWGuiExtent extent = m_extent;

    // g_nGUIWidth / g_nGUIHeight are unsigned in the build even though the header declares them
    // int, so the casts stay at the use sites.
    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset;
    int nYOffset;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager())
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
    }
    else
    {
        fXMult = static_cast<float>(g_nScreenWidth) / static_cast<float>(static_cast<unsigned int>(g_nGUIWidth));
        fYMult = static_cast<float>(g_nScreenHeight) / static_cast<float>(static_cast<unsigned int>(g_nGUIHeight));
    }

    if (bScaleSize)
    {
        extent.w = static_cast<int>(extent.w * fXMult);
        extent.h = static_cast<int>(extent.h * fYMult);
    }
    if (bScaleOrigin)
    {
        extent.x = static_cast<int>(extent.x * fXMult);
        extent.y = static_cast<int>(extent.y * fYMult);
    }

    m_pBackground->SetExtent(extent);
    if (bApplyOffset)
    {
        extent.x += g_nScreenLeftOffset;
        extent.y += g_nScreenTopOffset;
    }
    SetExtent(extent);

    m_stateFlags |= 1;
}
#endif

#if KOTOR1
void CSWGuiPanel::InitControl(CSWGuiControl& control, const CExoString& sControlName, int bAddToTable)
{
    control.Load(this, m_pLayoutRes, m_layoutList, sControlName);

    if (bAddToTable)
    {
        int nIndex = control.m_tail.nControlId;
        if (nIndex >= 0)
        {
            if (nIndex < m_controls.m_count)
            {
                m_controls.m_pArray[nIndex] = &control;
            }
            else
            {
                while (nIndex > m_controls.m_count)
                    m_controls.Add(0);
                m_controls.Add(&control);
            }
        }
    }
}
#else
#ifndef K2_UI_TOUCH
// pc / xbox: a list's SetExtent keeps its scroll bar's layout width, so the bar is scaled with the
// list, as K1 scales it; the slim (xbox conversation) bar keeps it.
static void K2ScaleListScrollBar(CSWGuiControl& control, float fXMult)
{
    CSWGuiListBox* pList = control.AsListBox();
    if (pList && !pList->m_scrollBar.bSlimBar)
        pList->m_scrollBar.m_extent.w = static_cast<int>(__builtin_ceilf(pList->m_scrollBar.m_extent.w * fXMult));
}
#endif

// Binds a layout control to one of the panel's member controls and re-expresses its extent in
// screen space: offset, letterbox scale, edge snap, safe-area push, then the id-addressed insert.
void CSWGuiPanel::InitControl(CSWGuiControl& control, const CExoString& sControlName,
                              int bAddToTable, int bScale, int bOffsetToEdge, int nXAdd, int nWAdd)
{
    control.Load(this, m_pLayoutRes, m_layoutList, sControlName);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkLayoutLabel(control, sControlName);   // the store / crafting value + skill readouts
#endif

    control.m_bCenterOnNativeRect = m_bCenterOnNativeRect;

    CSWGuiExtent extent = control.m_extent;
    extent.x += nXAdd;
    if (nWAdd >= 1)
        extent.w += nWAdd;

    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset;
    int nYOffset;
    if (bScale)
    {
        if (g_pAppManager->m_pClientExoApp->GetSWGuiManager())
            g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
                m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
                static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
    }

    extent.x = static_cast<int>(extent.x * fXMult);
    extent.w = static_cast<int>(extent.w * fXMult);
    extent.y = static_cast<int>(extent.y * fYMult);
    extent.h = static_cast<int>(extent.h * fYMult);

    // pc-layout edge-snap: each axis is pushed to the screen half it already sits in and, when the
    // result lands in the middle 8/16ths of the screen, spread to that half's 4:3 letterbox scale.
    if (bScale && g_scaleMipc != 0 && g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        float fL = static_cast<float>(extent.x);
        float fT = static_cast<float>(extent.y);
        float fR = static_cast<float>(extent.x) + static_cast<float>(extent.w);
        float fB = static_cast<float>(extent.y) + static_cast<float>(extent.h);
        float fScreenW = static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth);
        float fScreenH = static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight);

        struct ScaleToEdge
        {
            float fScreenW;
            float fScreenH;
            float fScaleX;

            float ScaleWidth(float fPos, float fOther)
            {
                float fHalf = fScreenW * 0.5f;
                bool bEdge = (g_scaleMipc == 4);
                if (!bEdge && fOther >= fScreenH * 0.5f && g_scaleMipc == 1)
                {
                    float fFrac = fPos / fScreenW;
                    bEdge = (fFrac >= 0.4375f && fFrac <= 0.5625f);
                }
                if (bEdge)
                    return fHalf + (fPos - fHalf) * fScaleX;
                if (fHalf > fPos || g_scaleMipc == 2)
                    return fPos * fScaleX;
                return fScreenW - (fScreenW - fPos) * fScaleX;
            }

            float ScaleHeight(float fPos)
            {
                float fHalf = fScreenH * 0.5f;
                if (!(fHalf > fPos || g_scaleMipc == 2))
                    return fScreenH - (fScreenH - fPos) * 1.0f;
                return fPos * 1.0f;
            }
        };

        ScaleToEdge ste;
        ste.fScreenW = fScreenW;
        ste.fScreenH = fScreenH;
        ste.fScaleX = 1.3333334f / (fScreenW / fScreenH);

        // The right edge is scaled with the already-scaled top edge; the heights have no band test.
        float fNewL = ste.ScaleWidth(fL, fT);
        float fNewT = ste.ScaleHeight(fT);
        float fNewR = ste.ScaleWidth(fR, fNewT);
        float fNewB = ste.ScaleHeight(fB);

        extent.x = static_cast<int>(fNewL);
        extent.y = static_cast<int>(fNewT);
        extent.w = static_cast<int>(fNewR - fNewL);
        extent.h = static_cast<int>(fNewB - fNewT);
    }

    // Push the widget toward the safe-area edge it is already nearest.
    if (bOffsetToEdge)
    {
        CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        extent.x += extent.x >= g_nScreenWidth / 2 ? -pGuiManager->m_nUIEdgeInset
                                                   : pGuiManager->m_nUIEdgeInset;
    }

#ifndef K2_UI_TOUCH
    K2ScaleListScrollBar(control, fXMult);
#endif
    control.SetExtent(extent);

    // A progress/status bar keeps the layout file's default (flat) fill instead of its own.
    if (strstr(sControlName.CStr(), "LBL_BAR"))
    {
        CSWGuiLabel* pLabel = control.AsLabel();
        if (pLabel)
            pLabel->m_border.m_params.m_flags.nFillStyle = 2;
    }

    // Index the control into the panel's id-addressed control table; a control already occupying
    // that slot is relocated to the tail and renumbered rather than overwritten.
    if (bAddToTable)
    {
        int nIndex = control.m_tail.nControlId;
        if (nIndex >= 0)
        {
            if (nIndex < m_controls.m_count)
            {
                CSWGuiControl* pOld = m_controls.m_pArray[nIndex];
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
                // A re-layout (SetVideoMode) binds a control into the slot it already
                // holds; moving it to the tail would list it twice.
                if (pOld == &control)
                    pOld = 0;
#endif
                if (pOld)
                {
                    m_controls.Add(pOld);
                    m_controls.m_pArray[nIndex] = 0;
                    K2_ASSUME(m_controls.m_pArray[m_controls.m_count - 1] != 0);   // pOld, which Add appended past nIndex
                    m_controls.m_pArray[m_controls.m_count - 1]->m_tail.nControlId = m_controls.m_count - 1;
                }
                m_controls.m_pArray[nIndex] = &control;
            }
            else
            {
                while (nIndex > m_controls.m_count)
                    m_controls.Add(0);
                m_controls.Add(&control);
            }
        }
    }
}

// The layout tag is optional: an empty one means a hand-built control, which is parented here
// rather than loaded from a layout controls row.
void CSWGuiPanel::InitControl(CSWGuiControl& control, int bAddToTable, int bScale,
                              const CExoString& sControlName, int bUnused)
{
    K2_UNUSED(bUnused);
    if (sControlName != static_cast<const char*>(0))
        control.Load(this, m_pLayoutRes, m_layoutList, sControlName);
    else
        control.m_pContainer = this;

    control.m_bCenterOnNativeRect = m_bCenterOnNativeRect;

    CSWGuiExtent extent = control.m_extent;

    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset;
    int nYOffset;
    if (bScale)
    {
        if (g_pAppManager->m_pClientExoApp->GetSWGuiManager())
            g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
                m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
                static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
    }

    extent.x = static_cast<int>(extent.x * fXMult);
    extent.w = static_cast<int>(extent.w * fXMult);
    extent.y = static_cast<int>(extent.y * fYMult);
    extent.h = static_cast<int>(extent.h * fYMult);

    // pc-layout edge-snap, as in the 7-argument overload above.
    if (bScale && g_scaleMipc != 0 && g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
    {
        float fL = static_cast<float>(extent.x);
        float fT = static_cast<float>(extent.y);
        float fR = static_cast<float>(extent.x) + static_cast<float>(extent.w);
        float fB = static_cast<float>(extent.y) + static_cast<float>(extent.h);
        float fScreenW = static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth);
        float fScreenH = static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight);

        struct ScaleToEdge
        {
            float fScreenW;
            float fScreenH;
            float fScaleX;

            float ScaleWidth(float fPos, float fOther)
            {
                float fHalf = fScreenW * 0.5f;
                bool bEdge = (g_scaleMipc == 4);
                if (!bEdge && fOther >= fScreenH * 0.5f && g_scaleMipc == 1)
                {
                    float fFrac = fPos / fScreenW;
                    bEdge = (fFrac >= 0.4375f && fFrac <= 0.5625f);
                }
                if (bEdge)
                    return fHalf + (fPos - fHalf) * fScaleX;
                if (fHalf > fPos || g_scaleMipc == 2)
                    return fPos * fScaleX;
                return fScreenW - (fScreenW - fPos) * fScaleX;
            }

            float ScaleHeight(float fPos)
            {
                float fHalf = fScreenH * 0.5f;
                if (!(fHalf > fPos || g_scaleMipc == 2))
                    return fScreenH - (fScreenH - fPos) * 1.0f;
                return fPos * 1.0f;
            }
        };

        ScaleToEdge ste;
        ste.fScreenW = fScreenW;
        ste.fScreenH = fScreenH;
        ste.fScaleX = 1.3333334f / (fScreenW / fScreenH);

        float fNewL = ste.ScaleWidth(fL, fT);
        float fNewT = ste.ScaleHeight(fT);
        float fNewR = ste.ScaleWidth(fR, fNewT);
        float fNewB = ste.ScaleHeight(fB);

        extent.x = static_cast<int>(fNewL);
        extent.y = static_cast<int>(fNewT);
        extent.w = static_cast<int>(fNewR - fNewL);
        extent.h = static_cast<int>(fNewB - fNewT);
    }

#ifndef K2_UI_TOUCH
    K2ScaleListScrollBar(control, fXMult);
#endif
    control.SetExtent(extent);

    // The same id-addressed insert, plus the hand-built-control case.
    if (bAddToTable)
    {
        int nIndex = control.m_tail.nControlId;
        if (nIndex < 0)
        {
            if (sControlName == static_cast<const char*>(0))
            {
                m_controls.Add(&control);
                control.m_tail.nControlId = m_controls.m_count - 1;
            }
        }
        else if (nIndex >= m_controls.m_count)
        {
            while (nIndex > m_controls.m_count)
                m_controls.Add(0);
            m_controls.Add(&control);
        }
        else
        {
            CSWGuiControl* pOld = m_controls.m_pArray[nIndex];
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
            if (pOld == &control)   // as in the overload above
                pOld = 0;
#endif
            if (pOld)
            {
                m_controls.Add(pOld);
                m_controls.m_pArray[nIndex] = 0;
                K2_ASSUME(m_controls.m_pArray[m_controls.m_count - 1] != 0);   // pOld, which Add appended past nIndex
                m_controls.m_pArray[m_controls.m_count - 1]->m_tail.nControlId = m_controls.m_count - 1;
            }
            m_controls.m_pArray[nIndex] = &control;
        }
    }
}
#endif

void CSWGuiPanel::StopLoadFromLayout()
{
    if (m_stateFlags & 2)
    {
#if KOTOR1 && !defined(K2_UI_TOUCH)
        K1LayoutScaleSet(m_pLayoutRes, 1.0f);
#endif
        m_pLayoutRes->Release();
        delete m_pLayoutRes;
        m_pLayoutRes = 0;
        m_stateFlags &= ~2;
        ConnectControlMoves();
    }
}

CSWGuiPanel* CSWGuiPanel::AsSWGuiPanel()
{
    return this;
}

void CSWGuiPanel::Update(float f)
{
    K2_UNUSED(f);
}

#if !KOTOR1  // K2-only slot (K1 _ZTV11CSWGuiPanel has none)
bool CSWGuiPanel::ShouldFreeMouseOnAdd()
{
    return 1;
}
#endif

void CSWGuiPanel::SetBackground(const CResRef& resRef)
{
    m_pBackground->m_params.SetFillImage(resRef);
}

// The pad-button hooks differ only in the event id they forward.
void CSWGuiPanel::OnAButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_A, 1);
}

void CSWGuiPanel::OnBButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_B, 1);
}

void CSWGuiPanel::OnXButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_X, 1);
}

void CSWGuiPanel::OnYButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_Y, 1);
}

void CSWGuiPanel::OnBlackButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_BLACK, 1);
}

// The panel forwards the event to whichever control has focus.
void CSWGuiPanel::HandleInputEvent(int type, int val)
{
    if (m_pActiveControl)
        m_pActiveControl->HandleInputEvent(type, val);
}

void CSWGuiPanel::SetVisible(int flag)
{
    m_stateFlags = static_cast<unsigned short>((m_stateFlags & ~4) | ((flag & 1) << 2));
    if (m_pGuiManager)
        m_pGuiManager->DetermineNeedToDraw();
}

void CSWGuiPanel::ClearControls()
{
    m_controls.RemoveAll(1);
}

CSWGuiControl* CSWGuiPanel::GetActiveControl()
{
    return m_pActiveControl;
}

CSWGuiControl* CSWGuiPanel::GetControl(int nId)
{
    if (m_controls.m_count <= nId)
        return 0;
    return m_controls.m_pArray[nId];
}

// Resizes the background border to match, anchored at the origin.
void CSWGuiPanel::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;

    CSWGuiExtent bgExtent;
    bgExtent.w = extent.w;
    bgExtent.h = extent.h;
    CSWGuiBorder* pBackground = m_pBackground;
    bgExtent.x = 0;
    bgExtent.y = 0;
    pBackground->SetExtent(bgExtent);
}

// Centres the panel and resizes its background border to match.
#if KOTOR1
void CSWGuiPanel::CenterPanel()
{
    int x = (m_pGuiManager->m_nScreenWidth - m_extent.w) / 2;
    int y = (m_pGuiManager->m_nScreenHeight - m_extent.h) / 2;
    m_stateFlags |= 1;
    m_extent.x = x;
    m_extent.y = y;
}
#else
void CSWGuiPanel::CenterPanel()
{
    int x;
    int y;

    if (m_bCenterOnNativeRect)
    {
        x = (g_nScaledNativeWidth - m_extent.w) / 2 + g_nScreenLeftOffset;
        y = g_nScreenTopOffset + (g_nScaledNativeHeight - m_extent.h) / 2;
    }
    else
    {
        x = (m_pGuiManager->m_nScreenWidth - m_extent.w) / 2;
        y = (m_pGuiManager->m_nScreenHeight - m_extent.h) / 2;
    }

    CSWGuiBorder* pBackground = m_pBackground;

    m_extent.x = x;
    m_extent.y = y;

    CSWGuiExtent bgExtent;
    bgExtent.x = 0;
    bgExtent.y = 0;
    bgExtent.w = m_extent.w;
    bgExtent.h = m_extent.h;
    pBackground->SetExtent(bgExtent);

    m_stateFlags |= 1;
}
#endif

#ifndef K2_UI_TOUCH
// A full-screen backdrop's tint is white, or black when the data has no such texture, which would
// draw as plain white -- the port's "bg_replacement" (KOTOR II), "black" and "and_menu" (KOTOR 1)
// are not in the PC installs.
static bool BackdropExists(const CExoString& sName)
{
    CResRef resRef(sName);
    return g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0)) ||
           g_pExoResMan->Exists(resRef, 3007, static_cast<unsigned int*>(0));
}

static CSWGuiColor BackdropColor(const CExoString& sName)
{
    static CExoString s_sName;
    static bool s_bExists;
    if (s_sName != sName || s_sName == "") {
        s_sName = sName;
        s_bExists = BackdropExists(sName);
    }
    return s_bExists ? CSWGuiColor(1.0f, 1.0f, 1.0f) : CSWGuiColor(0.0f, 0.0f, 0.0f);
}
#endif

// Builds the chained full-screen border panel on demand and fills it with the named background.
#if !KOTOR1  // K2-only (K1 has no m_pNextInChain)
void CSWGuiPanel::AddExtraBorderPanel(CSWGuiManager* pGuiManager, CExoString name)
{
    K2_UNUSED(pGuiManager);
    CSWGuiPanel* pPanel = m_pNextInChain;
    if (!pPanel)
    {
        pPanel = new CSWGuiPanel(m_pGuiManager);
        m_pNextInChain = pPanel;
    }

    {
        CResRef resRef(name);
        pPanel->m_pBackground->m_params.SetFillImage(resRef, 0);
    }
#ifndef K2_UI_TOUCH
    pPanel->m_pBackground->m_params.m_color = BackdropColor(name);
#endif

    CSWGuiPanel* pChain = m_pNextInChain;

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = g_nScreenWidth;
    extent.h = g_nScreenHeight;
    pChain->SetExtent(extent);

    m_pNextInChain->m_pBackground->m_params.m_flags.nFillStyle = 2;
}
#endif

// The two halves are independent ifs: the int store that lands between them may alias
// m_stateFlags.
void CSWGuiPanel::GetExtentAccountingForPanelOffset(CSWGuiExtent* pExtent)
{
    *pExtent = m_extent;

#if KOTOR1
    // K1's centre bit is 3; its anchor bits sit one higher than K2's.
    if (m_stateFlags & 8)
    {
        pExtent->x += (m_pGuiManager->m_nScreenWidth - pExtent->w) / 2;
        pExtent->y += (m_pGuiManager->m_nScreenHeight - pExtent->h) / 2;
        return;
    }
    if (m_stateFlags & PANEL_STATE_CENTER_X)
        pExtent->x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
    const volatile unsigned short* pK1Flags = &m_stateFlags;
    if (*pK1Flags & PANEL_STATE_CENTER_Y)
        pExtent->y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
#else
    if (m_stateFlags & 8)
        return;

    if (m_stateFlags & PANEL_STATE_CENTER_X)
        pExtent->x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
    // The volatile view forces this second read of the field instead of reusing the loaded value.
    const volatile unsigned short* pFlags = &m_stateFlags;
    if (*pFlags & PANEL_STATE_CENTER_Y)
        pExtent->y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
#endif
}

// Hands focus off. The outgoing control gets the lose-focus event and the incoming one the gain.
void CSWGuiPanel::SetActiveControl(CSWGuiControl* pControl, int bPlaySound)
{
    CSWGuiControl* pOldControl = m_pActiveControl;
    if (pOldControl == pControl)
        return;

    if (pOldControl)
        pOldControl->HandleInputEvent(SWGUIEVENT_UNFOCUS, 1);

    m_pActiveControl = pControl;
    if (!pControl)
        return;

    pControl->HandleInputEvent(SWGUIEVENT_FOCUS, 1);

    if (!bPlaySound)
        return;

    if (m_pGuiManager)
        m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nActivateSound);
}

// Corrects the manager's mouse position against the panel's own origin and resolution.
void CSWGuiPanel::GetLocalMouseCoords(int* pX, int* pY)
{
    CSWGuiManager* pGuiMan = g_pGuiMan;
    *pX = pGuiMan->m_currentMousePos.x;
    *pY = pGuiMan->m_currentMousePos.y;

    int x = m_extent.x;
    int y = m_extent.y;

#if KOTOR1  // K1's centre bit is 3; its anchor bits sit one higher
    if (!(m_stateFlags & 1))
    {
        *pX -= (g_nScreenWidth - 640) / 2;
        *pY -= (g_nScreenHeight - 480) / 2;
    }
    else if (m_stateFlags & 8)
    {
        x += (m_pGuiManager->m_nScreenWidth - m_extent.w) / 2;
        y += (m_pGuiManager->m_nScreenHeight - m_extent.h) / 2;
    }
    else
    {
        if (m_stateFlags & PANEL_STATE_CENTER_X)
            x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
        if (m_stateFlags & PANEL_STATE_CENTER_Y)
            y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }
#else
    if (!(m_stateFlags & 1))
    {
        *pX -= (g_nScreenWidth - 640) / 2;
        *pY -= (g_nScreenHeight - 480) / 2;
    }
    else if (!(m_stateFlags & 8))
    {
        if (m_stateFlags & PANEL_STATE_CENTER_X)
            x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
        if (m_stateFlags & PANEL_STATE_CENTER_Y)
            y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }
#endif

    *pX -= x;
    *pY -= y;
}

// Removes the control from the panel's list and clears its container. The control is located
// through the shared control-list helper, which cannot be open-coded here.
void CSWGuiPanel::RemoveControl(CSWGuiControl* pControl)
{
    int nIndex = m_controls.IndexOf(pControl);

    if (nIndex == -1)
        return;

    pControl->m_pContainer = 0;
    m_controls.DelIndex(nIndex);
}

// Runs the second pass of layout loading: turns the navigation graph's numeric control IDs into
// pointers. The count is read once into a local and reused by the loop bound and the four range
// checks.
void CSWGuiPanel::ConnectControlMoves()
{
    int nCount = m_controls.m_count;

    for (int i = 0; i < nCount; i++)
    {
        CSWGuiControl* pControl = m_controls.m_pArray[i];
        if (!pControl)
            continue;

        CSWGuiNavigable* pNavigable = pControl->AsNavigable();
        if (!pNavigable)
            continue;

        for (int nDir = 0; nDir < 4; nDir++)
        {
            int nId = static_cast<int>(reinterpret_cast<intptr_t>(pNavigable->GetMoveToControl(nDir)));

            if (nId >= 0 && nId < nCount)
                pNavigable->SetMoveToControl(nDir, m_controls.m_pArray[nId]);
            else
                pNavigable->SetMoveToControl(nDir, 0);
        }
    }
}

void CSWGuiPanel::BringControlToFront(CSWGuiControl* pControl)
{
    int nIndex = m_controls.IndexOf(pControl);

    if (nIndex == -1)
        return;

    m_controls.DelIndex(nIndex);
    m_controls.Add(pControl);
}

void CSWGuiPanel::SendControlToBack(CSWGuiControl* pControl)
{
    int nIndex = m_controls.IndexOf(pControl);

    if (nIndex == -1)
        return;

    m_controls.DelIndex(nIndex);
    m_controls.Insert(pControl, 0);
}

// A control already at the front is left untouched.
void CSWGuiPanel::BringControlForward(CSWGuiControl* pControl)
{
    int nIndex = m_controls.IndexOf(pControl);

    if (nIndex == -1)
        return;

    if (m_controls.m_count - 1 <= nIndex)
        return;

    m_controls.DelIndex(nIndex);
    m_controls.Insert(pControl, nIndex + 1);
}

// The destination is one slot down, so a control already at the back is left untouched.
void CSWGuiPanel::SendControlBack(CSWGuiControl* pControl)
{
    int nIndex = m_controls.IndexOf(pControl);
    int nDest = nIndex - 1;

    if (nDest < 0)
        return;

    m_controls.DelIndex(nIndex);
    m_controls.Insert(pControl, nDest);
}

// Both the array base and the count are re-read every iteration, since the body calls out.
void CSWGuiPanel::ResetFont()
{
    if (!m_bAllowFontReset)
        return;

    for (int i = 0; i < m_controls.m_count; i++)
    {
        CSWGuiControl* pControl = m_controls.m_pArray[i];
        if (pControl)
            pControl->ReSetFont();
    }
}

// The three tooltip entry points share one gate: the suppression flag, then the client's tooltip
// option reached through the app manager.

// Hit-tests the mouse down the panel stack and hands the result to the two mouse-over fields.
// When a modal panel is up only its top entry is tested and the normal stack is never consulted.
void CSWGuiManager::UpdateMouseOverControl()
{
    int x = m_currentMousePos.x;
    int y = m_currentMousePos.y;

    CSWGuiControl* pControl = 0;
    if (m_modalPanels.m_count)
    {
        pControl = m_modalPanels.m_pArray[m_modalPanels.m_count - 1]->HitCheckMouse(x, y);
    }
    else
    {
        for (int i = m_panels.m_count - 1; i >= 0; i--)
        {
            pControl = m_panels.m_pArray[i]->HitCheckMouse(x, y);
            if (pControl)
                break;
        }
    }

    if (m_pMouseOverControl != pControl)
    {
        if (m_pMouseOverControl)
            m_pMouseOverControl->HandleMouseOver(0);
        m_pMouseOverControl = pControl;
    }

    if (m_pLastMouseOverControl != pControl)
        m_nMouseStateFlags &= ~1;
    m_pLastMouseOverControl = pControl;
}

void CSWGuiManager::ResetToolTipTimer()
{
    if (m_bToolTipDisabled)
        return;

    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bEnableToolTips)
        m_fToolTipTimer = 0;
}

// The flag clear is unconditional and comes before the option gate; only the timer reset is gated.
void CSWGuiManager::RemoveToolTip()
{
    m_nMouseStateFlags &= ~8;

    ResetToolTipTimer();

    if (m_pToolTipControl)
    {
        m_pToolTipControl->m_tail.bToolTipShown = 0;
        m_pToolTipControl = 0;
    }
}

// Mirrors RemoveToolTip: bit 3 of m_nMouseStateFlags is the tooltip-up flag, set here and cleared
// there.
void CSWGuiManager::DisplayToolTip(const CExoString& text)
{
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    // the console UI has no hover tooltips: the pad moves no pointer, and the pointer parked after a
    // click would otherwise hover whatever lies under it
    K2_UNUSED(text);
    return;
#endif
    if (m_bToolTipDisabled)
        return;

    if (!g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bEnableToolTips)
        return;

    m_nMouseStateFlags |= 8;
    m_pToolTipPanel->SetToolTipText(text);
    m_pToolTipPanel->PositionToolTip();
}

// Suppresses or re-enables tooltips around a transient interaction (a drag, a modal, a menu).
// Suppressing arms the timer and latches the control the tooltip would have been for; re-enabling
// re-arms the timer only when tooltips were not already suppressed and the client option is on.
void CSWGuiManager::SetTemporarilyDisableToolTip(int bDisable)
{
    CSWGuiControl* pPending = 0;

    if (bDisable)
    {
        m_fToolTipTimer = -1.0f;
        if (m_pMouseOverControl)
        {
            if (m_pMouseOverControl->AsListBox())
                pPending = m_pMouseOverControl->AsListBox()->HitCheckMouseLocal(0);
            else
                pPending = m_pMouseOverControl;
        }
    }
    else if (!m_bToolTipDisabled &&
             g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bEnableToolTips)
    {
        m_fToolTipTimer = 0.0f;
    }

    m_pPendingToolTipControl = pPending;
    m_bToolTipDisabled = bDisable;
}

void CSWGuiManager::StoreMouseForMouseTeleportingToButtons()
{
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bMouseTeleportToButtons)
        m_storedMousePos = m_currentMousePos;
}

// Every visible panel re-reads its font, then the tooltip panel, then the in-game HUD.
void CSWGuiManager::UpdateAllFonts()
{
    for (int i = 0; i < m_panels.m_count; i++)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (pPanel->m_bAllowFontReset)
            pPanel->ResetFont();
    }

    if (m_pToolTipPanel)
        m_pToolTipPanel->ResetFont();

    if (g_pAppManager->m_pClientExoApp->GetInGameGui())
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ResetInterfaceFont();
}

// The engine's own mouse origin is bottom-left while the GUI works top-left, which is what the
// `g_nScreenHeight - y - 1` flip below is doing.

void CSWGuiManager::MoveMouseToPosition(int x, int y)
{
    m_currentMousePos.x = x;
    m_currentMousePos.y = y;

    g_pExoInput->SetMousePos(x, g_nScreenHeight - y - 1, false);
    HandleMouseMove(x, y);
}

// The stored position doubles as its own nothing-stored sentinel: the guard is a sign test on the
// x half, and the reset writes -1 to both halves.
void CSWGuiManager::RestoreOldMousePosition()
{
    if (m_storedMousePos.x < 0)
        return;

    // Field by field rather than a struct assignment; the calls below read x and y separately.
    m_currentMousePos.x = m_storedMousePos.x;
    m_currentMousePos.y = m_storedMousePos.y;

    g_pExoInput->SetMousePos(m_currentMousePos.x, g_nScreenHeight - m_currentMousePos.y - 1, false);
    g_pAppManager->m_pClientExoApp->m_pInternal->RestoreMousePositionTo(m_currentMousePos.x,
                                                                       m_currentMousePos.y);

    m_storedMousePos.x = -1;
    m_storedMousePos.y = -1;

    UpdateMouseMove();
}

// The option gate wrapped around RestoreOldMousePosition.
void CSWGuiManager::RestoreMouseForMouseTeleportingToButtons()
{
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bMouseTeleportToButtons)
        RestoreOldMousePosition();
}

void CSWGuiManager::ReloadToolTipPanel()
{
    delete m_pToolTipPanel;
    m_pToolTipPanel = new CSWGuiToolTipPanel(this);
}

// A threshold ladder naming the five resolutions the build ships. The compared value is the
// height, which is what makes this a static member rather than a method with an implicit `this`.
CExoString CSWGuiManager::GetScreenResolutionString(unsigned int nWidth, unsigned int nHeight)
{
    K2_UNUSED(nWidth);
    if (nHeight >= 1200)
        return CExoString("1600x1200");
    if (nHeight >= 1024)
        return CExoString("1280x1024");
    if (nHeight >= 960)
        return CExoString("1280x960");
    if (nHeight >= 768)
        return CExoString("1024x768");
    return CExoString("800x600");
}

// Removes the top of the modal stack and hands focus back to the panel now beneath it.
CSWGuiPanel* CSWGuiManager::PopModalPanel()
{
    if (m_modalPanels.m_count == 0)
        return 0;

    CSWGuiPanel* pPanel = m_modalPanels.m_pArray[m_modalPanels.m_count - 1];
    // inlined CExoArrayList<CSWGuiPanel*>::DelIndex(m_count - 1)
    m_modalPanels.m_count--;

    DetermineNeedToDraw();

    if (m_modalPanels.m_count - 1 >= 0 &&
        m_modalPanels.m_pArray[m_modalPanels.m_count - 1]->GetActiveControl())
    {
        int nTop = m_modalPanels.m_count;
        m_modalPanels.m_pArray[nTop - 1]->GetActiveControl()->HandleFocusChange(1);
    }

    return pPanel;
}

void CSWGuiManager::CaptureMouse(CSWGuiObject* pObject, int nFlags)
{
    if (m_pMouseCaptureObject)
        m_pMouseCaptureObject->HandleLoseMouseFocus();
    m_pMouseCaptureObject = pObject;
    m_nCaptureFlags = nFlags;
}

void CSWGuiManager::ReleaseMouse(CSWGuiObject* pObject)
{
    if (pObject && m_pMouseCaptureObject == pObject)
    {
        m_pMouseCaptureObject = 0;
        pObject->HandleLoseMouseFocus();
    }
    m_nCaptureFlags = 0;
}

// Returns whether the click was consumed.
int CSWGuiManager::HandleLMouseUp()
{
    int nHandled = 0;
    if (m_pMouseCaptureObject)
    {
        if (m_nCaptureFlags == 1)
        {
            m_pMouseCaptureObject->HandleLMouseUp();
            nHandled = 1;
        }
    }
    m_nMouseStateFlags &= ~1;
    return nHandled;
}

int CSWGuiManager::HandleRMouseUp()
{
    if (m_pMouseCaptureObject)
    {
        if (m_nCaptureFlags == 4)
        {
            m_pMouseCaptureObject->HandleRMouseUp();
            return 1;
        }
        return 0;
    }
    return 0;
}

// The click path. Records the mouse-down mode, drops any tooltip, then hit-tests and hands the
// click to whatever control is under the cursor. Returns whether the click was consumed.
int CSWGuiManager::HandleLMouseDown(int aMode)
{
    unsigned char bOldFlags = m_nMouseStateFlags;
    m_nMouseStateFlags = static_cast<unsigned char>((bOldFlags & ~1) | (aMode & 1));
    if (bOldFlags & 8)
        RemoveToolTip();

    if (m_pMouseCaptureObject)
    {
        if (m_nCaptureFlags == 1)
            return 0;
    }

    // While the input class is 3 every panel gets the "cancel/back out" event first.
    if (g_pAppManager->m_pClientExoApp->GetInputClass() == 3)
    {
        for (int i = m_panels.m_count; i > 0; i--)
            m_panels.m_pArray[i - 1]->HandleInputEvent(SWGUIEVENT_DOUBLE_CLICK, 1);
    }

    UpdateMouseOverControl();
    SetTemporarilyDisableToolTip(1);

    CSWGuiControl* pControl = m_pMouseOverControl;
    if (pControl == 0)
        return 0;

    pControl->HandleLMouseDown();
    return 1;
}

// The mouse-wheel handler. Same opening as the two click handlers, then a hit test of the cursor
// and the wheel routed to whatever is under it: a list box steps one row per 120 units of delta,
// any other button-like control gets a single press/release, and a control that is not selectable
// walks up through its selectable parent first.
int CSWGuiManager::HandleZMouse(int zDelta)
{
    if (m_nMouseStateFlags & 8)
        RemoveToolTip();

    CSWGuiPanel* pPanel;
    CSWGuiControl* pControl;
    HitCheckMouse(m_currentMousePos.x, m_currentMousePos.y, &pPanel, &pControl, 0);

    if (pControl != m_pMouseOverControl)
        m_pMouseOverControl = pControl;

    if (!pControl)
        return 0;

    if (pControl->AsListBox())
    {
        if (zDelta > 0)
        {
            while (zDelta > 0)
            {
                pControl->AsListBox()->HandleInputEvent(SWGUIEVENT_WHEEL_UP, 1);
                zDelta -= 120;
            }
        }
        else if (zDelta < 0)
        {
            while (zDelta < 0)
            {
                pControl->AsListBox()->HandleInputEvent(SWGUIEVENT_WHEEL_DOWN, 1);
                zDelta += 120;
            }
        }
        else
        {
            return 1;
        }

        if (m_bToolTipDisabled == 1)
        {
            ResetToolTipTimer();
            m_pPendingToolTipControl = 0;
            m_bToolTipDisabled = 0;
        }

        pControl->AsListBox()->HandleMouseMove();
        return 1;
    }

    if (!pControl->GetIsSelectable())
    {
        pControl = pControl->GetSelectableParent();
        if (!pControl)
            return 1;
    }

    if (pControl->AsButton() && zDelta != 0)
        pControl->HandleInputEvent(zDelta > 0 ? 500 : 501, 1);

    return 1;
}

// Handles a right-button click as the structural twin of HandleLMouseDown, with the same tooltip
// teardown, mouse-capture bail-out and two helpers, and the dispatch on the control's right-button
// handler.
int CSWGuiManager::HandleRMouseDown()
{
    if (m_nMouseStateFlags & 8)
        RemoveToolTip();

    if (m_pMouseCaptureObject)
        return 0;

    UpdateMouseOverControl();
    SetTemporarilyDisableToolTip(1);

    if (m_pMouseOverControl)
    {
        m_pMouseOverControl->HandleRMouseDown();
        return 1;
    }

    return 0;
}

// The per-frame mouse-move path, the largest of the four mouse entry points. It hit-tests the
// cursor, updates the mouse-over pair, hands focus to the control under the cursor, un-hovers the
// outgoing one (walking its ancestors to decide how), and finally routes the move to a list box
// when there is one. The hit-test results are two locals, both live: the control drives the whole
// mouse-over half and the panel is only asked for its active control.
//
// The mouse-over comparison is written with the local on the left. The outgoing control is captured
// in its own local before the field is overwritten, and the accumulator that decides how it is
// un-hovered is seeded from the new control's enabled bit and ORed with every selectable
// ancestor's.
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
// Whether the move changed the cursor position (the list box's hover selection reads it); a
// replay at the parked cursor leaves it false.
bool g_bK2HoverMoved = false;
#endif
void CSWGuiManager::HandleMouseMove(int x, int y)
{
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    const bool bMoved = m_currentMousePos.x != x || m_currentMousePos.y != y;
    g_bK2HoverMoved = bMoved;
#endif
    m_currentMousePos.x = x;
    m_currentMousePos.y = y;

    if (m_pMouseCursorModel)
    {
        Vector vMousePos;
        vMousePos.x = static_cast<float>(x) * 0.01f;
        vMousePos.y = static_cast<float>(m_nScreenHeight - y) * 0.01f;
        vMousePos.z = 0.0f;
        m_pMouseCursorModel->SetPosition(vMousePos);
    }

    // The captured object eats the move while a drag is in flight; everything below is skipped.
    if (m_pMouseCaptureObject && m_pMouseCaptureObject->HandleMouseCapturedMovement(x, y))
        return;

    CSWGuiControl* pControl;
    CSWGuiPanel* pPanel;
    HitCheckMouse(x, y, &pPanel, &pControl, 0);

    if (pControl != m_pMouseOverControl)
    {
        if (m_nMouseStateFlags & 8)
            RemoveToolTip();
        ResetToolTipTimer();
        m_pPendingToolTipControl = 0;
        m_bToolTipDisabled = 0;
    }

    if (!pControl)
    {
        if (m_pMouseOverControl)
        {
            if (m_pMouseOverControl->m_tail.bActive && !m_pMouseOverControl->m_tail.bFocusOnHover)
            {
                m_pMouseOverControl->HandleMouseOver(0);
                CSWGuiControl* pSel = m_pMouseOverControl->GetSelectableParent();
                if (pSel)
                    pSel->HandleMouseOver(0);
            }
            else
            {
                CSWGuiControl* pSel = m_pMouseOverControl->GetSelectableParent();
                if (pSel && pSel->m_tail.bActive && !m_pMouseOverControl->m_tail.bFocusOnHover)
                    pSel->HandleMouseOver(0);
            }

            m_pMouseOverControl = 0;
        }

        ResetToolTipTimer();
        m_pPendingToolTipControl = 0;
        m_bToolTipDisabled = 0;
        return;
    }

    // Re-enable tail of SetTemporarilyDisableToolTip. A pending tooltip armed for a control that is
    // neither a listbox nor the control now under the cursor is dropped.
    if (m_bToolTipDisabled == 1 &&
        !pControl->AsListBox() &&
        pControl != m_pPendingToolTipControl)
    {
        ResetToolTipTimer();
        m_pPendingToolTipControl = 0;
        m_bToolTipDisabled = 0;
    }

    CSWGuiControl* pOld = m_pMouseOverControl;
    m_pMouseOverControl = pControl;

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    // ProcessInput replays the move at the parked cursor after every input event, so only a real
    // move takes the focus; otherwise each D-pad step (or a panel's starting focus) is undone.
    if (bMoved && !pControl->AsRadioButton() && !m_pMouseOverControl->AsEditBox())
#else
    if (!pControl->AsRadioButton() && !m_pMouseOverControl->AsEditBox())
#endif
    {
        if (m_pMouseOverControl != pPanel->GetActiveControl())
        {
            if (m_pMouseOverControl->GetIsSelectable() &&
                m_pMouseOverControl->m_tail.bFocusOnHover && m_pMouseOverControl->m_tail.bEnabled)
            {
                if (pPanel->GetActiveControl())
                    pPanel->GetActiveControl()->HandleFocusChange(0);
                m_pMouseOverControl->HandleFocusChange(1);
            }
            else
            {
                CSWGuiControl* pSel = m_pMouseOverControl->GetSelectableParent();
                if (pSel && !pSel->AsRadioButton() && !pSel->AsEditBox())
                {
                    if (pSel->m_tail.bFocusOnHover)
                    {
                        if (pSel->m_tail.bEnabled && pPanel->GetActiveControl() != pSel)
                            pSel->HandleFocusChange(1);
                    }
                    else if (pSel->m_tail.bEnabled)
                    {
                        pSel->HandleMouseOver(1);
                    }
                }
            }
        }
    }

    if (pOld != m_pMouseOverControl)
    {
        unsigned int bSetMouseOver = 0;

        if (m_pMouseOverControl->GetIsSelectable())
        {
            m_pMouseOverControl->HandleMouseOver(1);
            if (m_pMouseOverControl->m_tail.bFocusOnHover)
                bSetMouseOver = m_pMouseOverControl->m_tail.bEnabled;
        }

        // A for loop, not a while: the continue must still run the advance.
        for (CSWGuiControl* pParent = m_pMouseOverControl->m_pParent; pParent;
             pParent = pParent->m_pParent)
        {
            if (pParent != pOld)
            {
                if (pParent->GetIsSelectable())
                {
                    pParent->HandleMouseOver(1);
                    if (!pParent->m_tail.bFocusOnHover)
                        continue;
                    bSetMouseOver = pParent->m_tail.bEnabled ? 1 : bSetMouseOver;
                }
            }
        }

        if (bSetMouseOver)
        {
            if (pOld)
            {
                if (!pOld->m_pParent && !pOld->GetIsChild(m_pMouseOverControl))
                    pOld->HandleMouseOver(0);
                if (pOld->GetSelectableParent())
                    pOld->HandleMouseOver(0);
            }
        }
        else if (pOld)
        {
            if (pOld->m_tail.bActive && !pOld->m_tail.bFocusOnHover)
            {
                pOld->HandleMouseOver(0);

                CSWGuiControl* pSel = pOld->GetSelectableParent();
                if (pSel && pSel != m_pMouseOverControl &&
                    pSel != m_pMouseOverControl->GetSelectableParent())
                    pSel->HandleMouseOver(0);
            }
            else
            {
                CSWGuiControl* pSel = pOld->GetSelectableParent();
                if (pSel && pSel->m_tail.bActive && !pSel->m_tail.bFocusOnHover &&
                    pSel != m_pMouseOverControl &&
                    pSel != m_pMouseOverControl->GetSelectableParent())
                    pSel->HandleMouseOver(0);
            }
        }
    }

    CSWGuiListBox* pListBox = pControl->AsListBox();
    if (pListBox)
    {
        if (m_bToolTipDisabled == 1 &&
            pListBox->HitCheckMouseLocal(0) != m_pPendingToolTipControl)
        {
            ResetToolTipTimer();
            m_pPendingToolTipControl = 0;
            m_bToolTipDisabled = 0;
        }

        pListBox->HandleMouseMove();
    }
}

// Draws the full-screen background label, building it on first use.
#if KOTOR1
#ifndef K2_UI_TOUCH
extern SDL_Window* Android_Window;
#endif
#if !defined(K2_UI_TOUCH)
int K1ScaleLoadedPadding(int nPadding, CResGFF* pGFF)
{
    float f = 1.0f;
    for (unsigned int i = 0; i < 16; i++)
        if (s_aK1LayoutScale[i].pGFF == pGFF && pGFF != 0)
            f = s_aK1LayoutScale[i].fScale;
    return static_cast<int>(static_cast<float>(nPadding) * f * static_cast<float>(g_SDL_mode.h) / 768.0f + 0.5f);
}
#endif
extern "C" void android_port_glEnable(unsigned int cap);
extern "C" void android_port_glDisable(unsigned int cap);
extern "C" void android_port_glClearColor(float r, float g, float b, float a);
extern "C" void android_port_glScissor(int x, int y, int w, int h);
extern "C" void android_port_glClear(unsigned int mask);
int IosGetTemporaryDisableInteraction();
#endif

void CSWGuiManager::DrawPCBG(float fAlpha)
{
#if KOTOR1
    // When the background image changes the label is re-laid out for the window, or for a pazaak
    // background to the GUI size.
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, m_nScreenWidth, m_nScreenHeight, Vector(0.0f, 0.0f, 0.0f), 0, 1.0f))
    {
        if (m_pBackgroundLabel == 0)
        {
            CSWGuiLabel* pLabel = new CSWGuiLabel();
            m_pBackgroundLabel = pLabel;
            CSWGuiExtent extent;
            extent.w = m_nScreenWidth;
            extent.h = m_nScreenHeight;
            CSWGuiTextParams textParams;
            Vector color(1.0f, 1.0f, 1.0f);
            CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(m_fullScreenBG),
                                            0, 0, 1.0f, color, 0);
            pLabel->Initialize(extent, textParams, borderParams, 1.0f);
        }
        static CExoString s_sLastBG;
        if (s_sLastBG != m_fullScreenBG)
        {
            s_sLastBG = m_fullScreenBG;
            if (strstr(m_fullScreenBG.CStr(), "pazaak"))
            {
                int nScreenWidth, nScreenHeight;
                if (IosIsIphone(nScreenWidth, nScreenHeight) && nScreenWidth > 1024)
                {
                    CSWGuiLabel* pLabel = m_pBackgroundLabel;
                    CSWGuiExtent extent;
                    extent.w = m_nScreenWidth;
                    extent.h = m_nScreenHeight;
                    CSWGuiTextParams textParams;
                    Vector color(1.0f, 1.0f, 1.0f);
                    CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(m_fullScreenBG),
                                                    0, 0, 1.0f, color, 0);
                    pLabel->Initialize(extent, textParams, borderParams, 1.0f);
                }
            }
            else
            {
                CSWGuiLabel* pLabel = m_pBackgroundLabel;
                CSWGuiExtent extent;
                extent.w = g_SDL_mode.w;
                extent.h = g_SDL_mode.h;
                CSWGuiTextParams textParams;
                Vector color(1.0f, 1.0f, 1.0f);
                CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(m_fullScreenBG),
                                                0, 0, 1.0f, color, 0);
                pLabel->Initialize(extent, textParams, borderParams, 1.0f);
            }
        }
        m_pBackgroundLabel->m_border.m_params.SetFillImage(m_fullScreenBG, 0);
        m_pBackgroundLabel->m_border.m_params.m_flags.nFillStyle = 2;
#ifndef K2_UI_TOUCH
        m_pBackgroundLabel->m_border.m_params.m_color = BackdropColor(m_fullScreenBG);
#endif
        m_pBackgroundLabel->Draw(fAlpha);
#ifndef K2_UI_TOUCH
        static const bool s_bSidePanels = BackdropExists("and_menu");
        if (s_bSidePanels)
#endif
        {
            m_pFrameImage->Draw(fAlpha);
            m_pMirroredFrameImage->Draw(fAlpha);
        }
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
#else
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, m_nScreenWidth, m_nScreenHeight, Vector(0.0f, 0.0f, 0.0f), 0, 1.0f))
    {
        if (m_pBackgroundLabel == 0)
        {
            CSWGuiLabel* pLabel = new CSWGuiLabel();
            m_pBackgroundLabel = pLabel;
            CSWGuiExtent extent;
            extent.w = m_nScreenWidth;
            extent.h = m_nScreenHeight;
            CSWGuiTextParams textParams;
            Vector color(1.0f, 1.0f, 1.0f);
            CSWGuiBorderParams borderParams(CResRef(), CResRef(), CResRef(m_fullScreenBG),
                                            0, 0, 0, 1.0f, color, 0, 0);
            pLabel->Initialize(extent, textParams, borderParams, 1.0f);
        }
        m_pBackgroundLabel->m_border.m_params.SetFillImage(m_fullScreenBG, 0);
        m_pBackgroundLabel->m_border.m_params.m_flags.nFillStyle = 2;
#ifndef K2_UI_TOUCH
        m_pBackgroundLabel->m_border.m_params.m_color = BackdropColor(m_fullScreenBG);
#endif
        m_pBackgroundLabel->Draw(fAlpha);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
#endif
}

// Creates the software-mouse scene and its cursor model, places the camera at the centre of the
// screen and shows the cursor at the current mouse position.
int CSWGuiManager::ActivateSoftwareMouse()
{
    CSWGuiScene* pScene = new CSWGuiScene();
    m_pSoftwareMouse = pScene;

    {
        CSWGuiExtent extent;
        extent.w = m_nScreenWidth;
        extent.h = m_nScreenHeight;
        pScene->SetExtent(extent);
    }

    m_pMouseCursorModel = m_pSoftwareMouse->AddModel(CExoString("gui_mouse"), -1);
    if (m_pMouseCursorModel == 0)
        return 0;

    m_pMouseCursorModel->ReplaceTexture(const_cast<char*>(""), m_mouseCursorRes.GetResRefStr());

    // Centre the camera in scene units (0.01f per pixel), with the eye distance scaled to the
    // design width.
    Vector vCameraPos;
    vCameraPos.x = m_nScreenWidth * 0.5f * 0.01f;
    vCameraPos.y = m_nScreenHeight * 0.5f * 0.01f;
    vCameraPos.z = 5.8f * m_nScreenWidth / 640.0f;
    m_pSoftwareMouse->m_pCamera->SetPosition(vCameraPos);

    // Screen-space (y down) mouse position in scene units, with y flipped and z flat.
    Vector vMousePos;
    vMousePos.x = static_cast<float>(m_currentMousePos.x) * 0.01f;
    vMousePos.y = static_cast<float>(m_nScreenHeight - m_currentMousePos.y) * 0.01f;
    vMousePos.z = 0.0f;
    m_pMouseCursorModel->SetPosition(vMousePos);

    if (m_nMouseStateFlags & 4)
    {
        m_pMouseCursorModel->PlayAnimation(const_cast<char*>("center"), 1.0f, 0, 0.0f);
        m_pMouseCursorModel->AnimateExternal(1.0f);
        m_pMouseCursorModel->PlayAnimation(const_cast<char*>(""), 0.0f, AnimRun::kAnimRemove, 0.0f);
    }

    m_pMouseCursorModel->SetIllumination(Vector(1.0f, 1.0f, 1.0f), 1);
    return 1;
}

void CSWGuiManager::SetSoftwareMouseCursor(int nCursor, int nX, int bRound)
{
    m_nSoftwareMouseCursor = nCursor;
    if (bRound)
        nCursor += nCursor % 2;   // round up to even (negative stays negative: this is n + n%2)
    m_mouseCursorRes = g_asMouseCursorNames[nCursor];
    if (m_pMouseCursorModel != 0)
    {
        m_pMouseCursorModel->ReplaceTexture(const_cast<char*>(""), m_mouseCursorRes.GetResRefStr());
        if (((m_nMouseStateFlags >> 2) & 1) != nX)
        {
            m_pMouseCursorModel->PlayAnimation(const_cast<char*>(nX != 0 ? "center" : "default"), 1.0f, 0, 0.0f);
            m_pMouseCursorModel->AnimateExternal(1.0f);
            m_pMouseCursorModel->PlayAnimation(const_cast<char*>(""), 0.0f, AnimRun::kAnimRemove, 0.0f);
        }
    }
    m_nMouseStateFlags = static_cast<unsigned char>((m_nMouseStateFlags & ~4) | ((nX & 1) << 2));
}

void CSWGuiManager::ShowSoftwareMouse()
{
    if (m_pMouseCursorModel == 0)
        ActivateSoftwareMouse();
}

void CSWGuiManager::HideSoftwareMouse()
{
    if (m_pSoftwareMouse)
    {
        delete m_pSoftwareMouse;
        m_pSoftwareMouse = 0;
    }
    if (m_pMouseCursorModel)
        m_pMouseCursorModel = 0;
}

void CSWGuiManager::DeactivateSoftwareMouse()
{
    if (m_pSoftwareMouse)
    {
        delete m_pSoftwareMouse;
        m_pSoftwareMouse = 0;
    }
    if (m_pMouseCursorModel)
        m_pMouseCursorModel = 0;
}

// Publishes this as the global GUI manager and builds the tooltip panel and the background image.
CSWGuiManager::CSWGuiManager()
{
    m_nRepeatFilterTime = 0;
    m_nScreenWidth = 0;
    m_nScreenHeight = 0;
    m_bSuppressCameraRender = 0;
    m_nCapturedPanelCount = 0;
    m_nRepeatFilterEvent = 0;

    m_pBackgroundLabel = 0;
    m_pSafeRegionPanel = 0;
    m_pSafeRegionLabel = 0;

#if KOTOR1  // K1's backdrop is plain black
    m_fullScreenBG = "black";
#else
    m_fullScreenBG = "800x600";
    m_fullScreenBG = m_fullScreenBG + FULLSCREEN_BGS[0];
#endif

    m_pMouseOverControl = 0;
    m_nCaptureFlags = 0;
    m_pMouseCaptureObject = 0;
    m_pLastMouseOverControl = 0;
    m_pFocusedEditbox = 0;

    m_nMouseStateFlags &= ~3;

    m_pSoftwareMouse = 0;
    m_pMouseCursorModel = 0;

    m_mouseCursorRes = const_cast<char*>("");

    m_nMouseStateFlags &= ~4;
    m_nSoftwareMouseCursor = 1;

    m_pToolTipPanel = new CSWGuiToolTipPanel(this);

    m_fToolTipTimer = 0.0f;
    m_pToolTipControl = 0;
    m_bToolTipDisabled = 0;

    m_paGuiSounds = 0;
    m_nGuiSounds = 0;

    m_pPendingToolTipControl = 0;
    m_pDragDropControl = 0;

    m_nMouseStateFlags &= ~8;
    g_pGuiMan = this;
    m_nCurrentInputEvent = 0;

    if (g_pAppManager->m_pClientExoApp->GetClientOptions())
    {
        m_fToolTipTimeDelay =
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_fToolTipTimeDelay;
    }

    m_storedMousePos.x = -1;
    m_storedMousePos.y = -1;

#if KOTOR1
    // K1: the two "and_menu" side panels flank the 4:3 GUI rectangle (1024 x 768 scaled to the
    // window height), the right one mirrored.
    m_nReserved1 &= ~1;
    int nGuiWidth = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 768.0f * 1024.0f);
    int nGuiLeft = (g_SDL_mode.w - nGuiWidth) / 2;
    CSWGuiExtent extent;
    extent.x = nGuiLeft;
    extent.y = 0;
    extent.w = nGuiWidth;
    extent.h = g_SDL_mode.h;
    m_pFrameImage = new CSWGuiImage();
    m_pMirroredFrameImage = new CSWGuiImage();
    extent.w = static_cast<int>(ceilf(static_cast<float>(g_SDL_mode.h) * 204.0f / 512.0f));
    extent.h = g_SDL_mode.h;
    extent.x = nGuiLeft - extent.w;
    m_pFrameImage->Initialize(extent, CSWGuiImageParams(CResRef("and_menu"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER, 1.0f,
                                                      Vector(1.0f, 1.0f, 1.0f)));
    extent.x = g_SDL_mode.w - extent.x - extent.w;
    m_pMirroredFrameImage->Initialize(extent, CSWGuiImageParams(CResRef("and_menu"), 0, 8, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER, 1.0f,
                                                      Vector(1.0f, 1.0f, 1.0f)));
#else
#if defined(K2_UI_PC) || (defined(K2_HOST_BUILD) && !defined(K2P_MOBILE_HOST))
    // pc, and any UI on a desktop: the HUD at the screen edges, as the PC build (the inset is the
    // phone's; at 4:3 it pushes the action bar and the portraits onto the combat queue)
    m_nUIEdgeInset = 0;
#else
    m_nUIEdgeInset = g_nUIEdgeOffset;
#endif

    m_pFrameImage = new CSWGuiImage();

    // Scoped so the temporary can share the space of the CExoString above.
    {
        m_pFrameImage->SetExtent(CSWGuiExtent(0, 0, 800, 600));
    }

    m_pFrameImage->m_params.SetImage(CResRef("swoop"));
#endif
}

// Deletes the owned objects, then the software mouse, the tooltip panel and the GUI sounds.
CSWGuiManager::~CSWGuiManager()
{
#if KOTOR1  // K1's two side panels go first
    if (m_pFrameImage)
    {
        delete m_pFrameImage;
        m_pFrameImage = 0;
    }
    if (m_pMirroredFrameImage)
    {
        delete m_pMirroredFrameImage;
        m_pMirroredFrameImage = 0;
    }
#endif
    if (m_pSafeRegionLabel)
    {
        delete m_pSafeRegionLabel;
        m_pSafeRegionLabel = 0;
    }
    if (m_pSafeRegionPanel)
    {
        delete m_pSafeRegionPanel;
        m_pSafeRegionPanel = 0;
    }
    if (m_pBackgroundLabel)
    {
        delete m_pBackgroundLabel;
        m_pBackgroundLabel = 0;
    }
    DeactivateSoftwareMouse();
    if (m_pToolTipPanel)
    {
        delete m_pToolTipPanel;
        m_pToolTipPanel = 0;
    }
    UnloadGuiSounds();
#if !KOTOR1
    if (m_pFrameImage)
    {
        delete m_pFrameImage;
        m_pFrameImage = 0;
    }
#endif
    g_pGuiMan = 0;
}

// Recomputes bit 6 of every visible panel's m_stateFlags, the must-draw-this-frame bit. Both
// stacks are walked top-down and the first panel that is both visible and opaque latches the
// frame: it keeps its bit, its background name becomes the manager's cached backdrop, and every
// panel below it has the bit cleared because it is completely covered. Panels that are also on
// the modal stack are skipped by the second walk, since the first one already decided them.
//
// The flag word is read once into a local before the walk.
void CSWGuiManager::DetermineNeedToDraw()
{
    int bCovered = 0;
    for (int i = m_modalPanels.m_count - 1; i >= 0; i--)
    {
        CSWGuiPanel* pPanel = m_modalPanels.m_pArray[i];
        if (pPanel)
        {
            unsigned short nFlags = pPanel->m_stateFlags;
            if (nFlags & 4)
            {
                if (bCovered)
                {
#if KOTOR1  // K1: the draw bit is 7
                    pPanel->m_stateFlags = nFlags & ~PANEL_STATE_DRAW;
#else
                    pPanel->m_stateFlags = nFlags & ~PANEL_STATE_DRAW;
#endif
                }
                else
                {
#if KOTOR1  // K1: the draw bit is 7
                    pPanel->m_stateFlags = nFlags | PANEL_STATE_DRAW;
#else
                    pPanel->m_stateFlags = nFlags | PANEL_STATE_DRAW;
#endif
                    if (nFlags & 8)
                    {
                        m_fullScreenBG = pPanel->GetFullScreenBG();
                        bCovered = 1;
                    }
                    else
                    {
                        bCovered = 0;
                    }
                }
            }
        }
    }
    // The removal walk is a pre-decrement while, not a descending for.
    int i = m_panels.m_count;
    while (--i >= 0)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (pPanel)
        {
            unsigned short nFlags = pPanel->m_stateFlags;
            if ((nFlags & 4) && GetPosInModalStack(pPanel) == -1)
            {
                if (bCovered)
                {
#if KOTOR1  // K1: the draw bit is 7
                    pPanel->m_stateFlags = nFlags & ~PANEL_STATE_DRAW;
#else
                    pPanel->m_stateFlags = nFlags & ~PANEL_STATE_DRAW;
#endif
                }
                else
                {
#if KOTOR1  // K1: the draw bit is 7
                    pPanel->m_stateFlags = nFlags | PANEL_STATE_DRAW;
#else
                    pPanel->m_stateFlags = nFlags | PANEL_STATE_DRAW;
#endif
                    if (nFlags & 8)
                    {
                        m_fullScreenBG = pPanel->GetFullScreenBG();
                        bCovered = 1;
                    }
                    else
                    {
                        bCovered = 0;
                    }
                }
            }
        }
    }
}

#ifndef K2_UI_TOUCH
// pc and xbox UIs: the software cursor as a GUI image at the pointer, nSize pixels square, in place of the
// gui_mouse scene (which draws it opaque -- the scene pass turns blending off -- and scaled toward
// the centre off 4:3). Hidden while the right button turns the camera.
static void DrawPcCursor(CSWGuiManager* pMan, int nSize)
{
    extern int g_bMouseCameraRotateToggle;
    if (g_bMouseCameraRotateToggle)
        return;
    static CAurGUIImageInternal* s_pCursor;
    static CResRef s_cursorRes;
    if (s_pCursor == 0 || !(s_cursorRes == pMan->m_mouseCursorRes))
    {
        s_cursorRes = pMan->m_mouseCursorRes;
        if (s_pCursor == 0)
            s_pCursor = NewCAurGUIImage(pMan->m_mouseCursorRes.GetResRefStr());
        else
            s_pCursor->SetTexture(pMan->m_mouseCursorRes.GetResRefStr());
    }
    int nX = pMan->m_currentMousePos.x;
    int nY = pMan->m_currentMousePos.y;
    if (pMan->m_nMouseStateFlags & 4)   // the "center" cursors (targeting) sit on the hotspot
    {
        nX -= nSize / 2;
        nY -= nSize / 2;
    }
    AurGUIStartLayer();
    s_pCursor->Draw(nX, nY, nSize, nSize, 0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
    AurGUIStopLayer();
}
#endif

// Runs the per-frame draw pass: the two-stack panel walk, the minigame overlay, the tooltip ticker,
// the software-mouse scene, and the pending-destroy sweep at the end.
void CSWGuiManager::Draw(float fDelta)
{
#if KOTOR1
    // The PC backdrop comes first while camera rendering is suppressed; the minigame layer is the
    // pause button plus the two 4:3 pillarbox bars.
    int nCount = m_panels.m_count;

    AurMakeCurrent();
    CSWGuiListBox::UpdateArrowPulsing(fDelta);
    CSWGuiDrawable::UpdatePulsing(fDelta);

    if (m_bSuppressCameraRender)
        DrawPCBG(fDelta);

    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (!pPanel)
        {
            m_panels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        if (GetPosInModalStack(pPanel) == -1)
            pPanel->Draw(fDelta);
    }

    nCount = m_modalPanels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_modalPanels.m_pArray[i];
        if (!pPanel)
        {
            m_modalPanels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        pPanel->Draw(fDelta);
    }

    if (m_pDragDropControl)
    {
        AurGUIStartLayer();
        if (AurGUISetupViewport(0, 0, m_nScreenWidth, m_nScreenHeight,
                                Vector(-1.0f, -1.0f, -1.0f), 0, 1.0f))
        {
            m_pDragDropControl->DrawDragDrop(m_currentMousePos.x, m_currentMousePos.y);
            AurGUICloseViewport();
        }
        AurGUIStopLayer();
    }

    if (!g_pAppManager->m_pClientExoApp->GetInFreeLook())
    {
        if (m_nMouseStateFlags & 8)
            m_pToolTipPanel->Draw(fDelta);

        if (m_fToolTipTimer >= 0.0f)
        {
            m_fToolTipTimer += fDelta;
            if (m_fToolTipTimer > m_fToolTipTimeDelay)
            {
                if (m_pMouseOverControl && m_pMouseOverControl->DisplayToolTip())
                    m_pToolTipControl = m_pMouseOverControl;
                m_fToolTipTimer = -1.0f;
            }
        }
    }

#ifndef K2_UI_TOUCH
    if (m_pSoftwareMouse)
    {
        // The cursor art is 32 pixels; on a high-density display that is 32 x the back-buffer
        // pixels per window point, and at least 32 per 1080 lines of the screen.
        int nWinW, nWinH;
        SDL_GetWindowSize(Android_Window, &nWinW, &nWinH);
        int nScale = nWinH > 0 ? (g_SDL_mode.h + nWinH / 2) / nWinH : 1;
#ifdef _WIN32
        if (nScale < (g_SDL_mode.h + 270) / 540)
            nScale = (g_SDL_mode.h + 270) / 540;
#else
        if (nScale < (g_SDL_mode.h + 540) / 1080)
            nScale = (g_SDL_mode.h + 540) / 1080;
#endif
        DrawPcCursor(this, 32 * (nScale > 1 ? nScale : 1));
    }
#else
    if (m_pSoftwareMouse)
    {
        AurGUIStartLayer();
        m_pSoftwareMouse->Draw(fDelta);
        AurGUIStopLayer();
    }
#endif

    int i = m_panels.m_count;
    while (--i >= 0)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        unsigned short nFlags = pPanel->m_stateFlags;
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
    }

    if (g_bDrawSafeRegion)
        DrawSafeRegion(fDelta);

    if (GetMiniGamePlayingType())
    {
        int bLoadScreen = g_pAppManager->m_pClientExoApp->GetLoadScreenVisible();
        AurGUIStartLayer();
        static CAurGUIImageInternal* pImgPause = NewCAurGUIImage(const_cast<char*>("i_pause"));
        static CAurGUIImageInternal* pImgPause3 = NewCAurGUIImage(const_cast<char*>("i_pause3"));
        CAurGUIImageInternal* pPause = Ios_minigamePaused ? pImgPause3 : pImgPause;
        int nScreenWidth, nScreenHeight;
        IosIsIphone(nScreenWidth, nScreenHeight);
        if (!bLoadScreen)
        {
#ifdef K2_UI_TOUCH
            // The pause button is touch art (KOTOR II's arm below draws it on the touch UI only).
            if (IosGetTemporaryDisableInteraction() <= 0)
#else
            if (false)
#endif
            {
                int nSize = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 640.0f * 64.0f);
                pPause->Draw(g_SDL_mode.w / 2 + g_SDL_mode.h * 2 / 3 - nSize, 0, nSize, nSize, 0,
                             0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
            }
            int nBar = static_cast<int>((static_cast<float>(g_SDL_mode.w) - static_cast<float>(g_SDL_mode.h) / 768.0f * 1024.0f) / 2);
            android_port_glEnable(GL_SCISSOR_TEST);
            android_port_glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            android_port_glScissor(0, 0, nBar, g_SDL_mode.h);
            android_port_glClear(GL_COLOR_BUFFER_BIT);
            android_port_glScissor(g_SDL_mode.w - nBar, 0, nBar, g_SDL_mode.h);
            android_port_glClear(GL_COLOR_BUFFER_BIT);
            android_port_glScissor(0, 0, g_SDL_mode.w, g_SDL_mode.h);
            android_port_glDisable(GL_SCISSOR_TEST);
        }
        AurGUIStopLayer();
    }
#else
    int nCount = m_panels.m_count;

    AurMakeCurrent();
    CSWGuiListBox::UpdateArrowPulsing(fDelta);
    CSWGuiDrawable::UpdatePulsing(fDelta);

    int nMiniGame = GetMiniGamePlayingType();
    if (nMiniGame)
    {
        int nScreenHeight = g_nScreenHeight;
        int nScreenWidth = g_nScreenWidth;
        AurGUIStartLayer();
        if (!g_pAppManager->m_pClientExoApp->GetLoadScreenVisible() && nMiniGame == 2 &&
            static_cast<float>(nScreenWidth) / static_cast<float>(nScreenHeight) > 1.3333f && m_pFrameImage)
        {
#ifndef K2_UI_TOUCH
            // The swoop HUD's widescreen frame is the Android data's "swoop"; a PC install has none,
            // and the null texture would draw the frame's extent as a white sheet over the race.
            static const bool s_bSwoopFrame = BackdropExists("swoop");
            if (s_bSwoopFrame)
#endif
            m_pFrameImage->Draw(fDelta);
        }
        AurGUIStopLayer();
    }

    nMiniGame = GetMiniGamePlayingType();
    int nInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
#ifndef K2_UI_TOUCH
    // Only the touch UI has minigame touch widgets: the pause button, the swoop jump/boost and
    // the paused forfeit are Android-only art. Only the draw goes; the touch hit areas stay live.
    nMiniGame = 0;
#endif
    if (nMiniGame && nInputClass == 1)
    {
        AurGUIStartLayer();

        static CAurGUIImageInternal* pImgPause = NewCAurGUIImage(const_cast<char*>("i_pause"));
        static CAurGUIImageInternal* pImgPause3 = NewCAurGUIImage(const_cast<char*>("i_pause3"));
        static CAurGUIImageInternal* pImgForfeit = NewCAurGUIImage(const_cast<char*>("ipho_forfeit"));
        static CAurGUIImageInternal* pImgJump = NewCAurGUIImage(const_cast<char*>("ipho_jump"));

        float fScale = static_cast<float>(m_nScreenHeight) / 768.0f;
        int nSize = static_cast<int>(fScale * 64.0f);
        float fSize = static_cast<float>(nSize);
        float fScaled = fScale * 64.0f;

        CAurGUIImageInternal* pPause = Ios_minigamePaused ? pImgPause3 : pImgPause;
        pPause->Draw(nSize, nSize, nSize, nSize, 0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);

        if (nMiniGame == 2)
        {
            int nButtonW = static_cast<int>(fSize * 1.75f);
            int nJumpX = static_cast<int>(static_cast<double>(g_nScreenWidth / 2) + static_cast<double>(g_nScreenHeight) * -1.9 * 0.5);

            if (nJumpX >= 0)
                pImgJump->Draw(nJumpX - nButtonW / 4,
                               nButtonW / 4 - static_cast<int>(fScaled + static_cast<float>(nButtonW)) + m_nScreenHeight,
                               nButtonW, nButtonW, 0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
            else if (nJumpX + nButtonW > 10)
                pImgJump->Draw(10,
                               nButtonW / 4 - static_cast<int>(fScaled + static_cast<float>(nButtonW)) + m_nScreenHeight,
                               nButtonW, nButtonW, 0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
            else
                pImgJump->Draw(nSize - nButtonW / 4,
                               nButtonW / 4 - static_cast<int>(fScaled + static_cast<float>(nButtonW)) + m_nScreenHeight,
                               nButtonW, nButtonW, 0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);

            if (Ios_minigamePaused)
                pImgForfeit->Draw(m_nScreenWidth - static_cast<int>(fScaled + fSize), nSize, nSize, nSize, 0,
                                  0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
        }

        AurGUIStopLayer();
    }

    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (!pPanel)
        {
            m_panels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        if (GetPosInModalStack(pPanel) == -1)
            pPanel->Draw(fDelta);
    }

    nCount = m_modalPanels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_modalPanels.m_pArray[i];
        if (!pPanel)
        {
            m_modalPanels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        pPanel->Draw(fDelta);
    }

    if (m_pDragDropControl)
    {
        AurGUIStartLayer();
        if (AurGUISetupViewport(0, 0, m_nScreenWidth, m_nScreenHeight,
                                Vector(-1.0f, -1.0f, -1.0f), 0, 1.0f))
        {
            m_pDragDropControl->DrawDragDrop(m_currentMousePos.x, m_currentMousePos.y);
            AurGUICloseViewport();
        }
        AurGUIStopLayer();
    }

    if (!g_pAppManager->m_pClientExoApp->GetInFreeLook())
    {
        if (m_nMouseStateFlags & 8)
            m_pToolTipPanel->Draw(fDelta);

        if (m_fToolTipTimer >= 0.0f)
        {
            m_fToolTipTimer += fDelta;
            if (m_fToolTipTimer > m_fToolTipTimeDelay)
            {
                if (m_pMouseOverControl && m_pMouseOverControl->DisplayToolTip())
                    m_pToolTipControl = m_pMouseOverControl;
                m_fToolTipTimer = -1.0f;
            }
        }
    }

#ifndef K2_UI_TOUCH
    if (m_pSoftwareMouse)
#ifdef _WIN32
        DrawPcCursor(this, 32 * (m_nScreenHeight >= 810 ? (m_nScreenHeight + 270) / 540 : 1));
#else
        DrawPcCursor(this, 32 * (m_nScreenHeight >= 1620 ? (m_nScreenHeight + 540) / 1080 : 1));
#endif
#else
    if (m_pSoftwareMouse)
    {
        AurGUIStartLayer();
        m_pSoftwareMouse->Draw(fDelta);
        AurGUIStopLayer();
    }
#endif

    int i = m_panels.m_count;
    while (--i >= 0)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        unsigned short nFlags = pPanel->m_stateFlags;
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
    }

    if (g_bDrawSafeRegion)
        DrawSafeRegion(fDelta);
#endif
}

// The per-frame tick. Two stacks are walked in the same shape, the main panel list and then the
// modal list, and a panel is ticked only when it is visible and not already on the modal stack. A
// null slot is compacted out and the index is backed up so the shifted neighbour is re-examined.
// Finally every panel carrying a pending-destroy flag is removed.
//
// Both walks use a count snapshotted before the loop, so the size is the pre-frame one.
void CSWGuiManager::Update(float fDeltaT)
{
    int nCount = m_panels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (!pPanel)
        {
            m_panels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        if (GetPosInModalStack(pPanel) == -1)
            pPanel->Update(fDeltaT);
    }

    nCount = m_modalPanels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        CSWGuiPanel* pPanel = m_modalPanels.m_pArray[i];
        if (!pPanel)
        {
            m_modalPanels.DelIndex(i);
            i--;
            continue;
        }
        if (!(pPanel->m_stateFlags & 4))
            continue;
        pPanel->Update(fDeltaT);
    }

    int i = m_panels.m_count;
    while (--i >= 0)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        unsigned short nFlags = pPanel->m_stateFlags;
#if KOTOR1  // K1: the panel state bits sit one higher
        if (nFlags & PANEL_REMOVE_DEFERRED)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
#else
        if (nFlags & PANEL_REMOVE_DEFERRED)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
#endif
    }
}

// Puts a panel on screen. A panel that is already in the list is only un-hidden; a new one is
// pushed onto m_panels after its opacity bit and the camera-suppression counter are updated. Both
// then take the optional modal push, the panel-added notification and the draw refresh.
void CSWGuiManager::AddPanel(CSWGuiPanel* pPanel, int nFlags, int bPlaySound)
{
    if (!pPanel)
        return;

#if KOTOR1
    // K1 maps nFlags bits 1-2 onto panel state bits 3-4 (centre / no offset) and keeps a second
    // counter; re-adding a panel clears K1's state bits 8-10.
    if (bPlaySound)
        PlayGuiSound(pPanel->m_nOpenSoundIndex);

    if (GetPosInPanelList(pPanel) == -1)
    {
        pPanel->m_stateFlags = static_cast<unsigned short>((pPanel->m_stateFlags & ~(PANEL_STATE_FULLSCREEN | PANEL_STATE_NOT_CAPTURING)) | (((nFlags >> 1) & 3) << 3));

        if (nFlags & 2)
            m_bSuppressCameraRender++;
        if (!(pPanel->m_stateFlags & PANEL_STATE_NOT_CAPTURING))
            m_nCapturedPanelCount++;

        m_panels.Add(pPanel);
    }
    else
    {
        unsigned short nState = pPanel->m_stateFlags;
        if ((nState & PANEL_REMOVE_MASK) == 0)
            return;
        pPanel->m_stateFlags = nState & ~PANEL_REMOVE_STATE_MASK;
    }

    if (nFlags & 1)
        PushModalPanel(pPanel);

    pPanel->OnPanelAdded();
    DetermineNeedToDraw();
#else

    if (bPlaySound)
        PlayGuiSound(pPanel->m_nOpenSoundIndex);

    if (GetPosInPanelList(pPanel) == -1)
    {
#if !KOTOR1
        if (pPanel->ShouldFreeMouseOnAdd())
            EnableMouseCameraRotateToggle(false, 0);
#endif

        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~8) | ((nFlags << 2) & 8);

        if (nFlags & 2)
            m_bSuppressCameraRender++;

        m_panels.Add(pPanel);
    }
    else
    {
        unsigned short nState = pPanel->m_stateFlags;
        if ((nState & PANEL_REMOVE_MASK) == 0)
            return;
        pPanel->m_stateFlags = nState & ~PANEL_REMOVE_STATE_MASK;
    }

    if (nFlags & 1)
        PushModalPanel(pPanel);

    pPanel->OnPanelAdded();
    DetermineNeedToDraw();
#endif
}

// The inverse of AddPanel. Returns whether the panel was actually removed.
//
// The modal arm is PopModalPanel: when the panel is the top of the modal stack it goes through
// the pop, otherwise it is erased in place.
//
// The main-interface search is open-coded rather than using the panel-list helper because what
// escapes the loop is control flow rather than an index.
int CSWGuiManager::RemovePanel(CSWGuiPanel* pPanel)
{
    if (!pPanel)
        return 0;

    if (GetPosInPanelList(pPanel) == -1)
        return 0;

    int nModal = GetPosInModalStack(pPanel);
    if (nModal != -1)
    {
        if (nModal == m_modalPanels.m_count - 1)
            PopModalPanel();
        else
            m_modalPanels.DelIndex(nModal);
    }

    pPanel->OnPanelRemoved();

    int nIndex = GetPosInPanelList(pPanel);
    if (nIndex == -1)
        return 0;

    m_panels.DelIndex(nIndex);

    if (pPanel->m_stateFlags & 8)
        m_bSuppressCameraRender--;
#if KOTOR1  // K1 keeps a second counter and has no camera toggle or mouse refresh
    if (!(pPanel->m_stateFlags & PANEL_STATE_NOT_CAPTURING))
        m_nCapturedPanelCount--;

    DetermineNeedToDraw();

    m_pMouseOverControl = 0;
    m_pMouseCaptureObject = 0;
    return 1;
#else

    DetermineNeedToDraw();

    CSWGuiMainInterface* pMain =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
    // The main menu panel keeps a menu flag in the internal app; clear it when this removes the
    // in-game GUI's main interface and no main menu is up.
    void* pMenu = g_pAppManager->m_pClientExoApp->m_pInternal->m_pMainMenuPanel;
    if (pMain && !pMenu)
    {
        for (int i = 0; i < m_panels.m_count; i++)
        {
            if (m_panels.m_pArray[i] == pMain)
            {
                if (m_modalPanels.m_count == 0)
                    EnableMouseCameraRotateToggle(false);
                break;
            }
        }
    }

    m_pMouseOverControl = 0;
    UpdateMouseMove();
    m_pMouseCaptureObject = 0;
    return 1;
#endif
}

// Draws the safe-region overlay: a bare panel holding one label whose border is inset by the
// region's own frame thickness. The pair is built once and re-laid-out every frame.
void CSWGuiManager::DrawSafeRegion(float fAlpha)
{
    if (m_pSafeRegionPanel == 0)
    {
        m_pSafeRegionPanel = new CSWGuiPanel(this);
        m_pSafeRegionLabel = new CSWGuiLabel();
        m_pSafeRegionPanel->AddControl(m_pSafeRegionLabel);
        // All three colour components are -1.0f.
        m_pSafeRegionPanel->m_vColor = Vector(-1.0f, -1.0f, -1.0f);

        CSWGuiExtent extent;
        CSWGuiTextParams textParams;
        Vector color(1.0f, 1.0f, 1.0f);
        CSWGuiBorderParams borderParams(CResRef("border2b"), CResRef("border1b"), CResRef(),
                                       0, 0, 0, 1.0f, color, 0, 0);
        m_pSafeRegionLabel->Initialize(extent, textParams, borderParams, 1.0f);
    }

    CSWGuiExtent extent;
    extent.w = m_nScreenWidth;
    extent.h = m_nScreenHeight;
    m_pSafeRegionPanel->SetExtent(extent);

    // The label is inset by the border's own quote, twice per axis; the getter is called once per
    // axis even though it cannot differ between them.
    int nInsetX = 5 - m_pSafeRegionLabel->m_border.m_params.GetBorderDim();
    int nInsetY = 5 - m_pSafeRegionLabel->m_border.m_params.GetBorderDim();
    int nWidth = m_nScreenWidth - nInsetX * 2;
    int nHeight = m_nScreenHeight - nInsetY * 2;
    CSWGuiExtent labelExtent;
    labelExtent.x = nInsetX;
    labelExtent.y = nInsetY;
    labelExtent.w = nWidth;
    labelExtent.h = nHeight;
    m_pSafeRegionLabel->SetExtent(labelExtent);

    m_pSafeRegionPanel->Draw(fAlpha);
}

// Re-lays-out the GUI for a new physical screen size, early-returning when nothing changed.
//
// The backdrop extent is a copy of the background image's extent with x, w and h overridden, and
// the GUI width is derived from the physical height (the 1.59 letterbox aspect) and then centred
// inside the screen width.
void CSWGuiManager::SetSize(int nWidth, int nHeight)
{
    if (m_nScreenWidth == nWidth && m_nScreenHeight == nHeight)
        return;

#if KOTOR1  // K1 has no native-resolution rescale, letterbox extent or edge inset
    m_nScreenWidth = static_cast<short>(nWidth);
    m_nScreenHeight = static_cast<short>(nHeight);
#else
    ScaleNativeResolution();
    m_nScreenWidth = static_cast<short>(nWidth);
    m_nScreenHeight = static_cast<short>(nHeight);

    // The copy has to come before the GUI width is computed.
    CSWGuiExtent ext = m_pFrameImage->m_extent;
    int nGuiWidth = static_cast<int>(g_nScreenHeight * 1.59);
    ext.x = (g_nScreenWidth - nGuiWidth) / 2;
    ext.w = nGuiWidth;
    ext.h = m_nScreenHeight;
    m_pFrameImage->SetExtent(ext);

    // The edge offset is the left operand of the multiply.
#if defined(K2_UI_PC) || (defined(K2_HOST_BUILD) && !defined(K2P_MOBILE_HOST))
    // pc, and any UI on a desktop: the HUD at the screen edges, as the PC build (the inset is the
    // phone's; at 4:3 it pushes the action bar and the portraits onto the combat queue)
    m_nUIEdgeInset = 0;
#else
    m_nUIEdgeInset = IsTablet() ? 0 : static_cast<int>(static_cast<unsigned>(g_nUIEdgeOffset)
                                            * (static_cast<unsigned>(nWidth) / static_cast<unsigned>(g_nGUIWidth)));
#endif
#endif

    for (int i = 0; i < m_panels.m_count; i++)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        if (pPanel->m_bAllowFontReset)
            pPanel->ResetFont();
    }

    if (m_pToolTipPanel)
        m_pToolTipPanel->ResetFont();

    if (g_pAppManager->m_pClientExoApp->GetInGameGui())
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ResetInterfaceFont();

    if (m_pBackgroundLabel)
    {
        delete m_pBackgroundLabel;
        m_pBackgroundLabel = 0;
        DetermineNeedToDraw();
    }
}

// Builds m_paGuiSounds from the SoundResRef column of guisounds.2da; returns 0 when the 2DA will
// not load, 1 otherwise.
int CSWGuiManager::LoadGuiSounds()
{
    C2DA* p2DA = new C2DA(CResRef("guisounds"), 0);
    if (!p2DA->Load2DArray())
    {
        delete p2DA;
        return 0;
    }

    if (p2DA->m_nRows >= 1)
    {
        CExoString sEntry;
        m_nGuiSounds = static_cast<char>(p2DA->m_nRows);
        m_paGuiSounds = new CExoSoundSource*[m_nGuiSounds];

        for (int i = 0; i < m_nGuiSounds; i++)
        {
            if (p2DA->GetCExoStringEntry(i, CExoString("SoundResRef"), &sEntry))
            {
                m_paGuiSounds[i] = new CExoSoundSource(CResRef(sEntry));
                if (m_paGuiSounds[i])
                    m_paGuiSounds[i]->SetPriorityGroup(PRIORITY_GROUP_GUI);
            }
        }
    }

    delete p2DA;
    return 1;
}

void CSWGuiManager::ClearPanels()
{
    m_modalPanels.SetSize(0);
    m_panels.SetSize(0);
}

// The modal search has no null-element guard, unlike the panel-list search.
int CSWGuiManager::GetPosInModalStack(CSWGuiPanel* pPanel)
{
    int nCount = m_modalPanels.m_count;
    for (int i = 0; i < nCount; i++)
    {
        if (m_modalPanels.m_pArray[i] == pPanel)
            return i;
    }
    return -1;
}

// Pushes a panel onto the modal stack, adding it to the normal panel list first if it is not
// already there. Returns whether it was pushed.
int CSWGuiManager::PushModalPanel(CSWGuiPanel* pPanel)
{
    if (!pPanel)
        return 0;

    if (m_pMouseOverControl)
        m_pMouseOverControl->HandleMouseOver(0);

    if (GetPosInPanelList(pPanel) == -1)
        AddPanel(pPanel, 0, 1);

    if (GetPosInModalStack(pPanel) != -1)
        return 0;

    if (m_pFocusedEditbox)
        m_pFocusedEditbox->HandleFocusChange(0);

    m_modalPanels.Add(pPanel);
    return 1;
}

// "Front" is the end of m_panels, i.e. drawn last, so this is a plain remove-then-append. A panel
// on the modal stack is exempt: modal z-order is owned by that stack, and that early return is
// the one path which skips the draw refresh.
void CSWGuiManager::BringPanelToFront(CSWGuiPanel* pPanel)
{
    if (m_modalPanels.IndexOf(pPanel) != -1)
        return;

    int nIndex = m_panels.IndexOf(pPanel);
    if (nIndex != -1)
    {
        m_panels.DelIndex(nIndex);
        m_panels.Add(pPanel);
    }
    DetermineNeedToDraw();
}

// The other three of the panel z-order quartet. Same skeleton as BringPanelToFront, only the
// destination index differs, and the early-outs fall through to the shared draw refresh rather
// than returning.
void CSWGuiManager::SendPanelToBack(CSWGuiPanel* pPanel)
{
    if (m_modalPanels.IndexOf(pPanel) != -1)
        return;

    int nIndex = m_panels.IndexOf(pPanel);
    if (nIndex != -1)
    {
        m_panels.DelIndex(nIndex);
        m_panels.Insert(pPanel, 0);
    }
    DetermineNeedToDraw();
}

void CSWGuiManager::BringPanelForward(CSWGuiPanel* pPanel)
{
    if (m_modalPanels.IndexOf(pPanel) != -1)
        return;

    int nIndex = m_panels.IndexOf(pPanel);
    if (nIndex != -1)
    {
        if (m_panels.m_count - 1 > nIndex)
        {
            m_panels.DelIndex(nIndex);
            m_panels.Insert(pPanel, nIndex + 1);
        }
    }
    DetermineNeedToDraw();
}

void CSWGuiManager::SendPanelBack(CSWGuiPanel* pPanel)
{
    if (m_modalPanels.IndexOf(pPanel) != -1)
        return;

    int nIndex = m_panels.IndexOf(pPanel);
    int nDest = nIndex - 1;
    if (nDest >= 0)
    {
        m_panels.DelIndex(nIndex);
        m_panels.Insert(pPanel, nDest);
    }
    DetermineNeedToDraw();
}

void CSWGuiManager::ChangeToolTipText(const CExoString& text)
{
    if (m_pToolTipPanel)
        m_pToolTipPanel->SetToolTipText(text);
}

void CSWGuiManager::PlayGuiSound(char nSound)
{
    if (nSound == -1)
        return;
    if (m_nGuiSounds <= nSound)
        return;
    if (m_paGuiSounds == 0)
        return;

    CExoSoundSource* pSound = m_paGuiSounds[static_cast<int>(nSound)];
    if (pSound)
        pSound->Play();
}

// Shows or hides every panel. The count is re-read on each iteration because hiding a panel can
// resize the list.
void CSWGuiManager::HideAllPanels()
{
    for (int i = 0; i < m_panels.m_count; i++)
        m_panels.m_pArray[i]->SetVisible(0);
}

void CSWGuiManager::ShowAllPanels()
{
    for (int i = 0; i < m_panels.m_count; i++)
        m_panels.m_pArray[i]->SetVisible(1);
}

// Walks each stack from the top down, skipping panels with the pending-destroy bits set, and
// reports whether the first eligible one is the argument. The modal stack wins; only when it
// yields nothing is the normal stack consulted.
int CSWGuiManager::IsOnTop(CSWGuiPanel* pPanel)
{
#if KOTOR1
    // K1 walks the same stacks with its state bits one higher.
    for (int i = m_modalPanels.m_count - 1; i >= 0; i--)
    {
        CSWGuiPanel* p = m_modalPanels.m_pArray[i];
        if ((p->m_stateFlags & PANEL_REMOVE_MASK) == 0)
            return p == pPanel;
    }
    for (int i = m_panels.m_count - 1; i >= 0; i--)
    {
        CSWGuiPanel* p = m_panels.m_pArray[i];
        if ((p->m_stateFlags & PANEL_REMOVE_MASK) == 0)
            return p == pPanel;
    }
#else
    for (int i = m_modalPanels.m_count - 1; i >= 0; i--)
    {
        CSWGuiPanel* p = m_modalPanels.m_pArray[i];
        if ((p->m_stateFlags & PANEL_REMOVE_MASK) == 0)
            return p == pPanel;
    }
    for (int i = m_panels.m_count - 1; i >= 0; i--)
    {
        CSWGuiPanel* p = m_panels.m_pArray[i];
        if ((p->m_stateFlags & PANEL_REMOVE_MASK) == 0)
            return p == pPanel;
    }
#endif
    return 0;
}

// The window-resize hook. Three blocks with the same body over three panel lists: the live stack,
// the modal stack, then the in-game HUD's fixed 8-slot screen-panel array. Per panel: scale its
// extent so a panel designed at its own size fills the new window, apply the same rectangle to the
// background border, then repeat the scale for every control that is itself a panel.
//
// The scale pair is computed once per panel and reused by that panel's control walk.
void CSWGuiManager::ReloadPanels(int nWidth, int nHeight)
{
    CSWGuiExtent extent;

    for (int i = 0; i < m_panels.m_count; i++)
    {
        extent = m_panels.m_pArray[i]->m_extent;
        float fScaleX = static_cast<float>(nWidth) / static_cast<float>(extent.w);
        float fScaleY = static_cast<float>(nHeight) / static_cast<float>(extent.h);
        extent.x = static_cast<int>(extent.x * fScaleX);
        extent.y = static_cast<int>(extent.y * fScaleY);
        extent.w = static_cast<int>(extent.w * fScaleX);
        extent.h = static_cast<int>(extent.h * fScaleY);

        m_panels.m_pArray[i]->SetExtent(extent);
        m_panels.m_pArray[i]->m_pBackground->SetExtent(extent);

        for (int j = 0; j < m_panels.m_pArray[i]->m_controls.m_count; j++)
        {
            if (m_panels.m_pArray[i]->m_controls.m_pArray[j] != 0 &&
                m_panels.m_pArray[i]->m_controls.m_pArray[j]->AsSWGuiControl() != 0)
            {
                extent = m_panels.m_pArray[i]->m_controls.m_pArray[j]->AsSWGuiControl()->m_extent;
                extent.x = static_cast<int>(extent.x * fScaleX);
                extent.y = static_cast<int>(extent.y * fScaleY);
                extent.w = static_cast<int>(extent.w * fScaleX);
                extent.h = static_cast<int>(extent.h * fScaleY);

                m_panels.m_pArray[i]->m_controls.m_pArray[j]->SetExtent(extent);
            }
        }
    }

    for (int i = 0; i < m_modalPanels.m_count; i++)
    {
        extent = m_modalPanels.m_pArray[i]->m_extent;
        float fScaleX = static_cast<float>(nWidth) / static_cast<float>(extent.w);
        float fScaleY = static_cast<float>(nHeight) / static_cast<float>(extent.h);
        extent.x = static_cast<int>(extent.x * fScaleX);
        extent.y = static_cast<int>(extent.y * fScaleY);
        extent.w = static_cast<int>(extent.w * fScaleX);
        extent.h = static_cast<int>(extent.h * fScaleY);

        m_modalPanels.m_pArray[i]->SetExtent(extent);
        m_modalPanels.m_pArray[i]->m_pBackground->SetExtent(extent);

        for (int j = 0; j < m_modalPanels.m_pArray[i]->m_controls.m_count; j++)
        {
            if (m_modalPanels.m_pArray[i]->m_controls.m_pArray[j] != 0 &&
                m_modalPanels.m_pArray[i]->m_controls.m_pArray[j]->AsSWGuiControl() != 0)
            {
                extent = m_modalPanels.m_pArray[i]->m_controls.m_pArray[j]->AsSWGuiControl()->m_extent;
                extent.x = static_cast<int>(extent.x * fScaleX);
                extent.y = static_cast<int>(extent.y * fScaleY);
                extent.w = static_cast<int>(extent.w * fScaleX);
                extent.h = static_cast<int>(extent.h * fScaleY);

                m_modalPanels.m_pArray[i]->m_controls.m_pArray[j]->SetExtent(extent);
            }
        }
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0)
    {
        CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        for (int i = 0; i < 8; i++)
        {
            if (pInGameGui->m_apScreenPanels[i] == 0)
                continue;

            extent = pInGameGui->m_apScreenPanels[i]->m_extent;
            float fScaleX = static_cast<float>(nWidth) / static_cast<float>(extent.w);
            float fScaleY = static_cast<float>(nHeight) / static_cast<float>(extent.h);
            extent.x = static_cast<int>(extent.x * fScaleX);
            extent.y = static_cast<int>(extent.y * fScaleY);
            extent.w = static_cast<int>(extent.w * fScaleX);
            extent.h = static_cast<int>(extent.h * fScaleY);

            pInGameGui->m_apScreenPanels[i]->SetExtent(extent);
            pInGameGui->m_apScreenPanels[i]->m_pBackground->SetExtent(extent);

            for (int j = 0; j < pInGameGui->m_apScreenPanels[i]->m_controls.m_count; j++)
            {
                if (pInGameGui->m_apScreenPanels[i]->m_controls.m_pArray[j] != 0 &&
                    pInGameGui->m_apScreenPanels[i]->m_controls.m_pArray[j]->AsSWGuiControl() != 0)
                {
                    extent = pInGameGui->m_apScreenPanels[i]->m_controls.m_pArray[j]
                                 ->AsSWGuiControl()->m_extent;
                    extent.x = static_cast<int>(extent.x * fScaleX);
                    extent.y = static_cast<int>(extent.y * fScaleY);
                    extent.w = static_cast<int>(extent.w * fScaleX);
                    extent.h = static_cast<int>(extent.h * fScaleY);

                    pInGameGui->m_apScreenPanels[i]->m_controls.m_pArray[j]->SetExtent(extent);
                }
            }
        }
    }
}

// The pointer and the count are re-read after every delete.
void CSWGuiManager::UnloadGuiSounds()
{
    if (m_paGuiSounds)
    {
        for (int i = 0; i < m_nGuiSounds; i++)
        {
            CExoSoundSource* pSound = m_paGuiSounds[i];
            if (pSound)
            {
                delete pSound;
                m_paGuiSounds[i] = 0;
            }
        }
        delete[] m_paGuiSounds;
        m_paGuiSounds = 0;
    }
    m_nGuiSounds = 0;
}

// Routes a key press to the focused edit box. With a modal panel up the edit box gets the key
// only if it is that panel's active control; the return value is 0 either way.
int CSWGuiManager::HandleKeyPress(int nKey)
{
    if (m_modalPanels.m_count)
    {
        if (m_modalPanels.m_pArray[m_modalPanels.m_count - 1]->GetActiveControl() ==
            static_cast<CSWGuiControl*>(m_pFocusedEditbox))
        {
            m_pFocusedEditbox->HandleKeyPress(nKey);
        }
        return 0;
    }

    int bHandled = 0;

    if (m_pFocusedEditbox)
    {
        m_pFocusedEditbox->HandleKeyPress(nKey);
        bHandled = 1;
    }

    return bHandled;
}

// Appends the resolution/font-size suffix a GUI font name needs, in place. "fnt_console" is the
// one name that is never suffixed.
//
// The suffix index is an unsigned char, so the length is truncated to 8 bits before use.
void CSWGuiManager::GetUpdatedFontName(char* szFontName)
{
    if (strcmp("fnt_console", szFontName))
    {
        unsigned char nLen = static_cast<unsigned char>(strlen(szFontName));
        unsigned char nFontSize = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nFontSize;

        if (nFontSize == 1)
            szFontName[nLen] = 'a';
        else if (nFontSize == 2)
            szFontName[nLen] = 'b';
        else
        {
            char* pSuffix = &szFontName[nLen];
            *pSuffix = g_nScreenWidth < 1280 ? 'a' : 'b';
        }
        szFontName[nLen + 1] = '\0';
    }
}

// Derives the scale factors and origin a panel needs to map its design-space extent onto the
// physical screen. `bUseNativeRect` selects between the letterboxed 4:3 sub-rectangle, which has a
// non-zero top-left origin, and the raw screen, which has none.
//
// The non-native branch reads the cached resolution off the manager reached through the app
// manager, not off `this`, even though the two are the same object at every call site.
void CSWGuiManager::GenerateAspectMultipliers(int bUseNativeRect, float& fXMult, float& fYMult,
                                              int& nXOffset, int& nYOffset,
                                              float fBaseWidth, float fBaseHeight) const
{
    if (bUseNativeRect)
    {
        fXMult = g_nScaledNativeWidth / fBaseWidth;
        fYMult = g_nScaledNativeHeight / fBaseHeight;
        nXOffset = g_nScreenLeftOffset;
        nYOffset = g_nScreenTopOffset;
    }
    else
    {
        fXMult = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth / fBaseWidth;
        fYMult = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight / fBaseHeight;
        nXOffset = 0;
        nYOffset = 0;
    }
}

// Appends a control unless it is already in the list. The duplicate check goes through the
// control-list helper; the append is CExoArrayList<CSWGuiControl*>::Add.
void CSWGuiPanel::AddControl(CSWGuiControl* pControl)
{
    if (m_controls.IndexOf(pControl) != -1)
        return;

    pControl->m_pContainer = this;
    m_controls.Add(pControl);
#if !KOTOR1
    pControl->m_bCenterOnNativeRect = m_bCenterOnNativeRect;
#endif
}

// Hit-tests the panel's rectangle and then, top-down, its controls, returning the first control
// that claims the point.
//
// With the panel's centre bit set its rectangle is nudged by the manager's cached resolution;
// with it clear the panel stays put and the mouse is moved instead, against the screen globals.
CSWGuiControl* CSWGuiPanel::HitCheckMouse(int x, int y)
{
    CSWGuiExtent extent = m_extent;

#if KOTOR1  // K1's centre bit is 3; its anchor bits sit one higher
    if (m_stateFlags & 1)
    {
        if (m_stateFlags & 8)
        {
            extent.x += (m_pGuiManager->m_nScreenWidth - extent.w) / 2;
            extent.y += (m_pGuiManager->m_nScreenHeight - extent.h) / 2;
        }
        else
        {
            if (m_stateFlags & PANEL_STATE_CENTER_X)
                extent.x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
            if (m_stateFlags & PANEL_STATE_CENTER_Y)
                extent.y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
        }
    }
#else
    if (m_stateFlags & 1)
    {
        if (!(m_stateFlags & 8))
        {
            if (m_stateFlags & PANEL_STATE_CENTER_X)
                extent.x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
            if (m_stateFlags & PANEL_STATE_CENTER_Y)
                extent.y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
        }
    }
#endif
    else
    {
        x -= (g_nScreenWidth - 640) / 2;
        y -= (g_nScreenHeight - 480) / 2;
    }

    // The four bounds tests, in this order.
    if (x < extent.x || x > extent.x + extent.w || y < extent.y || y > extent.y + extent.h)
        return 0;

    for (int i = m_controls.m_count; i > 0; i--)
    {
        CSWGuiControl* pControl = m_controls.m_pArray[i - 1];
        if (pControl)
        {
            CSWGuiControl* pHit = pControl->HitCheckMouse(x - extent.x, y - extent.y);
            if (pHit)
                return pHit;
        }
    }

    return 0;
}

// Draws the panel's background and every visible drawable control inside one GUI layer.
#if KOTOR1
void CSWGuiPanel::Draw(float fDelta)
{
    if (!(m_stateFlags & PANEL_STATE_DRAW))
        return;

    int nControls = m_controls.m_count;
    int x = m_extent.x;
    int y = m_extent.y;
    int w = m_extent.w;
    int h = m_extent.h;

    if (m_stateFlags & 8)
    {
        x += (m_pGuiManager->m_nScreenWidth - w) / 2;
        y += (m_pGuiManager->m_nScreenHeight - h) / 2;
    }
    else
    {
        if (m_stateFlags & PANEL_STATE_CENTER_X)
            x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
        if (m_stateFlags & PANEL_STATE_CENTER_Y)
            y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }

    AurGUIStartLayer();

    if (AurGUISetupViewport(x, y, w, h, m_vColor, false, m_fAlpha))
    {
        m_pBackground->Draw(fDelta);

        for (int i = 0; i < nControls; i++)
        {
            CSWGuiControl* pControl = m_controls.m_pArray[i];
            if (pControl && pControl->m_tail.bVisible)
                pControl->Draw(fDelta);
        }

        AurGUICloseViewport();
    }

    AurGUIStopLayer();
}
#else
void CSWGuiPanel::Draw(float fDelta)
{
    if (!(m_stateFlags & PANEL_STATE_DRAW))
        return;

    if (m_pNextInChain)
        m_pNextInChain->Draw(fDelta);

    // The guard reads the fields directly, not four locals, so that every test runs in order: h
    // before w, and the locals declared as x, y, w, h below.
    if (m_extent.x < 0 || m_extent.y < 0 || m_extent.h < 1 || m_extent.w < 1 ||
        m_extent.w + m_extent.x > m_pGuiManager->m_nScreenWidth ||
        m_extent.h + m_extent.y > m_pGuiManager->m_nScreenHeight)
    {
        if (!m_bDrawOffScreen)
            return;
    }

    int x = m_extent.x;
    int y = m_extent.y;
    int w = m_extent.w;
    int h = m_extent.h;
    int nControls = m_controls.m_count;

    // The same centring correction as GetExtentAccountingForPanelOffset.
    if (!(m_stateFlags & 8))
    {
        if (m_stateFlags & PANEL_STATE_CENTER_X)
            x += (m_pGuiManager->m_nScreenWidth - 640) / 2;
        if (m_stateFlags & PANEL_STATE_CENTER_Y)
            y += (m_pGuiManager->m_nScreenHeight - 480) / 2;
    }

    AurGUIStartLayer();

    if (AurGUISetupViewport(x, y, w, h, m_vColor, false, m_fAlpha))
    {
        m_pBackground->Draw(fDelta);

        for (int i = 0; i < nControls; i++)
        {
            CSWGuiControl* pControl = m_controls.m_pArray[i];
            if (pControl && pControl->m_tail.bVisible)
                pControl->Draw(fDelta);
        }

        AurGUICloseViewport();
    }

    AurGUIStopLayer();
}
#endif

// Runs when the panel has just gone on screen: re-applies the font, re-runs the mouse-over hit
// test, and gives the control under the cursor its highlight when it belongs to this panel and is
// enabled. When that control is a button and is not already the active one, the active control is
// told the selection moved.
void CSWGuiPanel::OnPanelAdded()
{
    ResetFont();
    g_pGuiMan->UpdateMouseOverControl();

    CSWGuiControl* pControl = m_pGuiManager->m_pMouseOverControl;
    if (!pControl)
        return;

    // An open-coded search on a flag, not an index.
    int bFound = 0;
    for (int i = 0; i < m_controls.m_count; i++)
    {
        if (m_controls.m_pArray[i] == pControl)
        {
            bFound = 1;
            break;
        }
    }
    if (!bFound)
        return;

    if (!pControl->m_tail.bEnabled)
        return;

    pControl->HandleMouseOver(1);

    if (m_pActiveControl && pControl != m_pActiveControl && pControl->AsButton() &&
        pControl->m_tail.bFocusOnHover)
    {
        m_pActiveControl->HandleInputEvent(SWGUIEVENT_UNFOCUS, 1);
    }
}

// Runs when the panel is leaving the manager's stack: drops any tooltip it owns, hands keyboard
// focus back if this panel's active control had it, and cancels the mouse-over if the hovered
// control belongs to this panel.
void CSWGuiPanel::OnPanelRemoved()
{
    g_pGuiMan->RemoveToolTip();

    if (g_pGuiMan->m_pFocusedEditbox &&
        g_pGuiMan->m_pFocusedEditbox == static_cast<CSWGuiEditbox*>(GetActiveControl()))
    {
        g_pGuiMan->m_pFocusedEditbox->HandleFocusChange(0);
    }

    CSWGuiControl* pMouseOver = m_pGuiManager->m_pMouseOverControl;
    if (pMouseOver)
    {
        for (int i = 0; i < m_controls.m_count; i++)
        {
            if (m_controls.m_pArray[i] == pMouseOver)
            {
                m_pGuiManager->m_pMouseOverControl = 0;
                pMouseOver->HandleMouseOver(0);
                return;
            }
        }
    }
}

// Builds the resource name of the panel's full-screen background: the resolution name
// concatenated with the per-panel suffix out of CSWGuiManager::FULLSCREEN_BGS.
CExoString CSWGuiPanel::GetFullScreenBG()
{
#if KOTOR1
    // K1 has no per-resolution table; always the plain black backdrop.
    return CExoString("black");
#else
    CExoString sName("800x600");

    if (g_nScreenHeight >= 1200)
        sName = "1600x1200";
    else if (g_nScreenHeight >= 1024)
        sName = "1280x1024";
    else if (g_nScreenHeight >= 960)
        sName = "1280x960";
    else if (g_nScreenHeight >= 768)
        sName = "1024x768";

    sName = sName + CSWGuiManager::FULLSCREEN_BGS[m_nFullScreenBGIndex];

    return sName;
#endif
}

// Feeds the GUI's keyboard channel. Every input event is remapped from the port's raw codes into
// the engine's event ids and dispatched to the modal panel if one is up, otherwise to every panel
// in a snapshot of the live stack. Finally any panel carrying a pending-destroy flag is removed,
// and the menu image packs go with it.
//
// The snapshot is a by-value copy of the panel list, which is what stops a panel that removes
// itself from its own handler from invalidating the walk.
//
// Three of the port's raw codes are remapped, directionally: the incoming code decides the base
// and the event's sign (pressed or repeat) picks between the base and its neighbours. The repeat
// filter debounces across the two paired codes rather than within one, and the second switch's
// default is a fallthrough.
void CSWGuiManager::HandleInputEvent(int nType, int nVal)
{
#if KOTOR1 && defined(K2_UI_PC)
    // K1's GUI acts on the A event whatever its value: the port's pad and Chromebook keys send presses
    // only, so the keyboard's Enter does too.
    if ((nType == INPUTEVENT_KEY_RETURN || nType == INPUTEVENT_KEY_NUMPAD_ENTER) && nVal == 0)
        return;
#endif
    m_nCurrentInputEvent = nType;

    int bHandled = 0;
    if (nVal != 0)
    {
        switch (nType)
        {
        case INPUTEVENT_GUI_STICK_Y: nType = nVal == 1 ? INPUTEVENT_GUI_KEY_DOWN : (nVal == -1 ? INPUTEVENT_GUI_KEY_UP : INPUTEVENT_GUI_STICK_Y); break;
        case INPUTEVENT_GUI_STICK_X: nType = nVal == 1 ? INPUTEVENT_GUI_KEY_RIGHT : (nVal == -1 ? INPUTEVENT_GUI_KEY_LEFT : INPUTEVENT_GUI_STICK_X); break;
        case INPUTEVENT_GUI_RIGHT_STICK_Y: nType = nVal == 1 ? INPUTEVENT_GUI_SCROLL_DOWN : (nVal == -1 ? INPUTEVENT_GUI_SCROLL_UP : INPUTEVENT_GUI_RIGHT_STICK_Y); break;
        case INPUTEVENT_GUI_RIGHT_STICK_X: nType = nVal == 1 ? INPUTEVENT_GUI_SCROLL_RIGHT : (nVal == -1 ? INPUTEVENT_GUI_SCROLL_LEFT : INPUTEVENT_GUI_RIGHT_STICK_X); break;
        }

        unsigned int nTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        bHandled = 1;

        if (nType <= INPUTEVENT_KEY_ESCAPE - 1)
        {
            if (nType == INPUTEVENT_GUI_DPAD_LEFT || nType == INPUTEVENT_GUI_DPAD_RIGHT)
            {
                if (m_nRepeatFilterEvent == INPUTEVENT_GUI_DPAD_UP || m_nRepeatFilterEvent == INPUTEVENT_GUI_DPAD_DOWN)
                {
                    if (nTime - m_nRepeatFilterTime < 150)
                        return;
                    m_nRepeatFilterEvent = static_cast<unsigned char>(nType);
                }
                else if (m_nRepeatFilterEvent == 0)
                {
                    m_nRepeatFilterEvent = static_cast<unsigned char>(nType);
                }
                m_nRepeatFilterTime = nTime;
            }
            else if (nType == INPUTEVENT_GUI_DPAD_UP || nType == INPUTEVENT_GUI_DPAD_DOWN)
            {
                if (m_nRepeatFilterEvent == INPUTEVENT_GUI_DPAD_LEFT || m_nRepeatFilterEvent == INPUTEVENT_GUI_DPAD_RIGHT)
                {
                    if (nTime - m_nRepeatFilterTime < 150)
                        return;
                    m_nRepeatFilterEvent = static_cast<unsigned char>(nType);
                }
                else if (m_nRepeatFilterEvent == 0)
                {
                    m_nRepeatFilterEvent = static_cast<unsigned char>(nType);
                }
                m_nRepeatFilterTime = nTime;
            }
        }
    }

    switch (nType)
    {
    case INPUTEVENT_KEY_ESCAPE:
    case INPUTEVENT_GUI:
        if (bHandled)
            nType = INPUTEVENT_GUI_B;
        break;
    case INPUTEVENT_KEY_RETURN:
    case INPUTEVENT_KEY_NUMPAD_ENTER:
        nType = INPUTEVENT_GUI_A;
        break;
    case INPUTEVENT_KEY_UP: nType = INPUTEVENT_GUI_KEY_UP; break;
    case INPUTEVENT_KEY_DOWN: nType = INPUTEVENT_GUI_KEY_DOWN; break;
    case INPUTEVENT_KEY_LEFT: nType = INPUTEVENT_GUI_KEY_LEFT; break;
    case INPUTEVENT_KEY_RIGHT: nType = INPUTEVENT_GUI_KEY_RIGHT; break;
    }

    if (m_modalPanels.m_count != 0)
    {
        // A modal panel swallows the event whether or not it handles it.
        m_modalPanels.m_pArray[m_modalPanels.m_count - 1]->HandleInputEvent(nType, bHandled);
    }
    else if (m_panels.m_count != 0)
    {
        CExoArrayList<CSWGuiPanel*> aPanels;
        aPanels.SetSize(m_panels.m_count);
        for (int i = 0; i < m_panels.m_count; i++)
            aPanels.m_pArray[i] = m_panels.m_pArray[i];

        for (int i = 0; i < aPanels.m_count; i++)
        {
            CSWGuiPanel* pPanel = aPanels.m_pArray[i];
            if (m_panels.IndexOf(pPanel) != -1)
                pPanel->HandleInputEvent(nType, bHandled);
        }
    }

    int i = m_panels.m_count;
    while (--i >= 0)
    {
        CSWGuiPanel* pPanel = m_panels.m_pArray[i];
        unsigned short nFlags = pPanel->m_stateFlags;
#if KOTOR1  // K1's panel state bits sit one higher
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
#else
        if ((nFlags & PANEL_REMOVE_MASK) == 0)
            continue;
        if ((nFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE)
        {
            RemovePanel(pPanel);
            delete pPanel;
        }
        else
        {
            pPanel->m_stateFlags = nFlags & ~PANEL_REMOVE_STATE_MASK;
            RemovePanel(pPanel);
        }
#endif
    }

    if (g_pExoResMan->bMainMenuImageFileLoaded)
    {
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:MAINMENU"));
        g_pExoResMan->bMainMenuImageFileLoaded = 0;
    }
    if (g_pExoResMan->bChargenImageFileLoaded)
    {
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:CHARGEN"));
        g_pExoResMan->bChargenImageFileLoaded = 0;
    }
}

// Finds the topmost panel under the cursor and the control it hit. Both out-parameters are
// optional and each is null-checked independently. A modal panel swallows the click whether or
// not it was hit, and the normal stack is not consulted at all in that case.
//
// The modal path falls off the end of a non-void function, which is what lets the null checks
// survive.
int CSWGuiManager::HitCheckMouse(int x, int y, CSWGuiPanel** ppPanel, CSWGuiControl** ppControl,
                                 int nUnused)
{
    K2_UNUSED(nUnused);
    int bHit;

    if (m_modalPanels.m_count == 0)
    {
        for (int i = m_panels.m_count - 1; i >= 0; i--)
        {
            CSWGuiControl* pControl = m_panels.m_pArray[i]->HitCheckMouse(x, y);
            if (pControl)
            {
                if (ppPanel)
                    *ppPanel = m_panels.m_pArray[i];
                if (ppControl)
                    *ppControl = pControl;
                return 1;
            }
        }

        if (ppPanel)
            *ppPanel = 0;
        if (ppControl)
            *ppControl = 0;
        return 0;
    }

    {
        int nTop = m_modalPanels.m_count - 1;
        CSWGuiControl* pControl = m_modalPanels.m_pArray[nTop]->HitCheckMouse(x, y);
#ifdef K2_FIX_ENGINE_BUGS
        // Crash fix: otherwise the flag is left holding the hit control, nonzero whenever one was hit,
        // which is undefined behaviour and makes the whole modal path unreachable.
        bHit = pControl != 0;
#endif

        if (ppPanel)
        {
            if (pControl)
                *ppPanel = m_modalPanels.m_pArray[nTop];
            else
                *ppPanel = 0;
        }
        if (ppControl)
            *ppControl = pControl;
    }

    return bHit;
}

void CSWGuiManager::StoreCurrentMousePosition()
{
    m_storedMousePos = m_currentMousePos;
}

void CSWGuiManager::GetSafeMargin(int& xMargin, int& yMargin)
{
    xMargin = 5;
    yMargin = 5;
}

void CSWGuiExtent::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    CResStruct sExtent;
    if (pGFF->GetStructFromStruct(&sExtent, &resStruct, const_cast<char*>("EXTENT")))
    {
        int nSuccess;
        x = pGFF->ReadFieldINT(&sExtent, const_cast<char*>("LEFT"), nSuccess, 0);
        y = pGFF->ReadFieldINT(&sExtent, const_cast<char*>("TOP"), nSuccess, 0);
        w = pGFF->ReadFieldINT(&sExtent, const_cast<char*>("WIDTH"), nSuccess, 0);
        h = pGFF->ReadFieldINT(&sExtent, const_cast<char*>("HEIGHT"), nSuccess, 0);
#if KOTOR1 && !defined(K2_UI_TOUCH)
        K1ScaleLoadedExtent(*this, pGFF);
#endif
    }
}

// The two static rectangle helpers. Union is Intersection with every min/max flipped.
CSWGuiExtent CSWGuiExtent::Intersection(const CSWGuiExtent& a, const CSWGuiExtent& b)
{
    int nLeft = a.x > b.x ? a.x : b.x;
    int nTop = a.y > b.y ? a.y : b.y;
    int nWidth = (a.x + a.w < b.x + b.w ? a.x + a.w : b.x + b.w) - nLeft;
    int nHeight = (a.y + a.h < b.y + b.h ? a.y + a.h : b.y + b.h) - nTop;
    return CSWGuiExtent(nLeft, nTop, nWidth, nHeight);
}

CSWGuiExtent CSWGuiExtent::Union(const CSWGuiExtent& a, const CSWGuiExtent& b)
{
    int nLeft = a.x < b.x ? a.x : b.x;
    int nTop = a.y < b.y ? a.y : b.y;
    int nWidth = (a.x + a.w > b.x + b.w ? a.x + a.w : b.x + b.w) - nLeft;
    int nHeight = (a.y + a.h > b.y + b.h ? a.y + a.h : b.y + b.h) - nTop;
    return CSWGuiExtent(nLeft, nTop, nWidth, nHeight);
}
