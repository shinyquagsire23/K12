#include "k2stdint.h"
#include "swguiskillflowchart.h"
#include "CExoArrayListImpl.h"
#include "nwscreaturestats.h"
#include "nwccreaturestats.h"
#include "nwclass.h"
#include "nwscreature.h"
#include "exostring.h"
#include "exoresman.h"
#include "swguicontrol.h"
#include <stddef.h>

#include "appmanager.h"
#include "clientexoapp.h"
#include "swgui.h"
#include "guiingame.h"
#include "port/k2qolapi.h"
#include "nwscriptconst.h"
#include "exoinput.h"

float GetResolutionTextScalar();

#if KOTOR1 && !defined(K2_UI_TOUCH)
#include "port/k1gui.h"

// Returns the height of a feat or power row: 40 units of the 480-line console layout, scaled to
// the screen height.
static int K1FlowRowHeight()
{
    return static_cast<int>(static_cast<float>(g_SDL_mode.h) / 480.0f * 40.0f + 0.5f);
}
#endif

CSWGuiSkillFlowChart::CSWGuiSkillFlowChart() {
    m_nSelectedSlot = 0;
    m_nSelectedFlow = 0;
}

CSWGuiSkillFlowChart::~CSWGuiSkillFlowChart() {
    for (int64_t i = 0; i < m_aFlows.m_count; i++) {
        if (m_aFlows.m_pArray[i])
            delete m_aFlows.m_pArray[i];
        m_aFlows.m_pArray[i] = NULL;
    }
    m_aFlows.m_count = 0;
}

void CSWGuiSkillFlowChart::ClearChart() {
    for (int64_t i = 0; i < m_aFlows.m_count; i++) {
        if (m_aFlows.m_pArray[i])
            delete m_aFlows.m_pArray[i];
        m_aFlows.m_pArray[i] = NULL;
    }
    m_aFlows.m_count = 0;
}


#define FLOWRADKEY(ss) ((ss)->GetSubRadialSpell(0))
#define FEATPREREQS(ss) (static_cast<short>((ss)->m_nPrereqFeat1) == -1 ? 0 : static_cast<short>((ss)->m_nPrereqFeat2) == -1 ? 1 : 2)

void CSWGuiSkillFlowChart::BuildSortedPowerList(CExoArrayList<unsigned>& list)
{
#if !KOTOR1  // K1: the sorted list is rebuilt every time (no CGuiInGame cache)
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui->m_aSortedPowers.m_count > 0) {
        list = pGui->m_aSortedPowers;
        return;
    }
#endif

    for (int nSpell = 0; nSpell < g_pRules->m_pSpellArray->m_nCount; nSpell++) {
        CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpell);
        if (!pSpell)
            continue;
#if KOTOR1  // K1: usertype 1 only
        if (pSpell->m_nUserType != 1)
            continue;
#else
        if (pSpell->m_nUserType != 6 && pSpell->m_nUserType != 1)
            continue;
#endif
        if (!pSpell->m_nNameStrRef)
            continue;

        for (int64_t i = 0; i < list.m_count; i++) {
            CSWSpell* pOther = g_pRules->m_pSpellArray->GetSpell(list.m_pArray[i]);
            if (!pOther)
                continue;

            unsigned nO1 = FLOWRADKEY(pOther);
            unsigned nO2 = FLOWRADKEY(pOther);
            unsigned nM1 = FLOWRADKEY(pSpell);
            unsigned nM2 = FLOWRADKEY(pSpell);
            if (static_cast<int>(nO1 - nO2 / 10 * 10) > static_cast<int>(nM1 - nM2 / 10 * 10))
                goto iInsert;
            nO1 = FLOWRADKEY(pOther);
            nO2 = FLOWRADKEY(pOther);
            nM1 = FLOWRADKEY(pSpell);
            nM2 = FLOWRADKEY(pSpell);
            if (nO1 - nO2 / 10 * 10 != nM1 - nM2 / 10 * 10)
                continue;

            nO1 = FLOWRADKEY(pOther);
            nM1 = FLOWRADKEY(pSpell);
            if (static_cast<int>(nO1) / 1000 < static_cast<int>(nM1) / 1000)
                goto iInsert;
            nO1 = FLOWRADKEY(pOther);
            nM1 = FLOWRADKEY(pSpell);
            if (static_cast<int>(nO1) / 1000 != static_cast<int>(nM1) / 1000)
                continue;

            nO1 = FLOWRADKEY(pOther);
            nO2 = FLOWRADKEY(pOther);
            nM1 = FLOWRADKEY(pSpell);
            nM2 = FLOWRADKEY(pSpell);
            if (static_cast<int>(nO1 - nO2 / 1000 * 1000) / 10 <= static_cast<int>(nM1 - nM2 / 1000 * 1000) / 10)
                continue;
        iInsert:
            list.Insert(nSpell, static_cast<int>(i));
            goto nextSpell;
        }
        list.Add(nSpell);
    nextSpell:;
    }

#if !KOTOR1
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_aSortedPowers = list;
#endif
}

unsigned char CSWGuiSkillFlowChart::GetSkillStatus(unsigned int nId) {
    for (int64_t i = 0; i < m_aFlows.m_count; i++) {
        CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[i];
        int nSlot = 1;
        if (pFlow->m_aSlots[0].m_nId == static_cast<int>(nId)) nSlot = 0;
        else if (pFlow->m_aSlots[1].m_nId == static_cast<int>(nId)) nSlot = 1;
        else if (pFlow->m_aSlots[2].m_nId == static_cast<int>(nId)) nSlot = 2;
        else continue;
        return pFlow->m_aSlots[nSlot].m_bStatus;
    }
    return 0xff;
}

void CSWGuiSkillFlowChart::SetSkillStatus(unsigned int nId, unsigned char bStatus) {
    for (int64_t i = 0; i < m_aFlows.m_count; i++) {
        CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[i];
        if (pFlow->m_aSlots[0].m_nId == static_cast<int>(nId)) { pFlow->SetSkillStatus(0, bStatus); return; }
        if (pFlow->m_aSlots[1].m_nId == static_cast<int>(nId)) { pFlow->SetSkillStatus(1, bStatus); return; }
        if (pFlow->m_aSlots[2].m_nId == static_cast<int>(nId)) { pFlow->SetSkillStatus(2, bStatus); return; }
    }
}

void CSWGuiSkillFlowChart::SetSkillStatusAll(unsigned char bStatus) {
    for (int64_t i = 0; i < m_aFlows.m_count; i++) {
        m_aFlows.m_pArray[i]->SetSkillStatus(0, bStatus);
        m_aFlows.m_pArray[i]->SetSkillStatus(1, bStatus);
        m_aFlows.m_pArray[i]->SetSkillStatus(2, bStatus);
    }
}

