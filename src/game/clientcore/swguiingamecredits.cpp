// The end-of-game credits panel.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguiingamecredits.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "exostring.h"
#include "exoresman.h"
#include "twodimarray.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "exosound.h"
#include "exoinput.h"
#include "soundprioritygroup.h"

void RestoreResolutionFromMovie();

extern CSWGuiManager* g_pGuiMan;
extern CTlkTable*     g_pTlkTable;
extern int            g_nScreenWidth;
extern int            g_nScreenHeight;

static int s_nSavedGuiWidth;
static int s_nSavedGuiHeight;
static int s_nSavedScreenWidth;
static int s_nSavedScreenHeight;
#if KOTOR1
#endif

CSWGuiInGameCredits::CSWGuiInGameCredits(CSWGuiManager* pGuiManager, const CExoString& sText)
    : CSWGuiPanel(pGuiManager)
{
    m_stateFlags |= 1;

#if KOTOR1
    StartLoadFromLayout(CResRef("credits"), 1, 1, 1);
    InitControl(m_listBox, CExoString("LB_CREDITS"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
    {
        s_nSavedGuiWidth = g_pGuiMan->m_nScreenWidth;
        s_nSavedGuiHeight = g_pGuiMan->m_nScreenHeight;
        s_nSavedScreenWidth = g_nScreenWidth;
        s_nSavedScreenHeight = g_nScreenHeight;
        g_nScreenWidth = g_pGuiMan->m_nScreenWidth;
        g_nScreenHeight = g_pGuiMan->m_nScreenHeight;
    }

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = g_pGuiMan->m_nScreenWidth;
    extent.h = g_pGuiMan->m_nScreenHeight;
    SetExtent(extent);

#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    extent.x = static_cast<int>(s_fScale * 40.0f);
    extent.y = static_cast<int>(s_fScale * 40.0f);
    extent.w = static_cast<int>(static_cast<float>(m_extent.w) - s_fScale * 80.0f);
    extent.h = static_cast<int>(static_cast<float>(m_extent.h) - s_fScale * 80.0f);
    m_listBox.SetExtent(extent);

    extent.x = static_cast<int>(static_cast<float>(extent.x) + s_fScale * 4.0f);
    extent.y = static_cast<int>(static_cast<float>(extent.y) + s_fScale * 4.0f);
    extent.w = static_cast<int>(static_cast<float>(extent.w) - s_fScale * 8.0f);
    extent.h = static_cast<int>(static_cast<float>(extent.h) - s_fScale * 8.0f);

    CSWGuiLabel* pTemplate = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_labelText.Initialize(extent, pTemplate, static_cast<float>(m_extent.h) / 480.0f);
#else
    StartLoadFromLayout(CResRef("credits_p"), 1, 1, 1);
    InitControl(m_listBox, CExoString("LB_CREDITS"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
    {
        s_nSavedGuiWidth = g_pGuiMan->m_nScreenWidth;
        s_nSavedGuiHeight = g_pGuiMan->m_nScreenHeight;
        s_nSavedScreenWidth = g_nScreenWidth;
        s_nSavedScreenHeight = g_nScreenHeight;
        g_nScreenWidth = 640;
        g_nScreenHeight = 480;
        g_pGuiMan->SetSize(640, 480);
    }

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = g_pGuiMan->m_nScreenWidth;
    extent.h = g_pGuiMan->m_nScreenHeight;
    SetExtent(extent);

    extent.x = 40;
    extent.y = 40;
    extent.w = m_extent.w - 80;
    extent.h = m_extent.h - 80;
    m_listBox.SetExtent(extent);

    extent.x += 4;
    extent.y += 4;
    extent.w -= 8;
    extent.h -= 8;

    CSWGuiLabel* pTemplate = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_labelText.Initialize(extent, pTemplate, static_cast<float>(m_extent.h) / 480.0f);
#endif

    extent.x = 0;
    extent.y = 0;
    extent.w = g_pGuiMan->m_nScreenWidth;
    extent.h = g_pGuiMan->m_nScreenHeight;
    m_labelFull.Initialize(extent, pTemplate, 1.0f);

    AddControl(&m_labelFull);
    m_labelFull.AddEvent(
        INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));

    m_labelText.m_text.m_params.SetBaseFont(CResRef("fnt_credits"));

    m_nTextCount = 0;
    m_paNameStrRefs = 0;

    C2DA* p2DA = new C2DA(CResRef("credits"), 0);
    if (p2DA->Load2DArray() != 0)
    {
        m_nTextCount = static_cast<unsigned char>(p2DA->m_nRows);
        m_paNameStrRefs = new uint64_t[m_nTextCount];

        int nValue;
        int bOk;
        for (uint64_t i = 0; i < m_nTextCount; i++)
        {
            {
                CExoString sColumn("Name");
                bOk = p2DA->GetINTEntry(static_cast<int>(i), sColumn, &nValue);
            }
            m_paNameStrRefs[i] = bOk != 0 ? static_cast<uint64_t>(nValue) : 0;
        }

        m_fLineStep = 0;
        m_sMusicName = sText;
        m_pMusic = 0;
        Initialize();
    }
}

void CSWGuiInGameCredits::Initialize()
{
    m_fScrollAccum = 0;
    m_fFadeTimer = 0;
    m_nOldInputClass = 2;
    m_nCurrentIndex = 0;
    m_nUpdateMode = 1;
    m_nFadePhase = 1;
    m_fLineStep = 0;

    SetNextText();

    m_labelText.m_text.m_params.SetAlignment(SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE);
    m_labelText.m_text.m_params.m_fAlpha = 0;
}

CSWGuiInGameCredits::~CSWGuiInGameCredits()
{
    if (m_pMusic != 0)
    {
        delete m_pMusic;
    }

    if (m_paNameStrRefs != 0)
    {
        delete[] m_paNameStrRefs;
    }

    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui != 0)
        pGui->FadeAndStopDialogAmbientTrack();

    g_pAppManager->m_pClientExoApp->StopCreditSequence();
    g_pAppManager->m_pClientExoApp->SetInputClass(m_nOldInputClass, 1);

    if (pGui != 0)
        pGui->SetGlobalUnpauseableState(0);

    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bEvilEndingHackInProgress != 0)
    {
        g_nScreenWidth = s_nSavedScreenWidth;
        g_nScreenHeight = s_nSavedScreenHeight;
        g_pGuiMan->SetSize(s_nSavedGuiWidth, s_nSavedGuiHeight);
        g_pAppManager->m_pClientExoApp->m_pInternal->SetEvilEndingHackInProgress(0);
        RestoreResolutionFromMovie();
    }

    g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_EVIL_ENDING);
}


