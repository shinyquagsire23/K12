#include "k2stdint.h"
#include "nwsplayer.h"
#include "nwsplayerluo.h"
#include "nwsjournal.h"
#include "nwsobject.h"
#include "nwsarea.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "nwscreature.h"
#include "nwsrules.h"
#include "nwrules.h"
#include "gameobjectarray.h"
#include "nwmessagetypes.h"
#include "restype.h"
#include "partytables.h"
#include "nwsitem.h"
#include "CExoLinkedList.h"
#include "netlayer.h"
#include "nwupdatestats.h"
#include "exoresman.h"
#include "resgff.h"
#include "globalvartables.h"
#include "nwssendplayermessage.h"
#include "exofile.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "Vector.h"
#include "exobase.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "exostringinline.h"

CSWSPlayerInventoryGUI::CSWSPlayerInventoryGUI()
{
    m_bOpen = 0;
    m_pInventory = new CSWSPlayerLUOInventory;
}

CSWSPlayerInventoryGUI::~CSWSPlayerInventoryGUI()
{
    delete m_pInventory;
}

// The snapshot is discarded when the panel opens to closed, not every time it is told to close.
void CSWSPlayerInventoryGUI::SetOpen(int bOpen)
{
    if (bOpen == 0 && m_bOpen != 0)
    {
        m_pInventory->ClearSlots();
        m_pInventory->ClearRepository();
    }

    m_bOpen = bOpen;
}

int CSWSPlayer::GetIsAllowedToSave()
{
    return m_bIsPrimaryPlayer != 0;
}

CSWSPlayerContainerGUI::CSWSPlayerContainerGUI()
{
    m_bOpen = 0;
    m_objectId = CGameObjectArray::INVALIDOBJID;
}

// Only a change of viewed container makes the client's believed contents stale; re-opening the same
// one is exempt. The container list lives in the player's inventory snapshot, not here.
void CSWSPlayerContainerGUI::SetOpen(CSWSPlayer* pPlayer, unsigned long long objectId, int bOpen)
{
    if (m_objectId != objectId)
    {
        pPlayer->m_pInventoryGUI->m_pInventory->ClearContainer();
    }

    m_objectId = objectId;
    m_bOpen = bOpen;
}

CSWSObject* CSWSPlayer::GetGameObject()
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidSWSObject);

    if (pObject != 0 && pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA)
    {
        return static_cast<CSWSObject*>(pObject);
    }

    return 0;
}

// Compares the creatures' object ids rather than their addresses, so a creature re-fetched through a
// different path still compares equal.
int CSWSPlayer::PermittedToDisplayCharacterSheet(unsigned long long objectId)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;

    if (pServerApp == 0)
    {
        return 0;
    }

    CSWSCreature* pRequested = pServerApp->GetCreatureByGameObjectID(objectId);

    if (pRequested != 0)
    {
        CSWSCreature* pOwn =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidSWSObject);

        if (pOwn != 0)
        {
            return pOwn->m_objectId == pRequested->m_objectId;
        }
    }

    return 0;
}

void CSWSPlayer::ClearPlayerLastUpdateObject()
{
    if (m_pLastUpdateObject != 0)
    {
        m_pLastUpdateObject->ClearActionQueue();
        m_pLastUpdateObject->ClearKnownSpells();
        m_pLastUpdateObject->ClearKnownSpellUsesLeft();
        m_pLastUpdateObject->ClearSpellAddDeleteLists();
        m_pLastUpdateObject->ClearFeats();
    }
}

CSWSPlayerLastUpdateObject* CSWSPlayer::CreateNewPlayerLastUpdateObject()
{
    if (m_pLastUpdateObject != 0)
    {
        delete m_pLastUpdateObject;
        m_pLastUpdateObject = 0;
    }

    m_pLastUpdateObject = new CSWSPlayerLastUpdateObject;
    m_pLastUpdateObject->ClearKnownSpellUsesLeft();

    return m_pLastUpdateObject;
}

// Only the custom named bitmap carries a name; every other id selects a stock loading screen by index
// and leaves the string alone. The id itself is stored regardless.
void CSWSPlayer::SetAreaTransitionBMP(int transitionId, const CExoString& bmp)
{
    if (transitionId == 1)
    {
        m_sAreaTransitionBMP = bmp;
    }

    m_nAreaTransitionBMP = transitionId;
}

CLastUpdateObject* CSWSPlayer::GetLastUpdateObject(unsigned long long id)
{
    CExoLinkedListNode* pNode = m_pLastUpdateObjectList->m_pInternal->m_pHead;

    while (pNode != 0)
    {
        CLastUpdateObject* pLast =
            m_pLastUpdateObjectList->GetAtPos(pNode);

        if (pLast->m_objectId == id)
        {
            return pLast;
        }

        m_pLastUpdateObjectList->GetNext(pNode);
    }

    return 0;
}

CExoString CSWSPlayer::GetPlayerName()
{
    CNetLayerPlayerInfo* pInfo =
        g_pAppManager->m_pServerExoApp->GetNetLayer()->GetPlayerInfo(m_nPlayerId);

    if (pInfo != 0)
    {
        return pInfo->m_sName;
    }

    return "";
}

