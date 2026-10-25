#include "k2stdint.h"
#include "port/sdlapi.h"
#define ASPYR_FLOATINGICON_CTOR_OUT_OF_LINE

#include "swguisaveload.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "serverexoappinternal.h"
#include "guiingame.h"
#include "partytables.h"
#include "globalvartables.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "swguiingame.h"
#include "nwchandlemessage.h"
#include "AndroidJNI.h"
#include "exotlktable.h"
#include "exobase.h"
#include "exoresman.h"
#include "CExoArrayListImpl.h"
#include "exosound.h"
#include "exofile.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "restype.h"
#if KOTOR1
extern unsigned int pressedGamepadButtons;
#endif

extern int g_bEnableLiveContentIgnore;

#include "Win32Find.h"

// Aspyr port entry points.
void DOS2MacPath(const char* sDosPath, char* sMacPath);
void IosCloudWrite(const char* sPath, const char* sName, unsigned int nSize);
void IosCloudDelete();
#if KOTOR1
void IosWriteCloud(const char* sPath, const char* sName, unsigned int nSize);
void GameMouseMove(bool bAbsolute, int nX, int nY);
void GameClick(int nX, int nY);
#endif

// Cloud-sync entry points.
bool IosCloudIsAvailable();
void SynchCloudSave();
CloudSynchStatus GetCloudSynchStatus();
void SetCloudSynchStatus(CloudSynchStatus eStatus);
const char* GetCloudStatusString();

extern bool g_cloudSaveChanged;
extern CExoSound* g_pExoSound;

#include "port/ios/IosLocalizedStrings.h"
#include "port/crtapi.h"
#include "exoinput.h"
#include "strref.h"
#include "exostringinline.h"

// The longest save name the name panel's edit box and the virtual keyboard take.
enum { SAVE_NAME_MAX_LENGTH = 16 };

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

extern CSWGuiSaveLoadEntry* pCloudSave;
#if !KOTOR1
CSWGuiSaveLoadEntry* pCloudSave;
#endif
#if KOTOR1
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
bool IosIcloudAvailable();
#endif

#if KOTOR1
CSWGuiSaveLoad::CSWGuiSaveLoad(CSWGuiManager* pGuiManager, int nSaveMode, int nExitMode)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("saveload"));

    InitControl(m_lblPanelName,  CExoString("LBL_PANELNAME"),  1);
    InitControl(m_lblScreenshot, CExoString("LBL_SCREENSHOT"), 1);
    InitControl(m_lblPlanetName, CExoString("LBL_PLANETNAME"), 1);
    InitControl(m_lblAreaName,   CExoString("LBL_AREANAME"),   1);
    InitControl(m_aPMLabels[0],  CExoString("LBL_PM1"),        1);
    InitControl(m_aPMLabels[1],  CExoString("LBL_PM2"),        1);
    InitControl(m_aPMLabels[2],  CExoString("LBL_PM3"),        1);
    InitControl(m_saveList,      CExoString("LB_GAMES"),       1);
    InitControl(m_btnSaveLoad,   CExoString("BTN_SAVELOAD"),   1);
    InitControl(m_btnBack,       CExoString("BTN_BACK"),       1);
    InitControl(m_btnDelete,     CExoString("BTN_DELETE"),     1);
    StopLoadFromLayout();

    if (IosIcloudAvailable()) {
        CSWGuiExtent extentCloud = m_lblScreenshot.m_extent;
        extentCloud.x = extentCloud.x + extentCloud.w - 117;
        extentCloud.w = 117;
        extentCloud.h = 77;
        m_btnCloud.SetExtent(extentCloud);
        m_btnCloud.m_text.m_params.SetText(CExoString(""));
        m_btnCloud.m_border.m_params.SetFillImage(CResRef("ipho_cloud"), 0);
        m_btnCloud.m_borderHilight.m_params.SetFillImage(CResRef("ipho_cloud2"), 0);
        m_btnCloud.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
        AddControl(&m_btnCloud);
        m_btnCloud.m_tail.bFocusOnHover = 0;
        m_btnCloud.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleCloudButton));
    }
    ScaleExtentForResolution();

    m_sResourceDir = "";
    m_saveList.bSelectOnLeftClick = 1;
    m_btnSaveLoad.m_tail.bFocusOnHover = 0;
    m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDelete.m_tail.bFocusOnHover = 0;
    m_btnDelete.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_btnDelete.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::DeleteGame));
    if (nSaveMode != 0)
        m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
    else
        m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::VerifyLoadGame));

    CSWGuiButton* pProtoRow = static_cast<CSWGuiButton*>(m_saveList.m_pProtoItem->AsButton());
    pProtoRow->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    pProtoRow->m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    pProtoRow->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_nModeFlags = static_cast<unsigned char>((m_nModeFlags & ~(SAVELOAD_MODE_SAVE | SAVELOAD_MODE_EXIT_MODULE)) | ((nSaveMode & 1) | ((nExitMode & 1) << 1)));
    m_nNextSaveSlot = 2;
    if ((nSaveMode & 1) == 0)
    {
        m_btnSaveLoad.m_text.m_params.SetStrRef(STRREF_LOAD);
        m_lblPanelName.m_text.m_params.SetStrRef(STRREF_LOAD_GAME);
    }
    else
    {
        m_btnSaveLoad.m_text.m_params.SetStrRef(STRREF_SAVE);
        m_lblPanelName.m_text.m_params.SetStrRef(STRREF_SAVE_GAME);
    }

    PopulateGameList();
    g_pExoResMan->AddResourceDirectory(CExoString("TEMP:"));
    SetActiveControl(&m_saveList, 0);

    int nScreenWidth;
    int nScreenHeight;
    if (IosIsIphone(nScreenWidth, nScreenHeight)) {
        CSWGuiExtent extent = m_lblScreenshot.m_extent;
        float fScreenAspect = static_cast<float>(nScreenWidth) / static_cast<float>(nScreenHeight);
        if (static_cast<float>(extent.w) / static_cast<float>(extent.h) != fScreenAspect) {
            int nHeight = static_cast<int>(static_cast<float>(extent.w) / fScreenAspect);
            int nY = extent.y + (extent.h - nHeight) / 2;
            extent.h = nHeight;
            extent.y = nY;
            m_lblScreenshot.SetExtent(extent);
            extent = m_btnCloud.m_extent;
            extent.y = nY;
            m_btnCloud.SetExtent(extent);
        }
    }

    IosAddButtonIcon(&m_btnDelete, 'x');
    IosAddButtonIcon(&m_btnSaveLoad, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
    if (IosIcloudAvailable())
        m_iconCloud.InitAndAdd(this, 'y', m_btnCloud.m_extent, -30, -30);
    if (nSaveMode == 0 && IosIcloudAvailable())
        SynchCloudSave();
}
#else
CSWGuiSaveLoad::CSWGuiSaveLoad(CSWGuiManager* pGuiManager, int nSaveMode, int nExitMode,
                               int nEndGameMode)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("saveload_p"), 1, 1, 1);

    // Layout binding order.
    InitControl(m_lblPanelName,  CExoString("LBL_PANELNAME"),  1, 1, 0, 0, 0);
    InitControl(m_lblScreenshot, CExoString("LBL_SCREENSHOT"), 1, 1, 0, 0, 0);
    InitControl(m_lblPlanetName, CExoString("LBL_PLANETNAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblAreaName,   CExoString("LBL_AREANAME"),   1, 1, 0, 0, 0);
    InitControl(m_aPMLabels[0],     CExoString("LBL_PM1"),        1, 1, 0, 0, 0);
    InitControl(m_aPMLabels[1],     CExoString("LBL_PM2"),        1, 1, 0, 0, 0);
    InitControl(m_aPMLabels[2],     CExoString("LBL_PM3"),        1, 1, 0, 0, 0);
    InitControl(m_saveList,      CExoString("LB_GAMES"),       1, 1, 0, 0, 0);
    InitControl(m_lblPCName,     CExoString("LBL_PCNAME"),     1, 1, 0, 0, 0);
    InitControl(m_lblTimePlayed, CExoString("LBL_TIMEPLAYED"), 1, 1, 0, 0, 0);
    InitControl(m_btnSaveLoad,   CExoString("BTN_SAVELOAD"),   1, 1, 0, 0, 0);
    InitControl(m_btnBack,       CExoString("BTN_BACK"),       1, 1, 0, 0, 0);
    InitControl(m_btnDelete,     CExoString("BTN_DELETE"),     1, 1, 0, 0, 0);
    InitControl(m_btnFilter,     CExoString("BTN_FILTER"),     1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[0],   CExoString("LBL_BAR1"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1],   CExoString("LBL_BAR2"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2],   CExoString("LBL_BAR3"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3],   CExoString("LBL_BAR4"),       1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // The cloud button, in LBL_SCREENSHOT's top-right corner.
    CSWGuiExtent extentCloud = m_lblScreenshot.m_extent;
    int nCloudW = static_cast<int>(static_cast<float>(extentCloud.w) / 3.0f);
    int nCloudH = static_cast<int>(static_cast<float>(extentCloud.h) / 2.5f);
    extentCloud.x += extentCloud.w - nCloudW;
    extentCloud.w = nCloudW;
    extentCloud.h = nCloudH;
    m_btnCloud.SetExtent(extentCloud);
    m_btnCloud.m_text.m_params.SetText(CExoString(""));
    m_btnCloud.m_border.m_params.SetFillImage(CResRef("ipho_cloud"), 0);
    m_btnCloud.m_borderHilight.m_params.SetFillImage(CResRef("ipho_cloud2"), 0);

    m_btnCloud.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    AddControl(&m_btnCloud);
    m_btnCloud.m_tail.bFocusOnHover = 0;
    m_btnCloud.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleCloudButton));
    m_btnCloud.m_tail.bVisible = 0;

    m_sResourceDir = "";
    m_saveList.bSelectOnLeftClick = 1;
    m_btnSaveLoad.m_tail.bFocusOnHover = 0;
    m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDelete.m_tail.bFocusOnHover = 0;
    m_btnDelete.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_btnDelete.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::DeleteGame));
    m_btnFilter.m_tail.bFocusOnHover = 0;
    m_btnFilter.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleFilterPress));
    if (nSaveMode != 0)
        m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
    else
        m_btnSaveLoad.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::VerifyLoadGame));

    AspyrAddButtonIcon(&m_btnSaveLoad, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDelete, 'x');
    AspyrAddButtonIcon(&m_btnFilter, 'd');

    // Every rebuilt list row is cloned from the list box's prototype.
    CSWGuiButton* pProtoRow = static_cast<CSWGuiButton*>(m_saveList.m_pProtoItem->AsButton());
    pProtoRow->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    // Bit 0 selects the save/load flavour, bit 1 records nExitMode.
    m_nModeFlags = static_cast<unsigned char>((m_nModeFlags & ~(SAVELOAD_MODE_SAVE | SAVELOAD_MODE_EXIT_MODULE)) | (nSaveMode & 1));
    m_nModeFlags = static_cast<unsigned char>(m_nModeFlags | ((nExitMode & 1) << 1));
    m_nFilterIndex = 0;
    m_nNextSaveSlot = 2;
    if ((nSaveMode & 1) == 0)
    {
        m_btnSaveLoad.m_text.m_params.SetStrRef(STRREF_LOAD);
        m_lblPanelName.m_text.m_params.SetStrRef(STRREF_LOAD_GAME);
    }
    else
    {
        m_btnSaveLoad.m_text.m_params.SetStrRef(STRREF_SAVE);
        m_lblPanelName.m_text.m_params.SetStrRef(STRREF_SAVE_GAME);
    }
    m_nReserved1 = 0;
    m_nCloudSyncState = CLOUD_SYNC_NONE;
    m_bEndGameMode = nEndGameMode;
    m_nModeFlags &= ~SAVELOAD_MODE_FILTERS_BUILT;

    PopulateGameList();
    // Screenshots live in the temp resource directory, mounted here and removed by the destructor.
    g_pExoResMan->AddResourceDirectory(CExoString("TEMP:"));
    SetActiveControl(&m_saveList, 0);

    // The cloud glyph, hidden until a sync is available.
    AspyrAddButtonIcon(&m_btnSaveLoad, 'a');
    m_iconCloud.InitAndAdd(this, 'y', m_btnCloud.m_extent, -7, -7);
    m_iconCloud.m_tail.bVisible = 0;
}
#endif

CSWGuiSaveLoad::~CSWGuiSaveLoad()
{
#if KOTOR1  // the rows belong to the list box; out-of-line IsEmpty
    g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));

    if (!m_sResourceDir.IsEmpty())
        g_pExoResMan->RemoveResourceDirectory(m_sResourceDir);
#else
    for (int64_t i = 0; i < m_entries.m_count; i++)
        delete m_entries.m_pArray[i];

    g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));

    if (m_sResourceDir.m_sString != NULL && m_sResourceDir.m_sString[0] != 0)
        g_pExoResMan->RemoveResourceDirectory(m_sResourceDir);
#endif
}