void CSWGuiSkillFlowChart::SetSelectedSkill(unsigned int nId) {
    for (int i = 0; i < m_aFlows.m_count; i++) {
        CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[i];
        int nSlot;
        if (pFlow->m_aSlots[0].m_nId == static_cast<int>(nId)) nSlot = 0;
        else if (pFlow->m_aSlots[1].m_nId == static_cast<int>(nId)) nSlot = 1;
        else if (pFlow->m_aSlots[2].m_nId == static_cast<int>(nId)) nSlot = 2;
        else continue;

        if (nSlot != m_nSelectedSlot || i != m_nSelectedFlow) {
            m_aFlows.m_pArray[m_nSelectedFlow]->SetSkillSelected(m_nSelectedSlot, 0);
            m_nSelectedSlot = static_cast<unsigned char>(nSlot);
            m_nSelectedFlow = static_cast<unsigned char>(i);
        }
        m_aFlows.m_pArray[m_nSelectedFlow]->SetSkillSelected(m_nSelectedSlot, 1);
        return;
    }
}

int CSWGuiSkillFlowChart::SetTopLeft() {
    if (m_nSelectedSlot || m_nSelectedFlow) {
        if (m_nSelectedFlow < m_aFlows.m_count) {
            CSWGuiSkillFlow* pOld = m_aFlows.m_pArray[m_nSelectedFlow];
            if (pOld->m_aSlots[m_nSelectedSlot].m_bSelected & 1) {
                pOld->m_aSlots[m_nSelectedSlot].m_bSelected &= ~1;
                pOld->SetSkillStatus(m_nSelectedSlot, pOld->m_aSlots[m_nSelectedSlot].m_bStatus);
            }
        }
        m_nSelectedSlot = 0;
        m_nSelectedFlow = 0;
    }
    if (m_aFlows.m_count < 1) return -1;
    CSWGuiSkillFlow* pTop = m_aFlows.m_pArray[0];
#if KOTOR1
    // K1 routes this through the frame-painting SetSkillStatus instead of setting both alphas.
    if (!(pTop->m_aSlots[0].m_bSelected & 1)) {
        pTop->m_aSlots[0].m_bSelected |= 1;
        pTop->SetSkillStatus(0, pTop->m_aSlots[0].m_bStatus);
    }
#else
    if (!(pTop->m_aSlots[0].m_bSelected & 1)) {
        CSWGuiSkillFlow::Slot& sTop = pTop->m_aSlots[0];
        sTop.m_bSelected |= 1;
        sTop.m_imageIcon.m_params.m_fAlpha = 1.0f;
        sTop.m_imageBg.m_params.m_fAlpha = 1.0f;
        switch (sTop.m_bStatus) {
            case CSWGuiSkillFlow::SKILL_STATUS_AVAILABLE:
                sTop.m_imageIcon.m_params.m_fAlpha = 0.25f;
                sTop.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_GREEN;
                break;
            case CSWGuiSkillFlow::SKILL_STATUS_OWNED:
            case CSWGuiSkillFlow::SKILL_STATUS_GRANTED:
                sTop.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_TEXT;
                break;
            case CSWGuiSkillFlow::SKILL_STATUS_UNAVAILABLE:
                sTop.m_imageIcon.m_params.m_fAlpha = 0.25f;
                sTop.m_imageBg.m_params.m_color = CGuiInGame::COLOR_RED;
                break;
            case CSWGuiSkillFlow::SKILL_STATUS_CHOSEN:
                sTop.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_HIGHLIGHT;
                break;
            case CSWGuiSkillFlow::SKILL_STATUS_BONUS:
                sTop.m_imageBg.m_params.m_color = *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS);
                break;
            case CSWGuiSkillFlow::SKILL_STATUS_VIEW:
                sTop.m_imageBg.m_params.m_color = (sTop.m_bSelected & 1) ? CGuiInGame::COLOR_NEW_HIGHLIGHT
                                                                          : CGuiInGame::COLOR_NEW_GREEN;
                break;
        }
        if (sTop.m_bSelected & 1)
            sTop.m_imageBg.m_params.bHilight = 1;
        else
            sTop.m_imageBg.m_params.bHilight = 0;
    }
#endif
    return m_aFlows.m_pArray[0]->m_aSlots[0].m_nId;
}

void CSWGuiSkillFlowChart::BuildSortedFeatList(CExoArrayList<unsigned short>& list)
{
#if !KOTOR1  // K1: the sorted list is rebuilt every time (no CGuiInGame cache)
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui->m_aSortedFeats.m_count > 0) {
        list = pGui->m_aSortedFeats;
        return;
    }
#endif

    for (unsigned nFeat = 0; nFeat < g_pRules->m_nFeats; nFeat++) {
        CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
#if KOTOR1  // K1: no power-id skips here
        if (!pFeat || !pFeat->m_nNameStrRef)
#else
        if (!pFeat || nFeat == CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS || nFeat == CSWSCreatureStats::FEAT_MINE_IMMUNITY || !pFeat->m_nNameStrRef)
#endif
            continue;
        int nMine = FEATPREREQS(pFeat);

        for (int64_t i = 0; i < list.m_count; i++) {
            CSWFeat* pOther = g_pRules->GetFeat(list.m_pArray[i]);
            if (!pOther)
                continue;
            if (FEATPREREQS(pOther) <= nMine)
                continue;
            list.Insert(static_cast<unsigned short>(nFeat), static_cast<int>(i));
            goto nextFeat;
        }
        list.Add(static_cast<unsigned short>(nFeat));
    nextFeat:;
    }

#if !KOTOR1
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_aSortedFeats = list;
#endif
}

void CSWGuiSkillFlowChart::AddFeatSet(unsigned int nFeatA, unsigned int nFeatB, unsigned int nFeatC) {
    CResRef resRef;
    CSWGuiSkillFlow* pFlow = new CSWGuiSkillFlow();

    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeatA));
    if (pFeat)
        resRef = pFeat->m_icon;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[0].m_nId = nFeatA;
    pFlow->m_aSlots[0].m_imageIcon.m_params.SetImage(resRef, 0);

    pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeatB));
    if (pFeat)
        resRef = pFeat->m_icon;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[1].m_nId = nFeatB;
    pFlow->m_aSlots[1].m_imageIcon.m_params.SetImage(resRef, 0);

    pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeatC));
    if (pFeat)
        resRef = pFeat->m_icon;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[2].m_nId = nFeatC;
    pFlow->m_aSlots[2].m_imageIcon.m_params.SetImage(resRef, 0);