// Points the sheet at a different creature. The snapshot is thrown away and rebuilt only on a real
// change of id, which keeps a redundant full sheet update from going out every frame; a sheet that
// was never given a snapshot just records the new id.
void CSWSPlayerCharSheetGUI::SetCreatureDisplayed(unsigned long long objectId)
{
    if (m_objectId != objectId && m_pStatsUpdate != 0)
    {
        delete m_pStatsUpdate;
        m_pStatsUpdate = new CSWCreatureStatsUpdate;
    }

    m_objectId = objectId;
}

// Returns the character-sheet change mask for one group of the sheet: 0 the stats block, 1 the skill
// ranks, 2 the feat lists, and any other value, the sentinel that clears the icons, returns 0. The
// armour-class term only matters when the sheet shows a creature other than the one this player
// drives, since a player's own AC rides on the ordinary per-object update.
int CSWSPlayerCharSheetGUI::ComputeCharacterSheetUpdateRequired(CSWSPlayer* pPlayer)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_objectId);
    if (pCreature == 0)
        return 0;

    int bViewingOther = 0;
    if (pPlayer->GetGameObject() != 0)
        bViewingOther = (pPlayer->GetGameObject()->m_objectId != m_objectId);

    CSWSCreatureStats* pStats = pCreature->m_pStats;
    CSWCreatureStatsUpdate* pUpdate = m_pStatsUpdate;

    switch (m_nAction)
    {
        case ACTION_STATS:
        {
            // One accumulator per group of six ability terms, merged before the DEX-AC term.
            bool bStatsChanged = (pStats->GetSTRStat() != pUpdate->m_anAbilityStats[0]);
            bStatsChanged |= (pStats->GetDEXStat() != pUpdate->m_anAbilityStats[1]);
            bStatsChanged |= (pStats->GetCONStat() != pUpdate->m_anAbilityStats[2]);
            bStatsChanged |= (pStats->GetINTStat() != pUpdate->m_anAbilityStats[3]);
            bStatsChanged |= (pStats->GetWISStat() != pUpdate->m_anAbilityStats[4]);
            bStatsChanged |= (pStats->GetCHAStat() != pUpdate->m_anAbilityStats[5]);
            bool bBasesChanged = (pStats->m_nSTRBase != pUpdate->m_anAbilityBases[0]);
            bBasesChanged |= (pStats->m_nDEXBase != pUpdate->m_anAbilityBases[1]);
            bBasesChanged |= (pStats->m_nCONBase != pUpdate->m_anAbilityBases[2]);
            bBasesChanged |= (pStats->m_nINTBase != pUpdate->m_anAbilityBases[3]);
            bBasesChanged |= (pStats->m_nWISBase != pUpdate->m_anAbilityBases[4]);
            bBasesChanged |= (pStats->m_nCHABase != pUpdate->m_anAbilityBases[5]);
            bStatsChanged |= bBasesChanged;
            bStatsChanged |= (static_cast<char>(pStats->GetDEXMod(1)) - static_cast<char>(pStats->GetDEXMod(0))
                              != pUpdate->m_nDexACBonus);
            bool bBonusesChanged = (static_cast<unsigned char>(pStats->GetTotalSTRBonus())
                                    != pUpdate->m_anAbilityEffectBonuses[0]);
            bBonusesChanged |= (static_cast<unsigned char>(pStats->GetTotalDEXBonus())
                                != pUpdate->m_anAbilityEffectBonuses[1]);
            bBonusesChanged |= (static_cast<unsigned char>(pStats->GetTotalCONBonus())
                                != pUpdate->m_anAbilityEffectBonuses[2]);
            bBonusesChanged |= (static_cast<unsigned char>(pStats->GetTotalINTBonus())
                                != pUpdate->m_anAbilityEffectBonuses[3]);
            bBonusesChanged |= (static_cast<unsigned char>(pStats->GetTotalWISBonus())
                                != pUpdate->m_anAbilityEffectBonuses[4]);
            bBonusesChanged |= (static_cast<unsigned char>(pStats->GetTotalCHABonus())
                                != pUpdate->m_anAbilityEffectBonuses[5]);

            int nUpdate = bStatsChanged | bBonusesChanged;

            bool bFortChanged =
                (static_cast<unsigned char>(pStats->GetFortSavingThrow(0)) != pUpdate->m_nFortSave);
            bool bWillChanged =
                (static_cast<unsigned char>(pStats->GetWillSavingThrow(0)) != pUpdate->m_nWillSave);
            bool bReflexChanged =
                (static_cast<unsigned char>(pStats->GetReflexSavingThrow(0)) != pUpdate->m_nReflexSave);
            bool bExperienceChanged = (pStats->m_nExperience != pUpdate->m_nExperience);

            nUpdate |= (bFortChanged << 1)
                | (bWillChanged << 2)
                | (bReflexChanged << 3)
                | (bExperienceChanged << 4);

            if (pStats->GetBaseAttackBonus() != pUpdate->m_nBaseAttackBonus
                || pStats->GetUseMonkAttackTables(0) != pUpdate->m_bMonkUnarmed)
            {
                nUpdate |= MSG_CHARACTER_SHEET_BASE_ATTACK_BONUS;
            }

            if (*pStats->m_pCombatInformation != *pUpdate->m_pCombatInformation)
                nUpdate |= MSG_CHARACTER_SHEET_COMBAT_INFORMATION;

            if (bViewingOther)
            {
                unsigned short nArmorClass = pCreature->GetArmorClass();
                if (nArmorClass != pUpdate->m_nArmorClass)
                    nUpdate |= MSG_CHARACTER_SHEET_ARMOR_CLASS;
            }

            return nUpdate;
        }

        case ACTION_SKILLS:
        {
            pUpdate->m_nSkillsChanged = 0;

            for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
            {
                if (static_cast<unsigned char>(pStats->GetSkillRank(i, 0, 0))
                    != pUpdate->m_paSkillRanks[i])
                {
                    pUpdate->m_nSkillsChanged |= 1 << i;
                }
            }

            return (pUpdate->m_nSkillsChanged != 0) << 7;
        }

        case ACTION_FEATS:
        {
            int bFeatsChanged = (pCreature->m_objectId != pUpdate->m_objectId);
            if (static_cast<unsigned short>(pStats->m_feats.m_count) != pUpdate->m_feats.m_count)
                bFeatsChanged = 1;
            if (static_cast<unsigned short>(pStats->m_bonusFeats.m_count) != pUpdate->m_bonusFeats.m_count)
                bFeatsChanged = 1;

            // Both loops walk the whole snapshot list, even after a feat is found missing.
            if (bFeatsChanged == 0)
            {
                for (unsigned short i = 0;
                     i < static_cast<unsigned short>(pCreature->m_pStats->m_feats.m_count); i++)
                {
                    unsigned short nFeat = pCreature->m_pStats->GetFeat(i);
                    int bFound = 0;
                    for (int j = 0; j < pUpdate->m_feats.m_count; j++)
                    {
                        if (pUpdate->m_feats.m_pArray[j] == nFeat)
                            bFound = 1;
                    }
                    if (bFound == 0)
                        bFeatsChanged = 1;
                }
            }

            if (bFeatsChanged == 0)
            {
                for (unsigned short i = 0;
                     i < static_cast<unsigned short>(pCreature->m_pStats->m_bonusFeats.m_count); i++)
                {
                    unsigned short nFeat = pCreature->m_pStats->GetBonusFeat(i);
                    int bFound = 0;
                    for (int j = 0; j < pUpdate->m_bonusFeats.m_count; j++)
                    {
                        if (pUpdate->m_bonusFeats.m_pArray[j] == nFeat)
                            bFound = 1;
                    }
                    if (bFound == 0)
                        bFeatsChanged = 1;
                }
            }

            if (bFeatsChanged == 1)
            {
                unsigned long long objectId = pCreature->m_objectId;
                if (pPlayer->GetGameObject() != 0)
                {
                    return (pPlayer->GetGameObject()->m_objectId != objectId) << 9;
                }
            }

            return 0;
        }
    }

    return 0;
}