#if KOTOR1
void CSWGuiSaveLoad::PopulateGameList()
{
    CSWGuiSaveLoadEntry* pTemplate = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.m_pProtoItem);
    CExoArrayList<CExoString> fileList;
    CExoArrayList<CSWGuiControl*> aEntries;
    CSWGuiExtent extent(0, 0, 0, 42);
    CExoString unused;
    CSWGuiSaveLoadEntry* pCloud = 0;

    g_pExoBase->GetDirectoryList(&fileList, CExoString("SAVES:"), RESTYPE_INVALID, 1, 1, 0);
    if (g_pExoResMan->CreateDirectory2(CExoString("TEMP:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);

    for (int64_t i = 0; i < fileList.m_count; i++)
    {
        CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
        pEntry->Initialize(extent, pTemplate);

        int nScreenWidth;
        int nScreenHeight;
        if (IosIsIphone(nScreenWidth, nScreenHeight))
        {
#ifndef K2_UI_TOUCH
            // The 480-line layout units, scaled to 768.
            CSWGuiExtent rowExtent(pEntry->m_extent.x, pEntry->m_extent.y, pEntry->m_extent.w,
                                   pEntry->m_extent.h * 768 / 480);
#else
            CSWGuiExtent rowExtent(pEntry->m_extent.x, pEntry->m_extent.y, pEntry->m_extent.w, 60);
#endif
            pEntry->SetExtent(rowExtent);
            pEntry->ScaleExtentForResolution();
        }

        pEntry->LoadData(fileList.m_pArray[i]);
        pEntry->SetXboxTitle(fileList.m_pArray[i]);

        if ((pEntry->m_nSaveSlot & CSWGuiSaveLoadEntry::SAVESLOT_CLOUD) != 0)
        {
            pCloud = pEntry;
            if ((m_nModeFlags & SAVELOAD_MODE_SAVE) != 0)
            {
                delete pEntry;
                continue;
            }
        }

        if ((m_nModeFlags & SAVELOAD_MODE_SAVE) == 0)
        {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::VerifyLoadGame));
        }
        else if (pEntry->m_nSaveSlot < 2)
        {
            delete pEntry;
            continue;
        }
        else
        {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
        }

        pEntry->AddEvent(INPUTEVENT_GUI_X, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::DeleteGame));
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::ShowGame));

        if (IosIcloudAvailable())
            pEntry->AddEvent(INPUTEVENT_GUI_Y, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleCloudButton));

        aEntries.Add(pEntry);

        if (i == 0)
            ShowGame(pEntry);
    }

    unsigned int nNextSlot;
    if ((m_nModeFlags & SAVELOAD_MODE_SAVE) != 0)
    {
        unsigned int nMaxSlot = 0;
        nNextSlot = 2;
        for (int64_t i = 0; i < aEntries.m_count; i++)
        {
            unsigned int nSlot = static_cast<CSWGuiSaveLoadEntry*>(aEntries.m_pArray[i])->m_nSaveSlot;
            if (nSlot == nNextSlot)
                nNextSlot++;
            if (nMaxSlot < nSlot)
                nMaxSlot = nSlot;
        }

        CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
        pEntry->Initialize(extent, pTemplate);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
        {
            // Sized as the save rows above are.
            int nScreenWidth;
            int nScreenHeight;
            if (IosIsIphone(nScreenWidth, nScreenHeight))
            {
#ifndef K2_UI_TOUCH
                CSWGuiExtent rowExtent(pEntry->m_extent.x, pEntry->m_extent.y, pEntry->m_extent.w,
                                       pEntry->m_extent.h * 768 / 480);
#else
                CSWGuiExtent rowExtent(pEntry->m_extent.x, pEntry->m_extent.y, pEntry->m_extent.w, 60);
#endif
                pEntry->SetExtent(rowExtent);
                pEntry->ScaleExtentForResolution();
            }
        }
#endif
        pEntry->m_text.m_params.SetStrRef(STRREF_NEW_SLOT);
        int nSlot = static_cast<int>(nMaxSlot) + 1;
        pEntry->m_nSaveSlot = nSlot <= 2 ? 2 : nSlot;
        pEntry->m_bNewSlot = 1;
        pEntry->m_bEmptySlot = 1;
        pEntry->AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::ShowGame));
        int64_t i = aEntries.m_count;
        aEntries.Add(pEntry);
        if (static_cast<int>(i) > 0) {
            do {
                aEntries.m_pArray[i] = aEntries.m_pArray[i - 1];
                i--;
            } while (i > 0);
        }
        aEntries.m_pArray[0] = pEntry;
    }
    else
    {
        if (aEntries.m_count == 0)
        {
            // Nothing to load: close the panel with the "no saves" box.
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) != 0)
                g_pAppManager->m_pClientExoApp->DisplayMainMenu(0);
            else
                m_pGuiManager->PopModalPanel();

            CSWGuiMessageBox* pMsgBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pMsgBox->SetAllowCancel(0);
            pMsgBox->SetCallback(0, 0, 0);
            pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_NO_SAVED_GAMES));
            m_pGuiManager->AddPanel(pMsgBox, 1, 1);
            return;
        }
        nNextSlot = 2;
    }

    m_saveList.AddControls(aEntries, 1, 0, 0);
    m_nNextSaveSlot = nNextSlot;

    if (pCloud != 0)
    {
        for (int64_t i = 0; i < aEntries.m_count; i++)
        {
            CSWGuiSaveLoadEntry* pRow = static_cast<CSWGuiSaveLoadEntry*>(aEntries.m_pArray[i]);
            if (pRow->m_nTimePlayed == pCloud->m_nTimePlayed &&
                pRow->m_moduleName == pCloud->m_moduleName && pRow != pCloud)
            {
                pRow->m_border.m_params.SetFillImage(CResRef("ipho_mcloud"), 0);
                pRow->m_border.m_params.m_flags.nFillStyle = 2;
                pRow->m_borderHilight.m_params.SetFillImage(CResRef("ipho_mcloud"), 0);
                pRow->m_borderHilight.m_params.m_flags.nFillStyle = 2;
            }
        }
    }
}
#else
void CSWGuiSaveLoad::PopulateGameList()
{
    CSWGuiSaveLoadEntry* pTemplate = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.m_pProtoItem);
    CExoArrayList<CExoString> fileList;
    CSWGuiExtent extent(0, 0, 0, 36);
    CExoArrayList<CSWGuiControl*> aEntries;
    CExoString unused;

    pCloudSave = 0;

    g_pExoBase->GetDirectoryList(&fileList, CExoString("SAVES:"), RESTYPE_INVALID, 1, 1);
    if (g_pExoResMan->CreateDirectory2(CExoString("TEMP:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);

    // No saves on disk: add the cloud placeholder and start the sync.
    if (fileList.m_count == 0 && m_nCloudSyncState == CLOUD_SYNC_NONE)
    {
        fileList.Add(CExoString("Cloud0000"));
        m_nCloudSyncState = CLOUD_SYNC_STARTED;
    }
    else if (m_nCloudSyncState == CLOUD_SYNC_STARTED)
    {
        m_nCloudSyncState = CLOUD_SYNC_PENDING;
    }

    // Status box once a sync has finished: succeeded, failed or found no data.
    if (GetCloudSynchStatus() == CloudSynchStatus_Succeeded ||
        GetCloudSynchStatus() == CloudSynchStatus_Failed ||
        GetCloudSynchStatus() == CloudSynchStatus_NoData)
    {
        CSWGuiMessageBox* pMsgBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pMsgBox->SetAllowCancel(0);
        pMsgBox->SetAllowDismissal(1);
        pMsgBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::OnCloudSyncSuccess), 0);
        pMsgBox->SetMessage(CExoString(GetCloudStatusString()));
        m_pGuiManager->AddPanel(pMsgBox, 1, 1);
        m_pGuiManager->BringPanelToFront(pMsgBox);

        if (GetCloudSynchStatus() == CloudSynchStatus_Succeeded)
            m_nModeFlags &= ~SAVELOAD_MODE_FILTERS_BUILT;
        if (GetCloudSynchStatus() != CloudSynchStatus_Failed)
            SetCloudSynchStatus(CloudSynchStatus_Reported);
    }

    unsigned int nMaxSlot = 0;
    unsigned int nNextSlot = 2;

    if (fileList.m_count > 0)
    {
        for (int64_t i = 0; i < fileList.m_count; i++)
        {
            CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
            pEntry->Initialize(extent, pTemplate);
            pEntry->LoadData(fileList.m_pArray[i]);
            pEntry->SetXboxTitle(fileList.m_pArray[i]);

            // A cloud-backed row (slot bit 31) is remembered globally.
            if ((pEntry->m_nSaveSlot & CSWGuiSaveLoadEntry::SAVESLOT_CLOUD) != 0)
            {
                pCloudSave = pEntry;
                if ((m_nModeFlags & SAVELOAD_MODE_SAVE) != 0)
                    continue;
            }

            if ((m_nModeFlags & SAVELOAD_MODE_SAVE) == 0)
            {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this,
                    static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::VerifyLoadGame));
            }
            else if (pEntry->m_nSaveSlot < 2)
            {
                delete pEntry;
                continue;
            }
            else
            {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this,
                    static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
            }

            pEntry->AddEvent(INPUTEVENT_GUI_X, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::DeleteGame));
            pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::ShowGame));

            if (IosCloudIsAvailable())
                pEntry->AddEvent(INPUTEVENT_GUI_Y, this,
                    static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleCloudButton));

            // Rows are not collected while a sync is in flight.
            if (m_nCloudSyncState != CLOUD_SYNC_STARTED)
                aEntries.Add(pEntry);

            if (i == 0)
                ShowGame(pEntry);
        }
    }

    if ((m_nModeFlags & SAVELOAD_MODE_SAVE) != 0)
    {
        // The save screen always offers one extra empty slot.
        nMaxSlot = 0;
        nNextSlot = 2;
        for (int64_t i = 0; i < aEntries.m_count; i++)
        {
            unsigned int nSlot = static_cast<CSWGuiSaveLoadEntry*>(aEntries.m_pArray[i])->m_nSaveSlot;
            if (nSlot == nNextSlot)
                nNextSlot++;
            if (nMaxSlot < nSlot)
                nMaxSlot = nSlot;
        }

        CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
        pEntry->Initialize(extent, pTemplate);
        pEntry->m_text.m_params.SetStrRef(STRREF_NEW_SLOT);
        unsigned int nSlot = nMaxSlot + 1;
        pEntry->m_nSaveSlot = nSlot < 2 ? 2 : nSlot;
        pEntry->m_bNewSlot = 1;
        pEntry->m_bEmptySlot = 1;
        pEntry->AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleSaveButton));
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::ShowGame));
        aEntries.Insert(pEntry, 0);
    }
    else if (aEntries.m_count == 0)
    {
        if (m_nCloudSyncState != CLOUD_SYNC_STARTED)
        {
            // Nothing loadable and no sync pending: leave the screen.
            ExitPanel();

            CSWGuiMessageBox* pMsgBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pMsgBox->SetAllowCancel(0);
            pMsgBox->SetCallback(0, 0, 0);
            pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_NO_SAVED_GAMES));
            m_pGuiManager->AddPanel(pMsgBox, 1, 1);
            return;
        }

        nNextSlot = 2;
    }
    else
    {
        nNextSlot = 2;
    }

    if (aEntries.m_count >= 2)
        SortGamesByTimeStamp(aEntries);

    // Tab names come from the rows' PC names, deduplicated in first-seen order.
    if ((m_nModeFlags & SAVELOAD_MODE_FILTERS_BUILT) == 0)
    {
        CExoString sFilter;
        for (int64_t i = 0; i < aEntries.m_count; i++)
        {
            sFilter = CExoString(static_cast<CSWGuiSaveLoadEntry*>(aEntries.m_pArray[i])->m_pcName);
            if (sFilter.m_sString != 0 && *sFilter.m_sString != 0)
                m_filterNames.AddUnique(sFilter);
        }
    }

    // Show the party leader's own tab when it exists, otherwise insert it at the head.
    if ((m_nModeFlags & SAVELOAD_MODE_FILTERS_BUILT) == 0)
    {
        m_nFilterIndex = 0;
        if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) == 0)
        {
            if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
            {
                CExoString sPcName(
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->m_saveTableName);
                if (m_filterNames.Contains(sPcName) != 0)
                {
                    for (int64_t i = 0; i < m_filterNames.m_count; i++)
                    {
                        if (m_filterNames.m_pArray[i] == sPcName)
                        {
                            m_nFilterIndex = static_cast<int>(i);
                            break;
                        }
                    }
                }
                else
                {
                    m_filterNames.Insert(CExoString(sPcName), 0);
                }
            }
        }
        m_nModeFlags |= SAVELOAD_MODE_FILTERS_BUILT;
    }

    m_entries.m_count = 0;
    for (int64_t i = 0; i < aEntries.m_count; i++)
        m_entries.Add(static_cast<CSWGuiSaveLoadEntry*>(aEntries.m_pArray[i]));

    PopulateGameLB();
    m_nNextSaveSlot = nNextSlot;
}
#endif

#if !KOTOR1  // K2 only
static inline int SaveNameIsEmpty(const CExoString& s)
{
    return s.m_sString == 0 || *s.m_sString == 0;
}

