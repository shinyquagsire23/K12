
#include "k2stdint.h"
#include "nwsdialog.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "clientexoapp.h"
#include "partytables.h"
#include "gameobjectarray.h"
#include "nwsplayer.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "nwssendplayermessage.h"
#include "guiingame.h"
#include "nwscreature.h"
#include "nwsjournal.h"
#include "nwsparty.h"
#include "worldtimer.h"
#include "nwsplaceable.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "exolocstring.h"
#include "exostring.h"
#include "nwscript.h"
#include "resgff.h"
#include "exoresman.h"
#include "nwsrules.h"
#include "twodimarray.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "exosound.h"
#include "serverexoappinternal.h"
#include "nwcobject.h"
#include "strref.h"

// The number of override directories the stream-voice lookup tries.
enum {
    MAX_OVERRIDE_PATHS = 32,
};
#include "swparty.h"
#include "nwcobject.h"
#include "AURORA/aurmodel.h"
#include "exobase.h"
#include "clientoptions.h"

extern float checkaabbtime;
extern int g_bRenderClipWalkable;
extern int g_nCombatAttackRoll;
extern int g_bDisableDamage;
float g_AIModuleTime;
extern int g_bLogEventAdd;
extern int RenderPlaceableBBox;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/crtapi.h"
#if KOTOR1
#include "swguimaininterfacepc2.h"
void EvaluateAchievement(uint64_t nStrRef);
void QueueHelpGui(int nHelp);
#endif
extern int g_bLogConversations;
extern int g_bDisableSound;
#if !KOTOR1
bool s_oondarDone;
#endif

#include "port/crtapi.h"
#include "exostringinline.h"
#if !KOTOR1
static void __attribute__((noinline)) FormatPath(char* pBuf, int nLen, const char* pszFmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    __vsprintf_chk(pBuf, 0, nLen, pszFmt, ap);
    __builtin_va_end(ap);
}
#endif
extern unsigned char g_bVODebugEnabled;
#if !KOTOR1
unsigned char g_bVODebugEnabled;
#endif
extern char sOut[255];
#if !KOTOR1
char sOut[255] = "                                                  ";
#endif

CSWSDialog::CSWSDialog()
{
    m_paEntries = 0;
    m_nEntryCount = 0;
    m_paReplies = 0;
    m_nReplyCount = 0;
    m_paStartList = 0;
    m_nStartCount = 0;

    m_bSkippable = 0;
    m_bUnequipItems = 0;

    m_bEnded = 0;
#if !KOTOR1  // K1 has no alien VO table and no music ducking
    m_bNodeUnskippable = 0;
#endif

    m_bRepliesPending = 0;
    m_bBark = 0;
    m_nCurrentEntry = 0;
    m_nHandledReply = NODE_INDEX_NONE;

    m_nPlayerGender = CTlkTable::GENDER_MALE;

    m_oidCurrentPlayer = OBJECT_INVALID;
    m_ownerId = OBJECT_INVALID;

    m_paSpeakers = 0;
    m_nSpeakerCount = 0;
    m_bFirstPlayerOnly = 0;
    m_bPrivate = 0;
    m_bUnequipHItem = 0;

    m_oidLastEntrySpeaker = OBJECT_INVALID;
    m_oidLastReplySpeaker = OBJECT_INVALID;

    m_nCameraAngle = 0;
    m_nCameraAngleDelta = 1;
    m_bCameraAngleForward = 0;
    m_anCameraAngles[0] = 1;
    m_anCameraAngles[1] = 3;
    m_anCameraAngles[2] = 1;
    m_anCameraAngles[3] = 3;
    m_anCameraAngles[4] = 2;
    m_anCameraAngles[5] = 2;
    m_anCameraAngles[6] = 1;
    m_anCameraAngles[7] = 3;
    m_anCameraAngles[8] = 1;
    m_anCameraAngles[9] = 2;
    m_anCameraAngles[10] = 1;
    m_anCameraAngles[11] = 2;
    m_anCameraAngles[12] = 1;
    m_anCameraAngles[13] = 3;
    m_anCameraAngles[14] = 2;
    m_anCameraAngles[15] = 3;
    m_anCameraAngles[16] = 1;
    m_anCameraAngles[17] = 1;
    m_anCameraAngles[18] = 1;
#if !KOTOR1
    m_nAlienVOVariation = 0;
    m_nAlienVOVariationDelta = 1;
    m_bAlienVOForward = 0;
    m_anAlienVOVariations[0] = 1;
    m_anAlienVOVariations[1] = 1;
    m_anAlienVOVariations[2] = 2;
    m_anAlienVOVariations[3] = 3;
    m_anAlienVOVariations[4] = 1;
    m_anAlienVOVariations[5] = 2;
    m_anAlienVOVariations[6] = 1;
    m_anAlienVOVariations[7] = 2;
    m_anAlienVOVariations[8] = 2;
    m_anAlienVOVariations[9] = 3;
    m_anAlienVOVariations[10] = 3;
    m_anAlienVOVariations[11] = 3;
    m_anAlienVOVariations[12] = 1;
    m_anAlienVOVariations[13] = 3;
    m_anAlienVOVariations[14] = 2;
    m_anAlienVOVariations[15] = 1;
    m_anAlienVOVariations[16] = 3;
    m_anAlienVOVariations[17] = 2;
    m_anAlienVOVariations[18] = 2;
#endif
    m_bFirstEntryLine = 0;
    m_bFirstReplyLine = 0;

    m_rAmbientTrack = const_cast<char*>("");

    m_bRanEndScript = 0;
    m_bNeverBark = 0;
#if !KOTOR1
    m_bComputerConversation = 0;
    m_nAlienRaceOwner = 0;
    m_pAnimObject = 0;
#endif

    m_ownerSavedAiLevel = 2;
    m_nParticipantWaitMs = 0;
    m_nParticipantWaitDay = 0;
    m_nParticipantWaitTod = 0;

#if !KOTOR1
    g_pExoSound->StartDuckingMusicAndAmbientSounds();
#endif
}

CSWSDialog::~CSWSDialog()
{
#if !KOTOR1  // K1 has no music ducking and no animation object
    g_pExoSound->StopDuckingMusicAndAmbientSounds();
    if (m_pAnimObject != 0)
    {
        if (m_pAnimObject->GetAnimationBase()->GetModel(CSWCObject::ANIM_MODEL_BODY) != 0)
            m_pAnimObject->GetAnimationBase()->GetModel(CSWCObject::ANIM_MODEL_BODY)
                ->StopOutOfOrderAnimation(const_cast<char*>("emotions"));
        if (m_pAnimObject->GetAnimationBase()->GetModel(CSWCObject::ANIM_MODEL_HEAD) != 0)
            m_pAnimObject->GetAnimationBase()->GetModel(CSWCObject::ANIM_MODEL_HEAD)
                ->StopOutOfOrderAnimation(const_cast<char*>("emotions"));
        m_pAnimObject = 0;
    }
#endif
    ReEquipItemsForDialog();
    ReEquipHItemForDialog();
    Cleanup();
}

void CSWSDialog::SetDialogWaitFlags(CSWSObject* pObj, unsigned int flags)
{
    pObj->m_dialogWaitFlags = flags;
}

void CSWSDialog::SetDialogOwner(CSWSObject* pObj)
{
    // Save the owner's current AI level, raise it to at least 3 so a talking creature stays in a
    // servicing AI bucket, and record the owner's object id.
    int aiLevel = pObj->m_aiLevel;
    m_ownerSavedAiLevel = aiLevel;
    if (aiLevel <= 2)
        pObj->m_aiLevel = 3;
    m_ownerId = pObj->m_objectId;
}

#if !KOTOR1
const char* vodebug(char* pszArgs)
{
    int nOn = 0;
    if (sscanf(pszArgs, "%d", &nOn) == 1)
    {
        const char* pszMsg = nOn == 0 ? "VO Debug logging terminated."
                                     : "VO Debug logging started.";
        g_bVODebugEnabled = (nOn != 0);
        // The message is its own format string; the buffer is 255 bytes.
        FormatPath(sOut, 255, pszMsg);
        return sOut;
    }
    return "usage: VODebug <0:Off, 1:On>";
}
#endif

void CSWSDialog::InitializePsuedoRandomCameraAngles(CResGFF* pGff)
{
    CResRef resRef;
    CExoString sResRef;
    unsigned short nResType;
    pGff->GetResRef(resRef, nResType);
    resRef.CopyToString(sResRef);

    unsigned int nHash = 0;
    for (int i = 0; i < sResRef.GetLength(); i++)
    {
        int nChar = static_cast<signed char>(sResRef[i]);
#if KOTOR1  // the exponent is converted straight to double
        int nPow = static_cast<int>(__builtin_pow(10.0, static_cast<double>(i)));
#else
        int nPow = static_cast<int>(__builtin_pow(10.0, static_cast<float>(i)));
#endif
        nHash += nChar + nChar * nPow;
    }

    m_nCameraAngle = nHash % 19;
    m_nCameraAngleDelta = nHash % 5 + 1;
    m_bCameraAngleForward = nHash & 1;
}

#if !KOTOR1
void CSWSDialog::InitializePseudoRandomAlienVOVariation(CResGFF* pGff)
{
    CResRef resRef;
    CExoString sResRef;
    unsigned short nResType;
    pGff->GetResRef(resRef, nResType);
    resRef.CopyToString(sResRef);

    unsigned int nHash = 0;
    for (int i = 0; i < sResRef.GetLength(); i++)
    {
        int nChar = static_cast<signed char>(sResRef[i]);
        int nPow = static_cast<int>(__builtin_pow(10.0, static_cast<float>(i)));
        nHash += nChar + nChar * nPow;
    }

    m_nAlienVOVariationDelta = nHash % 5 + 1;
    m_nAlienVOVariation = nHash % 19;
    m_bAlienVOForward = nHash & 1;
}
#endif

int CSWSDialog::AssignPseudoRandomCameraAngle()
{
    int nAngle = m_anCameraAngles[m_nCameraAngle];
    m_nCameraAngle += m_bCameraAngleForward ? m_nCameraAngleDelta : -m_nCameraAngleDelta;
    if (m_nCameraAngle > 18)
        m_nCameraAngle = 0;
    if (m_nCameraAngle < 0)
        m_nCameraAngle = 18;
    return nAngle;
}

#if !KOTOR1
int CSWSDialog::AssignPseudoRandomAlienVOVariation()
{
    int nVariation = m_anAlienVOVariations[m_nAlienVOVariation];
    m_nAlienVOVariation += m_bAlienVOForward ? m_nAlienVOVariationDelta : -m_nAlienVOVariationDelta;
    if (m_nAlienVOVariation > 18)
        m_nAlienVOVariation = 0;
    if (m_nAlienVOVariation < 0)
        m_nAlienVOVariation = 18;
    return nVariation;
}
#endif

CExoString CSWSDialog::GetExoStringFromLocString(CExoLocString& sLoc, unsigned char nGender)
{
#if KOTOR1  // the server's language for the player creature
    unsigned long long nPlayerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CExoString sResult;
    sLoc.GetString(g_pAppManager->m_pServerExoApp->GetPlayerLanguage(static_cast<unsigned int>(nPlayerId)),
                   &sResult, nGender);
    return sResult;
#else
    g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CExoString sResult;
    sLoc.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sResult, nGender);
    return sResult;
#endif
}

void CSWSDialog::GiveDialogPlotXP(int nXpKind, float fAmount)
{
    if (nXpKind != -1 && fAmount > 0.0f
        && g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GivePlotXP(
            nXpKind, static_cast<int>(__builtin_ceilf(fAmount * 100.0f)));
}

void CSWSDialog::AddJournalEntry(const CExoString& sEntry, unsigned int nState,
                                 unsigned int nPlayerId)
{
    if (sEntry.IsEmpty())
        return;

    CSWSPlayer* pPlayer =
        static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(nPlayerId, 0));
    if (pPlayer == 0)
        return;
    if (pPlayer->AsSWSPlayer() == 0)
        return;
    CSWSObject* pPlayerObject = pPlayer->AsSWSPlayer()->GetGameObject();
    if (pPlayerObject == 0)
        return;
    if (pPlayerObject->AsSWSCreature() == 0)
        return;
    CSWSCreature* pCreature = pPlayerObject->AsSWSCreature();

    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);

    if (pCreature != 0 && pCreature->GetJournal()->GetState(sEntry) < static_cast<int>(nState))
    {
        pCreature->GetJournal()->SetDate(sEntry, nDay, 0);
        pCreature->GetJournal()->SetTime(sEntry, nTod, 0);
        pCreature->GetJournal()->SetState(sEntry, nState, 0);
        int nXp = pCreature->GetJournal()->GetQuestEntryXP(sEntry);
        if (nXp != 0)
        {
            if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
                g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nXp, 1);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, nXp);
        }
    }

    CSWSFaction* pFaction = pCreature->GetFaction();
    if (pFaction == 0)
        return;
    unsigned long long* paMembers = pFaction->m_members.m_pArray;
    if (paMembers == 0)
        return;
    int nMemberCount = pFaction->m_members.m_count;

    for (int i = 0; i < nMemberCount; i++)
    {
        CSWSCreature* pMember =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(paMembers[i]);
        if (pMember == 0)
            continue;
        if (pMember == pCreature)
            continue;
        if (pMember->GetJournal()->GetState(sEntry) >= static_cast<int>(nState))
            continue;

        pMember->GetJournal()->SetDate(sEntry, nDay, 0);
        pMember->GetJournal()->SetTime(sEntry, nTod, 0);
        pMember->GetJournal()->SetState(sEntry, nState, 0);
        int nXp = pMember->GetJournal()->GetQuestEntryXP(sEntry);
        if (nXp != 0)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, nXp);
    }
}

