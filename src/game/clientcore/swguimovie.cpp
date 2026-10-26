// The title screen's movies page.

#include "k2stdint.h"
#include "swguimovie.h"

#include "CExoArrayListImpl.h"

#include "twodimarray.h"
#include "appmanager.h"
#include "clientoptions.h"
#include "exobase.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "exoresman.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "nwsrules.h"
#include "exotlktable.h"
#include "twodimarrays.h"

#include <stdio.h>

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "restype.h"
#include "strref.h"

#if KOTOR1 && !defined(K2_UI_TOUCH)
#include "port/sdlapi.h"

// A row is 56 units of the 480-line console layout and the panel around it is scaled from 768
// lines to the screen (ScaleExtentForResolution), so a row needs both, as the save list's do.
static int K1MovieRowHeight()
{
    return (56 * 768 / 480 * g_SDL_mode.h + 767) / 768;   // rounded up, as the scaling does
}
#endif

const char* GetOverridePath(int nIndex, bool bFlag);

extern CTlkTable* g_pTlkTable;

// Adds a controller glyph to a button.
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

// The movie row height is scaled by the resolution text scalar.
float GetResolutionTextScalar();

// Loads the layout, binds the controls, colours the listbox prototype row, builds the movie list and
// activates the listbox.
CSWGuiTitleMovies::CSWGuiTitleMovies(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1 uses the "titlemovie" layout with three controls and a different prototype-row palette.
    StartLoadFromLayout(CResRef("titlemovie"));

    InitControl(m_listBox, CExoString("LB_MOVIES"), 1);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiButton* pTemplate = m_listBox.m_pProtoItem->AsButton();
    if (pTemplate != 0)
    {
        pTemplate->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        pTemplate->m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
        pTemplate->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    }

    m_btnBack.m_tail.bFocusOnHover = 0;

    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    PopulateMovies();

    SetActiveControl(&m_listBox, 0);

    IosAddButtonIcon(&m_btnBack, 'b');
#else
    StartLoadFromLayout(CResRef("titlemovie_p"), 1, 1, 1);

    InitControl(m_listBox, CExoString("LB_MOVIES"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aBars[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_labelUnlocked, CExoString("LBL_UNLOCKED"), 1, 1, 0, 0, 0);
    InitControl(m_labelUnlockedValue, CExoString("LBL_UNLOCKED_VALUE"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitLabel(&m_labelTitle.m_text);
    K2QolFitLabel(&m_labelUnlocked.m_text);
    K2QolFitLabel(&m_labelUnlockedValue.m_text);
#endif

    m_labelUnlocked.m_tail.bVisible = 1;
    m_labelUnlockedValue.m_tail.bVisible = 1;

    CSWGuiButton* pTemplate = m_listBox.m_pProtoItem->AsButton();
    if (pTemplate != 0)
    {
        pTemplate->m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        pTemplate->m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pTemplate->m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        pTemplate->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
    }

    m_btnBack.m_tail.bFocusOnHover = 0;

    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    PopulateMovies();

    SetActiveControl(&m_listBox, 0);

    AspyrAddButtonIcon(&m_btnBack, 'b');
#endif
}

// Scans the override directories, the movies directory and the content packages for movies, hides
// the ones the player has not unlocked and inserts each row by the Movies 2DA's Order column.
void CSWGuiTitleMovies::PopulateMovies()
{
#if KOTOR1
    // K1 has no override-directory scan; a "credits" row is always first and the Order sort starts
    // behind it, rows are unscaled, and there is no unlocked-count label.
    CExoString sName;
    CExoArrayList<CExoString> aDirs;
    CExoArrayList<CSWGuiControl*> aRows;
    int nThisOrder = 0;
    CSWGuiExtent extent = m_listBox.m_innerExtent;

    g_pExoBase->GetDirectoryList(&aDirs, CExoString("MOVIES:"), RESTYPE_BIK, 0, 0, 0);

    CExoString sLive;
    for (int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++)
    {
        sLive.Format(const_cast<char*>("live%d"), i);
        if (g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0).GetLength() < 1)
            continue;

        sLive = sLive + CExoString(":movies");
        g_pExoBase->GetDirectoryList(&aDirs, sLive, RESTYPE_BIK, 0, 0, 0);
    }

    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    int nNumDirs = aDirs.m_count;
    CExoString sCaption;

    CSWGuiButton* pCredits = new CSWGuiButton();
    pCredits->Initialize(extent, pProto);
    pCredits->m_text.m_params.SetText(g_pTlkTable->GetSimpleString(STRREF_CREDITS));
    pCredits->m_tail.m_nUserData = 0;
#ifndef K2_UI_TOUCH
    extent.h = K1MovieRowHeight();
#else
    extent.h = 56;
#endif
    pCredits->SetExtent(extent);
    pCredits->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));
    pCredits->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));
    aRows.Add(pCredits);
    m_aMovieNames.Add(CExoString("credits"));

    for (int i = 0; i < nNumDirs; i++)
    {
        sName = aDirs.m_pArray[i];
        if (sName.GetLength() == 0)
            continue;
        sName = sName.SubString(0, sName.GetLength() - 4);

        int nAlwaysShow;
        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("AlwaysShow"), &nAlwaysShow);
        if (nAlwaysShow != 1)
        {
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMovieShown(
                    CResRef(sName)) == 0)
            {
                continue;
            }
        }

        CSWGuiButton* pRow = new CSWGuiButton();
        pRow->Initialize(extent, pProto);

        int nStrrefName = 0;
        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("StrrefName"), &nStrrefName);
        if (nStrrefName != 0)
        {
            sCaption = g_pTlkTable->GetSimpleString(nStrrefName);
        }
        else
        {
            sCaption = sName;
        }
        pRow->m_text.m_params.SetText(sCaption);

        CExoString* pMovieName = new CExoString(sName);
        pRow->m_tail.m_nUserData = reinterpret_cast<unsigned long long>(pMovieName);

