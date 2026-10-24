#include "nwportrait.h"
#include "AURORA/aurmodel.h"
#include "exostring.h"
#include "twodimarray.h"
#include "twodimarrays.h"
#include "nwsrules.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"

CSWPortrait::CSWPortrait()
{
    m_baseResRef = const_cast<char*>("");
}

CSWPortrait::~CSWPortrait()
{
}

// An empty base resref yields an empty ref; a state outside 1..5 returns the base unchanged.
CResRef CSWPortrait::GetPortrait(unsigned char nState)
{
    CResRef resRef = m_baseResRef;
    if (resRef.GetResRef()[0] == 0)
        return CResRef("");

    switch (nState)
    {
        case 1: resRef += CExoString("T"); break;
        case 2: resRef += CExoString("S"); break;
        case 3: resRef += CExoString("M"); break;
        case 4: resRef += CExoString("L"); break;
        case 5: resRef += CExoString("H"); break;
    }
    return resRef;
}

// Swaps a model's base texture for a portrait strip and marks it as a portrait, so the texture
// manager will not downsample it.
int CSWPortrait::ReplacePortraitTexture(CAurObject* pObject, const CResRef& resRef)
{
    char szResRef[20];
    resRef.CopyToString(szResRef);

    pObject->ReplaceTexture(const_cast<char*>("gui_replace01"), szResRef);
    pObject->SetTextureAttributes(const_cast<char*>("gui_replace01"), const_cast<char*>("portraits"));
    return 0;
}

// Looks up a portraits.2da row into m_baseResRef; returns 0 for the two reserved ids. The gore
// setting selects between the "LowGore" and "BaseResRef" columns.
int CSWPortrait::SetPortraitId(unsigned short nPortraitId)
{
    CExoString sPortrait;
    CExoString sAlt;
    int        bFound;

    if (nPortraitId > PORTRAIT_ID_LAST)
        return 0;

    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel != 0)
    {
        bFound = g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(nPortraitId,
                                                                       "BaseResRef", &sPortrait);
        if (!bFound)
        {
            sAlt = "";
            sPortrait = "";
        }
    }
    else
    {
        bFound = g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(nPortraitId,
                                                                       "LowGore", &sPortrait);
        if (!bFound)
        {
            bFound = g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(nPortraitId,
                                                                           "BaseResRef", &sPortrait);
            if (!bFound)
            {
                sAlt = "";
                sPortrait = "";
            }
        }
    }

    sPortrait = sAlt + sPortrait;
    m_baseResRef = sPortrait;
    return 1;
}