#if KOTOR1  // one condition script per link, one action script per node, no computer-conversation flag
unsigned int CSWSDialog::GetStartEntry(CSWSObject* pObj)
{
    int nResult = -1;
    for (unsigned long long i = 0; i < m_nStartCount; i++)
    {
        if (m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker != const_cast<char*>(""))
        {
            if (!pObj->m_sTag.CompareNoCase(m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker))
            {
                if (GetSpeaker(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker) == 0)
                    continue;
            }
        }

        if (CheckScript(pObj, m_paStartList[i].m_rActive) == 0)
            continue;

        if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount != 0 || m_bNeverBark != 0)
        {
            if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount == 1
                && m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                       .m_nEntryCount == 0
                && m_bNeverBark == 0)
            {
                BarkOneLine(m_paStartList[i].m_nIndex);
                RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript);
                RunScript(pObj,
                          m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                              .m_rScript);

                while (m_lstParticipants.m_pInternal->m_pHead != 0)
                {
                    CSWSDialogPlayer* pPlayer =
                        static_cast<CSWSDialogPlayer*>(m_lstParticipants.RemoveHead());
                    if (pPlayer != 0)
                        delete pPlayer;
                }
                return nResult;
            }

            return m_paStartList[i].m_nIndex;
        }

        BarkOneLine(m_paStartList[i].m_nIndex);
        RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript);

        while (m_lstParticipants.m_pInternal->m_pHead != 0)
        {
            CSWSDialogPlayer* pPlayer =
                static_cast<CSWSDialogPlayer*>(m_lstParticipants.RemoveHead());
            if (pPlayer != 0)
                delete pPlayer;
        }
        return nResult;
    }
    return nResult;
}
#else
unsigned int CSWSDialog::GetStartEntry(CSWSObject* pObj)
{
    int nResult = -1;
    if (m_nStartCount == 0)
        return nResult;

    for (unsigned long long i = 0; i < m_nStartCount; i++)
    {
        if (m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker != const_cast<char*>(""))
        {
            if (!pObj->m_sTag.CompareNoCase(m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker))
            {
                if (GetSpeaker(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker) == 0)
                    continue;
            }
        }

        int nResult1;
        int bScript1;
        if (m_paStartList[i].m_rActive == const_cast<char*>(""))
        {
            bScript1 = 0;
            nResult1 = 1;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(
                m_paStartList[i].m_nParam1, m_paStartList[i].m_nParam2,
                m_paStartList[i].m_nParam3, m_paStartList[i].m_nParam4,
                m_paStartList[i].m_nParam5, m_paStartList[i].m_sParamStrA);
            nResult1 = CheckScript(pObj, m_paStartList[i].m_rActive);
            if (m_paStartList[i].m_bNot != 0)
                nResult1 = (nResult1 == 0);
            bScript1 = 1;
        }

        if (m_paStartList[i].m_rActive2 == const_cast<char*>(""))
        {
            if (bScript1 != 0 && nResult1 == 0)
                continue;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(
                m_paStartList[i].m_nParam1b, m_paStartList[i].m_nParam2b,
                m_paStartList[i].m_nParam3b, m_paStartList[i].m_nParam4b,
                m_paStartList[i].m_nParam5b, m_paStartList[i].m_sParamStrB);
            int nResult2 = CheckScript(pObj, m_paStartList[i].m_rActive2);
            if (m_paStartList[i].m_bNot2 != 0)
                nResult2 = (nResult2 == 0);

            int bResult;
            if (bScript1 == 0)
                bResult = nResult2;
            else if (m_paStartList[i].m_nLogic == 0)
                bResult = (nResult1 != 0) && (nResult2 != 0);
            else
                bResult = (nResult1 != 0) || (nResult2 != 0);
            if (bResult == 0)
                continue;
        }

        if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount != 0
            || m_bNeverBark != 0 || m_bComputerConversation != 0)
        {
            if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount == 1
                && m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                       .m_nEntryCount == 0
                && m_bNeverBark == 0 && m_bComputerConversation == 0)
            {
                BarkOneLine(m_paStartList[i].m_nIndex);
                g_pVirtualMachine->SetScriptParams(
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam1,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam2,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam3,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam4,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam5,
                    m_paEntries[m_paStartList[i].m_nIndex].m_sActionParamStrA);
                RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript);
                g_pVirtualMachine->SetScriptParams(
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam1b,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam2b,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam3b,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam4b,
                    m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam5b,
                    m_paEntries[m_paStartList[i].m_nIndex].m_sActionParamStrB);
                RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript2);
                g_pVirtualMachine->SetScriptParams(
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam1,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam2,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam3,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam4,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam5,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_sActionParamStrA);
                RunScript(pObj,
                          m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                              .m_rScript);
                g_pVirtualMachine->SetScriptParams(
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam1b,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam2b,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam3b,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam4b,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_nActionParam5b,
                    m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                        .m_sActionParamStrB);
                RunScript(pObj,
                          m_paReplies[m_paEntries[m_paStartList[i].m_nIndex].m_paReplies[0].m_nIndex]
                              .m_rScript2);

                while (m_lstParticipants.m_pInternal->m_pHead != 0)
                {
                    CSWSDialogPlayer* pPlayer =
                        static_cast<CSWSDialogPlayer*>(m_lstParticipants.RemoveHead());
                    if (pPlayer != 0)
                        delete pPlayer;
                }
                return nResult;
                }

            return m_paStartList[i].m_nIndex;
        }

        BarkOneLine(m_paStartList[i].m_nIndex);
        g_pVirtualMachine->SetScriptParams(
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam1,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam2,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam3,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam4,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam5,
            m_paEntries[m_paStartList[i].m_nIndex].m_sActionParamStrA);
        RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript);
        g_pVirtualMachine->SetScriptParams(
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam1b,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam2b,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam3b,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam4b,
            m_paEntries[m_paStartList[i].m_nIndex].m_nActionParam5b,
            m_paEntries[m_paStartList[i].m_nIndex].m_sActionParamStrB);
        RunScript(pObj, m_paEntries[m_paStartList[i].m_nIndex].m_rScript2);

        while (m_lstParticipants.m_pInternal->m_pHead != 0)
        {
            CSWSDialogPlayer* pPlayer =
                static_cast<CSWSDialogPlayer*>(m_lstParticipants.RemoveHead());
            if (pPlayer != 0)
                delete pPlayer;
        }
        return nResult;
    }
    return nResult;
}
#endif

#if KOTOR1  // the link's one condition script decides alone
int CSWSDialog::GetStartEntryOneLiner(CSWSObject* pObj, CExoLocString& sText, CResRef& rSound,
                                      CResRef& rScript)
{
    for (unsigned long long i = 0; i < m_nStartCount; i++)
    {
        if (m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker != const_cast<char*>(""))
            continue;
        if (CheckScript(pObj, m_paStartList[i].m_rActive) == 0)
            continue;
        if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount != 0)
            continue;

        sText = m_paEntries[m_paStartList[i].m_nIndex].m_sText;
        rSound = m_paEntries[m_paStartList[i].m_nIndex].m_rVO_ResRef;
        rScript = m_paEntries[m_paStartList[i].m_nIndex].m_rScript;
        return 1;
    }
    return 0;
}
#else
int CSWSDialog::GetStartEntryOneLiner(CSWSObject* pObj, CExoLocString& sText, CResRef& rSound,
                                      CResRef& rScript)
{
    for (unsigned long long i = 0; i < m_nStartCount; i++)
    {
        if (m_paEntries[m_paStartList[i].m_nIndex].m_sSpeaker != const_cast<char*>(""))
            continue;

        int nResult1;
        int bScript1;
        if (m_paStartList[i].m_rActive == const_cast<char*>(""))
        {
            bScript1 = 0;
            nResult1 = 1;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(
                m_paStartList[i].m_nParam1, m_paStartList[i].m_nParam2,
                m_paStartList[i].m_nParam3, m_paStartList[i].m_nParam4,
                m_paStartList[i].m_nParam5, m_paStartList[i].m_sParamStrA);
            nResult1 = CheckScript(pObj, m_paStartList[i].m_rActive);
            if (m_paStartList[i].m_bNot != 0)
                nResult1 = (nResult1 == 0);
            bScript1 = 1;
        }

        if (m_paStartList[i].m_rActive2 == const_cast<char*>(""))
        {
            if (bScript1 != 0 && nResult1 == 0)
                continue;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(
                m_paStartList[i].m_nParam1b, m_paStartList[i].m_nParam2b,
                m_paStartList[i].m_nParam3b, m_paStartList[i].m_nParam4b,
                m_paStartList[i].m_nParam5b, m_paStartList[i].m_sParamStrB);
            int nResult2 = CheckScript(pObj, m_paStartList[i].m_rActive2);
            if (m_paStartList[i].m_bNot2 != 0)
                nResult2 = (nResult2 == 0);

            int bResult;
            if (bScript1 == 0)
                bResult = nResult2;
            else if (m_paStartList[i].m_nLogic == 0)
                bResult = (nResult1 != 0) && (nResult2 != 0);
            else
                bResult = (nResult1 != 0) || (nResult2 != 0);
            if (bResult == 0)
                continue;
        }

        if (m_paEntries[m_paStartList[i].m_nIndex].m_nReplyCount != 0)
            continue;

        sText = m_paEntries[m_paStartList[i].m_nIndex].m_sText;
        rSound = m_paEntries[m_paStartList[i].m_nIndex].m_rVO_ResRef;
        rScript = m_paEntries[m_paStartList[i].m_nIndex].m_rScript;
        return 1;
    }
    return 0;
}
#endif

void CSWSDialog::LoadDialogLinkedNode(CSWSDialogLinkEntry& link, CResGFF* pGff, CResStruct& sStruct,
                                      int& nReadOk, unsigned int nMaxIndex)
{
    link.m_rActive = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("Active"), nReadOk, CResRef(""));
#if !KOTOR1  // K1 stores a link as "Active" plus "Index"
    link.m_nParam1 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param1"), nReadOk, 0);
    link.m_nParam2 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param2"), nReadOk, 0);
    link.m_nParam3 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param3"), nReadOk, 0);
    link.m_nParam4 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param4"), nReadOk, 0);
    link.m_nParam5 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param5"), nReadOk, 0);
    link.m_rActive2 = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("Active2"), nReadOk, CResRef(""));
    link.m_nParam1b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param1b"), nReadOk, 0);
    link.m_nParam2b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param2b"), nReadOk, 0);
    link.m_nParam3b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param3b"), nReadOk, 0);
    link.m_nParam4b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param4b"), nReadOk, 0);
    link.m_nParam5b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Param5b"), nReadOk, 0);
    link.m_bNot = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("Not"), nReadOk, 0);
    link.m_bNot2 = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("Not2"), nReadOk, 0);
    link.m_nLogic = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Logic"), nReadOk, 0);
    link.m_sParamStrA
        = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("ParamStrA"), nReadOk, CExoString(""));
    link.m_sParamStrB
        = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("ParamStrB"), nReadOk, CExoString(""));
#endif
    link.m_nIndex = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("Index"), nReadOk, 0);
    if (link.m_nIndex > nMaxIndex)
        nReadOk = 0;
}

void CSWSDialog::ClearDialogOwnerInObject(unsigned long long objectId)
{
    CGameObject* pGameObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pGameObject)
        == CGameObjectArray::SUCCESS)
    {
        CSWSObject* pObject = static_cast<CSWSObject*>(pGameObject);
        if (pObject->m_dialogOwnerId == m_ownerId)
        {
            pObject->SetDialogOwner(OBJECT_INVALID);
            if (pObject->AsSWSCreature() != 0)
                pObject->AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 0, 0);
        }
    }
}

void CSWSDialog::ClearAllPlayerHostileActions()
{
    unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerId);
    unsigned long long gameObjectId = pPlayer->m_gameObjectId;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();

    for (long long i = 0; i < pArea->m_aObjectsByX.m_count; i++)
    {
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(pArea->m_aObjectsByX.m_pArray[i], &pObject)
            != CGameObjectArray::SUCCESS)
            continue;
        if (pObject == 0)
            continue;
        if (pObject->AsSWSCreature() == 0)
            continue;
        if (pObject->AsSWSCreature()->m_bInParty != 0)
            continue;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->GetReputation(gameObjectId, 0) > 10)
            continue;
        pCreature->ClearAllActions(1);
        if (pCreature->GetClientCreature() != 0)
            pCreature->GetClientCreature()->ClearAllActions();
    }
}

int CSWSDialog::CheckScript(CSWSObject* pObj, const CResRef& rScript)
{
    if (rScript == const_cast<char*>(""))
        return 1;

    CExoString sScript;
    rScript.CopyToString(sScript);

    int nResult = g_pVirtualMachine->RunScript(&sScript, pObj->m_objectId, 1);
    if (nResult == 1)
    {
        int nType;
        void* pValue;
        nResult = 0;
        if (g_pVirtualMachine->GetRunScriptReturnValue(&nType, &pValue) && nType == 3)
            nResult = pValue != 0;
    }
    return nResult;
}

void CSWSDialog::ReEquipItemsForDialog()
{
    CExoLinkedListNode* pNode = m_lstUnequippedItems.m_pInternal->m_pHead;
    while (pNode != 0)
    {
        unsigned long long* pId =
            static_cast<unsigned long long*>(m_lstUnequippedItems.GetAtPos(pNode));
        if (pId != 0)
        {
            unsigned long long nId = *pId;
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(nId);
            if (pObj != 0 && pObj->AsSWCCreature() != 0)
                pObj->AsSWCCreature()->ShowEquippedItems();
            delete pId;
        }
        m_lstUnequippedItems.GetNext(pNode);
    }
}

void CSWSDialog::ReEquipHItemForDialog()
{
    CExoLinkedListNode* pNode = m_lstUnequippedHItems.m_pInternal->m_pHead;
    while (pNode != 0)
    {
        unsigned long long* pId =
            static_cast<unsigned long long*>(m_lstUnequippedHItems.GetAtPos(pNode));
        if (pId != 0)
        {
            unsigned long long nId = *pId;
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(nId);
            if (pObj != 0 && pObj->AsSWCCreature() != 0)
                pObj->AsSWCCreature()->ShowEquippedHeadItem();
            delete pId;
        }
        m_lstUnequippedHItems.GetNext(pNode);
    }
}

float CSWSDialog::SetDialogDelay(CSWSObject* pObj, CExoLocString& sLoc, unsigned int a, int b,
                                 unsigned int c)
{
    CExoString sText;
    float fDelay = 0.0f;
    if (b != 0 && m_lstParticipants.m_pInternal->m_pHead == 0)
    {
        CSWSMessage::GetLocStringServerTime(PLAYER_ID_ALL, sLoc, sText, fDelay, m_nPlayerGender);
#if KOTOR1  // CExoString::GetLength is out of line
        if (fDelay < 0.15f)
            fDelay = static_cast<float>(static_cast<int>(sText.GetLength())) * 0.15f * 0.75f;
#else
        if (fDelay < 0.15f)
            fDelay = static_cast<float>(sText.GetLength()) * 0.15f * 0.75f;
#endif
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut == 0 && c == 0)
    {
        if (fDelay < 0.01f)
            fDelay = 0.01f;
    }

    if (static_cast<unsigned int>(fDelay) < a)
        fDelay = static_cast<float>(a);

    pObj->SetDialogDelay(fDelay);
    return fDelay;
}

void CSWSDialog::RunScript(CSWSObject* pObj, const CResRef& rScript)
{
    if (rScript == const_cast<char*>(""))
        return;

    CExoString sScript;
    rScript.CopyToString(sScript);
    g_pVirtualMachine->RunScript(&sScript, pObj->m_objectId, 1);
}

void CSWSDialog::RunEndConversationScript(CSWSObject* pObj, int bBark)
{
    if (m_bBark == 0 || m_bRanEndScript != 0)
    {
        RunScript(pObj, m_rScriptEndConverAbort);
        return;
    }

    m_bRanEndScript = 1;
    RunScript(pObj, m_rScriptEndConversation);

    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pGameObject;
    pObjectArray->GetGameObject(m_ownerId, &pGameObject);
    if (pGameObject == 0)
        return;

    // The dialog owner is the object that speaks; it is also what OnEndDialog is handed as its
    // speaker argument below.
    CSWSObject* pSwsObject = pGameObject->AsSWSObject();
    if (pSwsObject == 0)
        return;

    CSWSArea* pArea = static_cast<CSWSArea*>(pSwsObject->GetArea());
    if (bBark != 0)
    {
        if (pSwsObject->AsSWSCreature() != 0)
            pSwsObject->AsSWSCreature()->OnEndDialog(pSwsObject);
        else if (pSwsObject->AsSWSPlaceable() != 0)
            pSwsObject->AsSWSPlaceable()->OnEndDialog(pSwsObject);
    }
    else
    {
        for (long long i = 0; i < pArea->m_aObjectsByX.m_count; )
        {
            pObjectArray->GetGameObject(pArea->m_aObjectsByX.m_pArray[i++], &pGameObject);
            if (pGameObject == 0)
                continue;
            if (pGameObject->AsSWSCreature() != 0)
                pGameObject->AsSWSCreature()->OnEndDialog(pSwsObject);
            else if (pGameObject->AsSWSPlaceable() != 0)
                pGameObject->AsSWSPlaceable()->OnEndDialog(pSwsObject);
        }
    }
}

int CSWSDialog::LoadDialog(CResGFF* pGff, int a)
{
    CResList list;
    CResList subList;
    CResStruct sStruct;
    CResStruct sEntryStruct;
    CResStruct sLinkStruct;
    int nReadOk = 1;
    int bSuccess = 1;

    InitializePsuedoRandomCameraAngles(pGff);
    m_bFirstEntryLine = 1;
    m_bFirstReplyLine = 1;
#if !KOTOR1  // K1 has no alien VO variation, no "AlienRaceOwner" and no computer-conversation flag
    InitializePseudoRandomAlienVOVariation(pGff);
#endif
    Cleanup();

    pGff->GetTopLevelStruct(&sStruct);

    m_rCameraModel = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("CameraModel"), bSuccess, CResRef(""));

    unsigned int nDefaultDelay = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("DelayEntry"), nReadOk, 0);
    pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("DelayReply"), nReadOk, 0);

    m_rScriptEndConversation
        = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("EndConversation"), bSuccess, CResRef(""));
    m_rScriptEndConverAbort
        = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("EndConverAbort"), bSuccess, CResRef(""));
    m_bSkippable = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("Skippable"), bSuccess, 1);
    int nConversationType = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ConversationType"), bSuccess, 0);
    unsigned char nComputerType
        = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("ComputerType"), bSuccess, 0);
    m_rAmbientTrack = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("AmbientTrack"), bSuccess, CResRef(""));
    m_bUnequipItems = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("UnequipItems"), bSuccess, 0);
    m_bUnequipHItem = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("UnequipHItem"), bSuccess, 0);
    unsigned char nAnimatedCut = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("AnimatedCut"), bSuccess, 0);
    unsigned char nOldHitCheck = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("OldHitCheck"), bSuccess, 0);