#ifndef K2_UI_TOUCH
        extent.h = K1MovieRowHeight();
#else
        extent.h = 56;
#endif
        pRow->SetExtent(extent);

        pRow->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));
        pRow->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));

        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("Order"), &nThisOrder);

        int nIndex = 1;
        for (; nIndex < aRows.m_count; nIndex++)
        {
            int nOrder = 0;
            g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
                m_aMovieNames.m_pArray[nIndex], CExoString("Order"), &nOrder);
            if (nOrder > nThisOrder)
            {
                break;
            }
        }

        aRows.Insert(pRow, nIndex);
        m_aMovieNames.Insert(sName, nIndex);
    }

    m_listBox.AddControls(aRows, 1, 0, 0);
#else
    CExoString sName;
    CExoArrayList<CExoString> aDirs;
    CExoArrayList<CSWGuiControl*> aRows;
    int nThisOrder = 0;
    CSWGuiExtent extent = m_listBox.m_innerExtent;

    int i = 0;
    const char* pszOverridePath;
    while ((pszOverridePath = GetOverridePath(i, false)) != 0)
    {
        char szDir[200];
        sprintf(szDir, "%s\\movies", pszOverridePath);
        g_pExoBase->GetDirectoryList(&aDirs, CExoString(szDir), RESTYPE_BIK, 0, 0);
        i++;
    }

    g_pExoBase->GetDirectoryList(&aDirs, CExoString("MOVIES:"), RESTYPE_BIK, 0, 0);

    CExoString sLive;
    for (int nPkg = 1; nPkg < CTlkTable::NUM_CONTENT_PKGS; nPkg++)
    {
        sLive.Format(const_cast<char*>("live%d"), nPkg);

        const char* pszAliasPath = g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0).m_sString;
        if (pszAliasPath == 0 || static_cast<int>(strlen(pszAliasPath)) < 1)
        {
            continue;
        }

        sLive = sLive + CExoString(":movies");
        g_pExoBase->GetDirectoryList(&aDirs, sLive, RESTYPE_BIK, 0, 0);
    }

    CExoString sCaption;
    int nCount = 0;
    int nNumDirs = aDirs.m_count;
    for (int nDir = 0; nDir < nNumDirs; nDir++)
    {
        sName = aDirs.m_pArray[nDir];
        sName = sName.SubString(0, (sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0) - 4);

        int nAlwaysShow;
        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("AlwaysShow"), &nAlwaysShow);
        if (nAlwaysShow != 1)
        {
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMovieShown(
                    CResRef(sName)) == 0)
            {
                continue;
            }
        }

        CSWGuiButton* pRow = new CSWGuiButton();
        pRow->Initialize(extent, static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem));

        int nStrrefName = 0;
        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("StrrefName"), &nStrrefName);
        if (nStrrefName != 0)
        {
            sCaption = g_pTlkTable->GetSimpleString(nStrrefName);
        }
        else
        {
            sCaption = sName;
        }
        pRow->m_text.m_params.SetText(sCaption);

        CExoString* pMovieName = new CExoString(sName);
        pRow->m_tail.m_nUserData = reinterpret_cast<unsigned long long>(pMovieName);

        extent.h = 56;
        extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
        pRow->SetExtent(extent);

        pRow->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));
        pRow->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiTitleMovies::OnMoviePicked));

        // This row's Order value, then the first accepted row that already sorts after it.
        g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
            sName, CExoString("Order"), &nThisOrder);

        int nIndex = 0;
        for (; nIndex < aRows.m_count; nIndex++)
        {
            int nOrder = 0;
            g_pRules->m_p2DArrays->m_pMovies->GetINTEntry(
                m_aMovieNames.m_pArray[nIndex], CExoString("Order"), &nOrder);
            if (nOrder > nThisOrder)
            {
                break;
            }
        }

        aRows.Insert(pRow, nIndex);
        m_aMovieNames.Insert(sName, nIndex);
        nCount++;
    }

    CExoString sLabel;
    sLabel.Format(const_cast<char*>("%d / %d"), nCount, g_pRules->m_p2DArrays->m_pMovies->m_nRows);
    m_labelUnlockedValue.m_text.m_params.SetText(sLabel);

    m_listBox.AddControls(aRows, 1, 0, 0);