CSWSPlayerCharSheetGUI::CSWSPlayerCharSheetGUI()
{
    m_objectId = OBJECT_INVALID;
    m_pStatsUpdate = new CSWCreatureStatsUpdate;
    m_nAction = -1;
    m_nReserved1 = 1;
}

CSWSPlayerCharSheetGUI::~CSWSPlayerCharSheetGUI()
{
    delete m_pStatsUpdate;
}

// Tears down seven owned objects in order, nulling each behind itself. Only the per-object snapshot
// list owns its elements, so those are pumped out through RemoveHead first: the list destructor frees
// the backing internal list, never the payloads. The party list holds no separate records, so a bare
// delete is enough.
CSWSPlayer::~CSWSPlayer()
{
    delete m_pLastUpdatePartyObjectList;
    m_pLastUpdatePartyObjectList = 0;

    if (m_pLastUpdateObjectList != 0)
    {
        while (m_pLastUpdateObjectList->m_pInternal->m_pHead != 0)
        {
            CLastUpdateObject* pLast =
                m_pLastUpdateObjectList->RemoveHead();

            if (pLast != 0)
            {
                delete pLast;
            }
        }

        delete m_pLastUpdateObjectList;
        m_pLastUpdateObjectList = 0;
    }

    delete m_pCharSheetGUI;
    m_pCharSheetGUI = 0;

    delete m_pJournalState;
    m_pJournalState = 0;

    delete m_pInventoryGUI;
    m_pInventoryGUI = 0;

    delete m_pContainerGUI;
    m_pContainerGUI = 0;

    delete m_pLastUpdateObject;
    m_pLastUpdateObject = 0;
}