#if KOTOR1 && !defined(K2_UI_TOUCH)
    pFlow->SetExtent(CSWGuiExtent(0, 0, 242, K1FlowRowHeight()));
#elif KOTOR1
    pFlow->SetExtent(CSWGuiExtent(0, 0, 242, 40));
#else
    pFlow->SetExtent(CSWGuiExtent(0, 0, 262, static_cast<int>(GetResolutionTextScalar() * 40.0f)));
#endif

    m_aFlows.Add(pFlow);
}

// AddFeatSet's sibling, with the same three-slot icon loop, but the powers come from the spell
// array and the row is a fixed 262 x 40 rather than resolution-scaled. nSecret forces the secret
// icon before the spell's own icon is consulted.
#if !KOTOR1
void CSWGuiSkillFlowChart::AddPowerSet(unsigned int nPowerA, unsigned int nPowerB, unsigned int nPowerC, int nSecret) {
    CResRef resRef;
    CSWGuiSkillFlow* pFlow = new CSWGuiSkillFlow();

    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerA);
    if (nSecret)
        resRef = const_cast<char*>("ip_secret");
    else if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[0].m_nId = nPowerA;
    pFlow->m_aSlots[0].m_imageIcon.m_params.SetImage(resRef, 0);

    pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerB);
    if (nSecret)
        resRef = const_cast<char*>("ip_secret");
    else if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[1].m_nId = nPowerB;
    pFlow->m_aSlots[1].m_imageIcon.m_params.SetImage(resRef, 0);

    pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerC);
    if (nSecret)
        resRef = const_cast<char*>("ip_secret");
    else if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[2].m_nId = nPowerC;
    pFlow->m_aSlots[2].m_imageIcon.m_params.SetImage(resRef, 0);

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = 262;
    extent.h = 40;
    pFlow->SetExtent(extent);

    m_aFlows.Add(pFlow);
}
#endif

#if KOTOR1
// K1 has no secret icon: each slot takes the spell's own icon, or none.
void CSWGuiSkillFlowChart::AddPowerSet(unsigned int nPowerA, unsigned int nPowerB, unsigned int nPowerC) {
    CResRef resRef;
    CSWGuiSkillFlow* pFlow = new CSWGuiSkillFlow();

    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerA);
    if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[0].m_nId = nPowerA;
    pFlow->m_aSlots[0].m_imageIcon.m_params.SetImage(resRef, 0);

    pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerB);
    if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[1].m_nId = nPowerB;
    pFlow->m_aSlots[1].m_imageIcon.m_params.SetImage(resRef, 0);

    pSpell = g_pRules->m_pSpellArray->GetSpell(nPowerC);
    if (pSpell)
        resRef = pSpell->m_iconResRef;
    else
        resRef = const_cast<char*>("");
    pFlow->m_aSlots[2].m_nId = nPowerC;
    pFlow->m_aSlots[2].m_imageIcon.m_params.SetImage(resRef, 0);

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = 262;
#if !defined(K2_UI_TOUCH)
    extent.h = K1FlowRowHeight();
#else
    extent.h = 40;
#endif
    pFlow->SetExtent(extent);

    m_aFlows.Add(pFlow);
}
#endif

// Refills the chart: clears it, asks BuildSortedFeatList for the feat pool, then walks the sorted
// list giving every known prereq-less feat a row whose B/C slots are filled from the feat's
// prereq1-chained descendants (at most two, gated on class useable-or-known). The walk stops at
// the first sorted feat whose prereq1 is set; the list is sorted ascending by prereq count, so
// everything past it is chained too.
void CSWGuiSkillFlowChart::CreateFeatChart(CSWSCreatureStats* pStats) {
    CExoArrayList<unsigned short> feats;
    CSWFeat* pFeat;
    CSWFeat* pChain;
    int nB;
    int nC;
#if KOTOR1
    CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClass(pStats->m_numClasses - 1)];
#else
    unsigned int nClassSel = 0xff;
    if (pStats->m_numClasses != 0) {
        unsigned char bClassIdx = pStats->m_numClasses - 1;
        nClassSel = pStats->m_classInfo[bClassIdx].m_class;
    }
    CSWClass* pClass = &g_pRules->m_paClasses[nClassSel];
#endif

    ClearChart();

    BuildSortedFeatList(feats);

    for (int64_t nIdx = 0, nNext = 1; nIdx < feats.m_count; nIdx++, nNext++) {
        pFeat = g_pRules->GetFeat(feats.m_pArray[nIdx]);
#if KOTOR1
        if (!pFeat)
            continue;
#else
        if (feats.m_pArray[nIdx] == CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS)
            continue;
        if (!pFeat)
            continue;
        if (feats.m_pArray[nIdx] == CSWSCreatureStats::FEAT_MINE_IMMUNITY)
            continue;
#endif
        if (static_cast<short>(pFeat->m_nPrereqFeat1) != -1)
            break;
        if (!pStats->HasFeat(feats.m_pArray[nIdx]))
            continue;

        nB = -1;
        nC = -1;
        for (int nChain = static_cast<int>(nIdx + 1); nChain < feats.m_count; nChain++) {
            if (nB != -1 && nC != -1)
                break;
            if (!pStats->HasFeat(feats.m_pArray[nChain]))
                continue;
            pChain = g_pRules->GetFeat(feats.m_pArray[nChain]);
            if (!pChain || pChain->m_nPrereqFeat1 != feats.m_pArray[nIdx])
                continue;
            if (pClass->IsFeatUseable(feats.m_pArray[nChain]) || pStats->HasFeat(feats.m_pArray[nChain])) {
                if (pChain->m_nPrereqFeat2 == 0xffff)
                    nB = feats.m_pArray[nChain];
                else
                    nC = feats.m_pArray[nChain];
            }
        }
        AddFeatSet(feats.m_pArray[nIdx], nB, nC);
    }
}