void CSWGuiSaveLoad::PopulateGameLB()
{
    CExoArrayList<CSWGuiControl*> aControls;

    m_lblTimePlayed.m_text.m_params.SetText(CExoString(""));
    m_saveList.ClearItems();
    if (m_filterNames.m_count > 0)
        m_lblPCName.m_text.m_params.SetText(m_filterNames[m_nFilterIndex]);
    else
        m_lblPCName.m_text.m_params.SetText(CExoString(""));
    m_lblPlanetName.m_text.m_params.SetText(CExoString(""));
    m_lblAreaName.m_text.m_params.SetText(CExoString(""));

    // The party-member panels lose their border images while the list is rebuilt.
    int i;
    for (i = 0; i < 3; i++)
        m_aPMLabels[i].m_border.m_params.SetFillImage(CResRef(), 0);

    m_lblScreenshot.m_text.m_params.SetStrRef(STRREF_NEW_SLOT);
    m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(), 0);

    CSWGuiSaveLoadEntry* pEntry;
    for (i = 0; i < m_entries.m_count; i++)
    {
        pEntry = new CSWGuiSaveLoadEntry(m_entries.m_pArray[i]);
        if (m_filterNames.m_count > 0
            && !(CExoString(pEntry->m_pcName) == m_filterNames[m_nFilterIndex])
            && !SaveNameIsEmpty(CExoString(pEntry->m_pcName)))
        {
            delete pEntry;
            continue;
        }

        if (pEntry->m_bNewSlot)
            aControls.Insert(pEntry, 0);
        else
            aControls.Add(pEntry);
    }

    if (aControls.m_count > 0)
    {
        m_saveList.AddControls(aControls, 1, 0, 0);
        m_saveList.SetSelectedControl(m_saveList.GetControl(0), 0);
        SetActiveControl(&m_saveList, 0);
    }
    else if ((m_nModeFlags & SAVELOAD_MODE_SAVE) == 0)
    {
        // Not the save screen: the "no saves" browser prompt.
        m_lblScreenshot.m_text.m_params.SetStrRef(STRREF_NO_SAVES);
    }

    // The cloud row wears the port's cloud borders.
    if (pCloudSave != 0)
    {
        for (int nRow = 0; nRow < aControls.m_count; nRow++)
        {
            CSWGuiSaveLoadEntry* pRow = static_cast<CSWGuiSaveLoadEntry*>(aControls.m_pArray[nRow]);
            if (pRow->m_nTimePlayed == pCloudSave->m_nTimePlayed &&
                pRow->m_moduleName == pCloudSave->m_moduleName &&
                pRow->m_nSaveSlot != static_cast<unsigned int>(-2))
            {
                pRow->m_border.m_params.SetFillImage(CResRef("ipho_mcloud"), 0);
                pRow->m_border.m_params.m_flags.nFillStyle |= 3;
                pRow->m_borderHilight.m_params.SetFillImage(CResRef("ipho_mcloud"), 0);
                pRow->m_borderHilight.m_params.m_flags.nFillStyle |= 3;
            }
        }
        if ((m_nModeFlags & SAVELOAD_MODE_SAVE) != 0)
        {
            delete pCloudSave;
            pCloudSave = 0;
        }
    }
}
#endif

#if !KOTOR1  // K2 only
void CSWGuiSaveLoad::SetEndGameMode()
{
    m_bEndGameMode = 1;
}
#endif

void CSWGuiSaveLoad::FinishSave(CSWGuiControl* pControl) { K2_UNUSED(pControl); }

void CSWGuiSaveLoad::DisplayLoadError(uint64_t nStrRef) { K2_UNUSED(nStrRef); }

// The port's frame pump.
void UpdateScreen(float fDeltaTime, int a, int b);

// Step the load bar and pump one frame.
int LoadScreenUpdate(int current, int total, int rangeStart, int rangeWidth)
{
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(
        static_cast<int>(static_cast<float>(current) / static_cast<float>(total) * static_cast<float>(rangeWidth)) + rangeStart, 1);
    UpdateScreen(0.033333335f, 0, 0);
    return 0;
}

KOTOR_AUTOSAVE_PARAMS::KOTOR_AUTOSAVE_PARAMS()
{
    Reset();
}

// Each field is read with its current value as the default.
void KOTOR_AUTOSAVE_PARAMS::LoadFromGFF(CResGFF* pGff, CResStruct& pStruct)
{
    int nSuccess;
    CResStruct statusStruct;

    m_loadMusic = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("LOADMUSIC"), nSuccess, m_loadMusic);
    m_startWaypoint =
        pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("STARTWAYPOINT"), nSuccess, m_startWaypoint);
    m_movie1 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE1"), nSuccess, m_movie1);
    m_movie2 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE2"), nSuccess, m_movie2);
    m_movie3 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE3"), nSuccess, m_movie3);
    m_movie4 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE4"), nSuccess, m_movie4);
    m_movie5 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE5"), nSuccess, m_movie5);
    m_movie6 = pGff->ReadFieldCExoString(&pStruct, const_cast<char*>("MOVIE6"), nSuccess, m_movie6);

    unsigned int nYear = pGff->ReadFieldDWORD(&pStruct, const_cast<char*>("TIME_YEAR"), nSuccess, 0);
    unsigned char nMonth =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("TIME_MONTH"), nSuccess, 0));
    unsigned char nDay =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("TIME_DAY"), nSuccess, 0));
    unsigned char nHour =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("TIME_HOUR"), nSuccess, 0));
    unsigned short nMinute = pGff->ReadFieldWORD(&pStruct, const_cast<char*>("TIME_MINUTE"), nSuccess, 0);
    unsigned short nSecond = pGff->ReadFieldWORD(&pStruct, const_cast<char*>("TIME_SECOND"), nSuccess, 0);
    unsigned short nMillisecond =
        pGff->ReadFieldWORD(&pStruct, const_cast<char*>("TIME_MILLISECOND"), nSuccess, 0);

    m_year = nYear;
    m_month = nMonth;
    m_day = nDay;
    m_hour = nHour;
    m_minute = nMinute;
    m_second = nSecond;
    m_millisecond = nMillisecond;

    m_pauseDay = pGff->ReadFieldDWORD(&pStruct, const_cast<char*>("TIME_PAUSEDAY"), nSuccess, 0);
    m_pauseTime = pGff->ReadFieldDWORD(&pStruct, const_cast<char*>("TIME_PAUSETIME"), nSuccess, 0);

    if (pGff->GetStructFromStruct(&statusStruct, &pStruct, const_cast<char*>("STATUSSUMMARY")) != 0)
        m_statusSummary.LoadFromGFF(pGff, statusStruct);
}

#if KOTOR1  // K1's ctor calls Reset out of line
__attribute__((noinline))
#endif
void KOTOR_AUTOSAVE_PARAMS::Reset()
{
    m_startWaypoint = "";
    m_loadMusic = "";
    m_startWaypoint = "";
    m_movie1 = "";
    m_movie2 = "";
    m_movie3 = "";
    m_movie4 = "";
    m_movie5 = "";
    m_movie6 = "";

    m_year = 0;
    m_month = 0;
    m_day = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_millisecond = 0;
    m_pauseDay = 0;
    m_pauseTime = 0;

    m_statusSummary = CStatusSummary();
}

#if KOTOR1
void CSWGuiSaveLoad::LoadPCAutoSave(const CExoString& sModule)
{
    CExoString sSavesDir;
    CExoString sSaveName;
    CExoString sGamePath;
    CExoString sDeadSlot;

    g_pAppManager->m_pServerExoApp->UnloadModule();
    g_pAppManager->m_pServerExoApp->SetLoadingModule(1);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(5, 1);

    sSaveName.Format(const_cast<char*>("%06d - %s"), 1, "AUTOSAVE");

    sSavesDir = "SAVES:";
    sSavesDir = sSavesDir + sSaveName;

    if (g_pExoResMan->CreateDirectory2(CExoString("GAMEINPROGRESS:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("GAMEINPROGRESS:"), 0, 0);

    CERFFile* pErf = new CERFFile();
    sGamePath = sSavesDir + "/" + "SAVEGAME";
    pErf->Read(sGamePath);
    pErf->ReadHeaderVariance(sGamePath);

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 1);
    UpdateScreen(0.033333335f, 0, 0);
    pErf->ExportFilesFromERF(sGamePath, CExoString("GAMEINPROGRESS:"), static_cast<void*>(0), LoadScreenUpdate,
                             10, 10);

    g_pExoResMan->CreateDirectory2(CExoString("TEMP:"));
    sGamePath = sSavesDir + "/" + "pifo";
    g_pExoResMan->FileCopy(sGamePath, CExoString("TEMP:pifo"), RESTYPE_IFO, RESTYPE_INVALID);

    g_pAppManager->m_pClientExoApp->SetLoadFromIFO(1);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(25, 1);
    UpdateScreen(0.033333335f, 0, 0);
    g_pExoResMan->AddResourceDirectory(sSavesDir);
    g_pAppManager->m_pServerExoApp->GetPartyTable()->LoadTableInfo();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(30, 1);
    UpdateScreen(0.033333335f, 0, 0);
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->Load();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(35, 1);
    UpdateScreen(0.033333335f, 0, 0);

    CResGFF* pGff = new CResGFF(0, const_cast<char*>("NFO "), CResRef("savenfo"));
    CResStruct topStruct;
    pGff->GetTopLevelStruct(&topStruct);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(40, 1);
    UpdateScreen(0.033333335f, 0, 0);

    KOTOR_AUTOSAVE_PARAMS params;
    CResStruct autoSaveStruct;
    if (pGff->GetStructFromStruct(&autoSaveStruct, &topStruct, const_cast<char*>("AUTOSAVEPARAMS")) != 0)
        params.LoadFromGFF(pGff, autoSaveStruct);

    delete pGff;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(45, 1);
    UpdateScreen(0.033333335f, 0, 0);

    if (params.m_startWaypoint != "*")
        g_pAppManager->m_pServerExoApp->SetMoveToModuleStartWaypoint(params.m_startWaypoint);
    else
        g_pAppManager->m_pServerExoApp->SetMoveToModuleStartWaypoint(CExoString(""));

    g_pAppManager->m_pServerExoApp->SetMoveToWorldTime(
        params.m_year, params.m_month, params.m_day, params.m_hour, params.m_minute,
        params.m_second, params.m_millisecond);
    g_pAppManager->m_pServerExoApp->SetMoveToPausedTimeOfDay(params.m_pauseTime);
    g_pAppManager->m_pServerExoApp->SetMoveToPausedCalendarDay(params.m_pauseDay);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie1);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie2);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie3);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie4);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie5);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie6);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetStatusSummary(params.m_statusSummary);
    g_pAppManager->m_pClientExoApp->SetLoadMusic(params.m_loadMusic);

    g_pExoResMan->RemoveResourceDirectory(sSavesDir);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(50, 1);
    UpdateScreen(0.033333335f, 0, 0);

    m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 1);
    g_pAppManager->m_pClientExoApp->SetLoadStep(0, 10);
    g_pAppManager->m_pClientExoApp->SetLoadStep(1, 20);
    g_pAppManager->m_pClientExoApp->SetLoadStep(2, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(3, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(4, 23);
    g_pAppManager->m_pServerExoApp->SetDoingStartNewModule(1);
    g_pAppManager->m_pServerExoApp->ExecuteLoadModule(sModule);
}
#else
void CSWGuiSaveLoad::LoadPCAutoSave(const CExoString& sModule, bool bCloud)
{
    CExoString sSavesDir;
    CExoString sSaveName;
    CExoString sGamePath;
    CExoString sDeadSlot;
    CSWGuiPanel* pPanel;

    g_pAppManager->m_pServerExoApp->UnloadModule();
    g_pAppManager->m_pServerExoApp->SetLoadingModule(1);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(5, 1);

    if (bCloud)
        sSaveName.Format(const_cast<char*>("%06d - %s"), 2, "CLOUD");
    else
        sSaveName.Format(const_cast<char*>("%06d - %s"), 1, "AUTOSAVE");

    sSavesDir = "SAVES:";
    sSavesDir = sSavesDir + sSaveName;

    if (g_pExoResMan->CreateDirectory2(CExoString("GAMEINPROGRESS:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("GAMEINPROGRESS:"), 0, 0);

    CERFFile* pErf = new CERFFile();
    sGamePath = sSavesDir + "\\" + "SAVEGAME";
    pErf->Read(sGamePath);
    pErf->ReadHeaderVariance(sGamePath);

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 1);
    UpdateScreen(0.033333335f, 0, 0);
    pErf->ExportFilesFromERF(sGamePath, CExoString("GAMEINPROGRESS:"), reinterpret_cast<void*>(-1), LoadScreenUpdate,
                             10, 10);

    g_pExoResMan->CreateDirectory2(CExoString("TEMP:"));
    sGamePath = sSavesDir + "\\" + "pifo";
    g_pExoResMan->FileCopy(sGamePath, CExoString("TEMP:pifo"), RESTYPE_IFO, RESTYPE_INVALID);

    g_pAppManager->m_pClientExoApp->SetLoadFromIFO(1);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(25, 1);
    UpdateScreen(0.033333335f, 0, 0);
    g_pExoResMan->AddResourceDirectory(sSavesDir);
    g_pAppManager->m_pServerExoApp->GetPartyTable()->LoadTableInfo();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(30, 1);
    UpdateScreen(0.033333335f, 0, 0);
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->Load();
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(35, 1);
    UpdateScreen(0.033333335f, 0, 0);

    CResGFF* pGff = new CResGFF(0, const_cast<char*>("NFO "), CResRef("savenfo"));
    CResStruct topStruct;
    pGff->GetTopLevelStruct(&topStruct);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(40, 1);
    UpdateScreen(0.033333335f, 0, 0);

    KOTOR_AUTOSAVE_PARAMS params;
    CResStruct autoSaveStruct;
    if (pGff->GetStructFromStruct(&autoSaveStruct, &topStruct, const_cast<char*>("AUTOSAVEPARAMS")) != 0)
        params.LoadFromGFF(pGff, autoSaveStruct);

    delete pGff;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(45, 1);
    UpdateScreen(0.033333335f, 0, 0);

    if (params.m_startWaypoint != "*")
        g_pAppManager->m_pServerExoApp->SetMoveToModuleStartWaypoint(params.m_startWaypoint);
    else
        g_pAppManager->m_pServerExoApp->SetMoveToModuleStartWaypoint(CExoString(""));

    g_pAppManager->m_pServerExoApp->SetMoveToWorldTime(
        params.m_year, params.m_month, params.m_day, params.m_hour, params.m_minute,
        params.m_second, params.m_millisecond);
    g_pAppManager->m_pServerExoApp->SetMoveToPausedTimeOfDay(params.m_pauseTime);
    g_pAppManager->m_pServerExoApp->SetMoveToPausedCalendarDay(params.m_pauseDay);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie1);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie2);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie3);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie4);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie5);
    g_pAppManager->m_pClientExoApp->AddMoveToModuleMovie(params.m_movie6);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetStatusSummary(params.m_statusSummary);
    g_pAppManager->m_pClientExoApp->SetLoadMusic(params.m_loadMusic);

    g_pExoResMan->RemoveResourceDirectory(sSavesDir);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(50, 1);
    UpdateScreen(0.033333335f, 0, 0);

    pPanel = this;
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(10, 1);
    g_pAppManager->m_pClientExoApp->SetLoadStep(0, 10);
    g_pAppManager->m_pClientExoApp->SetLoadStep(1, 20);
    g_pAppManager->m_pClientExoApp->SetLoadStep(2, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(3, 23);
    g_pAppManager->m_pClientExoApp->SetLoadStep(4, 23);
    g_pAppManager->m_pServerExoApp->SetDoingStartNewModule(1);
    g_pAppManager->m_pServerExoApp->ExecuteLoadModule(sModule);
}
#endif