// Re-points the player at a different driven object and does the three pieces of bookkeeping that
// makes necessary: move the area player-count, invalidate the panel snapshots, and re-parent the
// party's shared item store. A null argument re-arms the invalid object id.
void CSWSPlayer::SetGameObject(CSWSObject* pObject)
{
    CSWSArea* pOldArea = 0;
    unsigned long long oidOld = m_oidSWSObject;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;

    if (pServerApp != 0)
    {
        CGameObject* pOld = pServerApp->GetGameObject(oidOld);

        if (pOld != 0 && pOld->m_objectType > CGameObject::OBJECT_TYPE_AREA)
        {
            pOldArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pOld)->GetArea());
        }
    }

    CSWSArea* pNewArea = 0;

    if (pObject == 0)
    {
        m_oidSWSObject = OBJECT_INVALID;
    }
    else
    {
        unsigned long long oidNew = pObject->m_objectId;
        m_oidSWSObject = oidNew;

        CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;

        if (pApp != 0)
        {
            CGameObject* pNew = pApp->GetGameObject(oidNew);

            if (pNew != 0 && pNew->m_objectType > CGameObject::OBJECT_TYPE_AREA)
            {
                pNewArea = static_cast<CSWSArea*>(static_cast<CSWSObject*>(pNew)->GetArea());
            }
        }

        if (oidOld != oidNew)
        {
            m_pJournalState->m_bFullUpdateNeeded = 1;

            if (m_pLastUpdateObject != 0)
            {
                m_pLastUpdateObject->ClearActionQueue();
                m_pLastUpdateObject->ClearKnownSpells();
                m_pLastUpdateObject->ClearKnownSpellUsesLeft();
                m_pLastUpdateObject->ClearSpellAddDeleteLists();
                m_pLastUpdateObject->ClearFeats();
            }
        }

        if (g_pAppManager->m_pServerExoApp != 0
            && g_pAppManager->m_pServerExoApp->GetPartyTable() != 0
            && g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems() != 0)
        {
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->SetParentId(
                m_oidSWSObject);
        }
    }

    if (pOldArea != pNewArea)
    {
        if (pNewArea != 0)
        {
            pNewArea->IncrementPlayersInArea();
        }

        if (pOldArea != 0)
        {
            pOldArea->DecrementPlayersInArea();
        }
    }
}

// Six owned objects allocated up front, and every id field armed to the invalid sentinel. The journal
// state takes no parentheses on purpose: the class declares no constructor, so a value-initializing
// new would zero the flags the two stores below set.
CSWSPlayer::CSWSPlayer(unsigned int nPlayerId)
    : CSWSClient(nPlayerId)
{
    m_bCheatToggle = 0;
    m_connectionState = 0;
    m_oidSWSObject = CGameObjectArray::INVALIDOBJID;
    m_lastGameObjUpdateTime = 0;
    m_lastKnownAreaId = CGameObjectArray::INVALIDOBJID;

    m_pLastUpdateObjectList = new CExoLinkedList<CLastUpdateObject>;
    m_pLastUpdatePartyObjectList = new CExoLinkedList<CLastUpdatePartyObject>;
    m_pCharSheetGUI = new CSWSPlayerCharSheetGUI;

    m_nAreaTransitionBMP = -1;
    m_sAreaTransitionBMP = "";

    m_pJournalState = new CSWSPlayerJournalState;

    m_pInventoryGUI = new CSWSPlayerInventoryGUI;
    m_pContainerGUI = new CSWSPlayerContainerGUI;

    m_bIsPrimaryPlayer = 1;
    m_bRestoredPosition = 0;
    m_playerIndex = -1;
    m_bInputModeReceived = 1;
    m_state = 0;
    m_resRef = const_cast<char*>("");
    m_characterLoaded = 0;
    m_gameObjectId = CGameObjectArray::INVALIDOBJID;
    m_pLastUpdateObject = 0;
    m_bPlayModuleListingCharacters = 0;
    m_saveListIndex = 0;
}

// Returns the player to its just-constructed state without destroying it: every owned object is
// deleted and immediately rebuilt, so the same player object can be handed to the next module load.
// Each rebuild is guarded, so a null member stays null rather than gaining a replacement. The
// per-object snapshot list is the exception: its elements are freed but the list itself is kept,
// because the party list holds no owned payloads and can simply be replaced.
void CSWSPlayer::ClearPlayerOnDestroyGame()
{
    if (m_pInventoryGUI->m_pInventory != 0)
    {
        m_pInventoryGUI->m_pInventory->ClearContainer();
    }

    if (m_pLastUpdatePartyObjectList != 0)
    {
        delete m_pLastUpdatePartyObjectList;
        m_pLastUpdatePartyObjectList = new CExoLinkedList<CLastUpdatePartyObject>;
    }

    if (m_pLastUpdateObjectList != 0)
    {
        while (m_pLastUpdateObjectList->m_pInternal->m_pHead != 0)
        {
            CLastUpdateObject* pLast =
                m_pLastUpdateObjectList->RemoveHead();

            if (pLast != 0)
            {
                delete pLast;
            }
        }
    }

    if (m_pCharSheetGUI != 0)
    {
        delete m_pCharSheetGUI;
        m_pCharSheetGUI = new CSWSPlayerCharSheetGUI;
    }

    if (m_pContainerGUI != 0)
    {
        delete m_pContainerGUI;
        m_pContainerGUI = new CSWSPlayerContainerGUI;
    }

    if (m_pJournalState != 0)
    {
        delete m_pJournalState;
        m_pJournalState = new CSWSPlayerJournalState;
    }

    if (m_pInventoryGUI != 0)
    {
        delete m_pInventoryGUI;
        m_pInventoryGUI = new CSWSPlayerInventoryGUI;
    }
}