// Fills the companion chart, from the overload above's skeleton but reading the client-side stats.
// Every surviving feat must also be grantable: granted and known, or the bonus/normal test picked
// by nBonus, unless the companion tag ladder grants it. A non-zero per-companion grant byte
// bypasses the gate, and baodur and kreia continue instead when theirs is negative. The chain scan's
// own baodur/kreia ladder suppresses the B/C assign for feats the companion's grant byte excludes.
#if KOTOR1
void CSWGuiSkillFlowChart::CreateFeatChart(CSWCCreatureStats* pStats, int nBonus) {
    CExoArrayList<unsigned short> feats;
    CSWFeat* pFeat;
    CSWFeat* pChain;
    unsigned char nGrantedLevel;
    int nB;
    int nC;
    CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClassLeveledUpIn()];

    ClearChart();

    BuildSortedFeatList(feats);

    for (int64_t nIdx = 0; nIdx < feats.m_count; nIdx++) {
        pFeat = g_pRules->GetFeat(feats.m_pArray[nIdx]);
        if (!pFeat)
            continue;
        if (static_cast<short>(pFeat->m_nPrereqFeat1) != -1)
            break;
        if (!pStats->HasFeat(feats.m_pArray[nIdx]) && !pClass->IsFeatUseable(feats.m_pArray[nIdx]))
            continue;
        if (!((pClass->IsGrantedFeat(feats.m_pArray[nIdx], nGrantedLevel) &&
               pStats->HasFeat(feats.m_pArray[nIdx])) ||
              (nBonus ? pClass->IsBonusFeat(feats.m_pArray[nIdx])
                      : pClass->IsNormalFeat(feats.m_pArray[nIdx]))))
            continue;

        nB = -1;
        nC = -1;
        for (int nChain = static_cast<int>(nIdx + 1); nChain < feats.m_count; nChain++) {
            if (nB != -1 && nC != -1)
                break;
            pChain = g_pRules->GetFeat(feats.m_pArray[nChain]);
            if (!pChain || pChain->m_nPrereqFeat1 != feats.m_pArray[nIdx])
                continue;
            if (!pStats->HasFeat(feats.m_pArray[nChain]) && !pClass->IsFeatUseable(feats.m_pArray[nChain]))
                continue;
            if (pChain->m_nPrereqFeat2 == 0xffff)
                nB = feats.m_pArray[nChain];
            else
                nC = feats.m_pArray[nChain];
        }
        AddFeatSet(feats.m_pArray[nIdx], nB, nC);
    }
}
#else
void CSWGuiSkillFlowChart::CreateFeatChart(CSWCCreatureStats* pStats, int nBonus) {
    CExoArrayList<unsigned short> feats;
    CSWFeat* pFeat;
    CSWFeat* pChain;
    unsigned char nGrantedLevel;
    int bGrantable;
    bool bChainGate;
    int nB;
    int nC;
    CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClassLeveledUpIn()];

    ClearChart();

    BuildSortedFeatList(feats);

    int bGrant = 0;
    for (int64_t nIdx = 0, nNext = 1; nIdx < feats.m_count; nIdx++, nNext++) {
        pFeat = g_pRules->GetFeat(feats.m_pArray[nIdx]);
        if (feats.m_pArray[nIdx] == CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS)
            continue;
        if (!pFeat)
            continue;
        if (feats.m_pArray[nIdx] == CSWSCreatureStats::FEAT_MINE_IMMUNITY)
            continue;
        if (static_cast<short>(pFeat->m_nPrereqFeat1) != -1)
            break;
        if (!pStats->HasFeat(feats.m_pArray[nIdx]) && !pClass->IsFeatUseable(feats.m_pArray[nIdx]))
            continue;

        bGrantable = (pClass->IsGrantedFeat(feats.m_pArray[nIdx], nGrantedLevel) &&
                           pStats->HasFeat(feats.m_pArray[nIdx])) ||
                          (nBonus ? pClass->IsBonusFeat(feats.m_pArray[nIdx])
                                  : pClass->IsNormalFeat(feats.m_pArray[nIdx]));

        if (CExoString(pStats->m_sTag).CompareNoCase("handmaiden") != 0) {
            if (pFeat->m_nHandmaidenGrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        } else if (CExoString(pStats->m_sTag).CompareNoCase("baodur") != 0) {
            if (pFeat->m_nBaodurGrantLevel == 0xff)
                continue;
            if (pFeat->m_nBaodurGrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        } else if (CExoString(pStats->m_sTag).CompareNoCase("kreia") != 0) {
            if (pFeat->m_nKreiaGrantLevel == 0xff)
                continue;
        } else if (CExoString(pStats->m_sTag).CompareNoCase("hanharr") != 0) {
            if (pFeat->m_nHanharrGrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        } else if (CExoString(pStats->m_sTag).CompareNoCase("hk47") != 0) {
            if (pFeat->m_nHK47GrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        } else if (CExoString(pStats->m_sTag).CompareNoCase("g0t0") != 0) {
            if (pFeat->m_nG0T0GrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        } else if (CExoString(pStats->m_sTag).CompareNoCase("atton") != 0) {
            if (pFeat->m_nAttonGrantLevel != 0) {
                bGrant = 1;
                bGrantable = 1;
            }
        }
        if (!bGrantable)
            continue;

        nB = -1;
        nC = -1;
        for (int nChain = static_cast<int>(nNext); nChain < feats.m_count; nChain++) {
            if (nB != -1 && nC != -1)
                break;
            pChain = g_pRules->GetFeat(feats.m_pArray[nChain]);
            if (CExoString(pStats->m_sTag).CompareNoCase("baodur") != 0 &&
                pChain->m_nBaodurGrantLevel == 0xff) {
                bChainGate = true;
            } else if (CExoString(pStats->m_sTag).CompareNoCase("kreia") != 0 &&
                       pChain->m_nKreiaGrantLevel == 0xff) {
                bChainGate = true;
            } else if (!pChain) {
                continue;
            } else {
                bChainGate = false;
            }
            if (pChain->m_nPrereqFeat1 != feats.m_pArray[nIdx])
                continue;
            if (!pStats->HasFeat(feats.m_pArray[nChain]) &&
                !(pClass->IsFeatUseable(feats.m_pArray[nChain]) || bGrant))
                continue;
            if (pChain->m_nPrereqFeat2 == 0xffff) {
                if (!bChainGate)
                    nB = feats.m_pArray[nChain];
            } else {
                if (!bChainGate)
                    nC = feats.m_pArray[nChain];
            }
        }
        AddFeatSet(feats.m_pArray[nIdx], nB, nC);
    }
}
#endif

// Fills the power chart. The known test is chosen by bShowAvailable (server-side class-level row versus a
// plain HasSpell), the chain scan matches subradial keys by their group digits instead of prereq
// chaining, and the %10 == 1 / %10 == 2 tail fills B/C. A not-known row still charts as a bare
// secret icon when the creature is a player-made character and the power is a bonus-radial id.
void CSWGuiSkillFlowChart::CreatePowerChart(CSWSCreatureStats* pStats, int bShowAvailable) {
    CExoArrayList<unsigned int> powers;
    CSWSpell* pChain;
    unsigned nRadA;
    unsigned nRadB;
#if !KOTOR1
    unsigned nPower;
#endif
    unsigned nCKeyA;
    unsigned nCKeyB;
    unsigned nCKeyC;
    unsigned nMKeyA;
    unsigned nMKeyB;
    unsigned nMKeyC;
    int nB;
    int nC;

    ClearChart();
    BuildSortedPowerList(powers);

    for (int64_t nIdx = 0, nNext = 1; nIdx < powers.m_count; nIdx++, nNext++) {
        CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(powers.m_pArray[nIdx]);
        if (!pSpell)
            continue;
        nRadA = FLOWRADKEY(pSpell);
        nRadB = FLOWRADKEY(pSpell);
        if (static_cast<int>(nRadA) - static_cast<int>(nRadB / 10 * 10) > 0)
            break;

        bool bKnown;
#if KOTOR1  // K1: lists every power, with no class spell-level test
        bKnown = bShowAvailable != 0 || pStats->HasSpell(0, powers.m_pArray[nIdx], 0) != 0;
#else
        if (bShowAvailable)
            bKnown = pSpell->GetSpellLevel(pStats->GetClass(pStats->m_numClasses - 1)) != 0xff;
        else
            bKnown = pStats->HasSpell(0, powers.m_pArray[nIdx], 0) != 0;
#endif
        if (!bKnown) {
#if !KOTOR1  // K1: no unlearned-power entries for a player-made character
            if (bShowAvailable == 0 && pStats->m_pOwner->m_bPlayerMadeCharacter &&
                !pStats->HasSpell(0, powers.m_pArray[nIdx], 0)) {
                nPower = powers.m_pArray[nIdx];
                // Force Sight through Beast Trick but Mind Trick, and Breath Control.
                if ((static_cast<unsigned>(nPower - FORCE_POWER_FORCE_SIGHT)
                         <= FORCE_POWER_BEAST_TRICK - FORCE_POWER_FORCE_SIGHT
                     && (nPower - FORCE_POWER_FORCE_SIGHT) != FORCE_POWER_MIND_TRICK - FORCE_POWER_FORCE_SIGHT) ||
                    nPower == FORCE_POWER_BREATH_CONTROL)
                    AddPowerSet(nPower, 0xffffffff, 0xffffffff, 1);
            }
#endif
        } else {
            nB = -1;
            nC = -1;
            for (int nChain = static_cast<int>(nIdx + 1); nChain < powers.m_count; nChain++) {
                if (nB != -1 && nC != -1)
                    break;
#if KOTOR1  // K1: the chain members are looked up with HasSpell's third argument 1
                if (bShowAvailable == 0 && !pStats->HasSpell(0, powers.m_pArray[nChain], 1))
                    continue;
#else
                if (bShowAvailable == 0 && !pStats->HasSpell(0, powers.m_pArray[nChain], 0))
                    continue;
#endif
                pChain = g_pRules->m_pSpellArray->GetSpell(powers.m_pArray[nChain]);
                if (!pChain)
                    continue;
                nCKeyA = FLOWRADKEY(pChain);
                nMKeyA = FLOWRADKEY(pSpell);
                if (static_cast<int>(nCKeyA) / 1000 != static_cast<int>(nMKeyA) / 1000)
                    continue;
                nCKeyB = FLOWRADKEY(pChain);
                nCKeyC = FLOWRADKEY(pChain);
                nMKeyB = FLOWRADKEY(pSpell);
                nMKeyC = FLOWRADKEY(pSpell);
                if (static_cast<int>(nCKeyB - nCKeyC / 1000 * 1000) / 10 !=
                    static_cast<int>(nMKeyB - nMKeyC / 1000 * 1000) / 10)
                    continue;
                nCKeyA = FLOWRADKEY(pChain);
                nCKeyB = FLOWRADKEY(pChain);
                if (nCKeyA - nCKeyB / 10 * 10 == 1)
                    nB = powers.m_pArray[nChain];
                else {
                    nCKeyA = FLOWRADKEY(pChain);
                    nCKeyB = FLOWRADKEY(pChain);
                    if (nCKeyA - nCKeyB / 10 * 10 != 2)
                        continue;
                    nC = powers.m_pArray[nChain];
                }
            }
#if KOTOR1
            AddPowerSet(powers.m_pArray[nIdx], nB, nC);
#else
            AddPowerSet(powers.m_pArray[nIdx], nB, nC, 0);
#endif
        }
    }
}

int CSWGuiSkillFlowChart::HandleInput(int nKey) {
    unsigned char bFlow = m_nSelectedFlow;
    unsigned char bSel = m_nSelectedSlot;
    switch (nKey) {
    case INPUTEVENT_GUI_DPAD_LEFT:
    case INPUTEVENT_GUI_KEY_LEFT:
        if (m_nSelectedSlot != 0) {
            g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            --m_nSelectedSlot;
        }
        break;
    case INPUTEVENT_GUI_DPAD_RIGHT:
    case INPUTEVENT_GUI_KEY_RIGHT:
        if (m_nSelectedSlot <= 1) {
            CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[m_nSelectedFlow];
            if (pFlow->m_aSlots[static_cast<unsigned char>(m_nSelectedSlot + 1)].m_nId != -1) {
                g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                ++m_nSelectedSlot;
            }
        }
        break;
    case INPUTEVENT_GUI_DPAD_UP:
    case INPUTEVENT_GUI_KEY_UP: {
        m_nSelectedFlow = static_cast<unsigned char>((m_nSelectedFlow ? m_nSelectedFlow : m_aFlows.m_count) - 1);
        CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[m_nSelectedFlow];
        if (pFlow->m_aSlots[m_nSelectedSlot].m_nId == -1)
            while (pFlow->m_aSlots[--m_nSelectedSlot].m_nId == -1);
        break;
    }
    case INPUTEVENT_GUI_DPAD_DOWN:
    case INPUTEVENT_GUI_KEY_DOWN: {
        m_nSelectedFlow = m_nSelectedFlow == m_aFlows.m_count - 1 ? 0 : m_nSelectedFlow + 1;
        CSWGuiSkillFlow* pFlow = m_aFlows.m_pArray[m_nSelectedFlow];
        if (pFlow->m_aSlots[m_nSelectedSlot].m_nId == -1)
            while (pFlow->m_aSlots[--m_nSelectedSlot].m_nId == -1);
        break;
    }
    }
    if (bFlow != m_nSelectedFlow || bSel != m_nSelectedSlot) {
        CSWGuiSkillFlow* pOld = m_aFlows.m_pArray[bFlow];
        if (pOld->m_aSlots[bSel].m_bSelected & 1) {
            pOld->m_aSlots[bSel].m_bSelected &= ~1;
            pOld->SetSkillStatus(bSel, pOld->m_aSlots[bSel].m_bStatus);
        }
        unsigned char nSelNow = m_nSelectedFlow;
        CSWGuiSkillFlow* pSel = m_aFlows.m_pArray[nSelNow];
        if (!(pSel->m_aSlots[m_nSelectedSlot].m_bSelected & 1)) {
            pSel->m_aSlots[m_nSelectedSlot].m_bSelected |= 1;
            pSel->SetSkillStatus(m_nSelectedSlot, pSel->m_aSlots[m_nSelectedSlot].m_bStatus);
        }
    }
    return m_aFlows.m_pArray[m_nSelectedFlow]->m_aSlots[m_nSelectedSlot].m_nId;
}

// The in-game skill-check popup. Loads "skillinfo_p", binds the three widgets the layout carries,
// builds the twenty entry rows from LB_SKILLS' prototype row, and wires BTN_OK to the base's
// OnAButtonPressed.
CSWGuiSkillInfoBox::CSWGuiSkillInfoBox(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    StartLoadFromLayout(CResRef("skillinfo"));  // K1 loads the plain "skillinfo" layout
#else
    StartLoadFromLayout(CResRef("skillinfo_p"), 1, 1, 1);
#endif

    InitControl(m_listBox,    CExoString("LB_SKILLS"),   1, 1, 0, 0, 0);
    InitControl(m_lblMessage, CExoString("LBL_MESSAGE"), 1, 1, 0, 0, 0);
    InitControl(m_btnOk,      CExoString("BTN_OK"),      1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    // The skill list has no highlight: the arrow keys scroll the window instead of moving a
    // selection. It also clicks nothing, and its rows make the arrow-move sound.
    m_listBox.bNoSelection = 1;
    m_listBox.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    m_listBox.SetEnabled(0);

    // Build the twenty entry rows from LB_SKILLS' prototype row. nWidth is computed once, off
    // the list box's usable width.
    CSWGuiLabelHilight* pProtoRow =
        static_cast<CSWGuiLabelHilight*>(m_listBox.m_pProtoItem->AsLabelHilight());
    int nWidth = m_listBox.m_innerExtent.w - m_listBox.m_nPadding * 2;
#if KOTOR1
    for (int i = 0; i < 10; i++)
#else
    for (int i = 0; i < 20; i++)
#endif
    {
        m_aEntries[i].Initialize(pProtoRow->m_text, pProtoRow->m_border,
                                 pProtoRow->m_hilightBorder, nWidth);
    }

    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                    &CSWGuiPanel::OnAButtonPressed));
    CenterPanel();

#if !KOTOR1
    m_pCallback = 0;
    m_pCallbackObj = 0;
#endif
}

CSWGuiSkillInfoBox::~CSWGuiSkillInfoBox()
{
    m_listBox.ClearItems();
}

#ifdef K2_QOL_IMPROVEMENTS
// The game's own row height, which is also its icon frame's size.
static int K2QolSkillInfoRowBase()
{
    return static_cast<int>(GetResolutionTextScalar() * 42.0f);
}

// Sizes a row for the Text Size, as the game's own row height times the setting, grown when its
// caption does not fit, and capped at a third of the list's height (the same cap AddControls' row
// growth uses). Always sized from the base height, so rows reused by the next list do not compound.
// Also sizes the Abilities page's skill rows.
void K2QolSizeSkillInfoRow(CSWGuiInGameSkillEntry& row, const CSWGuiListBox& list)
{
    CSWGuiExtent extent = row.m_extent;
    if (extent.w < 43)
        return;
    const int nBase = K2QolSkillInfoRowBase();
    const int nMax = list.m_innerExtent.h / 3;
    int nH = nBase * k2qol_dialog_text_scale() / 100;
    if (nH > nMax)
        nH = nMax;
    if (nH < nBase)
        nH = nBase;
    extent.h = nH;
    row.SetExtent(extent);
    if (nH >= nMax || k2qol_dialog_text_scale() <= 100)
        return;
    // For the caption at the setting and at its width, grow the row by what its box lacks.
    const int nBox = row.m_text.m_extent.h;
    const int nWant = K2QolFitText(row.m_text, 0x7fffffff);
    if (nWant > nBox) {
        extent.h = nH + nWant - nBox < nMax ? nH + nWant - nBox : nMax;
        row.SetExtent(extent);
    }
}

#endif

void CSWGuiSkillInfoBox::SetSkillList(const CExoArrayList<unsigned int>& skills, unsigned char nType)
{
    CExoArrayList<CSWGuiControl*> aControls;
    if (static_cast<int>(skills.m_count) > 0)
    {
        for (int i = 0; i < skills.m_count; i++)
        {
            m_aEntries[i].SetSkill(skills.m_pArray[i], nType);
#ifdef K2_QOL_IMPROVEMENTS
            K2QolSizeSkillInfoRow(m_aEntries[i], m_listBox);
#endif
            aControls.Add(&m_aEntries[i]);
        }
    }
#if KOTOR1
    m_listBox.AddControls(aControls, 0, 0, 0);
#else
    m_listBox.AddControls(aControls, 0, 1, 0);
#endif
    SetActiveControl(&m_listBox, 0);
}

void CSWGuiSkillInfoBox::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
#if KOTOR1
            // K1: B closes the box too, the state stamp uses the K1 bit map, and there is no
            // modal pop or callback.
            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
                m_stateFlags = nFlags;
                break;
            }
#else
            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                // Stamp the panel's state field: bit 8 set for state 1, which the manager's
                // per-frame walk then pops and frees; a box already in state 2 only clears bit 7.
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
                m_pGuiManager->PopModalPanel();
                if (m_pCallbackObj && m_pCallback)
                    (m_pCallbackObj->*m_pCallback)(0);
                break;
            }
#endif
            default:
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

#if !KOTOR1  // K2-only (K1 has no dismiss callback)
void CSWGuiSkillInfoBox::SetCallback(CSWGuiObject* pObj, void (CSWGuiObject::*pFn)(CSWGuiControl*)) {
    m_pCallback = pFn;
    m_pCallbackObj = pObj;
}
#endif

#if !KOTOR1
void CSWGuiSkillInfoBox::OnPanelRemoved() {
    m_pCallbackObj = 0;
    m_pCallback = 0;
}
#endif

float GetResolutionTextScalar();

// One row group of the skill/feat/power flow chart: three icon+background slot pairs plus the
// two arrows that connect them.
CSWGuiSkillFlow::CSWGuiSkillFlow() {
#if KOTOR1
    // K1: bordered slots, icon drawstyle 1, "lbl_indent" backgrounds, "lbl_skarr" arrows.
    for (int i = 0; i < 3; i++) {
        m_aSlots[i].m_border.m_params.SetCornerImage("border2d", 0);
        m_aSlots[i].m_border.m_params.SetEdgeImage("border1d", 0);
#ifdef K2_UI_TOUCH
        m_aSlots[i].m_imageIcon.m_params.SetDrawStyle(1);
#else
        // pc / xbox: the icon fills its slot (style 1 draws the texture at its own size, small
        // in the rows K1FlowRowHeight scales to the screen).
        m_aSlots[i].m_imageIcon.m_params.SetDrawStyle(0);
#endif
        m_aSlots[i].m_imageIcon.m_params.SetAlignment(18);
        m_aSlots[i].m_imageBg.m_params.SetImage("lbl_indent", 0);
    }
    for (int i = 0; i < 2; i++) {
#ifdef K2_UI_TOUCH
        m_aArrow[i].m_params.SetDrawStyle(1);
#else
        m_aArrow[i].m_params.SetDrawStyle(0);   // pc / xbox: fills the box SetExtent scales
#endif
        m_aArrow[i].m_params.SetAlignment(18);
        m_aArrow[i].m_params.SetImage("lbl_skarr", 0);
    }
#else
    for (int i = 0; i < 3; i++) {
        m_aSlots[i].m_imageIcon.m_params.SetDrawStyle(0);
        m_aSlots[i].m_imageIcon.m_params.SetAlignment(18);
        m_aSlots[i].m_imageBg.m_params.SetImage("UIBIT_ABI_BACK", 0);
    }
    for (int i = 0; i < 2; i++) {
        m_aArrow[i].m_params.SetDrawStyle(1);
        m_aArrow[i].m_params.SetAlignment(18);
        m_aArrow[i].m_params.SetImage("UIBIT_ABI_ARROW", 0);
    }
#endif
    Clear();
}

void CSWGuiSkillFlow::Clear() {
    for (int i = 0; i < 3; i++) {
        m_aSlots[i].m_bSelected &= ~1;
        m_aSlots[i].m_nId = -1;
        m_aSlots[i].m_bStatus = 0;
    }
}

#if KOTOR1
// K1: no aspect or text-scale fitting. Three square slots (left, centre, right) whose border
// takes the extent and whose icon sits in the border's inner extent, two fixed 32x32 arrows, and
// the base SetExtent last.
void CSWGuiSkillFlow::SetExtent(const CSWGuiExtent& extent) {
    CSWGuiExtent slotExtent;
    CSWGuiExtent arrowExtent;
#if KOTOR1 && !defined(K2_UI_TOUCH)
    const int nArrow = extent.h * 4 / 5;   // pc / xbox: 32 of the 40-unit row
#else
    const int nArrow = 32;
#endif
    arrowExtent.w = nArrow;
    arrowExtent.h = nArrow;

    slotExtent.x = extent.x;
    slotExtent.y = extent.y;
    slotExtent.w = extent.h;
    slotExtent.h = extent.h;
    m_aSlots[0].m_border.m_extent = slotExtent;
    m_aSlots[0].m_imageBg.SetExtent(slotExtent);
    slotExtent.x = extent.x + (extent.w - extent.h) / 2;
    m_aSlots[1].m_border.m_extent = slotExtent;
    m_aSlots[1].m_imageBg.SetExtent(slotExtent);
    slotExtent.x = extent.x + extent.w - extent.h;
    m_aSlots[2].m_border.m_extent = slotExtent;
    m_aSlots[2].m_imageBg.SetExtent(slotExtent);

    arrowExtent.y = extent.y + (extent.h - nArrow) / 2;
    arrowExtent.x = extent.x + (extent.w + extent.h) / 4 - nArrow / 2;
    m_aArrow[0].SetExtent(arrowExtent);
    arrowExtent.x = extent.x + (extent.w * 3 - extent.h) / 4 - nArrow / 2;
    m_aArrow[1].SetExtent(arrowExtent);

    for (int i = 0; i < 3; i++)
        m_aSlots[i].m_imageIcon.SetExtent(m_aSlots[i].m_border.GetInnerExtent());

    CSWGuiObject::SetExtent(extent);
}
#else
void CSWGuiSkillFlow::SetExtent(const CSWGuiExtent& extent) {
    int nX = extent.x;
    int nY = extent.y;

    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    int nAspectWidth = 262;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != NULL) {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned>(g_nGUIHeight)));
        nAspectWidth = static_cast<int>(fXMult * 262.0f);
    }

    float fScalar = GetResolutionTextScalar();
    int nScaleH = static_cast<int>(fScalar * 44.0f);
