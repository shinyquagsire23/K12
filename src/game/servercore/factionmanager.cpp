#include "nwsparty.h"
#include "factionmanager.h"
#include "exostring.h"
#include "resgff.h"
#include "nwsrules.h"
#include "CExoArrayListImpl.h"

CFactionManager::CFactionManager()
{
    m_pFactionPlayer = 0;
    m_pFactionHostile1 = 0;
    m_pFactionHostile2 = 0;
    m_pFactionFriendly1 = 0;
    m_pFactionFriendly2 = 0;
    m_pFactionNeutral = 0;
    m_pFactionInsane = 0;
    CExoArrayList<CSWSFaction*>* pFactions = new CExoArrayList<CSWSFaction*>();
    m_paReputations = 0;
    m_paFactions = pFactions;
    m_nNPCFactionCount = 0;
}

CFactionManager::~CFactionManager()
{
    CExoArrayList<CSWSFaction*>* p = m_paFactions;
    if (p != 0)
    {
        if (p->m_count >= 1)
        {
            int i = 0;
            do
            {
                CSWSFaction* pFaction = p->m_pArray[i];
                if (pFaction != 0)
                {
                    pFaction->~CSWSFaction();
                    operator delete(pFaction);
                    p = m_paFactions;
                }
                i++;
            } while (i < p->m_count);
        }
        if (p != 0)
        {
            delete p;
        }
    }
    if (m_paReputations != 0)
    {
        delete m_paReputations;
        m_paReputations = 0;
    }
}

CSWSFaction* CFactionManager::GetFaction(int nFactionId)
{
    if (nFactionId < 0)
        return 0;
    CExoArrayList<CSWSFaction*>* p = m_paFactions;
    if (p->m_count <= nFactionId)
        return 0;
    return p->m_pArray[nFactionId];
}

int CFactionManager::GetIsNPCFaction(int nFactionId)
{
    if (nFactionId < 1)
        goto FAIL;
    if (m_nNPCFactionCount < nFactionId)
        goto FAIL;
    return 1;
FAIL:
    return 0;
}

void CFactionManager::SetNPCFactionReputation(int nFactionId1, int nFactionId2, int nRep)
{
    if (nFactionId2 < 1)
        return;
    if (m_nNPCFactionCount < nFactionId2)
        return;
    if (nFactionId1 < 0)
        return;
    if (m_nNPCFactionCount < nFactionId1)
        return;
    CExoArrayList<int>* p = m_paReputations;
    int idx = m_nNPCFactionCount * nFactionId1 + nFactionId2 - 1u;
    if (idx >= p->m_count)
        return;
    int c = nRep < 0 ? 0 : nRep;
    c = c < 100 ? c : 100;
    p->m_pArray[idx] = c;
}

int CFactionManager::GetNPCFactionReputation(int nFactionId1, int nFactionId2)
{
    // An invalid id1 with a valid id2 retries with the arguments swapped.
    if ((nFactionId1 < 1 || m_nNPCFactionCount < nFactionId1)
        && (nFactionId2 < 1 || m_nNPCFactionCount < nFactionId2))
        return 50;
    if ((nFactionId1 < 1 || m_nNPCFactionCount < nFactionId1)
        && nFactionId2 >= 1 && nFactionId2 <= m_nNPCFactionCount)
        return GetNPCFactionReputation(nFactionId2, nFactionId1);
    int idx = m_nNPCFactionCount * nFactionId2 + nFactionId1 - 1u;
    if (idx >= 0 && idx < m_paReputations->m_count)
        return m_paReputations->m_pArray[idx];
    return 50;
}

void CFactionManager::DeleteFaction(int nFactionId)
{
    CExoArrayList<CSWSFaction*>* p = m_paFactions;
#ifdef K2_FIX_ENGINE_BUGS
    // Slots up to m_nNPCFactionCount are written by SaveFactions, so they must stay allocated.
    if (p == 0 || nFactionId <= m_nNPCFactionCount || nFactionId >= p->m_count)
        return;
#else
    if (p == 0)
        return;
#endif
    CSWSFaction* pFaction = p->m_pArray[nFactionId];
    if (pFaction == 0)
        return;
    pFaction->~CSWSFaction();
    operator delete(pFaction);
    m_paFactions->m_pArray[nFactionId] = 0;
}