// Mirrors CGuiInGame::UnpackQuickSaveGame.
bool CSWGuiSaveLoad::UnpackGame(unsigned int nSlot, const CExoString& sModule, CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CopyGameToFutureGame(nSlot, sModule);

    CExoString gameInProgressPath =
        g_pExoBase->m_pAliasList->ResolveFileName("GAMEINPROGRESS:", RESTYPE_INVALID);
    CExoString futureGamePath = g_pExoBase->m_pAliasList->ResolveFileName("FUTUREGAME:", RESTYPE_INVALID);

    g_pExoResMan->NukeDirectory("GAMEINPROGRESS:", 1, 1);

    futureGamePath = futureGamePath.Left(
        futureGamePath.GetLength() - 1);
    gameInProgressPath = gameInProgressPath.Left(
        gameInProgressPath.GetLength() - 1);

#if KOTOR1
    static_cast<void>(_rename(futureGamePath.CStr(), gameInProgressPath.CStr()));
#else
    static_cast<void>(rename(futureGamePath.CStr(), gameInProgressPath.CStr()));
#endif

    return true;
}

// Mirrors CGuiInGame::CopyQuickSaveGameToFutureGame.
bool CSWGuiSaveLoad::CopyGameToFutureGame(unsigned int a, const CExoString& b)
{
    CExoString saveSlot;
    CExoString saveName;
    CExoString saveGameName;
    CExoString unused;

    if (a == 0xffffffff)
        saveName = b;
    else
        saveName.Format(const_cast<char*>("%06d - %s"), a, b.CStr());

    saveSlot = "SAVES:";
    saveSlot = saveSlot + saveName;

    g_pExoResMan->CreateDirectory2(CExoString("FUTUREGAME:"));
    g_pExoResMan->CleanDirectory(CExoString("FUTUREGAME:"), 0, 0);

#if KOTOR1  // K1: "/" separator, null export filter
    saveGameName = saveSlot + "/" + "SAVEGAME";
#else
    saveGameName = saveSlot + "\\" + "SAVEGAME";
#endif

    CERFFile* pErf = new CERFFile();
    pErf->Read(saveGameName);
    pErf->ReadHeaderVariance(saveGameName);
#if KOTOR1
    pErf->ExportFilesFromERF(saveGameName, CExoString("FUTUREGAME:"), static_cast<void*>(0), LoadScreenUpdate, 0,
                             g_pAppManager->m_pClientExoApp->GetLoadStep(0) & 0xff);
#else
    pErf->ExportFilesFromERF(saveGameName, CExoString("FUTUREGAME:"), reinterpret_cast<void*>(-1), LoadScreenUpdate, 0,
                             g_pAppManager->m_pClientExoApp->GetLoadStep(0) & 0xff);
#endif
    pErf->~CERFFile();
    operator delete(pErf);

    return true;
}

void CSWGuiSaveLoad::WriteGame(const CExoString& sModule)
{
#if !KOTOR1
    CSWGuiPanel* pPanel;
#endif
    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.GetSelectedControl());

    if (pEntry->m_nSaveSlot >= 1000) {
        // No room left in the 0..999 slot space.
        if (m_nNextSaveSlot >= 1000) {
            CSWGuiMessageBox* pMsgBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            CExoString sMsg = g_pTlkTable->GetSimpleString(STRREF_MAX_SAVES_REACHED);
            g_pTlkTable->ParseStr(sMsg);
            pMsgBox->SetAllowCancel(0);
            pMsgBox->SetMessage(sMsg);
            m_pGuiManager->AddPanel(pMsgBox, 1, 1);
            return;
        }
        pEntry->m_nSaveSlot = m_nNextSaveSlot;
    }

    // Send only when the row has no save, or the confirm box was accepted.
    if (pEntry->m_bSaveExists == 0 ||
        (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted & 1) != 0) {
        CExoString sName;
        sName.Format(const_cast<char*>("Game%d"), pEntry->m_nSaveSlot - 1);
#if KOTOR1
        g_pAppManager->m_pClientExoApp->SendSaveGameRequest(pEntry->m_nSaveSlot, sName, sModule);

        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_saveNumber = pEntry->m_nSaveSlot;
        g_pAppManager->m_pClientExoApp->SendSaveGameRequest(pEntry->m_nSaveSlot, sName, sModule);

        pPanel = this;
        do {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif

        m_pGuiManager->PopModalPanel();
    }
}

// Seed the name panel from the row's own name, or the empty string for a fresh slot.
void CSWGuiSaveLoad::PromptForSaveName(CSWGuiControl* pControl)
{
    CExoString sName;

    if (pControl != 0) {
        if (static_cast<CSWGuiSaveLoadEntry*>(pControl)->m_bSaveExists == 0) {
            sName = CExoString("");
        } else {
            if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
                return;
            sName = static_cast<CSWGuiSaveLoadEntry*>(pControl)->m_saveGameName;
        }

        CSWGuiSaveNamePanel* pPanel = new CSWGuiSaveNamePanel(m_pGuiManager);
        pPanel->m_pSaveLoad = this;
        pPanel->m_editBox.m_editText.SetText(sName);
        g_pGuiMan->AddPanel(pPanel, 1, 1);
#if KOTOR1  // K1: a Chromebook gets the soft keyboard
        if (SDL_IsChromebook())
            SDL_StartTextInput();
#endif
    }
}

CExoString CSWGuiSaveLoadEntry::GetGameDirectory()
{
    CExoString sResult;
    sResult.Format(const_cast<char*>("SAVES:%06d - %s"), m_nSaveSlot, m_saveName.CStr());
    return sResult;
}

// Delete the selected save's directory and re-seat the selection.
void CSWGuiSaveLoad::RemoveGame(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.GetSelectedControl());
    if (pEntry == 0)
        return;

    // A cloud-backed save gets its cloud copy dropped too.
    if (pEntry->m_nSaveSlot & CSWGuiSaveLoadEntry::SAVESLOT_CLOUD)
        IosCloudDelete();

    g_pExoResMan->NukeDirectory(pEntry->GetGameDirectory(), 1, 1);

    // Re-seat on the old index, clamped to the new last row.
    int nSelected = m_saveList.m_nSelectedIndex;
    PopulateGameList();
    int nLast = m_saveList.m_aControls.m_count - 1;
#if KOTOR1
    m_saveList.SetSelectedControl(nLast > nSelected ? nSelected : nLast, 0);
#else
    m_saveList.SetSelectedControl(nLast < nSelected ? nLast : nSelected, 0);
#endif

    SetActiveControl(0, 0);
    SetActiveControl(&m_saveList, 0);
}

// Re-list when the cloud flag went up, preserving the selected row.
void CSWGuiSaveLoad::Update(float f)
{
    K2_UNUSED(f);
#if KOTOR1  // K1: the test-and-relist runs under g_UI_mutex
    SDL_LockMutex(g_UI_mutex);
    if (g_cloudSaveChanged) {
        g_cloudSaveChanged = 0;

        int nSelected = m_saveList.m_nSelectedIndex;
        PopulateGameList();
        int nLast = m_saveList.m_aControls.m_count - 1;
        m_saveList.SetSelectedControl(nLast > nSelected ? nSelected : nLast, 0);
    }
    SDL_UnlockMutex(g_UI_mutex);
#else
    if (g_cloudSaveChanged) {
        g_cloudSaveChanged = 0;

        int nSelected = m_saveList.m_nSelectedIndex;
        PopulateGameList();
        int nLast = m_saveList.m_aControls.m_count - 1;
        m_saveList.SetSelectedControl(nLast < nSelected ? nLast : nSelected, 0);
    }
#endif
}

// nResult 1 means OK; only then is the typed name committed.
void CSWGuiSaveLoad::SaveNamePanelClose(int nResult, CExoString sName)
{
    if (nResult == 1)
        WriteGame(sName);
}

// Upload the selected save to the cloud.
#if KOTOR1
void CSWGuiSaveLoad::HandleCloudButton2(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    GameMouseMove(true, 0, 0);
    GameClick(0, 0);

    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.GetSelectedControl());
    if (pEntry == 0)
        return;
    if (pEntry->m_bNewSlot)
        return;

    CExoString sUnusedName(pEntry->m_saveName);

    CExoString sResolved = g_pExoBase->m_pAliasList->ResolveFileName(pEntry->GetGameDirectory(), RESTYPE_INVALID);

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadScreenByModuleName(CExoString("default"));
    pClientExoApp->ShowLoadScreen(0, 0, 1);
    pClientExoApp->SetLoadScreenHint(pClientExoApp->GetNextLoadScreenHintSTRREF(CExoString(""), 2));
    pClientExoApp->SetLoadScreenHintUpdateTimer(0);
    pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    IosWriteCloud(sResolved.CStr(), pEntry->m_saveGameName.CStr(), pEntry->m_nTimePlayed);

    pClientExoApp->HideLoadScreen(0, 1);
}
#else
void CSWGuiSaveLoad::HandleCloudButton2(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.GetSelectedControl());
    if (pEntry == 0)
        return;
    if (pEntry->m_bNewSlot)
        return;

    CExoString sUnusedName(pEntry->m_saveName);

    CExoString sResolved = g_pExoBase->m_pAliasList->ResolveFileName(pEntry->GetGameDirectory(), RESTYPE_INVALID);

    char sMacPath[1024];
    DOS2MacPath(sResolved.CStr(), sMacPath);

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadScreenByModuleName(CExoString("default"));
    pClientExoApp->ShowLoadScreen(0, 0, 1, 1);
    pClientExoApp->SetLoadScreenHint(pClientExoApp->GetNextLoadScreenHintSTRREF(CExoString(""), 2));
    pClientExoApp->SetLoadScreenHintUpdateTimer(0);
    pClientExoApp->GetSWGuiManager()->Draw(0.033333335f);
    UpdateScreen(0.033333335f, 0, 0);

    IosCloudWrite(sMacPath, pEntry->m_saveGameName.CStr(), pEntry->m_nTimePlayed);

    pClientExoApp->HideLoadScreen(0, 1, 1);
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
}
#endif

CResRef CSWGuiSaveLoadEntry::GetPlayerPortrait(int nIndex)
{
    if (nIndex >= 0 && nIndex <= 2)
        return m_aPlayerPortrait[nIndex];
    return CResRef();
}

#if !KOTOR1  // K2 only: K1 rows show the time played, not a date
// Split the saved timestamp into its date and time fields and format each one.
CExoString CSWGuiSaveLoadEntry::FormatTimeToString()
{
    unsigned long long timeStamp = m_timeStamp;
    if (timeStamp == 0) {
        CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
        if (pServerExoApp == 0)
            return CExoString("");
        timeStamp = pServerExoApp->GetSystemTimeAsDWORD64();
        m_timeStamp = timeStamp;
    }

    FILETIME fileTime;
    fileTime.dwLowDateTime = static_cast<unsigned int>(timeStamp);
    fileTime.dwHighDateTime = static_cast<unsigned int>(timeStamp >> 32);
    SYSTEMTIME sysTime;
    FileTimeToSystemTime(&fileTime, &sysTime);

    CExoString sDay;
    CExoString sYear;
    CExoString sHour;
    CExoString sMinute;
    CExoString sSecond;
    CExoString sMonth;

    switch (sysTime.wMonth) {
    case 1:  sMonth = "Jan"; break;
    case 2:  sMonth = "Feb"; break;
    case 3:  sMonth = "Mar"; break;
    case 4:  sMonth = "Apr"; break;
    case 5:  sMonth = "May"; break;
    case 6:  sMonth = "Jun"; break;
    case 7:  sMonth = "Jul"; break;
    case 8:  sMonth = "Aug"; break;
    case 9:  sMonth = "Sep"; break;
    case 10: sMonth = "Oct"; break;
    case 11: sMonth = "Nov"; break;
    case 12: sMonth = "Dec"; break;
    default: sMonth.Format(const_cast<char*>("%.2d"), sysTime.wMonth); break;
    }

    sYear.Format(const_cast<char*>("%.4d"), sysTime.wYear);
    sDay.Format(const_cast<char*>("%.2d"), sysTime.wDay);
    sHour.Format(const_cast<char*>("%.2d"), sysTime.wHour);
    sMinute.Format(const_cast<char*>("%.2d"), sysTime.wMinute);
    sSecond.Format(const_cast<char*>("%.2d"), sysTime.wSecond);

    CExoString sResult = g_pTlkTable->GetSimpleString(STRREF_SAVE_TIMESTAMP_FORMAT);
    g_pTlkTable->SetCustomToken(0, sMonth);
    g_pTlkTable->SetCustomToken(1, sDay);
    g_pTlkTable->SetCustomToken(2, sYear);
    g_pTlkTable->SetCustomToken(3, sHour);
    g_pTlkTable->SetCustomToken(4, sMinute);
    g_pTlkTable->SetCustomToken(5, sSecond);
    g_pTlkTable->ParseStr(sResult);
    return sResult;
}
#endif