// The early-out paths skip CRes::Release(): Demand and Release are balanced only on the success path,
// which looks like a leak but is what the engine does on both branches.
int CSWSPlayer::GetCharacterInfoFromIFO(unsigned char& nClasses, int* pnClassIds,
                                        unsigned char* pnClassLevels, unsigned int& nExperience)
{
    CResStruct topStruct;
    CResStruct playerStruct;
    CResStruct classStruct;
    CResList   playerList;
    CResList   classList;
    int        bSuccess;
    int        bResult;

    CRes* pRes = g_pExoResMan->GetResObject(CResRef("Module"), RESTYPE_IFO);

    if (pRes == 0)
    {
        return 0;
    }

    pRes->Demand();

    CResGFF* pGff = static_cast<CResGFF*>(pRes);
    pGff->GetTopLevelStruct(&topStruct);

    if (pGff->GetList(&playerList, &topStruct, const_cast<char*>("Mod_PlayerList"))
        && pGff->GetListElement(&playerStruct, &playerList, m_playerIndex))
    {
        nExperience = pGff->ReadFieldDWORD(&playerStruct, const_cast<char*>("Experience"), bSuccess, 0);

        if (pGff->GetList(&classList, &playerStruct, const_cast<char*>("ClassList")) == 1)
        {
            nClasses = static_cast<unsigned char>(pGff->GetListCount(&classList));

            for (unsigned int i = 0; i < nClasses; i++)
            {
                pGff->GetListElement(&classStruct, &classList, i);
                pnClassIds[i] = pGff->ReadFieldINT(&classStruct, const_cast<char*>("Class"), bSuccess, -1);
                pnClassLevels[i] = static_cast<unsigned char>(
                    pGff->ReadFieldSHORT(&classStruct, const_cast<char*>("ClassLevel"), bSuccess, 0));
            }
        }

        pRes->Release();
        bResult = 1;
    }
    else
    {
        bResult = 0;
    }

    g_pExoResMan->ReleaseResObject(pRes);

    return bResult;
}

// Reads a .bic, a "BIC " GFF, through the resource manager and stamps every field of it onto the
// creature. The GFF reader is heap-allocated and hand-freed on both exits. The stats block is read
// first and is the only step that can fail; its result short-circuits the rest of the load.
// DetectMode is read and discarded, since detection is re-enabled unconditionally on load, unlike
// StealthMode two statements later.
int64_t CSWSPlayer::LoadCreatureData(CResRef resRef, CSWSCreature* pCreature)
{
    CResStruct topStruct;
    int        bSuccess;

    CResGFF* pGff = new CResGFF(RESTYPE_BIC, const_cast<char*>("BIC "), resRef);

    if (pGff->m_nValid != 0)
    {
        SetFileName(resRef);
        pGff->GetTopLevelStruct(&topStruct);

        int64_t nResult =
            pCreature->m_pStats->ReadStatsFromGff(pGff, &topStruct, &pCreature->m_appearance);

        if (nResult != 0)
        {
            pGff->Dump(0);
            delete pGff;

            return nResult;
        }

        pCreature->SetCurrentHitPoints(pCreature->GetMaxHitPoints(1));

        pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("DetectMode"), bSuccess, 0);
        pCreature->SetActivity(CSWSCreature::ACTIVITY_DETECT, 1, 0);

        pCreature->SetActivity(
            1, pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("StealthMode"), bSuccess, 0), 0);

        pCreature->m_nCreatureSize =
            pGff->ReadFieldINT(&topStruct, const_cast<char*>("CreatureSize"), bSuccess, 3);

        pCreature->m_bDisguised =
            pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PM_IsDisguised"), bSuccess, 0);

        if (pCreature->m_bDisguised != 0)
        {
            pCreature->m_nDisguiseAppearance =
                pGff->ReadFieldWORD(&topStruct, const_cast<char*>("PM_Appearance"), bSuccess, 0);
        }

        pCreature->ReadScriptsFromGff(pGff, &topStruct);
        pCreature->ReadItemsFromGff(pGff, &topStruct, 0, 1, 0);
        pCreature->m_pStats->ReadSpellsFromGff(pGff, &topStruct);
        pCreature->PostProcess();
    }

    pGff->Dump(0);
    delete pGff;

    return 0;
}

// Handles the multiplayer and server-vault half of character loading. Builds a bare creature,
// drives the player with it, then fills it from the vault's copy of the .bic. Re-adding the
// server-vault directory is not a no-op: AddResourceDirectory re-indexes it, which is how a .bic
// written since the last load is picked up.
int64_t CSWSPlayer::LoadServerCharacter(CResRef resRef)
{
    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 1);

    SetGameObject(pCreature);

    m_gameObjectId = pCreature->m_objectId;
    m_state = 3;

    g_pExoResMan->RemoveResourceDirectory(CExoString("SERVERVAULT:"));
    g_pExoResMan->AddResourceDirectory(CExoString("SERVERVAULT:"));

    int64_t nResult = LoadCreatureData(resRef, pCreature);

    if (nResult == 0)
    {
        return 0;
    }

    SetGameObject(0);
    m_gameObjectId = OBJECT_INVALID;

    delete pCreature;

    return nResult;
}