#endif
}

// Plays the picked row's movie, or runs the credits sequence when the row carries no movie name.
void CSWGuiTitleMovies::OnMoviePicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiButton* pRow = pControl->AsButton();
        CExoString* pMovieName = reinterpret_cast<CExoString*>(pRow->m_tail.m_nUserData);

        if (pMovieName == 0)
        {
            g_pAppManager->m_pClientExoApp->StartCreditSequence(
                Vector(0.0f, 0.0f, 0.0f), CExoString("credits"));
        }
        else
        {
            int nResult = 0;
#if KOTOR1
            g_pAppManager->m_pClientExoApp->PlayMovie(*pMovieName, nResult, 0, 1);
#else
            g_pAppManager->m_pClientExoApp->PlayMovie(*pMovieName, nResult, 1, 1, 0);
#endif
        }
    }
}

// A zero parameter skips the dispatch entirely; every arm falls into the base handler.
void CSWGuiTitleMovies::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_START)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        }
        else if (type == INPUTEVENT_GUI_B || type == INPUTEVENT_GUI_BACK)
        {
            g_pAppManager->m_pClientExoApp->SetStartMenuMusicOnMovieEnd(1);
            g_pAppManager->m_pClientExoApp->StartMenuMusic(1, 127, 2);
#if !KOTOR1  // K1 has no main-menu idle timer
            g_pAppManager->m_pClientExoApp->EnableMainMenuIdleTimer();
#endif
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            SetToBeDeleted(0);
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}

// Frees the movie name each listbox row parked in its control tail.
CSWGuiTitleMovies::~CSWGuiTitleMovies()
{
    for (int i = 0; i < m_listBox.m_aControls.m_count; i++)
    {
        CExoString* pMovieName =
            reinterpret_cast<CExoString*>(m_listBox.GetControl(i)->AsButton()->m_tail.m_nUserData);
        if (pMovieName != 0)
            delete pMovieName;
    }
}