float GetResolutionTextScalar();

// Row caption with the slot title, time, cloud and modded markers.
#if KOTOR1
void CSWGuiSaveLoadEntry::SetXboxTitle(const CExoString& sTitle)
{
    m_sTitle = sTitle;

    CExoString sUnused;

    if (m_nSaveSlot == 1)
        m_sTitle = g_pTlkTable->GetSimpleString(STRREF_AUTO_SAVE);
    else if (m_nSaveSlot == 0)
        m_sTitle = g_pTlkTable->GetSimpleString(STRREF_QUICK_SAVE);

    CExoString sTlk;
    unsigned int nHours = static_cast<unsigned int>(m_nTimePlayed) / 3600;
    unsigned int nMinutes = static_cast<unsigned int>(m_nTimePlayed) / 60 % 60;

    if (m_nSaveSlot <= 1)
    {
        g_pTlkTable->SetCustomToken(0, CExoString(nHours));
        g_pTlkTable->SetCustomToken(1, CExoString(nMinutes));
        sTlk = g_pTlkTable->GetSimpleString(STRREF_SAVE_PLAY_TIME);
        g_pTlkTable->ParseStr(sTlk);
        m_sTitle = m_sTitle + sTlk;
    }
    else
    {
        g_pTlkTable->SetCustomToken(0, CExoString(m_nSaveSlot - 1));
        g_pTlkTable->SetCustomToken(1, CExoString(nHours));
        g_pTlkTable->SetCustomToken(2, CExoString(nMinutes));
        g_pTlkTable->SetCustomToken(3, m_saveGameName);
        sTlk = g_pTlkTable->GetSimpleString(STRREF_SAVE_ENTRY_TITLE);
        g_pTlkTable->ParseStr(sTlk);
        m_sTitle = sTlk;
    }

    if ((m_nSaveSlot & CSWGuiSaveLoadEntry::SAVESLOT_CLOUD) != 0)
        m_sTitle = "CLOUD SAVE";

    m_text.m_params.SetText(m_sTitle);
}
#else
void CSWGuiSaveLoadEntry::SetXboxTitle(const CExoString& sTitle)
{
    m_sTitle = sTitle;

    CExoString sUnused;

    if (m_nSaveSlot == 1)
    {
        m_sTitle = g_pTlkTable->GetSimpleString(STRREF_AUTO_SAVE);
    }
    else if (m_bCorrupt != 0)
    {
        m_sTitle = sTitle;
    }
    else if (m_nSaveSlot == 0)
    {
        m_sTitle = g_pTlkTable->GetSimpleString(STRREF_QUICK_SAVE);
    }

    CExoString sTlk;

    if (m_nSaveSlot <= 1)
    {
        m_sTitle = m_sTitle + "\n" + FormatTimeToString();
    }
    else
    {
        CExoString sTime = FormatTimeToString();
        g_pTlkTable->SetCustomToken(0, CExoString(m_nSaveNumber));
        g_pTlkTable->SetCustomToken(1, m_saveGameName);
        g_pTlkTable->SetCustomToken(2, sTime);
        sTlk = g_pTlkTable->GetSimpleString(STRREF_SAVE_ENTRY_TITLE);
        g_pTlkTable->ParseStr(sTlk);
        m_sTitle = sTlk;
    }

    if ((m_nSaveSlot & CSWGuiSaveLoadEntry::SAVESLOT_CLOUD) != 0)
        m_sTitle = "CLOUD SAVE";

    m_text.m_params.SetText(m_sTitle);

    if (m_bModded != 0)
        m_text.m_params.SetText(CExoString("[Modded] ") + m_sTitle);

    if (m_bModded != IsGameModded())
        m_text.m_params.SetDefaultColor(CGuiInGame::DISABLED_TEXT);
}
#endif

CSWGuiSaveLoadEntry::CSWGuiSaveLoadEntry()
#if KOTOR1
{
    m_gamePlayHint = 0;
    m_storyHints[0] = 0;
    m_nSaveSlot = 0;
    m_nTimePlayed = 0;

    m_bSaveExists = 0;
    m_bCorrupt = 0;
    m_bNewSlot = 0;
    m_bRebootAutoSave = 0;
    m_bPCAutoSave = 0;
    m_bEmptySlot = 0;
    m_bCheatUsed = 0;
    m_nBlocksShort = 0;

    m_saveGameName = "Bad";
}
#else
    : m_bCloudSave(0)
{
    m_gamePlayHint = 0;
    for (int i = 0; i < 10; i++)
        m_storyHints[i] = 0;

    m_nSaveSlot = 0;
    m_nTimePlayed = 0;
    m_nBlocksShort = 0;

    m_bSaveExists = 0;
    m_bCorrupt = 0;
    m_bNewSlot = 0;
    m_bRebootAutoSave = 0;
    m_bPCAutoSave = 0;
    m_bEmptySlot = 0;
    m_bCheatUsed = 0;
    m_bModded = 0;

    m_saveGameName = "Bad";
    m_nSaveNumber = 0;
    m_timeStamp = 0;
}
#endif

#if !KOTOR1  // K2 only
CSWGuiSaveLoadEntry::CSWGuiSaveLoadEntry(CSWGuiSaveLoadEntry* pOther)
    : m_bCloudSave(0)
{
    CSWGuiExtent extent(0, 0, 0, 36);
    extent.h = static_cast<int>(extent.h * GetResolutionTextScalar());
    Initialize(extent, pOther);

    m_gamePlayHint = pOther->m_gamePlayHint;
    for (int i = 0; i < 10; i++)
        m_storyHints[i] = pOther->m_storyHints[i];

    m_bSaveExists = pOther->m_bSaveExists;
    m_bRebootAutoSave = pOther->m_bRebootAutoSave;
    m_bPCAutoSave = pOther->m_bPCAutoSave;
    m_nSaveSlot = pOther->m_nSaveSlot;
    m_nTimePlayed = pOther->m_nTimePlayed;
    m_bNewSlot = pOther->m_bNewSlot;
    m_nBlocksShort = pOther->m_nBlocksShort;
    m_bCorrupt = pOther->m_bCorrupt;
    m_bEmptySlot = pOther->m_bEmptySlot;
    m_bCheatUsed = pOther->m_bCheatUsed;
    m_saveGameName = pOther->m_saveGameName;
    m_bModded = pOther->m_bModded;
    m_bLiveContent = pOther->m_bLiveContent;
    m_saveName = pOther->m_saveName;
    m_areaName = pOther->m_areaName;
    m_moduleName = pOther->m_moduleName;
    m_sReserved1 = pOther->m_sReserved1;
    m_sTitle = pOther->m_sTitle;
    m_screenshot = pOther->m_screenshot;

    for (int i = 0; i < 3; i++)
        m_aPlayerPortrait[i] = CResRef(pOther->m_aPlayerPortrait[i]);

    m_timeStamp = pOther->m_timeStamp;
    m_pcName = CExoString(pOther->m_pcName);
    m_nSaveNumber = pOther->m_nSaveNumber;
    m_bCloudSave = (pOther->m_bCloudSave != 0);

    CSWGuiObject* pObject = 0;
    void (CSWGuiObject::*pFunc)(CSWGuiControl*) = 0;

    pOther->GetEvent(INPUTEVENT_GUI_A, &pObject, &pFunc);
    if (pObject && pFunc)
        AddEvent(INPUTEVENT_GUI_A, pObject, pFunc);
    pOther->GetEvent(INPUTEVENT_GUI_X, &pObject, &pFunc);
    if (pObject && pFunc)
        AddEvent(INPUTEVENT_GUI_X, pObject, pFunc);
    pOther->GetEvent(0, &pObject, &pFunc);
    if (pObject && pFunc)
        AddEvent(SWGUIEVENT_FOCUS, pObject, pFunc);
    pOther->GetEvent(INPUTEVENT_GUI_Y, &pObject, &pFunc);
    if (pObject && pFunc)
        AddEvent(INPUTEVENT_GUI_Y, pObject, pFunc);
}
#endif


bool IsJoystickConnected();
extern int gDontMoveCursor_Mac;
void ASLPlat_ShowVirtualKeyboard(const char* sText, unsigned int nMaxLength);
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
void IosAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);

#if KOTOR1
CSWGuiSaveNamePanel::CSWGuiSaveNamePanel(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_pSaveLoad = 0;
    StartLoadFromLayout(CResRef("savename"), 1, 1, 1);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_editBox, CExoString("EDITBOX"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    StopLoadFromLayout();

    m_lblIcon = m_lblTitle;

    CSWGuiExtent extent = m_editBox.m_extent;
    extent.x -= 200;
    m_lblIcon.SetExtent(extent);
    m_lblIcon.m_text.m_params.SetText(CExoString());
    m_lblIcon.m_tail.bIgnoreMouse = 1;
    AddControl(&m_lblIcon);
    IosAddLabelIcon(&m_lblIcon, 'x');
    IosAddButtonIcon(&m_btnOk, 'a');
    IosAddButtonIcon(&m_btnCancel, 'b');
    ScaleExtentForResolution();
    CenterPanel();

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveNamePanel::HandleOkButton));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveNamePanel::HandleCancelButton));

    m_editBox.m_editText.m_nMaxLength = SAVE_NAME_MAX_LENGTH;
}
#else
CSWGuiSaveNamePanel::CSWGuiSaveNamePanel(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_pSaveLoad = 0;
    StartLoadFromLayout(CResRef("savename_p"), 1, 1, 1);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_editBox, CExoString("EDITBOX"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    StopLoadFromLayout();

    // The icon label starts as a copy of the title label.
    m_lblIcon = m_lblTitle;

    CSWGuiExtent extent = m_editBox.m_extent;
    extent.x = static_cast<int>(static_cast<float>(g_nScaledNativeWidth) / static_cast<float>(g_nScreenWidth) * -200.0f + extent.x);
    m_lblIcon.SetExtent(extent);
    m_lblIcon.m_text.m_params.SetText(CExoString());
    m_lblIcon.m_tail.bIgnoreMouse = 1;
    AddControl(&m_lblIcon);
    IosAddLabelIcon(&m_lblIcon, 'x');
    CenterPanel();

    CSWGuiExtent extentPanel = m_extent;
    extentPanel.y = static_cast<short>(m_pGuiManager->m_nScreenHeight / 3);
    SetExtent(extentPanel);

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveNamePanel::HandleOkButton));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveNamePanel::HandleCancelButton));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    AspyrAddButtonIcon(&m_btnOk, 'a');
    AspyrAddButtonIcon(&m_btnCancel, 'b');

    m_editBox.m_editText.m_nMaxLength = SAVE_NAME_MAX_LENGTH;
}
#endif

void CSWGuiSaveNamePanel::SetTextAndPointer(CSWGuiSaveLoad* pSaveLoad, const CExoString& sText)
{
    m_pSaveLoad = pSaveLoad;
    m_editBox.m_editText.SetText(sText);
}

// Dismiss the modal without writing anything.
void CSWGuiSaveNamePanel::HandleCancelButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1
    if (SDL_IsChromebook())
        SDL_StopTextInput();
    m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    CSWGuiPanel* pPanel = this;
    do {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    m_pGuiManager->PopModalPanel();
}

// Commit the typed name and close.
void CSWGuiSaveNamePanel::HandleOkButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pSaveLoad->WriteGame(CExoString(m_editBox.m_editText.m_sText));

#if KOTOR1
    if (SDL_IsChromebook())
        SDL_StopTextInput();
    m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    CSWGuiPanel* pPanel = this;
    do {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    m_pGuiManager->PopModalPanel();
}

// OK, cancel and the virtual keyboard map onto the panel handlers.
void CSWGuiSaveNamePanel::HandleInputEvent(int nType, int val)
{
    K2_UNUSED(val);
    switch (nType) {
    case INPUTEVENT_GUI_A:
    case INPUTEVENT_GUI_START:
        HandleOkButton(0);
        break;
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_BACK:
        HandleCancelButton(0);
        break;
    case INPUTEVENT_GUI_X:
#if KOTOR1
        pressedGamepadButtons = 0;
        if (SDL_IsChromebook())
            m_editBox.m_editText.SetText(CExoString(""));
#endif
        ASLPlat_ShowVirtualKeyboard(m_editBox.m_editText.m_sText.CStr(), SAVE_NAME_MAX_LENGTH);
        break;
    }
}

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
void k2namekeyboard_push_savename(CSWGuiSaveNamePanel* pPanel);
#endif

// Focus the name field, then park the pointer on the panel.
void CSWGuiSaveNamePanel::OnPanelAdded()
{
    m_editBox.SetFocus();
    CSWGuiPanel::OnPanelAdded();
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    k2namekeyboard_push_savename(this);
#endif
#if !KOTOR1  // K1: focus and base only, no pointer park

    if (IsJoystickConnected())
        return;
    if (gDontMoveCursor_Mac != 0)
        return;

    int nX = m_extent.x;
    int nY = m_extent.y;
    g_pGuiMan->StoreCurrentMousePosition();
    g_pGuiMan->MoveMouseToPosition(nX, nY);
#endif
}