#if !KOTOR1
    m_nAlienRaceOwner = pGff->ReadFieldINT(&sStruct, const_cast<char*>("AlienRaceOwner"), bSuccess, 0);
#endif

    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut = nAnimatedCut;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bOldHitCheck = nOldHitCheck;

#if KOTOR1
    if (nConversationType == 2)
        m_bNeverBark = 1;
    if (nConversationType == 1)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetConversationType(1, nComputerType);
    }
    else
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetConversationType(0, 0);
        g_pAppManager->m_pServerExoApp->GetPartyTable()->UnstealthParty(1);  // K1: no SetKeepStealthInDialog
    }
#else

    // Only the computer conversation skips the stealth release below; it and the normal arm rejoin at
    // the unpauseable-state test.
    if (nConversationType == 1)
    {
        m_bComputerConversation = 1;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetConversationType(1, nComputerType);
    }
    else
    {
        if (nConversationType == 2)
            m_bNeverBark = 1;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetConversationType(0, 0);

        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bKeepStealthInDialog == 0)
            g_pAppManager->m_pServerExoApp->GetPartyTable()->UnstealthParty(1);
    }
#endif

    if (nAnimatedCut != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(1);

    pGff->GetList(&list, &sStruct, const_cast<char*>("ReplyList"));
    m_nReplyCount = pGff->GetListCount(&list);
    pGff->GetList(&list, &sStruct, const_cast<char*>("EntryList"));
    m_nEntryCount = pGff->GetListCount(&list);
    m_paEntries = new CSWSDialogEntry[m_nEntryCount];

    if (nReadOk == 0)
        goto fail;

    for (unsigned long long i = 0; i < m_nEntryCount; i++)
    {
        pGff->GetListElement(&sEntryStruct, &list, static_cast<unsigned int>(i));
        LoadDialogBase(m_paEntries[i], pGff, sEntryStruct, nDefaultDelay, nReadOk);
        LoadDialogCamera(m_paEntries[i].m_camera, pGff, sEntryStruct, nReadOk);

        pGff->GetList(&subList, &sEntryStruct, const_cast<char*>("RepliesList"));
        m_paEntries[i].m_nReplyCount = pGff->GetListCount(&subList);

        if (a == 0)
        {
            if (nReadOk == 0)
                goto fail;
        }
        else
        {
            if (m_paEntries[i].m_nReplyCount != 0)
                m_paEntries[i].m_paReplies
                    = new CSWSDialogReplyLink[m_paEntries[i].m_nReplyCount];

            if (nReadOk == 0)
                goto fail;

            for (unsigned long long j = 0; j < m_paEntries[i].m_nReplyCount; j++)
            {
                pGff->GetListElement(&sLinkStruct, &subList, static_cast<unsigned int>(j));
                LoadDialogLinkedNode(m_paEntries[i].m_paReplies[j], pGff, sLinkStruct, nReadOk,
                                     m_nReplyCount);
                m_paEntries[i].m_paReplies[j].m_nIsChild
                    = pGff->ReadFieldBYTE(&sLinkStruct, const_cast<char*>("DisplayInactive"), bSuccess, 0) != 0;

                if (nReadOk == 0)
                    goto fail;
            }
        }
    }

    if (a == 0)
    {
        m_nReplyCount = 0;
    }
    else
    {
        pGff->GetList(&list, &sStruct, const_cast<char*>("ReplyList"));
        m_paReplies = new CSWSDialogReply[m_nReplyCount];

        for (unsigned long long j = 0; j < m_nReplyCount; j++)
        {
            pGff->GetListElement(&sEntryStruct, &list, static_cast<unsigned int>(j));
            LoadDialogBase(m_paReplies[j], pGff, sEntryStruct, nDefaultDelay, nReadOk);
            LoadDialogCamera(m_paReplies[j].m_camera, pGff, sEntryStruct, nReadOk);

            pGff->GetList(&subList, &sEntryStruct, const_cast<char*>("EntriesList"));
            m_paReplies[j].m_nEntryCount = pGff->GetListCount(&subList);
            if (m_paReplies[j].m_nEntryCount != 0)
                m_paReplies[j].m_paEntries
                    = new CSWSDialogLinkEntry[m_paReplies[j].m_nEntryCount];

            if (nReadOk == 0)
                goto fail;

            for (unsigned long long k = 0; k < m_paReplies[j].m_nEntryCount; k++)
            {
                pGff->GetListElement(&sLinkStruct, &subList, static_cast<unsigned int>(k));
                LoadDialogLinkedNode(m_paReplies[j].m_paEntries[k], pGff, sLinkStruct, nReadOk,
                                     m_nEntryCount);

                if (nReadOk == 0)
                    goto fail;
            }
        }
    }

    pGff->GetList(&subList, &sStruct, const_cast<char*>("StartingList"));
    m_nStartCount = pGff->GetListCount(&subList);
    if (m_nStartCount != 0)
        m_paStartList = new CSWSDialogLinkEntry[m_nStartCount];
    else
        m_paStartList = 0;

    for (unsigned long long k = 0; k < m_nStartCount && nReadOk != 0; k++)
    {
        pGff->GetListElement(&sLinkStruct, &subList, static_cast<unsigned int>(k));
        LoadDialogLinkedNode(m_paStartList[k], pGff, sLinkStruct, nReadOk, m_nEntryCount);
    }

    if (nReadOk == 0)
        Cleanup();

    CResList stuntList;
    pGff->GetList(&stuntList, &sStruct, const_cast<char*>("StuntList"));
    m_nModelCount = pGff->GetListCount(&stuntList);
    if (m_nModelCount != 0)
    {
        m_paStuntModels = new CSWSDialogStuntModel[m_nModelCount];

        if (nReadOk != 0)
        {
            for (unsigned long long k = 0; k < m_nModelCount && nReadOk != 0; k++)
            {
                CResStruct sModelStruct;
                pGff->GetListElement(&sModelStruct, &stuntList, static_cast<unsigned int>(k));
                m_paStuntModels[k].m_sObject = pGff->ReadFieldCExoString(
                    &sModelStruct, const_cast<char*>("Participant"), nReadOk, CExoString(""));
                m_paStuntModels[k].m_rModel = pGff->ReadFieldCResRef(
                    &sModelStruct, const_cast<char*>("StuntModel"), nReadOk, CResRef(""));
            }
        }
    }
    else
    {
        m_paStuntModels = 0;
    }

    return nReadOk;

fail:
    Cleanup();
    return nReadOk;
}

void CSWSDialog::LoadDialogBase(CSWSDialogBase& base, CResGFF* pGff, CResStruct& sStruct,
                                unsigned int& nDefaultDelay, int& nReadOk)
{
    int bSuccess = 0;
    int bFadeSuccess = 0;

    base.m_sText = pGff->ReadFieldCExoLocString(&sStruct, const_cast<char*>("Text"), nReadOk, CExoLocString());

    base.m_rScript = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("Script"), bSuccess, CResRef(""));
#if !KOTOR1
    base.m_nActionParam1 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam1"), bSuccess, 0);
    base.m_nActionParam2 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam2"), bSuccess, 0);
    base.m_nActionParam3 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam3"), bSuccess, 0);
    base.m_nActionParam4 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam4"), bSuccess, 0);
    base.m_nActionParam5 = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam5"), bSuccess, 0);

    base.m_rScript2 = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("Script2"), bSuccess, CResRef(""));
    base.m_nActionParam1b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam1b"), bSuccess, 0);
    base.m_nActionParam2b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam2b"), bSuccess, 0);
    base.m_nActionParam3b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam3b"), bSuccess, 0);
    base.m_nActionParam4b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam4b"), bSuccess, 0);
    base.m_nActionParam5b = pGff->ReadFieldINT(&sStruct, const_cast<char*>("ActionParam5b"), bSuccess, 0);
    base.m_sActionParamStrA = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("ActionParamStrA"),
                                                       bSuccess, CExoString(""));
    base.m_sActionParamStrB = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("ActionParamStrB"),
                                                       bSuccess, CExoString(""));
    base.m_nNodeUnskippable
        = pGff->ReadFieldINT(&sStruct, const_cast<char*>("NodeUnskippable"), bSuccess, 0);
#endif
    base.m_sSpeaker = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("Speaker"), bSuccess,
                                                CExoString(""));
    base.m_nWaitFlags = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("WaitFlags"), bSuccess, 0);
    base.m_sQuest
        = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("Quest"), bSuccess, CExoString(""));
    base.m_nQuestEntry = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("QuestEntry"), bSuccess, 0);
    base.m_nPlotIndex = pGff->ReadFieldINT(&sStruct, const_cast<char*>("PlotIndex"), bSuccess, 0);
    base.m_fPlotXPPercentage
        = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("PlotXPPercentage"), bSuccess, 0.0f);
    base.m_nDelay = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("Delay"), bSuccess, 0);
    base.m_nFadeType = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("FadeType"), bFadeSuccess, 0);
    base.m_vFadeColor
        = pGff->ReadFieldVector(&sStruct, const_cast<char*>("FadeColor"), bSuccess, Vector(0.0f, 0.0f, 0.0f));
    base.m_fFadeDelay = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("FadeDelay"), bSuccess, 0.0f);
    base.m_fFadeLength = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("FadeLength"), bSuccess, 0.0f);
    base.m_nEstimateDelay = -1;
#if KOTOR1
    base.m_rVO_ResRef = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("Sound"), bSuccess, CResRef(""));
#else
    base.m_rVO_ResRef = const_cast<char*>("");
#endif
    CResRef rSound
        = pGff->ReadFieldCResRef(&sStruct, const_cast<char*>("VO_ResRef"), bSuccess, CResRef(""));
    base.m_nSoundExists = pGff->ReadFieldBYTE(&sStruct, const_cast<char*>("SoundExists"), bSuccess, CSWSDialogBase::SOUND_EXISTS_UNSET);

#if !KOTOR1
    if (!base.m_rVO_ResRef.IsValid())
    {
        base.m_nAlienRaceNode = pGff->ReadFieldINT(&sStruct, const_cast<char*>("AlienRaceNode"), bSuccess, 0);
        base.m_nEmotion = pGff->ReadFieldINT(&sStruct, const_cast<char*>("Emotion"), bSuccess, 0);
        base.m_nFacialAnim = pGff->ReadFieldINT(&sStruct, const_cast<char*>("FacialAnim"), bSuccess, 0);

        int nAlienRaceNode;
        if (base.m_sSpeaker == "")
        {
            nAlienRaceNode = base.m_nAlienRaceNode;
            if (nAlienRaceNode <= 0)
            {
                nAlienRaceNode = m_nAlienRaceOwner;
                if (nAlienRaceNode > 0)
                    goto alienVO;
                goto skipAlienVO;
            }
        }
        else
        {
            nAlienRaceNode = base.m_nAlienRaceNode;
            if (nAlienRaceNode < 1)
                goto skipAlienVO;
        }
    alienVO:
        {
            CExoString sLabel;
            CExoString sRef;
            if (g_pRules->m_p2DArrays->m_pEmotion->GetCExoStringEntry(
                    base.m_nEmotion, CExoString("LABEL"), &sLabel))
            {
                if (g_pRules->m_p2DArrays->m_pAlienVO->GetCExoStringEntry(
                        nAlienRaceNode, sLabel, &sRef))
                {
                    base.m_rVO_ResRef = sRef;
                    if (base.m_rVO_ResRef.IsValid())
                        base.m_nSoundExists |= 3;
                }
            }
        }
    skipAlienVO:;
    }
#endif

    if (base.m_rVO_ResRef.IsValid())
    {
        base.m_bVOResRefUnused = 1;
    }
    else
    {
        base.m_rVO_ResRef = rSound;
        if (!base.m_rVO_ResRef.IsValid())
        {
            base.m_bVOResRefUnused = 1;
            base.m_nSoundExists = 0;
        }
    }

    if (base.m_nDelay == 0xffffffff)
    {
        if (base.m_rVO_ResRef.IsValid())
        {
            base.m_nDelay = nDefaultDelay;
            if (base.m_nWaitFlags == 0)
                base.m_nWaitFlags = 2;
        }
        else if (base.m_nWaitFlags == 0)
        {
            base.m_rVO_ResRef = const_cast<char*>("");
        }
        else
        {
            base.m_nEstimateDelay = 0;
            base.m_nDelay = nDefaultDelay;
        }
    }
    else
    {
        base.m_nWaitFlags |= CSWSDialogBase::WAIT_FLAG_FIXED_DELAY;
    }

    if (base.m_nFadeType == 1 || base.m_nFadeType == 2)
        base.m_fFadeLength = 0.0f;
    if (base.m_nFadeType == 0 || bFadeSuccess == 0)
    {
        base.m_nFadeType = 0;
        base.m_vFadeColor = Vector(0.0f, 0.0f, 0.0f);
        base.m_fFadeLength = 0.0f;
        base.m_fFadeDelay = 0.0f;
    }

    CResList animList;
    pGff->GetList(&animList, &sStruct, const_cast<char*>("AnimList"));
    base.m_nAnimCount = pGff->GetListCount(&animList);
    if (base.m_nAnimCount != 0)
    {
        base.m_paAnimList = new CSWSDialogAnim[base.m_nAnimCount];
        for (unsigned int i = 0; i < base.m_nAnimCount; i++)
        {
            CResStruct animStruct;
            pGff->GetListElement(&animStruct, &animList, i);
            base.m_paAnimList[i].m_sParticipant
                = pGff->ReadFieldCExoString(&animStruct, const_cast<char*>("Participant"), nReadOk,
                                            CExoString(""));
            base.m_paAnimList[i].m_nAnimation
                = pGff->ReadFieldWORD(&animStruct, const_cast<char*>("Animation"), nReadOk, 0);
        }
    }
}

void CSWSDialog::LoadDialogCamera(CSWSDialogCamera& camera, CResGFF* pGff, CResStruct& sStruct,
                                  int& nUnused)
{
    K2_UNUSED(nUnused);
    int bSuccess;

    camera.m_sListener
        = pGff->ReadFieldCExoString(&sStruct, const_cast<char*>("Listener"), bSuccess, CExoString(""));
    camera.m_nCameraAngle
        = pGff->ReadFieldDWORD(&sStruct, const_cast<char*>("CameraAngle"), bSuccess, 0);
    camera.m_nCameraID
        = pGff->ReadFieldINT(&sStruct, const_cast<char*>("CameraID"), bSuccess, 0);
    camera.m_fHeightOffset
        = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("CamHeightOffset"), bSuccess, 0.0f);
    camera.m_fTargetHeightOffset
        = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("TarHeightOffset"), bSuccess, 0.0f);
    camera.m_nCameraAnimation
        = pGff->ReadFieldWORD(&sStruct, const_cast<char*>("CameraAnimation"), bSuccess, 0);
    camera.m_nCamVidEffect
        = pGff->ReadFieldINT(&sStruct, const_cast<char*>("CamVidEffect"), bSuccess, -1);

    float fFov = pGff->ReadFieldFLOAT(&sStruct, const_cast<char*>("CamFieldOfView"), bSuccess, 0.0f);
    if (!(fFov <= 0.0f) && bSuccess != 0)
        camera.m_fFieldOfView = fFov;
    else
        camera.m_fFieldOfView = -1.0f;

    if (camera.m_nCameraAngle != 6)
        camera.m_nCameraID = -1;
}