void CSWGuiInGameCredits::Draw(float f)
{
    if (g_pAppManager->m_pServerExoApp == 0 ||
        g_pAppManager->m_pServerExoApp->GetPauseState(2) == 0)
    {
        if (m_nUpdateMode == 0)
            UpdateScrolling(f);
        else
            UpdateFading(f);
    }

    CSWGuiPanel::Draw(f);
}

void CSWGuiInGameCredits::UpdateFading(float f)
{
    int nPhase = m_nFadePhase;

    float fTimer = m_fFadeTimer + f;
    m_fFadeTimer = fTimer;


    if (fTimer >= 1.0f)
    {
        if (nPhase != 0)
        {
            m_labelText.m_text.m_params.m_fAlpha = 1.0f;

            if (fTimer >= 3.0f)
            {
                m_fFadeTimer = fTimer - static_cast<float>(static_cast<int>(fTimer / 3.0f)) * 3.0f;
                m_nFadePhase = 0;
            }
        }
        else
        {
            m_labelText.m_text.m_params.m_fAlpha = 0;

            if (fTimer >= 2.0f)
            {
                m_fFadeTimer = fTimer - (static_cast<float>(static_cast<int>(fTimer * 0.5f)) + static_cast<float>(static_cast<int>(fTimer * 0.5f)));
                m_nFadePhase = 1;
                SetNextText();
            }
        }
    }
    else
    {
        if (nPhase == 0)
            m_labelText.m_text.m_params.m_fAlpha = 1.0f - fTimer;
        else
            m_labelText.m_text.m_params.m_fAlpha = fTimer;
    }
}