// Restore the pointer the panel stashed.
void CSWGuiSaveNamePanel::OnPanelRemoved()
{
    CSWGuiPanel::OnPanelRemoved();
    g_pGuiMan->RestoreOldMousePosition();
}

// Handles the name-field keys: delete, forward A/B to the panel, or type.
void CSWGuiSaveGameEditBox::HandleKeyPress(int nKey)
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
    default:
        if (nKey == EDITKEY_DELETE) {
            m_editText.RemoveLastChar();
        } else if (nKey >= EDITKEY_FIRST_PRINTABLE) {
            static char sChar[2];
            sChar[0] = static_cast<char>(nKey);
            sChar[1] = 0;
            m_editText.AddNewChar(sChar);
        }
        break;
    }
}

CSWGuiControl* CSWGuiEditbox::AsEditBox()
{
    return this;
}

void CSWGuiSaveLoad::ExitPanel()
{
#if KOTOR1
    m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    CSWGuiPanel* pPanel = this;
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) != 0)
        g_pAppManager->m_pClientExoApp->DisplayMainMenu(0);
    else
        m_pGuiManager->PopModalPanel();
}

#if !KOTOR1  // K2 only
// Cycle the filter tab, wrapping at both ends.
void CSWGuiSaveLoad::HandleFilterPress(CSWGuiControl* pControl)
{
    if (pControl != 0 && pControl->m_tail.nCurrentEventArg == 0)
        return;

    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    int nNext = m_nFilterIndex + 1;
    m_nFilterIndex = (nNext >= m_filterNames.m_count) ? 0 : nNext;

    PopulateGameLB();
}
#endif

// Gamepad and keyboard event router.
void CSWGuiSaveLoad::HandleInputEvent(int nType, int nValue)
{
#if KOTOR1
    if (nValue != 0)
    {
        switch (nType)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ExitPanel();
            break;
        }
    }
#else
    if (nValue != 0)
    {
        int nDelta;
        switch (nType)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_bEndGameMode != 0)
            {
                g_pAppManager->m_pClientExoApp->SetEndGamePending(1, 0.0f, 1);
                CSWGuiPanel* pPanel = this;
                do
                {
                    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                    pPanel = pPanel->m_pNextInChain;
                } while (pPanel != 0);
                return;
            }
            ExitPanel();
            break;

        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_LEFT_SHOULDER:
            nDelta = -1;
            goto CycleFilterTabs;

        case INPUTEVENT_GUI_DPAD_RIGHT:
        case INPUTEVENT_GUI_RIGHT_SHOULDER:
            nDelta = 1;

        CycleFilterTabs:
            if (m_filterNames.m_count == 0)
                break;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_nFilterIndex += nDelta;
            if (m_nFilterIndex < 0)
                m_nFilterIndex = m_filterNames.m_count - 1;
            else if (m_nFilterIndex >= m_filterNames.m_count)
                m_nFilterIndex = 0;
            PopulateGameLB();
            break;
        }
    }
#endif

    CSWGuiPanel::HandleInputEvent(nType, nValue);
}

// Cloud-upload counterpart of DeleteGame.
void CSWGuiSaveLoad::HandleCloudButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pMsgBox->SetAllowCancel(1);

#if KOTOR1  // K1: the override table is "data/misc.txt"
    static IosLocalizedStrings sLocalized("data/misc.txt", true);
#else
    static IosLocalizedStrings sLocalized("data/miscand.txt", true);
#endif
    pMsgBox->SetMessage(sLocalized.GetString(1, 2));

    pMsgBox->SetCallback(
        this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::HandleCloudButton2), pControl);
    m_pGuiManager->AddPanel(pMsgBox, 1, 1);
}

// Check the writable resource and the free space before saving.
void CSWGuiSaveLoad::HandleSaveButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    g_pExoResMan->AddResourceDirectory(CExoString("OPTIONS:"));

    unsigned int nNeeded;
    {
        CExoFile file(CExoString("OPTIONS:OPT"), 0, CExoString("r"));
        if (file.FileOpened() != 0 && file.GetSize() != 0)
            nNeeded = 1600;
        else
            nNeeded = 1602;
    }

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(m_saveList.GetSelectedControl());
    if (pEntry == 0)
        return;

    if (pEntry->m_bSaveExists != 0)
    {
        // The row already has a save: the free-space result is discarded.
        unsigned long long nFree;
        g_pExoResMan->GetFreeDiskSpace(CExoString("SAVES:"), &nFree);

        CSWGuiMessageBox* pMsgBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pMsgBox->SetAllowCancel(1);
        pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_CONFIRM_OVERWRITE_SAVE));
        pMsgBox->SetCallback(
            this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::PromptForSaveName),
            pEntry);
        m_pGuiManager->AddPanel(pMsgBox, 1, 1);
        return;
    }

    unsigned long long nFree;
    g_pExoResMan->GetFreeDiskSpace(CExoString("SAVES:"), &nFree);

    if (nNeeded <= (nFree >> 14))
    {
        PromptForSaveName(pEntry);
        return;
    }

    // How many 16 KiB blocks short, reported in MB.
#if KOTOR1
    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    int nShortBy = nNeeded - static_cast<int>(nFree >> 14);
#else
    int nShortBy = nNeeded - static_cast<int>(nFree >> 14);
    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
#endif
    pEntry->m_nBlocksShort = nShortBy;

    CExoString sTlk;
    CExoString sMsg;
    sMsg.Format(const_cast<char*>("%2.3f"), static_cast<float>(nShortBy << 14) * 1.0e-6f);
    g_pTlkTable->SetCustomToken(0, sMsg);
#if KOTOR1
    sTlk = g_pTlkTable->GetSimpleString(STRREF_NOT_ENOUGH_SPACE_FOR_NEW_SLOT);
#else
    sTlk = g_pTlkTable->GetSimpleString(STRREF_NOT_ENOUGH_SPACE_FOR_NEW_SLOT);
#endif
    g_pTlkTable->ParseStr(sTlk);

    pMsgBox->SetAllowCancel(0);
    pMsgBox->SetMessage(sTlk);
    pMsgBox->SetCallback(this, 0, 0);
    m_pGuiManager->AddPanel(pMsgBox, 1, 1);
}

// Deletes this save: confirm, then RemoveGame.
void CSWGuiSaveLoad::DeleteGame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pMsgBox->SetAllowCancel(1);
    pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_CONFIRM_DELETE_SAVE));
    pMsgBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::RemoveGame),
                         pControl);
    m_pGuiManager->AddPanel(pMsgBox, 1, 1);
}

#if !KOTOR1  // K2 only
// Finish the cloud sync started by CheckCloudSync.
void CSWGuiSaveLoad::OnCloudSyncSuccess(CSWGuiControl*)
{
    if (m_nCloudSyncState != CLOUD_SYNC_PENDING)
        return;

    CSWGuiControl* pControl = m_saveList.GetControl(0);
    if (pControl != 0)
        m_saveList.SetSelectedControl(pControl, 0);

    m_nCloudSyncState = CLOUD_SYNC_FINISHED;
}
#endif

#if !KOTOR1  // K2 only
// Selection sort by descending timestamp.
void CSWGuiSaveLoad::SortGamesByTimeStamp(CExoArrayList<CSWGuiControl*>& list)
{
    int nCount = list.m_count;
    if (nCount < 2)
        return;

    for (int i = 0; i < nCount - 1; i++)
    {
        CSWGuiControl* pBest = list[i];
        unsigned long long nBest = static_cast<CSWGuiSaveLoadEntry*>(pBest)->m_timeStamp;
        int nBestIdx = i;

        for (int k = i + 1; k < nCount; k++)
        {
            unsigned long long nTime = static_cast<CSWGuiSaveLoadEntry*>(list[k])->m_timeStamp;
            if (nTime > nBest)
            {
                nBestIdx = k;
                nBest = nTime;
            }
        }

        list[i] = list[nBestIdx];
        list[nBestIdx] = pBest;
    }
}
#endif

#if !KOTOR1
// Drive a cloud sync when the platform supports it.
void CSWGuiSaveLoad::CheckCloudSync(bool bParam)
{
    if (bParam || !IosCloudIsAvailable())
        return;

    // The status is read once per comparison.
    if (GetCloudSynchStatus() != CloudSynchStatus_Idle &&
        GetCloudSynchStatus() != CloudSynchStatus_Failed)
        return;

    SynchCloudSave();

    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pMsgBox->SetAllowCancel(0);
    pMsgBox->SetAllowDismissal(0);
    pMsgBox->SetCallback(0, 0, 0);

    pMsgBox->SetMessage(CExoString(GetCloudStatusString()));

    float fYMult = 1.0f;
    float fXMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    m_pGuiManager->GenerateAspectMultipliers(m_bCenterOnNativeRect, fXMult, fYMult,
                                             nXOffset, nYOffset, static_cast<float>(static_cast<unsigned>(g_nGUIWidth)),
                                             static_cast<float>(static_cast<unsigned>(g_nGUIHeight)));

    pMsgBox->SetExtent(CSWGuiExtent(pMsgBox->m_extent.x, pMsgBox->m_extent.y,
                                    pMsgBox->m_extent.w, static_cast<int>(fYMult * 50.0f)));
    m_pGuiManager->AddPanel(pMsgBox, 1, 1);
    m_pGuiManager->BringPanelToFront(pMsgBox);
}
#endif

// Load directly on the fast path, otherwise confirm through the box.
void CSWGuiSaveLoad::VerifyLoadGame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiControl* pEntry = m_saveList.GetSelectedControl();
    if (pEntry == 0)
        return;

#if KOTOR1
    if (m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) {
        LoadGame(pEntry);
        return;
    }
#else
    if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) || m_bEndGameMode != 0) {
        LoadGame(pEntry);

        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
        jmethodID mid = env->GetStaticMethodID(cls, "OnLoadingSavedGames", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
        return;
    }
#endif

    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pMsgBox->SetAllowCancel(1);
    pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_LOAD_WITHOUT_SAVING));
    pMsgBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::LoadGame),
                         pEntry);
    m_pGuiManager->AddPanel(pMsgBox, 1, 1);
}

// Handles the repair prompt: re-lists the saves and hands input back to the GUI.
void CSWGuiSaveLoad::CorruptGame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pMsgBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pMsgBox->SetAllowCancel(0);
    pMsgBox->SetMessage(static_cast<uint64_t>(STRREF_UNABLE_TO_LOAD_SAVE));
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pMsgBox, 1, 1);
    pMsgBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSaveLoad::ReloadGamesList), 0);

    g_pExoInput->SetActive(1);
    g_pAppManager->m_pClientExoApp->EnableInput();
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

// Repopulate the list and re-seat the selection.
void CSWGuiSaveLoad::ReloadGamesList(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    int nSelected = m_saveList.m_nSelectedIndex;
    PopulateGameList();
    int nLast = m_saveList.m_aControls.m_count - 1;
#if KOTOR1
    m_saveList.SetSelectedControl(nLast > nSelected ? nSelected : nLast, 0);
#else
    m_saveList.SetSelectedControl(nLast < nSelected ? nLast : nSelected, 0);
#endif
}

#if !KOTOR1  // K2 only
// Load the first row when it is the party's own save.
int CSWGuiSaveLoad::LaunchMostRecentSave()
{
    CSWGuiControl* pControl = m_saveList.GetControl(0);
    if (pControl == 0)
        return 0;

    CExoString sSaveName(static_cast<CSWGuiSaveLoadEntry*>(pControl)->m_pcName);

    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    if (pServerExoApp == 0)
        return 0;

    CExoString sPartyName(pServerExoApp->GetPartyTable()->m_saveTableName);
    if (sPartyName.CompareNoCase(sSaveName) == 0)
        return 0;

    LoadGame(pControl);
    return 1;
}
#endif

// Three ways to start a load: PC autosave, leave-the-module, or in place.
void CSWGuiSaveLoad::LoadGame(CSWGuiControl* pControl)
{
    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(pControl);

#if KOTOR1  // K1: no end-game mode; the body runs under g_UI_mutex
    SDL_LockMutex(g_UI_mutex);
    if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) == 0 &&
        !g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
    {
        SDL_UnlockMutex(g_UI_mutex);
        return;
    }
#else
    if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) == 0 &&
        !g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted &&
        m_bEndGameMode == 0)
        return;
#endif

    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);

    if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) != 0)
        g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();
    else
        g_pAppManager->m_pClientExoApp->BusyWaitMuteSound();

    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;

    if (pEntry->m_bLiveContent != 0 && g_bEnableLiveContentIgnore == 0)
    {
        CSWGuiMessageBox* pBox = pClient->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);

        CExoString text;
        text = g_pTlkTable->GetSimpleString(0);
        g_pTlkTable->SetCustomToken(0, pEntry->m_sTokenValue);
        g_pTlkTable->ParseStr(text);
        pBox->SetMessage(text);
        m_pGuiManager->AddPanel(pBox, 1, 1);
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
        SDL_UnlockMutex(g_UI_mutex);
#endif
        return;  // K1: still holding g_UI_mutex (no unlock on this path)
    }

    CSWPartyTable::ResetCurrentSessionStartTime();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(1);

#if KOTOR1
    if (pEntry->m_bRebootAutoSave != 0)
    {
        SDL_UnlockMutex(g_UI_mutex);
        return;
    }
#else
    if (pEntry->m_bRebootAutoSave != 0)
        return;