int CSWSDialog::SendDialogModels(CSWSObject* pObj, unsigned int nPlayerId)
{
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CResRef* paResRefs = new CResRef[m_nModelCount];
    unsigned long long* paObjectIds = new unsigned long long[m_nModelCount];

    for (unsigned long long i = 0; i < m_nModelCount; i++)
    {
        paObjectIds[i] = OBJECT_INVALID;

        unsigned long long nId;
        if (m_paStuntModels[i].m_sObject == "PLAYER")
            nId = m_lstParticipants.m_pInternal->m_pHead != 0 ? m_oidCurrentPlayer : OBJECT_INVALID;
        else if (m_paStuntModels[i].m_sObject == "OWNER" || m_paStuntModels[i].m_sObject == "")
            nId = m_ownerId;
        else
            nId = pObj->GetNearestObjectByName(m_paStuntModels[i].m_sObject, 0.0f);

        paObjectIds[i] = pClientApp->ServerToClientObjectId(nId);
        paResRefs[i] = m_paStuntModels[i].m_rModel;
    }

    if (pClientApp->GetInGameGui()->HandleDialogModels(nPlayerId, m_nModelCount, paObjectIds,
                                                       paResRefs, m_rCameraModel,
                                                       m_rAmbientTrack))
        g_pAppManager->m_pClientExoApp->SetDialogModelsSent(1);

    delete[] paObjectIds;
    delete[] paResRefs;
    return 1;
}

unsigned long long CSWSDialog::GetDialogModel(int nIndex, int& nFlag)
{
    nFlag = 0;
    if (nIndex < 0)
        return OBJECT_INVALID;
    if (static_cast<int>(m_nModelCount) <= nIndex)
        return OBJECT_INVALID;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_ownerId);
    if (pObject == 0)
        return OBJECT_INVALID;
    if (pObject->AsSWSObject() == 0)
        return OBJECT_INVALID;
    CSWSObject* pOwner = pObject->AsSWSObject();

    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;

    if (m_paStuntModels[nIndex].m_sObject == "PLAYER")
        return pClientApp->ServerToClientObjectId(
            m_lstParticipants.m_pInternal->m_pHead != 0 ? m_oidCurrentPlayer : OBJECT_INVALID);
    if (m_paStuntModels[nIndex].m_sObject == "OWNER"
        || m_paStuntModels[nIndex].m_sObject == "")
        return pClientApp->ServerToClientObjectId(m_ownerId);

    unsigned long long nModelId
        = pOwner->GetNearestObjectByName(m_paStuntModels[nIndex].m_sObject, 0.0f);
    unsigned long long nClientId = pClientApp->ServerToClientObjectId(nModelId);
    nFlag = pClientApp->GetInGameGui()->IsOnDialogNonDependantList(
        m_paStuntModels[nIndex].m_sObject);
    return nClientId;
}
int CSWSDialog::RemovePlayer(unsigned long long objectId)
{
    CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
    CSWSDialogPlayer* pPlayer = 0;
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.m_pInternal->m_pHead->m_pData);
    bool bAtHead = true;
    while (pPlayer != 0)
    {
        if (pPlayer->m_objectId == objectId)
        {
            if (m_lstParticipants.m_pInternal->m_nCount == 1)
                return 0;
            if (bAtHead && m_bFirstPlayerOnly != 0)
                return 0;

            m_lstParticipants.Remove(pNode);
            bool bWasSpeaker = pPlayer->m_objectId == m_oidCurrentPlayer;
            delete pPlayer;
            if (bWasSpeaker)
            {
                CExoLinkedListNode* pNewHead = m_lstParticipants.m_pInternal->m_pHead;
                if (pNewHead != 0)
                {
                    CSWSDialogPlayer* pNewPlayer = static_cast<CSWSDialogPlayer*>(pNewHead->m_pData);
                    if (pNewPlayer != 0)
                    {
                        m_oidCurrentPlayer = pNewPlayer->m_objectId;
                        m_nPlayerGender = pNewPlayer->m_nGender;
                    }
                }
            }
            return 1;
        }
        pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        bAtHead = false;
    }
    return m_lstParticipants.m_pInternal->m_pHead != 0;
}

void CSWSDialog::UnequipItemsForDialog(unsigned long long objectId)
{
    if (objectId == OBJECT_INVALID)
        return;

    unsigned long long clientId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objectId);
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(clientId);
    if (pObj != 0 && pObj->AsSWCCreature() != 0)
    {
        CSWCCreature* pCreature = pObj->AsSWCCreature();

        CExoLinkedListNode* pNode = m_lstUnequippedItems.m_pInternal->m_pHead;
        while (pNode != 0)
        {
            if (*static_cast<unsigned long long*>(m_lstUnequippedItems.GetAtPos(pNode))
                == pCreature->m_objectId)
                return;
            m_lstUnequippedItems.GetNext(pNode);
        }

        unsigned long long* pId = new unsigned long long;
        *pId = pCreature->m_objectId;
        m_lstUnequippedItems.AddHead(pId);
        pCreature->HideEquippedItems(CResRef(), CResRef());
    }
}

void CSWSDialog::UnequipHItemForDialog(unsigned long long objectId)
{
    if (objectId == OBJECT_INVALID)
        return;

    unsigned long long clientId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objectId);
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(clientId);
    if (pObj != 0 && pObj->AsSWCCreature() != 0)
    {
        CSWCCreature* pCreature = pObj->AsSWCCreature();

        CExoLinkedListNode* pNode = m_lstUnequippedHItems.m_pInternal->m_pHead;
        while (pNode != 0)
        {
            if (*static_cast<unsigned long long*>(m_lstUnequippedHItems.GetAtPos(pNode))
                == pCreature->m_objectId)
                return;
            m_lstUnequippedHItems.GetNext(pNode);
        }

        unsigned long long* pId = new unsigned long long;
        *pId = pCreature->m_objectId;
        m_lstUnequippedHItems.AddHead(pId);
        pCreature->HideEquippedHeadItem();
    }
}

CSWSObject* CSWSDialog::GetSpeaker(CSWSObject* pObj, const CExoString& sName)
{
    unsigned long long idSpeaker K2_UNSET;
    unsigned int i = 0;
    for (; i < m_nSpeakerCount; i++)
        if (m_paSpeakers[i].m_sName == sName)
        {
            idSpeaker = m_paSpeakers[i].m_objectId;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // a cached speaker is trusted without checking that it still exists
            break;
#else
            if (g_pAppManager->m_pServerExoApp->GetGameObject(idSpeaker) != 0)
                break;
            if (m_paSpeakers != 0)
                delete[] m_paSpeakers;
            m_paSpeakers = 0;
            m_nSpeakerCount = 0;
            i = 0;
            break;
#endif
        }

    if (i == m_nSpeakerCount)
    {
        if (pObj->m_sTag == sName)
            idSpeaker = pObj->m_objectId;
        else
            idSpeaker = pObj->GetNearestObjectByName(
                sName, CSWSArea::MAX_OBJECT_CONVERSATION_DISTANCE);

        CSWSDialogSpeaker* paSpeakers = m_paSpeakers;
        m_paSpeakers = new CSWSDialogSpeaker[m_nSpeakerCount + 1];
        for (unsigned int j = 0; j < m_nSpeakerCount; j++)
        {
            m_paSpeakers[j].m_sName = paSpeakers[j].m_sName;
            m_paSpeakers[j].m_objectId = paSpeakers[j].m_objectId;
        }
        m_paSpeakers[m_nSpeakerCount].m_sName = sName;
        m_paSpeakers[m_nSpeakerCount].m_objectId = idSpeaker;
        m_nSpeakerCount++;
        if (paSpeakers != 0)
            delete[] paSpeakers;
    }

    if (idSpeaker == OBJECT_INVALID)
        return 0;

    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(idSpeaker, &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    CSWSObject* pSpeaker = static_cast<CSWSObject*>(pObject);

    float fDistX = pSpeaker->m_position.x - pObj->m_position.x;
    float fDistY = pSpeaker->m_position.y - pObj->m_position.y;
    float fDistZ = pSpeaker->m_position.z - pObj->m_position.z;
    float fMaxDist = CSWSArea::MAX_OBJECT_CONVERSATION_DISTANCE;
    if (pSpeaker->GetArea() != pObj->GetArea()
#if KOTOR1  // no ForceAlwaysUpdate override
        || fDistX * fDistX + fDistY * fDistY + fDistZ * fDistZ > fMaxDist * fMaxDist)
#else
        || (fDistX * fDistX + fDistY * fDistY + fDistZ * fDistZ > fMaxDist * fMaxDist
            && pSpeaker->m_bForceAlwaysUpdate == 0))
#endif
    {
        if (pSpeaker->m_dialogOwnerId == m_ownerId)
            ClearDialogOwnerInObject(idSpeaker);
        return 0;
    }

    if (pSpeaker->m_dialogOwnerId != OBJECT_INVALID)
    {
        if (pSpeaker->m_dialogOwnerId == m_ownerId)
            return pSpeaker;

        if (g_pAppManager->m_pServerExoApp->GetObjectArray()
                ->GetGameObject(pSpeaker->m_dialogOwnerId, &pObject) == CGameObjectArray::SUCCESS
            && static_cast<CSWSObject*>(pObject)->m_dialogOwnerId == pSpeaker->m_dialogOwnerId)
            return 0;

        pSpeaker->SetDialogOwner(OBJECT_INVALID);
        if (pSpeaker->m_dialogOwnerId != OBJECT_INVALID)
            return pSpeaker;
    }

    pSpeaker->SetDialogOwner(m_ownerId);
    if (pSpeaker->AsSWSCreature() != 0)
    {
        pSpeaker->AsSWSCreature()->ActionManager(2);
        pSpeaker->AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 1, 0);
    }
    return pSpeaker;
}

void CSWSDialog::Cleanup()
{
    CSWSObject* pOwner = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(m_ownerId));
    if (pOwner != 0)
    {
        if (pOwner->AsSWSObject() != 0)
        {
            CSWSObject* pSpeaker = pOwner->AsSWSObject();
            pSpeaker->m_aiLevel = m_ownerSavedAiLevel;
            pOwner->AsSWSObject()->StopSoundPlayingInDialog();
        }
#if !KOTOR1  // K1 restores the owner's AI level only from the saved value
        unsigned char nSavedAILevel = pOwner->AsSWSObject()->m_nSavedAILevel;
        pOwner->AsSWSObject()->m_aiLevel = nSavedAILevel;
        pOwner->AsSWSObject()->m_nSavedAILevel = CSWSObject::SAVED_AI_LEVEL_NONE;
#endif
    }

    ClearDialogOwnerInObject(m_ownerId);

    if (m_lstParticipants.m_pInternal->m_pHead == 0)
        ClearDialogOwnerInObject(m_oidCurrentPlayer);

    while (m_lstParticipants.m_pInternal->m_pHead != 0)
    {
        CSWSDialogPlayer* pPlayer = m_lstParticipants.RemoveHead();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->CloseDialog();
        ClearDialogOwnerInObject(pPlayer->m_objectId);
        delete pPlayer;
    }

    if (m_paSpeakers != 0)
    {
        for (unsigned int i = 0; i < m_nSpeakerCount; i++)
            ClearDialogOwnerInObject(m_paSpeakers[i].m_objectId);

        delete[] m_paSpeakers;
        m_paSpeakers = 0;
        m_nSpeakerCount = 0;
    }

    if (m_paEntries != 0)
    {
        for (unsigned int i = 0; i < m_nEntryCount; i++)
        {
            if (m_paEntries[i].m_paReplies != 0)
            {
                delete[] m_paEntries[i].m_paReplies;
                m_paEntries[i].m_paReplies = 0;
            }
            if (m_paEntries[i].m_paAnimList != 0)
            {
                delete[] m_paEntries[i].m_paAnimList;
                m_paEntries[i].m_paAnimList = 0;
                m_paEntries[i].m_nAnimCount = 0;
            }
        }

        delete[] m_paEntries;
        m_paEntries = 0;
        m_nEntryCount = 0;
    }

    if (m_paReplies != 0)
    {
        for (unsigned int i = 0; i < m_nReplyCount; i++)
        {
            if (m_paReplies[i].m_paEntries != 0)
            {
                delete[] m_paReplies[i].m_paEntries;
                m_paReplies[i].m_paEntries = 0;
            }
            if (m_paReplies[i].m_paAnimList != 0)
            {
                delete[] m_paReplies[i].m_paAnimList;
                m_paReplies[i].m_paAnimList = 0;
                m_paReplies[i].m_nAnimCount = 0;
            }
        }

        delete[] m_paReplies;
        m_paReplies = 0;
        m_nReplyCount = 0;
    }

    if (m_paStartList != 0)
    {
        delete[] m_paStartList;
        m_paStartList = 0;
        m_nStartCount = 0;
    }

    m_bRepliesPending = 0;
    m_bBark = 0;
    m_nCurrentEntry = 0;
    m_nHandledReply = NODE_INDEX_NONE;
    m_oidCurrentPlayer = OBJECT_INVALID;
    m_ownerId = OBJECT_INVALID;
    m_nPlayerGender = CTlkTable::GENDER_MALE;
    m_bFirstPlayerOnly = 0;
    m_bPrivate = 0;
}