#ifdef K2_QOL_IMPROVEMENTS
    const int nBaseH = nScaleH;
    nScaleH = K2QolSkillFlowHeight(nAspectWidth, nScaleH);   // the icons follow the Text Size
#endif

    CSWGuiExtent slotExtent;
    slotExtent.x = nX;
    slotExtent.y = nY + 2;
    slotExtent.w = nScaleH - 4;
    slotExtent.h = nScaleH - 4;

    m_aSlots[0].m_imageBg.SetExtent(slotExtent);
    slotExtent.x = nX + (nAspectWidth - nScaleH) / 2;
    m_aSlots[1].m_imageBg.SetExtent(slotExtent);
    slotExtent.x = nX + (nAspectWidth - nScaleH);
    m_aSlots[2].m_imageBg.SetExtent(slotExtent);

    CSWGuiExtent arrowExtent;
    arrowExtent.y = static_cast<int>((static_cast<float>(nScaleH) - fScalar * 32.0f) * 0.5f + static_cast<float>(nY));
    arrowExtent.x = static_cast<int>(static_cast<float>(nX + (nAspectWidth + nScaleH) / 4) + fScalar * -16.0f);
    arrowExtent.w = static_cast<int>(fScalar * 32.0f);
    arrowExtent.h = arrowExtent.w;
    m_aArrow[0].SetExtent(arrowExtent);
    arrowExtent.x = nX + (nAspectWidth * 3 - nScaleH) / 4 - 16;
    m_aArrow[1].SetExtent(arrowExtent);