// Loads the local character, LoadServerCharacter's single-player twin. The resref is the hard-wired
// scratch character the character generator writes, and no resource-directory re-index is needed
// because the file is already on the normal search path. On top of the load, the PC's total level,
// summed across every class since the engine has no single level field, and alignment are published
// to the script-visible global table as G_PC_LEVEL and G_PC_Align_Val. Both jobs run even when the
// load failed: the teardown is the last thing here, not an early return.
int64_t CSWSPlayer::LoadLocalCharacter()
{
    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 1);

    SetGameObject(pCreature);

    m_gameObjectId = pCreature->m_objectId;
    m_state = 1;

    int64_t nResult = LoadCreatureData(CResRef("TEMP_CHAR"), pCreature);

    pCreature->SetInParty(1, 1);

#if !KOTOR1  // K1 publishes no G_PC_LEVEL / G_PC_Align_Val globals
    int i;
    int nTotalLevel = 0;

    for (i = 0; i < pCreature->m_pStats->m_numClasses; i++)
    {
        nTotalLevel += pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0);
    }

    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
        CExoString("G_PC_LEVEL"), static_cast<char>(nTotalLevel));
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
        CExoString("G_PC_Align_Val"), static_cast<char>(pCreature->m_pStats->m_nAlignment));
#endif

    if (nResult != 0)
    {
        SetGameObject(0);
        m_gameObjectId = OBJECT_INVALID;

        delete pCreature;
    }

    return nResult;
}

// Serialises the player's creature into the outgoing character-download message. There is no in-memory
// GFF-to-buffer path, so the round trip goes through the filesystem: build a "BIC " GFF, write it to a
// scratch resource, mount that prefix as a resource directory, read the file straight back as bytes,
// ship the bytes, then unmount and delete the file again. A disguised PC is saved under their real
// appearance, so the disguise effect is stripped first, stopping at the first match.
int CSWSPlayer::PackCreatureIntoMessage()
{
    int bReturn = 0;

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();

    CResGFF*    pGff    = new CResGFF;
    CResStruct* pStruct = new CResStruct();

    if (pGff->CreateGFFFile(pStruct, CExoString("BIC "), CExoString("V2.0")))
    {
        CExoString sFileName;
        CExoString sUnused;
        CSWSCreature* pCreature;
        unsigned int nSize;
        unsigned char* pData;
        CExoFile* pFile;

        pCreature = GetGameObject()->AsSWSCreature();

        if (pCreature != 0)
        {
            if (pCreature->m_bDisguised == 1)
            {
                for (int i = 0; i < pCreature->m_effects.m_count; i++)
                {
                    if (pCreature->m_effects.m_pArray[i]->m_subType
                        == CSWSEffectListHandler::EFFECT_DISGUISE)
                    {
                        pCreature->m_effects.m_pArray[i]->SetInteger(2, 1);
                        pCreature->RemoveEffectById(pCreature->m_effects.m_pArray[i]->m_effectId);
                        break;
                    }
                }

                CSWSCreatureStats* pStats = pCreature->m_pStats;

                pStats->UpdateCombatInformation();
            }

            pMessage->CreateWriteMessage(4096, MSG_PLAYERID_ALL_CLIENTS, 1);
            pCreature->SaveCreature(pGff, pStruct);

            sFileName = "TEMP:temp_charfile";
            pGff->WriteGFFFile(sFileName, RESTYPE_BIC);

            g_pExoResMan->AddResourceDirectory(CExoString("TEMP:"));

            pFile = new CExoFile(sFileName, RESTYPE_BIC, CExoString("rb"));

            if (pFile->FileOpened())
            {
                nSize = pFile->GetSize();
                pData = new unsigned char[nSize];

                pFile->Read(pData, 1, nSize);

                delete pFile;

                pMessage->WriteCExoLocStringServer(pCreature->GetFirstName(), 0);
                pMessage->WriteCExoLocStringServer(pCreature->GetLastName(), 0);
                pMessage->WriteDWORD(nSize, 32);
                pMessage->WriteVOIDPtr(pData, nSize);

                g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));
                g_pExoResMan->RemoveFile(sFileName, RESTYPE_BIC);

#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the new[] buffer with scalar delete
                delete pData;
#else
                delete[] pData;
#endif

                bReturn = 1;
            }
            else
            {
                delete pFile;
            }
        }
    }

    delete pStruct;
    delete pGff;

    return bReturn;
}