#if KOTOR1  // the line in the player's server-side language; no generic bark list, no random VO variant
void CSWSDialog::BarkOneLine(int nEntry)
{
    CExoString sText;
    CResRef rSound;

    unsigned long long nPlayerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CExoLocString& rText = m_paEntries[nEntry].m_sText;
    rText.GetString(g_pAppManager->m_pServerExoApp->GetPlayerLanguage(static_cast<unsigned int>(nPlayerId)), &sText,
                    m_nPlayerGender);

    rSound = m_paEntries[nEntry].m_rVO_ResRef;

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 0;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(1, 0.0f, 1.0f, Vector());
    }

    if (!rSound.IsValid())
    {
        STR_RES sRes;
        unsigned int nStrRef = static_cast<unsigned int>(m_paEntries[nEntry].m_sText.m_nStrRef);
        g_pTlkTable->Fetch(nStrRef, sRes, CTlkTable::GENDER_MALE);
        rSound = sRes.soundResRef;
    }

    if (sText.CompareNoCase(""))
        return;

    CGameObject* pObject;
    unsigned long long nSpeakerId;
    unsigned long long nClientId;
    CGameObject* pOwnerObject;
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nPlayerId);
    if (pObject == 0)
        return;

    if (m_paEntries->m_sSpeaker == "")
        nSpeakerId = m_ownerId;
    else
        nSpeakerId = pObject->AsSWSObject()->GetNearestObjectByName(m_paEntries->m_sSpeaker, 0.0f);

    nClientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nSpeakerId);
    int bIgnoreRange = g_pAppManager->m_pServerExoApp->GetIgnoreConversationStartRange();
    g_pAppManager->m_pClientExoApp->BarkString(nClientId, sText, rSound, bIgnoreRange);

    if (nPlayerId != OBJECT_INVALID && g_bLogConversations)
    {
        CGameObject* pLogged = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        CExoString sMsg;
        CSWCCreature* pLoggedCreature;
        if (pLogged != 0 && pLogged->AsSWCCreature() != 0
            && (pLoggedCreature = pLogged->AsSWCCreature()) != 0)
        {
            sMsg = pLoggedCreature->m_pStats->GetFullName();
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sMsg + ": " + sText, 1, 0);
        }
    }

    pOwnerObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_ownerId);
    if (pOwnerObject != 0)
    {
        CSWSObject* pOwner = pOwnerObject->AsSWSObject();
        if (pOwner != 0)
        {
            m_bBark = 1;
            RunEndConversationScript(pOwner, 1);
        }
    }
}
#else
// EFFECTIVE: the g_pTlkTable GOT load before the Fetch is scheduled one instruction earlier.
void CSWSDialog::BarkOneLine(int nEntry)
{
    CExoString sText;
    CResRef rSound;

    unsigned long long nPlayerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    unsigned long long nSpeakerId;
    unsigned long long nClientId;
    int nStrRef;

    m_paEntries[nEntry].m_sText.GetString(
        g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, m_nPlayerGender);

    if (m_paEntries[nEntry].m_sText.m_nStrRef == STRREF_TUTORIAL_PLASMA_TORCH)
    {
        // Keep everything up to and including the second-to-last '.', dropping the VO variant tail. The
        // walk stops as soon as Find fails, and nDot lags one dot behind nLastDot.
        int nPos = 0;
        int nDot = 0;
        int nLastDot = 0;
        while (nPos != -1)
        {
            nPos = sText.Find('.', nPos);
            if (nPos != -1)
            {
                nDot = nLastDot;
                nLastDot = nPos;
                nPos++;
            }
        }
        if (nDot != 0)
            sText = sText.Left(nDot + 1);
    }

    rSound = m_paEntries[nEntry].m_rVO_ResRef;

    nStrRef = static_cast<int>(m_paEntries[nEntry].m_sText.m_nStrRef);
    switch (nStrRef)
    {
    case STRREF_TUTORIAL_LOCKER_KEYCARD:
    case STRREF_TUTORIAL_NEW_QUESTS:
    case STRREF_TUTORIAL_PARTY_MEMBER:
    case STRREF_TUTORIAL_DIALOG:
    case STRREF_TUTORIAL_TARGETING:
    case STRREF_TUTORIAL_EQUIP_WEAPON:
    case STRREF_TUTORIAL_SOLO_MODE:
    case STRREF_TUTORIAL_DROID_FLAMETHROWER:
    case STRREF_TUTORIAL_LOCKER_KEYCARD_2:
    case STRREF_TUTORIAL_DROID_FLAMETHROWER_2:
    case STRREF_TUTORIAL_REPAIR_KIT:
    case STRREF_TUTORIAL_BASH:
    case STRREF_TUTORIAL_PLOT_COURSE:
    case STRREF_TUTORIAL_MAP:
    case STRREF_TUTORIAL_MOVE_T3M4:
    case STRREF_TUTORIAL_TARGET_OBJECTS:
        rSound = CResRef();
        break;
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 0;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(1, 0.0f, 1.0f, Vector());
    }

    if (rSound.IsValid())
    {
        if (m_paEntries[nEntry].m_nSoundExists & 2)
        {
            CExoString sNum;
            sNum.Format(const_cast<char*>("%d"), rand_wincompatible() % 3 + 1);
            rSound += sNum;
        }
    }
    else
    {
        STR_RES sRes;
        g_pTlkTable->Fetch(static_cast<unsigned int>(m_paEntries[nEntry].m_sText.m_nStrRef), sRes, CTlkTable::GENDER_MALE);
        rSound = sRes.soundResRef;
    }

    if (sText.CompareNoCase(""))
        return;

    // The speaker name is read through the array pointer, so it is the first entry's rather than this
    // entry's.
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nPlayerId);
    if (pObject == 0)
        return;

    if (m_paEntries->m_sSpeaker == "")
        nSpeakerId = m_ownerId;
    else
        nSpeakerId = pObject->AsSWSObject()->GetNearestObjectByName(m_paEntries->m_sSpeaker, 0.0f);

    nClientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nSpeakerId);
    CGameObject* pOwnerObject;
    g_pAppManager->m_pClientExoApp->BarkString(nClientId, sText, rSound, 0);

    if (nPlayerId != OBJECT_INVALID && g_bLogConversations)
    {
        CGameObject* pLogged = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        CExoString sMsg;
        CSWCCreature* pLoggedCreature;
        if (pLogged != 0 && pLogged->AsSWCCreature() != 0
            && (pLoggedCreature = pLogged->AsSWCCreature()) != 0)
        {
            sMsg = pLoggedCreature->m_pStats->m_name;
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sMsg + ": " + sText, 1, 0);
        }
    }

    pOwnerObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_ownerId);
    if (pOwnerObject != 0)
    {
        CSWSObject* pOwner = pOwnerObject->AsSWSObject();
        if (pOwner != 0)
        {
            m_bBark = 1;
            RunEndConversationScript(pOwner, 1);
        }
    }
}
#endif

int CSWSDialog::SendDialogEntryNode(unsigned int nEntry, unsigned long long objIdA,
                                    unsigned long long objIdC, unsigned long long objIdB,
                                    unsigned long long objIdD, unsigned char gender,
                                    CSWSObject* pObj, CSWSDialogEntry& entry,
                                    unsigned int& nResult)
{
    // The entry's AnimList resolves to three parallel arrays, all indexed by the same counter,
    // holding the client object id each animated participant resolved to, the animation word to
    // play on it, and, for the name-lookup branch only, the "not on the non-dependant list" flag.
    int nAnimCount = entry.m_nAnimCount;
    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    unsigned long long* paClientIds = 0;
    unsigned short* paAnims = 0;
    int* paFlags = 0;
    int bHasAnimList = 0;
    if (nAnimCount != 0)
    {
        paClientIds = new unsigned long long[static_cast<unsigned int>(nAnimCount)];
        paAnims = new unsigned short[static_cast<unsigned int>(nAnimCount)];
        paFlags = new int[static_cast<unsigned int>(nAnimCount)];
        bHasAnimList = nAnimCount > 0;
        for (int i = 0; i < nAnimCount; i++)
        {
            unsigned long long objectId;
            if (entry.m_paAnimList[i].m_sParticipant == "PLAYER")
            {
                // While the conversation has participants the listener id is the one IsDialogDelay
                // publishes; with an empty list there is nobody to listen, so the id is OBJECT_INVALID
                // and the animation is skipped below.
                objectId = (m_lstParticipants.m_pInternal->m_pHead != 0) ? m_oidCurrentPlayer : OBJECT_INVALID;
            }
            else if (entry.m_paAnimList[i].m_sParticipant == "OWNER")
            {
                objectId = m_ownerId;
            }
            else if (entry.m_paAnimList[i].m_sParticipant == "NPC1")
            {
                CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
                objectId = (pMember != 0)
                    ? g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pMember->m_objectId)
                    : OBJECT_INVALID;
            }
            else if (entry.m_paAnimList[i].m_sParticipant == "NPC2")
            {
                CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
                objectId = (pMember != 0)
                    ? g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pMember->m_objectId)
                    : OBJECT_INVALID;
            }
            else
            {
                objectId = pObj->GetNearestObjectByName(entry.m_paAnimList[i].m_sParticipant, 0.0f);
                paFlags[i] = (g_pAppManager->m_pClientExoApp->GetInGameGui()
                                  ->IsOnDialogNonDependantList(entry.m_paAnimList[i].m_sParticipant) == 0);
            }
            if (g_pAppManager->m_pServerExoApp->GetGameObject(objectId) != 0)
            {
                paClientIds[i] = pClientExoApp->ServerToClientObjectId(objectId);
                paAnims[i] = entry.m_paAnimList[i].m_nAnimation;
            }
            else
            {
                CExoString sError;
                sError.Format(const_cast<char*>("Error: dialogue can't find object '%s'!"), entry.m_paAnimList[i].m_sParticipant.CStr());
                paClientIds[i] = OBJECT_INVALID;
                paAnims[i] = 0;
            }
        }
    }

    // Each of the three listening objects must exist as a client object and have reached the client. A
    // server object that never spawned client-side defers the whole entry.
    if (objIdB != OBJECT_INVALID)
    {
        CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(objIdB);
        if (pServerObject == 0 || pServerObject->AsSWSObject() == 0)
        {
            nResult = 10;
            return 1;
        }
        nResult = 0;
        unsigned long long nClientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objIdB);
        CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pClientObject == 0 || pClientObject->AsSWCObject() == 0)
        {
            nResult = 10;
#if !KOTOR1  // K1 has no "Waiting on listening object" message
            CExoString sMessage(const_cast<char*>("Waiting on listening object to spawn clientside: "));
            CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(objIdB);
            sMessage = sMessage + CExoString(pServerObject->AsSWSObject()->m_sTag.CStr());
#endif
            return 1;
        }
        nResult = 0;
    }
    if (objIdC != OBJECT_INVALID)
    {
        CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(objIdC);
        if (pServerObject == 0 || pServerObject->AsSWSObject() == 0)
        {
            nResult = 10;
            return 1;
        }
        nResult = 0;
        unsigned long long nClientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objIdC);
        CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pClientObject == 0 || pClientObject->AsSWCObject() == 0)
        {
            nResult = 10;
            return 1;
        }
        nResult = 0;
    }
    if (objIdD != OBJECT_INVALID)
    {
        CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(objIdD);
        if (pServerObject == 0 || pServerObject->AsSWSObject() == 0)
        {
            nResult = 10;
            return 1;
        }
        nResult = 0;
        unsigned long long nClientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objIdD);
        CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pClientObject == 0 || pClientObject->AsSWCObject() == 0)
        {
            nResult = 10;
            return 1;
        }
        nResult = 0;
    }

    // How long since the last attempt, so the re-arm below and the wait loop can tell a fresh try from
    // one that has already staled.
    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nElapsedUnused;
    unsigned int nElapsed;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nParticipantWaitDay, m_nParticipantWaitTod, &nElapsedUnused, &nElapsed);
    m_nParticipantWaitDay = nDay;
    m_nParticipantWaitTod = nTod;
    int bExpired = 0;
    if (m_nParticipantWaitMs != 0)
    {
        if (static_cast<unsigned int>(m_nParticipantWaitMs) > nElapsed)
        {
            m_nParticipantWaitMs -= nElapsed;
        }
        else
        {
            m_nParticipantWaitMs = 0;
            bExpired = 1;
        }
    }

    // The name-resolved participants have to be present on both halves too. Once the wait has expired
    // the check is skipped and the line goes out anyway.
    if (bHasAnimList)
    {
        for (int i = 0; i < nAnimCount; i++)
        {
            if (paFlags[i] == 0)
                continue;
            CGameObject* pServerObject = g_pAppManager->m_pServerExoApp->GetGameObject(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(paClientIds[i]));
            if ((pServerObject != 0 && pServerObject->AsSWSObject() != 0) || bExpired)
                nResult = 0;
            else
            {
                nResult = 10;
                break;
            }
            CGameObject* pClientObject = g_pAppManager->m_pClientExoApp->GetGameObject(paClientIds[i]);
            if ((pClientObject != 0 && pClientObject->AsSWCObject() != 0) || bExpired)
                nResult = 0;
            else
            {
                nResult = 10;
                break;
            }
        }
    }
    if (nResult == 10)
    {
        // Re-arm the wait, so the deferred entry is retried later.
        if (m_nParticipantWaitMs == 0)
            m_nParticipantWaitMs = 1500;
        delete[] paClientIds;
        delete[] paAnims;
        delete[] paFlags;
        return 1;
    }

    // Pick this playthrough's camera angle and alien-VO variation, and commit the angle to the entry.
    // A zero camera angle means the DLG picked none; the delay flag raised by SetDialogDelay then
    // forces the stock camera instead of the randomized angle.
    int nCameraAngle = AssignPseudoRandomCameraAngle();
    if (entry.m_camera.m_nCameraAngle == 0)
    {
        if (m_bFirstEntryLine != 0)
            entry.m_camera.m_nCameraAngle = 2;
        else
            entry.m_camera.m_nCameraAngle = nCameraAngle;
    }
    m_bFirstEntryLine = 0;
#if !KOTOR1
    entry.m_camera.m_nAlienVOVariation = AssignPseudoRandomAlienVOVariation();
#endif

    CExoString sText = GetExoStringFromLocString(entry.m_sText, gender);

    // Hide the listening objects' equipment for the duration of the line, remembering the ones just
    // hidden so nothing is restored twice.
    if (m_bUnequipItems != 0)
    {
        UnequipItemsForDialog(objIdC);
        UnequipItemsForDialog(objIdB);
        UnequipItemsForDialog(objIdD);
        for (int i = 0; i < nAnimCount; i++)
            UnequipItemsForDialog(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(paClientIds[i]));
    }
    if (m_bUnequipHItem != 0)
    {
        UnequipHItemForDialog(objIdC);
        UnequipHItemForDialog(objIdB);
        UnequipHItemForDialog(objIdD);
        for (int i = 0; i < nAnimCount; i++)
            UnequipHItemForDialog(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(paClientIds[i]));
    }

    // bPlaySound starts at 1; the entry's own flag replaces it unless the entry is marked "no sound",
    // the spoken line exists, and the user has left sound on.
    int bPlaySound = 1;
    if (!(entry.m_nSoundExists & 2) && (entry.m_nSoundExists & 1) && g_bDisableSound != 1)
        bPlaySound = entry.m_bVOResRefUnused;

    // One specific talk-table line latches a global of its own. Nothing in the engine reads it.
#if !KOTOR1  // K1 has no s_oondarDone latch
    if (entry.m_sText.m_nStrRef == STRREF_OONDAR_GIVES_UP)
        s_oondarDone = 1;
#endif

    g_pAppManager->m_pClientExoApp->GetInGameGui()->HandleDialogEntry(
        nEntry,
        pClientExoApp->ServerToClientObjectId(objIdA),
        pClientExoApp->ServerToClientObjectId(objIdC),
        pClientExoApp->ServerToClientObjectId(objIdB),
        pClientExoApp->ServerToClientObjectId(objIdD),
        gender, sText, nAnimCount, paClientIds, paAnims,
        entry.m_camera.m_nCameraAngle, entry.m_camera.m_nCameraID,
        entry.m_camera.m_fHeightOffset, entry.m_camera.m_fTargetHeightOffset,
        entry.m_nWaitFlags, entry.m_camera.m_nCameraAnimation,
        entry.m_camera.m_fFieldOfView, entry.m_camera.m_nCamVidEffect,
        entry.m_nFadeType, entry.m_vFadeColor,
        entry.m_fFadeDelay, entry.m_fFadeLength, bPlaySound);

    delete[] paClientIds;
    delete[] paAnims;
    delete[] paFlags;
    return 1;
}

int CSWSDialog::SendDialogReplies(CSWSObject* pObj, unsigned int nPlayerId)
{
    int bResult = 0;
    if (m_nCurrentEntry >= m_nEntryCount)
        return bResult;

    unsigned int nReplyCount = m_paEntries[m_nCurrentEntry].m_nReplyCount;
    const unsigned int nSlots = nReplyCount + 1;
    CSWSDialogReply* paReplies = new CSWSDialogReply[nSlots];
    // Index 0 is the "no reply is available" placeholder: an empty loc string with the no-talk-table-row
    // sentinel and the no-reply index.
    paReplies[0].m_sText.m_nStrRef = 0xffffffff;
    paReplies[0].m_nLinkIndex = NODE_INDEX_NONE;

    CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
    CSWSDialogPlayer* pPlayer = 0;
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        pPlayer = static_cast<CSWSDialogPlayer*>(pNode->m_pData);

    bResult = 1;
    int bFirst = 1;
    unsigned long long nSavedId = 0;
    do
    {
        if (pPlayer != 0)
        {
            // A named player: skip every participant that is not the one asked for.
            if (nPlayerId != PLAYER_ID_ALL && pPlayer->m_nPlayerId != nPlayerId)
            {
                pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
                bFirst = 0;
                continue;
            }

            nSavedId = m_oidCurrentPlayer;
            m_oidCurrentPlayer = pPlayer->m_objectId;
        }
        else if (nPlayerId != PLAYER_ID_ALL)
            break;

        // Each pass stages at most one reply, so both the front slot and the back slot lie in
        // 1..nReplyCount.
        unsigned int nIndex = 1;
        unsigned int nChild = 0;
        for (unsigned long long i = 0; i < m_paEntries[m_nCurrentEntry].m_nReplyCount; i++)
        {
#if KOTOR1  // the link's one condition script decides alone
            int bUse = CheckScript(pObj, m_paEntries[m_nCurrentEntry].m_paReplies[i].m_rActive);
#else
            int nResult1;
            int bScript1;
            if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_rActive == const_cast<char*>(""))
            {
                bScript1 = 0;
                nResult1 = 1;
            }
            else
            {
                g_pVirtualMachine->SetScriptParams(
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam1,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam2,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam3,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam4,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam5,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_sParamStrA);
                nResult1 = CheckScript(pObj, m_paEntries[m_nCurrentEntry].m_paReplies[i].m_rActive);
                if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_bNot != 0)
                    nResult1 = (nResult1 == 0);
                bScript1 = 1;
            }

            int bUse = 1;
            if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_rActive2 == const_cast<char*>(""))
            {
                if (bScript1 != 0 && nResult1 == 0)
                    bUse = 0;
            }
            else
            {
                g_pVirtualMachine->SetScriptParams(
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam1b,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam2b,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam3b,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam4b,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nParam5b,
                    m_paEntries[m_nCurrentEntry].m_paReplies[i].m_sParamStrB);
                int nResult2 =
                    CheckScript(pObj, m_paEntries[m_nCurrentEntry].m_paReplies[i].m_rActive2);
                if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_bNot2 != 0)
                    nResult2 = (nResult2 == 0);

                int bResult2;
                if (bScript1 == 0)
                    bResult2 = nResult2;
                else if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nLogic == 0)
                    bResult2 = (nResult1 != 0) && (nResult2 != 0);
                else
                    bResult2 = (nResult1 != 0) || (nResult2 != 0);
                if (bResult2 == 0)
                    bUse = 0;
            }