#ifdef K2_QOL_IMPROVEMENTS
    if (nScaleH != nBaseH) {
        // The arrows grow with the icons, centred in the gaps between them.
        int nArrow = static_cast<int>(fScalar * 32.0f) * nScaleH / nBaseH;
        arrowExtent.w = nArrow;
        arrowExtent.h = nArrow;
        arrowExtent.y = nY + (nScaleH - nArrow) / 2;
        arrowExtent.x = nX + (nAspectWidth + nScaleH) / 4 - nArrow / 2;
        m_aArrow[0].SetExtent(arrowExtent);
        arrowExtent.x = nX + (nAspectWidth * 3 - nScaleH) / 4 - nArrow / 2;
        m_aArrow[1].SetExtent(arrowExtent);
    }
#endif

    for (int i = 0; i < 3; i++) {
        CSWGuiExtent iconExtent = m_aSlots[i].m_imageBg.m_extent;
        int nW = static_cast<int>(static_cast<float>(iconExtent.w) * 0.8f);
        int nH = static_cast<int>(static_cast<float>(iconExtent.h) * 0.8f);
        iconExtent.w = nW;
        iconExtent.h = nH;
        iconExtent.x += (m_aSlots[i].m_imageBg.m_extent.w - nW) / 2;
        iconExtent.y += (m_aSlots[i].m_imageBg.m_extent.h - nH) / 2;
        m_aSlots[i].m_imageIcon.SetExtent(iconExtent);
    }

    m_extent.x = nX;
    m_extent.y = nY;
    m_extent.w = nAspectWidth;
    m_extent.h = nScaleH;
}
#endif

