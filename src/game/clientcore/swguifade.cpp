#include "k2stdint.h"
#include <algorithm>

#include "swguifade.h"
#include "appmanager.h"
#include "guiingame.h"
#include "swguimaininterfacepc2.h"
#include "clientexoapp.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "ClientExoAppInternal.h"
#include "worldtimer.h"
#include "aurscene.h"
#if KOTOR1
void IosTemporaryDisableInteraction(int nDisable);
#endif

CSWGuiFade::CSWGuiFade(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

#if KOTOR1  // K1: the plain "fade" layout, with the screen-sized extent set through CSWGuiObject::SetExtent
    StartLoadFromLayout(CResRef("fade"), 1, 1, 1);
    InitControl(m_label, CExoString("LBL_MSG"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiExtent extent = m_extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight;
    CSWGuiObject::SetExtent(extent);
    ResetFade();
#else
    StartLoadFromLayout(CResRef("fade_p"), 1, 1, 1);
    InitControl(m_label, CExoString("LBL_MSG"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_extent.x = 0;
    m_extent.y = 0;
    m_extent.w = m_pGuiManager->m_nScreenWidth;
    m_extent.h = m_pGuiManager->m_nScreenHeight;
    ResetFade();
#endif
}

void CSWGuiFade::ResetFade()
{
    m_fAlpha = 0.0f;
    m_vColor = Vector(0.0f, 0.0f, 0.0f);
    SetMsg(0);

    m_fadeElapsed = 0.0f;
    m_fadeDuration = 0.0f;
    m_fadeAmount = 0.0f;
    m_nFadeType = 0;

    m_nLastDrawTime = static_cast<int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                           ->GetSnapshotTime() / 1000);
}

void CSWGuiFade::SetMsg(uint64_t nMsg)
{
    CExoString sText;
    if (nMsg != 0)
    {
        sText = g_pTlkTable->GetSimpleString(nMsg);
        g_pTlkTable->ParseStr(sText);
        m_label.m_text.m_params.SetText(sText);
    }
    else
    {
        m_label.m_text.m_params.SetText(CExoString(""));
    }
}

void CSWGuiFade::SetFade(int nType, float fLength, float fDelay, const Vector& vColor)
{
    m_vColor = vColor;
    CSWGuiExtent extent = m_extent;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight;
    CSWGuiObject::SetExtent(extent);
    m_fAlpha = (nType == 0) ? 0.0f : 1.0f;
    m_fadeElapsed = fLength;
    m_fadeDuration = fDelay;
    m_fadeAmount = 0.1f;
    m_nFadeType = nType;

    m_nLastDrawTime = static_cast<int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                           ->GetSnapshotTime() / 1000);
}

void CSWGuiFade::Draw(float f)
{
    unsigned int nNow = static_cast<unsigned int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                                           ->GetSnapshotTime() / 1000);
    unsigned int nPrev = m_nLastDrawTime;
    m_nLastDrawTime = nNow;

    // While the in-game GUI shows its fade override panel, that panel owns the fade and nothing is
    // drawn here, base panel included.
    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0
        && g_pAppManager->m_pClientExoApp->GetInGameGui()->GetFadeOverridePanelVisible())
        return;

    if (m_fadeAmount > 0.0f)
    {
        float fDelta = static_cast<float>(nNow - nPrev) / 1000.0f;
        if (fDelta < 5.0f)
            m_fadeAmount += fDelta;

        if (m_fadeAmount > m_fadeElapsed)
        {
            float fAlpha = (m_fadeAmount - m_fadeElapsed) / m_fadeDuration;
            // mode 0 fades in (alpha ramps toward 1), any other mode fades out (1 - t toward 0).
            if (m_nFadeType == 0)
                fAlpha = std::min(fAlpha, 1.0f);
            else
                fAlpha = std::max(1.0f - fAlpha, 0.0f);
#if KOTOR1  // K1 releases touch input once the fade is nearly clear
            float fClamped = fAlpha < 1.0f ? fAlpha : 1.0f;
            fClamped = fClamped > 0.0f ? fClamped : 0.0f;
            m_fAlpha = fClamped;
            if (fAlpha <= 0.1f)
                IosTemporaryDisableInteraction(0);
#else
            fAlpha = fAlpha < 1.0f ? fAlpha : 1.0f;
            fAlpha = fAlpha > 0.0f ? fAlpha : 0.0f;
            m_fAlpha = fAlpha;
#endif
        }
    }

    CSWGuiPanel::Draw(f);
}

int CSWGuiFade::IsFadeDone() {
    if (m_fadeAmount == 0.0f)
        return 1;
    return m_fadeAmount > m_fadeElapsed + m_fadeDuration;
}

CSWGuiControl* CSWGuiFade::HitCheckMouse(int x, int y)
{
    if (__builtin_expect(m_fAlpha > 0.001f, 0))
        return CSWGuiPanel::HitCheckMouse(x, y);
    return 0;
}

#if !KOTOR1  // K2-only slot
bool CSWGuiFade::ShouldFreeMouseOnAdd() { return false; }
#endif