// Restores the player's creature straight out of an IFO Mod_PlayerList entry, the resume-a-saved-game
// and hand-the-module-back-the-party-leader path. An index of -1 means the scratch copy: mount the
// scratch prefix, read the IFO, and manufacture and install an empty GFF under that name if the
// resource manager has never seen it. Any other index reads the live module's own IFO. The saved area
// id and position are stamped onto the creature only on the module-IFO path; the orientation is
// applied on both.
int CSWSPlayer::LoadCharacterFromIFO(unsigned int playerIndex, int bRestorePosition,
                                     int bPostProcess)
{
    CResStruct   topStruct;
    CResStruct   playerStruct;
    CResList     playerList;
    Vector       vOrientation;
    int          bSuccess;
    unsigned int nIndex;
    int          bNewGff  = 0;
    int          bTempDir = 0;
    CResGFF*     pGff;

    if (playerIndex != 0xffffffff)
    {
        nIndex = playerIndex;
        pGff = static_cast<CResGFF*>(g_pExoResMan->GetResObject(CResRef("Module"), RESTYPE_IFO));
    }
    else
    {
        g_pExoResMan->AddResourceDirectory(CExoString("TEMP:"));

        nIndex = m_saveListIndex;
        pGff = static_cast<CResGFF*>(g_pExoResMan->GetResObject(CResRef("pifo"), RESTYPE_IFO));

        if (pGff == 0)
        {
            pGff = new CResGFF;
            g_pExoResMan->SetResObject(CResRef("pifo"), RESTYPE_IFO, pGff);
            pGff->SetFileType("IFO ");

            bNewGff = 1;
        }

        bTempDir = 1;
    }

    if (pGff == 0)
    {
        return 0;
    }

    pGff->Demand();
    pGff->GetTopLevelStruct(&topStruct);

    if (pGff->GetList(&playerList, &topStruct, const_cast<char*>("Mod_PlayerList")) == 0
        || pGff->GetListElement(&playerStruct, &playerList, nIndex) == 0)
    {
        pGff->Release();
        g_pExoResMan->ReleaseResObject(pGff);

        return 0;
    }

    unsigned long long objectId =
        pGff->ReadFieldDWORD(&playerStruct, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);

    CSWSCreature* pCreature = new CSWSCreature(objectId, 1);

    SetGameObject(pCreature);

    m_gameObjectId = pCreature->m_objectId;

    unsigned int areaId = pGff->ReadFieldDWORD(&playerStruct, const_cast<char*>("AreaId"), bSuccess, 0);

    pCreature->SetInParty(1, 1);

    if (pCreature->m_pStats->ReadStatsFromGff(pGff, &playerStruct, &pCreature->m_appearance) != 0)
    {
        g_pExoResMan->ReleaseResObject(pGff);

        delete pCreature;

        return 0;
    }

    pCreature->LoadJournal(pGff, &playerStruct);

    pGff->ReadFieldBYTE(&playerStruct, const_cast<char*>("DetectMode"), bSuccess, 0);
    pCreature->SetActivity(CSWSCreature::ACTIVITY_DETECT, 1, 0);

    pCreature->SetActivity(
        1, pGff->ReadFieldBYTE(&playerStruct, const_cast<char*>("StealthMode"), bSuccess, 0), 0);

    pCreature->m_nCreatureSize =
        pGff->ReadFieldINT(&playerStruct, const_cast<char*>("CreatureSize"), bSuccess, 3);

    pCreature->ReadItemsFromGff(pGff, &playerStruct, !bTempDir, 1, 0);
    pCreature->LoadObjectState(pGff, &playerStruct);
    pCreature->m_pStats->ReadSpellsFromGff(pGff, &playerStruct);

    pCreature->m_bDisguised =
        pGff->ReadFieldBYTE(&playerStruct, const_cast<char*>("PM_IsDisguised"), bSuccess, 0);

    if (pCreature->m_bDisguised != 0)
    {
        pCreature->m_nDisguiseAppearance =
            pGff->ReadFieldWORD(&playerStruct, const_cast<char*>("PM_Appearance"), bSuccess, 0);
    }

    pCreature->ReadScriptsFromGff(pGff, &playerStruct);

    float fX = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("XPosition"), bSuccess, 0.0f);
    float fY = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("YPosition"), bSuccess, 0.0f);
    float fZ = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("ZPosition"), bSuccess, 0.0f);

    vOrientation.x = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("XOrientation"), bSuccess, 0.0f);
    vOrientation.y = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("YOrientation"), bSuccess, 0.0f);
    vOrientation.z = pGff->ReadFieldFLOAT(&playerStruct, const_cast<char*>("ZOrientation"), bSuccess, 0.0f);

    pCreature->SetOrientation(vOrientation);

    m_bRestoredPosition = bTempDir == 0 && bRestorePosition != 0;

    if (bPostProcess)
    {
        pCreature->PostProcess();
    }

    pGff->Release();
    g_pExoResMan->ReleaseResObject(pGff);

    if (bNewGff)
    {
        delete pGff;
    }

    if (bTempDir)
    {
        g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));
    }
    else
    {
        pCreature->m_currentAreaId = areaId;
        pCreature->m_vDesiredAreaLocation.x = fX;
        pCreature->m_vDesiredAreaLocation.y = fY;
        pCreature->m_vDesiredAreaLocation.z = fZ;
        pCreature->m_bDesiredAreaUpdateComplete = 0;
    }

    return 1;
}