void CSWGuiInGameCredits::UpdateScrolling(float f)
{
    if (m_listBox.m_scrollBar.m_nCurValue == m_listBox.m_scrollBar.m_nMaxValue)
    {
        if (m_pMusic->IsPlaying() == 0)
            SetToBeDeleted(0);
    }
    else
    {
        m_fScrollAccum += f;

        while (m_fScrollAccum >= m_fLineStep)
        {
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            m_fScrollAccum -= m_fLineStep;

            if (m_listBox.m_scrollBar.m_nCurValue == m_listBox.m_scrollBar.m_nMaxValue)
                break;
        }
    }
}

void CSWGuiInGameCredits::OnPanelAdded()
{
    g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_EVIL_ENDING);
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGui->SetGlobalUnpauseableState(1);

    m_nOldInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();

    CSWGuiPanel::OnPanelAdded();

    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);

    m_pMusic = new CExoStreamingSoundSource(0);
    m_pMusic->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);
    m_pMusic->Play(CResRef(m_sMusicName.CStr()), 8, 0, 1, 0);
}

void CSWGuiInGameCredits::HandleInputEvent(int nType, int nArg)
{
    if (nArg == 0)
        return;

    switch (nType)
    {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        {
#if KOTOR1
            m_stateFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pChain = this;
            do
            {
                pChain->m_stateFlags = (pChain->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
                pChain = pChain->m_pNextInChain;
            } while (pChain != 0);
#endif
            break;
        }
    }
}

void CSWGuiInGameCredits::SetNextText()
{
    int bLast;

    if (static_cast<int>(m_nTextCount - 1) <= static_cast<int>(m_nCurrentIndex))
    {
        bLast = 1;
        m_nUpdateMode = 0;
        m_labelText.m_text.m_params.SetAlignment(SWGUIALIGN_CENTER | SWGUIALIGN_TOP);
        m_labelText.m_text.m_params.m_fAlpha = 1.0f;
    }
    else
    {
        bLast = 0;
    }

    SetText(m_paNameStrRefs[m_nCurrentIndex], bLast);

    if (m_nUpdateMode == 0)
        m_fLineStep = 305.0f / static_cast<float>(m_labelText.m_extent.h);

    m_nCurrentIndex++;
}

void CSWGuiInGameCredits::SetText(uint64_t nStrRef, int bFlag)
{
    CSWGuiExtent extent;

    if (m_nUpdateMode == 0)
    {
        int nLines;
        STR_RES strRes;
        CExoString sText;
        CExoString sPad;
        g_pTlkTable->Fetch(static_cast<unsigned int>(nStrRef), strRes, CTlkTable::GENDER_MALE);

        nLines = static_cast<int>(static_cast<float>(m_labelText.m_extent.h) /
                       ((static_cast<float>(m_labelText.m_text.GetFontHeight()) * static_cast<float>(m_extent.h)) / 480.0f));
        sPad = "";
        for (int i = 0; i <= nLines; i++)
            sPad = sPad + CExoString("\n");

        sText = sPad + strRes.text + sPad;
        m_labelText.m_text.m_params.SetText(sText);
    }
    else
    {
        m_labelText.m_text.m_params.SetStrRef(nStrRef);
    }

    m_listBox.ClearItems();

    if (bFlag != 0)
    {
        CSWGuiText* pText = &m_labelText.m_text;
        extent = m_labelText.m_extent;
        extent.h = pText->GetIdealHeight();
        m_labelText.SetExtent(extent);
    }

    CSWGuiControl* apRows[1] = { &m_labelText };
    m_listBox.AddControls(apRows, 1, 0, 0, 0);
    m_listBox.SetSelectedControl(&m_labelText, 0);

    extent = m_listBox.m_extent;
    m_listBox.m_fVisibleLines = static_cast<float>(extent.h);
#if KOTOR1
    m_listBox.m_fScrollHintTime = 100.0f;
#endif
}