#endif

    if (pEntry->m_bPCAutoSave != 0)
    {
        m_pGuiManager->PopModalPanel();
        pClient->GetInGameGui()->HideSWInGameGui(0);
        pClient->SetLoadScreenByModuleName(pEntry->m_moduleName);
        pClient->SetLoadMusic(CExoString(""));
#if KOTOR1
        pClient->ShowLoadScreen(1, 1, 0);
#else
        pClient->ShowLoadScreen(1, 1, 0, 1);
#endif
        pClient->SetLoadBarProgress(0, 1);
        UpdateScreen(1.0f / 30.0f, 0, 0);

        if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) == 0)
        {
            g_pAppManager->DestroyServer();
            g_pAppManager->m_pClientExoApp->UnloadModule();
        }

        g_pAppManager->CreateServer(0);
        g_pAppManager->m_pClientExoApp->GetWorldTimer()->PauseWorldTimer();
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->PauseWorldTimer();

        pClient->GetNetLayer()->StartConnectToSession(0, pClient->GetClientName(),
                                                      CExoString(""), 10,
                                                      CNetLayer::CONNECTTYPE_PLAYER);

        pClient->GetSWCMessage()->SendPlayerToServerStatus_RequestStatus();
        pClient->SetInputClass(INPUTCLASS_GAME, 1);
        pClient->DisableInput();
#if KOTOR1
        LoadPCAutoSave(pEntry->m_moduleName);
#else
        LoadPCAutoSave(pEntry->m_moduleName, pEntry->m_bCloudSave != 0);
#endif

#if KOTOR1
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        CSWGuiPanel* pPanel = this;
        do
        {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif
    }
    else if ((m_nModeFlags & SAVELOAD_MODE_EXIT_MODULE) != 0)
    {
        pClient->SetLoadScreenByModuleName(pEntry->m_moduleName);
        pClient->SetLoadMusic(CExoString(""));
#if KOTOR1
        pClient->ShowLoadScreen(0, 1, 0);
#else
        pClient->ShowLoadScreen(0, 1, 0, 1);
#endif
        pClient->SetGamePlayHint(pEntry->m_gamePlayHint);
#if KOTOR1  // K1: one story hint
        pClient->SetStoryHint(pEntry->m_storyHints[0]);
#else
        pClient->SetStoryHint(pEntry->m_storyHints[0], 0);
        pClient->SetStoryHint(pEntry->m_storyHints[1], 1);
        pClient->SetStoryHint(pEntry->m_storyHints[2], 2);
        pClient->SetStoryHint(pEntry->m_storyHints[3], 3);
        pClient->SetStoryHint(pEntry->m_storyHints[4], 4);
        pClient->SetStoryHint(pEntry->m_storyHints[5], 5);
        pClient->SetStoryHint(pEntry->m_storyHints[6], 6);
        pClient->SetStoryHint(pEntry->m_storyHints[7], 7);
        pClient->SetStoryHint(pEntry->m_storyHints[8], 8);
        pClient->SetStoryHint(pEntry->m_storyHints[9], 9);
#endif
#if KOTOR1  // K1: the hint is drawn from the generic pool, not the save's module
        pClient->SetLoadScreenHint(pClient->GetNextLoadScreenHintSTRREF(CExoString(""), 2));
#else
        pClient->SetLoadScreenHint(
            pClient->GetNextLoadScreenHintSTRREF(pEntry->m_moduleName, 2));
#endif
        pClient->SetLoadScreenHintUpdateTimer(0);

        unsigned int nSaveSlot = pEntry->m_nSaveSlot;
        CExoString sSaveName(pEntry->m_saveName);
        CExoString sModule(pEntry->m_moduleName);
        UnpackGame(pEntry->m_nSaveSlot, pEntry->m_saveName, pControl);

#if KOTOR1
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        for (CSWGuiPanel* pPanel = this; pPanel != 0; pPanel = pPanel->m_pNextInChain)
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#endif

        g_pAppManager->CreateServer(0);
        g_pAppManager->m_pClientExoApp->GetWorldTimer()->PauseWorldTimer();
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->PauseWorldTimer();

        pClient->GetNetLayer()->StartConnectToSession(0, pClient->GetClientName(),
                                                      CExoString(""), 10,
                                                      CNetLayer::CONNECTTYPE_PLAYER);

        pClient->GetSWCMessage()->SendPlayerToServerStatus_RequestStatus();
        pClient->SetPlayerCharacterName(CExoString("test"));
        pClient->SetInputClass(INPUTCLASS_GAME, 1);
        pClient->DisableInput();
        g_pAppManager->m_pServerExoApp->SetLoadFromSaveGame(1);
        g_pAppManager->m_pClientExoApp->SetLoadingSaveGame(1);
        g_pAppManager->m_pClientExoApp->SendLoadGameRequest(nSaveSlot, sSaveName, sModule);
    }
    else
    {
        pClient = g_pAppManager->m_pClientExoApp;
        pClient->SetLoadScreenByModuleName(pEntry->m_moduleName);
        pClient->SetLoadMusic(CExoString(""));
#if KOTOR1
        pClient->ShowLoadScreen(0, 1, 0);
#else
        pClient->ShowLoadScreen(0, 1, 0, 1);
#endif
        pClient->SetGamePlayHint(pEntry->m_gamePlayHint);
#if KOTOR1  // K1: one story hint
        pClient->SetStoryHint(pEntry->m_storyHints[0]);
#else
        pClient->SetStoryHint(pEntry->m_storyHints[0], 0);
        pClient->SetStoryHint(pEntry->m_storyHints[1], 1);
        pClient->SetStoryHint(pEntry->m_storyHints[2], 2);
        pClient->SetStoryHint(pEntry->m_storyHints[3], 3);
        pClient->SetStoryHint(pEntry->m_storyHints[4], 4);
        pClient->SetStoryHint(pEntry->m_storyHints[5], 5);
        pClient->SetStoryHint(pEntry->m_storyHints[6], 6);
        pClient->SetStoryHint(pEntry->m_storyHints[7], 7);
        pClient->SetStoryHint(pEntry->m_storyHints[8], 8);
        pClient->SetStoryHint(pEntry->m_storyHints[9], 9);
#endif
#if KOTOR1  // K1: the hint is drawn from the generic pool, not the save's module
        pClient->SetLoadScreenHint(pClient->GetNextLoadScreenHintSTRREF(CExoString(""), 2));
#else
        pClient->SetLoadScreenHint(
            pClient->GetNextLoadScreenHintSTRREF(pEntry->m_moduleName, 2));
#endif
        pClient->SetLoadScreenHintUpdateTimer(0);

        UnpackGame(pEntry->m_nSaveSlot, pEntry->m_saveName, pControl);

#if KOTOR1
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        CSWGuiPanel* pPanel = this;
        do
        {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif

        while (m_pGuiManager->m_modalPanels.m_count != 0)
            m_pGuiManager->PopModalPanel();

        pClient->GetInGameGui()->HideSWInGameGui(1);
        g_pAppManager->m_pServerExoApp->SetLoadFromSaveGame(1);
        g_pAppManager->m_pClientExoApp->SetLoadingSaveGame(1);
        g_pAppManager->m_pClientExoApp->SendLoadGameRequest(pEntry->m_nSaveSlot, pEntry->m_saveName,
                                                            pEntry->m_moduleName);
    }
#if KOTOR1
    SDL_UnlockMutex(g_UI_mutex);
#endif
}

// Repaint the details pane for the selected row.
void CSWGuiSaveLoad::ShowGame(CSWGuiControl* pControl)
{
#if KOTOR1
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(pControl);
    if (pEntry->AsButton() == 0)
        return;

    int nVisible = !pEntry->m_bNewSlot;
    if (static_cast<int>(pEntry->m_nSaveSlot) < 0)
        nVisible = 0;
    m_btnCloud.m_tail.bVisible = nVisible;
    m_iconCloud.m_tail.bVisible = nVisible;

    if (pEntry->m_bSaveExists && !pEntry->m_bCorrupt)
    {
        m_btnDelete.SetEnabled(1);

        int nPos = pEntry->m_areaName.Find(CExoString("-"), 0);
        if (nPos == -1)
        {
            m_lblPlanetName.m_text.m_params.SetText(pEntry->m_areaName);
            m_lblAreaName.m_text.m_params.SetText(CExoString(""));
        }
        else
        {
            CExoString sPlanet = pEntry->m_areaName.Left(nPos - 1);
            CExoString sRest = pEntry->m_areaName.Right(pEntry->m_areaName.GetLength() - nPos - 2);
            m_lblPlanetName.m_text.m_params.SetText(sPlanet);
            m_lblAreaName.m_text.m_params.SetText(sRest);
        }

        for (int i = 0; i < 3; i++)
            m_aPMLabels[i].m_border.m_params.SetFillImage(pEntry->GetPlayerPortrait(i), 0);

        if (pEntry->m_bRebootAutoSave || pEntry->m_bPCAutoSave || pEntry->m_nSaveSlot == 0)
        {
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(pEntry->m_screenshot), 0);
        }
        else
        {
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(""), 0);
            if (!m_sResourceDir.IsEmpty())
                g_pExoResMan->RemoveResourceDirectory(m_sResourceDir);
            m_sResourceDir = pEntry->GetGameDirectory();
            g_pExoResMan->AddResourceDirectory(m_sResourceDir);
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef("screen"), 0);
            if (!g_pExoResMan->Exists(CResRef("screen"), RESTYPE_TGA))
                m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(pEntry->m_screenshot), 0);
        }

        m_lblScreenshot.m_text.m_params.SetText(CExoString(""));
        if (pEntry->m_bCheatUsed)
        {
            m_lblScreenshot.m_text.m_params.SetText(CExoString("Cheat Used"));
            m_lblScreenshot.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_lblScreenshot.m_tail.bVisible = 1;
        }
    }
    else
    {
        m_lblPlanetName.m_text.m_params.SetText(CExoString(""));
        m_lblAreaName.m_text.m_params.SetText(CExoString(""));
        for (int i = 0; i < 3; i++)
            m_aPMLabels[i].m_border.m_params.SetFillImage(CResRef(), 0);
        m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(), 0);
        m_lblScreenshot.m_text.m_params.SetStrRef(STRREF_NEW_SLOT);
        m_btnDelete.SetEnabled(0);
    }

    if (m_nModeFlags & SAVELOAD_MODE_SAVE)
    {
        m_btnSaveLoad.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        const Vector& vSave = CGuiInGame::ENABLED_ICON_COLOR;
        m_btnSaveLoad.m_border.m_params.m_color.r = vSave.x;
        m_btnSaveLoad.m_border.m_params.m_color.g = vSave.y;
        m_btnSaveLoad.m_border.m_params.m_color.b = vSave.z;
    }
    else
    {
        m_btnSaveLoad.m_text.m_params.SetColor(
            pEntry->m_bLiveContent ? CGuiInGame::DISABLED_TEXT : CGuiInGame::COLOR_BLUE);
        const Vector& vSave = pEntry->m_bLiveContent ? CGuiInGame::DISABLED_ICON_COLOR
                                                     : CGuiInGame::ENABLED_ICON_COLOR;
        m_btnSaveLoad.m_border.m_params.m_color.r = vSave.x;
        m_btnSaveLoad.m_border.m_params.m_color.g = vSave.y;
        m_btnSaveLoad.m_border.m_params.m_color.b = vSave.z;
    }
    m_btnDelete.m_text.m_params.SetColor(
        pEntry->m_bSaveExists ? CGuiInGame::COLOR_BLUE : CGuiInGame::DISABLED_TEXT);
    const Vector& vDelete = pEntry->m_bSaveExists ? CGuiInGame::ENABLED_ICON_COLOR
                                                  : CGuiInGame::DISABLED_ICON_COLOR;
    m_btnDelete.m_border.m_params.m_color.r = vDelete.x;
    m_btnDelete.m_border.m_params.m_color.g = vDelete.y;
    m_btnDelete.m_border.m_params.m_color.b = vDelete.z;
#else
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiSaveLoadEntry* pEntry = static_cast<CSWGuiSaveLoadEntry*>(pControl);
    if (pEntry->AsButton() == 0)
        return;