#endif

            if (bUse != 0)
            {
                CSWSDialogReply* pReply =
                    &m_paReplies[m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nIndex];
                if (bFirst || m_bFirstPlayerOnly == 0)
                {
                    K2_ASSUME(nIndex < nSlots);
                    paReplies[nIndex] = *pReply;
                    paReplies[nIndex].m_nLinkIndex = static_cast<unsigned int>(i);
                    nIndex++;
                }
                else
                {
                    K2_ASSUME(nReplyCount - nChild < nSlots);
                    paReplies[nReplyCount - nChild] = *pReply;
                    paReplies[nReplyCount - nChild].m_nLinkIndex = static_cast<unsigned int>(i);
                    nChild++;
                }
                continue;
            }

            // A failing reply that the designer flagged as a child is still forwarded, staged from the end
            // of the array so the passing replies can grow forward past it.
            if (m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nIsChild != 0)
            {
                K2_ASSUME(nReplyCount - nChild < nSlots);
                paReplies[nReplyCount - nChild] =
                    m_paReplies[m_paEntries[m_nCurrentEntry].m_paReplies[i].m_nIndex];
                paReplies[nReplyCount - nChild].m_nLinkIndex = static_cast<unsigned int>(i);
                nChild++;
            }
        }

        // The child replies were staged from the end of the array; move them up behind the passing ones
        // so the two blocks are contiguous from index 1.
        for (unsigned int j = 0; j < nChild; j++)
            paReplies[nIndex + j] = paReplies[nReplyCount + 1 - nChild + j];

        if (pPlayer != 0)
        {
            m_oidCurrentPlayer = nSavedId;
            if (nIndex + nChild < 2)
            {
                // Nothing passed: send the sentinel reply alone.
                m_bBark = 1;
                // Re-arm before each send: an empty-text auto-continue reply is answered synchronously
                // inside SendDialogReplyNode, whose first gate rejects any reply while a reply is still
                // being handled.
                if (nPlayerId == PLAYER_ID_ALL)
                    m_nHandledReply = NODE_INDEX_NONE;
                if (bFirst || m_bFirstPlayerOnly == 0)
                    bResult = SendDialogReplyNode(pPlayer->m_nPlayerId, pPlayer->m_objectId,
                                                  pPlayer->m_nGender, paReplies, 1, 0,
                                                  m_nCurrentEntry, 1);
                else
                    bResult = SendDialogReplyNode(pPlayer->m_nPlayerId, pPlayer->m_objectId,
                                                  pPlayer->m_nGender, paReplies, nIndex - 1,
                                                  nChild, m_nCurrentEntry, 1);
            }
            else
            {
                if (nIndex == 2 &&
                    m_paReplies[m_paEntries[m_nCurrentEntry]
                                    .m_paReplies[paReplies[1].m_nLinkIndex]
                                    .m_nIndex]
                            .m_nEntryCount == 0)
                    m_bBark = 1;
                if (nPlayerId == PLAYER_ID_ALL)   // The same re-arm, before the send
                    m_nHandledReply = NODE_INDEX_NONE;
                bResult = SendDialogReplyNode(pPlayer->m_nPlayerId, pPlayer->m_objectId,
                                              pPlayer->m_nGender, paReplies + 1, nIndex - 1,
                                              nChild, m_nCurrentEntry, m_bBark);
            }

            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
            bFirst = 0;
        }
        else
        {
            // Nobody is left to be offered the list: answer the entry ourselves.
            unsigned int nReplyIndex;
            if (nIndex < 2)
            {
                m_bBark = 1;
                nReplyIndex = paReplies[0].m_nLinkIndex;
            }
            else
            {
                nReplyIndex = paReplies[1].m_nLinkIndex;
                if (nIndex == 2 &&
                    m_paReplies[m_paEntries[m_nCurrentEntry]
                                    .m_paReplies[nReplyIndex]
                                    .m_nIndex]
                            .m_nEntryCount == 0)
                    m_bBark = 1;
            }
            bResult = pObj->ReplyDialog(PLAYER_ID_ALL, nReplyIndex, 0, m_nCurrentEntry, 0);
        }
    } while (bResult != 0 && pNode != 0);

    delete[] paReplies;

    if (pObj->m_pDialog != 0)
    {
        if (nPlayerId == PLAYER_ID_ALL)
        {
            m_bRepliesPending = 1;
            m_nHandledReply = NODE_INDEX_NONE;
        }
        else if (m_nHandledReply != NODE_INDEX_NONE)
        {
            // The picked reply's camera is the one whose "Listener" decides what the client is told the
            // line is aimed at.
            unsigned long long nObjectId = m_oidCurrentPlayer;
            CExoString sListener(
                m_paReplies[m_paEntries[m_nCurrentEntry].m_paReplies[m_nHandledReply].m_nIndex]
                    .m_camera.m_sListener);
            if (sListener != "")
            {
                // CompareNoCase returns 1 on equal, so each arm here runs on a mismatch.
                if (sListener.CompareNoCase("OWNER"))
                    nObjectId = pObj->m_objectId;
                else if (sListener.CompareNoCase("PLAYER"))
                    nObjectId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
                else
                {
                    CSWSObject* pSpeaker = GetSpeaker(pObj, sListener);
                    if (pSpeaker != 0)
                        nObjectId = pSpeaker->m_objectId;
                }
            }

            CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
            CExoString sText = GetExoStringFromLocString(
                m_paReplies[m_paEntries[m_nCurrentEntry].m_paReplies[m_nHandledReply].m_nIndex]
                    .m_sText,
                m_nPlayerGender);
            pGui->HandleDialogReplyChosen(
                nPlayerId, m_nHandledReply, m_nCurrentEntry, sText,
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nObjectId), m_nPlayerGender,
                m_bBark);
        }
    }

    return bResult;
}

int CSWSDialog::SendDialogReplyNode(unsigned int nPlayerId, unsigned long long objectId,
                                    unsigned char nGender, CSWSDialogReply* pReplies,
                                    unsigned int nParam5, unsigned int nParam6,
                                    unsigned int nEntry, int bBark)
{
    int nReplyCount = nParam5 + nParam6;

    // A reply list that mixes empty and non-empty lines is the designer error the client shows as the
    // "Last Conversation Node" message.
    int bAnyNonEmpty = 0;
    int bAnyEmpty = 0;
    for (int i = 0; i < nReplyCount; i++)
    {
        CExoString sText;
#if KOTOR1  // the replying player's server-side language
        pReplies[i].m_sText.GetString(g_pAppManager->m_pServerExoApp->GetPlayerLanguage(nPlayerId),
                                     &sText, nGender);
#else
        pReplies[i].m_sText.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                     &sText, nGender);
#endif
        if (sText == "")
            bAnyEmpty = 1;
        else
            bAnyNonEmpty = 1;
    }
    if (bAnyNonEmpty && bAnyEmpty)
    {
        CExoString sError(const_cast<char*>("CONVERSATION ERROR: Last Conversation Node Contains Either an END NODE or CONTINUE NODE.  Please contact a Designer!"));
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetDialogError(sError);
        return 0;
    }

    // The conversation's owner, resolved to its server-side object. Nothing null-checks the game object;
    // the listener block below is what tests the AsSWSObject result.
    CGameObject* pOwnerGameObject;
    g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_ownerId, &pOwnerGameObject);
    CSWSObject* pOwner = pOwnerGameObject->AsSWSObject();

    // The 18 parallel arrays HandleDialogReplies consumes. Declaration order is the allocation order;
    // the text array is a cookie-carrying string array and the position array is value-initialized,
    // which is what the memset here is: a plain new[] would not zero.
    CExoString* paText = new CExoString[nReplyCount];
    unsigned int* paA = new unsigned int[nReplyCount];
    unsigned long long* paB = new unsigned long long[nReplyCount];
    unsigned int* paC = new unsigned int[nReplyCount];
    int* paD = new int[nReplyCount];
    float* paE = new float[nReplyCount];
    float* paF = new float[nReplyCount];
    unsigned int* paG = new unsigned int[nReplyCount];
    unsigned short* paH = new unsigned short[nReplyCount];
    float* paI = new float[nReplyCount];
    int* paJ = new int[nReplyCount];
    unsigned char* paK = new unsigned char[nReplyCount];
    Vector* paPos = new Vector[nReplyCount]();
    float* paM = new float[nReplyCount];
    float* paN = new float[nReplyCount];
    unsigned int* paCount = new unsigned int[nReplyCount];
    unsigned long long** paU = new unsigned long long*[nReplyCount];
    unsigned short** paT = new unsigned short*[nReplyCount];

    // The camera angle this line would play on, drawn once for the whole list.
    int nAssignedAngle = AssignPseudoRandomCameraAngle();

    for (int i = 0; i < nReplyCount; i++)
    {
        paText[i] = GetExoStringFromLocString(pReplies[i].m_sText, nGender);
        paA[i] = pReplies[i].m_nLinkIndex;

        // The reply's camera listener is a scripted role name, otherwise a speaker of this
        // conversation. A player listener resolves the player and discards it, leaving the invalid
        // object id.
        unsigned long long listenerId = OBJECT_INVALID;
        if (pOwner != 0 && pReplies[i].m_camera.m_sListener != "")
        {
            if (pReplies[i].m_camera.m_sListener.CompareNoCase("OWNER"))
            {
                listenerId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_ownerId);
            }
            else if (pReplies[i].m_camera.m_sListener.CompareNoCase("PLAYER"))
            {
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                    g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            }
            else
            {
                CSWSObject* pSpeaker = GetSpeaker(pOwner, pReplies[i].m_camera.m_sListener);
                if (pSpeaker != 0)
                    listenerId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                        pSpeaker->m_objectId);
            }
        }

        // The reply's own AnimList, resolved as in SendDialogEntryNode.
        unsigned int nAnimCount = pReplies[i].m_nAnimCount;
        paU[i] = 0;
        paCount[i] = nAnimCount;
        paT[i] = 0;
        if (nAnimCount != 0)
        {
            paU[i] = new unsigned long long[nAnimCount];
            paT[i] = new unsigned short[nAnimCount];
            for (int64_t j = 0; j < static_cast<int>(nAnimCount); j++)
            {
                unsigned long long animObjectId;
                if (pReplies[i].m_paAnimList[j].m_sParticipant == "PLAYER")
                {
                    animObjectId = (m_lstParticipants.m_pInternal->m_pHead != 0) ? m_oidCurrentPlayer
                                                                                : OBJECT_INVALID;
                }
                else if (pReplies[i].m_paAnimList[j].m_sParticipant == "OWNER")
                {
                    animObjectId = m_ownerId;
                }
                else if (pReplies[i].m_paAnimList[j].m_sParticipant == "NPC1")
                {
                    CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()
                                                ->GetCharacter(1);
                    animObjectId = (pMember != 0)
                        ? g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pMember->m_objectId)
                        : OBJECT_INVALID;
                }
                else if (pReplies[i].m_paAnimList[j].m_sParticipant == "NPC2")
                {
                    CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()
                                                ->GetCharacter(2);
                    animObjectId = (pMember != 0)
                        ? g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pMember->m_objectId)
                        : OBJECT_INVALID;
                }
                else
                {
                    animObjectId = pOwner->GetNearestObjectByName(
                        pReplies[i].m_paAnimList[j].m_sParticipant, 0.0f);
                }
                if (g_pAppManager->m_pServerExoApp->GetGameObject(animObjectId) != 0)
                {
                    paU[i][j] = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(animObjectId);
                    paT[i][j] = pReplies[i].m_paAnimList[j].m_nAnimation;
                }
                else
                {
                    CExoString sError;
                    sError.Format(const_cast<char*>("Error: dialogue can't find object '%s'!"),
                                  pReplies[i].m_paAnimList[j].m_sParticipant.CStr());
                    paU[i][j] = OBJECT_INVALID;
                    paT[i][j] = 0;
                }
            }
        }
        paB[i] = listenerId;

        // Commit this line's camera angle; a zero angle means the DLG picked none.
        if (pReplies[i].m_camera.m_nCameraAngle == 0)
        {
            if (m_bFirstReplyLine == 0)
                pReplies[i].m_camera.m_nCameraAngle = nAssignedAngle;
            else
                pReplies[i].m_camera.m_nCameraAngle = 2;
        }
        paC[i] = pReplies[i].m_camera.m_nCameraAngle;
        paD[i] = pReplies[i].m_camera.m_nCameraID;
        paE[i] = pReplies[i].m_camera.m_fHeightOffset;
        paF[i] = pReplies[i].m_camera.m_fTargetHeightOffset;
        paG[i] = pReplies[i].m_nWaitFlags;
        paH[i] = pReplies[i].m_camera.m_nCameraAnimation;
        paI[i] = pReplies[i].m_camera.m_fFieldOfView;
        paJ[i] = pReplies[i].m_camera.m_nCamVidEffect;
        paK[i] = pReplies[i].m_nFadeType;
        paPos[i] = pReplies[i].m_vFadeColor;
        paM[i] = pReplies[i].m_fFadeDelay;
        paN[i] = pReplies[i].m_fFadeLength;
    }

    // Hide the listening objects' equipment for the duration of the lines.
    m_bFirstReplyLine = 0;
    if (m_bUnequipItems != 0)
    {
        for (int i = 0; i < nReplyCount; i++)
        {
            UnequipItemsForDialog(paB[i]);
            for (int64_t j = 0; j < static_cast<int>(paCount[i]); j++)
                UnequipItemsForDialog(paU[i][j]);
        }
    }
    if (m_bUnequipHItem != 0)
    {
        for (int i = 0; i < nReplyCount; i++)
        {
            UnequipHItemForDialog(paB[i]);
            for (int64_t j = 0; j < static_cast<int>(paCount[i]); j++)
                UnequipHItemForDialog(paU[i][j]);
        }
    }

    g_pAppManager->m_pClientExoApp->GetInGameGui()->HandleDialogReplies(
        nPlayerId,
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(objectId),
        nGender, paText, paA, paB, paC, paD, paE, paF, paG, paH, paI, paJ, paK,
        paPos, paM, paN, nParam5, nParam6, nEntry, paCount, paU, paT, bBark);

    delete[] paA;
    delete[] paB;
    delete[] paText;
    delete[] paC;
    delete[] paD;
    delete[] paE;
    delete[] paF;
    delete[] paG;
    delete[] paH;
    delete[] paI;
    delete[] paJ;
    delete[] paK;
    delete[] paPos;
    delete[] paM;
    delete[] paN;
    for (int i = 0; i < nReplyCount; i++)
    {
        delete[] paU[i];
        delete[] paT[i];
    }
    delete[] paU;
    delete[] paT;
    delete[] paCount;
    return 1;
}