void CFactionManager::ResolveFactionName(const CExoString& sName, CSWSFaction* pFaction)
{
    // CompareNoCase returns 1 on equal, so a non-zero result is a mismatch. Each slot is written
    // only by its own name; an unrecognised name stores nothing.
    CSWSFaction** pSlot;
    if (sName.CompareNoCase("player")) { pSlot = &m_pFactionPlayer; goto STORE; }
    if (sName.CompareNoCase("hostile_1")) { pSlot = &m_pFactionHostile1; goto STORE; }
    if (sName.CompareNoCase("friendly_1")) { pSlot = &m_pFactionFriendly1; goto STORE; }
    if (sName.CompareNoCase("hostile_2")) { pSlot = &m_pFactionHostile2; goto STORE; }
    if (sName.CompareNoCase("friendly_2")) { pSlot = &m_pFactionFriendly2; goto STORE; }
    if (sName.CompareNoCase("neutral")) { pSlot = &m_pFactionNeutral; goto STORE; }
    if (!sName.CompareNoCase("insane"))
        return;
    pSlot = &m_pFactionInsane;
STORE:
    *pSlot = pFaction;
}

int CFactionManager::GetFactionIdByName(const CExoString& sName)
{
    if (sName.CompareNoCase("hostile_1") != 0)
        return 1;
    if (sName.CompareNoCase("friendly_1") != 0)
        return 2;
    if (sName.CompareNoCase("friendly_2") != 0)
        return 4;
    if (sName.CompareNoCase("hostile_2") != 0)
        return 3;
    if (sName.CompareNoCase("neutral") != 0)
        return 5;
    if (sName.CompareNoCase("hostile_2") != 0)
        // A second "hostile_2" where "insane" was meant, so this arm is unreachable and "insane"
        // falls through to the table scan.
        return 6;

    CExoArrayList<CSWSFaction*>* p = m_paFactions;
    if (p == 0 || p->m_count < 1)
        return 0;
    int i = 0;
    do
    {
        CSWSFaction* pF = p->m_pArray[i];
        if (pF != 0)
        {
            if (pF->m_sName == sName)
                return i;
            p = m_paFactions;
        }
        i++;
    } while (i < p->m_count);
    return 0;
}

int CFactionManager::SaveFactions(CResGFF* pGFF, CResList* pList)
{
    if (m_nNPCFactionCount < 0)
        return 1;
    int i = 0;
    do
    {
        K2_ASSUME(i < m_paFactions->m_count);
        CSWSFaction* pFaction = (i < m_paFactions->m_count) ? m_paFactions->m_pArray[i] : 0;
        CResStruct s;
        pGFF->AddListElement(&s, pList, i);
        pGFF->WriteFieldCExoString(&s, pFaction->m_sName, const_cast<char*>("FactionName"));
        pGFF->WriteFieldDWORD(&s, pFaction->m_nParentFactionID, const_cast<char*>("FactionParentID"));
        pGFF->WriteFieldWORD(&s, static_cast<unsigned short>(pFaction->m_nGlobal), const_cast<char*>("FactionGlobal"));
    } while (i++ < m_nNPCFactionCount);
    return 1;
}

int CFactionManager::LoadReputationsFromSaveGame(CResGFF* pGFF, CResList* pList)
{
    LoadReputations();
    int bSuccess = 1;
    int count = pGFF->GetListCount(pList);
    if (count < 0)
        return 1;
    int i = 0;
    int result;
    do
    {
        CResStruct s;
        pGFF->GetListElement(&s, pList, i);
        int id1 = pGFF->ReadFieldDWORD(&s, const_cast<char*>("FactionID1"), bSuccess, 0);
        int id2 = pGFF->ReadFieldDWORD(&s, const_cast<char*>("FactionID2"), bSuccess, 0);
        unsigned int rep = pGFF->ReadFieldDWORD(&s, const_cast<char*>("FactionRep"), bSuccess, 0);
        if (id2 > 0 && m_nNPCFactionCount >= id2 && id1 >= 0 && m_nNPCFactionCount >= id1)
        {
            CExoArrayList<int>* pRep = m_paReputations;
            int idx = m_nNPCFactionCount * id1 + id2 - 1u;
            if (idx < pRep->m_count)
            {
                if (static_cast<int>(rep) < 0)
                    rep = 0;
                if (static_cast<int>(rep) > 100)
                    rep = 100;
                pRep->m_pArray[idx] = rep;
            }
        }
        result = bSuccess;
    } while (i++ < count && bSuccess);
    return result;
}

int CFactionManager::SaveReputations(CResGFF* pGFF, CResList* pList)
{
    if (m_nNPCFactionCount < 0)
        return 1;
    int i = 0;
    int id1 = 0;
    do
    {
        if (m_nNPCFactionCount >= 0)
        {
            int id2 = 0;
            do
            {
                int rep = GetNPCFactionReputation(id2, id1);
                if (rep != 100)
                {
                    CResStruct s;
                    pGFF->AddListElement(&s, pList, i);
                    pGFF->WriteFieldDWORD(&s, id1, const_cast<char*>("FactionID1"));
                    pGFF->WriteFieldDWORD(&s, id2, const_cast<char*>("FactionID2"));
                    pGFF->WriteFieldDWORD(&s, rep, const_cast<char*>("FactionRep"));
                    i++;
                }
            } while (id2++ < m_nNPCFactionCount);
        }
    } while (id1++ < m_nNPCFactionCount);
    return 1;
}