// The cloud button shows only for a row the cloud can hold.
    int nCloud = IosCloudIsAvailable();
    if (pEntry->m_bNewSlot || pEntry->m_bCloudSave != 0)
        nCloud = 0;
    int nVisible = nCloud;
    if (static_cast<int>(pEntry->m_nSaveSlot) < 0)
        nVisible = 0;
    m_btnCloud.m_tail.bVisible = nVisible;
    m_iconCloud.m_tail.bVisible = nVisible;

    if (pEntry->m_bSaveExists && !pEntry->m_bCorrupt)
    {
        m_btnDelete.SetEnabled(1);

        int nPos = pEntry->m_areaName.Find(CExoString("-"), 0);
        if (nPos == -1)
        {
            m_lblPlanetName.m_text.m_params.SetText(pEntry->m_areaName);
            m_lblAreaName.m_text.m_params.SetText(CExoString(""));
        }
        else
        {
#if KOTOR1  // K1: "<planet> - <area>" is split with no test for the spaces around the dash
            CExoString sPlanet = pEntry->m_areaName.Left(nPos - 1);
            CExoString sRest = pEntry->m_areaName.Right(pEntry->m_areaName.GetLength() - nPos - 2);
            m_lblPlanetName.m_text.m_params.SetText(sPlanet);
            m_lblAreaName.m_text.m_params.SetText(sRest);
#else
            CExoString sName(pEntry->m_areaName);
            CExoString sPlanet;
            CExoString sRest;
            if (sName[nPos] == ' ')
                sPlanet = sName.Left(nPos - 1);
            else
                sPlanet = sName.Left(nPos);
            if (sName[nPos + 1] == ' ')
                sRest = sName.Right((sName.m_sString != 0 ? static_cast<int>(strlen(sName.m_sString)) : 0) - nPos - 2);
            else
                sRest = sName.Right((sName.m_sString != 0 ? static_cast<int>(strlen(sName.m_sString)) : 0) - nPos - 1);
            m_lblPlanetName.m_text.m_params.SetText(sPlanet);
            m_lblAreaName.m_text.m_params.SetText(sRest);
#endif
        }

        for (int i = 0; i < 3; i++)
            m_aPMLabels[i].m_border.m_params.SetFillImage(pEntry->GetPlayerPortrait(i), 0);

        // Mount the slot's resource directory so "screen" resolves.
        if (pEntry->m_bRebootAutoSave || pEntry->m_bPCAutoSave)
        {
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(pEntry->m_screenshot), 0);
        }
        else
        {
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(""), 0);
            if (m_sResourceDir.m_sString != 0 && *m_sResourceDir.m_sString != 0)
                g_pExoResMan->RemoveResourceDirectory(m_sResourceDir);
            m_sResourceDir = pEntry->GetGameDirectory();
            g_pExoResMan->AddResourceDirectory(m_sResourceDir);
            m_lblScreenshot.m_border.m_params.SetFillImage(CResRef("screen"), 0);
        }

        m_lblScreenshot.m_text.m_params.SetText(CExoString(""));
        if (pEntry->m_bCheatUsed)
        {
            m_lblScreenshot.m_text.m_params.SetText(CExoString("Cheat Used"));
            m_lblScreenshot.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_lblScreenshot.m_tail.bVisible = 1;
        }
    }
    else
    {
        // No loadable save: clear every pane and use the "select a game" strref.
        m_lblPlanetName.m_text.m_params.SetText(CExoString(""));
        m_lblAreaName.m_text.m_params.SetText(CExoString(""));
        for (int i = 0; i < 3; i++)
            m_aPMLabels[i].m_border.m_params.SetFillImage(CResRef(), 0);
        m_lblScreenshot.m_border.m_params.SetFillImage(CResRef(), 0);
        m_lblScreenshot.m_text.m_params.SetStrRef(STRREF_NEW_SLOT);
        m_btnDelete.SetEnabled(0);
    }

    // Button tints; on the save screen BTN_SAVELOAD is painted unconditionally.
    if (m_nModeFlags & SAVELOAD_MODE_SAVE)
    {
        m_btnSaveLoad.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        const Vector& vSave = CGuiInGame::COLOR_NEW_GREEN;
        m_btnSaveLoad.m_border.m_params.m_color.r = vSave.x;
        m_btnSaveLoad.m_border.m_params.m_color.g = vSave.y;
        m_btnSaveLoad.m_border.m_params.m_color.b = vSave.z;
    }
    else
    {
        m_btnSaveLoad.m_text.m_params.SetDefaultColor(
            pEntry->m_bLiveContent ? CGuiInGame::COLOR_NEW_GREEN : CGuiInGame::COLOR_NEW_TEXT);
        const Vector& vSave =
            pEntry->m_bLiveContent ? CGuiInGame::DISABLED_TEXT : CGuiInGame::COLOR_NEW_GREEN;
        m_btnSaveLoad.m_border.m_params.m_color.r = vSave.x;
        m_btnSaveLoad.m_border.m_params.m_color.g = vSave.y;
        m_btnSaveLoad.m_border.m_params.m_color.b = vSave.z;
    }
    m_btnDelete.m_text.m_params.SetDefaultColor(
        pEntry->m_bSaveExists ? CGuiInGame::COLOR_NEW_TEXT : CGuiInGame::COLOR_NEW_GREEN);
    const Vector& vDelete =
        pEntry->m_bSaveExists ? CGuiInGame::COLOR_NEW_GREEN : CGuiInGame::DISABLED_TEXT;
    m_btnDelete.m_border.m_params.m_color.r = vDelete.x;
    m_btnDelete.m_border.m_params.m_color.g = vDelete.y;
    m_btnDelete.m_border.m_params.m_color.b = vDelete.z;

    // "Time played" line: hours and minutes tokens.
    CExoString sPlayed;
    g_pTlkTable->SetCustomToken(0, CExoString(static_cast<unsigned int>(pEntry->m_nTimePlayed) / 3600));
    g_pTlkTable->SetCustomToken(1, CExoString((static_cast<unsigned int>(pEntry->m_nTimePlayed) / 60) % 60));
    sPlayed = g_pTlkTable->GetSimpleString(STRREF_SAVE_PLAY_TIME);
    g_pTlkTable->ParseStr(sPlayed);
    m_lblTimePlayed.m_text.m_params.SetText(sPlayed);

#endif
}

// Parse the display name, then read the save's info-file fields.
#if KOTOR1
void CSWGuiSaveLoadEntry::LoadData(const CExoString& fileName)
{
    CExoString sPortraitField;
    CExoString sSaveDir;
    CExoString sUnused;
    int nType;
    CResStruct cResStruct;
    CResGFF* pGff;

    int nSep = fileName.Find(CExoString(" - "), 0);

    {
        CExoString sSlot = fileName.Left(nSep);
        m_nSaveSlot = sSlot.AsINT();
    }

    m_saveName = fileName.Right(fileName.GetLength() - nSep - 3);

    sSaveDir = CExoString("SAVES:") + fileName;
    g_pExoResMan->AddResourceDirectory(sSaveDir);

    CExoString sResolved =
        g_pExoBase->m_pAliasList->ResolveFileName(sSaveDir + "/" + "CORRUPT", 0);

    CExoFile file(sSaveDir + "/" + "CORRUPT", 0, "rb");

    if (file.FileOpened())
    {
        m_bSaveExists = 1;
        m_bCorrupt = 1;
        g_pExoResMan->RemoveResourceDirectory(sSaveDir);
        return;
    }

    pGff = new CResGFF(0, const_cast<char*>("NFO "), CResRef("savenfo"));
    pGff->GetTopLevelStruct(&cResStruct);

    m_areaName = pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("AREANAME"), nType, CExoString(""));
    m_moduleName =
        pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("LASTMODULE"), nType, CExoString(""));
    m_nTimePlayed = pGff->ReadFieldDWORD(&cResStruct, const_cast<char*>("TIMEPLAYED"), nType, 0);
    m_saveGameName =
        pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("SAVEGAMENAME"), nType, CExoString(""));

    if (nType == 0)
        m_saveGameName = CExoString("Old Save Game");

    m_bCheatUsed = static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("CHEATUSED"), nType, 0));
    m_bRebootAutoSave =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("REBOOTAUTOSAVE"), nType, 0));
    m_bPCAutoSave =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("PCAUTOSAVE"), nType, 0));
    m_screenshot = pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("SCREENSHOT"), nType, CExoString(""));

    m_gamePlayHint =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("GAMEPLAYHINT"), nType, m_gamePlayHint));
    m_storyHints[0] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT"), nType, m_storyHints[0]));

    unsigned char nLiveContent =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("LIVECONTENT"), nType, 0));

    if (nLiveContent)
    {
        CExoString sLive;
        for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++)
        {
            sLive.Format(const_cast<char*>("LIVE%d"), i);
            CExoString sValue = pGff->ReadFieldCExoString(&cResStruct, sLive.CStr(), nType, sLive);

            const CExoString& path = g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0);
            if (path.GetLength() == 0)
            {
                if (nLiveContent & (1 << (i - 1)))
                {
                    m_bSaveExists = 1;
                    m_bLiveContent = 1;
                    m_sTokenValue = sValue;
                    delete pGff;
                    g_pExoResMan->RemoveResourceDirectory(sSaveDir);
                    return;
                }
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        sPortraitField.Format(const_cast<char*>("%s%d"), "PORTRAIT", i);
        m_aPlayerPortrait[i] =
            pGff->ReadFieldCResRef(&cResStruct, sPortraitField.CStr(), nType, CResRef(""));
    }

    delete pGff;
    g_pExoResMan->RemoveResourceDirectory(sSaveDir);

    m_bCorrupt = 0;
    m_bLiveContent = 0;
    m_sTokenValue = "";
    m_bSaveExists = 1;
}
#else
void CSWGuiSaveLoadEntry::LoadData(const CExoString& fileName)
{
    CExoString sPortraitField;
    CExoString sSaveDir;
    CExoString sUnused;
    int nType;
    CResStruct cResStruct;
    CResGFF* pGff;

    int nSep = fileName.Find(CExoString(" - "), 0);

    {
        CExoString sSlot = fileName.Left(nSep);
        m_nSaveSlot = (sSlot.m_sString != 0) ? atoi(sSlot.m_sString) : 0;
    }

    m_saveName = fileName.Right(
        fileName.GetLength() - nSep - 3);

    if (m_nSaveSlot == 2 && m_saveName == "cloud")
        m_bCloudSave = 1;

    sSaveDir = CExoString("SAVES:") + fileName;
    g_pExoResMan->AddResourceDirectory(sSaveDir);

    CExoString sResolved =
        g_pExoBase->m_pAliasList->ResolveFileName(sSaveDir + "\\" + "CORRUPT", 0);

    CExoFile file(sSaveDir + "\\" + "CORRUPT", 0, "rb");

    if (file.FileOpened())
    {
        m_bSaveExists = 1;
        m_bCorrupt = 1;
        g_pExoResMan->RemoveResourceDirectory(sSaveDir);
        return;
    }

    pGff = new CResGFF(0, const_cast<char*>("NFO "), CResRef("savenfo"));
    pGff->GetTopLevelStruct(&cResStruct);

    m_areaName = pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("AREANAME"), nType, CExoString(""));
    m_moduleName =
        pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("LASTMODULE"), nType, CExoString(""));
    m_nTimePlayed = pGff->ReadFieldDWORD(&cResStruct, const_cast<char*>("TIMEPLAYED"), nType, 0);
    m_saveGameName =
        pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("SAVEGAMENAME"), nType, CExoString(""));

    if (nType == 0 && m_bCloudSave != 0)
        m_saveGameName = CExoString("Cloud Save");

    if (m_bCloudSave != 0)
    {
        if (m_saveGameName.m_sString != 0 && static_cast<int>(strlen(m_saveGameName.m_sString)) != 0)
            m_saveGameName = m_saveGameName + CExoString(" (cloud save") + CExoString(")");
        else
            m_saveGameName = CExoString("Cloud Save");
    }

    unsigned char nCheatUsed =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("CHEATUSED"), nType, 0));
    m_bCheatUsed = nCheatUsed & 1;
    m_bModded = (nCheatUsed >> 1) & 1;
    m_bRebootAutoSave =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("REBOOTAUTOSAVE"), nType, 0));
    m_bPCAutoSave =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("PCAUTOSAVE"), nType, 0));
    m_screenshot = pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("SCREENSHOT"), nType, CExoString(""));
    m_timeStamp = pGff->ReadFieldDWORD64(&cResStruct, const_cast<char*>("TIMESTAMP"), nType, 0);
    m_pcName = pGff->ReadFieldCExoString(&cResStruct, const_cast<char*>("PCNAME"), nType, CExoString(""));
    m_nSaveNumber = pGff->ReadFieldDWORD(&cResStruct, const_cast<char*>("SAVENUMBER"), nType, 0);

    m_gamePlayHint =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("GAMEPLAYHINT"), nType, m_gamePlayHint));
    m_storyHints[0] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT0"), nType, m_storyHints[0]));
    m_storyHints[1] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT1"), nType, m_storyHints[1]));
    m_storyHints[2] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT2"), nType, m_storyHints[2]));
    m_storyHints[3] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT3"), nType, m_storyHints[3]));
    m_storyHints[4] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT4"), nType, m_storyHints[4]));
    m_storyHints[5] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT5"), nType, m_storyHints[5]));
    m_storyHints[6] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT6"), nType, m_storyHints[6]));
    m_storyHints[7] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT7"), nType, m_storyHints[7]));
    m_storyHints[8] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT8"), nType, m_storyHints[8]));
    m_storyHints[9] =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("STORYHINT9"), nType, m_storyHints[9]));

    unsigned char nLiveContent =
        static_cast<unsigned char>(pGff->ReadFieldBYTE(&cResStruct, const_cast<char*>("LIVECONTENT"), nType, 0));

    if (nLiveContent)
    {
        CExoString sLive;
        for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++)
        {
            sLive.Format(const_cast<char*>("LIVE%d"), i);
            CExoString sValue = pGff->ReadFieldCExoString(&cResStruct, sLive.CStr(), nType, sLive);

            const CExoString& path = g_pExoBase->m_pAliasList->GetAliasPath(sLive, 0);
            if (path.GetLength() == 0)
            {
                if (nLiveContent & (1 << (i - 1)))
                {
                    m_bSaveExists = 1;
                    m_bLiveContent = 1;
                    m_sTokenValue = sValue;
                    delete pGff;
                    g_pExoResMan->RemoveResourceDirectory(sSaveDir);
                    return;
                }
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        sPortraitField.Format(const_cast<char*>("%s%d"), "PORTRAIT", i);
        m_aPlayerPortrait[i] =
            pGff->ReadFieldCResRef(&cResStruct, sPortraitField.CStr(), nType, CResRef(""));
    }

    delete pGff;
    g_pExoResMan->RemoveResourceDirectory(sSaveDir);

    m_bCorrupt = 0;
    m_bLiveContent = 0;
    m_sTokenValue = "";
    m_bSaveExists = 1;
}
#endif