int CSWSDialog::HandleReply(unsigned int listenerId, CSWSObject* pObj, unsigned int replyIndex,
                            int isEndDialog, unsigned int nEntry, int flag)
{
    if (isEndDialog == 0 && (m_nHandledReply != NODE_INDEX_NONE || m_nCurrentEntry != nEntry))
        return 1;

    // Locate the replying player. A match ends the listener's conversation outright when this is an
    // end-dialog reply, otherwise it stages the listener's object id and gender for the client GUI
    // below.
    CSWSDialogPlayer* pPlayer = 0;
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
    {
        CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
        pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.m_pInternal->m_pHead->m_pData);
        while (pPlayer != 0)
        {
            if (pPlayer->m_nPlayerId == listenerId)
            {
                if (isEndDialog != 0)
                {
                    CSWSObject* pObject;
                    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                            pPlayer->m_objectId, reinterpret_cast<CGameObject**>(&pObject))
                        == CGameObjectArray::SUCCESS)
                    {
                        if (pObject->AsSWSCreature() != 0)
                            pObject->AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 0, 0);
                        else
                            pObject->StopDialog();
                    }
                    return 1;
                }
                m_oidCurrentPlayer = pPlayer->m_objectId;
                m_nPlayerGender = pPlayer->m_nGender;
                goto located;
            }
            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        }
    }
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        return 1;
located:

    // The reply index is per-entry, so an out-of-range index is a no-op, and so is a listener that is not
    // in the server object array.
    if (replyIndex >= m_paEntries[m_nCurrentEntry].m_nReplyCount)
        return 0;
    CSWSObject* pListenerObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
            m_oidCurrentPlayer, reinterpret_cast<CGameObject**>(&pListenerObject))
        != CGameObjectArray::SUCCESS)
        return 0;

    m_nHandledReply = replyIndex;
    unsigned int nReply = m_paEntries[m_nCurrentEntry].m_paReplies[replyIndex].m_nIndex;

    // Tell every other participant's GUI which reply was chosen, and hand each of them the journal entry
    // and the plot XP the reply carries.
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
    {
        CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
        for (CSWSDialogPlayer* pMember
                 = static_cast<CSWSDialogPlayer*>(m_lstParticipants.m_pInternal->m_pHead->m_pData);
             pMember != 0;
             pMember = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode)))
        {
            if (pMember->m_nPlayerId != listenerId)
            {
                CGuiInGame* pGui;
                CExoString sListener
                    = m_paReplies[m_paEntries[m_nCurrentEntry]
                                      .m_paReplies[m_nHandledReply]
                                      .m_nIndex]
                          .m_camera.m_sListener;
                if (sListener != "" && !sListener.CompareNoCase("OWNER"))
                {
                    if (sListener.CompareNoCase("PLAYER"))
                        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
                    else
                    {
                        CSWSObject* pSpeaker = GetSpeaker(pObj, sListener);
                        K2_UNUSED(pSpeaker);
                    }
                }

                pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                CExoString sText
                    = GetExoStringFromLocString(m_paReplies[nReply].m_sText, m_nPlayerGender);
                pGui->HandleDialogReplyChosen(
                    pMember->m_nPlayerId, replyIndex, nEntry, sText,
                    g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_oidCurrentPlayer),
                    m_nPlayerGender, m_bBark);
            }

            AddJournalEntry(m_paReplies[nReply].m_sQuest, m_paReplies[nReply].m_nQuestEntry,
                            pMember->m_nPlayerId);
#if KOTOR1  // goes through GiveDialogPlotXP
            GiveDialogPlotXP(m_paReplies[nReply].m_nPlotIndex,
                             m_paReplies[nReply].m_fPlotXPPercentage);
#else
            if (m_paReplies[nReply].m_nPlotIndex != -1
                && m_paReplies[nReply].m_fPlotXPPercentage > 0.0f)
            {
                if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GivePlotXP(
                        m_paReplies[nReply].m_nPlotIndex,
                        static_cast<int>(ceilf(m_paReplies[nReply].m_fPlotXPPercentage * 100.0f)));
            }
#endif
        }
    }
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        m_oidLastReplySpeaker = m_oidCurrentPlayer;

    // The reply's own delay. The wait-flag bit selects the DLG's authored "Delay"; otherwise a -1
    // sentinel means "measure the line", falling back to the client's own deferred delay.
    if (flag == 0)
    {
        if (m_paReplies[nReply].m_nWaitFlags & 2)
            SetDialogDelay(pObj, m_paReplies[nReply].m_sText, m_paReplies[nReply].m_nDelay,
                           m_paReplies[nReply].m_nEntryCount != 0, 1);
        else if (m_paReplies[nReply].m_nEstimateDelay == -1)
        {
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut != 0)
                pObj->SetDialogDelay(0.0f);
            else
                SetDialogDelay(pObj, m_paReplies[nReply].m_sText,
                               static_cast<unsigned int>(m_paReplies[nReply].m_sText.GetSpokenTimeEstimate(
#if KOTOR1  // the listener's server-side language
                                   g_pAppManager->m_pServerExoApp->GetPlayerLanguage(
                                       static_cast<unsigned int>(m_oidCurrentPlayer)),
#else
                                   g_pAppManager->m_pClientExoApp->GetClientLanguage(),
#endif
                                   m_nPlayerGender)),
                               m_paReplies[nReply].m_nEntryCount != 0, 1);
        }
        else
            SetDialogDelay(pObj, m_paReplies[nReply].m_sText, 0,
                           m_paReplies[nReply].m_nEntryCount != 0, 1);

        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut != 0)
            pObj->m_dialogWaitFlags = 0;
        else
            pObj->m_dialogWaitFlags = m_paReplies[nReply].m_nWaitFlags;
    }

    // The reply's two action scripts, each with its own parameter block.
#if KOTOR1  // one action script, no parameters
    RunScript(pObj, m_paReplies[nReply].m_rScript);
#else
    g_pVirtualMachine->SetScriptParams(
        m_paReplies[nReply].m_nActionParam1, m_paReplies[nReply].m_nActionParam2,
        m_paReplies[nReply].m_nActionParam3, m_paReplies[nReply].m_nActionParam4,
        m_paReplies[nReply].m_nActionParam5, m_paReplies[nReply].m_sActionParamStrA);
    RunScript(pObj, m_paReplies[nReply].m_rScript);

    g_pVirtualMachine->SetScriptParams(
        m_paReplies[nReply].m_nActionParam1b, m_paReplies[nReply].m_nActionParam2b,
        m_paReplies[nReply].m_nActionParam3b, m_paReplies[nReply].m_nActionParam4b,
        m_paReplies[nReply].m_nActionParam5b, m_paReplies[nReply].m_sActionParamStrB);
    RunScript(pObj, m_paReplies[nReply].m_rScript2);
#endif

    // Finally pick the followed entry whose condition scripts accept. With none left the conversation is
    // over, which is also where an empty follow list lands.
    for (unsigned int i = 0; i < m_paReplies[nReply].m_nEntryCount; i++)
    {
        if (m_paEntries[m_paReplies[nReply].m_paEntries[i].m_nIndex].m_sSpeaker != ""
            && pObj->m_sTag.CompareNoCase(
                   m_paEntries[m_paReplies[nReply].m_paEntries[i].m_nIndex].m_sSpeaker)
                   == 0
            && GetSpeaker(pObj,
                          m_paEntries[m_paReplies[nReply].m_paEntries[i].m_nIndex].m_sSpeaker)
                   == 0)
            continue;

#if KOTOR1  // the link's one condition script decides alone
        if (CheckScript(pObj, m_paReplies[nReply].m_paEntries[i].m_rActive) == 0)
            continue;
#else
        int nResultA;
        int bRanA;
        if (m_paReplies[nReply].m_paEntries[i].m_rActive == const_cast<char*>("") /* idiom-exempt: canonical */)
        {
            nResultA = 1;
            bRanA = 0;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(m_paReplies[nReply].m_paEntries[i].m_nParam1,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam2,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam3,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam4,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam5,
                                               m_paReplies[nReply].m_paEntries[i].m_sParamStrA);
            nResultA = CheckScript(pObj, m_paReplies[nReply].m_paEntries[i].m_rActive);
            if (m_paReplies[nReply].m_paEntries[i].m_bNot != 0)
                nResultA = (nResultA == 0);
            bRanA = 1;
        }

        if (m_paReplies[nReply].m_paEntries[i].m_rActive2 == const_cast<char*>("") /* idiom-exempt: canonical */)
        {
            if (bRanA != 0 && nResultA == 0)
                continue;
        }
        else
        {
            g_pVirtualMachine->SetScriptParams(m_paReplies[nReply].m_paEntries[i].m_nParam1b,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam2b,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam3b,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam4b,
                                               m_paReplies[nReply].m_paEntries[i].m_nParam5b,
                                               m_paReplies[nReply].m_paEntries[i].m_sParamStrB);
            int nResultB = CheckScript(pObj, m_paReplies[nReply].m_paEntries[i].m_rActive2);
            if (m_paReplies[nReply].m_paEntries[i].m_bNot2 != 0)
                nResultB = (nResultB == 0);

            if (bRanA == 0)
            {
                if (nResultB == 0)
                    continue;
            }
            else if (m_paReplies[nReply].m_paEntries[i].m_nLogic == 0)
            {
                if (nResultA == 0 || nResultB == 0)
                    continue;
            }
            else
            {
                if (nResultA == 0 && nResultB == 0)
                    continue;
            }
        }
#endif
        return pObj->SendDialogEntry(m_paReplies[nReply].m_paEntries[i].m_nIndex);
    }

    m_bBark = 1;
    m_bEnded = 1;
    return 0;
}

