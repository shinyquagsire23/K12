#include "k2stdint.h"
#include "swguiDebugLoadModule.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "exobase.h"
#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "exoresman.h"
#include "swguidrawable.h"
#include "exotlktable.h"
#include <stdio.h>
#include "restype.h"

const char* GetOverridePath(int nIndex, bool bFlag);

extern CExoString g_sBuildNumber;

#include "port/libcstring.h"
#include "exoinput.h"

CSWGuiLoadModuleDebugMenu::CSWGuiLoadModuleDebugMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_nSelectedModule = 0;
#if KOTOR1  // K1 uses the plain "debug" layout
    StartLoadFromLayout(CResRef("debug"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("debug_p"), 1, 1, 1);
#endif
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1  // K1: the panel is scaled to the screen
    ScaleExtentForResolution();
#endif
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);

    CExoArrayList<CSWGuiControl*> aOptions;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    CExoString sCaption;
    CExoString sDir;
    CExoString sFile;
    CExoArrayList<CExoString> aDirs;
    CExoArrayList<CExoString> aFiles;
    // Unused; the locals below are laid out around it.
    CExoString sUnused;

#if !KOTOR1  // K1 has no override-folder module lists
    int nOverride = 0;
    const char* pszOverride;
    char szPath[200];
    while ((pszOverride = GetOverridePath(nOverride, false)) != 0)
    {
        sprintf(szPath, "%s\\modules", pszOverride);
        g_pExoBase->GetDirectoryList(&aFiles, CExoString(szPath), RESTYPE_MOD, 0, 1);
        g_pExoBase->GetDirectoryList(&aDirs, CExoString(szPath), RESTYPE_RIM, 0, 1);
        nOverride++;
    }
#endif

    g_pExoBase->GetDirectoryList(&aFiles, CExoString("MODULES:"), RESTYPE_MOD, 0, 1);
    g_pExoBase->GetDirectoryList(&aDirs, CExoString("MODULES:"), RESTYPE_RIM, 0, 1);

    CExoString sLive;
    for (int nPkg = 1; nPkg < CTlkTable::NUM_CONTENT_PKGS; nPkg++)
    {
#if !KOTOR1
        const char* pszAliasPath;
#endif
        sLive.Format(const_cast<char*>("live%d"), nPkg);

#if KOTOR1  // K1: CExoString::GetLength is out of line
        if (g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0).GetLength() < 1)
#else
        pszAliasPath = g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0).m_sString;
        if (pszAliasPath == 0 || static_cast<int>(strlen(pszAliasPath)) < 1)
#endif
        {
            continue;
        }

        sLive = sLive + CExoString(":modules");
        g_pExoBase->GetDirectoryList(&aFiles, sLive, RESTYPE_MOD, 0, 1);
        g_pExoBase->GetDirectoryList(&aDirs, sLive, RESTYPE_RIM, 0, 1);
    }

    int nNumDirs = aDirs.m_count;
    int nNumFiles = aFiles.m_count;
    int i = 0;
    int j = 0;
    bool bDir = i < nNumDirs;
    bool bFile = j < nNumFiles;
    while (bDir || bFile)
    {
        if (bDir)
        {
            sDir = aDirs.m_pArray[i];
            if (sDir.Find(CExoString("_s.rim"), 0) != -1)
            {
                i++;
                bDir = i < nNumDirs;
                bFile = j < nNumFiles;
                continue;
            }
        }
        else
        {
            sDir = CExoString("zzzzzzzz.zzz");
        }

        if (bFile)
        {
            sFile = aFiles.m_pArray[j];
            if (sFile.Find(CExoString("_s.rim"), 0) != -1)
            {
                i++;
                bDir = i < nNumDirs;
                bFile = j < nNumFiles;
                continue;
            }
        }
        else
        {
            sFile = CExoString("zzzzzzzz.zzz");
        }

#if KOTOR1  // K1: CExoString::GetLength is out of line
        sDir = sDir.SubString(0, sDir.GetLength() - 4);
        sFile = sFile.SubString(0, sFile.GetLength() - 4);
#else
        sDir = sDir.SubString(0, (sDir.m_sString ? static_cast<int>(strlen(sDir.m_sString)) : 0) - 4);
        sFile = sFile.SubString(0, (sFile.m_sString ? static_cast<int>(strlen(sFile.m_sString)) : 0) - 4);
#endif

        if (sDir.LowerCase() <= sFile.LowerCase())
        {
            sCaption = sDir;
            i++;
            if (sDir == sFile)
                j++;
        }
        else
        {
            sCaption = sFile;
            j++;
        }

        aOptions.Add(CreateOption(sCaption,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLoadModuleDebugMenu::OnModulePicked),
            pTemplate));

        bDir = i < nNumDirs;
        bFile = j < nNumFiles;
    }

    m_listBox.AddControls(aOptions, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    CenterPanel();
}

void CSWGuiLoadModuleDebugMenu::Back()
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideLoadModuleDebugMenu();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDebugMenu();
}

void CSWGuiLoadModuleDebugMenu::OnModulePicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideLoadModuleDebugMenu();
        CExoString sModule = static_cast<CSWGuiButton*>(pControl)->m_text.m_params.m_sText;
        g_pAppManager->m_pServerExoApp->SetMoveToModuleString(sModule);
        g_pAppManager->m_pServerExoApp->SetMoveToModulePending(1);
    }
}

void CSWGuiLoadModuleDebugMenu::HandleInputEvent(int type, int val)
{
    if (val) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            Back();
            break;

        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            if (m_nSelectedModule > 0)
                m_nSelectedModule--;
            else if (m_nSelectedModule == 0)
                m_nSelectedModule = m_nNumModules - 1;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;

        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            if (m_nSelectedModule < m_nNumModules)
                m_nSelectedModule++;
            else if (m_nSelectedModule == m_nNumModules - 1)
                m_nSelectedModule = 0;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

CSWGuiButton* CSWGuiLoadModuleDebugMenu::CreateOption(const CExoString& sText,
                                  void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                  CSWGuiButton* pTemplate)
{
#if KOTOR1
    // K1 sizes each option to its own text, as CSWGuiPowersFeatsSkillsDebugMenu does: a zero extent,
    // then the text's ideal height plus twice the larger border.
    CSWGuiExtent extent = {0, 0, 0, 0};
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    int nText = static_cast<CSWGuiDrawable*>(&pButton->m_text)->GetIdealHeight();
    int nHilight = pButton->m_borderHilight.m_params.GetBorderDim();
    int nBorder = pButton->m_border.m_params.GetBorderDim();
    extent.h = nText + (nHilight > nBorder ? nHilight : nBorder) * 2;
    pButton->SetExtent(extent);
#else
    CSWGuiExtent extent = pTemplate->m_extent;
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
#endif
    pButton->AddEvent(INPUTEVENT_GUI_START, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_A, this, pMethod);
    return pButton;
}