// Writes the player's creature back into the server vault as a .bic and remembers the resref it used.
// Only valid in save states 3 and 4. If the player already has a resref the file is overwritten in
// place; otherwise, or if that write fails, a unique name is invented by slugifying the creature's full
// name, listing the vault directory and appending 1, 2, 3, ... until nothing collides, trimming the base
// name as the suffix grows to stay at 16 characters. The disguise effect is stripped as in
// PackCreatureIntoMessage, minus the combat-info update. SaveCreature refuses to write a plot object,
// so the plot flag is cleared around the save for a creature in transit and re-armed on both exits.
int CSWSPlayer::SaveServerCharacter()
{
    CExoString sFileName;
    CExoString sPath;
    CExoString sVaultDir;
    CExoString sBaseName;
    CExoString sSuffix;
    CExoArrayList<CExoString> vaultFiles;
    CSWSCreature* pCreature;
    int bWritten;
    CResGFF* pGff;
    CResStruct* pStruct;
    int nSuffix;

    if (m_state != 3 && m_state != 4)
    {
        return 0;
    }

    pCreature = GetGameObject()->AsSWSCreature();

    if (pCreature == 0)
    {
        return 0;
    }

    if (pCreature->m_bDisguised == 1)
    {
        for (int i = 0; i < pCreature->m_effects.m_count; i++)
        {
            if (pCreature->m_effects.m_pArray[i]->m_subType
                == CSWSEffectListHandler::EFFECT_DISGUISE)
            {
                pCreature->m_effects.m_pArray[i]->SetInteger(2, 1);
                pCreature->RemoveEffectById(pCreature->m_effects.m_pArray[i]->m_effectId);
                break;
            }
        }
    }

    pGff = new CResGFF;
    pStruct = new CResStruct();

    if (pGff->CreateGFFFile(pStruct, CExoString("BIC "), CExoString("V2.0")) == 0)
    {
        delete pGff;
        delete pStruct;

        return 0;
    }

    // Both failure exits jump to the blocks at the bottom: deleting the GFF runs a virtual destructor,
    // so a copy of the teardown here would sit between the two calls that follow and split their
    // cleanup range.
    if (pCreature->m_inTransit != 0)
    {
        pCreature->m_bPlot = 0;

        if (pCreature->SaveCreature(pGff, pStruct) == 0)
        {
            goto failInTransit;
        }

        pCreature->m_bPlot = 1;
    }
    else
    {
        if (pCreature->SaveCreature(pGff, pStruct) == 0)
        {
            goto fail;
        }
    }

    delete pStruct;

    bWritten = 0;

    if (m_resRef != const_cast<char*>(""))
    {
        m_resRef.CopyToString(sFileName);

        if (m_state == 4)
        {
#if KOTOR1  // K1 (Android port): "/" separators
            sPath.Format(const_cast<char*>("SERVERVAULT:%s%s%s"), GetPlayerName().CStr(), "/",
#else
            sPath.Format(const_cast<char*>("SERVERVAULT:%s%s%s"), GetPlayerName().CStr(), "\\",
#endif
                         sFileName.CStr());
        }
        else
        {
            sPath.Format(const_cast<char*>("SERVERVAULT:%s"), sFileName.CStr());
        }

        if (pGff->WriteGFFFile(sPath, RESTYPE_BIC))
        {
            SetFileName(CResRef(sFileName));
            bWritten = 1;
        }
    }

    if (bWritten == 0)
    {
        m_state = 4;

        sVaultDir.Format(const_cast<char*>("SERVERVAULT:%s"), GetPlayerName().CStr());

        sBaseName = pCreature->m_pStats->GetFullName();
        sBaseName.StripNonAlphaNumeric(1, 0, 0);

        if (sBaseName[0] != 0)
        {
            sBaseName = sBaseName.SubString(0, 16);
        }
        else
        {
            sBaseName = "0";
        }

        sFileName = sBaseName;
#if KOTOR1  // K1 lower-cases the candidate file name (and the suffixed retries below)
        sFileName = sFileName.LowerCase();

        g_pExoBase->GetDirectoryList(&vaultFiles, sVaultDir, RESTYPE_BIC, 0, 0, 0);
#else

        g_pExoBase->GetDirectoryList(&vaultFiles, sVaultDir, RESTYPE_BIC, 0, 0);
#endif

        nSuffix = 1;

        for (;;)
        {
            if (vaultFiles.Contains(sFileName + ".bic") == 0)
                break;

            sSuffix = CExoString(nSuffix);
            sBaseName = sBaseName.SubString(
                0, 16 - sSuffix.GetLength());
            sFileName = sBaseName + sSuffix;
#if KOTOR1
            sFileName = sFileName.LowerCase();
#endif

            nSuffix++;
        }

#if KOTOR1  // K1 (Android port): "/" separators
        sPath.Format(const_cast<char*>("%s%s%s"), sVaultDir.CStr(), "/", sFileName.CStr());
#else
        sPath.Format(const_cast<char*>("%s%s%s"), sVaultDir.CStr(), "\\", sFileName.CStr());
#endif

        if (pGff->WriteGFFFile(sPath, RESTYPE_BIC) == 0)
        {
            delete pGff;

            return 0;
        }

        SetFileName(CResRef(sFileName));
    }

    g_pExoResMan->UpdateResourceDirectory(CExoString("SERVERVAULT:"));

    delete pGff;

    return 1;

failInTransit:
    delete pGff;
    delete pStruct;

    pCreature->m_bPlot = 1;

    return 0;

fail:
    delete pGff;
    delete pStruct;

    return 0;
}