void CSWGuiSkillFlow::Draw(float f) {
    for (int i = 0; i < 3; i++) {
        if (m_aSlots[i].m_nId == -1)
            continue;
        m_aSlots[i].m_imageBg.Draw(f);
        m_aSlots[i].m_imageIcon.Draw(f);
#if KOTOR1
        m_aSlots[i].m_border.Draw(f);  // K1: the slot's frame is drawn on top
#endif
    }
    for (int i = 0; i < 2; i++) {
        if (m_aSlots[i + 1].m_nId != -1)
            m_aArrow[i].Draw(f);
    }
}

void CSWGuiSkillFlow::SetSkillSelected(unsigned char bSlot, int nSelected) {
    if ((m_aSlots[bSlot].m_bSelected & 1) == nSelected) return;
    m_aSlots[bSlot].m_bSelected = (m_aSlots[bSlot].m_bSelected & ~1) | (nSelected & 1);
    SetSkillStatus(bSlot, m_aSlots[bSlot].m_bStatus);
}

// Kept out of line so the constant-slot callers make one call rather than a copy per site.
__attribute__((noinline))
void CSWGuiSkillFlow::SetSkillStatus(unsigned char bSlot, unsigned char bStatus) {
    Slot& s = m_aSlots[bSlot];
#if KOTOR1
    // K1: the status is shown on the slot's frame (colour and alpha, pulsing while selected);
    // the background image keeps its colour.
    s.m_border.m_params.m_fAlpha = 1.0f;
    s.m_bStatus = bStatus;
    s.m_imageIcon.m_params.m_fAlpha = 1.0f;
    s.m_imageBg.m_params.m_fAlpha = 1.0f;
    if (bSlot != 0)
        m_aArrow[bSlot - 1].m_params.m_fAlpha = 1.0f;
    switch (bStatus) {
        case SKILL_STATUS_AVAILABLE:
            s.m_imageIcon.m_params.m_fAlpha = 0.25f;
            if (s.m_bSelected & 1)
                s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
            else
                s.m_border.m_params.m_fAlpha = 0.0f;
            break;
        case SKILL_STATUS_OWNED:
        case SKILL_STATUS_GRANTED:
            s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_GREEN);
            if (!(s.m_bSelected & 1))
                s.m_border.m_params.m_fAlpha = 0.5f;
            break;
        case SKILL_STATUS_UNAVAILABLE:
            s.m_imageIcon.m_params.m_fAlpha = 0.25f;
            s.m_imageBg.m_params.m_fAlpha = 0.25f;
            if (s.m_bSelected & 1)
                s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
            else
                s.m_border.m_params.m_fAlpha = 0.0f;
            if (bSlot != 0)
                m_aArrow[bSlot - 1].m_params.m_fAlpha = 0.0f;
            break;
        case SKILL_STATUS_CHOSEN:
            s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_GREEN);
            break;
        case SKILL_STATUS_BONUS:
            if (s.m_bSelected & 1)
                s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
            else
                s.m_border.m_params.m_fAlpha = 0.0f;
            if (bSlot != 0)
                m_aArrow[bSlot - 1].m_params.m_fAlpha = 0.25f;
            break;
        case SKILL_STATUS_VIEW:
            if (s.m_bSelected & 1)
                s.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
            else
                s.m_border.m_params.m_fAlpha = 0.0f;
            break;
    }
    if (!(s.m_bSelected & 1))
        s.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    else
        s.m_border.m_params.SetPulsingAlpha(1, 1, 0);