#if KOTOR1  // no unskippable nodes, alien VO, VO debug trace or StrRef exclusions
int CSWSDialog::SendDialogEntry(CSWSObject* pObj, unsigned int a, unsigned int nEntry,
                                unsigned int& nStatus)
{
    if (a != PLAYER_ID_ALL)
    {
        CExoString sListener(m_paEntries[m_nCurrentEntry].m_camera.m_sListener);
        unsigned long long nListenerId;
        if (sListener != "")
        {
            if (sListener.CompareNoCase("OWNER"))
                nListenerId = pObj->m_objectId;
            else if (sListener.CompareNoCase("PLAYER"))
                nListenerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
            else
            {
                CSWSObject* pListener = GetSpeaker(pObj, sListener);
                nListenerId = pListener != 0 ? pListener->m_objectId : OBJECT_INVALID;
            }
        }
        else
            nListenerId = OBJECT_INVALID;

        int nResult;
        CSWSObject* pSpeaker = pObj;
        if (m_paEntries[m_nCurrentEntry].m_sSpeaker != ""
            && pObj->m_sTag.CompareNoCase(m_paEntries[m_nCurrentEntry].m_sSpeaker) == 0
            && (pSpeaker = GetSpeaker(pObj, m_paEntries[m_nCurrentEntry].m_sSpeaker)) == 0)
        {
            nResult = 0;
        }
        else
        {
            SendDialogEntryNode(a, pObj->m_objectId, pSpeaker->m_objectId, nListenerId,
                                m_oidCurrentPlayer, m_nPlayerGender, pObj, m_paEntries[m_nCurrentEntry],
                                nStatus);
            if (nStatus == 10 || m_bRepliesPending == 0)
                nResult = 1;
            else
                nResult = SendDialogReplies(pObj, a);
        }
        return nResult;
    }

    if (m_nEntryCount <= nEntry)
        return 0;

    CSWSObject* pSpeaker = pObj;
    if (m_paEntries[nEntry].m_sSpeaker != ""
        && pObj->m_sTag.CompareNoCase(m_paEntries[nEntry].m_sSpeaker) == 0
        && (pSpeaker = GetSpeaker(pObj, m_paEntries[nEntry].m_sSpeaker)) == 0)
        return 0;

    CExoString sListener(m_paEntries[nEntry].m_camera.m_sListener);
    unsigned long long nListenerId;
    if (sListener != "")
    {
        if (sListener.CompareNoCase("OWNER"))
            nListenerId = pObj->m_objectId;
        else if (sListener.CompareNoCase("PLAYER"))
            nListenerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
        else
        {
            CSWSObject* pListener = GetSpeaker(pObj, sListener);
            nListenerId = pListener != 0 ? pListener->m_objectId : OBJECT_INVALID;
        }
    }
    else
        nListenerId = OBJECT_INVALID;

    unsigned int* paPlayerIds = 0;
    m_bRepliesPending = 0;
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
    {
        CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
        CSWSDialogPlayer* pPlayer = 0;
        if (m_lstParticipants.m_pInternal->m_pHead != 0)
            pPlayer = static_cast<CSWSDialogPlayer*>(pNode->m_pData);
        while (pPlayer != 0)
        {
            SendDialogEntryNode(pPlayer->m_nPlayerId, pObj->m_objectId, pSpeaker->m_objectId,
                                nListenerId, m_oidCurrentPlayer, m_nPlayerGender, pObj,
                                m_paEntries[nEntry], nStatus);
            if (nStatus == 10)
                return 1;
            AddJournalEntry(m_paEntries[nEntry].m_sQuest, m_paEntries[nEntry].m_nQuestEntry,
                            pPlayer->m_nPlayerId);
            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        }

        paPlayerIds = new unsigned int[m_lstParticipants.m_pInternal->m_nCount];
        pNode = m_lstParticipants.m_pInternal->m_pHead;
        pPlayer = 0;
        if (m_lstParticipants.m_pInternal->m_pHead != 0)
            pPlayer = static_cast<CSWSDialogPlayer*>(pNode->m_pData);
        for (unsigned int i = 0; pPlayer != 0; i++)
        {
            paPlayerIds[i] = pPlayer->m_nPlayerId;
            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        }
    }

    GiveDialogPlotXP(m_paEntries[nEntry].m_nPlotIndex, m_paEntries[nEntry].m_fPlotXPPercentage);

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    int nVORejected = m_paEntries[nEntry].m_bVOResRefUnused;
    EvaluateAchievement(m_paEntries[nEntry].m_sText.m_nStrRef);

    // The Endar Spire tutorial, Trask's lines, and Carth's first line queue help popups.
    if (m_paEntries[nEntry].m_sSpeaker == "end_trask")
    {
        switch (nEntry)
        {
            case 40:
            case 43:
                QueueHelpGui(6);
                break;
            case 46:
            case 49:
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_btnEquip
                    .SetActive(1);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_btnEquip
                    .m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
                QueueHelpGui(5);
                break;
            case 55:
                QueueHelpGui(4);
                break;
        }
    }
    if (m_paEntries[nEntry].m_sSpeaker == "Carth" && nEntry == 1
        && m_paEntries[nEntry].m_sText.m_nStrRef == STRREF_CARTH_ESCAPE_POD)
        QueueHelpGui(7);

    int bPlaySound = g_pAppManager->m_pClientExoApp->GetInGameGui()->HandleDialogSound(
        pClientExoApp->ServerToClientObjectId(pSpeaker->m_objectId),
        m_paEntries[nEntry].m_rVO_ResRef);

    if (g_pAppManager->m_pClientExoApp->GetModuleResourceLoaded(8) == 1)
    {
        if (g_bDisableSound == 0)
            bPlaySound = m_paEntries[nEntry].m_nSoundExists & 1;
    }
    else
    {
        if (g_bDisableSound == 0)
        {
            CExoString sSound;
            m_paEntries[nEntry].m_rVO_ResRef.CopyToString(sSound);
            if ((sSound[0] == 'n' || sSound[0] == 'N') && sSound.GetLength() == 16
                && sSound[1] != '_')
            {
                char szModule[6];
                szModule[0] = sSound[1];
                szModule[1] = sSound[2];
                szModule[2] = sSound[3];
                szModule[3] = sSound[4];
                szModule[4] = sSound[5];
                szModule[5] = 0;
                char szSpeaker[7];
                szSpeaker[0] = sSound[6];
                szSpeaker[1] = sSound[7];
                szSpeaker[2] = sSound[8];
                szSpeaker[3] = sSound[9];
                szSpeaker[4] = sSound[10];
                szSpeaker[5] = sSound[11];
                szSpeaker[6] = 0;
                sSound.Format(const_cast<char*>("HD0:STREAMWAVES\\%s\\%s\\%s"), szModule, szSpeaker,
                              sSound.CStr());
            }
            else
                sSound.Format(const_cast<char*>("HD0:STREAMWAVES\\%s"), sSound.CStr());

            CExoString sResolved;
            sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sSound, 8);
            FILE* pFile = _fopen(
                "E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/servercore/nwsdialog.cpp",
                "SendDialogEntry", 2767, sResolved.CStr(), "r");
            if (pFile == 0)
            {
                sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sSound, 4);
                pFile = _fopen(
                    "E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/servercore/nwsdialog.cpp",
                    "SendDialogEntry", 2771, sResolved.CStr(), "r");
            }
            if (pFile != 0)
            {
                fclose(pFile);
                bPlaySound = 1;
            }
            else
                bPlaySound = 0;
        }

        if (bPlaySound == 0)
        {
            CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
            if ((nVORejected | pOptions->m_bFeedback2) == 0)
            {
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsCameraDialog() != 0)
                {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText = 1;
                    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogScrollDoneFlag != 0)
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDialogMessage();
                }
            }
            bPlaySound = 0;
        }
    }

    if (paPlayerIds != 0)
        delete[] paPlayerIds;

    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        m_oidLastEntrySpeaker = pSpeaker->m_objectId;

    if (bPlaySound == 0 && (m_paEntries[nEntry].m_nWaitFlags & CSWSDialogBase::WAIT_FLAG_FIXED_DELAY) == 0)
    {
        if (m_paEntries[nEntry].m_nEstimateDelay == -1)
        {
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut != 0)
                pObj->SetDialogDelay(0.0f);
            else
                SetDialogDelay(pObj, m_paEntries[nEntry].m_sText,
                               static_cast<unsigned int>(m_paEntries[nEntry].m_sText.GetSpokenTimeEstimate(
                                   g_pAppManager->m_pServerExoApp->GetPlayerLanguage(
                                       static_cast<unsigned int>(m_oidCurrentPlayer)),
                                   m_nPlayerGender)),
                               m_paEntries[nEntry].m_nReplyCount != 0, 0);
        }
        else
            SetDialogDelay(pObj, m_paEntries[nEntry].m_sText, 0,
                           m_paEntries[nEntry].m_nReplyCount != 0, 0);
    }
    else
        SetDialogDelay(pObj, m_paEntries[nEntry].m_sText, m_paEntries[nEntry].m_nDelay,
                       m_paEntries[nEntry].m_nReplyCount != 0, 0);

    pObj->m_dialogWaitFlags = m_paEntries[nEntry].m_nWaitFlags;
    RunScript(pObj, m_paEntries[nEntry].m_rScript);

    m_nCurrentEntry = nEntry;
    return pObj->SendDialogReplies();
}
#else
int CSWSDialog::SendDialogEntry(CSWSObject* pObj, unsigned int a, unsigned int nEntry,
                                unsigned int& nStatus)
{
    if (a != PLAYER_ID_ALL)
    {
        // The current entry, to one participant.
        CExoString sListener(m_paEntries[m_nCurrentEntry].m_camera.m_sListener);
        unsigned long long nListenerId;
        if (sListener != "")
        {
            if (sListener.CompareNoCase("OWNER"))
                nListenerId = pObj->m_objectId;
            else if (sListener.CompareNoCase("PLAYER"))
                nListenerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
            else
            {
                // The listener is spelled as a speaker name.
                CSWSObject* pListener = GetSpeaker(pObj, sListener);
                nListenerId = pListener != 0 ? pListener->m_objectId : OBJECT_INVALID;
            }
        }
        else
            nListenerId = OBJECT_INVALID;

        int nResult;
        CSWSObject* pSpeaker = pObj;
        if (m_paEntries[m_nCurrentEntry].m_sSpeaker != ""
            && pObj->m_sTag.CompareNoCase(m_paEntries[m_nCurrentEntry].m_sSpeaker) == 0
            && (pSpeaker = GetSpeaker(pObj, m_paEntries[m_nCurrentEntry].m_sSpeaker)) == 0)
        {
            nResult = 0;
        }
        else
        {
            m_bNodeUnskippable = m_paEntries[m_nCurrentEntry].m_nNodeUnskippable;
            SendDialogEntryNode(a, pObj->m_objectId, pSpeaker->m_objectId, nListenerId,
                                m_oidCurrentPlayer, m_nPlayerGender, pObj, m_paEntries[m_nCurrentEntry],
                                nStatus);
            // Status 10 means the client has deferred this entry and is holding onto it.
            if (nStatus == 10 || m_bRepliesPending == 0)
                nResult = 1;
            else
                nResult = SendDialogReplies(pObj, a);
        }
        return nResult;
    }

    if (m_nEntryCount <= nEntry)
        return 0;

    CSWSObject* pSpeaker = pObj;
    if (m_paEntries[nEntry].m_sSpeaker != ""
        && pObj->m_sTag.CompareNoCase(m_paEntries[nEntry].m_sSpeaker) == 0
        && (pSpeaker = GetSpeaker(pObj, m_paEntries[nEntry].m_sSpeaker)) == 0)
        return 0;

    CExoString sListener(m_paEntries[nEntry].m_camera.m_sListener);
    unsigned long long nListenerId;
    if (sListener != "")
    {
        if (sListener.CompareNoCase("OWNER"))
            nListenerId = pObj->m_objectId;
        else if (sListener.CompareNoCase("PLAYER"))
            nListenerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
        else
        {
            CSWSObject* pListener = GetSpeaker(pObj, sListener);
            nListenerId = pListener != 0 ? pListener->m_objectId : OBJECT_INVALID;
        }
    }
    else
        nListenerId = OBJECT_INVALID;

    // Push the entry node to every participant, then snapshot their player ids. The snapshot is what the
    // script parameters below are staged with.
    unsigned int* paPlayerIds = 0;
    m_bRepliesPending = 0;
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
    {
        CExoLinkedListNode* pNode = m_lstParticipants.m_pInternal->m_pHead;
        CSWSDialogPlayer* pPlayer = 0;
        if (m_lstParticipants.m_pInternal->m_pHead != 0)
            pPlayer = static_cast<CSWSDialogPlayer*>(pNode->m_pData);
        while (pPlayer != 0)
        {
            SendDialogEntryNode(pPlayer->m_nPlayerId, pObj->m_objectId, pSpeaker->m_objectId,
                                nListenerId, m_oidCurrentPlayer, m_nPlayerGender, pObj,
                                m_paEntries[nEntry], nStatus);
            if (nStatus == 10)
                return 1;
            AddJournalEntry(m_paEntries[nEntry].m_sQuest, m_paEntries[nEntry].m_nQuestEntry,
                            pPlayer->m_nPlayerId);
            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        }

        paPlayerIds = new unsigned int[m_lstParticipants.m_pInternal->m_nCount];
        pNode = m_lstParticipants.m_pInternal->m_pHead;
        pPlayer = 0;
        if (m_lstParticipants.m_pInternal->m_pHead != 0)
            pPlayer = static_cast<CSWSDialogPlayer*>(pNode->m_pData);
        for (unsigned int i = 0; pPlayer != 0; i++)
        {
            paPlayerIds[i] = pPlayer->m_nPlayerId;
            pPlayer = static_cast<CSWSDialogPlayer*>(m_lstParticipants.GetNext(pNode));
        }
    }

    // The entry's plot XP goes to the whole party.
    if (m_paEntries[nEntry].m_nPlotIndex != -1
        && m_paEntries[nEntry].m_fPlotXPPercentage > 0.0f)
    {
        if (g_pAppManager->m_pServerExoApp->GetPartyTable() != 0)
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GivePlotXP(
                m_paEntries[nEntry].m_nPlotIndex,
                static_cast<int>(ceilf(m_paEntries[nEntry].m_fPlotXPPercentage * 100.0f)));
    }

    // A VO resref that names an alien-race variation gets its index appended, once.
    if ((m_paEntries[nEntry].m_nSoundExists & 6) == 2)
    {
        CExoString sVariation;
        sVariation.Format(const_cast<char*>("%d"), m_paEntries[nEntry].m_camera.m_nAlienVOVariation);
        m_paEntries[nEntry].m_rVO_ResRef += sVariation;
        m_paEntries[nEntry].m_nSoundExists |= 4;
    }

    // The debug VO trace is three separate toggles on the same global, each bracketing one
    // sound-mode read, and the middle pair also ducks the engine back to the generic sound mode.
    if (g_bVODebugEnabled)
        g_pExoSound->GetSoundMode();
    if (g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_GENERIC)
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
    if (g_bVODebugEnabled)
        g_pExoSound->GetSoundMode();
    if (g_bVODebugEnabled)
    {
        char szSound[20];
        m_paEntries[nEntry].m_rVO_ResRef.CopyToString(szSound);
        g_pExoSound->GetSoundMode();
    }

    // The StrRefs whose lines the client must not be told to play.
    int nStrRef = static_cast<int>(m_paEntries[nEntry].m_sText.m_nStrRef);
    int nVORejected = m_paEntries[nEntry].m_bVOResRefUnused;
    switch (nStrRef)
    {
        case STRREF_TUTORIAL_PARTY_MEMBER: case STRREF_TUTORIAL_DIALOG: case STRREF_TUTORIAL_NEW_QUESTS: case STRREF_TUTORIAL_LOCKER_KEYCARD: case STRREF_TUTORIAL_TARGETING: case STRREF_TUTORIAL_EQUIP_WEAPON:
        case STRREF_TUTORIAL_SOLO_MODE: case STRREF_TUTORIAL_DROID_FLAMETHROWER: case STRREF_TUTORIAL_LOCKER_KEYCARD_2: case STRREF_TUTORIAL_DROID_FLAMETHROWER_2: case STRREF_TUTORIAL_REPAIR_KIT: case STRREF_TUTORIAL_BASH:
        case STRREF_TUTORIAL_PLOT_COURSE: case STRREF_TUTORIAL_MAP: case STRREF_TUTORIAL_MOVE_T3M4: case STRREF_TUTORIAL_TARGET_OBJECTS:
            break;
        default:
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HandleDialogSound(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pSpeaker->m_objectId),
                m_paEntries[nEntry].m_rVO_ResRef);
            break;
    }

    CExoString aPaths[MAX_OVERRIDE_PATHS];

    int bPlaySound = 0;
    if (g_pAppManager->m_pClientExoApp->GetModuleResourceLoaded(8) == 1)
    {
        if (g_bDisableSound == 0)
            bPlaySound = m_paEntries[nEntry].m_nSoundExists & 1;
    }
    else
    {
        if (g_bDisableSound == 0)
        {
            // Build the override-path candidate list for the VO.
            CExoString sSound;
            m_paEntries[nEntry].m_rVO_ResRef.CopyToString(sSound);
            int nLen;
            if (sSound.m_sString == 0 || (nLen = static_cast<int>(strlen(sSound.m_sString))) == 0)
                sSound = "HD0:STREAMVOICE\\";
            else
            {
                // The first three characters are the language code, a real one or a number.
                // Anything else is not a streamed VO at all.
                char szLang[4];
                szLang[0] = sSound[0];
                szLang[1] = sSound[1];
                szLang[2] = sSound[2];
                szLang[3] = 0;
                int nLang = atoi(szLang);
                if (_stricmp(szLang, "gbl") == 0 || _stricmp(szLang, "avo") == 0
                    || _stricmp(szLang, "000") == 0 || nLang > 0)
                {
                    // The candidate filename drops the language prefix and the trailing three characters, the
                    // ".wav" stem's own suffix.
                    char szSuffix[32];
                    uint64_t nTail = static_cast<int64_t>(nLen) - 3;
                    if (nTail < 4)
                        nTail = 3;
                    else
                        for (int i = 0; static_cast<uint64_t>(i) < nTail - 3; i++)
                            szSuffix[i] = sSound[i + 3];
                    szSuffix[nTail - 3] = 0;

                    for (int i = 0; i < MAX_OVERRIDE_PATHS; i++)
                    {
                        const char* pszOverride = GetOverridePath(i, true);
                        if (pszOverride == 0)
                            break;
                        char szPath[256];
                        FormatPath(szPath, sizeof(szPath), const_cast<char*>("%s\\streamvoice\\%s\\%s\\%s"),
                                   pszOverride, szLang, szSuffix, sSound.CStr());
                        aPaths[i] = szPath;
                    }
                    sSound.Format(const_cast<char*>("HD0:STREAMVOICE\\%s\\%s\\%s"), szLang, szSuffix,
                                  sSound.CStr());
                }
                else
                    sSound.Format(const_cast<char*>("HD0:STREAMVOICE\\%s"), sSound.CStr());
            }

            // The first candidate the alias list can resolve and open is the VO.
            CExoString sResolved;
            int bFound = 0;
            for (int i = 0; i < MAX_OVERRIDE_PATHS; i++)
            {
                if (aPaths[i] != "")
                {
                    sResolved = g_pExoBase->m_pAliasList->ResolveFileName(aPaths[i], 8);
                    FILE* pFile = fopen(sResolved.CStr(), "r");
                    if (pFile == 0)
                    {
                        sResolved = g_pExoBase->m_pAliasList->ResolveFileName(aPaths[i], 4);
                        pFile = fopen(sResolved.CStr(), "r");
                        if (pFile == 0)
                            continue;
                    }
                    fclose(pFile);
                    bFound = 1;
                }
                else
                    break;
            }
            if (bFound)
                bPlaySound = 1;
            else
            {
                // With nothing on disk, fall back to the single HD0 form before giving up.
                sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sSound, 8);
                FILE* pFile = fopen(sResolved.CStr(), "r");
                if (pFile == 0)
                {
                    sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sSound, 4);
                    pFile = fopen(sResolved.CStr(), "r");
                    if (pFile == 0)
                        goto textOnly;
                }
                fclose(pFile);
                bPlaySound = 1;
            }
        }
        else
        {
        textOnly:
            // No VO will play: hand the client the line as text-only, unless the user asked for no feedback
            // and the camera is not the dialog's own.
            CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
            if ((nVORejected | pOptions->m_bFeedback2) == 0)
            {
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsCameraDialog() != 0)
                {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText = 1;
                    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogScrollDoneFlag != 0)
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDialogMessage();
                }
            }
        }
    }

    if (paPlayerIds != 0)
        delete[] paPlayerIds;

    // The participant list re-seeds the dialog's current-listener id.
    if (m_lstParticipants.m_pInternal->m_pHead != 0)
        m_oidLastEntrySpeaker = pSpeaker->m_objectId;

    // The pause before the line is spoken.
    if (bPlaySound == 0 && (m_paEntries[nEntry].m_nWaitFlags & CSWSDialogBase::WAIT_FLAG_FIXED_DELAY) == 0)
    {
        if (m_paEntries[nEntry].m_nEstimateDelay == -1)
        {
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut != 0)
                pObj->SetDialogDelay(0.0f);
            else
            {
                unsigned int nTime;
                if (pObj->m_sTag.CompareNoCase("info_term") == 0)
                    nTime = static_cast<unsigned int>(m_paEntries[nEntry].m_sText.GetSpokenTimeEstimate(
                        g_pAppManager->m_pClientExoApp->GetClientLanguage(), m_nPlayerGender));
                else
                    nTime = 0;
                SetDialogDelay(pObj, m_paEntries[nEntry].m_sText, nTime,
                               m_paEntries[nEntry].m_nReplyCount != 0, 0);
            }
        }
        else
            SetDialogDelay(pObj, m_paEntries[nEntry].m_sText, 0,
                           m_paEntries[nEntry].m_nReplyCount != 0, 0);
    }
    else
        SetDialogDelay(pObj, m_paEntries[nEntry].m_sText, m_paEntries[nEntry].m_nDelay,
                       m_paEntries[nEntry].m_nReplyCount != 0, 0);

    // The entry's two action scripts, each with its own parameter block.
    pObj->m_dialogWaitFlags = m_paEntries[nEntry].m_nWaitFlags;
    g_pVirtualMachine->SetScriptParams(
        m_paEntries[nEntry].m_nActionParam1, m_paEntries[nEntry].m_nActionParam2,
        m_paEntries[nEntry].m_nActionParam3, m_paEntries[nEntry].m_nActionParam4,
        m_paEntries[nEntry].m_nActionParam5, m_paEntries[nEntry].m_sActionParamStrA);
    RunScript(pObj, m_paEntries[nEntry].m_rScript);

    g_pVirtualMachine->SetScriptParams(
        m_paEntries[nEntry].m_nActionParam1b, m_paEntries[nEntry].m_nActionParam2b,
        m_paEntries[nEntry].m_nActionParam3b, m_paEntries[nEntry].m_nActionParam4b,
        m_paEntries[nEntry].m_nActionParam5b, m_paEntries[nEntry].m_sActionParamStrB);
    RunScript(pObj, m_paEntries[nEntry].m_rScript2);

    m_bNodeUnskippable = m_paEntries[nEntry].m_nNodeUnskippable;
    m_nCurrentEntry = nEntry;
    return pObj->SendDialogReplies();
}
#endif