int CFactionManager::LoadFactionsFromSaveGame(CResGFF* pGFF, CResList* pList)
{
    CExoString sFactionName;
    int bSuccess = 1;
    unsigned int isGlobal;
    int count = pGFF->GetListCount(pList);
    int i;
    if (count > 0)
    {
        for (i = 0; i < count && bSuccess; i++)
        {
            CResStruct s;
            int parentId;
            pGFF->GetListElement(&s, pList, i);
            sFactionName = pGFF->ReadFieldCExoString(&s, const_cast<char*>("FactionName"), bSuccess, CExoString(""));
            parentId = pGFF->ReadFieldDWORD(&s, const_cast<char*>("FactionParentID"), bSuccess, 0);
            isGlobal = pGFF->ReadFieldWORD(&s, const_cast<char*>("FactionGlobal"), bSuccess, 0);
            if (bSuccess == 0)
            {
                isGlobal = 1;
                bSuccess = 1;
            }
            else
            {
                isGlobal = isGlobal & 0xffff;
            }
            CSWSFaction* pFaction = new CSWSFaction();
            pFaction->m_sName = sFactionName;
            pFaction->m_nGlobal = isGlobal;
            pFaction->m_nParentFactionID = parentId;
            ResolveFactionName(sFactionName, pFaction);
        }
    }
    LoadFactions(1);
    return bSuccess != 0;
}

int CFactionManager::LoadFactions(int bLoadSpecial)
{
    CExoString sName;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pRepute;
    int rowCount = pTable->m_nRows;
    int i = 0;
    if (bLoadSpecial != 0)
        i = m_paFactions->m_count;
    if (i < rowCount)
    {
        do
        {
            pTable->GetCExoStringEntry(i, "LABEL", &sName);
            CSWSFaction* pFaction = new CSWSFaction();
            pFaction->m_sName = sName;
            pFaction->m_nGlobal = 1;
            pFaction->m_nParentFactionID = -1;
            ResolveFactionName(sName, pFaction);
            i++;
        } while (rowCount != i);
    }
    return 1;
}

void CFactionManager::CreateDefaultFactions()
{
    m_pFactionPlayer = new CSWSFaction();
    m_pFactionHostile1 = new CSWSFaction();
    m_pFactionHostile2 = new CSWSFaction();
    m_pFactionFriendly1 = new CSWSFaction();
    m_pFactionFriendly2 = new CSWSFaction();
    m_pFactionNeutral = new CSWSFaction();
    m_pFactionInsane = new CSWSFaction();

    int total = (m_nNPCFactionCount + 1) * m_nNPCFactionCount;

    m_paReputations = new CExoArrayList<int>();
    m_paReputations->Add(0);
    for (int i = 1; i < total; i++)
        m_paReputations->Add(100);
    m_paReputations->SetSize(m_paReputations->m_count);
}

// No id1 >= 0 check here: id1 is a repute.2da row index and is never negative.
int CFactionManager::LoadReputations()
{
    int total = (m_nNPCFactionCount + 1) * m_nNPCFactionCount;
    if (m_paReputations != 0)
        delete m_paReputations;
    m_paReputations = new CExoArrayList<int>();
    for (int i = 0; i < total; i++)
        m_paReputations->Add(100);
    m_paReputations->SetSize(m_paReputations->m_count);

    C2DA* pTable = g_pRules->m_p2DArrays->m_pRepute;
    int rowCount = pTable->m_nRows;
    if (rowCount < 0)
        return 1;

    {
        int colCount = pTable->m_nColumns;
        int id1 = 0;
        int bSuccess = 1;
        do
        {
            if (bSuccess != 0 && colCount >= 1)
            {
                int id2 = 0;
                do
                {
                    int value;
                    bSuccess = pTable->GetINTEntry(id1, id2, &value);
                    if (id2 != 0 && m_nNPCFactionCount >= id1 && m_nNPCFactionCount >= id2)
                    {
                        CExoArrayList<int>* p = m_paReputations;
                        int idx = m_nNPCFactionCount * id1 + id2 - 1u;
                        if (idx < p->m_count)
                        {
                            int v = value < 0 ? 0 : value;
                            if (v > 100)
                                v = 100;
                            p->m_pArray[idx] = v;
                        }
                    }
                } while (bSuccess != 0 && ++id2 < colCount);
            }
        } while (bSuccess != 0 && id1++ < rowCount);
        return bSuccess;
    }
}