#else
    s.m_imageIcon.m_params.m_fAlpha = 1.0f;
    s.m_bStatus = bStatus;
    s.m_imageBg.m_params.m_fAlpha = 1.0f;
    if (bSlot != 0)
        m_aArrow[bSlot - 1].m_params.m_fAlpha = 1.0f;
    switch (bStatus) {
        case SKILL_STATUS_AVAILABLE:
            s.m_imageIcon.m_params.m_fAlpha = 0.25f;
            s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_GREEN;
            break;
        case SKILL_STATUS_OWNED:
        case SKILL_STATUS_GRANTED:
            s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_TEXT;
            break;
        case SKILL_STATUS_UNAVAILABLE:
            s.m_imageIcon.m_params.m_fAlpha = 0.25f;
            s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_RED;
            if (bSlot != 0)
                m_aArrow[bSlot - 1].m_params.m_fAlpha = 0.0f;
            break;
        case SKILL_STATUS_CHOSEN:
            s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_HIGHLIGHT;
            break;
        case SKILL_STATUS_BONUS:
            s.m_imageBg.m_params.m_color = *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS);
            if (bSlot != 0)
                m_aArrow[bSlot - 1].m_params.m_fAlpha = 0.25f;
            break;
        case SKILL_STATUS_VIEW:
            if (s.m_bSelected & 1)
                s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_HIGHLIGHT;
            else
                s.m_imageBg.m_params.m_color = CGuiInGame::COLOR_NEW_GREEN;
            break;
    }
    if (!(s.m_bSelected & 1))
        s.m_imageBg.m_params.bHilight = 0;
    else
        s.m_imageBg.m_params.bHilight = 1;
#endif
}

int CSWGuiSkillFlow::SkillHitCheckMouse(int nX, int nY) {
    int nSlot = 0;
    for (int i = 0; i < 3; i++) {
        if (m_aSlots[i].m_nId == -1)
            break;
#if KOTOR1
        const CSWGuiExtent& extent = m_aSlots[i].m_border.m_extent;  // K1: the frame
#else
        const CSWGuiExtent& extent = m_aSlots[i].m_imageBg.m_extent;
#endif
        if (extent.y + extent.h >= nY && extent.y <= nY &&
            extent.x <= nX && extent.x + extent.w >= nX) {
            nSlot = i;
            break;
        }
    }
    return m_aSlots[nSlot].m_nId;
}

CSWGuiControl* CSWGuiSkillFlow::AsSkillFlow() {
    return this;
}
